// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RWHV_QT_WIDGET_H_
#define CHROME_BROWSER_RWHV_QT_WIDGET_H_

#include <QGraphicsWidget>
#include <QOrientationReading>

#include "third_party/WebKit/Source/WebKit/chromium/public/WebTextInputType.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebInputEvent.h"
#include "third_party/skia/include/core/SkBitmap.h"

#include "content/browser/renderer_host/backing_store_x.h"

class QGraphicsItem;
class RenderWidgetHost;
class RenderWidgetHostViewQt;
class QTapGesture;
class QTapAndHoldGesture;
class QPanGesture;
class QPinchGesture;
class PanAnimation;
struct NativeWebKeyboardEvent;

namespace gfx {
class Rect;
}

/*!
 * \class RWHVQtWidget
 * \brief RWHVQtWidget is the Controller part of the RenderWidgetHostViewQt's real widget
 *
 * RWHVQtWidget is created and used by RenderWidgetHostViewQt. It's a custom QGraphics Widget to handle
 * UI related events from the parent UI, processing and forwarding them to RenderWidgetHostView
 * or RenderWidgetHost.
 *
 */

class RWHVQtWidget : public QGraphicsWidget
{
  Q_OBJECT
 public:

  RWHVQtWidget(RenderWidgetHostViewQt* host_view, QGraphicsItem *Parent = NULL);

  ~RWHVQtWidget();
  RenderWidgetHostViewQt* hostView();
  void setHostView(RenderWidgetHostViewQt* host_view);
  void imeUpdateTextInputState(WebKit::WebTextInputType type, const gfx::Rect& caret_rect);
  void imeCancelComposition();
  QtMobility::QOrientationReading::Orientation orientationAngle();
  void setOrientationAngle(QtMobility::QOrientationReading::Orientation angle);
  void UpdateSelectionRange(gfx::Point start, gfx::Point end, bool set);
#ifdef PINCH_FINI_DEBUG
  virtual void update ( qreal x, qreal y, qreal width, qreal height );
#endif
 protected:

  //! \reimp
  virtual void focusInEvent(QFocusEvent* event);
  virtual void focusOutEvent(QFocusEvent* event);
  virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
  virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);
  virtual void hoverMoveEvent(QGraphicsSceneHoverEvent* event);
  virtual void keyPressEvent(QKeyEvent* event);
  virtual void keyReleaseEvent(QKeyEvent* event);
  virtual void inputMethodEvent(QInputMethodEvent *event);
  virtual void showEvent(QShowEvent* event);
  virtual void hideEvent(QHideEvent* event);

  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
  virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
  virtual void resizeEvent(QGraphicsSceneResizeEvent* event);

  virtual QVariant inputMethodQuery(Qt::InputMethodQuery query);

  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget * widget);
  virtual bool event(QEvent *event);
  //! \reimp end
  virtual void setPinchImage();

 protected Q_SLOTS:
  void autoPanCallback(int dx, int dy);
  void onOrientationAngleChanged();
  void pinchFinishTimeout();

 private:

  typedef enum {
    SELECTION_HANDLER_NONE = 0,
    SELECTION_HANDLER_START,
    SELECTION_HANDLER_END,
  } SelectionHandlerID;

  bool shouldDeliverMouseMove();
  void deliverMousePressEvent();
  bool setDoingGesture(Qt::GestureType);
  bool clearDoingGesture(Qt::GestureType);
  bool isDoingGesture();
  bool isDoingGesture(Qt::GestureType);
  void fakeMouseRightButtonClick(QGestureEvent* event, QTapAndHoldGesture* gesture);
  void doZoom(qreal factor, QPointF pos, bool reset);

  void onKeyPressReleaseEvent(QKeyEvent* event);

  void finishPinch();
  //gestureEvent
  void gestureEvent(QGestureEvent* event);
  void tapAndHoldGestureEvent(QGestureEvent* event, QTapAndHoldGesture* gesture);
  void panGestureEvent(QGestureEvent* event, QPanGesture* gesture);
  void pinchGestureEvent(QGestureEvent* event, QPinchGesture* gesture);

  void zoom2TextAction(const QPointF&);
  // selection
  SelectionHandlerID findSelectionHandler(int x, int y);
  void InvokeSelection(QTapAndHoldGesture* gesture);
  void ModifySelection(SelectionHandlerID handler, gfx::Point new_pos);

  // Whether the input method is enabled by webkit or not.
  // It shall be set to false when an imUpdateStatus message with control ==
  // IME_DISABLE is received, and shall be set to true if control ==
  // IME_COMPLETE_COMPOSITION or IME_MOVE_WINDOWS.
  bool im_enabled_;

  RenderWidgetHostViewQt* host_view_;
  int gesture_flags_;
  bool cancel_next_mouse_release_event_;
  WebKit::WebMouseEvent mouse_press_event_;
  bool mouse_press_event_delivered_;
  PanAnimation *auto_pan_;
  WebKit::WebMouseWheelEvent last_pan_wheel_event_;
  QRect cursor_rect_;

  BackingStoreX* pinch_backing_store_;
  bool pinch_image_;
  qreal scale_factor_;
  QPointF pinch_center_;
  QRectF pinch_src_rect_;
  QTimer* pinch_release_timer_;
  gfx::Point pinch_view_pos_;
  QtMobility::QOrientationReading::Orientation orientation_angle_;
  bool hold_paint_;

  qint64 m_dbclkHackTimeStamp;
  QPointF m_dbclkHackPos;

  // info about selection
  gfx::Point selection_start_pos_;
  gfx::Point selection_end_pos_;
  bool in_selection_mode_;
  bool is_modifing_selection_;
  SelectionHandlerID current_selection_handler_;
};

#endif  // CHROME_BROWSER_RWHV_QT_WIDGET_H_
