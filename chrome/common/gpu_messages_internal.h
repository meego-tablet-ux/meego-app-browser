// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This header is meant to be included in multiple passes, hence no traditional
// header guard. It is included by backing_store_messages_internal.h
// See ipc_message_macros.h for explanation of the macros and passes.

// This file needs to be included again, even though we're actually included
// from it via utility_messages.h.

#include <vector>

#include "base/shared_memory.h"
#include "chrome/common/gpu_video_common.h"
#include "ipc/ipc_message_macros.h"

namespace gfx {
class Size;
}

namespace IPC {
struct ChannelHandle;
}

class GPUInfo;

//------------------------------------------------------------------------------
// GPU Messages
// These are messages from the browser to the GPU process.
IPC_BEGIN_MESSAGES(Gpu)

  // Tells the GPU process to create a new channel for communication with a
  // given renderer.  The channel name is returned in a
  // GpuHostMsg_ChannelEstablished message.  The renderer ID is passed so that
  // the GPU process reuses an existing channel to that process if it exists.
  // This ID is a unique opaque identifier generated by the browser process.
  IPC_MESSAGE_CONTROL1(GpuMsg_EstablishChannel,
                       int /* renderer_id */)

  // Provides a synchronization point to guarantee that the processing of
  // previous asynchronous messages (i.e., GpuMsg_EstablishChannel) has
  // completed. (This message can't be synchronous because the
  // GpuProcessHost uses an IPC::ChannelProxy, which sends all messages
  // asynchronously.) Results in a GpuHostMsg_SynchronizeReply.
  IPC_MESSAGE_CONTROL0(GpuMsg_Synchronize)

  IPC_MESSAGE_CONTROL2(GpuMsg_NewRenderWidgetHostView,
                       GpuNativeWindowHandle, /* parent window */
                       int32 /* view_id */)

  // Tells the GPU process to create a context for collecting graphics card
  // information.
  IPC_MESSAGE_CONTROL0(GpuMsg_CollectGraphicsInfo)

  // Tells the GPU process to crash.
  IPC_MESSAGE_CONTROL0(GpuMsg_Crash)

  // Tells the GPU process to hang.
  IPC_MESSAGE_CONTROL0(GpuMsg_Hang)

  // Creates a new backing store.
  IPC_MESSAGE_ROUTED2(GpuMsg_NewBackingStore,
                      int32, /* backing_store_routing_id */
                      gfx::Size /* size */)

  // Creates a new video layer.
  IPC_MESSAGE_ROUTED2(GpuMsg_NewVideoLayer,
                      int32, /* video_layer_routing_id */
                      gfx::Size /* size */)

  // Updates the backing store with the given bitmap. The GPU process will send
  // back a GpuHostMsg_PaintToBackingStore_ACK after the paint is complete to
  // let the caller know the TransportDIB can be freed or reused.
  IPC_MESSAGE_ROUTED4(GpuMsg_PaintToBackingStore,
                      base::ProcessId, /* process */
                      TransportDIB::Id, /* bitmap */
                      gfx::Rect, /* bitmap_rect */
                      std::vector<gfx::Rect>) /* copy_rects */


  IPC_MESSAGE_ROUTED4(GpuMsg_ScrollBackingStore,
                      int, /* dx */
                      int, /* dy */
                      gfx::Rect, /* clip_rect */
                      gfx::Size) /* view_size */

  // Tells the GPU process that the RenderWidgetHost has painted the window.
  // Depending on the platform, the accelerated content may need to be painted
  // over the top.
  IPC_MESSAGE_ROUTED0(GpuMsg_WindowPainted)

  // Updates the video layer with the given YUV data. The GPU process will send
  // back a GpuHostMsg_PaintToVideoLayer_ACK after the paint is complete to
  // let the caller know the TransportDIB can be freed or reused.
  IPC_MESSAGE_ROUTED3(GpuMsg_PaintToVideoLayer,
                      base::ProcessId, /* process */
                      TransportDIB::Id, /* bitmap */
                      gfx::Rect) /* bitmap_rect */

IPC_END_MESSAGES(Gpu)

//------------------------------------------------------------------------------
// GPU Host Messages
// These are messages from the GPU process to the browser.
IPC_BEGIN_MESSAGES(GpuHost)

  // Sent in response to GpuMsg_PaintToBackingStore, see that for more.
  IPC_MESSAGE_ROUTED0(GpuHostMsg_PaintToBackingStore_ACK)

  // Sent in response to GpuMsg_PaintToVideoLayer, see that for more.
  IPC_MESSAGE_ROUTED0(GpuHostMsg_PaintToVideoLayer_ACK)

  // Response to a GpuHostMsg_EstablishChannel message.
  IPC_MESSAGE_CONTROL2(GpuHostMsg_ChannelEstablished,
                       IPC::ChannelHandle, /* channel_handle */
                       GPUInfo /* GPU logging stats */)

  // Response to a GpuMsg_Synchronize message.
  IPC_MESSAGE_CONTROL0(GpuHostMsg_SynchronizeReply)

  // Response to a GpuMsg_CollectGraphicsInfo.
  IPC_MESSAGE_CONTROL1(GpuHostMsg_GraphicsInfoCollected,
                       GPUInfo /* GPU logging stats */)

#if defined(OS_LINUX)
  // Get the XID for a view ID.
  IPC_SYNC_MESSAGE_CONTROL1_1(GpuHostMsg_GetViewXID,
                              gfx::NativeViewId, /* view */
                              unsigned long /* xid */)
#elif defined(OS_MACOSX)
  // This message, used on Mac OS X 10.6 and later (where IOSurface is
  // supported), is sent from the GPU process to the browser to indicate that a
  // new backing store was allocated for the given "window" (fake
  // PluginWindowHandle). The renderer ID and render view ID are needed in
  // order to uniquely identify the RenderWidgetHostView on the browser side.
  IPC_MESSAGE_CONTROL1(GpuHostMsg_AcceleratedSurfaceSetIOSurface,
                       GpuHostMsg_AcceleratedSurfaceSetIOSurface_Params)

  // This message notifies the browser process that the renderer
  // swapped the buffers associated with the given "window", which
  // should cause the browser to redraw the compositor's contents.
  IPC_MESSAGE_CONTROL3(GpuHostMsg_AcceleratedSurfaceBuffersSwapped,
                       int32, /* renderer_id */
                       int32, /* render_view_id */
                       gfx::PluginWindowHandle /* window */)
#endif

IPC_END_MESSAGES(GpuHost)

//------------------------------------------------------------------------------
// GPU Channel Messages
// These are messages from a renderer process to the GPU process.
IPC_BEGIN_MESSAGES(GpuChannel)

  // Tells the GPU process to create a new command buffer that renders directly
  // to a native view. The |render_view_id| is currently needed only on Mac OS
  // X in order to identify the window on the browser side into which the
  // rendering results go. A corresponding GpuCommandBufferStub is created.
  IPC_SYNC_MESSAGE_CONTROL2_1(GpuChannelMsg_CreateViewCommandBuffer,
                              gfx::NativeViewId, /* view */
                              int32, /* render_view_id */
                              int32 /* route_id */)

  // Tells the GPU process to create a new command buffer that renders to an
  // offscreen frame buffer. If parent_route_id is not zero, the texture backing
  // the frame buffer is mapped into the corresponding parent command buffer's
  // namespace, with the name of parent_texture_id. This ID is in the parent's
  // namespace.
  IPC_SYNC_MESSAGE_CONTROL4_1(GpuChannelMsg_CreateOffscreenCommandBuffer,
                              int32, /* parent_route_id */
                              gfx::Size, /* size */
                              std::vector<int>, /* attribs */
                              uint32, /* parent_texture_id */
                              int32 /* route_id */)

  // The CommandBufferProxy sends this to the GpuCommandBufferStub in its
  // destructor, so that the stub deletes the actual CommandBufferService
  // object that it's hosting.
  // TODO(apatrick): Implement this.
  IPC_SYNC_MESSAGE_CONTROL1_0(GpuChannelMsg_DestroyCommandBuffer,
                              int32 /* instance_id */)

  // Create hardware video decoder && associate it with the output |decoder_id|;
  // We need this to be control message because we had to map the GpuChannel and
  // |decoder_id|.
  IPC_MESSAGE_CONTROL2(GpuChannelMsg_CreateVideoDecoder,
                       int32, /* context_route_id */
                       int32) /* decoder_id */

  // Release all resource of the hardware video decoder which was assocaited
  // with the input |decoder_id|.
  // TODO(hclam): This message needs to be asynchronous.
  IPC_SYNC_MESSAGE_CONTROL1_0(GpuChannelMsg_DestroyVideoDecoder,
                              int32 /* decoder_id */)

IPC_END_MESSAGES(GpuChannel)

//------------------------------------------------------------------------------
// GPU Command Buffer Messages
// These are messages between a renderer process to the GPU process relating to
// a single OpenGL context.
IPC_BEGIN_MESSAGES(GpuCommandBuffer)
  // Initialize a command buffer with the given number of command entries.
  // Returns the shared memory handle for the command buffer mapped to the
  // calling process.
  IPC_SYNC_MESSAGE_ROUTED1_1(GpuCommandBufferMsg_Initialize,
                             int32 /* size */,
                             base::SharedMemoryHandle /* ring_buffer */)

  // Get the current state of the command buffer.
  IPC_SYNC_MESSAGE_ROUTED0_1(GpuCommandBufferMsg_GetState,
                             gpu::CommandBuffer::State /* state */)

  // Get the current state of the command buffer asynchronously. State is
  // returned via UpdateState message.
  IPC_MESSAGE_ROUTED0(GpuCommandBufferMsg_AsyncGetState)

  // Synchronize the put and get offsets of both processes. Caller passes its
  // current put offset. Current state (including get offset) is returned.
  IPC_SYNC_MESSAGE_ROUTED1_1(GpuCommandBufferMsg_Flush,
                             int32 /* put_offset */,
                             gpu::CommandBuffer::State /* state */)

  // Asynchronously synchronize the put and get offsets of both processes.
  // Caller passes its current put offset. Current state (including get offset)
  // is returned via an UpdateState message.
  IPC_MESSAGE_ROUTED1(GpuCommandBufferMsg_AsyncFlush,
                      int32 /* put_offset */)

  // Return the current state of the command buffer following a request via
  // an AsyncGetState or AsyncFlush message. (This message is sent from the
  // GPU process to the renderer process.)
  IPC_MESSAGE_ROUTED1(GpuCommandBufferMsg_UpdateState,
                      gpu::CommandBuffer::State /* state */)

  // Indicates that a SwapBuffers call has been issued.
  IPC_MESSAGE_ROUTED0(GpuCommandBufferMsg_SwapBuffers)

  // Create a shared memory transfer buffer. Returns an id that can be used to
  // identify the transfer buffer from a comment.
  IPC_SYNC_MESSAGE_ROUTED1_1(GpuCommandBufferMsg_CreateTransferBuffer,
                             int32 /* size */,
                             int32 /* id */)

  // Destroy a previously created transfer buffer.
  IPC_SYNC_MESSAGE_ROUTED1_0(GpuCommandBufferMsg_DestroyTransferBuffer,
                             int32 /* id */)

  // Get the shared memory handle for a transfer buffer mapped to the callers
  // process.
  IPC_SYNC_MESSAGE_ROUTED1_2(GpuCommandBufferMsg_GetTransferBuffer,
                             int32 /* id */,
                             base::SharedMemoryHandle /* transfer_buffer */,
                             uint32 /* size */)

  // Send from command buffer stub to proxy when window is invalid and must be
  // repainted.
  IPC_MESSAGE_ROUTED0(GpuCommandBufferMsg_NotifyRepaint)

  // Tells the GPU process to resize an offscreen frame buffer.
  IPC_MESSAGE_ROUTED1(GpuCommandBufferMsg_ResizeOffscreenFrameBuffer,
                      gfx::Size /* size */)

#if defined(OS_MACOSX)
  // On Mac OS X the GPU plugin must be offscreen, because there is no
  // true cross-process window hierarchy. For this reason we must send
  // resize events explicitly to the command buffer stub so it can
  // reallocate its backing store and send the new one back to the
  // browser. This message is currently used only on 10.6 and later.
  IPC_MESSAGE_ROUTED1(GpuCommandBufferMsg_SetWindowSize,
                      gfx::Size /* size */)

  // This message is sent from the GPU process to the renderer process (and
  // from there the browser process) that the buffers associated with the
  // given "window" were swapped, which should cause the browser to redraw
  // the various accelerated surfaces.
  IPC_MESSAGE_ROUTED1(GpuCommandBufferMsg_AcceleratedSurfaceBuffersSwapped,
                      gfx::PluginWindowHandle /* window */)
#endif

IPC_END_MESSAGES(GpuCommandBuffer)

//------------------------------------------------------------------------------
// GPU Video Decoder Messages
// These messages are sent from Renderer process to GPU process.
IPC_BEGIN_MESSAGES(GpuVideoDecoder)
  // Initialize and configure GpuVideoDecoder asynchronously.
  IPC_MESSAGE_ROUTED1(GpuVideoDecoderMsg_Initialize,
                      GpuVideoDecoderInitParam)

  // Destroy and release GpuVideoDecoder asynchronously.
  IPC_MESSAGE_ROUTED0(GpuVideoDecoderMsg_Destroy)

  // Start decoder flushing operation.
  IPC_MESSAGE_ROUTED0(GpuVideoDecoderMsg_Flush)

  // Tell the decoder to start prerolling.
  IPC_MESSAGE_ROUTED0(GpuVideoDecoderMsg_Preroll)

  // Send input buffer to GpuVideoDecoder.
  IPC_MESSAGE_ROUTED1(GpuVideoDecoderMsg_EmptyThisBuffer,
                      GpuVideoDecoderInputBufferParam)

  // Ask the GPU process to produce a video frame with the ID.
  IPC_MESSAGE_ROUTED1(GpuVideoDecoderMsg_ProduceVideoFrame,
                      int32) /* Video Frame ID */

  // Sent from Renderer process to the GPU process to notify that textures are
  // generated for a video frame.
  IPC_MESSAGE_ROUTED2(GpuVideoDecoderMsg_VideoFrameAllocated,
                      int32, /* Video Frame ID */
                      std::vector<uint32>) /* Textures for video frame */

IPC_END_MESSAGES(GpuVideoDecoder)

//------------------------------------------------------------------------------
// GPU Video Decoder Host Messages
// These messages are sent from GPU process to Renderer process.
IPC_BEGIN_MESSAGES(GpuVideoDecoderHost)
  // Inform GpuVideoDecoderHost that a GpuVideoDecoder is created.
  IPC_MESSAGE_ROUTED1(GpuVideoDecoderHostMsg_CreateVideoDecoderDone,
                      int32) /* decoder_id */

  // Confirm GpuVideoDecoder had been initialized or failed to initialize.
  // TODO(hclam): Change this to Done instead of ACK.
  IPC_MESSAGE_ROUTED1(GpuVideoDecoderHostMsg_InitializeACK,
                      GpuVideoDecoderInitDoneParam)

  // Confrim GpuVideoDecoder had been destroyed properly.
  // TODO(hclam): Change this to Done instead of ACK.
  IPC_MESSAGE_ROUTED0(GpuVideoDecoderHostMsg_DestroyACK)

  // Confirm decoder had been flushed.
  // TODO(hclam): Change this to Done instead of ACK.
  IPC_MESSAGE_ROUTED0(GpuVideoDecoderHostMsg_FlushACK)

  // Confirm preroll operation is done.
  IPC_MESSAGE_ROUTED0(GpuVideoDecoderHostMsg_PrerollDone)

  // GpuVideoDecoder has consumed input buffer from transfer buffer.
  // TODO(hclam): Change this to Done instead of ACK.
  IPC_MESSAGE_ROUTED0(GpuVideoDecoderHostMsg_EmptyThisBufferACK)

  // GpuVideoDecoder require new input buffer.
  IPC_MESSAGE_ROUTED0(GpuVideoDecoderHostMsg_EmptyThisBufferDone)

  // GpuVideoDecoder reports that a video frame is ready to be consumed.
  IPC_MESSAGE_ROUTED4(GpuVideoDecoderHostMsg_ConsumeVideoFrame,
                      int32, /* Video Frame ID */
                      int64, /* Timestamp in microseconds */
                      int64, /* Duration in microseconds */
                      int32) /* Flags */

  // Allocate video frames for output of the hardware video decoder.
  IPC_MESSAGE_ROUTED4(GpuVideoDecoderHostMsg_AllocateVideoFrames,
                      int32,  /* Number of video frames to generate */
                      uint32, /* Width of the video frame */
                      uint32, /* Height of the video frame */
                      int32   /* Format of the video frame */)

  // Release all video frames allocated for a hardware video decoder.
  IPC_MESSAGE_ROUTED0(GpuVideoDecoderHostMsg_ReleaseAllVideoFrames)

  // GpuVideoDecoder report output format change.
  IPC_MESSAGE_ROUTED1(GpuVideoDecoderHostMsg_MediaFormatChange,
                      GpuVideoDecoderFormatChangeParam)

  // GpuVideoDecoder report error.
  IPC_MESSAGE_ROUTED1(GpuVideoDecoderHostMsg_ErrorNotification,
                      GpuVideoDecoderErrorInfoParam)

IPC_END_MESSAGES(GpuVideoDecoderHost)
