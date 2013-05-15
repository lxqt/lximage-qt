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

SaveImageJob::SaveImageJob(MainWindow* window, FmPath* filePath):
  cancellable_(g_cancellable_new()),
  path_(fm_path_ref(filePath)),
  mainWindow_(window),
  image_(window->image()),
  error_(NULL) {
}

SaveImageJob::~SaveImageJob() {
  g_object_unref(cancellable_);
  fm_path_unref(path_);
  if(error_)
    g_error_free(error_);
}

void SaveImageJob::start() {
  g_io_scheduler_push_job(GIOSchedulerJobFunc(saveImageThread),
                          this, GDestroyNotify(freeMe),
                          G_PRIORITY_DEFAULT, cancellable_);
}

// This is called from the worker thread, not main thread
gboolean SaveImageJob::saveImageThread(GIOSchedulerJob* job, GCancellable* cancellable, SaveImageJob* pThis) {
  GFile* gfile = fm_path_to_gfile(pThis->path_);
  GFileOutputStream* fileStream = g_file_replace(gfile, NULL, false,G_FILE_CREATE_PRIVATE, pThis->cancellable_, &pThis->error_);
  g_object_unref(gfile);

  if(fileStream) { // if the file stream is successfually opened
    const char* format = fm_path_get_basename(pThis->path_);
    format = strrchr(format, '.');
    if(format) // use filename extension as the image format
      ++format;

    QBuffer imageBuffer;
    pThis->image_.save(&imageBuffer, format); // save the image to buffer
    GOutputStream* outputStream = G_OUTPUT_STREAM(fileStream);
    g_output_stream_write_all(outputStream,
                              imageBuffer.data().constData(),
                              imageBuffer.size(),
                              NULL,
                              pThis->cancellable_,
                              &pThis->error_);
    g_output_stream_close(outputStream, NULL, NULL);
  }
  // do final step in the main thread
  if(!g_cancellable_is_cancelled(pThis->cancellable_))
    g_io_scheduler_job_send_to_mainloop(job, GSourceFunc(finish), pThis, NULL);
  return FALSE;
}

// this function is called from main thread only
gboolean SaveImageJob::finish(SaveImageJob* pThis) {
  // only do processing if the job is not cancelled
  if(!g_cancellable_is_cancelled(pThis->cancellable_)) {
    pThis->mainWindow_->onImageSaved(pThis);
  }
  return TRUE;
}

void SaveImageJob::freeMe(SaveImageJob* pThis) {
  delete pThis;
}
