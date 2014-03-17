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
#include <QPaintEvent>
#include <QPainter>
#include <QTimer>
#include <QPolygon>
#include <QDebug>

namespace LxImage {

ImageView::ImageView(QWidget* parent):
  QGraphicsView(parent),
  imageItem_(new QGraphicsRectItem()),
  scene_(new QGraphicsScene(this)),
  autoZoomFit_(false),
  cacheTimer_(NULL),
  scaleFactor_(1.0) {

  setScene(scene_);
  imageItem_->hide();
  imageItem_->setPen(QPen(Qt::NoPen)); // remove the border
  scene_->addItem(imageItem_);
}

ImageView::~ImageView() {
  delete imageItem_;
  if(cacheTimer_) {
    cacheTimer_->stop();
    delete cacheTimer_;
  }
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
  queueGenerateCache();
}

void ImageView::zoomIn() {
  autoZoomFit_ = false;
  if(!image_.isNull()) {
    resetTransform();
    scaleFactor_ *= 1.1;
    scale(scaleFactor_, scaleFactor_);
    queueGenerateCache();
  }
}

void ImageView::zoomOut() {
  autoZoomFit_ = false;
  if(!image_.isNull()) {
    resetTransform();
    scaleFactor_ /= 1.1;
    scale(scaleFactor_, scaleFactor_);
    queueGenerateCache();
  }
}

void ImageView::zoomOriginal() {
  resetTransform();
  scaleFactor_ = 1.0;
  autoZoomFit_ = false;
  queueGenerateCache();
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
  queueGenerateCache();
}

void ImageView::setScaleFactor(double factor) {
  if(factor != scaleFactor_) {
    scaleFactor_ = factor;
    resetTransform();
    scale(factor, factor);
    queueGenerateCache();
  }
}

void ImageView::paintEvent(QPaintEvent* event) {
  if(!cachedPixmap_.isNull()) { // if we have a high quality cached image
    QRect exposedRect = viewportToScene(event->rect());
    if(cachedRect_.contains(exposedRect)) { // we have the required image in the cache
      QPainter painter(viewport());
      painter.fillRect(event->rect(), backgroundBrush());
      painter.drawPixmap(event->rect(), cachedPixmap_);
      return;
    }
  }
  if(!image_.isNull()) { // we don't have a cache, generate one
    queueGenerateCache();
  }
  QGraphicsView::paintEvent(event);
}

void ImageView::queueGenerateCache() {
  if(!cachedPixmap_.isNull()) // clear the old pixmap if there's any
    cachedPixmap_ = QPixmap();

  if(!cacheTimer_) {
    cacheTimer_ = new QTimer();
    cacheTimer_->setSingleShot(true);
    connect(cacheTimer_, SIGNAL(timeout()), SLOT(generateCache()));
  }
  cacheTimer_->start(300); // restart the timer
}

// really generate the cache
void ImageView::generateCache() {
  cacheTimer_->deleteLater();
  cacheTimer_ = NULL;
  cachedRect_ = viewportToScene(viewport()->rect());
  
  // create a sub image without real data copy
  // http://stackoverflow.com/questions/12681554/dividing-qimage-to-smaller-pieces
  QRect subRect = image_.rect().intersect(cachedRect_);
  const uchar* bits = image_.constBits();
  unsigned int offset = subRect.x() * image_.depth() / 8 + subRect.y() * image_.bytesPerLine();
  QImage subImage = QImage(bits + offset, subRect.width(), subRect.height(), image_.bytesPerLine(), image_.format());
  // qDebug() << offset << cachedRect_ << image_.depth() << image_.bytesPerLine() << image_.format();
  QImage scaled = subImage.scaled(subRect.width() * scaleFactor_, subRect.height() * scaleFactor_, Qt::KeepAspectRatio, Qt::SmoothTransformation);

  cachedPixmap_ = QPixmap(viewport()->size());
  QPainter painter(&cachedPixmap_);
  painter.fillRect(viewport()->rect(), backgroundBrush());
  // FIXME: when the background brush is changed, we need to invalidate the cache and regenerate one.
  QRect imageRect = QRect((viewport()->width() - scaled.width())/2, (viewport()->height() - scaled.height())/2, scaled.width(), scaled.height());
  painter.drawImage(imageRect, scaled);
  // cachedPixmap_ = QPixmap::fromImage(scaled);
  viewport()->update(); // repaint the viewport
}

// convert viewport coordinate to the original image (not scaled).
QRect ImageView::viewportToScene(const QRect& rect) {
  // QPolygon poly = mapToScene(imageItem_->rect());
  QPoint topLeft = mapToScene(rect.topLeft()).toPoint();
  QPoint bottomRight = mapToScene(rect.bottomRight()).toPoint();
  return QRect(topLeft, bottomRight);
}

}
