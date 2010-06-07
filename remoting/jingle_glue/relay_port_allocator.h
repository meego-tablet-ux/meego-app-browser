// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_JINGLE_GLUE_RELAY_PORT_ALLOCATOR_H_
#define REMOTING_JINGLE_GLUE_RELAY_PORT_ALLOCATOR_H_

#include <string>

#include "talk/base/sigslot.h"
#include "talk/p2p/client/httpportallocator.h"

namespace buzz {
class XmppClient;
}  // namespace buzz

namespace remoting {

class RelayPortAllocator: public cricket::HttpPortAllocator,
                          public sigslot::has_slots<> {
 public:
  RelayPortAllocator(talk_base::NetworkManager* network_manager,
                     const std::string& user_agent):
      cricket::HttpPortAllocator(network_manager, user_agent) { }

  void OnJingleInfo(const std::string& token,
                    const std::vector<std::string>& relay_hosts,
                    const std::vector<talk_base::SocketAddress>& stun_hosts);

  void SetJingleInfo(buzz::XmppClient* client);

 private:
  DISALLOW_COPY_AND_ASSIGN(RelayPortAllocator);
};

}  // namespace remoting

#endif // REMOTING_JINGLE_GLUE_RELAY_PORT_ALLOCATOR_H_
