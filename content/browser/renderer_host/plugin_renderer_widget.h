// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PLUGIN_RENDERER_WIDGET_H_
#define CHROME_BROWSER_PLUGIN_RENDERER_WIDGET_H_

#include <QGraphicsWidget>
#include <QRect>

namespace gfx {
class Rect;
}

class RenderWidgetHostViewQt;

class PluginRendererWidget : public QGraphicsWidget
{
  Q_OBJECT
 public:
  PluginRendererWidget(RenderWidgetHostViewQt* host_view,
                       unsigned int id,
                       QGraphicsItem *Parent = NULL);

  ~PluginRendererWidget();

  void updatePluginWidget(unsigned int pixmap, QRect& rect, unsigned int seq);
  void setScaleFactor(double factor);

 protected:
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget * widget);

 private:
  RenderWidgetHostViewQt* host_view_;
  unsigned int pixmap_;
  double scale_factor_;
  QRect rect_;

  unsigned int id_;
  unsigned int seq_;
  unsigned int ack_;
};

#endif
