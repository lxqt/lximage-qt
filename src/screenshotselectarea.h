/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2013  <copyright holder> <email>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef LXIMAGE_SCREENSHOTDIALOG_SELECT_AREA_H
#define LXIMAGE_SCREENSHOTDIALOG_SELECT_AREA_H

#include "screenshotselectareagraphicsview.h"
#include <QDialog>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>


namespace LxImage {

class ScreenshotSelectArea : public QDialog {
  Q_OBJECT
public:
  ScreenshotSelectArea(const QImage & image, QWidget* parent = 0);
  QRect selectedArea();

private Q_SLOTS:
  void areaSelected(QRect rect);
  
private:
  QGraphicsScene *scene_;
  ScreenshotSelectAreaGraphicsView *view_;
  QRect selectedRect_;
};

}

#endif // LXIMAGE_SCREENSHOTDIALOG_SELECT_AREA_H
