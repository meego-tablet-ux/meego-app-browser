/*
 * Copyright 2010, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "fullscreen_window_mac.h"

#include <QuickTime/QuickTime.h>

#import "graphics_utils_mac.h"
#import "overlay_window_mac.h"
#import "plugin_mac.h"
#import "core/cross/display_mode.h"
#import "plugin/cross/o3d_glue.h"
#import "plugin/mac/graphics_utils_mac.h"

using glue::_o3d::PluginObject;

namespace o3d {

//----------------------------------------------------------------------
// Carbon full-screen implementation.
//

// Convenience function for fetching SInt32 parameters from Carbon EventRefs.
static SInt32 GetIntEventParam(EventRef inEvent, EventParamName    inName) {
  SInt32    value = 0;
  return (GetEventParameter(inEvent, inName, typeSInt32, NULL, sizeof(value),
                            NULL, &value) == noErr) ? value : 0;
}


// Maps the MacOS button numbers to the constants used by our
// event mechanism.  Not quite as obvious as you might think, as the Mac
// thinks the numbering should go left, right, middle and our W3C-influenced
// system goes left, middle, right.
// Defaults to left-button if passed a strange value.  Pass Cocoa mouse button
// codes as-is (they start at 0), pass Carbon button codes - 1.
o3d::Event::Button MacOSMouseButtonNumberToO3DButton(int inButton) {

  switch(inButton) {
    case 0:
      return o3d::Event::BUTTON_LEFT;
    case 1:
      return o3d::Event::BUTTON_RIGHT;
    case 2:
      return o3d::Event::BUTTON_MIDDLE;
    case 3:
      return o3d::Event::BUTTON_4;
    case 4:
      return o3d::Event::BUTTON_5;
  }

  return o3d::Event::BUTTON_LEFT;
}


// Handles the CarbonEvents that we get sent for the fullscreen mode window.
// Most of these can be converted to EventRecord events and handled by the
// HandleMacEvent() function in main_mac.mm, but some have no equivalent in
// that space, scroll-wheel events for example, and so must be handled here.
static OSStatus HandleFullscreenWindow(EventHandlerCallRef inHandlerCallRef,
                                       EventRef inEvent,
                                       void *inUserData) {
  OSType event_class = GetEventClass(inEvent);
  OSType event_kind = GetEventKind(inEvent);
  NPP instance = (NPP)inUserData;
  PluginObject* obj = (PluginObject*)(instance->pdata);
  HIPoint mouse_loc = { 0.0, 0.0 };
  bool is_scroll_event = event_class == kEventClassMouse &&
                         (event_kind == kEventMouseScroll ||
                          event_kind == kEventMouseWheelMoved);

  // If it's any kind of mouse event, get the global mouse loc.
  if (event_class == kEventClassMouse) {
    GetEventParameter(inEvent, kEventParamMouseLocation,
                      typeHIPoint, NULL,
                      sizeof(mouse_loc), NULL,
                      &mouse_loc);
  }

  // Handle the two kinds of scroll message we understand.
  if (is_scroll_event) {
    SInt32 x_scroll = 0;
    SInt32 y_scroll = 0;
    EventMouseWheelAxis axis = kEventMouseWheelAxisY;

    switch (event_kind) {
      // The newer kind of scroll event, as sent when two-finger
      // dragging on a touchpad.
      case kEventMouseScroll:
        x_scroll = GetIntEventParam(inEvent,
                                    kEventParamMouseWheelSmoothHorizontalDelta);
        y_scroll = GetIntEventParam(inEvent,
                                    kEventParamMouseWheelSmoothVerticalDelta);

        // only pass x or y value - pass whichever is larger
        if (x_scroll && y_scroll) {
          if (abs(x_scroll) > abs(y_scroll))
            y_scroll = 0;
          else
            x_scroll = 0;
        }
        break;
        // The older kind of scroll event, as sent when using the wheel on
        // a third-party mouse.
      case kEventMouseWheelMoved:
        GetEventParameter(inEvent,  kEventParamMouseWheelAxis,
                          typeMouseWheelAxis, NULL,
                          sizeof(axis), NULL,
                          &axis);

        if (axis == kEventMouseWheelAxisY) {
          y_scroll = GetIntEventParam(inEvent,
                                      kEventParamMouseWheelDelta);
        } else {
          x_scroll = GetIntEventParam(inEvent,
                                      kEventParamMouseWheelDelta);
        }
        break;
    }

    // Dispatch the event now that we have all the data.
    if (x_scroll || y_scroll) {
      o3d::Event event(o3d::Event::TYPE_WHEEL);
      event.set_delta(x_scroll, y_scroll);
      // Global and local locs are the same, in this case,
      // as we have a fullscreen window at 0,0.
      event.set_position(mouse_loc.x, mouse_loc.y,
                         mouse_loc.x, mouse_loc.y, true);
      obj->client()->AddEventToQueue(event);
    }
    return noErr;
  } else if (event_class == kEventClassMouse &&
             (event_kind == kEventMouseDown || event_kind == kEventMouseUp)) {

    o3d::Event::Type type = (event_kind == kEventMouseDown) ?
                            o3d::Event::TYPE_MOUSEDOWN :
                            o3d::Event::TYPE_MOUSEUP;
    o3d::Event event(type);
    event.set_position(mouse_loc.x, mouse_loc.y,
                       mouse_loc.x, mouse_loc.y, true);

    EventMouseButton button = 0;
    GetEventParameter(inEvent,  kEventParamMouseButton,
                      typeMouseButton, NULL,
                      sizeof(button), NULL,
                      &button);
    // Carbon mouse button numbers start at 1, so subtract 1 here -
    // Cocoa mouse buttons, by contrast,  start at 0).
    event.set_button(MacOSMouseButtonNumberToO3DButton(button - 1));

    // add the modifiers to the event, if any
    UInt32 carbonMods = GetIntEventParam(inEvent,
                                         kEventParamKeyModifiers);
    if (carbonMods) {
      int modifier_state = 0;
      if (carbonMods & controlKey) {
        modifier_state |= o3d::Event::MODIFIER_CTRL;
      }
      if (carbonMods & shiftKey) {
        modifier_state |= o3d::Event::MODIFIER_SHIFT;
      }
      if (carbonMods & optionKey) {
        modifier_state |= o3d::Event::MODIFIER_ALT;
      }
      if (carbonMods & cmdKey) {
        modifier_state |= o3d::Event::MODIFIER_META;
      }

      event.set_modifier_state(modifier_state);
    }

    obj->client()->AddEventToQueue(event);
  } else {  // not a scroll event or a click

    // All other events are currently handled by being converted to an
    // old-style EventRecord as passed by the classic NPAPI interface
    // and dispatched through our common routine.
    EventRecord eventRecord;

    if (ConvertEventRefToEventRecord(inEvent, &eventRecord)) {
      HandleMacEvent(&eventRecord, (NPP)inUserData);
      return noErr;
    } else {
      return eventNotHandledErr;
    }
  }
  return noErr;
}


static WindowRef CreateFullscreenWindow(WindowRef window,
                                        PluginObject *obj) {
  Rect        bounds = CGRect2Rect(CGDisplayBounds(CGMainDisplayID()));
  OSStatus    err = noErr;
  EventTypeSpec  eventTypes[] = {
    {kEventClassKeyboard, kEventRawKeyDown},
    {kEventClassKeyboard, kEventRawKeyRepeat},
    {kEventClassKeyboard, kEventRawKeyUp},
    {kEventClassMouse,    kEventMouseDown},
    {kEventClassMouse,    kEventMouseUp},
    {kEventClassMouse,    kEventMouseMoved},
    {kEventClassMouse,    kEventMouseDragged},
    {kEventClassMouse,    kEventMouseScroll},
    {kEventClassMouse,    kEventMouseWheelMoved}
  };

  if (window == NULL)
    err = CreateNewWindow(kSimpleWindowClass,
                          kWindowStandardHandlerAttribute,
                          &bounds,
                          &window);
  if (err)
    return NULL;

  SetWindowLevel(window, CGShieldingWindowLevel() + 1);

  InstallEventHandler(GetWindowEventTarget(window), HandleFullscreenWindow,
                      sizeof(eventTypes)/sizeof(eventTypes[0]), eventTypes,
                      obj->npp(), NULL);
  ShowWindow(window);
  return window;
}

class CarbonFullscreenWindowMac : public FullscreenWindowMac {
 public:
  CarbonFullscreenWindowMac(PluginObject* obj);
  virtual ~CarbonFullscreenWindowMac();

  virtual bool Initialize(int target_width, int target_height);
  virtual bool Shutdown(const GLint* last_buffer_rect);
  virtual CGRect GetWindowBounds() const;
  virtual bool IsActive() const;

 private:
  PluginObject* obj_;
  WindowRef fullscreen_window_;
  Ptr fullscreen_state_;

  DISALLOW_COPY_AND_ASSIGN(CarbonFullscreenWindowMac);
};

CarbonFullscreenWindowMac::CarbonFullscreenWindowMac(PluginObject* obj)
    : obj_(obj),
      fullscreen_window_(NULL),
      fullscreen_state_(NULL) {
}

CarbonFullscreenWindowMac::~CarbonFullscreenWindowMac() {
}

bool CarbonFullscreenWindowMac::Initialize(int target_width,
                                           int target_height) {
  // check which mode we are in now
  o3d::DisplayMode current_mode;
  GetCurrentDisplayMode(&current_mode);

  WindowRef temp_window = NULL;

  // Determine if screen mode switching is actually required.
  if (target_width != 0 &&
      target_height != 0 &&
      target_width != current_mode.width() &&
      target_height != current_mode.height()) {
    short short_target_width = target_width;
    short short_target_height = target_height;
    BeginFullScreen(&fullscreen_state_,
                    nil,  // Value of nil selects the main screen.
                    &short_target_width,
                    &short_target_height,
                    &temp_window,
                    NULL,
                    fullScreenCaptureAllDisplays);
  } else {
    SetSystemUIMode(kUIModeAllSuppressed, kUIOptionAutoShowMenuBar);
    fullscreen_state_ = NULL;
  }

  fullscreen_window_ = CreateFullscreenWindow(NULL, obj_);
  SetWindowForAGLContext(obj_->GetMacAGLContext(), fullscreen_window_);
  aglDisable(obj_->GetMacAGLContext(), AGL_BUFFER_RECT);
  // This must be done after all of the above setup in order for the
  // overlay window to appear on top.
  FullscreenWindowMac::Initialize(target_width, target_height);
  return true;
}

bool CarbonFullscreenWindowMac::Shutdown(const GLint* last_buffer_rect) {
  FullscreenWindowMac::Shutdown(last_buffer_rect);
  SetWindowForAGLContext(obj_->GetMacAGLContext(), obj_->GetMacWindow());
  aglSetInteger(obj_->GetMacAGLContext(), AGL_BUFFER_RECT, last_buffer_rect);
  aglEnable(obj_->GetMacAGLContext(), AGL_BUFFER_RECT);
  if (fullscreen_window_) {
    HideWindow(fullscreen_window_);
    ReleaseWindowGroup(GetWindowGroup(fullscreen_window_));
    DisposeWindow(fullscreen_window_);
    fullscreen_window_ = NULL;
  }
  if (fullscreen_state_) {
    EndFullScreen(fullscreen_state_, 0);
    fullscreen_state_ = NULL;
  } else {
    SetSystemUIMode(kUIModeNormal, 0);
  }
  return true;
}

CGRect CarbonFullscreenWindowMac::GetWindowBounds() const {
  Rect bounds = {0,0,0,0};
  ::GetWindowBounds(fullscreen_window_, kWindowContentRgn, &bounds);
  return Rect2CGRect(bounds);
}

bool CarbonFullscreenWindowMac::IsActive() const {
  return fullscreen_window_ == ActiveNonFloatingWindow();
}

//----------------------------------------------------------------------
// FullscreenWindowMac implementation.
//

FullscreenWindowMac* FullscreenWindowMac::Create(
    glue::_o3d::PluginObject* obj,
    int target_width,
    int target_height) {
  FullscreenWindowMac* window = new CarbonFullscreenWindowMac(obj);
  if (!window->Initialize(target_width, target_height)) {
    delete window;
    return NULL;
  }
  
  return window;
}

FullscreenWindowMac::~FullscreenWindowMac() {
}

bool FullscreenWindowMac::Initialize(int target_width, int target_height) {
#ifdef O3D_PLUGIN_ENABLE_FULLSCREEN_MSG
  overlay_window_.reset(new OverlayWindowMac());
#endif  
  return true;
}

void FullscreenWindowMac::IdleCallback() {
#ifdef O3D_PLUGIN_ENABLE_FULLSCREEN_MSG
  if (overlay_window_.get()) {
    overlay_window_->IdleCallback();
  }
#endif  
}

bool FullscreenWindowMac::Shutdown(const GLint* last_buffer_rect) {
#ifdef O3D_PLUGIN_ENABLE_FULLSCREEN_MSG
  overlay_window_.reset();
#endif
  return true;
}

}  // namespace o3d

