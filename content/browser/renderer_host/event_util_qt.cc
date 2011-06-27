// Copyright (c) 2010 The Chromium Authors.
// Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <QEvent>
#include <QKeyEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGestureEvent>
#include <QPanGesture>
#include <QGraphicsWidget>

#include "base/time.h"
#include "base/logging.h"
#include "content/browser/renderer_host/event_util_qt.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebInputEvent.h"
#include "event_util_qt_keyboard_event.h"

static int qtModifiersToWebEventModifiers(Qt::KeyboardModifiers qt_modifiers)
{
  int modifiers = 0;
  if (qt_modifiers &Qt::ShiftModifier)
    modifiers |= WebKit::WebInputEvent::ShiftKey;
  if (qt_modifiers &Qt::ControlModifier)
    modifiers |= WebKit::WebInputEvent::ControlKey;
  if (qt_modifiers &Qt::AltModifier)
    modifiers |= WebKit::WebInputEvent::AltKey;
  if (qt_modifiers &Qt::MetaModifier)
    modifiers |= WebKit::WebInputEvent::MetaKey;
  if (qt_modifiers &Qt::KeypadModifier)
    modifiers |= WebKit::WebInputEvent::IsKeyPad;

  return modifiers;
}

// Switch general mouse event to special mouse event for double click
WebKit::WebMouseEvent EventUtilQt::ToWebMouseDoubleClickEvent(const WebKit::WebMouseEvent wevent)
{
  WebKit::WebMouseEvent result;
  result.timeStampSeconds = wevent.timeStampSeconds;
  result.modifiers = wevent.modifiers;
  result.x = wevent.x;
  result.y = wevent.y;
  result.windowX = wevent.windowX;
  result.windowY = wevent.windowY;
  result.globalX = wevent.globalX;
  result.globalY = wevent.globalY;
  result.type = wevent.type;
  result.button = wevent.button;
  //clickCount = 2 means double click   
  result.clickCount = 2;
  return result;
}

WebKit::WebMouseEvent EventUtilQt::ToWebMouseEvent(const QGraphicsSceneMouseEvent *qevent, double scale)
{
  WebKit::WebMouseEvent result;

  result.timeStampSeconds = base::Time::Now().ToInternalValue() / 1000;
  result.modifiers = qtModifiersToWebEventModifiers(qevent->modifiers());

  result.x = static_cast<int>(qevent->pos().x() / scale);
  result.y = static_cast<int>(qevent->pos().y() / scale);

  result.windowX = result.x;
  result.windowY = result.y;
  result.globalX = static_cast<int>(qevent->screenPos().x());
  result.globalY = static_cast<int>(qevent->screenPos().y());

  switch (qevent->type()) {
  case QEvent::GraphicsSceneMousePress:
  case QEvent::GraphicsSceneMouseDoubleClick:
    result.type = WebKit::WebInputEvent::MouseDown;
    break;
  case QEvent::GraphicsSceneMouseRelease:
    result.type = WebKit::WebInputEvent::MouseUp;
    break;
  case QEvent::GraphicsSceneMouseMove:
    result.type = WebKit::WebInputEvent::MouseMove;
    break;
  case QEvent::GraphicsSceneHoverEnter:
    result.type = WebKit::WebInputEvent::MouseEnter;
    break;
  case QEvent::GraphicsSceneHoverLeave:
    result.type = WebKit::WebInputEvent::MouseLeave;
    break;
  case QEvent::GraphicsSceneHoverMove:
    result.type = WebKit::WebInputEvent::MouseMove;
    break;
  default:
    NOTREACHED();
  }

  result.button = WebKit::WebMouseEvent::ButtonLeft;
  if ((qevent->button() == Qt::LeftButton) || (qevent->buttons() & Qt::LeftButton))
    result.button = WebKit::WebMouseEvent::ButtonLeft;
  else if ((qevent->button() == Qt::MidButton) || (qevent->buttons() & Qt::MidButton))
    result.button = WebKit::WebMouseEvent::ButtonMiddle;
  else if ((qevent->button() == Qt::RightButton) || (qevent->buttons() & Qt::RightButton))
    result.button = WebKit::WebMouseEvent::ButtonRight;

  result.clickCount = 1;

  return result;
}

WebKit::WebMouseEvent EventUtilQt::ToWebMouseEvent(QEvent::Type type,
    Qt::MouseButton button, Qt::KeyboardModifiers modifiers,
                                                    int x, int y, int globalX, int globalY, double scale)
{
  WebKit::WebMouseEvent result;

  result.timeStampSeconds = base::Time::Now().ToInternalValue() / 1000;
  result.modifiers = modifiers;

  result.x = static_cast<int>(x / scale);
  result.y = static_cast<int>(y / scale);
  result.windowX = result.x;
  result.windowY = result.y;
  result.globalX = globalX;
  result.globalY = globalY;

  switch (type) {
  case QEvent::GraphicsSceneMousePress:
  case QEvent::GraphicsSceneMouseDoubleClick:
    result.type = WebKit::WebInputEvent::MouseDown;
    break;
  case QEvent::GraphicsSceneMouseRelease:
    result.type = WebKit::WebInputEvent::MouseUp;
    break;
  case QEvent::GraphicsSceneMouseMove:
    result.type = WebKit::WebInputEvent::MouseMove;
    break;
  case QEvent::GraphicsSceneHoverEnter:
    result.type = WebKit::WebInputEvent::MouseEnter;
    break;
  case QEvent::GraphicsSceneHoverLeave:
    result.type = WebKit::WebInputEvent::MouseLeave;
    break;
  case QEvent::GraphicsSceneHoverMove:
    result.type = WebKit::WebInputEvent::MouseMove;
    break;
  default:
    NOTREACHED();
  }

  result.button = WebKit::WebMouseEvent::ButtonLeft;
  if (button == Qt::LeftButton)
    result.button = WebKit::WebMouseEvent::ButtonLeft;
  else if (button == Qt::MidButton)
    result.button = WebKit::WebMouseEvent::ButtonMiddle;
  else if (button == Qt::RightButton)
    result.button = WebKit::WebMouseEvent::ButtonRight;

  result.clickCount = 1;

  return result;
}

WebKit::WebKeyboardEvent EventUtilQt::ToWebKeyboardEvent(const QKeyEvent *qevent)
{
  WebKit::WebKeyboardEvent result;
  
  result.timeStampSeconds = base::Time::Now().ToInternalValue() / 1000;
  result.modifiers = qtModifiersToWebEventModifiers(qevent->modifiers());
  if (qevent->isAutoRepeat())
    result.modifiers |= WebKit::WebInputEvent::IsAutoRepeat;
  
  switch (qevent->type()) {
  case QEvent::KeyPress:
    ///\todo Hmm, not sure why use RawKeyDown instead of KeyDown
    result.type = WebKit::WebInputEvent::RawKeyDown;
    break;
  case QEvent::KeyRelease:
    result.type = WebKit::WebInputEvent::KeyUp;
    break;
  case QEvent::ShortcutOverride:
    ///\todo Need to do some thing for QEvent::ShortcutOverride?
    return result;
  default:
    NOTREACHED();
  }

  // According to MSDN:
  // http://msdn.microsoft.com/en-us/library/ms646286(VS.85).aspx
  // Key events with Alt modifier and F10 are system key events.
  // We just emulate this behavior. It's necessary to prevent webkit from
  // processing keypress event generated by alt-d, etc.
  // F10 is not special on Linux, so don't treat it as system key.
  if (result.modifiers & WebKit::WebInputEvent::AltKey)
    result.isSystemKey = true;
  
  result.windowsKeyCode = windowsKeyCodeForQKeyEvent(qevent->key(), qevent->modifiers() & Qt::KeypadModifier);
  result.nativeKeyCode = qevent->nativeScanCode();

  const char* event_text = qevent->text().toAscii().data();
  strncpy((char *)result.unmodifiedText, event_text, sizeof(result.unmodifiedText) - 1); //Fixme: not correct
  strncpy((char *)result.text, event_text, sizeof(result.unmodifiedText) - 1);
  
  strncpy(result.keyIdentifier,
    keyIdentifierForQtKeyCode(qevent->key()).toAscii().data(),
    sizeof(result.keyIdentifier) - 1);
  
  return result;
}

WebKit::WebKeyboardEvent EventUtilQt::KeyboardEvent(wchar_t character, Qt::KeyboardModifier modifiers, double timeStampSeconds)
{
  WebKit::WebKeyboardEvent result;
  result.type = WebKit::WebInputEvent::Char;
  result.timeStampSeconds = timeStampSeconds;
  result.modifiers = qtModifiersToWebEventModifiers(modifiers);
  result.windowsKeyCode = character;
  result.nativeKeyCode = character;
  result.text[0] = character;
  result.unmodifiedText[0] = character;

  if (result.modifiers & WebKit::WebInputEvent::AltKey)
    result.isSystemKey = true;

  return result;
}

WebKit::WebMouseWheelEvent EventUtilQt::ToMouseWheelEvent(const QGraphicsSceneMouseEvent* qevent,
                                                          QtMobility::QOrientationReading::Orientation angle,
                                                          double scale)
{
  WebKit::WebMouseWheelEvent result;

  result.type = WebKit::WebInputEvent::MouseWheel;
  result.button = WebKit::WebMouseEvent::ButtonNone;

  result.timeStampSeconds = base::Time::Now().ToInternalValue() / 1000;

  result.x = static_cast<int>(qevent->pos().x() / scale);
  result.y = static_cast<int>(qevent->pos().y() / scale);

  result.windowX = result.x;
  result.windowY = result.y;
  result.globalX = static_cast<int>(qevent->screenPos().x());
  result.globalY = static_cast<int>(qevent->screenPos().y());

  int dx = static_cast<int>(qevent->pos().x() - qevent->lastPos().x());
  int dy = static_cast<int>(qevent->pos().y() - qevent->lastPos().y());
  
  switch (angle) {
    case QtMobility::QOrientationReading::TopUp:
      result.deltaX = dx;
      result.deltaY = dy;
      break;
    case QtMobility::QOrientationReading::RightUp:
      result.deltaX = dy;
      result.deltaY = 0 - dx;
      break;
    case QtMobility::QOrientationReading::TopDown:
      result.deltaX = 0 - dx;
      result.deltaY = 0 - dy;
      break;
    case QtMobility::QOrientationReading::LeftUp:
      result.deltaX = 0 - dy;
      result.deltaY = dx;
      break;
  }

  return result;
}

WebKit::WebMouseWheelEvent EventUtilQt::ToMouseWheelEvent(const QGestureEvent *qevent,
                                                           const QPanGesture *gesture, QGraphicsWidget *item,
                                                           QtMobility::QOrientationReading::Orientation angle)
{
  WebKit::WebMouseWheelEvent result;

  result.type = WebKit::WebInputEvent::MouseWheel;
  result.button = WebKit::WebMouseEvent::ButtonNone;

  result.timeStampSeconds = base::Time::Now().ToInternalValue() / 1000;

  result.globalX = static_cast<int>(gesture->hotSpot().x());
  result.globalY = static_cast<int>(gesture->hotSpot().y());
  result.x = static_cast<int>(item->mapFromScene(qevent->mapToGraphicsScene(gesture->hotSpot())).x());
  result.y = static_cast<int>(item->mapFromScene(qevent->mapToGraphicsScene(gesture->hotSpot())).y());
  result.windowX = result.x;
  result.windowY = result.y;

  switch (angle) {
    case QtMobility::QOrientationReading::TopUp:
      result.deltaX = gesture->delta().x();
      result.deltaY = gesture->delta().y();
      break;
    case QtMobility::QOrientationReading::RightUp:
      result.deltaX = gesture->delta().y();
      result.deltaY = 0 - gesture->delta().x();
      break;
    case QtMobility::QOrientationReading::TopDown:
      result.deltaX = 0 - gesture->delta().x();
      result.deltaY = 0 - gesture->delta().y();
      break;
    case QtMobility::QOrientationReading::LeftUp:
      result.deltaX = 0 - gesture->delta().y();
      result.deltaY = gesture->delta().x();
      break;
  }

  return result;
}

WebKit::WebMouseWheelEvent EventUtilQt::ToMouseWheelEvent(int x, int y,
                                                           int gx, int gy, int dx, int dy,
                                                           QtMobility::QOrientationReading::Orientation angle)
{
  WebKit::WebMouseWheelEvent result;

  result.type = WebKit::WebInputEvent::MouseWheel;
  result.button = WebKit::WebMouseEvent::ButtonNone;

  result.timeStampSeconds = base::Time::Now().ToInternalValue() / 1000;

  result.globalX = gx;
  result.globalY = gy;
  result.x = x;
  result.y = y;
  result.windowX = result.x;
  result.windowY = result.y;

  switch (angle) {
    case QtMobility::QOrientationReading::TopUp:
      result.deltaX = dx;
      result.deltaY = dy;
      break;
    case QtMobility::QOrientationReading::RightUp:
      result.deltaX = dy;
      result.deltaY = 0 - dx;
      break;
    case QtMobility::QOrientationReading::TopDown:
      result.deltaX = 0 - dx;
      result.deltaY = 0 - dy;
      break;
    case QtMobility::QOrientationReading::LeftUp:
      result.deltaX = 0 - dy;
      result.deltaY = dx;
      break;
  }

  return result;
}

WebKit::WebTouchEvent EventUtilQt::ToWebTouchEvent(const QGraphicsSceneMouseEvent *qevent, double scale)
{
  WebKit::WebTouchEvent result;

  result.timeStampSeconds = base::Time::Now().ToInternalValue() / 1000;
  result.modifiers = qtModifiersToWebEventModifiers(qevent->modifiers());

  result.touchPointsLength = 1;

  result.touchPoints[0].id = 1;
  result.touchPoints[0].position.x = static_cast<int>(qevent->pos().x() / scale);
  result.touchPoints[0].position.y = static_cast<int>(qevent->pos().y() / scale);
  result.touchPoints[0].screenPosition.x = static_cast<int>(qevent->screenPos().x());
  result.touchPoints[0].screenPosition.y = static_cast<int>(qevent->screenPos().y());

  switch (qevent->type()) {
  case QEvent::GraphicsSceneMousePress:
  case QEvent::GraphicsSceneMouseDoubleClick:
    result.type = WebKit::WebInputEvent::TouchStart;
    result.touchPoints[0].state = WebKit::WebTouchPoint::StatePressed;
    break;
  case QEvent::GraphicsSceneMouseRelease:
    result.type = WebKit::WebInputEvent::TouchEnd;
    result.touchPoints[0].state = WebKit::WebTouchPoint::StateReleased;
    break;
  case QEvent::GraphicsSceneMouseMove:
    result.type = WebKit::WebInputEvent::TouchMove;
    result.touchPoints[0].state = WebKit::WebTouchPoint::StateMoved;
    break;
  default:
    NOTREACHED();
  }

  return result;
}
