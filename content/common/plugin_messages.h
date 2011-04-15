// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Multiply-included message file, hence no include guard.

#include "build/build_config.h"
#include "content/common/common_param_traits.h"
#include "content/common/webkit_param_traits.h"
#include "ipc/ipc_channel_handle.h"
#include "ipc/ipc_message_macros.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/rect.h"
#include "webkit/glue/webcursor.h"

#if defined(OS_POSIX)
#include "base/file_descriptor_posix.h"
#endif

#define IPC_MESSAGE_START PluginMsgStart

IPC_STRUCT_BEGIN(PluginMsg_Init_Params)
  IPC_STRUCT_MEMBER(gfx::NativeViewId, containing_window)
  IPC_STRUCT_MEMBER(GURL,  url)
  IPC_STRUCT_MEMBER(GURL,  page_url)
  IPC_STRUCT_MEMBER(std::vector<std::string>, arg_names)
  IPC_STRUCT_MEMBER(std::vector<std::string>, arg_values)
  IPC_STRUCT_MEMBER(bool, load_manually)
  IPC_STRUCT_MEMBER(int, host_render_view_routing_id)
IPC_STRUCT_END()

IPC_STRUCT_BEGIN(PluginHostMsg_URLRequest_Params)
  IPC_STRUCT_MEMBER(std::string, url)
  IPC_STRUCT_MEMBER(std::string, method)
  IPC_STRUCT_MEMBER(std::string, target)
  IPC_STRUCT_MEMBER(std::vector<char>, buffer)
  IPC_STRUCT_MEMBER(int, notify_id)
  IPC_STRUCT_MEMBER(bool, popups_allowed)
  IPC_STRUCT_MEMBER(bool, notify_redirects)
IPC_STRUCT_END()

IPC_STRUCT_BEGIN(PluginMsg_DidReceiveResponseParams)
  IPC_STRUCT_MEMBER(unsigned long, id)
  IPC_STRUCT_MEMBER(std::string, mime_type)
  IPC_STRUCT_MEMBER(std::string, headers)
  IPC_STRUCT_MEMBER(uint32, expected_length)
  IPC_STRUCT_MEMBER(uint32, last_modified)
  IPC_STRUCT_MEMBER(bool, request_is_seekable)
IPC_STRUCT_END()

IPC_STRUCT_BEGIN(PluginMsg_UpdateGeometry_Param)
  IPC_STRUCT_MEMBER(gfx::Rect, window_rect)
  IPC_STRUCT_MEMBER(gfx::Rect, clip_rect)
  IPC_STRUCT_MEMBER(bool, transparent)
  IPC_STRUCT_MEMBER(TransportDIB::Handle, windowless_buffer)
  IPC_STRUCT_MEMBER(TransportDIB::Handle, background_buffer)

#if defined(OS_MACOSX)
  // This field contains a key that the plug-in process is expected to return
  // to the renderer in its ACK message, unless the value is -1, in which case
  // no ACK message is required.  Other than the special -1 value, the values
  // used in ack_key are opaque to the plug-in process.
  IPC_STRUCT_MEMBER(int, ack_key)
#endif
IPC_STRUCT_END()

//-----------------------------------------------------------------------------
// PluginProcess messages
// These are messages sent from the browser to the plugin process.
// Tells the plugin process to create a new channel for communication with a
// given renderer.  The channel name is returned in a
// PluginProcessHostMsg_ChannelCreated message.  The renderer ID is passed so
// that the plugin process reuses an existing channel to that process if it
// exists. This ID is a unique opaque identifier generated by the browser
// process.
IPC_MESSAGE_CONTROL2(PluginProcessMsg_CreateChannel,
                     int /* renderer_id */,
                     bool /* off_the_record */)

// Tells the plugin process to notify every connected renderer of the pending
// shutdown, so we don't mistake it for a crash.
IPC_MESSAGE_CONTROL0(PluginProcessMsg_NotifyRenderersOfPendingShutdown)


//-----------------------------------------------------------------------------
// PluginProcessHost messages
// These are messages sent from the plugin process to the browser process.
// Response to a PluginProcessMsg_CreateChannel message.
IPC_MESSAGE_CONTROL1(PluginProcessHostMsg_ChannelCreated,
                     IPC::ChannelHandle /* channel_handle */)

IPC_SYNC_MESSAGE_CONTROL0_1(PluginProcessHostMsg_GetPluginFinderUrl,
                            std::string /* plugin finder URL */)

#if defined(OS_WIN)
// Destroys the given window's parent on the UI thread.
IPC_MESSAGE_CONTROL2(PluginProcessHostMsg_PluginWindowDestroyed,
                     HWND /* window */,
                     HWND /* parent */)

IPC_MESSAGE_ROUTED3(PluginProcessHostMsg_DownloadUrl,
                    std::string /* URL */,
                    int /* process id */,
                    HWND /* caller window */)
#endif

#if defined(USE_X11)
// On X11, the mapping between NativeViewId and X window ids
// is known only to the browser.  This message lets the plugin process
// ask about a NativeViewId that was provided by the renderer.
// It will get 0 back if it's a bogus input.
IPC_SYNC_MESSAGE_CONTROL1_1(PluginProcessHostMsg_MapNativeViewId,
                           gfx::NativeViewId /* input: native view id */,
                           gfx::PluginWindowHandle /* output: X window id */)
#endif

#if defined(OS_MACOSX)
// On Mac OS X, we need the browser to keep track of plugin windows so
// that it can add and remove them from stacking groups, hide and show the
// menu bar, etc.  We pass the window rect for convenience so that the
// browser can easily tell if the window is fullscreen.

// Notifies the browser that the plugin has selected a window (i.e., brought
// it to the front and wants it to have keyboard focus).
IPC_MESSAGE_CONTROL3(PluginProcessHostMsg_PluginSelectWindow,
                     uint32 /* window ID */,
                     gfx::Rect /* window rect */,
                     bool /* modal */)

// Notifies the browser that the plugin has shown a window.
IPC_MESSAGE_CONTROL3(PluginProcessHostMsg_PluginShowWindow,
                     uint32 /* window ID */,
                     gfx::Rect /* window rect */,
                     bool /* modal */)

// Notifies the browser that the plugin has hidden a window.
IPC_MESSAGE_CONTROL2(PluginProcessHostMsg_PluginHideWindow,
                     uint32 /* window ID */,
                     gfx::Rect /* window rect */)

// Notifies the browser that a plugin instance has requested a cursor
// visibility change.
IPC_MESSAGE_CONTROL1(PluginProcessHostMsg_PluginSetCursorVisibility,
                     bool /* cursor visibility */)
#endif


//-----------------------------------------------------------------------------
// Plugin messages
// These are messages sent from the renderer process to the plugin process.
// Tells the plugin process to create a new plugin instance with the given
// id.  A corresponding WebPluginDelegateStub is created which hosts the
// WebPluginDelegateImpl.
IPC_SYNC_MESSAGE_CONTROL1_1(PluginMsg_CreateInstance,
                            std::string /* mime_type */,
                            int /* instance_id */)

// The WebPluginDelegateProxy sends this to the WebPluginDelegateStub in its
// destructor, so that the stub deletes the actual WebPluginDelegateImpl
// object that it's hosting.
IPC_SYNC_MESSAGE_CONTROL1_0(PluginMsg_DestroyInstance,
                            int /* instance_id */)

IPC_SYNC_MESSAGE_CONTROL0_1(PluginMsg_GenerateRouteID,
                           int /* id */)

// The messages below all map to WebPluginDelegate methods.
IPC_SYNC_MESSAGE_ROUTED1_1(PluginMsg_Init,
                           PluginMsg_Init_Params,
                           bool /* result */)

// Used to synchronously request a paint for windowless plugins.
IPC_SYNC_MESSAGE_ROUTED1_0(PluginMsg_Paint,
                           gfx::Rect /* damaged_rect */)

// Sent by the renderer after it paints from its backing store so that the
// plugin knows it can send more invalidates.
IPC_MESSAGE_ROUTED0(PluginMsg_DidPaint)

IPC_SYNC_MESSAGE_ROUTED0_1(PluginMsg_GetPluginScriptableObject,
                           int /* route_id */)

IPC_MESSAGE_ROUTED3(PluginMsg_DidFinishLoadWithReason,
                    GURL /* url */,
                    int /* reason */,
                    int /* notify_id */)

// Updates the plugin location.
IPC_MESSAGE_ROUTED1(PluginMsg_UpdateGeometry,
                    PluginMsg_UpdateGeometry_Param)

// A synchronous version of above.
IPC_SYNC_MESSAGE_ROUTED1_0(PluginMsg_UpdateGeometrySync,
                           PluginMsg_UpdateGeometry_Param)

IPC_SYNC_MESSAGE_ROUTED1_0(PluginMsg_SetFocus,
                           bool /* focused */)

IPC_SYNC_MESSAGE_ROUTED1_2(PluginMsg_HandleInputEvent,
                           IPC::WebInputEventPointer /* event */,
                           bool /* handled */,
                           WebCursor /* cursor type*/)

IPC_MESSAGE_ROUTED1(PluginMsg_SetContentAreaFocus,
                    bool /* has_focus */)

#if defined(OS_MACOSX)
IPC_MESSAGE_ROUTED1(PluginMsg_SetWindowFocus,
                    bool /* has_focus */)

IPC_MESSAGE_ROUTED0(PluginMsg_ContainerHidden)

IPC_MESSAGE_ROUTED3(PluginMsg_ContainerShown,
                    gfx::Rect /* window_frame */,
                    gfx::Rect /* view_frame */,
                    bool /* has_focus */)

IPC_MESSAGE_ROUTED2(PluginMsg_WindowFrameChanged,
                    gfx::Rect /* window_frame */,
                    gfx::Rect /* view_frame */)

IPC_MESSAGE_ROUTED1(PluginMsg_ImeCompositionCompleted,
                    string16 /* text */)
#endif

IPC_SYNC_MESSAGE_ROUTED3_0(PluginMsg_WillSendRequest,
                           unsigned long /* id */,
                           GURL /* url */,
                           int  /* http_status_code */)

IPC_MESSAGE_ROUTED1(PluginMsg_DidReceiveResponse,
                    PluginMsg_DidReceiveResponseParams)

IPC_MESSAGE_ROUTED3(PluginMsg_DidReceiveData,
                    unsigned long /* id */,
                    std::vector<char> /* buffer */,
                    int /* data_offset */)

IPC_MESSAGE_ROUTED1(PluginMsg_DidFinishLoading,
                    unsigned long /* id */)

IPC_MESSAGE_ROUTED1(PluginMsg_DidFail,
                    unsigned long /* id */)

IPC_MESSAGE_ROUTED4(PluginMsg_SendJavaScriptStream,
                    GURL /* url */,
                    std::string /* result */,
                    bool /* success */,
                    int /* notify_id */)

IPC_MESSAGE_ROUTED2(PluginMsg_DidReceiveManualResponse,
                    GURL /* url */,
                    PluginMsg_DidReceiveResponseParams)

IPC_MESSAGE_ROUTED1(PluginMsg_DidReceiveManualData,
                    std::vector<char> /* buffer */)

IPC_MESSAGE_ROUTED0(PluginMsg_DidFinishManualLoading)

IPC_MESSAGE_ROUTED0(PluginMsg_DidManualLoadFail)

IPC_MESSAGE_ROUTED0(PluginMsg_InstallMissingPlugin)

IPC_MESSAGE_ROUTED3(PluginMsg_HandleURLRequestReply,
                    unsigned long /* resource_id */,
                    GURL /* url */,
                    int /* notify_id */)

IPC_MESSAGE_ROUTED2(PluginMsg_HTTPRangeRequestReply,
                    unsigned long /* resource_id */,
                    int /* range_request_id */)

IPC_MESSAGE_CONTROL1(PluginMsg_SignalModalDialogEvent,
                     gfx::NativeViewId /* containing_window */)

IPC_MESSAGE_CONTROL1(PluginMsg_ResetModalDialogEvent,
                     gfx::NativeViewId /* containing_window */)

#if defined(OS_MACOSX)
// This message, used only on 10.6 and later, transmits the "fake"
// window handle allocated by the browser on behalf of the renderer
// to the GPU plugin.
IPC_MESSAGE_ROUTED1(PluginMsg_SetFakeAcceleratedSurfaceWindowHandle,
                    gfx::PluginWindowHandle /* window */)
#endif

IPC_MESSAGE_CONTROL3(PluginMsg_ClearSiteData,
                     std::string, /* site */
                     uint64, /* flags */
                     base::Time /* begin_time */)


//-----------------------------------------------------------------------------
// PluginHost messages
// These are messages sent from the plugin process to the renderer process.
// They all map to the corresponding WebPlugin methods.
// Sends the plugin window information to the renderer.
// The window parameter is a handle to the window if the plugin is a windowed
// plugin. It is NULL for windowless plugins.
IPC_SYNC_MESSAGE_ROUTED1_0(PluginHostMsg_SetWindow,
                           gfx::PluginWindowHandle /* window */)

#if defined(OS_WIN)
// The modal_loop_pump_messages_event parameter is an event handle which is
// passed in for windowless plugins and is used to indicate if messages
// are to be pumped in sync calls to the plugin process. Currently used
// in HandleEvent calls.
IPC_SYNC_MESSAGE_ROUTED1_0(PluginHostMsg_SetWindowlessPumpEvent,
                           HANDLE /* modal_loop_pump_messages_event */)
#endif

IPC_MESSAGE_ROUTED1(PluginHostMsg_URLRequest,
                    PluginHostMsg_URLRequest_Params)

IPC_MESSAGE_ROUTED1(PluginHostMsg_CancelResource,
                    int /* id */)

IPC_MESSAGE_ROUTED1(PluginHostMsg_InvalidateRect,
                    gfx::Rect /* rect */)

IPC_SYNC_MESSAGE_ROUTED1_1(PluginHostMsg_GetWindowScriptNPObject,
                           int /* route id */,
                           bool /* success */)

IPC_SYNC_MESSAGE_ROUTED1_1(PluginHostMsg_GetPluginElement,
                           int /* route id */,
                           bool /* success */)

IPC_MESSAGE_ROUTED3(PluginHostMsg_SetCookie,
                    GURL /* url */,
                    GURL /* first_party_for_cookies */,
                    std::string /* cookie */)

IPC_SYNC_MESSAGE_ROUTED2_1(PluginHostMsg_GetCookies,
                           GURL /* url */,
                           GURL /* first_party_for_cookies */,
                           std::string /* cookies */)

IPC_MESSAGE_ROUTED1(PluginHostMsg_MissingPluginStatus,
                    int /* status */)

IPC_MESSAGE_ROUTED0(PluginHostMsg_CancelDocumentLoad)

IPC_MESSAGE_ROUTED3(PluginHostMsg_InitiateHTTPRangeRequest,
                    std::string /* url */,
                    std::string /* range_info */,
                    int         /* range_request_id */)

IPC_MESSAGE_ROUTED2(PluginHostMsg_DeferResourceLoading,
                    unsigned long /* resource_id */,
                    bool /* defer */)

IPC_SYNC_MESSAGE_CONTROL1_0(PluginHostMsg_SetException,
                            std::string /* message */)

IPC_MESSAGE_CONTROL0(PluginHostMsg_PluginShuttingDown)

#if defined(OS_MACOSX)
IPC_MESSAGE_ROUTED1(PluginHostMsg_UpdateGeometry_ACK,
                    int /* ack_key */)

IPC_MESSAGE_ROUTED1(PluginHostMsg_FocusChanged,
                    bool /* focused */)

IPC_MESSAGE_ROUTED0(PluginHostMsg_StartIme)

// This message, used in Mac OS X 10.5 and earlier, is sent from the plug-in
// process to the renderer process to indicate that the plug-in allocated a
// new TransportDIB that holds the GPU's rendered image.  This information is
// then forwarded to the browser process via a similar message.
IPC_MESSAGE_ROUTED4(PluginHostMsg_AcceleratedSurfaceSetTransportDIB,
                    gfx::PluginWindowHandle /* window */,
                    int32 /* width */,
                    int32 /* height */,
                    TransportDIB::Handle /* handle to the TransportDIB */)

// Synthesize a fake window handle for the plug-in to identify the instance
// to the browser, allowing mapping to a surface for hardware accelleration
// of plug-in content. The browser generates the handle which is then set on
// the plug-in. |opaque| indicates whether the content should be treated as
// opaque.
IPC_MESSAGE_ROUTED1(PluginHostMsg_BindFakePluginWindowHandle,
                    bool /* opaque */)

// This message, used only on 10.6 and later, is sent from the plug-in process
// to the renderer process to indicate that the plugin allocated a new
// IOSurface object of the given width and height. This information is then
// forwarded on to the browser process.
//
// NOTE: the original intent was to pass a mach port as the IOSurface
// identifier but it looks like that will be a lot of work. For now we pass an
// ID from IOSurfaceGetID.
IPC_MESSAGE_ROUTED4(PluginHostMsg_AcceleratedSurfaceSetIOSurface,
                    gfx::PluginWindowHandle /* window */,
                    int32 /* width */,
                    int32 /* height */,
                    uint64 /* surface_id */)


// On the Mac, shared memory can't be allocated in the sandbox, so
// the TransportDIB used by the plug-in for rendering has to be allocated
// and managed by the browser.  This is a synchronous message, use with care.
IPC_SYNC_MESSAGE_ROUTED1_1(PluginHostMsg_AllocTransportDIB,
                           size_t /* requested memory size */,
                           TransportDIB::Handle /* output: DIB handle */)

// Since the browser keeps handles to the allocated transport DIBs, this
// message is sent to tell the browser that it may release them when the
// renderer is finished with them.
IPC_MESSAGE_ROUTED1(PluginHostMsg_FreeTransportDIB,
                    TransportDIB::Id /* DIB id */)

// This message notifies the renderer process (and from there the
// browser process) that the plug-in swapped the buffers associated
// with the given "window", which should cause the browser to redraw
// the various plug-ins' contents.
IPC_MESSAGE_ROUTED2(PluginHostMsg_AcceleratedSurfaceBuffersSwapped,
                    gfx::PluginWindowHandle /* window */,
                    uint64 /* surface_id */)
#endif

IPC_MESSAGE_CONTROL1(PluginHostMsg_ClearSiteDataResult,
                     bool /* success */)

IPC_MESSAGE_ROUTED2(PluginHostMsg_URLRedirectResponse,
                    bool /* allow */,
                    int  /* resource_id */)


//-----------------------------------------------------------------------------
// NPObject messages
// These are messages used to marshall NPObjects.  They are sent both from the
// plugin to the renderer and from the renderer to the plugin.
IPC_SYNC_MESSAGE_ROUTED0_0(NPObjectMsg_Release)

IPC_SYNC_MESSAGE_ROUTED1_1(NPObjectMsg_HasMethod,
                           NPIdentifier_Param /* name */,
                           bool /* result */)

IPC_SYNC_MESSAGE_ROUTED3_2(NPObjectMsg_Invoke,
                           bool /* is_default */,
                           NPIdentifier_Param /* method */,
                           std::vector<NPVariant_Param> /* args */,
                           NPVariant_Param /* result_param */,
                           bool /* result */)

IPC_SYNC_MESSAGE_ROUTED1_1(NPObjectMsg_HasProperty,
                           NPIdentifier_Param /* name */,
                           bool /* result */)

IPC_SYNC_MESSAGE_ROUTED1_2(NPObjectMsg_GetProperty,
                           NPIdentifier_Param /* name */,
                           NPVariant_Param /* property */,
                           bool /* result */)

IPC_SYNC_MESSAGE_ROUTED2_1(NPObjectMsg_SetProperty,
                           NPIdentifier_Param /* name */,
                           NPVariant_Param /* property */,
                           bool /* result */)

IPC_SYNC_MESSAGE_ROUTED1_1(NPObjectMsg_RemoveProperty,
                           NPIdentifier_Param /* name */,
                           bool /* result */)

IPC_SYNC_MESSAGE_ROUTED0_0(NPObjectMsg_Invalidate)

IPC_SYNC_MESSAGE_ROUTED0_2(NPObjectMsg_Enumeration,
                           std::vector<NPIdentifier_Param> /* value */,
                           bool /* result */)

IPC_SYNC_MESSAGE_ROUTED1_2(NPObjectMsg_Construct,
                           std::vector<NPVariant_Param> /* args */,
                           NPVariant_Param /* result_param */,
                           bool /* result */)

IPC_SYNC_MESSAGE_ROUTED2_2(NPObjectMsg_Evaluate,
                           std::string /* script */,
                           bool /* popups_allowed */,
                           NPVariant_Param /* result_param */,
                           bool /* result */)
