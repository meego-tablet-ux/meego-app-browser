// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_NOTIFIER_COMMUNICATOR_LOGIN_SETTINGS_H_
#define CHROME_BROWSER_SYNC_NOTIFIER_COMMUNICATOR_LOGIN_SETTINGS_H_
#include <string>

#include "chrome/browser/sync/notifier/communicator/xmpp_connection_generator.h"
#include "talk/base/scoped_ptr.h"

namespace buzz {
class XmppClientSettings;
}

namespace talk_base {
class FirewallManager;
class SocketAddress;
}

namespace notifier {
class ConnectionOptions;
struct ServerInformation;

class LoginSettings {
 public:
  LoginSettings(const buzz::XmppClientSettings& user_settings,
                const ConnectionOptions& options,
                std::string lang,
                ServerInformation* server_list,
                int server_count,
                talk_base::FirewallManager* firewall,
                bool no_gaia_auth,
                bool proxy_only);

  ~LoginSettings();

  // Note: firewall() may return NULL.
  //
  // Could be a const method, but it allows
  // modification of part (FirewallManager) of its state.
  talk_base::FirewallManager* firewall() {
    return firewall_;
  }

  bool no_gaia_auth() const {
    return no_gaia_auth_;
  }

  bool proxy_only() const {
    return proxy_only_;
  }

  const std::string& lang() const {
    return lang_;
  }

  const ServerInformation* server_list() const {
    return server_override_.get() ? server_override_.get() : server_list_.get();
  }

  int server_count() const {
    return server_override_.get() ? 1 : server_count_;
  }

  const buzz::XmppClientSettings& user_settings() const {
    return *user_settings_.get();
  }

  buzz::XmppClientSettings* modifiable_user_settings() {
    return user_settings_.get();
  }

  const ConnectionOptions& connection_options() const {
    return *connection_options_.get();
  }

  void set_server_override(const talk_base::SocketAddress& server);
  void clear_server_override();

 private:
  bool proxy_only_;
  bool no_gaia_auth_;
  talk_base::FirewallManager* firewall_;
  std::string lang_;

  talk_base::scoped_array<ServerInformation> server_list_;
  int server_count_;
  // Used to handle redirects
  scoped_ptr<ServerInformation> server_override_;

  scoped_ptr<buzz::XmppClientSettings> user_settings_;
  scoped_ptr<ConnectionOptions> connection_options_;
  DISALLOW_COPY_AND_ASSIGN(LoginSettings);
};
}  // namespace notifier
#endif  // CHROME_BROWSER_SYNC_NOTIFIER_COMMUNICATOR_LOGIN_SETTINGS_H_
