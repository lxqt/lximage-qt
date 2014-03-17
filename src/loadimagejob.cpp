/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  PCMan <email>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#include "loadimagejob.h"
#include "mainwindow.h"
#include <QImageReader>
#include <QBuffer>
#include <qvarlengtharray.h>
#include <libexif/exif-loader.h>

using namespace LxImage;

LoadImageJob::LoadImageJob(MainWindow* window, FmPath* filePath):
  Job(),
  path_(fm_path_ref(filePath)),
  mainWindow_(window) {
}

LoadImageJob::~LoadImageJob() {
  fm_path_unref(path_);
}

// This is called from the worker thread, not main thread
bool LoadImageJob::run() {
  GFile* gfile = fm_path_to_gfile(path_);
  GFileInputStream* fileStream = g_file_read(gfile, cancellable_, &error_);
  g_object_unref(gfile);

  if(fileStream) { // if the file stream is successfually opened
    QBuffer imageBuffer;
    GInputStream* inputStream = G_INPUT_STREAM(fileStream);
    while(!g_cancellable_is_cancelled(cancellable_)) {
      char buffer[4096];
      gssize readSize = g_input_stream_read(inputStream, 
                                            buffer, 4096,
                                            cancellable_, &error_);
      if(readSize == -1 || readSize == 0) // error or EOF
        break;
      // append the bytes read to the image buffer
        imageBuffer.buffer().append(buffer, readSize);
    }
    g_input_stream_close(inputStream, NULL, NULL);
    
    // FIXME: maybe it's a better idea to implement a GInputStream based QIODevice.
    if(!error_ && !g_cancellable_is_cancelled(cancellable_)) { // load the image from buffer if there are no errors
      image_ = QImage::fromData(imageBuffer.buffer());

      if(!image_.isNull()) { // if the image is loaded correctly
        // check if this file is a jpeg file
        // FIXME: can we use FmFileInfo instead if it's available?
        const char* basename = fm_path_get_basename(path_);
        char* mime_type = g_content_type_guess(basename, NULL, 0, NULL);
        if(mime_type && strcmp(mime_type, "image/jpeg") == 0) { // this is a jpeg file
          // use libexif to extract additional info embedded in jpeg files
          ExifLoader *exif_loader = exif_loader_new();
          // write image data to exif loader
          int ret = exif_loader_write(exif_loader, (unsigned char*)imageBuffer.data().constData(), (unsigned int)imageBuffer.size());
          ExifData *exif_data = exif_loader_get_data(exif_loader);
          exif_loader_unref(exif_loader);
          if(exif_data) {
            /* reference for EXIF orientation tag:
            * http://www.impulseadventure.com/photo/exif-orientation.html */
            ExifEntry* orient_ent = exif_data_get_entry(exif_data, EXIF_TAG_ORIENTATION);
            if(orient_ent) { /* orientation flag found in EXIF */
              gushort orient;
              ExifByteOrder bo = exif_data_get_byte_order(exif_data);
              /* bo == EXIF_BYTE_ORDER_INTEL ; */
              orient = exif_get_short (orient_ent->data, bo);
              qreal rotate_degrees = 0.0;
              switch(orient) {
                case 1: /* no rotation */
                  break;
                case 8:
                  rotate_degrees = 270.0;
                  break;
                case 3:
                  rotate_degrees = 180.0;
                  break;
                case 6:
                  rotate_degrees = 90.0;
                  break;
              }
              // rotate the image according to EXIF orientation tag
              if(rotate_degrees != 0.0) {
                QTransform transform;
                transform.rotate(rotate_degrees);
                image_ = image_.transformed(transform, Qt::SmoothTransformation);
              }
              // TODO: handle other EXIF tags as well
            }
            exif_data_unref(exif_data);
          }
        }
        g_free(mime_type);
      }
    }
  }
  return false;
}

// this function is called from main thread only
void LoadImageJob::finish() {
  // only do processing if the job is not cancelled
  if(!g_cancellable_is_cancelled(cancellable_)) {
    mainWindow_->onImageLoaded(this);
  }
}
