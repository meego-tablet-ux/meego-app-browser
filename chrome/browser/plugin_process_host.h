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

#ifndef CHROME_BROWSER_PLUGIN_PROCESS_HOST_H__
#define CHROME_BROWSER_PLUGIN_PROCESS_HOST_H__

#include <vector>

#include "base/basictypes.h"
#include "base/id_map.h"
#include "base/message_loop.h"
#include "base/process.h"
#include "base/scoped_ptr.h"
#include "base/task.h"
#include "chrome/browser/resource_message_filter.h"
#include "chrome/common/ipc_channel_proxy.h"
#include "chrome/browser/resource_message_filter.h"

class PluginService;
class PluginProcessHost;
class ResourceDispatcherHost;
class URLRequestContext;
struct ViewHostMsg_Resource_Request;
class GURL;

// Represents the browser side of the browser <--> plugin communication
// channel.  Different plugins run in their own process, but multiple instances
// of the same plugin run in the same process.  There will be one
// PluginProcessHost per plugin process, matched with a corresponding
// PluginProcess running in the plugin process.  The browser is responsible for
// starting the plugin process when a plugin is created that doesn't already
// have a process.  After that, most of the communication is directly between
// the renderer and plugin processes.
class PluginProcessHost : public IPC::Channel::Listener,
                          public IPC::Message::Sender,
                          public MessageLoop::Watcher {
 public:
  PluginProcessHost(PluginService* plugin_service);
  ~PluginProcessHost();

  // Initialize the new plugin process, returning true on success. This must
  // be called before the object can be used. If dll is the ActiveX-shim, then
  // activex_clsid is the class id of ActiveX control, otherwise activex_clsid
  // is ignored.
  bool Init(const std::wstring& dll,
            const std::string& activex_clsid,
            const std::wstring& locale);

  // IPC::Message::Sender implementation:
  virtual bool Send(IPC::Message* msg);

  // MessageLoop watcher callback
  virtual void OnObjectSignaled(HANDLE object);

  // IPC::Channel::Listener implementation:
  virtual void OnMessageReceived(const IPC::Message& msg);
  virtual void OnChannelConnected(int32 peer_pid);
  virtual void OnChannelError();

  // Getter to the process, may return NULL if there is no connection.
  HANDLE process() { return process_.handle(); }

  // Tells the plugin process to create a new channel for communication with a
  // renderer.  When the plugin process responds with the channel name,
  // reply_msg is used to send the name to the renderer.
  void OpenChannelToPlugin(ResourceMessageFilter* renderer_message_filter,
                           const std::string& mime_type,
                           IPC::Message* reply_msg);

  const std::wstring& dll_path() const { return dll_path_; }

  // Sends the reply to an open channel request to the renderer with the given
  // channel name.
  static void ReplyToRenderer(ResourceMessageFilter* renderer_message_filter,
                              const std::wstring& channel,
                              const std::wstring& plugin_path,
                              IPC::Message* reply_msg);

  // This function is called on the IO thread once we receive a reply from the
  // modal HTML dialog (in the form of a JSON string). This function forwards
  // that reply back to the plugin that requested the dialog.
  void OnModalDialogResponse(const std::string& json_retval,
                             IPC::Message* sync_result);

  // Shuts down the current plugin process instance.
  void Shutdown();

 private:
  // Sends a message to the plugin process to request creation of a new channel
  // for the given mime type.
  void RequestPluginChannel(ResourceMessageFilter* renderer_message_filter,
                            const std::string& mime_type,
                            IPC::Message* reply_msg);
  // Message handlers.
  void OnChannelCreated(int process_id, const std::wstring& channel_name);
  void OnDownloadUrl(const std::string& url, int source_pid,
                     HWND caller_window);
  void OnGetPluginFinderUrl(std::string* plugin_finder_url);
  void OnRequestResource(const IPC::Message& message,
                         int request_id,
                         const ViewHostMsg_Resource_Request& request);
  void OnCancelRequest(int request_id);
  void OnDataReceivedACK(int request_id);
  void OnUploadProgressACK(int request_id);
  void OnSyncLoad(int request_id,
                  const ViewHostMsg_Resource_Request& request,
                  IPC::Message* sync_result);
  void OnGetCookies(uint32 request_context, const GURL& url,
                    std::string* cookies);

  void OnPluginShutdownRequest();
  void OnPluginMessage(const std::vector<uint8>& data);
  void OnGetPluginDataDir(std::wstring* retval);

  struct ChannelRequest {
    ChannelRequest(ResourceMessageFilter* renderer_message_filter,
                    const std::string& m, IPC::Message* r) :
        renderer_message_filter_(renderer_message_filter), mime_type(m),
        reply_msg(r) { }
    std::string mime_type;
    IPC::Message* reply_msg;
    scoped_refptr<ResourceMessageFilter> renderer_message_filter_;
  };

  // These are channel requests that we are waiting to send to the
  // plugin process once the channel is opened.
  std::vector<ChannelRequest> pending_requests_;

  // These are the channel requests that we have already sent to
  // the plugin process, but haven't heard back about yet.
  std::vector<ChannelRequest> sent_requests_;

  // The handle to our plugin process.
  Process process_;

  // true while we're waiting the channel to be opened.  In the meantime,
  // plugin instance requests will be buffered.
  bool opening_channel_;

  // The IPC::Channel.
  scoped_ptr<IPC::Channel> channel_;

  // IPC Channel's id.
  std::wstring channel_id_;

  // Path to the DLL of that plugin.
  std::wstring dll_path_;

  PluginService* plugin_service_;

  ResourceDispatcherHost* resource_dispatcher_host_;

  DISALLOW_EVIL_CONSTRUCTORS(PluginProcessHost);
};

#endif  // CHROME_BROWSER_PLUGIN_PROCESS_HOST_H__
