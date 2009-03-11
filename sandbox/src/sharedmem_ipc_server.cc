// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "sandbox/src/sharedmem_ipc_server.h"
#include "sandbox/src/sharedmem_ipc_client.h"
#include "sandbox/src/sandbox.h"
#include "sandbox/src/sandbox_types.h"
#include "sandbox/src/crosscall_params.h"
#include "sandbox/src/crosscall_server.h"

namespace sandbox {

SharedMemIPCServer::SharedMemIPCServer(HANDLE target_process,
                                       DWORD target_process_id,
                                       HANDLE target_job,
                                       ThreadProvider* thread_provider,
                                       Dispatcher* dispatcher)
    : client_control_(NULL),
      thread_provider_(thread_provider),
      target_process_(target_process),
      target_process_id_(target_process_id),
      target_job_object_(target_job),
      call_dispatcher_(dispatcher) {
}

SharedMemIPCServer::~SharedMemIPCServer() {
  // Free the wait handles associated with the thread pool.
  if (!thread_provider_->UnRegisterWaits(this)) {
    // Better to leak than to crash.
    return;
  }
  // Free the IPC signal events.
  ServerContexts::iterator it;
  for (it = server_contexts_.begin(); it != server_contexts_.end(); ++it) {
    ServerControl* context = (*it);
    ::CloseHandle(context->ping_event);
    ::CloseHandle(context->pong_event);
    delete context;
  }
}

bool SharedMemIPCServer::Init(void* shared_mem, size_t shared_size,
                              size_t channel_size) {
  // The shared memory needs to be at least as big as a channel.
  if (shared_size < channel_size) {
    return false;
  }
  // The channel size should be aligned.
  if (0 != (channel_size % 32)) {
    return false;
  }

  // Calculate how many channels we can fit in the shared memory.
  shared_size -= offsetof(IPCControl, channels);
  size_t channel_count = shared_size / (sizeof(ChannelControl) + channel_size);

  // If we cannot fit even one channel we bail out.
  if (0 == channel_count) {
    return false;
  }
  // Calculate the start of the first channel.
  size_t base_start = (sizeof(ChannelControl)* channel_count) +
                       offsetof(IPCControl, channels);

  client_control_ = reinterpret_cast<IPCControl*>(shared_mem);
  client_control_->channels_count = 0;

  // This is the initialization that we do per-channel. Basically:
  // 1) make two events (ping & pong)
  // 2) create handles to the events for the client and the server.
  // 3) initialize the channel (client_context) with the state.
  // 4) initialize the server side of the channel (service_context).
  // 5) call the thread provider RegisterWait to register the ping events.
  for (size_t ix = 0; ix != channel_count; ++ix) {
    ChannelControl* client_context = &client_control_->channels[ix];
    ServerControl* service_context = new ServerControl;
    server_contexts_.push_back(service_context);

    if (!MakeEvents(&service_context->ping_event,
                    &service_context->pong_event,
                    &client_context->ping_event,
                    &client_context->pong_event)) {
      return false;
    }

    client_context->channel_base = base_start;
    client_context->state = kFreeChannel;

    // Note that some of these values are available as members of this
    // object but we put them again into the service_context because we
    // will be called on a static method (ThreadPingEventReady)
    service_context->shared_base = reinterpret_cast<char*>(shared_mem);
    service_context->channel_size = channel_size;
    service_context->channel = client_context;
    service_context->channel_buffer = service_context->shared_base +
                                      client_context->channel_base;
    service_context->dispatcher = call_dispatcher_;
    service_context->target_info.process = target_process_;
    service_context->target_info.process_id = target_process_id_;
    service_context->target_info.job_object = target_job_object_;
    // Advance to the next channel.
    base_start += channel_size;
    // Register the ping event with the threadpool.
    thread_provider_->RegisterWait(this, service_context->ping_event,
                                   ThreadPingEventReady, service_context);
  }

  // We create a mutex that the server locks. If the server dies unexpectedly,
  // the thread that owns it will fail to release the lock and windows will
  // report to the target (when it tries to acquire it) that the wait was
  // abandoned. Note: We purposely leak the local handle because we want it to
  // be closed by Windows itself so it is properly marked as abandoned if the
  // server dies.
  if (!::DuplicateHandle(::GetCurrentProcess(),
                         ::CreateMutexW(NULL, TRUE, NULL),
                         target_process_, &client_control_->server_alive,
                         SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, 0)) {
    return false;
  }
  // This last setting indicates to the client all is setup.
  client_control_->channels_count = channel_count;
  return true;
}

// Releases memory allocated for IPC arguments, if needed.
void ReleaseArgs(const IPCParams* ipc_params, void* args[kMaxIpcParams]) {
  for (size_t i = 0; i < kMaxIpcParams; i++) {
    if (args[i]) {
      switch (ipc_params->args[i]) {
        case WCHAR_TYPE: {
          delete reinterpret_cast<std::wstring*>(args[i]);
          args[i] = NULL;
          break;
        }
        case INOUTPTR_TYPE: {
          delete reinterpret_cast<CountedBuffer*>(args[i]);
          args[i] = NULL;
          break;
        }
        default: break;
      }
    }
  }
}

// Fills up the list of arguments (args and ipc_params) for an IPC call.
bool GetArgs(CrossCallParamsEx* params, IPCParams* ipc_params,
             void* args[kMaxIpcParams]) {
  if (kMaxIpcParams < params->GetParamsCount())
    return false;

  for (size_t i = 0; i < params->GetParamsCount(); i++) {
    size_t size;
    ArgType type;
    args[i] = params->GetRawParameter(i, &size, &type);
    if (args[i]) {
      ipc_params->args[i] = type;
      switch (type) {
        case WCHAR_TYPE: {
          std::wstring* data = new std::wstring;
          if (!params->GetParameterStr(i, data)) {
            args[i] = 0;
            ReleaseArgs(ipc_params, args);
            return false;
          }
          args[i] = data;
          break;
        }
        case ULONG_TYPE: {
          ULONG data;
          if (!params->GetParameter32(i, &data)) {
            ReleaseArgs(ipc_params, args);
            return false;
          }
          args[i] = bit_cast<void*>(data);
          break;
        }
        case INOUTPTR_TYPE: {
          if (!args[i]) {
            ReleaseArgs(ipc_params, args);
            return false;
          }
          CountedBuffer* buffer = new CountedBuffer(args[i] , size);
          args[i] = buffer;
          break;
        }
        default: break;
      }
    }
  }
  return true;
}

bool SharedMemIPCServer::InvokeCallback(const ServerControl* service_context,
                                        void* ipc_buffer,
                                        CrossCallReturn* call_result) {
  // Set the default error code;
  SetCallError(SBOX_ERROR_INVALID_IPC, call_result);
  size_t output_size = 0;
  // Parse, verify and copy the message. The handler operates on a copy
  // of the message so the client cannot play dirty tricks by changing the
  // data in the channel while the IPC is being processed.
  scoped_ptr<CrossCallParamsEx> params(
      CrossCallParamsEx::CreateFromBuffer(ipc_buffer,
                                          service_context->channel_size,
                                          &output_size));
  if (!params.get())
    return false;

  uint32 tag = params->GetTag();
  COMPILE_ASSERT(0 == INVALID_TYPE, Incorrect_type_enum);
  IPCParams ipc_params = {0} ;
  ipc_params.ipc_tag = tag;

  void* args[kMaxIpcParams];
  if (!GetArgs(params.get(), &ipc_params, args))
    return false;

  IPCInfo ipc_info = {0};
  ipc_info.ipc_tag = tag;
  ipc_info.client_info = &service_context->target_info;
  Dispatcher* dispatcher = service_context->dispatcher;
  DCHECK(dispatcher);
  bool error = true;
  Dispatcher* handler = NULL;

  Dispatcher::CallbackGeneric callback_generic;
  handler = dispatcher->OnMessageReady(&ipc_params, &callback_generic);
  if (handler) {
    switch (params->GetParamsCount()) {
      case 0: {
        // Ask the IPC dispatcher if she can service this IPC.
        Dispatcher::Callback0 callback =
            reinterpret_cast<Dispatcher::Callback0>(callback_generic);
        if (!(handler->*callback)(&ipc_info))
          break;
        error = false;
        break;
      }
      case 1: {
        Dispatcher::Callback1 callback =
            reinterpret_cast<Dispatcher::Callback1>(callback_generic);
        if (!(handler->*callback)(&ipc_info, args[0]))
          break;
        error = false;
        break;
      }
      case 2: {
        Dispatcher::Callback2 callback =
            reinterpret_cast<Dispatcher::Callback2>(callback_generic);
        if (!(handler->*callback)(&ipc_info, args[0], args[1]))
          break;
        error = false;
        break;
      }
      case 3: {
        Dispatcher::Callback3 callback =
            reinterpret_cast<Dispatcher::Callback3>(callback_generic);
        if (!(handler->*callback)(&ipc_info, args[0], args[1], args[2]))
          break;
        error = false;
        break;
      }
      case 4: {
        Dispatcher::Callback4 callback =
            reinterpret_cast<Dispatcher::Callback4>(callback_generic);
        if (!(handler->*callback)(&ipc_info, args[0], args[1], args[2],
                                  args[3]))
          break;
        error = false;
        break;
      }
      case 5: {
        Dispatcher::Callback5 callback =
            reinterpret_cast<Dispatcher::Callback5>(callback_generic);
        if (!(handler->*callback)(&ipc_info, args[0], args[1], args[2], args[3],
                                  args[4]))
          break;
        error = false;
        break;
      }
      case 6: {
        Dispatcher::Callback6 callback =
            reinterpret_cast<Dispatcher::Callback6>(callback_generic);
        if (!(handler->*callback)(&ipc_info, args[0], args[1], args[2], args[3],
                                  args[4], args[5]))
          break;
        error = false;
        break;
      }
      case 7: {
        Dispatcher::Callback7 callback =
            reinterpret_cast<Dispatcher::Callback7>(callback_generic);
        if (!(handler->*callback)(&ipc_info, args[0], args[1], args[2], args[3],
                                  args[4], args[5], args[6]))
          break;
        error = false;
        break;
      }
      case 8: {
        Dispatcher::Callback8 callback =
            reinterpret_cast<Dispatcher::Callback8>(callback_generic);
        if (!(handler->*callback)(&ipc_info, args[0], args[1], args[2], args[3],
                                  args[4], args[5], args[6], args[7]))
          break;
        error = false;
        break;
      }
      case 9: {
        Dispatcher::Callback9 callback =
            reinterpret_cast<Dispatcher::Callback9>(callback_generic);
        if (!(handler->*callback)(&ipc_info, args[0], args[1], args[2], args[3],
                                  args[4], args[5], args[6], args[7], args[8]))
          break;
        error = false;
        break;
      }
      default:  {
        NOTREACHED();
        break;
      }
    }
  }

  if (error) {
    if (handler)
      SetCallError(SBOX_ERROR_FAILED_IPC, call_result);
  } else {
    memcpy(call_result, &ipc_info.return_info, sizeof(*call_result));
    SetCallSuccess(call_result);
    if (params->IsInOut()) {
      // Maybe the params got changed by the broker. We need to upadte the
      // memory section.
      memcpy(ipc_buffer, params.get(), output_size);
    }
  }

  ReleaseArgs(&ipc_params, args);

  return !error;
}

// This function gets called by a thread from the thread pool when a
// ping event fires. The context is the same as passed in the RegisterWait()
// call above.
void __stdcall SharedMemIPCServer::ThreadPingEventReady(void* context,
                                                        unsigned char) {
  if (NULL == context) {
    DCHECK(false);
    return;
  }
  ServerControl* service_context = reinterpret_cast<ServerControl*>(context);
  // Since the event fired, the channel *must* be busy. Change to kAckChannel
  // while we service it.
  LONG last_state =
    ::InterlockedCompareExchange(&service_context->channel->state,
                                 kAckChannel, kBusyChannel);
  if (kBusyChannel != last_state) {
    DCHECK(false);
    return;
  }

  // Prepare the result structure. At this point we will return some result
  // even if the IPC is invalid, malformed or has no handler.
  CrossCallReturn call_result = {0};
  void* buffer = service_context->channel_buffer;

  InvokeCallback(service_context, buffer, &call_result);

  // Copy the answer back into the channel and signal the pong event. This
  // should wake up the client so he can finish the the ipc cycle.
  CrossCallParams* call_params = reinterpret_cast<CrossCallParams*>(buffer);
  memcpy(call_params->GetCallReturn(), &call_result, sizeof(call_result));
  ::InterlockedExchange(&service_context->channel->state, kAckChannel);
  ::SetEvent(service_context->pong_event);
}

bool SharedMemIPCServer::MakeEvents(HANDLE* server_ping, HANDLE* server_pong,
                                    HANDLE* client_ping, HANDLE* client_pong) {
  // Note that the IPC client has no right to delete the events. That would
  // cause problems. The server *owns* the events.
  const DWORD kDesiredAccess = SYNCHRONIZE | EVENT_MODIFY_STATE ;

  // The events are auto reset, and start not signaled.
  *server_ping = ::CreateEventW(NULL, FALSE, FALSE, NULL);
  if (!::DuplicateHandle(::GetCurrentProcess(), *server_ping, target_process_,
                         client_ping, kDesiredAccess, FALSE, 0)) {
    return false;
  }
  *server_pong = ::CreateEventW(NULL, FALSE, FALSE, NULL);
  if (!::DuplicateHandle(::GetCurrentProcess(), *server_pong, target_process_,
                         client_pong, kDesiredAccess, FALSE, 0)) {
    return false;
  }
  return true;
}

}  // namespace sandbox
