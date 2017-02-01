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


#include "screenshotselectarea.h"
#include <QMouseEvent>

using namespace LxImage;

ScreenshotSelectArea::ScreenshotSelectArea(const QImage & image, QWidget* parent) : QDialog(parent)
{
  scene_ = new QGraphicsScene(this);
  scene_->addPixmap(QPixmap::fromImage(image));
  
  view_ = new ScreenshotSelectAreaGraphicsView(scene_, this);
  view_->setRenderHints( QPainter::Antialiasing );
  view_->setHorizontalScrollBarPolicy ( Qt::ScrollBarAlwaysOff );
  view_->setVerticalScrollBarPolicy ( Qt::ScrollBarAlwaysOff );
  view_->show();
  view_->move(0,0);
  view_->resize(image.width(), image.height());
  setWindowState(windowState() | Qt::WindowFullScreen);
  connect(view_, &ScreenshotSelectAreaGraphicsView::selectedArea, this, &ScreenshotSelectArea::areaSelected);
}

QRect ScreenshotSelectArea::selectedArea()
{
  return selectedRect_;
}

void ScreenshotSelectArea::areaSelected(QRect rect)
{
  this->selectedRect_ = rect;
  accept();
}