// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TOOLS_FLIP_PROXY_CONFIG_H
#define NET_TOOLS_FLIP_PROXY_CONFIG_H
#pragma once

#include <arpa/inet.h>  // in_addr_t

#include <string>
#include <vector>

#include "base/logging.h"
#include "net/tools/flip_server/create_listener.h"

namespace net {

enum FlipHandlerType {
    FLIP_HANDLER_PROXY,
    FLIP_HANDLER_SPDY_SERVER,
    FLIP_HANDLER_HTTP_SERVER
};

class FlipAcceptor {
 public:
  enum FlipHandlerType flip_handler_type_;
  std::string listen_ip_;
  std::string listen_port_;
  std::string ssl_cert_filename_;
  std::string ssl_key_filename_;
  std::string http_server_ip_;
  std::string http_server_port_;
  std::string https_server_ip_;
  std::string https_server_port_;
  int spdy_only_;
  int accept_backlog_size_;
  bool disable_nagle_;
  int accepts_per_wake_;
  int listen_fd_;
  void* memory_cache_;
  int ssl_session_expiry_;
  bool ssl_disable_compression_;
  int idle_socket_timeout_s_;

  FlipAcceptor(enum FlipHandlerType flip_handler_type,
               std::string listen_ip,
               std::string listen_port,
               std::string ssl_cert_filename,
               std::string ssl_key_filename,
               std::string http_server_ip,
               std::string http_server_port,
               std::string https_server_ip,
               std::string https_server_port,
               int spdy_only,
               int accept_backlog_size,
               bool disable_nagle,
               int accepts_per_wake,
               bool reuseport,
               bool wait_for_iface,
               void *memory_cache)
      : flip_handler_type_(flip_handler_type),
        listen_ip_(listen_ip),
        listen_port_(listen_port),
        ssl_cert_filename_(ssl_cert_filename),
        ssl_key_filename_(ssl_key_filename),
        http_server_ip_(http_server_ip),
        http_server_port_(http_server_port),
        https_server_ip_(https_server_ip),
        https_server_port_(https_server_port),
        spdy_only_(spdy_only),
        accept_backlog_size_(accept_backlog_size),
        disable_nagle_(disable_nagle),
        accepts_per_wake_(accepts_per_wake),
        memory_cache_(memory_cache),
        ssl_session_expiry_(300),  // TODO(mbelshe):  Hook these up!
        ssl_disable_compression_(false),
        idle_socket_timeout_s_(300) {
    VLOG(1) << "Attempting to listen on " << listen_ip_.c_str() << ":"
            << listen_port_.c_str();
    if (!https_server_ip_.size())
      https_server_ip_ = http_server_ip_;
    if (!https_server_port_.size())
      https_server_port_ = http_server_port_;

    while (1) {
      int ret = CreateListeningSocket(listen_ip_,
                                      listen_port_,
                                      true,
                                      accept_backlog_size_,
                                      true,
                                      reuseport,
                                      wait_for_iface,
                                      disable_nagle_,
                                      &listen_fd_);
      if ( ret == 0 ) {
        break;
      } else if ( ret == -3 && wait_for_iface ) {
        // Binding error EADDRNOTAVAIL was encounted. We need
        // to wait for the interfaces to raised. try again.
        usleep(200000);
      } else {
        LOG(ERROR) << "Unable to create listening socket for: ret = " << ret
                   << ": " << listen_ip_.c_str() << ":"
                   << listen_port_.c_str();
        return;
      }
    }

    SetNonBlocking(listen_fd_);
    VLOG(1) << "Listening on socket: ";
    if (flip_handler_type == FLIP_HANDLER_PROXY)
      VLOG(1) << "\tType         : Proxy";
    else if (FLIP_HANDLER_SPDY_SERVER)
      VLOG(1) << "\tType         : SPDY Server";
    else if (FLIP_HANDLER_HTTP_SERVER)
      VLOG(1) << "\tType         : HTTP Server";
    VLOG(1) << "\tIP           : " << listen_ip_;
    VLOG(1) << "\tPort         : " << listen_port_;
    VLOG(1) << "\tHTTP Server  : " << http_server_ip_ << ":"
            << http_server_port_;
    VLOG(1) << "\tHTTPS Server : " << https_server_ip_ << ":"
            << https_server_port_;
    VLOG(1) << "\tSSL          : "
            << (ssl_cert_filename.size()?"true":"false");
    VLOG(1) << "\tCertificate  : " << ssl_cert_filename;
    VLOG(1) << "\tKey          : " << ssl_key_filename;
    VLOG(1) << "\tSpdy Only    : " << (spdy_only?"true":"flase");
  }
};

class FlipConfig {
 public:
  std::vector <FlipAcceptor*> acceptors_;
  double server_think_time_in_s_;
  enum logging::LoggingDestination log_destination_;
  std::string log_filename_;
  bool wait_for_iface_;
  int ssl_session_expiry_;
  bool ssl_disable_compression_;
  int idle_socket_timeout_s_;

  FlipConfig()
      : server_think_time_in_s_(0),
        log_destination_(logging::LOG_ONLY_TO_SYSTEM_DEBUG_LOG),
        wait_for_iface_(false) {}
  ~FlipConfig() {}

  void AddAcceptor(enum FlipHandlerType flip_handler_type,
                   std::string listen_ip,
                   std::string listen_port,
                   std::string ssl_cert_filename,
                   std::string ssl_key_filename,
                   std::string http_server_ip,
                   std::string http_server_port,
                   std::string https_server_ip,
                   std::string https_server_port,
                   int spdy_only,
                   int accept_backlog_size,
                   bool disable_nagle,
                   int accepts_per_wake,
                   bool reuseport,
                   bool wait_for_iface,
                   void *memory_cache) {
    // TODO(mbelshe): create a struct FlipConfigArgs{} for the arguments.
    acceptors_.push_back(new FlipAcceptor(flip_handler_type,
                                          listen_ip,
                                          listen_port,
                                          ssl_cert_filename,
                                          ssl_key_filename,
                                          http_server_ip,
                                          http_server_port,
                                          https_server_ip,
                                          https_server_port,
                                          spdy_only,
                                          accept_backlog_size,
                                          disable_nagle,
                                          accepts_per_wake,
                                          reuseport,
                                          wait_for_iface,
                                          memory_cache));
  }
};

}  // namespace

#endif  // NET_TOOLS_FLIP_PROXY_CONFIG_H

