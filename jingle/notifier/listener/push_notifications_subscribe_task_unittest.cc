// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "jingle/notifier/listener/push_notifications_subscribe_task.h"

#include "base/scoped_ptr.h"
#include "base/string_util.h"
#include "jingle/notifier/listener/xml_element_util.h"
#include "talk/xmpp/jid.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace buzz {
class XmlElement;
}

namespace notifier {

class PushNotificationsSubscribeTaskTest : public testing::Test {
 public:
  PushNotificationsSubscribeTaskTest()
      : jid_("to@jid.com/test123"), task_id_("taskid") {
    EXPECT_NE(jid_.Str(), jid_.BareJid().Str());
  }

 protected:
  const buzz::Jid jid_;
  const std::string task_id_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PushNotificationsSubscribeTaskTest);
};

TEST_F(PushNotificationsSubscribeTaskTest, MakeSubscriptionMessage) {
  std::vector<PushNotificationsSubscribeTask::PushSubscriptionInfo>
      subscription_list;

  PushNotificationsSubscribeTask::PushSubscriptionInfo subscription;
  subscription.channel = "test_channel1";
  subscription.from = "from.test.com";
  subscription_list.push_back(subscription);
  subscription.channel = "test_channel2";
  subscription.from = "from.test2.com";
  subscription_list.push_back(subscription);
  scoped_ptr<buzz::XmlElement> message(
      PushNotificationsSubscribeTask::MakeSubscriptionMessage(
          subscription_list, jid_, task_id_));
  std::string expected_xml_string =
      StringPrintf(
          "<cli:iq type=\"set\" to=\"%s\" id=\"%s\" "
                  "xmlns:cli=\"jabber:client\">"
            "<subscribe xmlns=\"google:push\">"
              "<item channel=\"test_channel1\" from=\"from.test.com\"/>"
              "<item channel=\"test_channel2\" from=\"from.test2.com\"/>"
            "</subscribe>"
          "</cli:iq>",
          jid_.BareJid().Str().c_str(), task_id_.c_str());

  EXPECT_EQ(expected_xml_string, XmlElementToString(*message));
}

}  // namespace notifier

