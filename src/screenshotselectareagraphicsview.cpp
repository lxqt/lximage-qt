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
  p_x0_ = p_y0_ = -1.0;
  selectedAreaRect_ = nullptr;
  setCursor(Qt::CrossCursor);
}

void ScreenshotSelectAreaGraphicsView::mousePressEvent(QMouseEvent *event)
{
  if(p_x0_ < 0) {
    p_x0_ = event->x();
    p_y0_ = event->y();
  } else {
    if(selectedAreaRect_ == nullptr) {
      QColor highlight = palette().color(QPalette::Active,QPalette::Highlight);
      QPen pen(highlight, 3, Qt::DashDotLine, Qt::RoundCap, Qt::RoundJoin);
      QColor color(highlight);
      color.setAlpha(128);
      QBrush brush(color);
      selectedAreaRect_ = scene()->addRect(p_x0_, p_y0_, p_x0_, p_y0_, pen, brush);
    } 
    selectedAreaRect_->setRect(rectPositionAndSize(event->x(),event->y()));
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
  int width = x - p_x0_;
  int height = y - p_y0_;
  
  if(width >= 0 && height >= 0) {
    return QRectF(p_x0_, p_y0_, width, height);
  } else if(width >= 0) {
    return QRectF(p_x0_, y, width, -height);
  } else if(height >= 0) {
    return QRectF(x, p_y0_, -width, height);
  }
  return QRectF(x, y, -width, -height);
}