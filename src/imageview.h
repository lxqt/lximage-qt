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


#ifndef LXIMAGE_IMAGEVIEW_H
#define LXIMAGE_IMAGEVIEW_H

#include "graphicsscene.h"
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QImage>
#include <QPixmap>
#include <QMovie>
#include <QRect>

class QTimer;

namespace LxImage {

class ImageView : public QGraphicsView {
  Q_OBJECT

public:
  ImageView(QWidget* parent = 0);
  virtual ~ImageView();

  void setImage(QImage image, bool show = true);
  void setGifAnimation(QString fileName);
  void setSVG(QString fileName);

  QImage image() {
    return image_;
  }

  void setScaleFactor(double scale);

  double scaleFactor() {
    return scaleFactor_;
  }

  void zoomIn();
  void zoomOut();
  void zoomFit();
  void zoomOriginal();

  bool autoZoomFit() {
    return autoZoomFit_;
  }

  // if set to true, zoomFit() is done automatically when the size of the window is changed.
  void setAutoZoomFit(bool value) {
    autoZoomFit_ = value;
  }

  // if set to true, hides the cursor after 3s of inactivity
  void hideCursor(bool enable);

Q_SIGNALS:
  void fileDropped(const QString file);

protected:
  virtual void wheelEvent(QWheelEvent* event);
  virtual void mouseDoubleClickEvent(QMouseEvent* event);
  virtual void mousePressEvent(QMouseEvent* event);
  virtual void mouseReleaseEvent(QMouseEvent* event);
  virtual void mouseMoveEvent(QMouseEvent* event);
  virtual void focusInEvent(QFocusEvent* event);
  virtual void resizeEvent(QResizeEvent* event);
  virtual void paintEvent(QPaintEvent* event);

private:
  void queueGenerateCache();
  QRect viewportToScene(const QRect& rect);
  QRect sceneToViewport(const QRectF& rect);

private Q_SLOTS:
  void onFileDropped(const QString file);
  void generateCache();
  void blankCursor();

private:
  GraphicsScene* scene_; // the topmost container of all graphic items
  QGraphicsRectItem* imageItem_; // the rect item used to draw the image
  QImage image_; // image to show
  QMovie *gifMovie_; // gif animation to show (should be deleted explicitly)
  QPixmap cachedPixmap_; // caching of current viewport content (high quality scaled image)
  QRect cachedRect_; // rectangle containing the cached region (in viewport coordinate)
  QRect cachedSceneRect_; // rectangle containing the cached region (in scene/original image coordinate)
  QTimer* cacheTimer_;
  QTimer *cursorTimer_; // for hiding cursor in fullscreen mode
  double scaleFactor_;
  bool autoZoomFit_;
  bool isSVG; // is the image an SVG file?
};

}

#endif // LXIMAGE_IMAGEVIEW_H
