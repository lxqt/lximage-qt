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


#include "screenshotselectareagraphicsview.h"
#include <QMouseEvent>

using namespace LxImage;

ScreenshotSelectAreaGraphicsView::ScreenshotSelectAreaGraphicsView(QGraphicsScene* scene, QWidget* parent) : QGraphicsView(scene, parent)
{
  p_x0 = p_y0 = -1.0;
  selectedAreaRect = nullptr;
  setCursor(Qt::CrossCursor);
}

void ScreenshotSelectAreaGraphicsView::mousePressEvent(QMouseEvent *event)
{
  if(p_x0 < 0) {
    p_x0 = event->x();
    p_y0 = event->y();
  } else {
    if(selectedAreaRect == nullptr) {
      QPen pen(Qt::green, 3, Qt::DashDotLine, Qt::RoundCap, Qt::RoundJoin);
      QColor color(Qt::green);
      color.setAlpha(128);
      QBrush brush(color);
      selectedAreaRect = scene()->addRect(p_x0, p_y0, p_x0, p_y0, pen, brush);
    } 
    selectedAreaRect->setRect(rectPositionAndSize(event->x(),event->y()));
  }
}

void ScreenshotSelectAreaGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
  mousePressEvent(event);
}

void ScreenshotSelectAreaGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
  QRectF rectF = rectPositionAndSize(event->x(),event->y());
  QRect rect(rectF.x(), rectF.y(), rectF.width(), rectF.height());
  Q_EMIT selectedArea(rect);
}

QRectF ScreenshotSelectAreaGraphicsView::rectPositionAndSize(int x, int y) {
  int width = x - p_x0;
  int height = y - p_y0;
  int x0 = p_x0;
  int y0 = p_y0;
  
  if(width >= 0 && height >= 0) {
    return QRectF(x0, y0, width, height);
  } else if(width >= 0) {
    return QRectF(x0, y, width, -height);
  } else if(height >= 0) {
    return QRectF(x, y0, -width, height);
  }
  return QRectF(x, y, -width, -height);
}