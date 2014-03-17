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

#ifndef LXIMAGE_JOB_H
#define LXIMAGE_JOB_H

#include <gio/gio.h>

namespace LxImage {

class Job {
public:
  Job();
  virtual ~Job();

  void cancel() {
    g_cancellable_cancel(cancellable_);
  }
  void start();

  GError* error() const {
    return error_;
  }

  bool isCancelled() const {
    return bool(g_cancellable_is_cancelled(cancellable_));
  }

protected:
  virtual bool run() = 0;
  virtual void finish() = 0;

protected:
  GCancellable* cancellable_;
  GError* error_;

private:
  static gboolean _jobThread(GIOSchedulerJob* job, GCancellable* cancellable, Job* pThis);
  static gboolean _finish(Job* pThis);
  static void _freeMe(Job* pThis);
};

}

#endif // LXIMAGE_JOB_H
