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
#include "content/browser/renderer_host/animation_utils.h"
#include "content/browser/renderer_host/backing_store_x.h"
#include "content/browser/renderer_host/event_util_qt.h"
#include "content/browser/renderer_host/render_view_host.h"
#include "content/browser/renderer_host/render_widget_host.h"
#include "content/browser/renderer_host/render_widget_host_view.h"
#include "content/browser/renderer_host/rwhv_qt_widget.h"
#include "content/common/native_web_keyboard_event.h"

#include <launcherapp.h>

#undef pow
#include <cmath> // for std::pow

namespace {
static const int kSnapshotWebPageWidth = 1500;
static const int kSnapshotWebPageHeight = 2000;

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
  cancel_next_mouse_release_event_ = false;
  mouse_press_event_delivered_ = false;
  pinch_image_ = false;
  hold_paint_ = false;

  selection_start_pos_ = gfx::Point(0, 0);
  selection_end_pos_ = gfx::Point(0, 0);
  in_selection_mode_ = false;
  current_selection_handler_ = SELECTION_HANDLER_NONE;

  m_dbclkHackTimeStamp = 0;
  m_dbclkHackPos = QPointF(0, 0);

  pinch_release_timer_ = new QTimer (this);
  connect(pinch_release_timer_, SIGNAL(timeout()),
          this, SLOT(pinchFinishTimeout()));
  pinch_release_timer_->setSingleShot (true);
  pinch_backing_store_ = (BackingStoreX*)host_view->AllocBackingStore(gfx::Size(kSnapshotWebPageWidth, kSnapshotWebPageHeight));

  auto_pan_ = new PanAnimation;
  cursor_rect_ = QRect();
  connect(auto_pan_, SIGNAL(panTriggered(int, int)), SLOT(autoPanCallback(int, int)));

  LauncherApp* app = reinterpret_cast<LauncherApp*>(qApp);
  connect(app, SIGNAL(orientationChanged()), this, SLOT(onOrientationAngleChanged()));
  onOrientationAngleChanged();

  if (!hostView()->IsPopup()) {
    // we must not grab focus when we are running in a popup mode
    setFocusPolicy(Qt::StrongFocus);

    grabGesture(Qt::PanGesture);
    grabGesture(Qt::TapAndHoldGesture);
    grabGesture(Qt::PinchGesture);
    setAcceptTouchEvents(true);
  } else {
    // We'd better not handle gesture other than Pan in popup mode
    grabGesture(Qt::PanGesture);
    setAcceptTouchEvents(true);
  }
}

RWHVQtWidget::~RWHVQtWidget()
{
  if (auto_pan_)
    delete auto_pan_;
  if (pinch_backing_store_)
    delete pinch_backing_store_;
}


RenderWidgetHostViewQt* RWHVQtWidget::hostView()
{
  return host_view_;
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
      qCritical("focus in");
  }
  hostView()->ShowCurrentCursor();
  hostView()->GetRenderWidgetHost()->GotFocus();

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
  qCritical("focus out");
  hostView()->GetRenderWidgetHost()->SetInputMethodActive(false);
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

void RWHVQtWidget::setPinchImage()
{
  pinch_image_ = true;
}

#ifdef PINCH_FINI_DEBUG
void RWHVQtWidget::update ( qreal x, qreal y, qreal width, qreal height )
{
  QGraphicsWidget::update (x, y, width, height);
  DLOG(INFO) << __PRETTY_FUNCTION__ << " " << x << " " << y <<
    " " << width << " " << height;
}
#endif

void RWHVQtWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget * widget)
{
  RenderWidgetHost *host;
#ifdef PINCH_FINI_DEBUG
  static int counter = 0;
  static int non_pinch_cnt = 0;
  static int rec_cnt = 3;

  counter++;
  DLOG(INFO) << __PRETTY_FUNCTION__ << " counter: " << counter;
#endif

  if (!hostView())
    return;

  if (pinch_image_) {
    QRectF exposed_rect = option->exposedRect;

    double left =  pinch_view_pos_.x() + exposed_rect.x() +
      pinch_center_.x() * (scale_factor_ - 1) / scale_factor_;

    double right = pinch_view_pos_.x() + exposed_rect.right() -
      (exposed_rect.width() - pinch_center_.x()) * (scale_factor_ - 1) / scale_factor_;

    double top = pinch_view_pos_.y() + exposed_rect.y() +
      pinch_center_.y() * (scale_factor_ - 1) / scale_factor_;

    double bottom = pinch_view_pos_.y() + exposed_rect.bottom() -
      (exposed_rect.height() - pinch_center_.y()) * (scale_factor_ - 1) / scale_factor_;


    QRectF source(left, top, abs(right - left), bottom - top);
    if (left < 0) source.moveLeft (0);
    if (top < 0) source.moveTop (0);

    int pinch_width = pinch_backing_store_->size().width();
    int pinch_height = pinch_backing_store_->size().height();

    if (source.right() > pinch_width) {
      double scale_down = (source.right() - pinch_width) / source.width();
      double vert_shrink = scale_down * source.height();
      source.setRight (pinch_width);
      source.setHeight (source.height() - vert_shrink);
    }
    if (source.bottom() > pinch_height) {
      double scale_down = (source.bottom() - pinch_height) / source.height();
      double horz_shrink = scale_down * source.width();
      source.setBottom (pinch_height);
      source.setWidth (source.width() - horz_shrink);
    }

    scale_factor_ = exposed_rect.width() / source.width();

    DLOG(INFO) << " scale factor: " << scale_factor_ << " pinch rects: \n" <<
      exposed_rect.x() << " " << exposed_rect.y() << " " <<
      exposed_rect.width() << " " << exposed_rect.height() << "\n" <<
      source.x() << " " << source.y() << " " <<
      source.width() << " " << source.height() << "\n";

    pinch_backing_store_->QPainterShowRect(painter, exposed_rect, source);
    QRectF* saved_rect = (QRectF*)&pinch_src_rect_;
    *saved_rect = source;
#ifdef PINCH_DEBUG
    QImage dbg_bmp(exposed_rect.width(), exposed_rect.height(),
                   QImage::Format_ARGB32_Premultiplied);
    {
      QPainter p(&dbg_bmp);
      p.drawPixmap (exposed_rect, *pinch_image_, source);
    }
    dbg_bmp.save ("/tmp/pinchstep.png");
#endif
    return;
  }

  host = hostView()->host_;
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

  QRectF paint_rect(0.0, 0.0, kMaxWindowWidth, kMaxWindowHeight);
  paint_rect &= exposed_rect;
  paint_rect |= invalid_rect;

  // Calling GetBackingStore maybe have changed |invalid_rect_|...
  hostView()->about_to_validate_and_paint_ = false;

  if (backing_store) {
    // Only render the widget if it is attached to a window; there's a short
    // period where this object isn't attached to a window but hasn't been
    // Destroy()ed yet and it receives paint messages...
#ifdef PINCH_FINI_DEBUG
    if (counter - non_pinch_cnt > 20) {
      if (--rec_cnt == 0) {
        non_pinch_cnt = counter;
        rec_cnt = 3;
      }
      QImage dbg_bmp(900, 500,
                     QImage::Format_ARGB32_Premultiplied);
      {
        QPainter p(&dbg_bmp);
        backing_store->QPainterShowRect(&p, paint_rect);
      }
      dbg_bmp.save (QString("/tmp/paint%1.bmp").arg(counter));
    }
    non_pinch_cnt++;
#endif

    if (hold_paint_) {
      hold_paint_ = false;
    }else if (painter) {
      backing_store->QPainterShowRect(painter, paint_rect);

      // Paint the video layer
      // TODO no video_play now
/*
      VideoLayerX* video_layer = static_cast<VideoLayerX*>(
        host->video_layer());
      if (video_layer)
        video_layer->QPainterShow(painter);
*/
    }
  } else {
    DNOTIMPLEMENTED();
  }
}

bool RWHVQtWidget::shouldDeliverMouseMove()
{
  int node_info = hostView()->webkit_node_info_;
  
  if (hostView()->IsPopup())
    return false;

  return (node_info & (RenderWidgetHostViewQt::NODE_INFO_IS_EMBEDDED_OBJECT
                        | RenderWidgetHostViewQt::NODE_INFO_IS_EDITABLE));
}

void RWHVQtWidget::deliverMousePressEvent()
{
  if (mouse_press_event_delivered_)
    return;

  mouse_press_event_delivered_ = true;
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
  switch (event->type()) {
  // case QEvent::GraphicsSceneMouseDoubleClick:
  //   // DLOG(INFO) << "==>" << __PRETTY_FUNCTION__ <<
  //   //   " GraphicsSceneMouseDoubleClick" << std::endl;
  //   {
  //     QGraphicsSceneMouseEvent* e = (QGraphicsSceneMouseEvent*)event;
  //     zoom2TextAction(e->pos());
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
  ic->reset();
  // FIXME: if we got unconfirmed composition text, and we try to move cursor
  // from one text entry to another, the unconfirmed composition text will be cancelled
  // but the focus will not move, unless you click another entry again.
  // This bug also exist in GTK code.

  bool is_enabled = (type != WebKit::WebTextInputTypeNone);
  
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

  if (!is_enabled) {
  //if (im_enabled_) {
      setFlag(QGraphicsItem::ItemAcceptsInputMethod, false);
      QEvent sip_request(QEvent::CloseSoftwareInputPanel);
      ic->filterEvent(&sip_request);
      im_enabled_ = false;
  //}
  } else {
    // Enable the InputMethod if it's not enabled yet.
  //  if (!im_enabled_) {
      setFlag(QGraphicsItem::ItemAcceptsInputMethod, true);
      QEvent sip_request(QEvent::RequestSoftwareInputPanel);
      ic->setFocusWidget(qApp->focusWidget());
      ic->filterEvent(&sip_request);
      im_enabled_ = true;
  //  }
  }
  
  if (type == WebKit::WebTextInputTypePassword) {
    setInputMethodHints(inputMethodHints() | Qt::ImhHiddenText | Qt::ImhNoPredictiveText );
  } else {
    setInputMethodHints(inputMethodHints() & ~(Qt::ImhHiddenText | Qt::ImhNoPredictiveText) );
  }
  ic->update();
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
  hostView()->SetSize(gfx::Size(event->newSize().width(), event->newSize().height()));
}

void RWHVQtWidget::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
  DLOG(INFO) << "--" << __PRETTY_FUNCTION__ << ": " <<
    "shouldDeliverMouseMove = " << shouldDeliverMouseMove() <<
    std::endl;

#if 0
  // Anyone need the touch move event?
  WebKit::WebTouchEvent touchEvent = EventUtilQt::ToWebTouchEvent(event);
  hostView()->host_->ForwardTouchEvent(touchEvent);
#endif

  if (is_modifing_selection_) {
    ModifySelection(current_selection_handler_,
        gfx::Point(static_cast<int>(event->pos().x()), static_cast<int>(event->pos().y())));
    goto done;
  }

  if (shouldDeliverMouseMove()) {
    // send out mouse press event, if it hadn't been sent out.
    deliverMousePressEvent();

    WebKit::WebMouseEvent mouseEvent = EventUtilQt::ToWebMouseEvent(event);
    hostView()->host_->ForwardMouseEvent(mouseEvent);
  }

done:
  event->accept();
}

void RWHVQtWidget::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
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

  if (!hostView()->IsPopup()) {
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    if (timestamp - m_dbclkHackTimeStamp < 350) {
      // we may hit a double tap
      qreal length = QLineF(event->pos(), m_dbclkHackPos).length();
      if (length < 40) {
        DLOG(INFO) << "WE HIT A DOUBLE CLICK " << length << std::endl;
        zoom2TextAction(event->pos());

        return;
      }
    }

    m_dbclkHackTimeStamp = timestamp;
    m_dbclkHackPos       = event->pos();
  }

  auto_pan_->stop();
  WebKit::WebTouchEvent touchEvent = EventUtilQt::ToWebTouchEvent(event);

  if (in_selection_mode_) {
    current_selection_handler_ = findSelectionHandler(static_cast<int>(event->pos().x()), static_cast<int>(event->pos().y()));
    if (current_selection_handler_ != SELECTION_HANDLER_NONE) {
      is_modifing_selection_ = true;
      goto done;
    }
  }

// we send a touch event first to give user a visual feedback on mouse down,
// but do not do actual mouse down works
  hostView()->host_->ForwardTouchEvent(touchEvent);

// Then query the node under current pos
  hostView()->host_->QueryNodeAtPosition(static_cast<int>(event->pos().x()),
      static_cast<int>(event->pos().y()));

// Finally, save the mouse press event for later usage
  mouse_press_event_ = EventUtilQt::ToWebMouseEvent(event);
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

done:
  event->accept();
}

void RWHVQtWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
  WebKit::WebTouchEvent touchEvent = EventUtilQt::ToWebTouchEvent(event);
// we don't do normal mouse release event when modifing selection
  if (is_modifing_selection_) {
    is_modifing_selection_ = false;
    current_selection_handler_ = SELECTION_HANDLER_NONE;
    goto done;
  }

// we send a touch event first to give user a visual feedback on mouse up
  hostView()->host_->ForwardTouchEvent(touchEvent);

  // we clear the TapAndHoldGesture here to prevent a PanGesture been invoked upon the same 
  // touch event of tapAndHoldGesture
  clearDoingGesture(Qt::TapAndHoldGesture);

  if (isDoingGesture())
    return;

// If no gesture is going on, it means that we are doing a short click
  if (!cancel_next_mouse_release_event_) {
    // send out mouse press event, if it hadn't been sent out.
    deliverMousePressEvent();
    // send out mouse release event
    WebKit::WebMouseEvent mouseEvent = EventUtilQt::ToWebMouseEvent(event);
    hostView()->host_->ForwardMouseEvent(mouseEvent);
  } else {
    ///\bug If we are doing gesture on a button in the page
    ///the bug will keep press down status since we cancel the mouse release event.
    cancel_next_mouse_release_event_= false;
  }

done:
  event->accept();
}

void RWHVQtWidget::autoPanCallback(int dx, int dy)
{
  WebKit::WebMouseWheelEvent wheelEvent = EventUtilQt::ToMouseWheelEvent(
      last_pan_wheel_event_.x, last_pan_wheel_event_.y,
      last_pan_wheel_event_.globalX, last_pan_wheel_event_.globalY,
      dx, dy);

  hostView()->host_->ForwardWheelEvent(wheelEvent);
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
            (hostView()->webkit_node_info_ & RenderWidgetHostViewQt::NODE_INFO_IS_EDITABLE)))
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
  if (is_modifing_selection_)
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

  if (shouldDeliverMouseMove()) {
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
      auto_pan_->feedMotion(wheelEvent.deltaX, wheelEvent.deltaY);
      break;
    case Qt::GestureFinished:
      if (isDoingGesture(Qt::PanGesture)) {
        cancel_next_mouse_release_event_ = true;
        auto_pan_->start();
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

  hostView()->host_->ForwardWheelEvent(wheelEvent);
  event->accept();
}

void RWHVQtWidget::finishPinch()
{
  DLOG(INFO) << __PRETTY_FUNCTION__ << "\n";

  clearDoingGesture(Qt::PinchGesture);
  //TODO: no api set_painting_observer
  //hostView()->host_->set_painting_observer (NULL);
  if (pinch_image_) {

    pinch_image_ = false;
    double current_factor;
    gfx::Point offset;
    hostView()->host_->QueryScrollOffset (offset);
    DLOG(INFO) << " current offset: " << offset.x() << " " << offset.y();
    hostView()->host_->QueryZoomFactor (current_factor);
    DLOG(INFO) << " current zoom factor: " << current_factor;

    hold_paint_ = true;
    offset = gfx::Point(pinch_src_rect_.x(), pinch_src_rect_.y());
    DLOG(INFO) << "new offset: " << offset.x() << " " << offset.y();
    hostView()->host_->SetScrollPosition (offset.x(), offset.y());

    current_factor = current_factor * scale_factor_;
    hostView()->host_->SetZoomFactor (current_factor);
    DLOG(INFO) << " new factor: " << current_factor;
    update (0.0, 0.0, 1000.0, 1000.0);

  }

  if (pinch_release_timer_)
    pinch_release_timer_->stop ();

  cancel_next_mouse_release_event_ = true;
}

void RWHVQtWidget::pinchFinishTimeout()
{
  finishPinch();
}

void RWHVQtWidget::pinchGestureEvent(QGestureEvent* event, QPinchGesture* gesture)
{
  if (is_modifing_selection_)
    return;

  if (shouldDeliverMouseMove()) {
      cancel_next_mouse_release_event_ = false;
      return;
  }

  switch (gesture->state())
  {
    case Qt::GestureStarted:
      gesture->setGestureCancelPolicy(QGesture::CancelAllInContext);
      setDoingGesture(Qt::PinchGesture);
      {
        static int sequence_num = 0;
        sequence_num++;
        gfx::Size size = gfx::Size(kSnapshotWebPageWidth,
                                   kSnapshotWebPageHeight);
        TransportDIB* dib =
          TransportDIB::Create(size.width() * 4 * size.height(),
                               sequence_num);

        int ret = hostView()->host_->PaintContents(dib->handle(),
                                                   gfx::Rect(0, 0, kSnapshotWebPageWidth,
                                                             kSnapshotWebPageHeight));
        if (!ret) {
          std::vector<gfx::Rect> copy_rects;
          copy_rects.push_back(gfx::Rect(0, 0, kSnapshotWebPageWidth,
                                         kSnapshotWebPageHeight));

          pinch_backing_store_->PaintToBackingStore(hostView()->host_->process(),
                                                    dib->handle(),
                                                    gfx::Rect(0, 0, kSnapshotWebPageWidth,
                                                              kSnapshotWebPageHeight),
                                                    copy_rects);
          setPinchImage ();
        }
        delete dib;
      }

      scale_factor_ = 1.0;
      pinch_center_ = gesture->centerPoint();
      hostView()->host_->QueryScrollOffset (pinch_view_pos_);

      if (pinch_release_timer_) {
        pinch_release_timer_->start (1000);
      }

      cancel_next_mouse_release_event_ = true;
      break;
    case Qt::GestureUpdated:
      setDoingGesture(Qt::PinchGesture);

      if (pinch_release_timer_)
        pinch_release_timer_->start (1000);

#if QT_VERSION >= 0x040701
      scale_factor_ = gesture->totalScaleFactor();
#else
      scale_factor_ = gesture->scaleFactor();
#endif
      update (0.0, 0.0, 1000.0, 1000.0);
      LOG(INFO) << __PRETTY_FUNCTION__ << " Updated: " << scale_factor_;
      cancel_next_mouse_release_event_ = true;
      break;
    case Qt::GestureFinished:
      finishPinch();
      break;
    case Qt::GestureCanceled:
      clearDoingGesture(Qt::PinchGesture);
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

QVariant RWHVQtWidget::inputMethodQuery(Qt::InputMethodQuery query) const
{
  ///\todo wait for implement, need to report correct MicroFocus
  if (query == Qt::ImMicroFocus) {
    return QVariant(cursor_rect_);
  } else {
    return QVariant();
  }

}

void RWHVQtWidget::zoom2TextAction(const QPointF& pos)
{
  RenderWidgetHost* host = hostView()->host_;
  WebKit::WebSettings::LayoutAlgorithm algo =
    host->GetLayoutAlgorithm();

  double factor;
  host->QueryZoomFactor (factor);

  if (algo == WebKit::WebSettings::kLayoutNormal) {
    host->Zoom2TextPre(pos.x(), pos.y());
    factor = 2;
    host->SetLayoutAlgorithm(WebKit::WebSettings::kLayoutFitColumnToScreen);
    host->SetZoomFactor(factor);
    host->Zoom2TextPost();
  } else {
    factor = 1;
    host->SetLayoutAlgorithm(WebKit::WebSettings::kLayoutNormal);
    host->SetZoomFactor(factor);
  }
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
      x, y, globalX, globalY);
  hostView()->host_->ForwardMouseEvent(rightButtonPressEvent);

  WebKit::WebMouseEvent rightButtonReleaseEvent = EventUtilQt::ToWebMouseEvent(
      QEvent::GraphicsSceneMouseRelease,
      Qt::RightButton, Qt::NoModifier,
      x, y, globalX, globalY);
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
  rvh->SelectItem(gfx::Point(x, y));
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

#include "moc_rwhv_qt_widget.cc"
