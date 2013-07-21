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


#include "preferencesdialog.h"
#include "application.h"

using namespace LxImage;

PreferencesDialog::PreferencesDialog(QWidget* parent):
  QDialog(parent) {
  ui.setupUi(this);

  Application* app = static_cast<Application*>(qApp);
  Settings& settings = app->settings();

  app->addWindow();
}

PreferencesDialog::~PreferencesDialog() {
  Application* app = static_cast<Application*>(qApp);
  app->removeWindow();
}

void PreferencesDialog::accept() {
  Application* app = static_cast<Application*>(qApp);
  Settings& settings = app->settings();
  
  
  
  settings.save();
  QDialog::accept();
}
