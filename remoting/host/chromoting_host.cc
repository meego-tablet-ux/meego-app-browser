// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/chromoting_host.h"

#include "base/stl_util-inl.h"
#include "base/task.h"
#include "build/build_config.h"
#include "remoting/base/constants.h"
#include "remoting/base/encoder.h"
#include "remoting/base/encoder_verbatim.h"
#include "remoting/base/encoder_vp8.h"
#include "remoting/base/encoder_zlib.h"
#include "remoting/host/chromoting_host_context.h"
#include "remoting/host/capturer.h"
#include "remoting/host/host_config.h"
#include "remoting/host/host_stub_fake.h"
#include "remoting/host/session_manager.h"
#include "remoting/protocol/connection_to_client.h"
#include "remoting/protocol/host_stub.h"
#include "remoting/protocol/input_stub.h"
#include "remoting/protocol/jingle_session_manager.h"
#include "remoting/protocol/session_config.h"

using remoting::protocol::ConnectionToClient;

namespace remoting {

ChromotingHost::ChromotingHost(ChromotingHostContext* context,
                               MutableHostConfig* config,
                               Capturer* capturer,
                               protocol::InputStub* input_stub)
    : context_(context),
      config_(config),
      capturer_(capturer),
      input_stub_(input_stub),
      host_stub_(new HostStubFake()),
      state_(kInitial) {
}

ChromotingHost::~ChromotingHost() {
}

void ChromotingHost::Start(Task* shutdown_task) {
  if (MessageLoop::current() != context_->main_message_loop()) {
    context_->main_message_loop()->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &ChromotingHost::Start, shutdown_task));
    return;
  }

  DCHECK(!jingle_client_);
  DCHECK(shutdown_task);

  // Make sure this object is not started.
  {
    AutoLock auto_lock(lock_);
    if (state_ != kInitial)
      return;
    state_ = kStarted;
  }

  // Save the shutdown task.
  shutdown_task_.reset(shutdown_task);

  std::string xmpp_login;
  std::string xmpp_auth_token;
  if (!config_->GetString(kXmppLoginConfigPath, &xmpp_login) ||
      !config_->GetString(kXmppAuthTokenConfigPath, &xmpp_auth_token)) {
    LOG(ERROR) << "XMPP credentials are not defined in the config.";
    return;
  }

  if (!access_verifier_.Init(config_))
    return;

  // Connect to the talk network with a JingleClient.
  jingle_client_ = new JingleClient(context_->jingle_thread());
  jingle_client_->Init(xmpp_login, xmpp_auth_token,
                       kChromotingTokenServiceName, this);

  heartbeat_sender_ = new HeartbeatSender();
  if (!heartbeat_sender_->Init(config_, jingle_client_.get())) {
    LOG(ERROR) << "Failed to initialize HeartbeatSender.";
    return;
  }
}

// This method is called when we need to destroy the host process.
void ChromotingHost::Shutdown() {
  if (MessageLoop::current() != context_->main_message_loop()) {
    context_->main_message_loop()->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &ChromotingHost::Shutdown));
    return;
  }

  // No-op if this object is not started yet.
  {
    AutoLock auto_lock(lock_);
    if (state_ != kStarted) {
      state_ = kStopped;
      return;
    }
    state_ = kStopped;
  }

  // Tell the session to pause and then disconnect all clients.
  if (session_.get()) {
    session_->Pause();
    session_->RemoveAllConnections();
  }

  // Disconnect the client.
  if (connection_) {
    connection_->Disconnect();
  }

  // Stop the heartbeat sender.
  if (heartbeat_sender_) {
    heartbeat_sender_->Stop();
  }

  // Stop chromotocol session manager.
  if (session_manager_) {
    session_manager_->Close(
        NewRunnableMethod(this, &ChromotingHost::OnServerClosed));
  }

  // Disconnect from the talk network.
  if (jingle_client_) {
    jingle_client_->Close();
  }

  // Lastly call the shutdown task.
  if (shutdown_task_.get()) {
    shutdown_task_->Run();
  }
}

// This method is called when a client connects.
void ChromotingHost::OnClientConnected(ConnectionToClient* connection) {
  DCHECK_EQ(context_->main_message_loop(), MessageLoop::current());

  // Create a new RecordSession if there was none.
  if (!session_.get()) {
    // Then we create a SessionManager passing the message loops that
    // it should run on.
    DCHECK(capturer_.get());

    Encoder* encoder = CreateEncoder(connection->session()->config());

    session_ = new SessionManager(context_->capture_message_loop(),
                                  context_->encode_message_loop(),
                                  context_->main_message_loop(),
                                  capturer_.release(),
                                  encoder);
  }

  // Immediately add the connection and start the session.
  session_->AddConnection(connection);
  session_->Start();
  VLOG(1) << "Session manager started";
}

void ChromotingHost::OnClientDisconnected(ConnectionToClient* connection) {
  DCHECK_EQ(context_->main_message_loop(), MessageLoop::current());

  // Remove the connection from the session manager and pause the session.
  // TODO(hclam): Pause only if the last connection disconnected.
  if (session_.get()) {
    session_->RemoveConnection(connection);
    session_->Pause();
  }

  // Close the connection to connection just to be safe.
  connection->Disconnect();

  // Also remove reference to ConnectionToClient from this object.
  connection_ = NULL;
}

////////////////////////////////////////////////////////////////////////////
// protocol::ConnectionToClient::EventHandler implementations
void ChromotingHost::OnConnectionOpened(ConnectionToClient* connection) {
  DCHECK_EQ(context_->main_message_loop(), MessageLoop::current());

  // Completes the connection to the client.
  VLOG(1) << "Connection to client established.";
  OnClientConnected(connection_.get());
}

void ChromotingHost::OnConnectionClosed(ConnectionToClient* connection) {
  DCHECK_EQ(context_->main_message_loop(), MessageLoop::current());

  VLOG(1) << "Connection to client closed.";
  OnClientDisconnected(connection_.get());
}

void ChromotingHost::OnConnectionFailed(ConnectionToClient* connection) {
  DCHECK_EQ(context_->main_message_loop(), MessageLoop::current());

  LOG(ERROR) << "Connection failed unexpectedly.";
  OnClientDisconnected(connection_.get());
}

////////////////////////////////////////////////////////////////////////////
// JingleClient::Callback implementations
void ChromotingHost::OnStateChange(JingleClient* jingle_client,
                                   JingleClient::State state) {
  if (state == JingleClient::CONNECTED) {
    DCHECK_EQ(jingle_client_.get(), jingle_client);
    VLOG(1) << "Host connected as " << jingle_client->GetFullJid();

    // Create and start session manager.
    protocol::JingleSessionManager* server =
        new protocol::JingleSessionManager(context_->jingle_thread());
    // TODO(ajwong): Make this a command switch when we're more stable.
    server->set_allow_local_ips(true);
    server->Init(jingle_client->GetFullJid(),
                 jingle_client->session_manager(),
                 NewCallback(this, &ChromotingHost::OnNewClientSession));
    session_manager_ = server;

    // Start heartbeating.
    heartbeat_sender_->Start();
  } else if (state == JingleClient::CLOSED) {
    VLOG(1) << "Host disconnected from talk network.";

    // Stop heartbeating.
    heartbeat_sender_->Stop();

    // TODO(sergeyu): We should try reconnecting here instead of terminating
    // the host.
    Shutdown();
  }
}

void ChromotingHost::OnNewClientSession(
    protocol::Session* session,
    protocol::SessionManager::IncomingSessionResponse* response) {
  AutoLock auto_lock(lock_);
  // TODO(hclam): Allow multiple clients to connect to the host.
  if (connection_.get() || state_ != kStarted) {
    *response = protocol::SessionManager::DECLINE;
    return;
  }

  // Check that the user has access to the host.
  if (!access_verifier_.VerifyPermissions(session->jid())) {
    *response = protocol::SessionManager::DECLINE;
    return;
  }

  scoped_ptr<protocol::CandidateSessionConfig>
      local_config(protocol::CandidateSessionConfig::CreateDefault());
  local_config->SetInitialResolution(
      protocol::ScreenResolution(capturer_->width(), capturer_->height()));
  // TODO(sergeyu): Respect resolution requested by the client if supported.
  protocol::SessionConfig* config = local_config->Select(
      session->candidate_config(), true /* force_host_resolution */);

  if (!config) {
    LOG(WARNING) << "Rejecting connection from " << session->jid()
                 << " because no compatible configuration has been found.";
    *response = protocol::SessionManager::INCOMPATIBLE;
    return;
  }

  session->set_config(config);

  *response = protocol::SessionManager::ACCEPT;

  VLOG(1) << "Client connected: " << session->jid();

  // If we accept the connected then create a client object and set the
  // callback.
  connection_ = new ConnectionToClient(context_->main_message_loop(),
                                       this, host_stub_.get(),
                                       input_stub_.get());
  connection_->Init(session);
}

void ChromotingHost::OnServerClosed() {
  // Don't need to do anything here.
}

// TODO(sergeyu): Move this to SessionManager?
Encoder* ChromotingHost::CreateEncoder(const protocol::SessionConfig* config) {
  const protocol::ChannelConfig& video_config = config->video_config();

  if (video_config.codec == protocol::ChannelConfig::CODEC_VERBATIM) {
    return new remoting::EncoderVerbatim();
  } else if (video_config.codec == protocol::ChannelConfig::CODEC_ZIP) {
    return new remoting::EncoderZlib();
  }
  // TODO(sergeyu): Enable VP8 on ARM builds.
#if !defined(ARCH_CPU_ARM_FAMILY)
  else if (video_config.codec == protocol::ChannelConfig::CODEC_VP8) {
    return new remoting::EncoderVp8();
  }
#endif

  return NULL;
}

}  // namespace remoting
