// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_PROTOCOL_HOST_MESSAGE_DISPATCHER_H_
#define REMOTING_PROTOCOL_HOST_MESSAGE_DISPATCHER_H_

#include "base/basictypes.h"
#include "base/ref_counted.h"
#include "base/scoped_ptr.h"
#include "base/task.h"

namespace remoting {

class ChromotocolConnection;
class EventMessage;
class MessageReader;

namespace protocol {

class ControlMessage;
class HostStub;
class InputStub;

// A message dispatcher used to listen for messages received in
// ChromotocolConnection. It dispatches messages to the corresponding
// handler.
//
// Internally it contains an EventStreamReader that decodes data on
// communications channels into protocol buffer messages.
// EventStreamReader is registered with ChromotocolConnection given to it.
//
// Object of this class is owned by ChromotingHost to dispatch messages
// to itself.
class HostMessageDispatcher :
      public base::RefCountedThreadSafe<HostMessageDispatcher> {
 public:
  // Construct a message dispatcher.
  HostMessageDispatcher();
  virtual ~HostMessageDispatcher();

  // Initialize the message dispatcher with the given connection and
  // message handlers.
  // Return true if initalization was successful.
  bool Initialize(ChromotocolConnection* connection,
                  HostStub* host_stub, InputStub* input_stub);

 private:
  // This method is called by |control_channel_reader_| when a control
  // message is received.
  void OnControlMessageReceived(ControlMessage* message);

  // This method is called by |event_channel_reader_| when a event
  // message is received.
  void OnEventMessageReceived(EventMessage* message);

  // MessageReader that runs on the control channel. It runs a loop
  // that parses data on the channel and then delegates the message to this
  // class.
  scoped_ptr<MessageReader> control_message_reader_;

  // MessageReader that runs on the event channel.
  scoped_ptr<MessageReader> event_message_reader_;

  // Stubs for host and input. These objects are not owned.
  // They are called on the thread there data is received, i.e. jingle thread.
  HostStub* host_stub_;
  InputStub* input_stub_;
};

}  // namespace protocol
}  // namespace remoting

#endif  // REMOTING_PROTOCOL_HOST_MESSAGE_DISPATCHER_H_
