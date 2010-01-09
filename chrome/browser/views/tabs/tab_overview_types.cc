// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/views/tabs/tab_overview_types.h"

#include <gdk/gdkx.h>
extern "C" {
#include <X11/Xlib.h>
}

#include "base/logging.h"
#include "base/singleton.h"
#include "base/scoped_ptr.h"
#include "chrome/common/x11_util.h"

namespace {

// A value from the Atom enum and the actual name that should be used to
// look up its ID on the X server.
struct AtomInfo {
  TabOverviewTypes::AtomType atom;
  const char* name;
};

// Each value from the Atom enum must be present here.
static const AtomInfo kAtomInfos[] = {
  { TabOverviewTypes::ATOM_CHROME_WINDOW_TYPE,
    "_CHROME_WINDOW_TYPE" },
  { TabOverviewTypes::ATOM_CHROME_WM_MESSAGE,
    "_CHROME_WM_MESSAGE" },
  { TabOverviewTypes::ATOM_MANAGER,
    "MANAGER" },
  { TabOverviewTypes::ATOM_NET_SUPPORTING_WM_CHECK,
    "_NET_SUPPORTING_WM_CHECK" },
  { TabOverviewTypes::ATOM_NET_WM_NAME,
    "_NET_WM_NAME" },
  { TabOverviewTypes::ATOM_PRIMARY,
    "PRIMARY" },
  { TabOverviewTypes::ATOM_STRING,
    "STRING" },
  { TabOverviewTypes::ATOM_UTF8_STRING,
    "UTF8_STRING" },
  { TabOverviewTypes::ATOM_WM_NORMAL_HINTS,
    "WM_NORMAL_HINTS" },
  { TabOverviewTypes::ATOM_WM_S0,
    "WM_S0" },
  { TabOverviewTypes::ATOM_WM_STATE,
    "WM_STATE" },
  { TabOverviewTypes::ATOM_WM_TRANSIENT_FOR,
    "WM_TRANSIENT_FOR" },
  { TabOverviewTypes::ATOM_WM_SYSTEM_METRICS,
    "WM_SYSTEM_METRICS" },
};

bool SetIntProperty(XID xid, Atom xatom, const std::vector<int>& values) {
  DCHECK(!values.empty());

  // TODO: Trap errors and return false on failure.
  XChangeProperty(x11_util::GetXDisplay(),
                  xid,
                  xatom,
                  xatom,
                  32,  // size in bits of items in 'value'
                  PropModeReplace,
                  reinterpret_cast<const unsigned char*>(&values.front()),
                  values.size());  // num items
  XFlush(x11_util::GetXDisplay());
  return true;
}

}  // namespace

// static
TabOverviewTypes* TabOverviewTypes::instance() {
  static TabOverviewTypes* instance = NULL;
  if (!instance)
    instance = Singleton<TabOverviewTypes>::get();
  return instance;
}

bool TabOverviewTypes::SetWindowType(
    GtkWidget* widget,
    WindowType type,
    const std::vector<int>* params) {
  std::vector<int> values;
  values.push_back(type);
  if (params)
    values.insert(values.end(), params->begin(), params->end());
  return SetIntProperty(x11_util::GetX11WindowFromGtkWidget(widget),
                        type_to_atom_[ATOM_CHROME_WINDOW_TYPE], values);
}

void TabOverviewTypes::SendMessage(const Message& msg) {
  XEvent e;
  e.xclient.type = ClientMessage;
  e.xclient.window = wm_;
  e.xclient.message_type = type_to_atom_[ATOM_CHROME_WM_MESSAGE];
  e.xclient.format = 32;  // 32-bit values
  e.xclient.data.l[0] = msg.type();

  // XClientMessageEvent only gives us five 32-bit items, and we're using
  // the first one for our message type.
  DCHECK_LE(msg.max_params(), 4);
  for (int i = 0; i < msg.max_params(); ++i)
    e.xclient.data.l[i+1] = msg.param(i);

  XSendEvent(x11_util::GetXDisplay(),
             wm_,
             False,  // propagate
             0,  // empty event mask
             &e);
}

bool TabOverviewTypes::DecodeMessage(const GdkEventClient& event,
                                     Message* msg) {
  if (wm_message_atom_ != gdk_x11_atom_to_xatom(event.message_type))
    return false;

  if (event.data_format != 32) {
    DLOG(WARNING) << "Ignoring ClientEventMessage with invalid bit "
                  << "format " << event.data_format
                  << " (expected 32-bit values)";
    return false;
  }

  msg->set_type(static_cast<Message::Type>(event.data.l[0]));
  if (msg->type() < 0 || msg->type() >= Message::kNumTypes) {
    DLOG(WARNING) << "Ignoring ClientEventMessage with invalid message "
                  << "type " << msg->type();
    return false;
  }

  // XClientMessageEvent only gives us five 32-bit items, and we're using
  // the first one for our message type.
  DCHECK_LE(msg->max_params(), 4);
  for (int i = 0; i < msg->max_params(); ++i)
    msg->set_param(i, event.data.l[i+1]);  // l[0] contains message type

  return true;
}

bool TabOverviewTypes::DecodeStringMessage(const GdkEventProperty& event,
                                           std::string* msg) {
  DCHECK(NULL != msg);
  if (type_to_atom_[ATOM_WM_SYSTEM_METRICS] !=
      gdk_x11_atom_to_xatom(event.atom))
    return false;

  DLOG(WARNING) << "Got property change notification for system metrics.";
  if (GDK_PROPERTY_DELETE == event.state) {
    DLOG(WARNING) << "Ignoring delete EventPropertyNotification";
    return false;
  }

  // We will be using DBus for this communication in the future, so I don't
  // really worry right now that we could generate more than 1KB here.
  // Also, I use "long" rather than int64 because that is what X expects.
  long acceptable_bytes = 1024;
  Atom actual_type;
  int actual_format;
  unsigned long num_items, bytes_left;
  unsigned char *output;
  if (Success != XGetWindowProperty(x11_util::GetXDisplay(),
                                    GDK_WINDOW_XID(event.window),
                                    type_to_atom_[ATOM_WM_SYSTEM_METRICS],
                                    0,
                                    acceptable_bytes,
                                    false,
                                    AnyPropertyType,
                                    &actual_type,
                                    &actual_format,
                                    &num_items,
                                    &bytes_left,
                                    &output)) {
    DLOG(WARNING) << "Could not read system metrics property from X.";
    return false;
  }
  if (actual_format == 0) {
    DLOG(WARNING) << "System Metrics property not set.";
    return false;
  }
  if (actual_format != 8) {
    DLOG(WARNING) << "Message was not encoded as a string of bytes...";
    return false;
  }
  if (bytes_left != 0) {
    DLOG(ERROR) << "We wanted all the bytes at once...";
    return false;
  }
  msg->assign(reinterpret_cast<char*>(output), num_items);
  XFree(output);
  return true;
}

TabOverviewTypes::TabOverviewTypes() {
  scoped_array<char*> names(new char*[kNumAtoms]);
  scoped_array<Atom> atoms(new Atom[kNumAtoms]);

  for (int i = 0; i < kNumAtoms; ++i) {
    // Need to const_cast because XInternAtoms() wants a char**.
    names[i] = const_cast<char*>(kAtomInfos[i].name);
  }

  XInternAtoms(x11_util::GetXDisplay(), names.get(), kNumAtoms,
               False,  // only_if_exists
               atoms.get());

  for (int i = 0; i < kNumAtoms; ++i) {
    type_to_atom_[kAtomInfos[i].atom] = atoms[i];
    atom_to_string_[atoms[i]] = kAtomInfos[i].name;
  }

  wm_message_atom_ = type_to_atom_[ATOM_CHROME_WM_MESSAGE];

  wm_ = XGetSelectionOwner(x11_util::GetXDisplay(), type_to_atom_[ATOM_WM_S0]);

  // Let the window manager know which version of the IPC messages we support.
  Message msg(Message::WM_NOTIFY_IPC_VERSION);
  // TODO: The version number is the latest listed in tab_overview_types.h --
  // ideally, once this header is shared between Chrome and the Chrome OS window
  // manager, we'll just define the version statically in the header.
  msg.set_param(0, 1);
  SendMessage(msg);
}
