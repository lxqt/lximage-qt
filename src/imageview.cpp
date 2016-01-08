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
#include <QStyle>
#include <QLabel>
#include <QGraphicsProxyWidget>

namespace LxImage {

ImageView::ImageView(QWidget* parent):
  QGraphicsView(parent),
  imageItem_(new QGraphicsRectItem()),
  scene_(new QGraphicsScene(this)),
  gifMovie_(NULL),
  autoZoomFit_(false),
  cacheTimer_(NULL),
  scaleFactor_(1.0) {

  setViewportMargins(0, 0, 0, 0);
  setContentsMargins(0, 0, 0, 0);
  setLineWidth(0);

  setScene(scene_);
  imageItem_->hide();
  imageItem_->setPen(QPen(Qt::NoPen)); // remove the border
  scene_->addItem(imageItem_);
}

ImageView::~ImageView() {
  scene_->clear(); // deletes all items
  if(gifMovie_)
    delete gifMovie_;
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

void ImageView::setImage(QImage image, bool show) {
  if(gifMovie_ && show) { // a gif animation was shown
    scene_->clear();
    delete gifMovie_;
    gifMovie_ = NULL;
    // recreate the rect item
    imageItem_ = new QGraphicsRectItem();
    imageItem_->hide();
    imageItem_->setPen(QPen(Qt::NoPen));
    scene_->addItem(imageItem_);
  }

  image_ = image;
  if(image.isNull()) {
    imageItem_->hide();
    imageItem_->setBrush(QBrush());
    scene_->setSceneRect(0, 0, 0, 0);
  }
  else {
    if(show) {
      imageItem_->setRect(0, 0, image_.width(), image_.height());
      imageItem_->setBrush(image_);
      imageItem_->show();
    }
    scene_->setSceneRect(0, 0, image_.width(), image_.height());
  }

  if(autoZoomFit_)
    zoomFit();
  queueGenerateCache();
}

void ImageView::setGifAnimation(QString fileName) {
  QImage image(fileName);
  if(image.isNull()) {
    image_ = QImage();
    imageItem_->hide();
    imageItem_->setBrush(QBrush());
    scene_->setSceneRect(0, 0, 0, 0);
  }
  else {
    scene_->clear();
    imageItem_ = NULL; // it's deleted by clear();
    if(gifMovie_) {
      delete gifMovie_;
      gifMovie_ = NULL;
    }
    QPixmap pix(image.size());
    pix.fill(Qt::transparent);
    QGraphicsItem *gifItem = new QGraphicsPixmapItem(pix);
    QLabel *gifLabel = new QLabel();
    gifMovie_ = new QMovie(fileName);
    QGraphicsProxyWidget* gifWidget = new QGraphicsProxyWidget(gifItem);
    gifLabel->setAttribute(Qt::WA_NoSystemBackground);
    gifLabel->setMovie(gifMovie_);
    gifWidget->setWidget(gifLabel);
    gifMovie_->start();
    /* the first frame won't be shown but will be
       used for tracking position and dimensions */
    image_ = gifMovie_->currentImage();
    scene_->addItem(gifItem);
    scene_->setSceneRect(gifItem->boundingRect());
  }

  if(autoZoomFit_)
    zoomFit();
  queueGenerateCache(); // deletes the cache timer in this case
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
  // if the image is scaled and we have a high quality cached image
  if(scaleFactor_ != 1.0 && !cachedPixmap_.isNull()) {
    // rectangle of the whole image in viewport coordinate
    QRect viewportImageRect = sceneToViewport(imageItem_->rect());
    // the visible part of the image.
    QRect desiredCachedRect = viewportToScene(viewportImageRect.intersected(viewport()->rect()));
    // check if the cached area is what we need and if the cache is out of date
    if(cachedSceneRect_ == desiredCachedRect) {
      // rect of the image area that needs repaint, in viewport coordinate
      QRect repaintImageRect = viewportImageRect.intersected(event->rect());
      // see if the part asking for repaint is contained by our cache.
      if(cachedRect_.contains(repaintImageRect)) {
        QPainter painter(viewport());
        painter.fillRect(event->rect(), backgroundBrush());
        painter.drawPixmap(repaintImageRect, cachedPixmap_);
        return;
      }
    }
  }
  if(!image_.isNull()) { // we don't have a cache yet or it's out of date already, generate one
    queueGenerateCache();
  }
  QGraphicsView::paintEvent(event);
}

void ImageView::queueGenerateCache() {
  if(!cachedPixmap_.isNull()) // clear the old pixmap if there's any
    cachedPixmap_ = QPixmap();

  // we don't need to cache the scaled image if its the same as the original image (scale:1.0)
  // no cache for gif animations either
  if(scaleFactor_ == 1.0 || gifMovie_) {
    if(cacheTimer_) {
      cacheTimer_->stop();
      delete cacheTimer_;
      cacheTimer_ = NULL;
    }
    return;
  }

  if(!cacheTimer_ && !gifMovie_) {
    cacheTimer_ = new QTimer();
    cacheTimer_->setSingleShot(true);
    connect(cacheTimer_, SIGNAL(timeout()), SLOT(generateCache()));
  }
  if(cacheTimer_)
    cacheTimer_->start(200); // restart the timer
}

// really generate the cache
void ImageView::generateCache() {
  // disable the one-shot timer
  cacheTimer_->deleteLater();
  cacheTimer_ = NULL;

  // generate a cache for "the visible part" of the scaled image
  // rectangle of the whole image in viewport coordinate
  QRect viewportImageRect = sceneToViewport(imageItem_->rect());
  // rect of the image area that's visible in the viewport (in viewport coordinate)
  cachedRect_ = viewportImageRect.intersected(viewport()->rect());

  // convert to the coordinate of the original image
  cachedSceneRect_ = viewportToScene(cachedRect_);
  // create a sub image of the visible without real data copy
  // Reference: http://stackoverflow.com/questions/12681554/dividing-qimage-to-smaller-pieces
  QRect subRect = image_.rect().intersected(cachedSceneRect_);
  const uchar* bits = image_.constBits();
  unsigned int offset = subRect.x() * image_.depth() / 8 + subRect.y() * image_.bytesPerLine();
  QImage subImage = QImage(bits + offset, subRect.width(), subRect.height(), image_.bytesPerLine(), image_.format());

  // If the original image has a color table, also use it for the subImage
  QVector<QRgb> colorTable = image_.colorTable();
  if (!colorTable.empty())
    subImage.setColorTable(colorTable);

  // QImage scaled = subImage.scaled(subRect.width() * scaleFactor_, subRect.height() * scaleFactor_, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  QImage scaled = subImage.scaled(cachedRect_.width(), cachedRect_.height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

  // convert the cached scaled image to pixmap
  cachedPixmap_ = QPixmap::fromImage(scaled);
  viewport()->update();
  /*
  qDebug() << "viewportImageRect" << viewportImageRect
  << "cachedRect_" << cachedRect_
  << "cachedSceneRect_" << cachedSceneRect_
  << "subRect" << subRect;
  */
}

// convert viewport coordinate to the original image (not scaled).
QRect ImageView::viewportToScene(const QRect& rect) {
  // QPolygon poly = mapToScene(imageItem_->rect());
  QPoint topLeft = mapToScene(rect.topLeft()).toPoint();
  QPoint bottomRight = mapToScene(rect.bottomRight()).toPoint();
  return QRect(topLeft, bottomRight);
}

QRect ImageView::sceneToViewport(const QRectF& rect) {
  QPoint topLeft = mapFromScene(rect.topLeft());
  QPoint bottomRight = mapFromScene(rect.bottomRight());
  return QRect(topLeft, bottomRight);
}


} // namespace LxImage
