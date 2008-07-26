// Copyright 2008, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef SANDBOX_SRC_CROSSCALL_SERVER_H__
#define SANDBOX_SRC_CROSSCALL_SERVER_H__

#include <string>
#include <vector>
#include "sandbox/src/crosscall_params.h"

// This is the IPC server interface for CrossCall: The  IPC for the Sandbox
// On the server, CrossCall needs two things:
// 1) threads: Or better said, someone to provide them, that is what the
//             ThreadProvider interface is defined for. These thread(s) are
//             the ones that will actually execute the  IPC data retrieval.
//
// 2) a dispatcher: This interface represents the way to route and process
//                  an  IPC call given the  IPC tag.
//
// The other class included here CrossCallParamsEx is the server side version
// of the CrossCallParams class of /sandbox/crosscall_params.h The difference
// is that the sever version is paranoid about the correctness of the IPC
// message and will do all sorts of verifications.
//
// A general diagram of the interaction is as follows:
//
//                                 ------------
//                                 |          |
//  ThreadProvider <--(1)Register--|  IPC     |
//      |                          | Implemen |
//      |                          | -tation  |
//     (2)                         |          |  OnMessage
//     IPC fired --callback ------>|          |--(3)---> Dispatcher
//                                 |          |
//                                 ------------
//
//  The  IPC implementation sits as a middleman between the handling of the
//  specifics of scheduling a thread to service the  IPC and the multiple
//  entities that can potentially serve each particular IPC.
namespace sandbox {

class InterceptionManager;

// This function signature is required as the callback when an  IPC call fires.
// context: a user-defined pointer that was set using  ThreadProvider
// reason: 0 if the callback was fired because of a timeout.
//         1 if the callback was fired because of an event.
typedef void (__stdcall * CrossCallIPCCallback)(void* context,
                                                unsigned char reason);

// ThreadProvider models a thread factory. The idea is to decouple thread
// creation and lifetime from the inner guts of the IPC. The contract is
// simple:
//   - the IPC implementation calls RegisterWait with a waitable object that
//     becomes signaled when an IPC arrives and needs to be serviced.
//   - when the waitable object becomes signaled, the thread provider conjures
//     a thread that calls the callback (CrossCallIPCCallback) function
//   - the callback function tries its best not to block and return quickly
//     and should not assume that the next callback will use the same thread
//   - when the callback returns the ThreadProvider owns again the thread
//     and can destroy it or keep it around.
class ThreadProvider {
 public:
  // Registers a waitable object with the thread provider.
  // client: A number to associate with all the RegisterWait calls, typically
  //         this is the address of the caller object. This parameter cannot
  //         be zero.
  // waitable_object : a kernel object that can be waited on
  // callback: a function pointer which is the function that will be called
  //           when the waitable object fires
  // context: a user-provider pointer that is passed back to the callback
  //          when its called
  virtual bool RegisterWait(const void* client, HANDLE waitable_object,
                            CrossCallIPCCallback callback,
                            void* context) = 0;

  // Removes all the registrations done with the same cookie parameter.
  // This frees internal thread pool resources.
  virtual bool UnRegisterWaits(void* cookie) = 0;
};

// Models the server-side of the original input parameters.
// Provides IPC buffer validation and it is capable of reading the parameters
// out of the IPC buffer.
class CrossCallParamsEx : public CrossCallParams {
 public:
  // Factory constructor. Pass an IPCbuffer (and buffer size) that contains a
  // pending IPCcall. This constructor will:
  // 1) validate the IPC buffer. returns NULL is the IPCbuffer is malformed.
  // 2) make a copy of the IPCbuffer (parameter capture)
  static CrossCallParamsEx* CreateFromBuffer(void* buffer_base,
                                             size_t buffer_size,
                                             size_t* output_size);

  // Provides IPCinput parameter raw access:
  // index : the parameter to read; 0 is the first parameter
  // returns NULL if the parameter is non-existent. If it exists it also
  // returns the size in *size
  void* GetRawParameter(size_t index, size_t* size, ArgType* type);

  // Gets a parameter that is four bytes in size.
  // Returns false if the parameter does not exist or is not 32 bits wide.
  bool GetParameter32(size_t index, void* param);

  // Gets a parameter that is a string. Returns false if the parameter does not
  // exist.
  bool GetParameterStr(size_t index, std::wstring* string);

  // Gets a parameter that is an in/out buffer. Returns false is the parameter
  // does not exist or if the size of the actual parameter is not equal to the
  // expected size.
  bool GetParameterPtr(size_t index, size_t expected_size, void** pointer);

  // Frees the memory associated with the IPC parameters.
  static void operator delete(void* raw_memory) throw();

 private:
  // Only the factory method CreateFromBuffer can construct these objects.
  CrossCallParamsEx();

  ParamInfo param_info_[1];
  DISALLOW_EVIL_CONSTRUCTORS(CrossCallParamsEx);
};

// Simple helper function that sets the members of CrossCallReturn
// to the proper state to signal a basic error.
void SetCallError(ResultCode error, CrossCallReturn* call_return);

// Sets the internal status of call_return to signify the that IPC call
// completed successfully.
void SetCallSuccess(CrossCallReturn* call_return);

// Represents the client process that initiated the IPC which boils down to the
// process handle and the job object handle that contains the client process.
struct ClientInfo {
  HANDLE process;
  HANDLE job_object;
  DWORD process_id;
};

// All IPC-related information to be passed to the IPC handler.
struct IPCInfo {
  int ipc_tag;
  const ClientInfo* client_info;
  CrossCallReturn return_info;
};

// This structure identifies IPC signatures.
struct IPCParams {
  int ipc_tag;
  ArgType args[kMaxIpcParams];

  bool Matches(IPCParams* other) const {
    return !memcmp(this, other, sizeof(*other));
  }
};

// Models an entity that can process an IPC message or it can route to another
// one that could handle it. When an IPC arrives the IPC implementation will:
// 1) call OnMessageReady() with the tag of the pending IPC. If the dispatcher
//    returns NULL it means that it cannot handle this IPC but if it returns
//    non-null, it must be the pointer to a dispatcher that can handle it.
// 2) When the  IPC finally obtains a valid Dispatcher the IPC
//    implementation creates a CrossCallParamsEx from the raw IPC buffer.
// 3) It calls the returned callback, with the IPC info and arguments.
class Dispatcher {
 public:
  // Called from the  IPC implementation to handle a specific IPC message.
  typedef bool (Dispatcher::*CallbackGeneric)();
  typedef bool (Dispatcher::*Callback0)(IPCInfo* ipc);
  typedef bool (Dispatcher::*Callback1)(IPCInfo* ipc, void* p1);
  typedef bool (Dispatcher::*Callback2)(IPCInfo* ipc, void* p1, void* p2);
  typedef bool (Dispatcher::*Callback3)(IPCInfo* ipc, void* p1, void* p2,
                                        void* p3);
  typedef bool (Dispatcher::*Callback4)(IPCInfo* ipc, void* p1, void* p2,
                                        void* p3, void* p4);
  typedef bool (Dispatcher::*Callback5)(IPCInfo* ipc, void* p1, void* p2,
                                        void* p3, void* p4, void* p5);
  typedef bool (Dispatcher::*Callback6)(IPCInfo* ipc, void* p1, void* p2,
                                        void* p3, void* p4, void* p5, void* p6);
  typedef bool (Dispatcher::*Callback7)(IPCInfo* ipc, void* p1, void* p2,
                                        void* p3, void* p4, void* p5, void* p6,
                                        void* p7);
  typedef bool (Dispatcher::*Callback8)(IPCInfo* ipc, void* p1, void* p2,
                                        void* p3, void* p4, void* p5, void* p6,
                                        void* p7, void* p8);
  typedef bool (Dispatcher::*Callback9)(IPCInfo* ipc, void* p1, void* p2,
                                        void* p3, void* p4, void* p5, void* p6,
                                        void* p7, void* p8, void* p9);

  // Called from the  IPC implementation when an  IPC message is ready override
  // on a derived class to handle a set of  IPC messages. Return NULL if your
  // subclass does not handle the message or return the pointer to the subclass
  // that can handle it.
  virtual Dispatcher* OnMessageReady(IPCParams* ipc, CallbackGeneric* callback);

  // Called when a target proces is created, to setup the interceptions related
  // with the given service (IPC).
  virtual bool SetupService(InterceptionManager* manager, int service) = 0;

  virtual ~Dispatcher() {}

 protected:
  // Structure that defines an IPC Call with all the parameters and the handler.
  struct IPCCall {
    IPCParams params;
    CallbackGeneric callback;
  };

  // List of IPC Calls supported by the class.
  std::vector<IPCCall> ipc_calls_;
};

}  // namespace sandbox

#endif  // SANDBOX_SRC_CROSSCALL_SERVER_H__
