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


#include "saveimagejob.h"
#include "mainwindow.h"
#include <QImageReader>
#include <QBuffer>
#include <qvarlengtharray.h>

using namespace LxImage;

SaveImageJob::SaveImageJob(MainWindow* window, const Fm::FilePath & filePath):
  mainWindow_{window},
  path_{filePath},
  image_{window->image()} {
}

SaveImageJob::~SaveImageJob() {
}

// This is called from the worker thread, not main thread
void SaveImageJob::exec() {
  //TODO: the error handling (emitError() etc...) how to?
  GError * error = nullptr;
  GFileOutputStream* fileStream = g_file_replace(path_.gfile().get(), NULL, false, G_FILE_CREATE_NONE, cancellable().get(), &error);

  if(fileStream) { // if the file stream is successfually opened
    const Fm::CStrPtr f = path_.baseName();
    char const * format = f.get();
    format = strrchr(format, '.');
    if(format) // use filename extension as the image format
      ++format;

    QBuffer imageBuffer;
    image_.save(&imageBuffer, format); // save the image to buffer
    GOutputStream* outputStream = G_OUTPUT_STREAM(fileStream);
    g_output_stream_write_all(outputStream,
                              imageBuffer.data().constData(),
                              imageBuffer.size(),
                              NULL,
                              cancellable().get(),
                              &error);
    g_output_stream_close(outputStream, NULL, NULL);
  }
}
