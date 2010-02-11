// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/notifier/listener/listen_task.h"

#include "base/logging.h"
#include "chrome/browser/sync/notification_method.h"
#include "chrome/browser/sync/notifier/listener/notification_constants.h"
#include "chrome/browser/sync/notifier/listener/xml_element_util.h"
#include "talk/base/task.h"
#include "talk/xmllite/qname.h"
#include "talk/xmllite/xmlelement.h"
#include "talk/xmpp/xmppclient.h"
#include "talk/xmpp/xmppconstants.h"
#include "talk/xmpp/xmppengine.h"

namespace browser_sync {

ListenTask::ListenTask(Task* parent, NotificationMethod notification_method)
    : buzz::XmppTask(parent, buzz::XmppEngine::HL_TYPE),
      notification_method_(notification_method) {
}

ListenTask::~ListenTask() {
}

int ListenTask::ProcessStart() {
  LOG(INFO) << "P2P: Listener task started.";
  return STATE_RESPONSE;
}

int ListenTask::ProcessResponse() {
  LOG(INFO) << "P2P: Listener response received.";
  const buzz::XmlElement* stanza = NextStanza();
  if (stanza == NULL) {
    return STATE_BLOCKED;
  }
  // Acknowledge receipt of the notification to the buzz server.
  scoped_ptr<buzz::XmlElement> response_stanza(MakeIqResult(stanza));
  SendStanza(response_stanza.get());

  // Inform listeners that a notification has been received.
  SignalUpdateAvailable();
  return STATE_RESPONSE;
}

bool ListenTask::HandleStanza(const buzz::XmlElement* stanza) {
  LOG(INFO) << "P2P: Stanza received: " << XmlElementToString(*stanza);
  // TODO(akalin): Do more verification on stanza depending on
  // notification_method_.
  if (IsValidNotification(stanza)) {
    QueueStanza(stanza);
    return true;
  }
  return false;
}

bool ListenTask::IsValidNotification(const buzz::XmlElement* stanza) {
  static const buzz::QName kQnNotifierGetAll(
      true, kNotifierNamespace, "getAll");
  // An update notificaiton has the following form.
  //  <cli:iq from="{bare_jid}" to="{full_jid}"
  //      id="#" type="set" xmlns:cli="jabber:client">
  //    <not:getAll xmlns:not="google:notifier">
  //      <Timestamp long="#" xmlns=""/>
  //    </not:getAll>
  //  </cli:iq>
  return
      (MatchRequestIq(stanza, buzz::STR_SET, kQnNotifierGetAll) &&
       (stanza->Attr(buzz::QN_TO) == GetClient()->jid().Str()) &&
       (stanza->Attr(buzz::QN_FROM) == GetClient()->jid().BareJid().Str()));
}

}  // namespace browser_sync
