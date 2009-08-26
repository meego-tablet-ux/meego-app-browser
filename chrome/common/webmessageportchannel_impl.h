// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_WEBMESSAGEPORTCHANNEL_IMPL_H_
#define CHROME_COMMON_WEBMESSAGEPORTCHANNEL_IMPL_H_

#include <queue>
#include <vector>

#include "base/basictypes.h"
#include "base/lock.h"
#include "base/string16.h"
#include "base/ref_counted.h"
#include "ipc/ipc_channel.h"
#include "webkit/api/public/WebMessagePortChannel.h"

// This is thread safe.
class WebMessagePortChannelImpl
    : public WebKit::WebMessagePortChannel,
      public IPC::Channel::Listener,
      public base::RefCountedThreadSafe<WebMessagePortChannelImpl> {
 public:
  WebMessagePortChannelImpl();
  WebMessagePortChannelImpl(int route_id, int message_port_id);

  // Queues received and incoming messages until there are no more in-flight
  // messages, then sends all of them to the browser process.
  void QueueMessages();
  int message_port_id() const { return message_port_id_; }

 private:
  friend class base::RefCountedThreadSafe<WebMessagePortChannelImpl>;
  ~WebMessagePortChannelImpl();

  // WebMessagePortChannel implementation.
  virtual void setClient(WebKit::WebMessagePortChannelClient* client);
  virtual void destroy();
  virtual void entangle(WebKit::WebMessagePortChannel* channel);
  virtual void postMessage(const WebKit::WebString& message,
                           WebKit::WebMessagePortChannelArray* channels);
  virtual bool tryGetMessage(WebKit::WebString* message,
                             WebKit::WebMessagePortChannelArray& channels);

  void Init();
  void Entangle(scoped_refptr<WebMessagePortChannelImpl> channel);
  void Send(IPC::Message* message);

  // IPC::Channel::Listener implementation.
  virtual void OnMessageReceived(const IPC::Message& message);

  void OnMessage(const string16& message,
                 const std::vector<int>& sent_message_port_ids,
                 const std::vector<int>& new_routing_ids);
  void OnMessagedQueued();

  struct Message {
    string16 message;
    std::vector<WebMessagePortChannelImpl*> ports;
  };

  typedef std::queue<Message> MessageQueue;
  MessageQueue message_queue_;

  WebKit::WebMessagePortChannelClient* client_;
  Lock lock_;  // Locks access to above.

  int route_id_;  // The routing id for this object.
  int message_port_id_;  // A globally unique identifier for this message port.

  DISALLOW_COPY_AND_ASSIGN(WebMessagePortChannelImpl);
};

#endif  // CHROME_COMMON_WEBMESSAGEPORTCHANNEL_IMPL_H_
