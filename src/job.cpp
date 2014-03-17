/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2014  <copyright holder> <email>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "job.h"

namespace LxImage {

Job::Job():
  cancellable_(g_cancellable_new()),
  error_(NULL) {
}

Job::~Job() {
  g_object_unref(cancellable_);
  if(error_)
    g_error_free(error_);
}

// This is called from the worker thread, not main thread
gboolean Job::_jobThread(GIOSchedulerJob* job, GCancellable* cancellable, Job* pThis) {
  bool ret = pThis->run();
  // do final step in the main thread
  if(!g_cancellable_is_cancelled(pThis->cancellable_))
    g_io_scheduler_job_send_to_mainloop(job, GSourceFunc(_finish), pThis, NULL);
  return FALSE;
}

void Job::start() {
  g_io_scheduler_push_job(GIOSchedulerJobFunc(_jobThread),
                          this, GDestroyNotify(_freeMe),
                          G_PRIORITY_DEFAULT, cancellable_);
}

// this function is called from main thread only
gboolean Job::_finish(Job* pThis) {
  // only do processing if the job is not cancelled
  if(!g_cancellable_is_cancelled(pThis->cancellable_)) {
    pThis->finish();
  }
  return TRUE;
}

void Job::_freeMe(Job* pThis) {
  delete pThis;
}

}
