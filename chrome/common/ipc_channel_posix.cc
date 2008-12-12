// Copyright (c) 2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/ipc_channel_posix.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stddef.h>

#include "base/logging.h"
#include "base/process_util.h"
#include "base/scoped_ptr.h"
#include "base/string_util.h"
#include "chrome/common/chrome_counters.h"
#include "chrome/common/ipc_message_utils.h"

#if defined(OS_LINUX)
#include <linux/un.h>
#elif defined(OS_MACOSX)
#include <sys/un.h>
#endif

namespace IPC {

//------------------------------------------------------------------------------
// TODO(playmobil): Only use FIFOs for debugging, for real work, use a
// socketpair.
namespace {

// The -1 is to take the NULL terminator into account.
#if defined(OS_LINUX)
const size_t kMaxPipeNameLength = UNIX_PATH_MAX - 1;
#elif defined(OS_MACOSX)
// OS X doesn't define UNIX_PATH_MAX
// Per the size specified for the sun_path structure of sockaddr_un in sys/un.h.
const size_t kMaxPipeNameLength = 104 - 1;
#endif

// Creates a Fifo with the specified name ready to listen on.
bool CreateServerFifo(const std::string &pipe_name, int* server_listen_fd) {
  DCHECK(server_listen_fd);
  DCHECK(pipe_name.length() > 0);
  DCHECK(pipe_name.length() < kMaxPipeNameLength);

  if (pipe_name.length() == 0 || pipe_name.length() > kMaxPipeNameLength) {
    return false;
  }

  // Create socket.
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    return false;
  }

  // Make socket non-blocking
  if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
    close(fd);
    return false;
  }

  // Delete any old FS instances.
  unlink(pipe_name.c_str());

  // Create unix_addr structure
  struct sockaddr_un unix_addr;
  memset(&unix_addr, 0, sizeof(unix_addr));
  unix_addr.sun_family = AF_UNIX;
  snprintf(unix_addr.sun_path, kMaxPipeNameLength + 1, "%s", pipe_name.c_str());
  size_t unix_addr_len = offsetof(struct sockaddr_un, sun_path) +
      strlen(unix_addr.sun_path) + 1;

  // Bind the socket.
  if (bind(fd, reinterpret_cast<const sockaddr*>(&unix_addr),
           unix_addr_len) != 0) {
    close(fd);
    return false;
  }

  // Start listening on the socket.
  const int listen_queue_length = 1;
  if (listen(fd, listen_queue_length) != 0) {
    close(fd);
    return false;
  }

  *server_listen_fd = fd;
  return true;
}

// Accept a connection on a fifo.
bool ServerAcceptFifoConnection(int server_listen_fd, int* server_socket) {
  DCHECK(server_socket);

  int accept_fd = accept(server_listen_fd, NULL, 0);
  if (accept_fd < 0)
    return false;

  *server_socket = accept_fd;
  return true;
}

bool ClientConnectToFifo(const std::string &pipe_name, int* client_socket) {
  DCHECK(client_socket);
  DCHECK(pipe_name.length() < kMaxPipeNameLength);

  // Create socket.
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    LOG(ERROR) << "fd is invalid";
    return false;
  }

  // Make socket non-blocking
  if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
    LOG(ERROR) << "fcnt failed";
    close(fd);
    return false;
  }

  // Create server side of socket.
  struct sockaddr_un  server_unix_addr;
  memset(&server_unix_addr, 0, sizeof(server_unix_addr));
  server_unix_addr.sun_family = AF_UNIX;
  snprintf(server_unix_addr.sun_path, kMaxPipeNameLength + 1, "%s",
           pipe_name.c_str());
  size_t server_unix_addr_len = offsetof(struct sockaddr_un, sun_path) +
      strlen(server_unix_addr.sun_path) + 1;

  int ret_val = -1;
  do {
    ret_val = connect(fd, reinterpret_cast<sockaddr*>(&server_unix_addr),
                      server_unix_addr_len);
  } while (ret_val == -1 && errno == EINTR);
  if (ret_val != 0) {
    close(fd);
    return false;
  }

  *client_socket = fd;
  return true;
}

}  // namespace
//------------------------------------------------------------------------------

Channel::ChannelImpl::ChannelImpl(const std::wstring& channel_id, Mode mode,
                                  Listener* listener)
    : mode_(mode),
    server_listen_connection_event_(new EventHolder()),
    read_event_(new EventHolder()),
    write_event_(new EventHolder()),
    message_send_bytes_written_(0),
    server_listen_pipe_(-1),
    pipe_(-1),
    listener_(listener),
    waiting_connect_(true),
    processing_incoming_(false),
    factory_(this) {
  if (!CreatePipe(channel_id, mode)) {
    // The pipe may have been closed already.
    LOG(WARNING) << "Unable to create pipe named \"" << channel_id <<
                    "\" in " << (mode == MODE_SERVER ? "server" : "client") <<
                    " mode error(" << strerror(errno) << ").";
  }
}

const std::wstring Channel::ChannelImpl::PipeName(
    const std::wstring& channel_id) const {
  std::wostringstream ss;
  // TODO(playmobil): This should live in the Chrome user data directory.
  // TODO(playmobil): Cleanup any stale fifos.
  ss << L"/var/tmp/chrome_" << channel_id;
  return ss.str();
}

bool Channel::ChannelImpl::CreatePipe(const std::wstring& channel_id,
                                      Mode mode) {
  DCHECK(server_listen_pipe_ == -1 && pipe_ == -1);

  // TODO(playmobil): Should we just change pipe_name to be a normal string
  // everywhere?
  pipe_name_ = WideToUTF8(PipeName(channel_id));

  if (mode == MODE_SERVER) {
    if (!CreateServerFifo(pipe_name_, &server_listen_pipe_)) {
      return false;
    }
  } else {
    if (!ClientConnectToFifo(pipe_name_, &pipe_)) {
      return false;
    }
    waiting_connect_ = false;
  }

  // Create the Hello message to be sent when Connect is called
  scoped_ptr<Message> msg(new Message(MSG_ROUTING_NONE,
                                      HELLO_MESSAGE_TYPE,
                                      IPC::Message::PRIORITY_NORMAL));
  if (!msg->WriteInt(base::GetCurrentProcId())) {
    Close();
    return false;
  }

  output_queue_.push(msg.release());
  return true;
}

bool Channel::ChannelImpl::Connect() {
  if (mode_ == MODE_SERVER) {
    if (server_listen_pipe_ == -1) {
      return false;
    }
    event *ev = &(server_listen_connection_event_->event);
    MessageLoopForIO::current()->WatchFileHandle(server_listen_pipe_,
                                                 EV_READ | EV_PERSIST,
                                                 ev,
                                                 this);
    server_listen_connection_event_->is_active = true;
  } else {
    if (pipe_ == -1) {
      return false;
    }
    MessageLoopForIO::current()->WatchFileHandle(pipe_,
                                                 EV_READ | EV_PERSIST,
                                                 &(read_event_->event),
                                                 this);
    read_event_->is_active = true;
    waiting_connect_ = false;
  }

  if (!waiting_connect_)
    return ProcessOutgoingMessages();
  return true;
}

bool Channel::ChannelImpl::ProcessIncomingMessages() {
  ssize_t bytes_read = 0;

  for (;;) {
    if (bytes_read == 0) {
      if (pipe_ == -1)
        return false;

      // Read from pipe.
      // recv() returns 0 if the connection has closed or EAGAIN if no data is
      // waiting on the pipe.
      do {
        bytes_read = read(pipe_, input_buf_, Channel::kReadBufferSize);
      } while (bytes_read == -1 && errno == EINTR);
      if (bytes_read < 0) {
        if (errno == EAGAIN) {
          return true;
        } else {
          LOG(ERROR) << "pipe error: " << strerror(errno);
          return false;
        }
      } else if (bytes_read == 0) {
        // The pipe has closed...
        Close();
        return true;
      }
    }
    DCHECK(bytes_read);

    // Process messages from input buffer.
    const char *p;
    const char *end;
    if (input_overflow_buf_.empty()) {
      p = input_buf_;
      end = p + bytes_read;
    } else {
      if (input_overflow_buf_.size() >
         static_cast<size_t>(kMaximumMessageSize - bytes_read)) {
        input_overflow_buf_.clear();
        LOG(ERROR) << "IPC message is too big";
        return false;
      }
      input_overflow_buf_.append(input_buf_, bytes_read);
      p = input_overflow_buf_.data();
      end = p + input_overflow_buf_.size();
    }

    while (p < end) {
      const char* message_tail = Message::FindNext(p, end);
      if (message_tail) {
        int len = static_cast<int>(message_tail - p);
        const Message m(p, len);
#ifdef IPC_MESSAGE_DEBUG_EXTRA
        DLOG(INFO) << "received message on channel @" << this <<
                      " with type " << m.type();
#endif
        if (m.routing_id() == MSG_ROUTING_NONE &&
            m.type() == HELLO_MESSAGE_TYPE) {
          // The Hello message contains only the process id.
          listener_->OnChannelConnected(MessageIterator(m).NextInt());
        } else {
          listener_->OnMessageReceived(m);
        }
        p = message_tail;
      } else {
        // Last message is partial.
        break;
      }
    }
    input_overflow_buf_.assign(p, end - p);

    bytes_read = 0;  // Get more data.
  }

  return true;
}

bool Channel::ChannelImpl::ProcessOutgoingMessages() {
  DCHECK(!waiting_connect_);  // Why are we trying to send messages if there's
                              // no connection?

  if (output_queue_.empty())
    return true;

  if (pipe_ == -1)
    return false;

  // If libevent was monitoring the socket for us (we blocked when trying to
  // write a message last time), then delete the underlying libevent structure.
  if (write_event_->is_active) {
    // TODO(playmobil): This calls event_del(), but we can probably
    // do with just calling event_add here.
    MessageLoopForIO::current()->UnwatchFileHandle(&(write_event_->event));
    write_event_->is_active = false;
  }

  // Write out all the messages we can till the write blocks or there are no
  // more outgoing messages.
  while (!output_queue_.empty()) {
    Message* msg = output_queue_.front();

    size_t amt_to_write = msg->size() - message_send_bytes_written_;
    const char *out_bytes = reinterpret_cast<const char*>(msg->data()) +
        message_send_bytes_written_;
    ssize_t bytes_written = -1;
    do {
      bytes_written = write(pipe_, out_bytes, amt_to_write);
    } while (bytes_written == -1 && errno == EINTR);

    if (bytes_written < 0) {
      LOG(ERROR) << "pipe error: " << strerror(errno);
      return false;
    }

    if (static_cast<size_t>(bytes_written) != amt_to_write) {
      message_send_bytes_written_ += bytes_written;

      // Tell libevent to call us back once things are unblocked.
      MessageLoopForIO::current()->WatchFileHandle(server_listen_pipe_,
                                                   EV_WRITE,
                                                   &(write_event_->event),
                                                   this);
      write_event_->is_active = true;

    } else {
      message_send_bytes_written_ = 0;

      // Message sent OK!
#ifdef IPC_MESSAGE_DEBUG_EXTRA
      DLOG(INFO) << "sent message @" << msg << " on channel @" << this <<
                    " with type " << msg->type();
#endif
      output_queue_.pop();
      delete msg;
    }
  }
  return true;
}

bool Channel::ChannelImpl::Send(Message* message) {
  chrome::Counters::ipc_send_counter().Increment();
#ifdef IPC_MESSAGE_DEBUG_EXTRA
  DLOG(INFO) << "sending message @" << message << " on channel @" << this
             << " with type " << message->type()
             << " (" << output_queue_.size() << " in queue)";
#endif

// TODO(playmobil): implement
//  #ifdef IPC_MESSAGE_LOG_ENABLED
//    Logging::current()->OnSendMessage(message, L"");
//  #endif

  output_queue_.push(message);
  if (!waiting_connect_) {
    if (!write_event_->is_active) {
      if (!ProcessOutgoingMessages())
        return false;
    }
  }

  return true;
}

// Called by libevent when we can read from th pipe without blocking.
void Channel::ChannelImpl::OnFileReadReady(int fd) {
  bool send_server_hello_msg = false;
  if (waiting_connect_ && mode_ == MODE_SERVER) {
    if (!ServerAcceptFifoConnection(server_listen_pipe_, &pipe_)) {
      Close();
    }

    // No need to watch the listening socket any longer since only one client
    // can connect.  So unregister with libevent.
    event *ev = &(server_listen_connection_event_->event);
    MessageLoopForIO::current()->UnwatchFileHandle(ev);
    server_listen_connection_event_->is_active = false;

    // Start watching our end of the socket.
    MessageLoopForIO::current()->WatchFileHandle(pipe_,
                                                 EV_READ | EV_PERSIST,
                                                 &(read_event_->event),
                                                 this);
    read_event_->is_active = true;
    waiting_connect_ = false;
    send_server_hello_msg = true;
  }

  if (!waiting_connect_ && fd == pipe_) {
    if (!ProcessIncomingMessages()) {
      Close();
      listener_->OnChannelError();
    }
  }

  // If we're a server and handshaking, then we want to make sure that we
  // only send our handshake message after we've processed the client's.
  // This gives us a chance to kill the client if the incoming handshake
  // is invalid.
  if (send_server_hello_msg) {
    ProcessOutgoingMessages();
  }
}

// Called by libevent when we can write to the pipe without blocking.
void Channel::ChannelImpl::OnFileWriteReady(int fd) {
  if (!ProcessOutgoingMessages()) {
    Close();
    listener_->OnChannelError();
  }
}

void Channel::ChannelImpl::Close() {
  // Close can be called multiple time, so we need to make sure we're
  // idempotent.

  // Unregister libevent for the listening socket and close it.
  if (server_listen_connection_event_ &&
      server_listen_connection_event_->is_active) {
    MessageLoopForIO::current()->UnwatchFileHandle(
          &(server_listen_connection_event_->event));
  }

  if (server_listen_pipe_ != -1) {
    close(server_listen_pipe_);
    server_listen_pipe_ = -1;
  }

  // Unregister libevent for the FIFO and close it.
  if (read_event_ && read_event_->is_active) {
    MessageLoopForIO::current()->UnwatchFileHandle(&(read_event_->event));
  }
  if (write_event_ && write_event_->is_active) {
    MessageLoopForIO::current()->UnwatchFileHandle(&(write_event_->event));
  }
  if (pipe_ != -1) {
    close(pipe_);
    pipe_ = -1;
  }

  delete server_listen_connection_event_;
  server_listen_connection_event_ = NULL;
  delete read_event_;
  read_event_ = NULL;
  delete write_event_;
  write_event_ = NULL;

  // Unlink the FIFO
  unlink(pipe_name_.c_str());

  while (!output_queue_.empty()) {
    Message* m = output_queue_.front();
    output_queue_.pop();
    delete m;
  }
}

//------------------------------------------------------------------------------
// Channel's methods simply call through to ChannelImpl.
Channel::Channel(const std::wstring& channel_id, Mode mode,
                 Listener* listener)
    : channel_impl_(new ChannelImpl(channel_id, mode, listener)) {
}

Channel::~Channel() {
  delete channel_impl_;
}

bool Channel::Connect() {
  return channel_impl_->Connect();
}

void Channel::Close() {
  channel_impl_->Close();
}

void Channel::set_listener(Listener* listener) {
  channel_impl_->set_listener(listener);
}

bool Channel::Send(Message* message) {
  return channel_impl_->Send(message);
}

}  // namespace IPC
