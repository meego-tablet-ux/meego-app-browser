// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/network_change_notifier_netlink_linux.h"

#include <fcntl.h>
// socket.h is needed to define types for the linux kernel header netlink.h
// so it needs to come before netlink.h.
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <string.h>
#include <unistd.h>

#include "base/logging.h"

namespace {

// Return true on success, false on failure.
// Too small a function to bother putting in a library?
bool SetNonBlocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (-1 == flags)
    return false;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0 ? true : false;
}

}  // namespace

int InitializeNetlinkSocket() {
  int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if (sock < 0) {
    PLOG(ERROR) << "Error creating netlink socket";
    return -1;
  }

  if (!SetNonBlocking(sock)) {
    PLOG(ERROR) << "Failed to set netlink socket to non-blocking mode.";
    if (close(sock) != 0)
      PLOG(ERROR) << "Failed to close socket";
    return -1;
  }

  struct sockaddr_nl local_addr;
  memset(&local_addr, 0, sizeof(local_addr));
  local_addr.nl_family = AF_NETLINK;
  local_addr.nl_pid = getpid();
  local_addr.nl_groups = RTMGRP_IPV4_IFADDR | RTMGRP_NOTIFY;
  int ret = bind(sock, reinterpret_cast<struct sockaddr*>(&local_addr),
                 sizeof(local_addr));
  if (ret < 0) {
    PLOG(ERROR) << "Error binding netlink socket";
    if (close(sock) != 0)
      PLOG(ERROR) << "Failed to close socket";
    return -1;
  }

  return sock;
}

bool HandleNetlinkMessage(char* buf, size_t len) {
  const struct nlmsghdr* netlink_message_header =
      reinterpret_cast<struct nlmsghdr*>(buf);
  DCHECK(netlink_message_header);
  for (; NLMSG_OK(netlink_message_header, len);
       netlink_message_header = NLMSG_NEXT(netlink_message_header, len)) {
    int netlink_message_type = netlink_message_header->nlmsg_type;
    switch (netlink_message_type) {
      case NLMSG_DONE:
        NOTREACHED()
            << "This is a monitoring netlink socket.  It should never be done.";
        return false;
      case NLMSG_ERROR:
        LOG(ERROR) << "Unexpected netlink error.";
        return false;
      // During IP address changes, we will see all these messages.  Only fire
      // the notification when we get a new address or remove an address.  We
      // may still end up notifying observers more than strictly necessary, but
      // if the primary interface goes down and back up, then this is necessary.
      case RTM_NEWADDR:
      case RTM_DELADDR:
        return true;
      case RTM_NEWLINK:
      case RTM_DELLINK:
        return false;
      default:
        LOG(DFATAL) << "Received unexpected netlink message type: "
                    << netlink_message_type;
        return false;
    }
  }

  return false;
}
