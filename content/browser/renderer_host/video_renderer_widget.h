// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VIDEO_RENDERER_WIDGET_H_
#define CHROME_BROWSER_VIDEO_RENDERER_WIDGET_H_

#include <QGraphicsWidget>
#include <QRect>

namespace gfx {
class Rect;
}

class VideoRendererWidget : public QGraphicsWidget
{
  Q_OBJECT
 public:

  VideoRendererWidget(QGraphicsItem *Parent = NULL);

  ~VideoRendererWidget();

  void updateVideoFrame(unsigned int pixmap, QRect& rect);
  void setScaleFactor(double factor);
  
 protected:
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget * widget);
    
 private:
  unsigned int pixmap_;
  double scale_factor_;
  QRect rect_;

  unsigned int update_count_;
};

#endif
