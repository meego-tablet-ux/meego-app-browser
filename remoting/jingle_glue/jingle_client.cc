// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(ajwong): Check the initialization sentinels.  Can we base it off of
// state_ instead of a member variable?  Also, we assign and read from a few of
// the member variables on two threads.  We need to audit this for thread
// safety.

#include "remoting/jingle_glue/jingle_client.h"

#include "base/logging.h"
#include "base/waitable_event.h"
#include "base/message_loop.h"
#include "chrome/common/net/notifier/communicator/gaia_token_pre_xmpp_auth.h"
#include "chrome/common/net/notifier/communicator/xmpp_socket_adapter.h"
#include "remoting/jingle_glue/jingle_thread.h"
#include "remoting/jingle_glue/relay_port_allocator.h"
#include "talk/base/asyncsocket.h"
#include "talk/base/ssladapter.h"
#include "talk/p2p/base/sessionmanager.h"
#include "talk/p2p/client/sessionmanagertask.h"
#ifdef USE_SSL_TUNNEL
#include "talk/session/tunnel/securetunnelsessionclient.h"
#endif
#include "talk/session/tunnel/tunnelsessionclient.h"
#include "talk/xmpp/prexmppauth.h"
#include "talk/xmpp/saslcookiemechanism.h"

namespace remoting {

JingleClient::JingleClient(JingleThread* thread)
    : client_(NULL),
      thread_(thread),
      state_(START),
      callback_(NULL) {
}

JingleClient::~JingleClient() {
  // JingleClient can be destroyed only after it's closed.
  DCHECK(state_ == CLOSED);
}

void JingleClient::Init(
    const std::string& username, const std::string& auth_token,
    const std::string& auth_token_service, Callback* callback) {
  DCHECK(username != "");
  DCHECK(callback != NULL);
  DCHECK(callback_ == NULL);  // Init() can be called only once.

  callback_ = callback;

  message_loop()->PostTask(
      FROM_HERE, NewRunnableMethod(this, &JingleClient::DoInitialize,
                                   username, auth_token, auth_token_service));
}

JingleChannel* JingleClient::Connect(const std::string& host_jid,
                                     JingleChannel::Callback* callback) {
  // Ownership if channel is given to DoConnect.
  scoped_refptr<JingleChannel> channel = new JingleChannel(callback);
  message_loop()->PostTask(
      FROM_HERE, NewRunnableMethod(this, &JingleClient::DoConnect,
                                   channel, host_jid, callback));
  return channel;
}

void JingleClient::DoConnect(scoped_refptr<JingleChannel> channel,
                             const std::string& host_jid,
                             JingleChannel::Callback* callback) {
  DCHECK_EQ(message_loop(), MessageLoop::current());

  talk_base::StreamInterface* stream =
      tunnel_session_client_->CreateTunnel(buzz::Jid(host_jid), "");
  DCHECK(stream != NULL);

  channel->Init(thread_, stream, host_jid);
}

void JingleClient::Close() {
  message_loop()->PostTask(
      FROM_HERE, NewRunnableMethod(this, &JingleClient::DoClose));
}

void JingleClient::DoClose() {
  DCHECK_EQ(message_loop(), MessageLoop::current());
  DCHECK(callback_ != NULL);  // Close() should only be called after Init().

  client_->Disconnect();
  // Client is deleted by TaskRunner.
  client_ = NULL;
  tunnel_session_client_.reset();
  port_allocator_.reset();
  session_manager_.reset();
  network_manager_.reset();
  UpdateState(CLOSED);
}

void JingleClient::DoInitialize(const std::string& username,
                                const std::string& auth_token,
                                const std::string& auth_token_service) {
  DCHECK_EQ(message_loop(), MessageLoop::current());

  buzz::Jid login_jid(username);

  buzz::XmppClientSettings settings;
  settings.set_user(login_jid.node());
  settings.set_host(login_jid.domain());
  settings.set_resource("chromoting");
  settings.set_use_tls(true);
  settings.set_token_service(auth_token_service);
  settings.set_auth_cookie(auth_token);
  settings.set_server(talk_base::SocketAddress("talk.google.com", 5222));

  client_ = new buzz::XmppClient(thread_->task_pump());
  client_->SignalStateChange.connect(
      this, &JingleClient::OnConnectionStateChanged);

  buzz::AsyncSocket* socket =
      new notifier::XmppSocketAdapter(settings, false);

  client_->Connect(settings, "", socket, CreatePreXmppAuth(settings));
  client_->Start();

  network_manager_.reset(new talk_base::NetworkManager());

  RelayPortAllocator* port_allocator =
      new RelayPortAllocator(network_manager_.get(), "transp2");
  port_allocator_.reset(port_allocator);
  port_allocator->SetJingleInfo(client_);

  session_manager_.reset(new cricket::SessionManager(port_allocator_.get()));
#ifdef USE_SSL_TUNNEL
  cricket::SecureTunnelSessionClient* session_client =
      new cricket::SecureTunnelSessionClient(client_->jid(),
                                             session_manager_.get());
  if (!session_client->GenerateIdentity())
    return false;
  tunnel_session_client_.reset(session_client);
#else  // !USE_SSL_TUNNEL
  tunnel_session_client_.reset(
      new cricket::TunnelSessionClient(client_->jid(),
                                       session_manager_.get()));
#endif  // USE_SSL_TUNNEL

  cricket::SessionManagerTask* receiver =
      new cricket::SessionManagerTask(client_, session_manager_.get());
  receiver->EnableOutgoingMessages();
  receiver->Start();

  tunnel_session_client_->SignalIncomingTunnel.connect(
      this, &JingleClient::OnIncomingTunnel);
}

std::string JingleClient::GetFullJid() {
  AutoLock auto_lock(full_jid_lock_);
  return full_jid_;
}

MessageLoop* JingleClient::message_loop() {
  return thread_->message_loop();
}

void JingleClient::OnConnectionStateChanged(buzz::XmppEngine::State state) {
  switch (state) {
    case buzz::XmppEngine::STATE_START:
      UpdateState(START);
      break;
    case buzz::XmppEngine::STATE_OPENING:
      UpdateState(CONNECTING);
      break;
    case buzz::XmppEngine::STATE_OPEN:
      {
        AutoLock auto_lock(full_jid_lock_);
        full_jid_ = client_->jid().Str();
      }
      UpdateState(CONNECTED);
      break;
    case buzz::XmppEngine::STATE_CLOSED:
      UpdateState(CLOSED);
      break;
    default:
      NOTREACHED();
      break;
  }
}

void JingleClient::OnIncomingTunnel(
    cricket::TunnelSessionClient* client, buzz::Jid jid,
    std::string description, cricket::Session* session) {
  // Decline connection if we don't have callback.
  if (!callback_) {
    client->DeclineTunnel(session);
    return;
  }

  JingleChannel::Callback* channel_callback;
  if (callback_->OnAcceptConnection(this, jid.Str(), &channel_callback)) {
    DCHECK(channel_callback != NULL);
    talk_base::StreamInterface* stream =
        client->AcceptTunnel(session);
    scoped_refptr<JingleChannel> channel(new JingleChannel(channel_callback));
    channel->Init(thread_, stream, jid.Str());
    callback_->OnNewConnection(this, channel);
  } else {
    client->DeclineTunnel(session);
    return;
  }
}

void JingleClient::UpdateState(State new_state) {
  if (new_state != state_) {
    state_ = new_state;
    if (callback_)
      callback_->OnStateChange(this, new_state);
  }
}

buzz::PreXmppAuth* JingleClient::CreatePreXmppAuth(
    const buzz::XmppClientSettings& settings) {
  buzz::Jid jid(settings.user(), settings.host(), buzz::STR_EMPTY);
  return new notifier::GaiaTokenPreXmppAuth(jid.Str(), settings.auth_cookie(),
                                            settings.token_service());
}

}  // namespace remoting
