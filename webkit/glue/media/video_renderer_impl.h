// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// The video renderer implementation to be use by the media pipeline. It lives
// inside video renderer thread and also WebKit's main thread. We need to be
// extra careful about members shared by two different threads, especially
// video frame buffers.

#ifndef WEBKIT_GLUE_MEDIA_VIDEO_RENDERER_IMPL_H_
#define WEBKIT_GLUE_MEDIA_VIDEO_RENDERER_IMPL_H_

#include "media/base/buffers.h"
#include "media/base/filters.h"
#include "media/filters/video_renderer_base.h"
#include "skia/ext/platform_canvas.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebMediaPlayer.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/size.h"
#include "webkit/glue/media/web_video_renderer.h"
#include "webkit/glue/webmediaplayer_impl.h"
#include "ipc/ipc_message.h"

namespace webkit_glue {

class VideoRendererImpl : public WebVideoRenderer {
 public:
  explicit VideoRendererImpl(bool pts_logging, int routing_id);
  virtual ~VideoRendererImpl();

  // WebVideoRenderer implementation.
  virtual void SetWebMediaPlayerImplProxy(WebMediaPlayerImpl::Proxy* proxy);
  virtual void SetRect(const gfx::Rect& rect);
  virtual void Paint(SkCanvas* canvas, const gfx::Rect& dest_rect);
  virtual void GetCurrentFrame(scoped_refptr<media::VideoFrame>* frame_out);
  virtual void PutCurrentFrame(scoped_refptr<media::VideoFrame> frame);

  gfx::Rect Rect() {return video_rect_;}

  void SetIsOverlapped(bool overlapped);
 protected:
  // Method called by VideoRendererBase during initialization.
  virtual bool OnInitialize(media::VideoDecoder* decoder);

  // Method called by the VideoRendererBase when stopping.
  virtual void OnStop(media::FilterCallback* callback);

  // Method called by the VideoRendererBase when a frame is available.
  virtual void OnFrameAvailable();

 private:
  // Send an IPC message to the browser process.  The routing ID of the message
  // is assumed to match |routing_id_|.
  void Send(IPC::Message* msg);

  // handle double pixmap
  void FreeVideoPixmap(bool notify);
  int GetVideoPixmap(bool create);

  // handle DirectPaint
  void InitDirectPaint(const gfx::Rect& dest_rect);
  void ExitDirectPaint();
  bool DirectPaintInited();
  void DirectPaint();
  void EnableDirectPaint(bool enable);

  // Determine the conditions to perform fast paint. Returns true if we can do
  // fast paint otherwise false.
  bool CanFastPaint(SkCanvas* canvas, const gfx::Rect& dest_rect);

  // Slow paint does a YUV => RGB, and scaled blit in two separate operations.
  void SlowPaint(media::VideoFrame* video_frame,
                 SkCanvas* canvas,
                 const gfx::Rect& dest_rect);

  // Fast paint does YUV => RGB, scaling, blitting all in one step into the
  // canvas. It's not always safe and appropriate to perform fast paint.
  // CanFastPaint() is used to determine the conditions.
  void FastPaint(media::VideoFrame* video_frame,
                 SkCanvas* canvas,
                 const gfx::Rect& dest_rect);

#if defined (TOOLKIT_MEEGOTOUCH)
  /*_DEV2_H264_*/
  void H264GetPixmapAndmXImage(WebMediaPlayerImpl::Proxy* proxy, Display *dTmp, int w_, int h_, XShmSegmentInfo * shm_tmp);
  void H264Paint(WebMediaPlayerImpl::Proxy* proxy, media::VideoFrame* video_frame, int dst_w, int dst_h, uint8 * pDst, int stride);
  void H264CreateXImage(WebMediaPlayerImpl::Proxy* proxy, Display *dTmp, int w_, int h_, XShmSegmentInfo * shm_tmp);
  void H264FreePixmap(WebMediaPlayerImpl::Proxy* proxy, Display *dTmp);
#endif

  void TransformToSkIRect(const SkMatrix& matrix, const gfx::Rect& src_rect,
                          SkIRect* dest_rect);

  // Pointer to our parent object that is called to request repaints.
  scoped_refptr<WebMediaPlayerImpl::Proxy> proxy_;

  // An RGB bitmap used to convert the video frames.
  SkBitmap bitmap_;

  // These two members are used to determine if the |bitmap_| contains
  // an already converted image of the current frame.  IMPORTANT NOTE:  The
  // value of |last_converted_frame_| must only be used for comparison purposes,
  // and it should be assumed that the value of the pointer is INVALID unless
  // it matches the pointer returned from GetCurrentFrame().  Even then, just
  // to make sure, we compare the timestamp to be sure the bits in the
  // |current_frame_bitmap_| are valid.
  media::VideoFrame* last_converted_frame_;
  base::TimeDelta last_converted_timestamp_;

  // The size of the video.
  gfx::Size video_size_;

  gfx::Size video_window_size_;

  // Whether we're logging video presentation timestamps (PTS).
  bool pts_logging_;
  int routing_id_;

  unsigned int video_seq_;
  unsigned int video_double_pixmap_[2];

  gfx::Rect video_rect_;
  void *video_display_;
  bool direct_paint_enabled_;
  bool direct_paint_inited_;
  bool direct_paint_init_tried_;
  bool paint_reset_;

  bool is_overlapped_;
  
  DISALLOW_COPY_AND_ASSIGN(VideoRendererImpl);
};

}  // namespace webkit_glue

#endif  // WEBKIT_GLUE_MEDIA_VIDEO_RENDERER_IMPL_H_
