// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_TELNET_SERVER_H_
#define NET_BASE_TELNET_SERVER_H_

#include "net/base/listen_socket.h"

// Implements the telnet protocol on top of the raw socket interface.
// DidRead calls to the delegate are buffered on a line by line basis.
// (for now this means that basic line editing is handled in this object)
class TelnetServer : public ListenSocket {
public:
  static TelnetServer* Listen(std::string ip, int port,
                              ListenSocketDelegate *del);
  virtual ~TelnetServer();

protected:
  void Listen() { ListenSocket::Listen(); }
  virtual void Read();
  virtual void Accept();
  virtual void SendInternal(const char* bytes, int len);

private:
  enum TelnetInputState {
    NOT_IN_IAC_OR_ESC_SEQUENCE,  // Currently not processing any IAC or ESC sequence.
    EXPECTING_NEW_LINE,  // Received carriage return (CR) expecting new line (LF).
    EXPECTING_COMMAND,  // Processing IAC expecting command.
    EXPECTING_OPTION,  // Processing IAC expecting option.
    SUBNEGOTIATION_EXPECTING_IAC,  // Inside subnegoation IAC,SE will end it.
    SUBNEGOTIATION_EXPECTING_SE,  // Ending subnegoation expecting SE.
    EXPECTING_FIRST_ESC_CHARACTER,  // Processing ESC sequence.
    EXPECTING_SECOND_ESC_CHARACTER,  // Processing ESC sequence with two characters
    EXPECTING_NUMBER_SEMICOLON_OR_END  // Processing "ESC [" sequence.
  };

  TelnetServer(SOCKET s, ListenSocketDelegate* del);

  // telnet commands
  void SendIAC(int command, int option);
  void StateMachineStep(unsigned char c);

  TelnetInputState input_state_;
  int iac_command_;  // Last command read.
  int iac_option_;  // Last option read.
  std::string command_line_;

  DISALLOW_EVIL_CONSTRUCTORS(TelnetServer);
};

#endif // BASE_TELNET_SERVER_H_

