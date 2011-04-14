// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_VIDEO_FFMPEG_VIDEO_DECODE_ENGINE_H_
#define MEDIA_VIDEO_FFMPEG_VIDEO_DECODE_ENGINE_H_

#include <deque>

#include "base/memory/scoped_ptr.h"
#include "media/ffmpeg/ffmpeg_common.h"
#include "media/video/video_decode_engine.h"

#if defined (TOOLKIT_MEEGOTOUCH)
//#define DEV2_TRACE()  LOG(ERROR) << "FUNCTION: <" << __FUNCTION__ << ">. LINE:  [" << __LINE__ << "]\n"
#define DEV2_TRACE()  

/*Enable VAAPI H264 Supporting for FFMPEG.*/
#define _DEV2_H264_
/*used for debuginfo*/
//#define _DEV2_DEBUG_

// _DEV2_H264_
#include <va/va.h>
#include <va/va_x11.h>

/* VA-API Formats */
#define VAAPI_H264                 (0x00000264)
/* Numbers of video surfaces */
/*MPEG2/4/VC1: 3 */
#define NUM_VIDEO_SURFACES_H264  21 /* 1 decode frame, up to 20 references */

/*free all memory allocated by calloc for global using*/
#define DEV2FreeCalloc(p)   \
{   			\
    if(p){		\
	free(p);	\
	p = NULL;	\
    }			\
}

struct VaapiSurface {
    VASurfaceID id;
    VAImage     image;
    int         is_bound; /* Flag: image bound to the surface? */
    int         used;     /* is used by codec*/
};

/*define hw context*/
struct hw_context{
    void *display;
    uint32_t config_id;
    uint32_t context_id;
    uint32_t res[12];
};

#endif

// FFmpeg types.
struct AVCodecContext;
struct AVFrame;

namespace media {

class FFmpegVideoAllocator;

class FFmpegVideoDecodeEngine : public VideoDecodeEngine {
 public:
  FFmpegVideoDecodeEngine();
  virtual ~FFmpegVideoDecodeEngine();

  // Implementation of the VideoDecodeEngine Interface.
  virtual void Initialize(MessageLoop* message_loop,
                          VideoDecodeEngine::EventHandler* event_handler,
                          VideoDecodeContext* context,
                          const VideoCodecConfig& config);
  virtual void ConsumeVideoSample(scoped_refptr<Buffer> buffer);
  virtual void ProduceVideoFrame(scoped_refptr<VideoFrame> frame);
  virtual void Uninitialize();
  virtual void Flush();
  virtual void Seek();

  VideoFrame::Format GetSurfaceFormat() const;

#if defined (TOOLKIT_MEEGOTOUCH)
// _DEV2_H264_

  /*callback of ffmpeg codec*/
  static int GetBufferAndSurface(AVCodecContext * ctx, AVFrame * pic);
  static void ReleaseBufferAndSurface(AVCodecContext * ctx, AVFrame * pic);
  static enum PixelFormat GetFormatAndConfig(struct AVCodecContext *avctx, const enum PixelFormat *pix_fmt);

/* Numbers of video surfaces */
/*MPEG2/4/VC1: 3 */
#define NUM_VIDEO_SURFACES_H264  21 /* 1 decode frame, up to 20 references */

  /*Public Variables*/
  struct hw_context    *hw_context_;
  VASurfaceID             hw_surface_ids_[NUM_VIDEO_SURFACES_H264 + 1]; /*last element is used for status checking*/                   
  struct VaapiSurface     *hw_free_surfaces_[NUM_VIDEO_SURFACES_H264 + 1];  /*last element is used for status checking*/               
  int                     hw_num_surfaces_;    
  int                     hw_free_surfaces_head_index_;   
  int                     hw_accel_;   

#endif

 private:
  void DecodeFrame(scoped_refptr<Buffer> buffer);
  void ReadInput();
  void TryToFinishPendingFlush();

  AVCodecContext* codec_context_;
  scoped_ptr_malloc<AVFrame, ScopedPtrAVFree> av_frame_;
  VideoDecodeEngine::EventHandler* event_handler_;

#if defined (TOOLKIT_MEEGOTOUCH)
// _DEV2_H264_
  int InitializeHwEngine(void);
  int ConfigHwEngine(uint32_t width, uint32_t height, uint32_t format);
  int CheckStatus(VAStatus status, const char *msg);
  int CopyBufferFrmSurface(scoped_refptr<VideoFrame> video_frame,
                      const AVFrame* frame);
  void UnInitializeHwEngine(void);
#endif
  // Frame rate of the video.
  int frame_rate_numerator_;
  int frame_rate_denominator_;

  // Whether direct rendering is used.
  bool direct_rendering_;

  // Used when direct rendering is used to recycle output buffers.
  scoped_ptr<FFmpegVideoAllocator> allocator_;

  // Indicate how many buffers are pending on input port of this filter:
  // Increment when engine receive one input packet from demuxer;
  // Decrement when engine send one input packet to demuxer;
  int pending_input_buffers_;

  // Indicate how many buffers are pending on output port of this filter:
  // Increment when engine receive one output frame from renderer;
  // Decrement when engine send one output frame to renderer;
  int pending_output_buffers_;

  // Whether end of stream had been reached at output side.
  bool output_eos_reached_;

  // Used when direct rendering is disabled to hold available output buffers.
  std::deque<scoped_refptr<VideoFrame> > frame_queue_available_;

  // Whether flush operation is pending.
  bool flush_pending_;

  DISALLOW_COPY_AND_ASSIGN(FFmpegVideoDecodeEngine);
};

}  // namespace media

#endif  // MEDIA_VIDEO_FFMPEG_VIDEO_DECODE_ENGINE_H_
