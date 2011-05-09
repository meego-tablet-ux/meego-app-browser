// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/render_messages.h"

#include <QtGui/QInputContext>
#include <QtCore/QVariant>
#include <QtGui/QGesture>
#include <qapplication.h>
#include <QTimer>
#include <QStyleOptionGraphicsItem>
#include <QtGui/QGraphicsSceneDragDropEvent>
#include <QtGui/QGraphicsSceneMouseEvent>
#include <QtGui/QGraphicsSceneHoverEvent>
#include <QtGui/QGraphicsSceneWheelEvent>
#include <QImage>
#include <QEvent>
#include <QDesktopWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QtDebug>
#include <algorithm>
#include <string>

#include "ui/base/l10n/l10n_util.h"
#include "ui/base/x/x11_util.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/string_util.h"
#include "base/task.h"
#include "base/time.h"

#include "chrome/browser/renderer_host/render_widget_host_view_qt.h"
#include "chrome/browser/browser_list.h"
#include "chrome/browser/ui/meegotouch/browser_window_qt.h"
#include "content/browser/renderer_host/animation_utils.h"
#include "content/browser/renderer_host/backing_store_x.h"
#include "content/browser/renderer_host/event_util_qt.h"
#include "content/browser/renderer_host/render_view_host.h"
#include "content/browser/renderer_host/render_widget_host.h"
#include "content/browser/renderer_host/render_widget_host_view.h"
#include "content/browser/renderer_host/rwhv_qt_widget.h"
#include "content/common/native_web_keyboard_event.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebWidget.h"

#include <launcherapp.h>

#undef pow
#include <cmath> // for std::pow

namespace {
static const int kSnapshotWebPageWidth = 1500;
static const int kSnapshotWebPageHeight = 2000;

static const qreal kMaxContentsScale = 5.0;
static const qreal kMinContentsScale = 1.0;
static const qreal kMaxPinchScale = 10;
static const qreal kMinPinchScale = 0.5;
static const qreal kNormalContentsScale = 1.0;
static const qreal kMaxDoubleTapScale = 1.6;
static const int kRebouceDuration = 200;
static const int kScrollDuration = 200;

static const int SelectionHandlerRadius = 30;
static const int SelectionHandlerRadiusSquare = (SelectionHandlerRadius * SelectionHandlerRadius);

// it might be a little over design to transfer gesture type from enum to int
// but just make sure we won't have any trouble later on custom gesture type.
static int toGestureFlag(Qt::GestureType type)
{
  int flag = 0x0;
  switch (type) {
    case Qt::TapGesture:
      flag = 0x1;
      break;
    case Qt::TapAndHoldGesture:
      flag = 0x2;
      break;
    case Qt::PanGesture:
      flag = 0x4;
      break;
    case Qt::PinchGesture:
      flag = 0x8;
      break;
    case Qt::SwipeGesture:
      flag = 0x10;
      break;
  }

  return flag;
}
}

RWHVQtWidget::RWHVQtWidget(RenderWidgetHostViewQt* host_view, QGraphicsItem* Parent)
    : QGraphicsWidget(Parent)
{
  setHostView(host_view);
  gesture_flags_ = 0;
  im_enabled_ = false;
  im_cursor_pos_ = -1;
  cancel_next_mouse_release_event_ = false;
  mouse_press_event_delivered_ = false;
  hold_paint_ = false;
  is_inputtext_selection_ = false;
  is_scrolling_scrollable_ = false;

  // double tap
  is_doing_double_tap_ = false;
   
  scale_animation_ = new QPropertyAnimation(this, "scale", this);
  connect(scale_animation_, SIGNAL(finished()), this, SLOT(onDoubleTapped()));
  
  // Create animation for rebounce effect 
  rebounce_animation_ = new QPropertyAnimation(this, "scale", this);
  connect(rebounce_animation_, SIGNAL(finished()), 
      this, SLOT(onAnimationFinished()));

  QEasingCurve curve(QEasingCurve::Linear);
  rebounce_animation_->setEasingCurve(curve);
  rebounce_animation_->setDuration(kRebouceDuration);
  rebounce_animation_->setEndValue(1.f);
  rebounce_animation_->setStartValue(1.f);

  // Create animation for scroll effect 

  scroll_animation_ = NULL;

  selection_start_pos_ = gfx::Point(0, 0);
  selection_end_pos_ = gfx::Point(0, 0);
  in_selection_mode_ = false;
  is_modifing_selection_ = false;
  current_selection_handler_ = SELECTION_HANDLER_NONE;

  m_dbclkHackTimeStamp = 0;
  m_dbclkHackPos = QPointF(0, 0);

  cursor_rect_ = QRect();

  LauncherApp* app = reinterpret_cast<LauncherApp*>(qApp);
  connect(app, SIGNAL(orientationChanged()), this, SLOT(onOrientationAngleChanged()));
  onOrientationAngleChanged();

  if (!hostView()->IsPopup()) {
    // we must not grab focus when we are running in a popup mode
    setFocusPolicy(Qt::StrongFocus);

    // use flickable to handle pan and flicking
    grabGesture(Qt::TapAndHoldGesture);
    grabGesture(Qt::PinchGesture);
    setAcceptTouchEvents(true);
  } else {
    // We'd better not handle gesture other than Pan in popup mode
    grabGesture(Qt::PanGesture);
    setAcceptTouchEvents(true);
  }

  installed_filter_ = false;

  connect(this, SIGNAL(sizeAdjusted()), this, SLOT(onSizeAdjusted()));
  
  pinchEmulationEnabled = false;
  pinch_completing_ = false;
  scale_ = pending_scale_ = kNormalContentsScale;

  delay_for_click_timer_ = new QTimer(this);
  connect(delay_for_click_timer_, SIGNAL(timeout()), this, SLOT(onClicked()));

  //vkb height 
  vkb_height_ = 0;
  vkb_flag_ = false;
  connect((const QObject*)qApp->inputContext(), SIGNAL(inputMethodAreaChanged(QRect)),
            this, SLOT(handleInputMethodAreaChanged(QRect)));
}

RWHVQtWidget::~RWHVQtWidget()
{
  delete rebounce_animation_;
  delete scroll_animation_;
}


RenderWidgetHostViewQt* RWHVQtWidget::hostView()
{
  return host_view_;
}

bool RWHVQtWidget::eventFilter ( QObject * obj, QEvent * event )
{
  if (eventEmulatePinch(event)) {
    return true;
  }
  else
  {
    return QObject::eventFilter(obj, event);
  }
}

void RWHVQtWidget::touchPointCopyPosToLastPos(QTouchEvent::TouchPoint &point)
{
  point.setLastPos(point.pos());
  point.setLastScenePos(point.scenePos());
  point.setLastScreenPos(point.screenPos());
}

void RWHVQtWidget::touchPointCopyMousePosToPointPos(QTouchEvent::TouchPoint &point, const QGraphicsSceneMouseEvent *event)
{
  point.setPos(event->pos());
  point.setScenePos(event->scenePos());
  point.setScreenPos(event->screenPos());
}

void RWHVQtWidget::touchPointCopyMousePosToPointStartPos(QTouchEvent::TouchPoint &point, const QGraphicsSceneMouseEvent *event)
{
  point.setStartPos(event->pos());
  point.setStartScenePos(event->scenePos());
  point.setStartScreenPos(event->screenPos());
}

void RWHVQtWidget::touchPointMirrorMousePosToPointPos(QTouchEvent::TouchPoint &point, const QGraphicsSceneMouseEvent *event)
{
  if (scene()->views().size() > 0) {
    QPointF windowPos(scene()->views().at(0)->pos());
    QSize resolution = qApp->desktop()->size();
    QPointF centerPoint(resolution.width() / 2, resolution.height() / 2);

    QPointF mirrorPoint = centerPoint + (centerPoint - event->screenPos() + windowPos);

    point.setPos(mirrorPoint);
    point.setScenePos(mirrorPoint);
    point.setScreenPos(mirrorPoint + windowPos);
  }
}

void RWHVQtWidget::touchPointMirrorMousePosToPointStartPos(QTouchEvent::TouchPoint &point, const QGraphicsSceneMouseEvent *event)
{
  if (scene()->views().size() > 0) {
    QPointF windowPos(scene()->views().at(0)->pos());
    QSize resolution = qApp->desktop()->size();
    QPointF centerPoint(resolution.width() / 2, resolution.height() / 2);

    QPointF mirrorPoint = centerPoint + (centerPoint - event->screenPos() + windowPos);

    DLOG(INFO) << "mirrorPoint " << mirrorPoint.x() << " " << mirrorPoint.y();

    point.setStartPos(mirrorPoint);
    point.setStartScenePos(mirrorPoint);
    point.setStartScreenPos(mirrorPoint + windowPos);
  }
}

bool RWHVQtWidget::eventEmulatePinch(QEvent *event)
{
#if !defined(NDEBUG)
  bool sendTouchEvent = false;
  QGraphicsSceneMouseEvent *e = static_cast<QGraphicsSceneMouseEvent*>(event);

  QEvent::Type touchEventType;
  Qt::TouchPointState touchPointState;

  if (QEvent::GraphicsSceneMousePress == event->type()) {
    if (Qt::LeftButton == e->button() && e->modifiers().testFlag(Qt::ControlModifier)) {
      pinchEmulationEnabled = true;

      touchPointMirrorMousePosToPointPos(emuPoint1,e);
      touchPointMirrorMousePosToPointStartPos(emuPoint1,e);
      emuPoint1.setState(Qt::TouchPointPressed);

      touchPointCopyMousePosToPointPos(emuPoint2,e);
      touchPointCopyMousePosToPointStartPos(emuPoint2,e);
      emuPoint2.setState(Qt::TouchPointPressed);

      touchEventType = QEvent::TouchBegin;
      touchPointState = Qt::TouchPointPressed;
      sendTouchEvent = true;
    }
  }

  if (pinchEmulationEnabled && QEvent::GraphicsSceneMouseMove == event->type()) {

    touchPointCopyPosToLastPos(emuPoint1);
    touchPointMirrorMousePosToPointPos(emuPoint1,e);
    emuPoint1.setState(Qt::TouchPointMoved);

    touchPointCopyPosToLastPos(emuPoint2);
    touchPointCopyMousePosToPointPos(emuPoint2,e);
    emuPoint2.setState(Qt::TouchPointMoved);

    touchEventType = QEvent::TouchUpdate;
    touchPointState = Qt::TouchPointMoved;
    sendTouchEvent = true;
  }

  if (pinchEmulationEnabled && QEvent::GraphicsSceneMouseRelease == event->type()) {
    if (Qt::LeftButton == e->button()) {

      touchPointCopyPosToLastPos(emuPoint1);
      emuPoint1.setState(Qt::TouchPointReleased);

      touchPointCopyPosToLastPos(emuPoint2);
      touchPointCopyMousePosToPointPos(emuPoint2,e);
      emuPoint2.setState(Qt::TouchPointReleased);

      touchEventType = QEvent::TouchEnd;
      touchPointState = Qt::TouchPointReleased;
      pinchEmulationEnabled = false;
      sendTouchEvent = true;
    }
  }

  if (sendTouchEvent) {
    QList<QTouchEvent::TouchPoint> touchList;
    touchList.append(emuPoint1);
    touchList.append(emuPoint2);

    QTouchEvent touchEvent(touchEventType, QTouchEvent::TouchPad, Qt::NoModifier, touchPointState, touchList);
    if (scene()->views().size()>0)
    {
      QApplication::sendEvent(scene()->views().at(0)->viewport(), &touchEvent);
      DLOG(INFO) << "QApplication::sendEvent touch event";
    }
    scene()->update();
    return true;
  }
#endif
  return false;
 }

void RWHVQtWidget::setHostView(RenderWidgetHostViewQt* host_view)
{
  host_view_ = host_view;
}

QtMobility::QOrientationReading::Orientation RWHVQtWidget::orientationAngle()
{
  return orientation_angle_;
}

void RWHVQtWidget::setOrientationAngle(QtMobility::QOrientationReading::Orientation angle)
{
  orientation_angle_ = angle;
}


void RWHVQtWidget::onOrientationAngleChanged() {
  LauncherApp* app = reinterpret_cast<LauncherApp*>(qApp);
  int orientation = app->getOrientation();
  QtMobility::QOrientationReading::Orientation angle =
      QtMobility::QOrientationReading::TopUp;

  switch (orientation)
  {
    case 1:
      angle = QtMobility::QOrientationReading::TopUp;
      break;
    case 3:
      angle = QtMobility::QOrientationReading::TopDown;
      break;
    case 2:
      angle = QtMobility::QOrientationReading::LeftUp;
      break;
    case 0:
      angle = QtMobility::QOrientationReading::RightUp;
      break;
    default:
      break;
  }
  setOrientationAngle(angle);
}

void RWHVQtWidget::showEvent(QShowEvent* event)
{
}

void RWHVQtWidget::hideEvent(QHideEvent* event)
{
}

void RWHVQtWidget::focusInEvent(QFocusEvent* event)
{
  // http://crbug.com/13389
  // If the cursor is in the render view, fake a mouse move event so that
  // webkit updates its state. Otherwise webkit might think the cursor is
  // somewhere it's not.

  ///\todo Check whether we need a fake mouse move in focus in event in QT or not

  QInputContext *ic = qApp->inputContext();
  ic->reset();
  hostView()->GetRenderWidgetHost()->SetInputMethodActive(true);
  if (im_enabled_) {
      ic->setFocusWidget(qApp->focusWidget());
      QEvent sip_request(QEvent::RequestSoftwareInputPanel);
      ic->setFocusWidget(qApp->focusWidget());
      ic->filterEvent(&sip_request);
  }
  hostView()->ShowCurrentCursor();
  hostView()->GetRenderWidgetHost()->GotFocus();
  vkb_flag_ = false;
  event->accept();
}

void RWHVQtWidget::focusOutEvent(QFocusEvent* event)
{
  // walkaround for focus issue with MTF VKB
  // when focus out, it will not get focus until a mouse press event set it back
  setFocusPolicy(Qt::NoFocus);
  
  // If we are showing a context menu, maintain the illusion that webkit has
  // focus.

  if (!hostView()->is_showing_context_menu_)
    hostView()->GetRenderWidgetHost()->Blur();

  QInputContext *ic = qApp->inputContext();
  ic->reset();
  QEvent sip_request(QEvent::CloseSoftwareInputPanel);
  ic->filterEvent(&sip_request);
  hostView()->GetRenderWidgetHost()->SetInputMethodActive(false);
  vkb_flag_ = false;
  event->accept();
  return;
}

void RWHVQtWidget::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
  DNOTIMPLEMENTED();
  //hostView()->OnEnterNotifyEvent(aEvent);
}

void RWHVQtWidget::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
  DNOTIMPLEMENTED();
  //hostView()->OnLeaveNotifyEvent(aEvent);
}

void RWHVQtWidget::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
  DNOTIMPLEMENTED();
  //hostView()->OnMoveEvent(aEvent);
}

void RWHVQtWidget::keyPressEvent(QKeyEvent* event)
{
  onKeyPressReleaseEvent(event);
}

void RWHVQtWidget::keyReleaseEvent(QKeyEvent* event)
{
  onKeyPressReleaseEvent(event);
}

void RWHVQtWidget::onKeyPressReleaseEvent(QKeyEvent* event)
{
  NativeWebKeyboardEvent nwke(event);
  hostView()->ForwardKeyboardEvent(nwke);

  ///\todo: Webkit need a keydown , char, keyup event to input a key,
  ///so we send keypress event a second time while modify it to a char event.
  ///Need to fix this when we take Input method into account.

  ///\todo: i don't know why KeyPress cannot be compiled
  if (event->type() == (QEvent::Type)6 /*QEvent::KeyPress*/) {
    nwke.type = WebKit::WebInputEvent::Char;
    hostView()->ForwardKeyboardEvent(nwke);
  }
  event->accept();
}

void RWHVQtWidget::inputMethodEvent(QInputMethodEvent *event)
{
  QString preedit = event->preeditString();
  QString commit_string = event->commitString();
  int replacement_length = event->replacementLength();
  int replacement_start = event->replacementStart();
  int cursor_pos = 0;
  QList<QInputMethodEvent::Attribute> attributes;

/*!
 *\todo 1. need to handle preedit string's attributes
 *\todo 2. need to disable inputMethodEvent in non text entry
 *\todo 3. need to dealing with rare case that im event not arrive with correct sequence.
 *\todo 4. need to handle QInputMethodEvent.replacementLength and QInputMethodEvent.replacementStart
*/

  attributes = event->attributes();
  for (int i = 0; i < attributes.size(); ++i) {
    const QInputMethodEvent::Attribute& a = attributes.at(i);
    switch (a.type) {
      case QInputMethodEvent::TextFormat:
        break;
      case QInputMethodEvent::Cursor:
        //TODO: need to handle .length and .value parameter.
        //LOG(INFO) << "a.length : " << a.length << " ,a.start: " << a.start << std::endl;
        cursor_pos = a.start;
        break;
      case QInputMethodEvent::Selection:
        break;
    }
  }

  if (replacement_length)
    DNOTIMPLEMENTED();

  if (!commit_string.isEmpty())
    hostView()->GetRenderWidgetHost()->ImeConfirmComposition(
        commit_string.utf16());

  if (!preedit.isEmpty()) {
    ///\todo no ImeSetComposition
    //hostView()->GetRenderWidgetHost()->ImeSetComposition(
    //    preedit.utf16(), cursor_pos, -1, -1);
  } else {
    hostView()->GetRenderWidgetHost()->ImeCancelComposition();
  }
}

void RWHVQtWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget * widget)
{
  RenderWidgetHost *host;

  if (!hostView())
    return;

  host = hostView()->host_;
  if(!host) return;

  BackingStoreX* backing_store = static_cast<BackingStoreX*>(
      host->GetBackingStore(true));

  ///\todo A little bit dirty here. It's said that Calling GetBackingStore maybe have changed |invalid_rect_|
  /// So we have to refer back to the invalid_rect_ and union it with exposedRect
  /// And then we need to set invalid_rect_ to 0 in case we get paint again due to window move etc.
  /// Might need to find a better solution!!!
  QRectF invalid_rect = QRectF(hostView()->invalid_rect_.x(),
      hostView()->invalid_rect_.y(),
      hostView()->invalid_rect_.width(),
      hostView()->invalid_rect_.height());

  hostView()->invalid_rect_ = gfx::Rect(0, 0, 0, 0);

  ///\todo It's said that ::exposeRect is only initialized when QGraphicsItem::ItemUsedExtendedStyleOption flag is set
  /// Need to checkout is it always valid for MWidget

  QRectF exposed_rect = option->exposedRect;

#if defined(TILED_BACKING_STORE)
  if (backing_store)
    backing_store->AdjustTiles();
#endif

  QRectF paint_rect = exposed_rect;
  paint_rect |= invalid_rect;

  // Calling GetBackingStore maybe have changed |invalid_rect_|...
  hostView()->about_to_validate_and_paint_ = false;

  if (backing_store) {
    // Only render the widget if it is attached to a window; there's a short
    // period where this object isn't attached to a window but hasn't been
    // Destroy()ed yet and it receives paint messages...

    if (hold_paint_) {
      hold_paint_ = false;
    }else if (painter) {
      backing_store->QPainterShowRect(painter, paint_rect);
    }
  } else {
    DNOTIMPLEMENTED();
  }

  if (pinchEmulationEnabled)
  {
    painter->drawEllipse(emuPoint1.scenePos(), 50.0, 50.0);
    painter->drawEllipse(emuPoint2.scenePos(), 50.0, 50.0);
  }
}

bool RWHVQtWidget::inScrollableArea()
{
  int node_info = hostView()->webkit_node_info_;
  
  if (hostView()->IsPopup())
    return false;

  return (node_info & WebKit::WebWidget::NODE_INFO_IS_SCROLLABLE_AREA);
}

bool RWHVQtWidget::shouldDeliverMouseMove()
{
  int node_info = hostView()->webkit_node_info_;
  
  if (hostView()->IsPopup())
    return false;

  return (node_info & (WebKit::WebWidget::NODE_INFO_IS_EMBEDDED_OBJECT
                       | WebKit::WebWidget::NODE_INFO_IS_EDITABLE));
}


// check whether we delive touch move by emulating from mouse move 
bool RWHVQtWidget::shouldDeliverTouchMove()
{
  int node_info = hostView()->webkit_node_info_;
  
  if (hostView()->IsPopup())
    return false;

  // only deliver the touch move event if and only if node has touch listener
  return (node_info & WebKit::WebWidget::NODE_INFO_HAS_TOUCH_LISTENER);
}

void RWHVQtWidget::deliverMousePressEvent()
{
  if (mouse_press_event_delivered_)
    return;

  mouse_press_event_delivered_ = true;
  if(hostView()->host_)
    hostView()->host_->ForwardMouseEvent(mouse_press_event_);
}

void RWHVQtWidget::gestureEvent(QGestureEvent *event)
{
  foreach(QGesture* gesture, event->gestures()) {
    if (Qt::TapAndHoldGesture == gesture->gestureType()) {
      QTapAndHoldGesture* tapAndHoldState = static_cast<QTapAndHoldGesture *>(gesture);
      tapAndHoldGestureEvent(event,tapAndHoldState);
    } else if (Qt::PanGesture == gesture->gestureType()) {
      QPanGesture* panState = static_cast<QPanGesture *>(gesture);
      panGestureEvent(event,panState);
    } else if (Qt::PinchGesture == gesture->gestureType()) {
      QPinchGesture* pinchState = static_cast<QPinchGesture *>(gesture);
      pinchGestureEvent(event,pinchState);
    }
  }
}

bool RWHVQtWidget::event(QEvent *event)
{
  if (!installed_filter_ && scene()) {
      scene()->installEventFilter(this);
      installed_filter_ = true;
  }
  switch (event->type()) {
  // case QEvent::GraphicsSceneMouseDoubleClick:
  //   // DLOG(INFO) << "==>" << __PRETTY_FUNCTION__ <<
  //   //   " GraphicsSceneMouseDoubleClick" << std::endl;
  //   {
  //     QGraphicsSceneMouseEvent* e = (QGraphicsSceneMouseEvent*)event;
  //     doubleTapAction(e->pos());
  //   }
  //   break;
  case QEvent::Gesture:
    gestureEvent(static_cast<QGestureEvent*>(event));
    break;
  case QEvent::TouchBegin:
    if(acceptTouchEvents()) {
      event->setAccepted(true);
      return true;
    }
    break;
  }

  return QGraphicsWidget::event(event);
}

void RWHVQtWidget::imeUpdateTextInputState(WebKit::WebTextInputType type, const gfx::Rect& caret_rect) {
  if (!hasFocus())
    return;
  DLOG(INFO) << "imUpdateStatus x,y,w,h = " << caret_rect.x() << " - "
    << caret_rect.y() << " - "
    << caret_rect.width() << " - "
    << caret_rect.height() << " - "
    << std::endl;

  cursor_rect_ = QRect(caret_rect.x(), caret_rect.y(), caret_rect.width(), caret_rect.height());

  QInputContext *ic = qApp->inputContext();
  // FIXME: if we got unconfirmed composition text, and we try to move cursor
  // from one text entry to another, the unconfirmed composition text will be cancelled
  // but the focus will not move, unless you click another entry again.
  // This bug also exist in GTK code.

  is_enabled_ = (type != WebKit::WebTextInputTypeNone);
  
  if (type == WebKit::WebTextInputTypeNumber) {
    setInputMethodHints(Qt::ImhDigitsOnly);
  } else if (type == WebKit::WebTextInputTypeTelephone){
    setInputMethodHints(Qt::ImhDialableCharactersOnly);
  } else if (type == WebKit::WebTextInputTypeEmail) {
    setInputMethodHints(Qt::ImhEmailCharactersOnly);
  } else if (type == WebKit::WebTextInputTypeUrl) {
    setInputMethodHints(Qt::ImhUrlCharactersOnly);
  } else {
    setInputMethodHints(Qt::ImhNone);
  }

  if (!is_enabled_) {
    if (im_enabled_) {
      ic->reset();
      setFlag(QGraphicsItem::ItemAcceptsInputMethod, false);
      QEvent sip_request(QEvent::CloseSoftwareInputPanel);
      ic->filterEvent(&sip_request);
      im_enabled_ = false;
    }
  } else {
    // Enable the InputMethod if it's not enabled yet.
    if (!im_enabled_) {
      ic->reset();
      setFlag(QGraphicsItem::ItemAcceptsInputMethod, true);
      QEvent sip_request(QEvent::RequestSoftwareInputPanel);
      ic->setFocusWidget(qApp->focusWidget());
      ic->filterEvent(&sip_request);
      im_enabled_ = true;
    } else {
      scrollAndZoomForTextInput(cursor_rect_, false);
    }
  }
  
  if (type == WebKit::WebTextInputTypePassword) {
    setInputMethodHints(inputMethodHints() | Qt::ImhHiddenText | Qt::ImhNoPredictiveText );
  } else {
    setInputMethodHints(inputMethodHints() & ~(Qt::ImhHiddenText | Qt::ImhNoPredictiveText) );
  }

  if(hostView()->host_) {
    hostView()->host_->QueryEditorCursorPosition(im_cursor_pos_);
    hostView()->host_->QueryEditorSelection(im_selection_);
    hostView()->host_->QueryEditorSurroundingText(im_surrounding_);
  }
  vkb_flag_ = true;
  ic->update();
}

void RWHVQtWidget::handleInputMethodAreaChanged(const QRect &newArea) {
  if (!vkb_flag_)
    return;
  vkb_height_ = newArea.height();
  scrollAndZoomForTextInput(cursor_rect_, true);
}

void RWHVQtWidget::scrollAndZoomForTextInput(const QRect& caret_rect, bool animation)
{
  if (vkb_height_ == 0) 
    return;

  RenderWidgetHost* host = hostView()->host_;
  if(!host) return;

  QGraphicsObject* webview = GetWebViewItem();
  QGraphicsObject* viewport_item = GetViewportItem();
  if (!webview || !viewport_item)
    return;
  int web_x = viewport_item->property("contentX").toInt();
  int web_y = viewport_item->property("contentY").toInt();
  int web_width = viewport_item->property("width").toInt();
  int web_height = viewport_item->property("height").toInt(); 
  int height_threshold = web_height/20;
//  if (vkb_height <= 0) vkb_height = web_height/3;
/*
  if (caret_rect.height()*scale_ + 0.5 < height_threshold) {
    //Here caret_rect.height()-1 is calculated, because first cursor always larger 1 than later cursor.
    double factor;
    factor = height_threshold*1.0/((caret_rect.height()-1)*scale_);

    //Get the TopLeft positon
    int middle_height = (web_height - vkb_height - caret_rect.height()*scale_*factor)/2;
    int x = 0;
    int y = 0;

    if (caret_rect.y()*scale_ > (web_y + middle_height)) {
      y = middle_height + caret_rect.y()*scale_*(factor-1);
      qCritical() << "scroll up";
    } else if (caret_rect.y()*scale_ < web_y) {
      y = (caret_rect.y()*scale_-50)*factor > 0? (caret_rect.y()*scale_-50)*factor:0;
    } 
    if (caret_rect.x()*scale_*factor < web_x*factor) {
      x = (caret_rect.x()*scale_-80)*factor > 0? (caret_rect.x()*scale_-80)*factor:0;
    } else if(caret_rect.x()*scale_*factor > web_x*factor + web_width) {
      x = (web_x + 50)*factor;
    } else {
      x = caret_rect.x()*scale_*(factor - 1);
    }
    topLeft_ = QPointF(-x, -3000);
    //Pinch
    pinch_start_pos_ = QPointF(-web_x, -web_y);
    qCritical() << -web_x << "  " << -web_y;
    pinch_scale_factor_ = factor;
    pending_scale_ = scale_*factor;
    //setting the pinch_center_
    pinch_center_=QPointF(0, 0);
    setTransformOriginPoint(QPointF(0, 0));	
    pinch_completing_ = true;
    BackingStoreX* backing_store = static_cast<BackingStoreX*>(
      host->GetBackingStore(false));
    backing_store->SetFrozen(true);
    setScale(factor);
    onAnimationFinished();
  }*/ 
  // only scroll the web
  if(scroll_animation_ == NULL) {
    scroll_animation_ = new QPropertyAnimation(viewport_item, "contentY", this);
    QEasingCurve curve_scroll(QEasingCurve::Linear);
    scroll_animation_->setEasingCurve(curve_scroll);
    scroll_animation_->setDuration(kScrollDuration);
    scroll_animation_->setEndValue(0);
    scroll_animation_->setStartValue(0);
  }
  int middle_height = (web_height - vkb_height_ - caret_rect.height()*scale_)/2;
  if (caret_rect.y()*scale_ > (web_y + middle_height)) {
    if (animation) {
      scroll_animation_->stop();
      scroll_animation_->setStartValue(web_y);
      scroll_animation_->setEndValue(caret_rect.y()*scale_ - middle_height);
      scroll_animation_->start();
    } else {
      viewport_item->setProperty("contentY", QVariant(caret_rect.y()*scale_ - middle_height));
    }
  } else if (caret_rect.y()*scale_ < web_y) {
    if (animation) {
      scroll_animation_->stop();
      scroll_animation_->setStartValue(web_y);
      scroll_animation_->setEndValue(caret_rect.y()*scale_-50>0? caret_rect.y()*scale_-50:0);
      scroll_animation_->start();
    } else {
      viewport_item->setProperty("contentY", QVariant(caret_rect.y()*scale_-50>0? caret_rect.y()*scale_-50:0));
    }
  }
  if (caret_rect.x()*scale_ < web_x) {
    viewport_item->setProperty("contentX", QVariant(caret_rect.x()*scale_-80>0? caret_rect.x()*scale_-80:0));
  } else if(caret_rect.x()*scale_ > web_x + web_width) {
    viewport_item->setProperty("contentX", QVariant(web_x + 50));
  } 
}

void RWHVQtWidget::imeCancelComposition() {
  if (!im_enabled_)
    return;

  QInputContext *ic = qApp->inputContext();
  ic->reset();
  ///\todo Seems this only happen when webkit could not handle the composition event.
  // If it happened, anything else we need to do?

}

void RWHVQtWidget::resizeEvent(QGraphicsSceneResizeEvent* event)
{
  DLOG(INFO) << "Should we call this?";

  ///\todo We should not use this resize event to resize RWHV
  /// Instead Tab contents should call RWHV->setSize directly.
  /// to remove;
  //hostView()->SetSize(gfx::Size(event->newSize().width(), event->newSize().height()));
}

void RWHVQtWidget::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
  DLOG(INFO) << "--" << __PRETTY_FUNCTION__ << ": " <<
      " shouldDeliverMouseMove = " << shouldDeliverMouseMove() <<
      " shouldDeliverTouchMove = " << shouldDeliverTouchMove() <<
      " inScrollableArea = " << inScrollableArea() <<
      std::endl;

#if 0
  // Anyone need the touch move event?
  WebKit::WebTouchEvent touchEvent = EventUtilQt::ToWebTouchEvent(event, scale());
  hostView()->host_->ForwardTouchEvent(touchEvent);
#endif

  if (is_modifing_selection_) {
    ModifySelection(current_selection_handler_,
        gfx::Point(static_cast<int>(event->pos().x() / scale()), static_cast<int>(event->pos().y() / scale())));
    goto done;
  }

  // whether forwarding touch move events to WebKit
  if (shouldDeliverTouchMove()) {
    setViewportInteractive(false);
    //temporarily use this flag to disable/enable flickable
    is_inputtext_selection_ = true;
    // forward touch events to the element which listen to touch events
    WebKit::WebTouchEvent touchEvent = EventUtilQt::ToWebTouchEvent(event, scale());
    hostView()->host_->ForwardTouchEvent(touchEvent);
  } else if (shouldDeliverMouseMove()) {
    setViewportInteractive(false);
    // although it may be forwarded to plugin, but it's okay to set this flag
    is_inputtext_selection_ = true;
 
    // send out mouse press event, if it hadn't been sent out.
    deliverMousePressEvent();

    WebKit::WebMouseEvent mouseEvent = EventUtilQt::ToWebMouseEvent(event, scale());
    if(hostView()->host_)
      hostView()->host_->ForwardMouseEvent(mouseEvent);
  } else if (inScrollableArea())
  {
    if (is_scrolling_scrollable_ == false)
    {
      int dx = static_cast<int>(event->pos().x() - event->buttonDownPos(Qt::LeftButton).x());
      int dy = static_cast<int>(event->pos().y() - event->buttonDownPos(Qt::LeftButton).y());
      if (abs(dx) >= 3 || abs(dy) >= 3)
      {
        setViewportInteractive(false);
        is_scrolling_scrollable_ = true;
      }
    }

    if (is_scrolling_scrollable_)
    {
      WebKit::WebMouseWheelEvent wheelEvent = EventUtilQt::ToMouseWheelEvent(
          event, orientationAngle(), scale());
      DLOG(INFO) << "wheelEvent " << wheelEvent.deltaX << " " << wheelEvent.deltaY;
      hostView()->host_->ForwardWheelEvent(wheelEvent);
      cancel_next_mouse_release_event_ = true;
    }
  }

done:
  event->accept();
}

void RWHVQtWidget::mousePressEvent(QGraphicsSceneMouseEvent* event)
{

  WebKit::WebTouchEvent touchEvent = EventUtilQt::ToWebTouchEvent(event, scale());

  if (!hostView()->IsPopup()) {

    setFocusPolicy(Qt::StrongFocus);
    if (!hasFocus()) {
      setFocus();
      QGraphicsItem *parent = parentItem();
      while (parent) {
        if (parent->flags() & QGraphicsItem::ItemIsFocusScope)
          parent->setFocus(Qt::OtherFocusReason);
        parent = parent->parentItem();
      }
    }

    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    if (timestamp - m_dbclkHackTimeStamp < 350) {
      // we may hit a double tap
      qreal length = QLineF(event->pos(), m_dbclkHackPos).length();
      if (length < 40) {
        DLOG(INFO) << "WE HIT A DOUBLE CLICK " << length << std::endl;
        if (!isDoingGesture() && !is_enabled_) {
          doubleTapAction(event->pos());
          if (delay_for_click_timer_->isActive()) {
            delay_for_click_timer_->stop();
          }
        }
        return;
      }
    }

    m_dbclkHackTimeStamp = timestamp;
    m_dbclkHackPos       = event->pos();
  }

  if (in_selection_mode_) {
    // clear double tap information
    m_dbclkHackTimeStamp = 0;
    m_dbclkHackPos = QPointF(0, 0);

    current_selection_handler_ = findSelectionHandler(static_cast<int>(event->pos().x() / scale()), static_cast<int>(event->pos().y() / scale()));
    if (current_selection_handler_ != SELECTION_HANDLER_NONE) {
      is_modifing_selection_ = true;
      setViewportInteractive(false);
      goto done;
    }
  }

  // we send a touch event first to give user a visual feedback on mouse down,
  // but do not do actual mouse down works
  if(hostView()->host_) {
    hostView()->host_->ForwardTouchEvent(touchEvent);

    // Then query the node under current pos
    hostView()->host_->QueryNodeAtPosition(static_cast<int>(event->pos().x() / scale()),
        static_cast<int>(event->pos().y() / scale()));

    // Finally, save the mouse press event for later usage
    mouse_press_event_ = EventUtilQt::ToWebMouseEvent(event, scale());
    mouse_press_event_delivered_ = false;
    cancel_next_mouse_release_event_ = false;

    //QGraphicsItem* mouseGrabber = controller->mouseGrabberItem();

    DLOG(INFO) << "--" << __PRETTY_FUNCTION__ << ": " <<
      "host = " << hostView()->host_ <<
      " is popup window:" << hostView()->IsPopup() <<
      ",x: " << mouse_press_event_.x <<
      ",y: " << mouse_press_event_.y <<
      ",gx: " << mouse_press_event_.globalX <<
      ",gy: " << mouse_press_event_.globalY <<
      std::endl;
  }
done:
  event->accept();
}

void RWHVQtWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
/*  if (!hostView()->IsPopup()) {
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    if (timestamp - m_dbclkHackTimeStamp < 350) {
      // we may hit a double tap
      qreal length = QLineF(event->pos(), m_dbclkHackPos).length();
      if (length < 40) {
        DLOG(INFO) << "WE HIT A DOUBLE CLICK " << length << std::endl;
        doubleTapAction(event->pos());

        return;
      }
    }

    m_dbclkHackTimeStamp = timestamp;
    m_dbclkHackPos       = event->pos();
  }
*/
  WebKit::WebTouchEvent touchEvent = EventUtilQt::ToWebTouchEvent(event, scale());
// we don't do normal mouse release event when modifing selection
  if (is_modifing_selection_) {
    CommitSelection();
    is_modifing_selection_ = false;
    current_selection_handler_ = SELECTION_HANDLER_NONE;
    setViewportInteractive(true);
    goto done;
  }

// we send a touch event first to give user a visual feedback on mouse up
  if(hostView()->host_)
    hostView()->host_->ForwardTouchEvent(touchEvent);

  // we clear the TapAndHoldGesture here to prevent a PanGesture been invoked upon the same 
  // touch event of tapAndHoldGesture
  clearDoingGesture(Qt::TapAndHoldGesture);

  if (isDoingGesture())
    return;

  // we don't want to block mouse release event for popup
  if (hostView()->IsPopup()) {
    deliverMousePressEvent();
    mouse_release_event_ = EventUtilQt::ToWebMouseEvent(event, scale());
    if(hostView()->host_) {
      hostView()->host_->ForwardMouseEvent(mouse_release_event_);
    }

    delay_for_click_timer_->stop();
    goto done;
  }


// If no gesture is going on, it means that we are doing a short click
  if (!cancel_next_mouse_release_event_) {
    if (!delay_for_click_timer_->isActive()) {
    // send out mouse press event, if it hadn't been sent out.
    deliverMousePressEvent();
    // send out mouse release event
    mouse_release_event_ = EventUtilQt::ToWebMouseEvent(event, scale());
    delay_for_click_timer_->start(350); 
    }  else {
      delay_for_click_timer_->stop();
    }
  } else {
    ///\bug If we are doing gesture on a button in the page
    ///the bug will keep press down status since we cancel the mouse release event.
    cancel_next_mouse_release_event_= false;
  }

  if(is_inputtext_selection_ || in_selection_mode_) {
    is_inputtext_selection_ = false;
    setViewportInteractive(true);
  }

  if (is_scrolling_scrollable_)
  {
    is_scrolling_scrollable_ = false;
    setViewportInteractive(true);
  }
done:
  event->accept();
}

void RWHVQtWidget::CommitSelection() {
  RenderViewHost* rvh = reinterpret_cast<RenderViewHost*>(hostView()->host_);
  if (!rvh)
    return;
  rvh->CommitSelection();
}

void RWHVQtWidget::onClicked()
{
  // send out mouse press and release event in pair
  deliverMousePressEvent();
  if(hostView()->host_) {
    hostView()->host_->ForwardMouseEvent(mouse_release_event_);
    delay_for_click_timer_->stop();
  }
}

void RWHVQtWidget::tapAndHoldGestureEvent(QGestureEvent* event, QTapAndHoldGesture* gesture)
{
  switch (gesture->state())
  {
    case Qt::GestureStarted:
      setDoingGesture(Qt::TapAndHoldGesture);
      break;
    case Qt::GestureUpdated:
      ///\todo do we need a ui indicator here for tap and hold gesture?
      setDoingGesture(Qt::TapAndHoldGesture);
      break;
    case Qt::GestureFinished:
      if (isDoingGesture(Qt::TapAndHoldGesture)) {
        // don't start another selection upon longpress when the previous one is still on going.
        if (!(in_selection_mode_ ||
              (hostView()->webkit_node_info_ & WebKit::WebWidget::NODE_INFO_IS_EDITABLE)))
	  InvokeSelection(gesture);
        // we might need to ignore this when other higher priority gesture is on going.
        fakeMouseRightButtonClick(event, gesture);
        ///\todo to trigger the context menu according to Dom item.
        cancel_next_mouse_release_event_ = true;
      }
      // we don't do clearDoingGesture(Qt::TapAndHoldGesture) here to prevent pan gesture from been invoked.
      // we will do  clearDoingGesture(Qt::TapAndHoldGesture) upon next mouse release event.
      break;
    case Qt::GestureCanceled:
      clearDoingGesture(Qt::TapAndHoldGesture);
      break;
    default:
      break;
  }

  event->accept();
}

void RWHVQtWidget::panGestureEvent(QGestureEvent* event, QPanGesture* gesture)
{
  DLOG(INFO) << "--" << __PRETTY_FUNCTION__ << ": " <<
      std::endl;

  if (is_modifing_selection_)
    return;

  if (!is_scrolling_scrollable_)
    return;

  //ignore pan gesture when doing TapAndHold Gesture
  if (isDoingGesture(Qt::TapAndHoldGesture)) {
    clearDoingGesture(Qt::PanGesture);
    return;
  }

///\todo fixme on orientation angle;
  WebKit::WebMouseWheelEvent wheelEvent = EventUtilQt::ToMouseWheelEvent(
      event, gesture, hostView()->native_view(), orientationAngle());
//  WebKit::WebMouseWheelEvent wheelEvent = EventUtilQt::ToMouseWheelEvent(
//      event, gesture, hostView()->native_view());

  if (shouldDeliverMouseMove() ||
      shouldDeliverTouchMove()) {
      cancel_next_mouse_release_event_ = false;
      return;
  }

  last_pan_wheel_event_ = wheelEvent;

  switch (gesture->state())
  {
    case Qt::GestureStarted:
      setDoingGesture(Qt::PanGesture);
      //gesture->setGestureCancelPolicy(QGesture::CancelAllInContext);
      break;
    case Qt::GestureUpdated:
      setDoingGesture(Qt::PanGesture);
      break;
    case Qt::GestureFinished:
      if (isDoingGesture(Qt::PanGesture)) {
        cancel_next_mouse_release_event_ = true;
      }
      clearDoingGesture(Qt::PanGesture);
      // resetting the double click timer
      m_dbclkHackTimeStamp = 0;
      break;
    case Qt::GestureCanceled:
      clearDoingGesture(Qt::PanGesture);
      break;
    default:
      break;
  }

  if(hostView()->host_)
    hostView()->host_->ForwardWheelEvent(wheelEvent);
  event->accept();
}

void RWHVQtWidget::onAnimationFinished()
{
  QGraphicsObject* viewport_item = GetViewportItem();
  if (!viewport_item)
    return;
 
  if((scale_ == kNormalContentsScale 
      && pending_scale_ < kNormalContentsScale) 
     || (scale_ == kMaxContentsScale 
         && pending_scale_ > kMaxContentsScale)) {
    pinch_completing_ = false;
    setViewportInteractive(true);
  }
 
  if(pending_scale_ < kNormalContentsScale)
  {
    host_view_->host_->SetScaleFactor(kNormalContentsScale);
    pinch_scale_factor_ = kNormalContentsScale/scale_;
    pending_scale_ = kNormalContentsScale;
  } else if(pending_scale_ > kMaxContentsScale) {
    host_view_->host_->SetScaleFactor(kMaxContentsScale);
    pinch_scale_factor_ = kMaxContentsScale/scale_;
    pending_scale_ = kMaxContentsScale;
  } else {
    host_view_->host_->SetScaleFactor(pending_scale_);
  }
 
  this->SetScaleFactor(pending_scale_);
 
  if (viewport_item)
  {
  //   topLeft_ = QPointF(-viewport_item->property("contentX").toInt(),
  //                  -viewport_item->property("contentY").toInt());
    //DLOG(INFO) << "Web view top left " << topLeft.x()
    //           << " " << topLeft.y();
    DLOG(INFO) << "Web view pinch start top left" << pinch_start_pos_.x()
               << " " << pinch_start_pos_.y();
    QPointF center = viewport_item->mapFromScene(pinch_center_);
    DLOG(INFO) << "Web view pinch center " << center.x() << " " << center.y();
    QPointF distance = pinch_start_pos_ - center;
  
   pending_webview_rect_ = QRectF(
        QPointF(distance * pinch_scale_factor_ + center + (topLeft_- pinch_start_pos_)),
        size() * pinch_scale_factor_);
 }

  UnFrozen();
}

void RWHVQtWidget::pinchGestureEvent(QGestureEvent* event, QPinchGesture* gesture)
{
  QPointF pos;
  if (is_modifing_selection_)
    return;

  if (shouldDeliverMouseMove()) {
      cancel_next_mouse_release_event_ = false;
      return;
  }

  RenderWidgetHost *host = hostView()->host_;
  if(!host) return;

  BackingStoreX* backing_store = static_cast<BackingStoreX*>(
      host->GetBackingStore(false));
  
  QGraphicsObject* viewport_item = GetViewportItem();

  switch (gesture->state())
  {
    case Qt::GestureStarted:
      {
        pinch_scale_factor_ = kNormalContentsScale;
        pending_scale_ = scale_;
        topLeft_ = QPointF(-viewport_item->property("contentX").toInt(),
                    -viewport_item->property("contentY").toInt());

        gesture->setGestureCancelPolicy(QGesture::CancelAllInContext);
        setDoingGesture(Qt::PinchGesture);
        if (delay_for_click_timer_->isActive()) {
            delay_for_click_timer_->stop();
        }
       
  
        if (backing_store)
          backing_store->SetFrozen(true);

        pinch_center_ = gesture->centerPoint();
        QPointF center = mapFromScene(pinch_center_);
        setTransformOriginPoint(center);
        
        cancel_next_mouse_release_event_ = true;
        if (viewport_item)
        {
          pinch_start_pos_ = QPointF(-viewport_item->property("contentX").toInt(),
                                     -viewport_item->property("contentY").toInt());
         }
        //TODO: enable interactive when doing pinch
        //Currently we disable it for we're confused by native rwhv gestures and Flickable gestures.
        //Flickable element will cause pinch jump when the pinch finger is firstly pressed first released
        setViewportInteractive(false);
      }
      break;
    case Qt::GestureUpdated:
      {
        setDoingGesture(Qt::PinchGesture);
        if (delay_for_click_timer_->isActive()) {
            delay_for_click_timer_->stop();
         }
        
        pinch_scale_factor_ = gesture->totalScaleFactor();
        if(pinch_scale_factor_ * scale_ > kMaxPinchScale)
          pinch_scale_factor_ = kMaxPinchScale / scale_;

        if(pinch_scale_factor_ * scale_ < kMinPinchScale) 
          pinch_scale_factor_ = kMinPinchScale / scale_;;

        // adjust pending scale
        pending_scale_ = flatScaleByStep(scale_ * pinch_scale_factor_);
        // re-set pinch_scale_factor after adjusting pending_scale for 
        // we guarantee pending_scale_ is flatten
        pinch_scale_factor_ = pending_scale_ / scale_;
        setScale(pinch_scale_factor_);

        if(pending_scale_ < kNormalContentsScale 
            || pending_scale_ > kMaxContentsScale) {
          rebounce_animation_->setStartValue(pinch_scale_factor_);
        } 

        cancel_next_mouse_release_event_ = true;
      }
      break;
    case Qt::GestureFinished:
      {
        pinch_completing_ = true;
        cancel_next_mouse_release_event_ = true;
        clearDoingGesture(Qt::PinchGesture);
        if (delay_for_click_timer_->isActive()) {
            delay_for_click_timer_->stop();
        }

        if(pending_scale_ < kNormalContentsScale) 
        {
          rebounce_animation_->setStartValue(pinch_scale_factor_);
          rebounce_animation_->setEndValue(kNormalContentsScale/scale_);
          rebounce_animation_->start();
        } else if(pending_scale_ > kMaxContentsScale) {
          rebounce_animation_->setStartValue(pinch_scale_factor_);
          rebounce_animation_->setEndValue(kMaxContentsScale/scale_);
          rebounce_animation_->start();
        } else {
          onAnimationFinished();
        }
      }
      break;
    case Qt::GestureCanceled:
      {
        clearDoingGesture(Qt::PinchGesture);
        
        if (backing_store)
          backing_store->SetFrozen(false);
        
        setViewportInteractive(true);
      }

      break;
    default:
      break;
  }

  DLOG(INFO) << "--" << __PRETTY_FUNCTION__ << ": " <<
    ", scaleFactor: " << gesture->scaleFactor() <<
    ", totalScaleFactor: " << gesture->totalScaleFactor() <<
    ", centerPoint x-y:" << gesture->centerPoint().x() << "-" << gesture->centerPoint().y() <<
    ", rotationAngle: " << gesture->rotationAngle() <<
    std::endl;
}

void RWHVQtWidget::SetScaleFactor(double scale)
{
  if(scale_ == scale) return;
  scale_ = scale;

  RenderWidgetHost *host = hostView()->host_;

  if(host) {
    host->SetScaleFactor(scale);
    BackingStoreX* backing_store = static_cast<BackingStoreX*>(
        host->GetBackingStore(false));
    if (backing_store)
      backing_store->SetContentsScale(scale_);
  }
}

QVariant RWHVQtWidget::inputMethodQuery(Qt::InputMethodQuery query) const
{
  ///\todo wait for implement, need to report correct MicroFocus
  switch ((int)query) {
    case Qt::ImMicroFocus:
      return QVariant(cursor_rect_);

    case Qt::ImCursorPosition:
      return QVariant(im_cursor_pos_);

    case Qt::ImCurrentSelection:
      return QVariant(QString::fromStdString(im_selection_));

    case Qt::ImSurroundingText:
      return QVariant(QString::fromStdString(im_surrounding_));

    default:
      return QVariant();
  }
}

// Handler of double tap:
// When user double-tap at the postion (x,y), browser sends
// a query request to render process to get the element area
// at (x, y), and the best scale factor is calculated from
// the element area, screen size and webview size. 
void RWHVQtWidget::doubleTapAction(const QPointF& pos)
{
  if(is_doing_double_tap_) return;
  
  RenderWidgetHost* host = hostView()->host_;
  if(!host) return;
  
  gfx::Rect rect;
  QGraphicsObject* viewport_item = GetViewportItem();
  QGraphicsObject* webview_item = GetWebViewItem();

  QPointF click_pos = viewport_item->mapFromItem(webview_item, pos);

  // Get the viewport rectangle of flickable widget
  QRectF viewport_rect = viewport_item->boundingRect();
  // Get the web view rectagle
  QRectF webview_rect = webview_item->boundingRect();
 
  // Query the element area at the click point
  gfx::Size real_size = gfx::Size(viewport_rect.width()/kMaxDoubleTapScale, 
                                  viewport_rect.height()/kMaxDoubleTapScale);
  host->QueryElementAreaAt(gfx::Point(pos.x()/scale_, pos.y()/scale_), 
                           real_size, rect);
                           
  // Freeze the backing store and prepare scaling RWHV widget
  BackingStoreX* backing_store = static_cast<BackingStoreX*>(
      host->GetBackingStore(false));
  backing_store->SetFrozen(true);

  // Remember content XY of flickable
  pinch_start_pos_ = QPointF(-viewport_item->property("contentX").toInt(),
                             -viewport_item->property("contentY").toInt());

  pending_scale_ = viewport_rect.width()/(rect.width()*scale_);

  pending_scale_ = qMin((qreal)flatScaleByStep(pending_scale_), kMaxDoubleTapScale);

  if(pending_scale_ < kNormalContentsScale 
      && scale_ == kNormalContentsScale) return;

  pinch_scale_factor_ = pending_scale_/scale_;

  QRectF r0; // area of viewport rectangle, says screen area
  QRectF r1; // area of element to be zoomed in/out
  QRectF r2; // area of webview rectangle
  QPointF pCenter; // center point for scaling

  // Zoom out the element
  if(pinch_scale_factor_ < 1.2) {
    pending_scale_ = 1.0;;
    pinch_scale_factor_ = pending_scale_/scale_;
    if(pinch_scale_factor_ == 1.0) return;
    
    r0 = QRectF(viewport_rect.x(), viewport_rect.y(), 
                viewport_rect.width(), viewport_rect.height());

    QPointF elem_topLeft = QPointF(rect.x()*scale_, rect.y()*scale_);
    QPointF p1 = viewport_item->mapFromItem(webview_item, elem_topLeft);
    r1 = QRectF(qMax(p1.x(), 0.0), 
                qMax(p1.y(), 0.0), 
                qMin(rect.width()*scale_,viewport_rect.width()),
                qMin(rect.height()*scale_, viewport_rect.height()));

    QPointF p2 = viewport_item->mapFromItem(webview_item, webview_rect.topLeft());
    r2 = QRectF(p2.x(), p2.y(), webview_rect.width(), webview_rect.height());

    // Horizontal Adjustment
    if(r0.width() == r2.width()) {
      pCenter.setX(click_pos.x());
    }
    // Left overflow of webview is less than the right overflow 
    // relative to viewport
    else if(qAbs(r2.x() - r0.x()) < qAbs(r0.x() + r0.width() - (r2.x() + r2.width())))
    {
      // Use the center of element as scale center point in case
      // of the left edge of webview is still overflow after 
      // zooming out.
      if(qAbs((r1.x() + r1.width()/2 - r2.x())*pinch_scale_factor_) >
          qAbs(r1.x() + r1.width()/2 - r0.x()))
        pCenter.setX(r1.x()+r1.width()/2);
      // Otherwise, calculate the center point to make sure that
      // the left edge of webview can align exactly with the left 
      // edge of viewport rectangle.
      else 
        pCenter.setX((pinch_scale_factor_*r2.x() - r0.x())/(pinch_scale_factor_-1));
    }
    // Left overflow of webview is greater than the right overflow
    // Instead take the right edge into consideration
    else if(qAbs(r2.x() - r0.x()) > 
        qAbs(r0.x() + r0.width() - (r2.x() + r2.width())))
    {
      if(qAbs((r2.x()+r2.width()-r1.x()-r1.width()/2 )*pinch_scale_factor_) >= 
          qAbs(r0.x()+r0.width()-r1.x()-r1.width()/2))
        pCenter.setX(r1.x() + r1.width() /2);
      else
        pCenter.setX((pinch_scale_factor_*(r2.x() + r2.width()) - 
              (r0.x() + r0.width()))/(pinch_scale_factor_-1));
    }
    // In case of left overflow equals to right overflow relative to viewport
    // Use the center point of webview
    else {
      pCenter.setX(r2.x()+r2.width()/2);
    }

    // Vertical adjustment 
    if(r0.height() == r2.height())
    {
      pCenter.setY(click_pos.y());
    }
    else if(qAbs(r2.y() - r0.y()) < 
        qAbs(r0.y() + r0.height() - (r2.y() + r2.height())))
    {
      if(qAbs((r1.y() + r1.height()/2 - r2.y())*pinch_scale_factor_) > 
          qAbs(r1.y() + r1.height()/2 - r0.y()))
        pCenter.setY(r1.y() + r1.height() / 2);
      else
        pCenter.setY((pinch_scale_factor_*r2.y() - r0.y())/(pinch_scale_factor_-1));
    } 
    else if(qAbs(r2.y() - r0.y()) >
        qAbs(r0.y() + r0.height() - (r2.y() + r2.height()))){
      if(qAbs((r2.y()+r2.height()-r1.y()-r1.height()/2 )*pinch_scale_factor_) >= 
          qAbs(r0.y()+r0.height()-r1.y()-r1.height()/2))
        pCenter.setY(r1.y() + r1.height() /2);
      else
        pCenter.setY((pinch_scale_factor_*(r2.y() + r2.height()) -
              (r0.y() + r0.height()))/(pinch_scale_factor_-1));
    } 
    else {
      pCenter.setY(r2.y() + r2.height()/2);
    }
  } 
  // Zoom in the element
  else {
    r0 = QRectF(viewport_rect.x(), viewport_rect.y(), 
                viewport_rect.width(), viewport_rect.height() );

    QPointF elem_topLeft = QPointF(rect.x()*scale_, rect.y()*scale_);
    QPointF p1 = viewport_item->mapFromItem(webview_item, elem_topLeft);
    r1= QRectF(qMax(p1.x(), 0.0), qMax(p1.y(), 0.0), 
              qMin(rect.width()*scale_, viewport_rect.width()),
              qMin(rect.height()*scale_, viewport_rect.height()));

    // Horizontal adjustment
    if(r0.width() == r1.width()) {
      pCenter.setX(click_pos.x());
    }
    else if(qAbs(r1.x() - r0.x()) < 
        qAbs(r0.x() + r0.width() - (r1.x() + r1.width())))
    {
      if(qAbs(r1.width()*(pinch_scale_factor_-1)/2) < qAbs(r1.x() - r0.x()))
        pCenter.setX(r1.x()+r1.width()/2);
      else 
        pCenter.setX((pinch_scale_factor_*r1.x() - r0.x())/(pinch_scale_factor_-1));
    } else if(qAbs(r1.x() - r0.x()) > qAbs(r0.x() + r0.width() - (r1.x() + r1.width()))) {
      if(qAbs(r1.width()*pinch_scale_factor_/2) < 
          qAbs(r0.x() + r0.width() - r1.x() - r1.width()/2))
        pCenter.setX(r1.x() + r1.width() /2);
      else
        pCenter.setX((pinch_scale_factor_*(r1.x() + r1.width()) - 
              (r0.x() + r0.width()))/(pinch_scale_factor_-1));
    } else {
      pCenter.setX(r1.x()+r1.width()/2);
    }

    // Vertical adjustment
    if(r0.height() == r1.height())
    {
      pCenter.setY(click_pos.y());
    }
    else if(qAbs(r1.y() - r0.y()) < 
        qAbs(r0.y() + r0.height() - (r1.y() + r1.height())))
    {
      if(qAbs(r1.height()*(pinch_scale_factor_-1)/2) < qAbs(r1.y() - r0.y()))
        pCenter.setY(r1.y() + r1.height() / 2);
      else
        pCenter.setY((pinch_scale_factor_*r1.y() - r0.y())/(pinch_scale_factor_-1));
    } else if(qAbs(r1.y() - r0.y()) > qAbs(r0.y() + r0.height() - (r1.y() + r1.height()))){
      if(qAbs(r1.height()*pinch_scale_factor_/2) < 
          qAbs(r0.y() + r0.height() - r1.y() - r1.height()/2))
        pCenter.setY(r1.y() + r1.height() /2);
      else
        pCenter.setY((pinch_scale_factor_*(r1.y() + r1.height()) - 
              (r0.y() + r0.height()))/(pinch_scale_factor_-1));
    } else {
      pCenter.setY(r1.y() + r1.height()/2);
    }
  }
  
  is_doing_double_tap_ = true;
 
  // Set scale center point and transform point
  pinch_center_ = pCenter;
  QPointF center = viewport_item->mapToItem(webview_item, pinch_center_);
  setTransformOriginPoint(center);

  topLeft_ = QPointF(-viewport_item->property("contentX").toInt(),
                  -viewport_item->property("contentY").toInt());
   
  host_view_->host_->SetScaleFactor(pending_scale_);

  this->SetScaleFactor(pending_scale_);

  scale_animation_->setDuration(200);
  scale_animation_->setStartValue(1.0);
  scale_animation_->setEndValue(pinch_scale_factor_);
  scale_animation_->start();
  pinch_completing_ = true;
 return;

}

void RWHVQtWidget::onDoubleTapped()
{
  QGraphicsObject* viewport_item = GetViewportItem();
  QGraphicsObject* webview_item = GetWebViewItem();
  if (!viewport_item)
    return;
 
  if (viewport_item)
  {
    QPointF center = pinch_center_;
    DLOG(INFO) << "Web view pinch center " << center.x() << " " << center.y();
    QPointF distance = pinch_start_pos_ - center;
    pending_webview_rect_ = QRectF(
        QPointF(distance * pinch_scale_factor_ + center + (topLeft_ - pinch_start_pos_)),
        size() * pinch_scale_factor_);
   }

  UnFrozen();

  is_doing_double_tap_ = false;
}
 
bool RWHVQtWidget::setDoingGesture(Qt::GestureType type)
{
  ///\todo we might also detect gesture priority here?
  int flag = toGestureFlag(type);
  if (flag == 0)
    return false;

  gesture_flags_ |= flag;
  return true;
}

bool RWHVQtWidget::clearDoingGesture(Qt::GestureType type)
{
  int flag = toGestureFlag(type);
  if (flag == 0)
    return false;

  gesture_flags_ &= ~flag;
  return true;
}

bool RWHVQtWidget::isDoingGesture(Qt::GestureType type)
{
  return ((gesture_flags_ & toGestureFlag(type)) != 0);
}

bool RWHVQtWidget::isDoingGesture()
{
  return (gesture_flags_ != 0);
}

void RWHVQtWidget::fakeMouseRightButtonClick(QGestureEvent* event, QTapAndHoldGesture* gesture)
{
  int globalX = static_cast<int>(gesture->hotSpot().x());
  int globalY = static_cast<int>(gesture->hotSpot().y());
  QPointF pos = hostView()->native_view()->mapFromScene(gesture->position());
  int x = static_cast<int>(pos.x());
  int y = static_cast<int>(pos.y());

  WebKit::WebMouseEvent rightButtonPressEvent = EventUtilQt::ToWebMouseEvent(
      QEvent::GraphicsSceneMousePress,
      Qt::RightButton, Qt::NoModifier,
      x, y, globalX, globalY, scale());
  hostView()->host_->ForwardMouseEvent(rightButtonPressEvent);

  WebKit::WebMouseEvent rightButtonReleaseEvent = EventUtilQt::ToWebMouseEvent(
      QEvent::GraphicsSceneMouseRelease,
      Qt::RightButton, Qt::NoModifier,
      x, y, globalX, globalY, scale());
  hostView()->host_->ForwardMouseEvent(rightButtonReleaseEvent);
}

void RWHVQtWidget::doZoom(qreal factor, QPointF pos, bool reset)
{
// the zoom level calculation and debounce algorithm is simple and stupid here, might need to improve it later?
// zoom in current way is pretty slow ...
  static int accumulated_zoom_level = 0;
  int new_zoom_level;

  const qreal zoom_step = 1.3;
  const qreal zoom_in_thresh_hold = 0.1;
  const qreal zoom_out_thresh_hold = 0.1;

  if (reset) {
    accumulated_zoom_level = 0;
    return;
  }

  if (factor > 1) {
    // we are in zoom in mode
    if (qAbs(factor - std::pow(zoom_step, accumulated_zoom_level)) > zoom_in_thresh_hold) {
      new_zoom_level = log(factor) / log(zoom_step);
    } else {
      return;
    }
  } else {
    qreal i_factor = 1.0 / factor;
    qreal i_previous_factor = 1.0 / std::pow(zoom_step, accumulated_zoom_level);

    if (qAbs(i_factor - i_previous_factor) > zoom_out_thresh_hold) {
      new_zoom_level = log(factor) / log(zoom_step);
    } else {
      return;
    }
  }

  if (new_zoom_level == accumulated_zoom_level)
    return;

  // the zoom function we need are not in RenderWidgetHost class but RenderViewHost class
  RenderViewHost* rvh = reinterpret_cast<RenderViewHost*>(hostView()->host_);
    if (!rvh)
      return;

  int zoom_level_diff = new_zoom_level - accumulated_zoom_level;

  for (int i = 0; i < qAbs(zoom_level_diff); i++) {
    if (zoom_level_diff > 0)
      rvh->Zoom(PageZoom::ZOOM_IN);
    else
      rvh->Zoom(PageZoom::ZOOM_OUT);
  }

  accumulated_zoom_level = new_zoom_level;

}

RWHVQtWidget::SelectionHandlerID RWHVQtWidget::findSelectionHandler(int x, int y) {

  RWHVQtWidget::SelectionHandlerID handler = SELECTION_HANDLER_NONE;
  int s_distance, e_distance;
  int dx = x - selection_start_pos_.x();
  int dy = y - selection_start_pos_.y();

  s_distance = dx * dx + dy * dy;
  if ((dx * dx + dy * dy) < SelectionHandlerRadiusSquare) {
    handler = SELECTION_HANDLER_START;
  }

  dx = x - selection_end_pos_.x();
  dy = y - selection_end_pos_.y();
  e_distance = dx * dx + dy * dy;
  if ((e_distance < SelectionHandlerRadiusSquare)
        && (e_distance < s_distance)) {
      handler = SELECTION_HANDLER_END;
  }

  return handler;
}

void RWHVQtWidget::UpdateSelectionRange(gfx::Point start,
    gfx::Point end, bool set) {
#if 0
  DLOG(INFO) << "--" << __PRETTY_FUNCTION__ << ": " <<
    ", start_pos x-y: " << start.x() << "-" << start.y()<<
    ", end_pos x-y: " << end.x() << "-" << end.y()<<
    ", selection set: " << set <<
    std::endl;
#endif
  if (!set) {
    in_selection_mode_ = false;
    current_selection_handler_ = SELECTION_HANDLER_NONE;
    return;
  }

  in_selection_mode_ = true;
  selection_start_pos_ = start;
  selection_end_pos_ = end;
}

void RWHVQtWidget::InvokeSelection(QTapAndHoldGesture* gesture) {
  RenderViewHost* rvh = reinterpret_cast<RenderViewHost*>(hostView()->host_);
    if (!rvh)
      return;

  QPointF pos = hostView()->native_view()->mapFromScene(gesture->position());
  int x = static_cast<int>(pos.x());
  int y = static_cast<int>(pos.y());
  rvh->SelectItem(gfx::Point(x / scale(), y / scale()));
}

void RWHVQtWidget::ModifySelection(SelectionHandlerID handler, gfx::Point new_pos) {
  RenderViewHost* rvh = reinterpret_cast<RenderViewHost*>(hostView()->host_);
    if (!rvh)
      return;

  if (handler == SELECTION_HANDLER_START) {
    rvh->SetSelectionRange(new_pos, selection_end_pos_, true);
  } else if (handler == SELECTION_HANDLER_END) {
    rvh->SetSelectionRange(selection_start_pos_, new_pos, true);
  }
}


void RWHVQtWidget::onSizeAdjusted()
{
  DLOG(INFO) << "onSizeAdjusted " << this << " "
             << geometry().x() << " "
             << geometry().y() << " "
             << geometry().width() << " "
             << geometry().height();
  QSizeF size(geometry().width(), geometry().height());
  setViewportInteractive(true);
  
  if (previous_size_ != size)
  {
    previous_size_ = size;

    if (pinch_completing_)
    {
      QPointF pos;
      pinch_completing_ = false;

      QGraphicsObject* viewport_item = GetViewportItem();

      if (viewport_item)
      {
        viewport_item->setProperty("contentX", QVariant(-pending_webview_rect_.x()));
        viewport_item->setProperty("contentY", QVariant(-pending_webview_rect_.y()));
        DLOG(INFO) << "set Web View pos " << pending_webview_rect_.x() << " "
                   << pending_webview_rect_.y();
      }
    }
    SetWebViewSize();
  }
}

QGraphicsObject* RWHVQtWidget::GetWebViewItem()
{
  // We have the assumption here that the QML "webView" item won't change on run.
  // If this is not the case later, we might need to fix the code here to refresh the item everytime.

  static QDeclarativeItem *webview_item = NULL;

  if (!webview_item) {
    Browser* browser = BrowserList::GetLastActive();
    BrowserWindowQt* browser_window = (BrowserWindowQt*)browser->window();
    QDeclarativeView *view = browser_window->DeclarativeView();
    webview_item = view->rootObject()->findChild<QDeclarativeItem*>("webView");
  }

  assert(webview_item);
  return webview_item;
}

QGraphicsObject* RWHVQtWidget::GetViewportItem()
{
  static QDeclarativeItem *viewport_item = NULL;

  // We have the assumption here that the QML "innerContent" item won't change on run.
  // If this is not the case later, we might need to fix the code here to refresh the item everytime.

  if (!viewport_item) {
    Browser* browser = BrowserList::GetLastActive();
    BrowserWindowQt* browser_window = (BrowserWindowQt*)browser->window();
    QDeclarativeView *view = browser_window->DeclarativeView();
    viewport_item = view->rootObject()->findChild<QDeclarativeItem*>("innerContent");
  }

  assert(viewport_item);
  return viewport_item;
}

void RWHVQtWidget::SetWebViewSize()
{
  QGraphicsObject* webview = GetWebViewItem();
  if (!webview)
    return;
  
  webview->setProperty("width", QVariant(size().width()));
  webview->setProperty("height", QVariant(size().height()));
  DLOG(INFO) << "set Web View size " << size().width() << " "
             << size().height();
}

void RWHVQtWidget::UnFrozen()
{
  RenderWidgetHost *host = hostView()->host_;
  BackingStoreX* backing_store = static_cast<BackingStoreX*>(
      host->GetBackingStore(false));
  if (backing_store)
  {
    backing_store->SetFrozen(false);
    backing_store->AdjustTiles();
  }
}

void RWHVQtWidget::WasHidden()
{
  QGraphicsObject* viewport_item = GetViewportItem();
  if (viewport_item)
  {
    QVariant contentX = viewport_item->property("contentX");
    QVariant contentY = viewport_item->property("contentY");

    flickable_content_pos_.setX(contentX.toInt());
    flickable_content_pos_.setY(contentY.toInt());
  }
}

void RWHVQtWidget::DidBecomeSelected()
{
  SetWebViewSize();
  QGraphicsObject* viewport = GetViewportItem();
  if(viewport) {
    viewport->setProperty("contentX", QVariant(flickable_content_pos_.x()));
    viewport->setProperty("contentY", QVariant(flickable_content_pos_.y()));
  }
}

QRect RWHVQtWidget::GetVisibleRect()
{
  QGraphicsObject* webview_item = GetWebViewItem();
  QGraphicsObject* viewprot_item = GetViewportItem();

  if (webview_item == NULL || viewprot_item == NULL)
    return QRect();
  
  QRectF itemRect;

  if(hostView()->IsPopup()) {
    itemRect = boundingRect();
    return itemRect.toAlignedRect();
  }

  itemRect = webview_item->boundingRect();

  if (pinch_completing_)
  {
    DLOG(INFO) << "RWHVQtWidget::GetVisibleRect in pending_webview_rect_";
    itemRect = pending_webview_rect_;
  }
  else
  {
    itemRect = webview_item->mapToItem(viewprot_item, itemRect).boundingRect();
  }


  //DLOG(INFO) << "RWHVQtWidget::GetVisibleRect "
  //           << itemRect.x()
  //           << " " << itemRect.y()
  //           << " " << itemRect.width()
  //           << " " << itemRect.height();

  QRectF viewport_rect = viewprot_item->boundingRect();
  itemRect = itemRect.intersected(viewport_rect);

  if (pinch_completing_)
  {
    itemRect = QRectF(-pending_webview_rect_.x(), -pending_webview_rect_.y(), itemRect.width(), itemRect.height());
  }
  else
  {
    itemRect = webview_item->mapFromItem(viewprot_item, itemRect).boundingRect();
  }
  
  return itemRect.toAlignedRect();
}

void RWHVQtWidget::DidBackingStoreScale()
{
  if (pending_webview_rect_ != QRectF())
  {
    DLOG(INFO) << "RWHVQtWidget::DidBackingStoreScale pending webview rect"
               << " " << pending_webview_rect_.width()
               << " " << pending_webview_rect_.height();
    RenderWidgetHost *host = hostView()->host_;
    BackingStoreX* backing_store = static_cast<BackingStoreX*>(
        host->GetBackingStore(false));
    if (backing_store)
    {
      QRect rect = backing_store->ContentsRect();
      setGeometry(QRectF(geometry().topLeft(),
                         QSizeF(rect.width(), rect.height())));
      emit sizeAdjusted();
    }
  }
}

void RWHVQtWidget::AdjustSize()
{
  setGeometry(QRectF(geometry().topLeft(),
                     QSizeF(host_view_->contents_size_.width() * scale(),
                            host_view_->contents_size_.height() * scale())));
  emit sizeAdjusted();
}

void RWHVQtWidget::SetScrollPosition(const gfx::Point& scrollPosition)
{
  QGraphicsObject* viewport = GetViewportItem();
  if(viewport) {
    viewport->setProperty("contentX", QVariant(scrollPosition.x() * scale()));
    viewport->setProperty("contentY", QVariant(scrollPosition.y() * scale()));
  }
}

void RWHVQtWidget::setViewportInteractive(bool interactive)
{
    QGraphicsObject* viewport_item = GetViewportItem();
    if (!viewport_item)
      return;

    viewport_item->setProperty("interactive", QVariant(interactive));
}


#include "moc_rwhv_qt_widget.cc"
