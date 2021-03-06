// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_WEB_UI_BINDINGS_H_
#define CONTENT_RENDERER_WEB_UI_BINDINGS_H_
#pragma once

#include "ipc/ipc_message.h"
#include "webkit/glue/cpp_bound_class.h"

// A DOMBoundBrowserObject is a backing for some object bound to the window
// in JS that knows how to dispatch messages to an associated c++ object living
// in the browser process.
class DOMBoundBrowserObject : public CppBoundClass {
 public:
  DOMBoundBrowserObject();
  virtual ~DOMBoundBrowserObject();

  // Set the message channel back to the browser.
  void set_message_sender(IPC::Message::Sender* sender) {
    sender_ = sender;
  }

  // Set the routing id for messages back to the browser.
  void set_routing_id(int routing_id) {
    routing_id_ = routing_id;
  }

  IPC::Message::Sender* sender() { return sender_; }
  int routing_id() { return routing_id_; }

  // Sets a property with the given name and value.
  void SetProperty(const std::string& name, const std::string& value);

 private:
  // Our channel back to the browser is a message sender
  // and routing id.
  IPC::Message::Sender* sender_;
  int routing_id_;

  // The list of properties that have been set.  We keep track of this so we
  // can free them on destruction.
  typedef std::vector<CppVariant*> PropertyList;
  PropertyList properties_;

  DISALLOW_COPY_AND_ASSIGN(DOMBoundBrowserObject);
};

// WebUIBindings is the class backing the "chrome" object accessible
// from Javascript from privileged pages.
//
// We expose one function, for sending a message to the browser:
//   send(String name, Object argument);
// It's plumbed through to the OnWebUIMessage callback on RenderViewHost
// delegate.
class WebUIBindings : public DOMBoundBrowserObject {
 public:
  WebUIBindings();
  virtual ~WebUIBindings();

  // The send() function provided to Javascript.
  void send(const CppArgumentList& args, CppVariant* result);
 private:
  DISALLOW_COPY_AND_ASSIGN(WebUIBindings);
};

#endif  // CONTENT_RENDERER_WEB_UI_BINDINGS_H_
