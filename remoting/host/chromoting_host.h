// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CHROMOTING_HOST_H_
#define REMOTING_CHROMOTING_HOST_H_

#include <string>

#include "base/thread.h"
#include "remoting/base/encoder.h"
#include "remoting/host/access_verifier.h"
#include "remoting/host/capturer.h"
#include "remoting/host/heartbeat_sender.h"
#include "remoting/jingle_glue/jingle_client.h"
#include "remoting/jingle_glue/jingle_thread.h"
#include "remoting/protocol/session_manager.h"
#include "remoting/protocol/connection_to_client.h"

class Task;

namespace remoting {

namespace protocol {
class ConnectionToClient;
class HostStub;
class InputStub;
class SessionConfig;
}  // namespace protocol

class Capturer;
class ChromotingHostContext;
class Encoder;
class MutableHostConfig;
class SessionManager;

// A class to implement the functionality of a host process.
//
// Here's the work flow of this class:
// 1. We should load the saved GAIA ID token or if this is the first
//    time the host process runs we should prompt user for the
//    credential. We will use this token or credentials to authenicate
//    and register the host.
//
// 2. We listen for incoming connection using libjingle. We will create
//    a ConnectionToClient object that wraps around linjingle for transport.
//    Also create a SessionManager with appropriate Encoder and Capturer and
//    add the ConnectionToClient to this SessionManager for transporting the
//    screen captures. An InputStub is created and registered with the
//    ConnectionToClient to receive mouse / keyboard events from the remote
//    client.
//    This is also the right time to create multiple threads to host
//    the above objects. After we have done all the initialization
//    we'll start the SessionManager. We'll then enter the running state
//    of the host process.
//
// 3. When the user is disconencted, we will pause the SessionManager
//    and try to terminate the threads we have created. This will allow
//    all pending tasks to complete. After all of that completed we
//    return to the idle state. We then go to step (2) if there a new
//    incoming connection.
class ChromotingHost : public base::RefCountedThreadSafe<ChromotingHost>,
                       public protocol::ConnectionToClient::EventHandler,
                       public JingleClient::Callback {
 public:
  ChromotingHost(ChromotingHostContext* context, MutableHostConfig* config,
                 Capturer* capturer, protocol::InputStub* input_stub);
  virtual ~ChromotingHost();

  // Asynchronously start the host process.
  //
  // After this is invoked, the host process will connect to the talk
  // network and start listening for incoming connections.
  //
  // |shutdown_task| is called if Start() has failed ot Shutdown() is called
  // and all related operations are completed.
  //
  // This method can only be called once during the lifetime of this object.
  void Start(Task* shutdown_task);

  // Asynchronously shutdown the host process.
  void Shutdown();

  // This method is called if a client is connected to this object.
  void OnClientConnected(protocol::ConnectionToClient* client);

  // This method is called if a client is disconnected from the host.
  void OnClientDisconnected(protocol::ConnectionToClient* client);

  ////////////////////////////////////////////////////////////////////////////
  // protocol::ConnectionToClient::EventHandler implementations
  virtual void OnConnectionOpened(protocol::ConnectionToClient* client);
  virtual void OnConnectionClosed(protocol::ConnectionToClient* client);
  virtual void OnConnectionFailed(protocol::ConnectionToClient* client);

  ////////////////////////////////////////////////////////////////////////////
  // JingleClient::Callback implementations
  virtual void OnStateChange(JingleClient* client, JingleClient::State state);

  // Callback for ChromotingServer.
  void OnNewClientSession(
      protocol::Session* session,
      protocol::SessionManager::IncomingSessionResponse* response);

 private:
  enum State {
    kInitial,
    kStarted,
    kStopped,
  };

  // This method connects to the talk network and start listening for incoming
  // connections.
  void DoStart(Task* shutdown_task);

  // Callback for protocol::SessionManager::Close().
  void OnServerClosed();

  // Creates encoder for the specified configuration.
  Encoder* CreateEncoder(const protocol::SessionConfig* config);

  // The context that the chromoting host runs on.
  ChromotingHostContext* context_;

  scoped_refptr<MutableHostConfig> config_;

  // Capturer to be used by SessionManager. Once the SessionManager is
  // constructed this is set to NULL.
  scoped_ptr<Capturer> capturer_;

  // constructed this is set to NULL.
  scoped_ptr<Encoder> encoder_;

  // InputStub in the host executes input events received from the client.
  scoped_ptr<protocol::InputStub> input_stub_;

  // HostStub in the host executes control events received from the client.
  scoped_ptr<protocol::HostStub> host_stub_;

  // The libjingle client. This is used to connect to the talk network to
  // receive connection requests from chromoting client.
  scoped_refptr<JingleClient> jingle_client_;

  scoped_refptr<protocol::SessionManager> session_manager_;

  // Objects that takes care of sending heartbeats to the chromoting bot.
  scoped_refptr<HeartbeatSender> heartbeat_sender_;

  AccessVerifier access_verifier_;

  // A ConnectionToClient manages the connectino to a remote client.
  // TODO(hclam): Expand this to a list of clients.
  scoped_refptr<protocol::ConnectionToClient> connection_;

  // Session manager for the host process.
  scoped_refptr<SessionManager> session_;

  // This task gets executed when this object fails to connect to the
  // talk network or Shutdown() is called.
  scoped_ptr<Task> shutdown_task_;

  // Tracks the internal state of the host.
  // This variable is written on the main thread of ChromotingHostContext
  // and read by jingle thread.
  State state_;

  // Lock is to lock the access to |state_|.
  Lock lock_;

  DISALLOW_COPY_AND_ASSIGN(ChromotingHost);
};

}  // namespace remoting

#endif  // REMOTING_HOST_CHROMOTING_HOST_H_
