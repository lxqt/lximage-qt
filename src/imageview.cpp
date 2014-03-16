/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  <copyright holder> <email>

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


#include "imageview.h"
#include <QWheelEvent>

using namespace LxImage;

ImageView::ImageView(QWidget* parent):
  QGraphicsView(parent),
  imageItem_(new QGraphicsRectItem()),
  scene_(new QGraphicsScene(this)),
  autoZoomFit_(false),
  scaleFactor_(1.0) {

  setScene(scene_);
  imageItem_->hide();
  // imageItem_->setPen(QPen(Qt::NoPen));
  scene_->addItem(imageItem_);
}

ImageView::~ImageView() {
  delete imageItem_;
}


void ImageView::wheelEvent(QWheelEvent* event) {
  int delta = event->delta();
  // Ctrl key is pressed
  if(event->modifiers() & Qt::ControlModifier) {
    if(delta > 0) { // forward
      zoomIn();
    }
    else { // backward
      zoomOut();
    }
  }
  else {
    // The default handler QGraphicsView::wheelEvent(event) tries to
    // scroll the view, which is not what we need.
    // Skip the default handler and use its parent QWidget's handler here.
    QWidget::wheelEvent(event);
  }
}

void ImageView::mouseDoubleClickEvent(QMouseEvent* event) {
  // The default behaviour of QGraphicsView::mouseDoubleClickEvent() is
  // not needed for us. We call its parent class instead so the event can be
  // filtered by event filter installed on the view.
  // QGraphicsView::mouseDoubleClickEvent(event);
  QAbstractScrollArea::mouseDoubleClickEvent(event);
}

void ImageView::resizeEvent(QResizeEvent* event) {
  QGraphicsView::resizeEvent(event);
  if(autoZoomFit_)
    zoomFit();
}

void ImageView::zoomFit() {
  if(!image_.isNull()) {
    // if the image is smaller than our view, use its original size
    // instead of scaling it up.
    if(image_.width() <= width() && image_.height() <= height()) {
      zoomOriginal();
      return;
    }
  }
  fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
  scaleFactor_ = transform().m11();
}

void ImageView::zoomIn() {
  if(!image_.isNull()) {
    resetTransform();
    scaleFactor_ *= 1.1;
    scale(scaleFactor_, scaleFactor_);
  }
}

void ImageView::zoomOut() {
  if(!image_.isNull()) {
    resetTransform();
    scaleFactor_ /= 1.1;
    scale(scaleFactor_, scaleFactor_);
  }
}

void ImageView::zoomOriginal() {
  resetTransform();
  scaleFactor_ = 1.0;
}

void ImageView::setImage(QImage image) {
  image_ = image;
  if(image.isNull()) {
    imageItem_->hide();
    imageItem_->setBrush(QBrush());
    scene_->setSceneRect(0, 0, 0, 0);
  }
  else {
    imageItem_->setRect(0, 0, image_.width(), image_.height());
    imageItem_->setBrush(image_);
    imageItem_->show();
    scene_->setSceneRect(0, 0, image_.width(), image_.height());
  }

  if(autoZoomFit_)
    zoomFit();
}

void ImageView::setScaleFactor(double factor) {
  if(factor != scaleFactor_) {
    scaleFactor_ = factor;
    resetTransform();
    scale(factor, factor);
  }
}
