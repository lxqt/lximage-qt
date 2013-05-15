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

using namespace LxImage;

LoadImageJob::LoadImageJob(MainWindow* window, FmPath* filePath):
  cancellable_(g_cancellable_new()),
  path_(fm_path_ref(filePath)),
  mainWindow_(window),
  error_(NULL) {
}

LoadImageJob::~LoadImageJob() {
  g_object_unref(cancellable_);
  fm_path_unref(path_);
  if(error_)
    g_error_free(error_);
}

void LoadImageJob::start() {
  g_io_scheduler_push_job(GIOSchedulerJobFunc(loadImageThread),
                          this, GDestroyNotify(freeMe),
                          G_PRIORITY_DEFAULT, cancellable_);
}

// This is called from the worker thread, not main thread
gboolean LoadImageJob::loadImageThread(GIOSchedulerJob* job, GCancellable* cancellable, LoadImageJob* pThis) {
  GFile* gfile = fm_path_to_gfile(pThis->path_);
  GFileInputStream* fileStream = g_file_read(gfile, pThis->cancellable_, &pThis->error_);
  g_object_unref(gfile);

  if(fileStream) { // if the file stream is successfually opened
    QBuffer imageBuffer;
    GInputStream* inputStream = G_INPUT_STREAM(fileStream);
    while(!g_cancellable_is_cancelled(pThis->cancellable_)) {
      char buffer[4096];
      gssize readSize = g_input_stream_read(inputStream, 
                                            buffer, 4096,
                                            pThis->cancellable_, &pThis->error_);
      if(readSize == -1 || readSize == 0) // error or EOF
        break;
      // append the bytes read to the image buffer
        imageBuffer.buffer().append(buffer, readSize);
    }
    g_input_stream_close(inputStream, NULL, NULL);
    
    // FIXME: utilize libexif to apply orientation flag if it's available here.
    
    // FIXME: maybe it's a better idea to implement a GInputStream based QIODevice.
    if(!pThis->error_ && !g_cancellable_is_cancelled(pThis->cancellable_)) // load the image from buffer if there are no errors
      pThis->image_ = QImage::fromData(imageBuffer.buffer());
  }
  // do final step in the main thread
  if(!g_cancellable_is_cancelled(pThis->cancellable_))
    g_io_scheduler_job_send_to_mainloop(job, GSourceFunc(onImageLoaded), pThis, NULL);
  return FALSE;
}

// this function is called from main thread only
gboolean LoadImageJob::onImageLoaded(LoadImageJob* pThis) {
  // only do processing if the job is not cancelled
  if(!g_cancellable_is_cancelled(pThis->cancellable_)) {
    pThis->mainWindow_->onImageLoaded(pThis);
  }
  return TRUE;
}

void LoadImageJob::freeMe(LoadImageJob* pThis) {
  delete pThis;
}
