// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_HEARTBEAT_SENDER_H_
#define REMOTING_HOST_HEARTBEAT_SENDER_H_

#include <string>

#include "base/scoped_ptr.h"
#include "base/ref_counted.h"
#include "remoting/host/host_key_pair.h"
#include "remoting/jingle_glue/iq_request.h"
#include "testing/gtest/include/gtest/gtest_prod.h"

namespace remoting {

class IqRequest;
class HostKeyPair;
class JingleClient;
class MutableHostConfig;

// HeartbeatSender periodically sends heartbeat stanzas to the Chromoting Bot.
// Each heartbeat stanza looks as follows:
//
// <iq type="set" to="remoting@bot.talk.google.com"
//     from="user@gmail.com/chromoting123123" id="5" xmlns="jabber:client">
//   <rem:heartbeat rem:hostid="a1ddb11e-8aef-11df-bccf-18a905b9cb5a"
//                  xmlns:rem="google:remoting">
//     <rem:signature rem:time="1279061748">.signature.</rem:signature>
//   </rem:heartbeat>
// </iq>
//
// The time attribute of the signature is the decimal time when the message
// was sent in second since the epoch (01/01/1970). The signature is a BASE64
// encoded SHA-1/RSA signature created with the host's private key. The message
// being signed is the full Jid concatenated with the time value, separated by
// space. For example, for the heartbeat stanza above the message that is being
// signed is "user@gmail.com/chromoting123123 1279061748".
// TODO(sergeyu): Is it enough to sign JID and nothing else?
class HeartbeatSender : public base::RefCountedThreadSafe<HeartbeatSender> {
 public:
  HeartbeatSender();
  virtual ~HeartbeatSender();

  // Initializes heart-beating for |jingle_client| with the specified
  // config. Returns false if the config is invalid (e.g. private key
  // cannot be parsed).
  bool Init(MutableHostConfig* config, JingleClient* jingle_client);

  // Starts heart-beating. Must be called after init.
  void Start();

  // Stops heart-beating. Must be called before corresponding JingleClient
  // is destroyed. This object will not be deleted until Stop() is called,
  // and it may (and will) crash after JingleClient is destroyed. Heartbeating
  // cannot be restarted after it has been stopped, A new sender must be created
  // instead.
  void Stop();

 private:
  FRIEND_TEST_ALL_PREFIXES(HeartbeatSenderTest, DoSendStanza);
  FRIEND_TEST_ALL_PREFIXES(HeartbeatSenderTest, CreateHeartbeatMessage);

  enum State {
    CREATED,
    INITIALIZED,
    STARTED,
    STOPPED,
  };

  void DoSendStanza();

  // Helper methods used by DoSendStanza() to generate heartbeat stanzas.
  // Caller owns the result.
  buzz::XmlElement* CreateHeartbeatMessage();
  buzz::XmlElement* CreateSignature();

  void ProcessResponse(const buzz::XmlElement* response);

  State state_;
  scoped_refptr<MutableHostConfig> config_;
  JingleClient* jingle_client_;
  scoped_ptr<IqRequest> request_;
  std::string host_id_;
  HostKeyPair key_pair_;

  DISALLOW_COPY_AND_ASSIGN(HeartbeatSender);
};

}  // namespace remoting

#endif  // REMOTING_HOST_HEARTBEAT_SENDER_H_
