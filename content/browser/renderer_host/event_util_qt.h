// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RENDERER_HOST_EVENT_UTIL_QT_H_
#define CHROME_BROWSER_RENDERER_HOST_EVENT_UTIL_QT_H_

namespace WebKit {
class WebMouseEvent;
class WebKeyboardEvent;
class WebMouseWheelEvent;
class WebTouchEvent;
}

#include <QKeyEvent>
#include <QOrientationReading>

class QGraphicsSceneMouseEvent;
class QGestureEvent;
class QPanGesture;
class QGraphicsWidget;

class EventUtilQt {
 public:

  /*!
   * \brief Convert QGraphicsSceneMouseEvent to WebKit::WebMouseEvent
   */
  static WebKit::WebMouseEvent ToWebMouseEvent(const QGraphicsSceneMouseEvent*, double scale = 1.0);

  /*!
   * \brief Convert input values into a WebKit::WebMouseEvent
   */
  static WebKit::WebMouseEvent ToWebMouseEvent(QEvent::Type type,
    Qt::MouseButton button, Qt::KeyboardModifiers modifiers,
    int x, int y, int globalX, int globalY, double scale = 1.0);

  /*!
   * \brief Convert QKeyEvent to WebKit::WebKeyboardEvent
   */
  static WebKit::WebKeyboardEvent ToWebKeyboardEvent(const QKeyEvent *qevent);

  /*!
   * \brief construct a WebInputEvent::Char event without using a QKeyEvent
   * \param character char to sent
   * \param modifiers keyboard modifiers
   * \param timeStampSeconds event time stamp
   */
  static WebKit::WebKeyboardEvent KeyboardEvent(wchar_t character, Qt::KeyboardModifier modifiers, double timeStampSeconds);

  /*!
   * \brief Convert QPanGesture to WebKit::WebMouseWheelEvent to simulate a pan event
   */
  // deprecated
  static WebKit::WebMouseWheelEvent ToMouseWheelEvent(const QGestureEvent *qevent, const QPanGesture *gesture,
      QGraphicsWidget *item, QtMobility::QOrientationReading::Orientation angle = QtMobility::QOrientationReading::TopUp);

  static WebKit::WebMouseWheelEvent ToMouseWheelEvent(int x, int y, int gx, int gy, int dx, int dy,
      QtMobility::QOrientationReading::Orientation angle = QtMobility::QOrientationReading::TopUp);

  static WebKit::WebMouseWheelEvent ToMouseWheelEvent(const QGraphicsSceneMouseEvent* event,
      QtMobility::QOrientationReading::Orientation angle = QtMobility::QOrientationReading::TopUp,
      double scale = 1.0);

  static WebKit::WebTouchEvent ToWebTouchEvent(const QGraphicsSceneMouseEvent *qevent, double scale = 1.0);
};

#endif  // CHROME_BROWSER_RENDERER_HOST_EVENT_UTIL_QT_H_
