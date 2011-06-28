#include "content/browser/renderer_host/video_renderer_widget.h"
#include <QGraphicsItem>
#include <QPainter>

//#define VIDEO_WIDGET_DEBUG

VideoRendererWidget::VideoRendererWidget(QGraphicsItem *parent):
    QGraphicsWidget(parent),
    pixmap_(0),
    scale_factor_(1.0),
    update_count_(0)
{
}

VideoRendererWidget::~VideoRendererWidget()
{
}

void VideoRendererWidget::updateVideoFrame(unsigned int pixmap, QRect& rect)
{
  pixmap_ = pixmap;

  if (rect_ != rect)
  {
    rect_ = rect;
    setGeometry(rect_.x() * scale_factor_, rect_.y() * scale_factor_,
                rect_.width() * scale_factor_, rect_.height() * scale_factor_);
  }

  update();

  update_count_++;
}

void VideoRendererWidget::setScaleFactor(double factor)
{
  scale_factor_ = factor;

  setGeometry(rect_.x() * scale_factor_, rect_.y() * scale_factor_,
              rect_.width() * scale_factor_, rect_.height() * scale_factor_);
}
  
void VideoRendererWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget * widget)
{
  QRectF src(0.0, 0.0, rect_.width(), rect_.height());
  QRectF dst(0.0, 0.0, rect_.width() * scale_factor_, rect_.height() * scale_factor_);

  if (pixmap_ != 0)
  {
    painter->drawPixmap(dst, QPixmap::fromX11Pixmap(pixmap_), src);
  }

#if defined(VIDEO_WIDGET_DEBUG)
  QPen pen(QColor("red"));
  pen.setWidth(3);
  painter->save();
  painter->setPen(pen);
  painter->drawRect(dst);
  QString str = QString(" (direct rendering mode: ") + QString::number(update_count_) + QString(")");
  painter->drawText(dst, Qt::AlignTop | Qt::AlignLeft, str);
  painter->restore();
#endif

}

