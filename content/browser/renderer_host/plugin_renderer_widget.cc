#include "content/browser/renderer_host/plugin_renderer_widget.h"
#include <QGraphicsItem>
#include <QPainter>

#include "base/logging.h"
#include "chrome/browser/renderer_host/render_widget_host_view_qt.h"

//#define PLUGIN_WIDGET_DEBUG

PluginRendererWidget::PluginRendererWidget(RenderWidgetHostViewQt* host_view,
                                           unsigned int id,
                                           QGraphicsItem *parent):
    QGraphicsWidget(parent),
    host_view_(host_view),
    pixmap_(0),
    scale_factor_(1.0),
    seq_(0),
    ack_(0),
    id_(id)
{
}

PluginRendererWidget::~PluginRendererWidget()
{
}

void PluginRendererWidget::updatePluginWidget(unsigned int pixmap,
                                              QRect& rect,
                                              unsigned int seq)
{
  pixmap_ = pixmap;

  if (rect_ != rect)
  {
    rect_ = rect;
    setGeometry(rect_.x() * scale_factor_, rect_.y() * scale_factor_,
                rect_.width() * scale_factor_, rect_.height() * scale_factor_);
  }

  if (seq > seq_)
  {
    update();
  }

  seq_ = seq;
}

void PluginRendererWidget::setScaleFactor(double factor)
{
  scale_factor_ = factor;

  setGeometry(rect_.x() * scale_factor_, rect_.y() * scale_factor_,
              rect_.width() * scale_factor_, rect_.height() * scale_factor_);
}

void PluginRendererWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget * widget)
{
  QRectF src(0.0, 0.0, rect_.width(), rect_.height());
  QRectF dst(0.0, 0.0, rect_.width() * scale_factor_, rect_.height() * scale_factor_);

  if (pixmap_ != 0)
  {
    painter->drawPixmap(dst, QPixmap::fromX11Pixmap(pixmap_), src);
#if !defined(PLUGIN_WIDGET_DEBUG)
    ack_ = seq_;
    host_view_->DidPaintPluginWidget(id_, ack_);
#endif
  }

#if defined(PLUGIN_WIDGET_DEBUG)
  static int count = 0;
  static uint64_t start_time = 0;
  static uint64_t end_time = 0;
  static int fps = 0;
  const int fsnumber = 50;

  if ( (count % fsnumber) == 0 ) {
    struct timeval tv;
    if (!gettimeofday(&tv, NULL)) {
      start_time = tv.tv_sec;
      start_time *= 1000;
      start_time += tv.tv_usec / 1000;
    }
  }

  if ((count % fsnumber) == (fsnumber - 1)) {
    struct timeval tv;
    if (!gettimeofday(&tv, NULL)) {
      end_time = tv.tv_sec;
      end_time *= 1000;
      end_time += tv.tv_usec / 1000;
    }
    fps = (fsnumber * 1000) / (end_time - start_time);
  }

  count++;

  ack_ = seq_;
  QPen pen(QColor("red"));
  pen.setWidth(3);
  painter->save();
  painter->setPen(pen);
  painter->drawRect(dst);
  QString str = QString(" (fps:") + QString::number(fps) + QString(" ") + QString::number(count) + QString("-") + QString::number(ack_) + QString(")");
  painter->drawText(dst, Qt::AlignTop | Qt::AlignLeft, str);
  painter->restore();

  host_view_->DidPaintPluginWidget(id_, ack_);
#endif

}
