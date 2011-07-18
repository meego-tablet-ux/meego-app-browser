// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RWHV_QT_WIDGET_H_
#define CHROME_BROWSER_RWHV_QT_WIDGET_H_

#include <QGraphicsWidget>
#include <QOrientationReading>
#include <string>
#include <QTimer>
#include "third_party/WebKit/Source/WebKit/chromium/public/WebTextInputType.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebInputEvent.h"
#include "third_party/skia/include/core/SkBitmap.h"

#include "content/browser/renderer_host/backing_store_x.h"
#include "chrome/common/render_tiling.h"

class QGraphicsObject;
class QGraphicsItem;
class RenderWidgetHost;
class RenderWidgetHostViewQt;
class QTapGesture;
class QTapAndHoldGesture;
class QPanGesture;
class QPinchGesture;
class QPropertyAnimation;
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
  void UpdateSelectionRange(gfx::Point start, gfx::Point end, int height, bool set);

  // for pinch emulation
  void touchPointCopyPosToLastPos(QTouchEvent::TouchPoint &point);
  void touchPointCopyMousePosToPointPos(QTouchEvent::TouchPoint &point, const QGraphicsSceneMouseEvent *event);
  void touchPointCopyMousePosToPointStartPos(QTouchEvent::TouchPoint &point, const QGraphicsSceneMouseEvent *event);
  void touchPointMirrorMousePosToPointPos(QTouchEvent::TouchPoint &point, const QGraphicsSceneMouseEvent *event);
  void touchPointMirrorMousePosToPointStartPos(QTouchEvent::TouchPoint &point, const QGraphicsSceneMouseEvent *event);

  bool eventEmulatePinch(QEvent *event);
  void UnFrozen();
  QRect GetVisibleRect();
  void WasHidden();
  void DidBecomeSelected();
  void DidBackingStoreScale();
  void AdjustSize();
  void SetScrollPosition(const gfx::Point& scrollPosition);
  qreal scale() { return flatScaleByStep(scale_); }
  void SetScaleFactor(double scale);
  
  // Check the contents size and calculate the proper
  // scale factor as the bottom value
  // Return false in case of it needs to reset preferred size, and
  // the desired_preferred_size is returned as the right size
  // Return true if no additional action is required.
  bool CheckContentsSize(gfx::Size& desired_preferred_size);

 protected:
  virtual bool eventFilter ( QObject * watched, QEvent * event );
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

  virtual QVariant inputMethodQuery(Qt::InputMethodQuery query) const;

  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget * widget);
  virtual bool event(QEvent *event);

  //! \reimp end
Q_SIGNALS:
  void updatePinchState(int pinchState);
  void setViewPos(QPointF& pos);
  void setViewSize(QSizeF& size);
  void sizeAdjusted();

protected Q_SLOTS:
  void onOrientationAngleChanged();
  void handleInputMethodAreaChanged(const QRect &newArea);
  void onSizeAdjusted();
  void onAnimationFinished();
  void onDoubleTapped();
  void deliverMouseClickEvent();
 private:

  typedef enum {
    SELECTION_HANDLER_NONE = 0,
    SELECTION_HANDLER_START,
    SELECTION_HANDLER_END,
  } SelectionHandlerID;
  
  void SetWebViewSize();
  
  QGraphicsObject* GetWebViewItem();
  QGraphicsObject* GetViewportItem();
  
  bool shouldDeliverHoverLeaveEvent();
  bool shouldDeliverHoverEnterEvent();
  bool shouldDeliverMouseMove();
  bool shouldDeliverDoubleClick();
  void deliverFakeHoverLeaveEvent(QGraphicsSceneMouseEvent* event);
  void deliverFakeHoverEnterEvent(QGraphicsSceneMouseEvent* event);
  void deliverMousePressEvent();
  void deliverMouseDoubleClickEvent();
  bool setDoingGesture(Qt::GestureType);
  bool clearDoingGesture(Qt::GestureType);
  bool isDoingGesture();
  bool isDoingGesture(Qt::GestureType);
  void fakeMouseRightButtonClick(QGestureEvent* event, QTapAndHoldGesture* gesture);
  void doZoom(qreal factor, QPointF pos, bool reset);

  void onKeyPressReleaseEvent(QKeyEvent* event);
  
  // touch events
  bool shouldDeliverTouchMove();

  // scrollable area check
  bool inScrollableArea();

  bool isScalablePage();
  
  //gestureEvent
  void gestureEvent(QGestureEvent* event);
  void tapAndHoldGestureEvent(QGestureEvent* event, QTapAndHoldGesture* gesture);
  void panGestureEvent(QGestureEvent* event, QPanGesture* gesture);
  void pinchGestureEvent(QGestureEvent* event, QPinchGesture* gesture);

  void doubleTapAction(const QPointF&);
  // only scroll and zoom on first input method update.
  void scrollAndZoomForTextInput(const QRect& caret_rect, bool animation);
  // selection
  SelectionHandlerID findSelectionHandler(int x, int y);
  void InvokeSelection(QTapAndHoldGesture* gesture);
  void ModifySelection(SelectionHandlerID handler, gfx::Point new_pos);
  void CommitSelection();

  void setViewportInteractive(bool interactive);

  // Sends a fake composition key event with specified event type. A composition
  // key event is a key event with special key code 229.
  void SendFakeCompositionKeyEvent(WebKit::WebInputEvent::Type type);

  // Whether the input method is enabled by webkit or not.
  // It shall be set to false when an imUpdateStatus message with control ==
  // IME_DISABLE is received, and shall be set to true if control ==
  // IME_COMPLETE_COMPOSITION or IME_MOVE_WINDOWS.
  bool im_enabled_;

  int im_cursor_pos_;
  std::string im_selection_;
  std::string im_surrounding_;

  RenderWidgetHostViewQt* host_view_;
  int gesture_flags_;
  bool cancel_next_mouse_release_event_;
  WebKit::WebMouseEvent mouse_press_event_;
  bool mouse_press_event_delivered_;
  WebKit::WebMouseWheelEvent last_pan_wheel_event_;
  QRect cursor_rect_;

  QPointF pinch_center_;
  QPointF pinch_start_pos_;
  
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

  //Timer for lower the priority of click event
  QTimer *delay_for_click_timer_;
  WebKit::WebMouseEvent mouse_release_event_;

  //Two finger gestures emulation variables
  QTouchEvent::TouchPoint emuPoint1, emuPoint2;
  bool pinchEmulationEnabled;

  bool installed_filter_;
  QPointF distance_;

  // Svae th flickable contentX and contentY
  QPoint flickable_content_pos_;

  bool pinch_completing_;
  QSizeF previous_size_;

  QRectF pending_webview_rect_;

  QPointF topLeft_;
  // Animation for rebounce effect
  QPropertyAnimation* rebounce_animation_;
  // Animation for vkb scroll effect
  QPropertyAnimation* scroll_animation_;
  int vkb_height_;
  int vkb_flag_;
  
  //track cursor whether in input entry
  bool is_enabled_;
  // whether in selecting characters in input entry
  bool is_inputtext_selection_;

  // whether scrolling scrollable area
  bool is_scrolling_scrollable_;

  // double tap
  bool is_doing_double_tap_;
  QPropertyAnimation* scale_animation_;

  // double click (forward to WebKit)
  bool double_click_;
  WebKit::WebMouseEvent last_mouse_press_event_;
  WebKit::WebMouseEvent last_mouse_release_event_;

  WebKit::WebTouchEvent touch_start_event_;
  WebKit::WebTouchEvent touch_end_event_;


  // Current scale factor
  qreal scale_;
  qreal pending_scale_;
  qreal pinch_scale_factor_;

  // factor for scaling to screen
  qreal scale_to_screen_factor_;
};

#endif  // CHROME_BROWSER_RWHV_QT_WIDGET_H_
