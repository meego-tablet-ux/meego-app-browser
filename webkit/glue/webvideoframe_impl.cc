// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/glue/webvideoframe_impl.h"

#include "media/base/video_frame.h"
#include "third_party/WebKit/WebKit/chromium/public/WebVideoFrame.h"

using namespace WebKit;

namespace webkit_glue {

media::VideoFrame* WebVideoFrameImpl::toVideoFrame(
    WebVideoFrame* web_video_frame) {
  WebVideoFrameImpl* wrapped_frame =
      static_cast<WebVideoFrameImpl*>(web_video_frame);
  if (wrapped_frame)
    return wrapped_frame->video_frame_.get();
  return NULL;
}

WebVideoFrameImpl::WebVideoFrameImpl(
    scoped_refptr<media::VideoFrame> video_frame)
    : video_frame_(video_frame) {
}

#define COMPILE_ASSERT_MATCHING_ENUM(webkit_name, chromium_name) \
    COMPILE_ASSERT(int(WebKit::WebVideoFrame::webkit_name) == \
                   int(media::VideoFrame::chromium_name), \
                   mismatching_enums)
COMPILE_ASSERT_MATCHING_ENUM(FormatInvalid, INVALID);
COMPILE_ASSERT_MATCHING_ENUM(FormatRGB555, RGB555);
COMPILE_ASSERT_MATCHING_ENUM(FormatRGB565, RGB565);
COMPILE_ASSERT_MATCHING_ENUM(FormatRGB24, RGB24);
COMPILE_ASSERT_MATCHING_ENUM(FormatRGB32, RGB32);
COMPILE_ASSERT_MATCHING_ENUM(FormatRGBA, RGBA);
COMPILE_ASSERT_MATCHING_ENUM(FormatYV12, YV12);
COMPILE_ASSERT_MATCHING_ENUM(FormatYV16, YV16);
COMPILE_ASSERT_MATCHING_ENUM(FormatNV12, NV12);
COMPILE_ASSERT_MATCHING_ENUM(FormatEmpty, EMPTY);
COMPILE_ASSERT_MATCHING_ENUM(FormatASCII, ASCII);

COMPILE_ASSERT_MATCHING_ENUM(SurfaceTypeSystemMemory, TYPE_SYSTEM_MEMORY);
COMPILE_ASSERT_MATCHING_ENUM(SurfaceTypeOMXBufferHead, TYPE_OMXBUFFERHEAD);
COMPILE_ASSERT_MATCHING_ENUM(SurfaceTypeEGLImage, TYPE_EGL_IMAGE);
COMPILE_ASSERT_MATCHING_ENUM(SurfaceTypeMFBuffer, TYPE_MFBUFFER);
COMPILE_ASSERT_MATCHING_ENUM(SurfaceTypeDirect3DSurface, TYPE_DIRECT3DSURFACE);

WebVideoFrame::SurfaceType WebVideoFrameImpl::surfaceType() const {
  if (video_frame_.get())
    return static_cast<WebVideoFrame::SurfaceType>(video_frame_->type());
  return WebVideoFrame::SurfaceTypeSystemMemory;
}

WebVideoFrame::Format WebVideoFrameImpl::format() const {
  if (video_frame_.get())
    return static_cast<WebVideoFrame::Format>(video_frame_->format());
  return WebVideoFrame::FormatInvalid;
}

unsigned WebVideoFrameImpl::width() const {
  if (video_frame_.get())
    return video_frame_->width();
  return NULL;
}

unsigned WebVideoFrameImpl::height() const {
  if (video_frame_.get())
    return video_frame_->height();
  return NULL;
}

unsigned WebVideoFrameImpl::planes() const {
  if (video_frame_.get())
    return video_frame_->planes();
  return NULL;
}

int WebVideoFrameImpl::stride(unsigned plane) const {
  if (video_frame_.get())
    return static_cast<int>(video_frame_->stride(plane));
  return NULL;
}

const void* WebVideoFrameImpl::data(unsigned plane) const {
  if (video_frame_.get())
    return static_cast<const void*>(video_frame_->data(plane));
  return NULL;
}

}  // namespace webkit_glue
