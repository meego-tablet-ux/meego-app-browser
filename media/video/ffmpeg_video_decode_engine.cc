// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/video/ffmpeg_video_decode_engine.h"

#include "base/command_line.h"
#include "base/string_number_conversions.h"
#include "base/task.h"
#include "media/base/buffers.h"
#include "media/base/callback.h"
#include "media/base/limits.h"
#include "media/base/media_switches.h"
#include "media/base/pipeline.h"
#include "media/ffmpeg/ffmpeg_common.h"
#include "media/filters/ffmpeg_demuxer.h"
#include "media/video/ffmpeg_video_allocator.h"

#if defined (TOOLKIT_MEEGOTOUCH)
/*
_DEV2_OPT_
*/
#include "x11_util.h"
#include <sys/syscall.h>
#include <sys/shm.h>
/*XA_WINDOW*/
//#include <X11/Xatom.h>
#include <X11/X.h>
extern Display* g_display;

static unsigned long frm ;

int shmkey = 0;
/*us*/
double GetTick(void)
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL))
        return 0;
    return tv.tv_usec+tv.tv_sec*1000000.0;
}

Display *mDisplay = NULL; 
extern Window subwin ;
unsigned int CodecID = 0;

#endif

namespace media {

#if defined (TOOLKIT_MEEGOTOUCH)
// _DEV2_H264_

#ifdef _DEV2_DEBUG_
struct VappiEqualizer {
    VADisplayAttribute brightness;
    VADisplayAttribute contrast;
    VADisplayAttribute hue;
    VADisplayAttribute saturation;        
};     

static VAImageFormat           *hw_image_formats_;     
static int                      hw_num_image_formats_;
static int                      hw_num_profiles_;                  
static int                      hw_num_entrypoints_;                 
static VAEntrypoint            *hw_entrypoints_;          
static VAImageFormat           *hw_subpic_formats_;           
static unsigned int            *hw_subpic_flags_;      
static int                      hw_num_subpic_formats_;  
static struct VappiEqualizer    hw_equalizer_; 
static VAProfile               *hw_profiles_;           
#endif

#endif

FFmpegVideoDecodeEngine::FFmpegVideoDecodeEngine()
    : codec_context_(NULL),
      event_handler_(NULL),
      frame_rate_numerator_(0),
      frame_rate_denominator_(0),
      direct_rendering_(false),
      pending_input_buffers_(0),
      pending_output_buffers_(0),
      output_eos_reached_(false),
      flush_pending_(false) {
}

FFmpegVideoDecodeEngine::~FFmpegVideoDecodeEngine() {
  if (codec_context_) {
    av_free(codec_context_->extradata);
    avcodec_close(codec_context_);
    av_free(codec_context_);
  }
}

void FFmpegVideoDecodeEngine::Initialize(
    MessageLoop* message_loop,
    VideoDecodeEngine::EventHandler* event_handler,
    VideoDecodeContext* context,
    const VideoCodecConfig& config) {
  allocator_.reset(new FFmpegVideoAllocator());

  // Always try to use three threads for video decoding.  There is little reason
  // not to since current day CPUs tend to be multi-core and we measured
  // performance benefits on older machines such as P4s with hyperthreading.
  //
  // Handling decoding on separate threads also frees up the pipeline thread to
  // continue processing. Although it'd be nice to have the option of a single
  // decoding thread, FFmpeg treats having one thread the same as having zero
  // threads (i.e., avcodec_decode_video() will execute on the calling thread).
  // Yet another reason for having two threads :)
  static const int kDecodeThreads = 2;
  static const int kMaxDecodeThreads = 16;

  // Initialize AVCodecContext structure.
  codec_context_ = avcodec_alloc_context();

  // TODO(scherkus): should video format get passed in via VideoCodecConfig?
  codec_context_->pix_fmt = PIX_FMT_YUV420P;
  codec_context_->codec_type = AVMEDIA_TYPE_VIDEO;
  codec_context_->codec_id = VideoCodecToCodecID(config.codec());
  codec_context_->coded_width = config.width();
  codec_context_->coded_height = config.height();

  frame_rate_numerator_ = config.frame_rate_numerator();
  frame_rate_denominator_ = config.frame_rate_denominator();

  if (config.extra_data() != NULL) {
    codec_context_->extradata_size = config.extra_data_size();
    codec_context_->extradata =
        reinterpret_cast<uint8_t*>(av_malloc(config.extra_data_size()));
    memcpy(codec_context_->extradata, config.extra_data(),
           config.extra_data_size());
  }

  // Enable motion vector search (potentially slow), strong deblocking filter
  // for damaged macroblocks, and set our error detection sensitivity.
  codec_context_->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;
  codec_context_->error_recognition = FF_ER_CAREFUL;

  AVCodec* codec = avcodec_find_decoder(codec_context_->codec_id);

  if (codec) {
#ifdef FF_THREAD_FRAME  // Only defined in FFMPEG-MT.
    direct_rendering_ = codec->capabilities & CODEC_CAP_DR1 ? true : false;
#endif

#if defined (TOOLKIT_MEEGOTOUCH)
    /*disable direct_rendering for H264.*/ 
    /*because Chromium Media Engine only support YV12, and YV16 format.*/
    /*add */
    CodecID = codec_context_->codec_id;
    /*FIXME : To Benchmark here for further optmization*/
// _DEV2_H264_
    if((codec_context_->codec_id == CODEC_ID_H264) ){
      int ret = -1;

      /*Serveral things do here: 
        a> do InitializeHwEngine 
        b> reset GetFormatAndConfig function.
        c> pass vaapi_ctx to codec internal.
      */
      DEV2_TRACE();
      ret = InitializeHwEngine();

      if(0 == ret){
      //if(0 ){ force sw decoding
          /*support VAAPI Hw acceleration*/

          /*pass hw_context to codec internal.*/
          codec_context_->hwaccel_context = hw_context_;

          /*reset GetFormatAndConfig to enable hw accelerated*/
          codec_context_->get_format = GetFormatAndConfig;
          codec_context_->thread_count = 1;
          codec_context_->slice_flags = SLICE_FLAG_CODED_ORDER|SLICE_FLAG_ALLOW_FIELD;
          codec_context_->get_buffer = GetBufferAndSurface;
          codec_context_->reget_buffer = GetBufferAndSurface;
          codec_context_->release_buffer = ReleaseBufferAndSurface;
          codec_context_->context_model = (int)this;
          //VA_RT_FORMAT_YUV420;
      hw_accel_      = 1;
          direct_rendering_ = 0;
            //DLOG(INFO) << "disable direct rendering for H264 codec";
      }else{
          if(mDisplay){
              XCloseDisplay(mDisplay);
              mDisplay = NULL;
          }
          /* we are not support sw H264 decoding*/
          codec = NULL;
          LOG(ERROR) << "No H264 Support on this platform" ;

      }

    }
#endif
    if (direct_rendering_) {
      DVLOG(1) << "direct rendering is used";
      allocator_->Initialize(codec_context_, GetSurfaceFormat());
    }
  }

  // TODO(fbarchard): Improve thread logic based on size / codec.
  // TODO(fbarchard): Fix bug affecting video-cookie.html
  int decode_threads = (codec_context_->codec_id == CODEC_ID_THEORA) ?
    1 : kDecodeThreads;

  const CommandLine* cmd_line = CommandLine::ForCurrentProcess();
  std::string threads(cmd_line->GetSwitchValueASCII(switches::kVideoThreads));
  if ((!threads.empty() &&
      !base::StringToInt(threads, &decode_threads)) ||
      decode_threads < 0 || decode_threads > kMaxDecodeThreads) {
    decode_threads = kDecodeThreads;
  }

#if defined (TOOLKIT_MEEGOTOUCH)
{
  if((CODEC_ID_H264 == codec_context_->codec_id) && hw_accel_){
      decode_threads = 1;
  }
}
#endif

  // We don't allocate AVFrame on the stack since different versions of FFmpeg
  // may change the size of AVFrame, causing stack corruption.  The solution is
  // to let FFmpeg allocate the structure via avcodec_alloc_frame().
  av_frame_.reset(avcodec_alloc_frame());
  VideoCodecInfo info;
  info.success = false;
  info.provides_buffers = true;
  info.stream_info.surface_type = VideoFrame::TYPE_SYSTEM_MEMORY;
  info.stream_info.surface_format = GetSurfaceFormat();
  info.stream_info.surface_width = config.width();
  info.stream_info.surface_height = config.height();

  // If we do not have enough buffers, we will report error too.
  bool buffer_allocated = true;
  frame_queue_available_.clear();
  if (!direct_rendering_) {
    // Create output buffer pool when direct rendering is not used.
    for (size_t i = 0; i < Limits::kMaxVideoFrames; ++i) {
      scoped_refptr<VideoFrame> video_frame;
      VideoFrame::CreateFrame(VideoFrame::YV12,
                              config.width(),
                              config.height(),
                              kNoTimestamp,
                              kNoTimestamp,
                              &video_frame);
      if (!video_frame.get()) {
        buffer_allocated = false;
        break;
      }
      frame_queue_available_.push_back(video_frame);
    }
  }

  if (codec &&
      avcodec_thread_init(codec_context_, decode_threads) >= 0 &&
      avcodec_open(codec_context_, codec) >= 0 &&
      av_frame_.get() &&
      buffer_allocated) {
    info.success = true;
  }
  event_handler_ = event_handler;
  event_handler_->OnInitializeComplete(info);
}

#if defined (TOOLKIT_MEEGOTOUCH)
// _DEV2_H264_

int FFmpegVideoDecodeEngine::CheckStatus(VAStatus status, const char *msg)
{
    if (status != VA_STATUS_SUCCESS) {
        LOG(ERROR) << " " << msg << vaErrorStr(status);
        return 1;
    }
    return 0;
}

#if 1

/*optimize Output to pixmap with VAAPI*/
int FFmpegVideoDecodeEngine::CopyBufferFrmSurface(scoped_refptr<VideoFrame> video_frame,
                      const AVFrame* frame) 
{
    int ret = 0;
    VAStatus status;
    VASurfaceID surface_id = (VASurfaceID)frame->data[3];

    DEV2_TRACE();

    /* return Surface to Render Engine */
    video_frame->data_[0] = (uint8_t*)mDisplay; /*VaapiSurface, structure*/
    //video_frame->data_[0] = frame->data[0]; /*VaapiSurface, structure*/
    video_frame->data_[1] = frame->data[1]; /*Reserved*/
    video_frame->data_[2] = frame->data[2]; /*Reserved*/
    video_frame->idx_ = (int)frame->data[3]; /*Surface Id*/

    /*video_frame->data_[1]: reserved to check in render engine for only FFMPEG/LIBVA */
    video_frame->data_[1] = (uint8_t*)0x264;

    /*hw_context*/
    video_frame->data_[2] = (uint8_t*)hw_context_->display;
    
    return ret;
}

#else
int FFmpegVideoDecodeEngine::CopyBufferFrmSurface(scoped_refptr<VideoFrame> video_frame,
                      const AVFrame* frame) 
{

    struct VaapiSurface *surface = (VaapiSurface *)frame->data[0];
    VASurfaceID surface_id = (uintptr_t)frame->data[3];
    VAStatus status;
    VAImage va_image;
    uint8_t *image_data = NULL;

    DEV2_TRACE();
    
    status = vaDeriveImage(hw_context_->display, surface_id, &va_image);
    if(status != VA_STATUS_SUCCESS){
        DEV2_TRACE();
        LOG(INFO) << "vaDeriveImage called error";
        return -1;
    }
    status = vaMapBuffer(hw_context_->display, va_image.buf,
                         (void **)&image_data);
    if(status != VA_STATUS_SUCCESS){
        DEV2_TRACE();
        LOG(INFO) << "vaMapBuffer called error";
        return -1;
    }
 
#if 0
    LOG(INFO) << "surface : " << surface << "\n" << "surface id : "<< surface_id;
    LOG(INFO) << "pitches.0: "<< va_image.pitches[0];
    LOG(INFO) << "pitches.1: "<< va_image.pitches[1];
    LOG(INFO) << "pitches.2: "<< va_image.pitches[2];
    LOG(INFO) << "offsets.0: "<< va_image.offsets[0];
    LOG(INFO) << "offsets.1: "<< va_image.offsets[1];
    LOG(INFO) << "offsets.2: "<< va_image.offsets[2];
    LOG(INFO) << "buf id   : "<< va_image.buf;
    LOG(INFO) << "buf addr : "<< (int)image_data;
    LOG(INFO) << "width    : "<< va_image.width;
    LOG(INFO) << "height   : "<< va_image.height;
    LOG(INFO)<< "format   : "<< va_image.format.fourcc; 
    LOG(INFO)<< "NV12     : "<< VA_FOURCC('N','V','1','2');
    LOG(INFO)<< "YV12     : "<< VA_FOURCC('Y','V','1','2');
    LOG(INFO)<< "YV16     : "<< VA_FOURCC('Y','V','1','6');
    LOG(INFO)<< "I420     : "<< VA_FOURCC('I','4','2','0');
    LOG(INFO)<< "IYUV     : "<< VA_FOURCC('I','Y','U','V');
#endif

    /*simulate what a I420 layer-out.*/
    /*because hw supports NV12 formats:*/
    /*need copy and re-assignment.*/
    uint8_t *psrcY = image_data;
    uint8_t *psrcU = image_data + va_image.offsets[1];
    uint8_t *psrcV = image_data + va_image.offsets[1] + 1;
    uint8_t *pdestY = video_frame->data_[0];
    uint8_t *pdestU = video_frame->data_[1];
    uint8_t *pdestV = video_frame->data_[2];
  
    int i = 0;
    int j = 0;
    int height = video_frame->height();
    int width = video_frame->width();

#if 0
    LOG(INFO) << "pSrcY   " <<  (int)psrcY;
    LOG(INFO) << "pSrcU   " <<  (int)psrcU;
    LOG(INFO) << "pSrcV   " <<  (int)psrcV;
    LOG (INFO) << "render surface ID " << frame->linesize[3];


{
    /*dump va_image to file, only for testing.*/
    static FILE* pfile = NULL;
    pfile = fopen("d1.yuv", "ab");
    uint8_t *pY  = psrcY;
    uint8_t *pU  = psrcU;
    uint8_t *pV  = psrcV;
    
    int ii , jj ; 
    /*Y*/
    for(ii = 0; ii < va_image.height; ii ++){
        fwrite (pY, 1, va_image.width, pfile);
        pY += va_image.pitches[0];
    }

    /*U*/
    for(ii = 0; ii < (va_image.height>>1); ii ++){
        for(jj = 0; jj < (va_image.width); jj+=2){
        fwrite (pU + jj, 1, 1 , pfile);
        }
        pU += va_image.pitches[1];
    }

    /*V*/
    for(ii = 0; ii < (va_image.height>>1); ii ++){
        for(jj = 0; jj < (va_image.width); jj+=2){
        fwrite (pV + jj, 1, 1 , pfile);
        }
        pV += va_image.pitches[1];
    }
    
    fclose(pfile);
}
#endif


    video_frame->idx_ = frame->linesize[3];
    //LOG(INFO) << "Part Copy: Green"; 
 
    /*Y*/
    for(i = 0 ; i < height; i ++){
    //for(i = 0 ; i < va_image.height; i ++){
    memcpy(pdestY, psrcY, width);
    pdestY += video_frame->strides_[0];
        psrcY  += va_image.pitches[0];
    }

    /*UV*/
    for(i = 0 ; i < (height>>1); i ++){
    /*one row of UV is copyed.*/
    for(j = 0; j < (width); j +=2){
        *(pdestU + (j>>1)) = *(psrcU + j);
        *(pdestV + (j>>1)) = *(psrcV + j);
    }
        psrcU += va_image.pitches[0];
        psrcV += va_image.pitches[0];
    pdestV += video_frame->strides_[1];
    pdestU += video_frame->strides_[1];
    }

    status = vaUnmapBuffer(hw_context_->display, va_image.buf);
    if(CheckStatus(status, "vaUnmapBuffer is failed")){
    DEV2_TRACE();
    return -1;
    }


    status = vaDestroyImage(hw_context_->display, va_image.image_id);
    if(CheckStatus(status, "vaDestroyImage is failed ")){
    DEV2_TRACE();
    return -1;
    }

    return 0;
}
#endif

#endif /*MEEGO TOUCH*/


static void CopyPlane(size_t plane,
                      scoped_refptr<VideoFrame> video_frame,
                      const AVFrame* frame,
                      size_t source_height) {
  DCHECK_EQ(video_frame->width() % 2, 0u);
  const uint8* source = frame->data[plane];
  const size_t source_stride = frame->linesize[plane];
  uint8* dest = video_frame->data(plane);
  const size_t dest_stride = video_frame->stride(plane);

  // Calculate amounts to copy and clamp to minium frame dimensions.
  size_t bytes_per_line = video_frame->width();
  size_t copy_lines = std::min(video_frame->height(), source_height);
  if (plane != VideoFrame::kYPlane) {
    bytes_per_line /= 2;
    if (video_frame->format() == VideoFrame::YV12) {
      copy_lines = (copy_lines + 1) / 2;
    }
  }
  bytes_per_line = std::min(bytes_per_line, source_stride);

  // Copy!
  for (size_t i = 0; i < copy_lines; ++i) {
    memcpy(dest, source, bytes_per_line);
    source += source_stride;
    dest += dest_stride;
  }
}

void FFmpegVideoDecodeEngine::ConsumeVideoSample(
    scoped_refptr<Buffer> buffer) {
  pending_input_buffers_--;
  if (flush_pending_) {
    TryToFinishPendingFlush();
  } else {
    // Otherwise try to decode this buffer.
    DecodeFrame(buffer);
  }
}

void FFmpegVideoDecodeEngine::ProduceVideoFrame(
    scoped_refptr<VideoFrame> frame) {
  // We should never receive NULL frame or EOS frame.
  DCHECK(frame.get() && !frame->IsEndOfStream());

  // Increment pending output buffer count.
  pending_output_buffers_++;

  // Return this frame to available pool or allocator after display.
  if (direct_rendering_)
    allocator_->DisplayDone(codec_context_, frame);
  else
    frame_queue_available_.push_back(frame);

  if (flush_pending_) {
    TryToFinishPendingFlush();
  } else if (!output_eos_reached_) {
    // If we already deliver EOS to renderer, we stop reading new input.
    ReadInput();
  }
}

// Try to decode frame when both input and output are ready.
void FFmpegVideoDecodeEngine::DecodeFrame(scoped_refptr<Buffer> buffer) {
  scoped_refptr<VideoFrame> video_frame;

  // Create a packet for input data.
  // Due to FFmpeg API changes we no longer have const read-only pointers.
  AVPacket packet;
  av_init_packet(&packet);
  packet.data = const_cast<uint8*>(buffer->GetData());
  packet.size = buffer->GetDataSize();

  PipelineStatistics statistics;
  statistics.video_bytes_decoded = buffer->GetDataSize();

  // Let FFmpeg handle presentation timestamp reordering.
  codec_context_->reordered_opaque = buffer->GetTimestamp().InMicroseconds();

  // This is for codecs not using get_buffer to initialize
  // |av_frame_->reordered_opaque|
  av_frame_->reordered_opaque = codec_context_->reordered_opaque;

  int frame_decoded = 0;
  int result = avcodec_decode_video2(codec_context_,
                                     av_frame_.get(),
                                     &frame_decoded,
                                     &packet);

#if defined (TOOLKIT_MEEGOTOUCH)
  av_frame_->reordered_opaque = codec_context_->reordered_opaque;
#endif

  // Log the problem if we can't decode a video frame and exit early.
  if (result < 0) {
    VLOG(1) << "Error decoding a video frame with timestamp: "
            << buffer->GetTimestamp().InMicroseconds() << " us, duration: "
            << buffer->GetDuration().InMicroseconds() << " us, packet size: "
            << buffer->GetDataSize() << " bytes";
    // TODO(jiesun): call event_handler_->OnError() instead.
#if defined (TOOLKIT_MEEGOTOUCH)
    if((CODEC_ID_H264 == codec_context_->codec_id) && hw_accel_){
        return;
    }
#endif
    event_handler_->ConsumeVideoFrame(video_frame, statistics);
    return;
  }

  // If frame_decoded == 0, then no frame was produced.
  // In this case, if we already begin to flush codec with empty
  // input packet at the end of input stream, the first time we
  // encounter frame_decoded == 0 signal output frame had been
  // drained, we mark the flag. Otherwise we read from demuxer again.
  if (frame_decoded == 0) {
    if (buffer->IsEndOfStream()) {  // We had started flushing.
      LOG(ERROR) << "End Of Stream Event";
#if defined (TOOLKIT_MEEGOTOUCH)
// _DEV2_H264_
      /*lost here?*/
      if(!hw_accel_){
      //if(CODEC_ID_H264 != codec_context_->codec_id){
    /*not H264 , run consume*/
      event_handler_->ConsumeVideoFrame(video_frame, statistics);
      }
#else
      event_handler_->ConsumeVideoFrame(video_frame, statistics);
#endif
      output_eos_reached_ = true;
    } else {
      LOG(ERROR) << "Loading Again.";
      ReadInput();
    }
    LOG(INFO) << " ";
    return;
  }

#if defined (TOOLKIT_MEEGOTOUCH)
// _DEV2_H264_
  frm ++;
  /*force to exit for performance evaluation*/
  if(!hw_accel_){
    /*run here only for no hw accelerated codec*/
#endif

  // TODO(fbarchard): Work around for FFmpeg http://crbug.com/27675
  // The decoder is in a bad state and not decoding correctly.
  // Checking for NULL avoids a crash in CopyPlane().
  if (!av_frame_->data[VideoFrame::kYPlane] ||
      !av_frame_->data[VideoFrame::kUPlane] ||
      !av_frame_->data[VideoFrame::kVPlane]) {
    // TODO(jiesun): call event_handler_->OnError() instead.
    event_handler_->ConsumeVideoFrame(video_frame, statistics);
    return;
  }

#if defined (TOOLKIT_MEEGOTOUCH)
// _DEV2_H264_
  }
#endif

  // Determine timestamp and calculate the duration based on the repeat picture
  // count.  According to FFmpeg docs, the total duration can be calculated as
  // follows:
  //   fps = 1 / time_base
  //
  //   duration = (1 / fps) + (repeat_pict) / (2 * fps)
  //            = (2 + repeat_pict) / (2 * fps)
  //            = (2 + repeat_pict) / (2 * (1 / time_base))
  DCHECK_LE(av_frame_->repeat_pict, 2);  // Sanity check.
  AVRational doubled_time_base;
  doubled_time_base.num = frame_rate_denominator_;
  doubled_time_base.den = frame_rate_numerator_ * 2;

  base::TimeDelta timestamp =
      base::TimeDelta::FromMicroseconds(av_frame_->reordered_opaque);
  base::TimeDelta duration =
      ConvertFromTimeBase(doubled_time_base, 2 + av_frame_->repeat_pict);

  if (!direct_rendering_) {
    // Available frame is guaranteed, because we issue as much reads as
    // available frame, except the case of |frame_decoded| == 0, which
    // implies decoder order delay, and force us to read more inputs.
    DCHECK(frame_queue_available_.size());
    video_frame = frame_queue_available_.front();
    frame_queue_available_.pop_front();

#if defined (TOOLKIT_MEEGOTOUCH)
// _DEV2_H264_
    //LOG(INFO) << "not Direct rendering: format : "<< video_frame->format() << " surface idx_: " << av_frame_.get()->linesize[3];

    if((CODEC_ID_H264 == codec_context_->codec_id) && hw_accel_){
      CopyBufferFrmSurface(video_frame.get(), av_frame_.get());
    }else
#endif
    {
    // Copy the frame data since FFmpeg reuses internal buffers for AVFrame
    // output, meaning the data is only valid until the next
    // avcodec_decode_video() call.
    size_t height = codec_context_->height;
    CopyPlane(VideoFrame::kYPlane, video_frame.get(), av_frame_.get(), height);
    CopyPlane(VideoFrame::kUPlane, video_frame.get(), av_frame_.get(), height);
    CopyPlane(VideoFrame::kVPlane, video_frame.get(), av_frame_.get(), height);
    }
  } else {
    // Get the VideoFrame from allocator which associate with av_frame_.
    video_frame = allocator_->DecodeDone(codec_context_, av_frame_.get());
#if defined (TOOLKIT_MEEGOTOUCH)
// _DEV2_H264_
  if((CODEC_ID_H264 == codec_context_->codec_id) && hw_accel_){
    /* direct rendering , no memory copy, theoretically, maybe cause playing jittering, 
     * because current buffer management is only simple 21-buf FIFO, it may
     * cause buffer data dirty if previous frame buffer is still holded 
     * by rendering process 
     * low propability case.
     */
     /*get pointers from VAAPI surface*/
  }
#endif

  }
  video_frame->SetTimestamp(timestamp);
  video_frame->SetDuration(duration);

  pending_output_buffers_--;
  event_handler_->ConsumeVideoFrame(video_frame, statistics);
}

void FFmpegVideoDecodeEngine::Uninitialize() {
#if defined (TOOLKIT_MEEGOTOUCH)
// _DEV2_H264_
  if((codec_context_->codec_id == CODEC_ID_H264) && hw_accel_)
  {
    /*FREE what VAAPI allcoate*/
    UnInitializeHwEngine();
  }
  
  if(shmkey){
      shmctl(shmkey, IPC_RMID, 0);
      shmkey = 0;
  }

/*
  if(subwin){
      XDestroyWindow(mDisplay, subwin);
     subwin = 0;
  }
*/

  if(mDisplay){
      XCloseDisplay(mDisplay);
      mDisplay = NULL;
  }
#endif
  if (direct_rendering_) {
    allocator_->Stop(codec_context_);
  }

  event_handler_->OnUninitializeComplete();
}

void FFmpegVideoDecodeEngine::Flush() {
  avcodec_flush_buffers(codec_context_);
  flush_pending_ = true;
  TryToFinishPendingFlush();
}

void FFmpegVideoDecodeEngine::TryToFinishPendingFlush() {
  DCHECK(flush_pending_);

  // We consider ourself flushed when there is no pending input buffers
  // and output buffers, which implies that all buffers had been returned
  // to its owner.
  if (!pending_input_buffers_ && !pending_output_buffers_) {
    // Try to finish flushing and notify pipeline.
    flush_pending_ = false;
    event_handler_->OnFlushComplete();
  }
}

void FFmpegVideoDecodeEngine::Seek() {
  // After a seek, output stream no longer considered as EOS.
  output_eos_reached_ = false;

  // The buffer provider is assumed to perform pre-roll operation.
  for (unsigned int i = 0; i < Limits::kMaxVideoFrames; ++i)
    ReadInput();

  event_handler_->OnSeekComplete();
}

void FFmpegVideoDecodeEngine::ReadInput() {
  DCHECK_EQ(output_eos_reached_, false);
  pending_input_buffers_++;
  event_handler_->ProduceVideoSample(NULL);
}

VideoFrame::Format FFmpegVideoDecodeEngine::GetSurfaceFormat() const {
  // J (Motion JPEG) versions of YUV are full range 0..255.
  // Regular (MPEG) YUV is 16..240.
  // For now we will ignore the distinction and treat them the same.
  switch (codec_context_->pix_fmt) {
    case PIX_FMT_YUV420P:
    case PIX_FMT_YUVJ420P:
      LOG(INFO) << "Supported Format : "<<  codec_context_->pix_fmt;
      return VideoFrame::YV12;
    case PIX_FMT_YUV422P:
    case PIX_FMT_YUVJ422P:
      LOG(INFO) << "Supported Format : "<<  codec_context_->pix_fmt;
      return VideoFrame::YV16;
    default:
      // TODO(scherkus): More formats here?
      return VideoFrame::INVALID;
  }
}

#if defined (TOOLKIT_MEEGOTOUCH)
// _DEV2_H264_

/*
 *  func: ReleaseBufferAndSurface 
 *  
 *  description:
 *          free a surface and return it to free_surfaces pool with unused status
 *
 */
void FFmpegVideoDecodeEngine::ReleaseBufferAndSurface(AVCodecContext * ctx, AVFrame * pic)
{
    int i = 0;
    struct VaapiSurface *surface = NULL;
    DEV2_TRACE();

    surface = (VaapiSurface *)pic->data[0];
    if(surface){
    /*status reset*/
        surface->used = 0;
    }

    //LOG(INFO) << "ReleaseBufferAndSurface:  be impl ok : idx: " << pic->linesize[3] << " addr pic "<< (int)pic;

    for(i=0; i<4; i++){ 
        pic->data[i]= NULL;
        pic->linesize[i] = 0;
    }


}

/*
 *  func: GetBufferAndSurface
 *  
 *  description:
 *          get a surface from free_surfaces pool with FIFO strategy, and assign to AVFrame 
 *          provided by FFMPEG/LIBVA wrapping code.
 *
 */

int FFmpegVideoDecodeEngine::GetBufferAndSurface(AVCodecContext * ctx, AVFrame * pic)
{
    int width = (ctx->width + 15 ) & (~15);
    int height = (ctx->height + 15) & (~15);
    struct VaapiSurface *surface = NULL;
    int i = 0, idx = 0;
    FFmpegVideoDecodeEngine* engine = reinterpret_cast<FFmpegVideoDecodeEngine*>(ctx->context_model);

    DEV2_TRACE();

    if(!engine){
    LOG(INFO) << "No this engine class \n";
    return -1;
    }

    if(ctx->pix_fmt != PIX_FMT_VAAPI_VLD){
    LOG(INFO) << "Not Supported Format \n";
    return -1;
    }

    /* Pop the least recently used free surface : FIFO*/
    assert(engine->hw_free_surfaces_[engine->hw_free_surfaces_head_index_]);
    surface = engine->hw_free_surfaces_[engine->hw_free_surfaces_head_index_];

    if (!surface){
        DEV2_TRACE();
    return -1;
    }

    while(surface->used){
    /*low percent cases: to seach whole surface lists*/
        /*all is used: use next one*/
    /*else: use unused one*/
    if(i++ < engine->hw_num_surfaces_){
            /*next one*/
        engine->hw_free_surfaces_head_index_ = (engine->hw_free_surfaces_head_index_ + 1) % engine->hw_num_surfaces_;
            surface = engine->hw_free_surfaces_[engine->hw_free_surfaces_head_index_];
    }else{
        LOG(INFO) << "GetBufferAndSurface: warning no buffer conflict " ;
        break;
    }
    
    }
    /*FIXME*/
    //LOG(INFO) << "GetBufferAndSurface:  surface num " << hw_num_surfaces_ << " idx: " << hw_free_surfaces_head_index_ << " addr pic " << (int)pic;
    surface->used = 1;

    pic->data[0] = (unsigned char *)surface;
    pic->data[1] = pic->data[2] = NULL;
    pic->data[3] = (unsigned char *)(uintptr_t)surface->id;

    /*I420, VAAPI support NV12 format.*/
    pic->linesize[0]=  width; 
    pic->linesize[1]=  width>>1; 
    pic->linesize[2]=  width>>1; 
    /*cur surface idx*/
    pic->linesize[3]=  engine->hw_free_surfaces_head_index_ ; 
    pic->type= 2;//FF_BUFFER_TYPE_USER; 

    /*pointer to next one*/
    engine->hw_free_surfaces_head_index_ = (engine->hw_free_surfaces_head_index_ + 1) % engine->hw_num_surfaces_;

    /*align to next 16*/
#if 0
    LOG(INFO) << "fmt is " << ctx->pix_fmt;
    LOG(INFO) << "w= " << width;
    LOG(INFO) << "h= " << height;
    LOG(INFO) << "Get Surface idx_ : " << pic->linesize[3];
    LOG(INFO) <<  "AVFrame: 0: " << pic->data[0]
          <<  "      id-3: " << (uintptr_t)pic->data[3];
#endif
  
    pic->opaque = NULL;
    pic->age= 256*256;

    return 0;
}

#ifdef _DEV2_DEBUG_
static const char *VAImageFormat2String(VAImageFormat *imgfmt)
{
    static char str_[5];

    DEV2_TRACE();
    *(unsigned int*)str_ = imgfmt->fourcc ;
    str_[4] = '\0';
    return str_;
}

#define DEV2SwitchCase(x)   case x: return #x;

static const char *VAProfile2String(VAProfile profile)
{
    DEV2_TRACE();
    switch (profile) {
    DEV2SwitchCase(VAProfileMPEG2Simple);
    DEV2SwitchCase(VAProfileMPEG2Main);
    DEV2SwitchCase(VAProfileMPEG4Simple);
    DEV2SwitchCase(VAProfileMPEG4AdvancedSimple);
    DEV2SwitchCase(VAProfileMPEG4Main);
    DEV2SwitchCase(VAProfileH264Baseline);
    DEV2SwitchCase(VAProfileH264Main);
    DEV2SwitchCase(VAProfileH264High);
    DEV2SwitchCase(VAProfileVC1Simple);
    DEV2SwitchCase(VAProfileVC1Main);
    DEV2SwitchCase(VAProfileVC1Advanced);
    }
    return "<Unknown VAProfile>";
}

static int ProfileSupport(VAProfile profile)
{
    DEV2_TRACE();
    if (hw_profiles_ && hw_num_profiles_ > 0) {
        int i;
        for (i = 0; i < hw_num_profiles_; i++) {
            if (hw_profiles_[i] == profile)
                return 1;
        }
    }
    return 0;
}

/* only H264
 */
static VAProfile CheckVAProfile()
{
    static const VAProfile H264_Profile[] =
        { VAProfileH264High, VAProfileH264Main, VAProfileH264Baseline, (VAProfile)-1 };

    DEV2_TRACE();

    for (int i = 0; H264_Profile[i] != -1; i++) {
        if (ProfileSupport(H264_Profile[i]))
            return H264_Profile[i];
    }
    
    return (VAProfile)-1;
}

static const char *VAEntryPoint2String(VAEntrypoint entrypoint)
{
    switch (entrypoint) {
    DEV2SwitchCase(VAEntrypointVLD);
    DEV2SwitchCase(VAEntrypointIZZ);
    DEV2SwitchCase(VAEntrypointIDCT);
    DEV2SwitchCase(VAEntrypointMoComp);
    DEV2SwitchCase(VAEntrypointDeblocking);
    }
    return "<Unknown VAEntryPoint>";
}

static int HasEntryPoint(VAEntrypoint entrypoint)
{
    if (hw_entrypoints_ && hw_num_entrypoints_ > 0) {
        int i;
        for (i = 0; i < hw_num_entrypoints_; i++) {
            if (hw_entrypoints_[i] == entrypoint)
                return 1;
        }
    }
    return 0;
}

/* only H264
 */
static int CheckVAEntryPoint()
{
    int ret = -1;

    ret = HasEntryPoint(VAEntrypointVLD);

    return ret;

}

#endif


/*
 * func: get_format callback function.
 *  
 * description: 
 *            is called to search a supported VAAPI codec for given format, 
 *          and well config it before real slice decoding
 * pix_fmt :
 *            PIX_FMT_DXVA2_VLD,
 *         PIX_FMT_VAAPI_VLD,
 *         PIX_FMT_YUV420P,
 *         PIX_FMT_NONE
 *
 */

enum PixelFormat FFmpegVideoDecodeEngine::GetFormatAndConfig(struct AVCodecContext *avctx, const enum PixelFormat *pix_fmt)
{
    
    int ret = 0;
    FFmpegVideoDecodeEngine* engine = reinterpret_cast<FFmpegVideoDecodeEngine*>(avctx->context_model);

    DEV2_TRACE();
    while (*pix_fmt != PIX_FMT_NONE ){
    if( PIX_FMT_VAAPI_VLD == *pix_fmt){
        LOG(INFO) << "found VAAPI VLD format supported ";
        //LOG(INFO) << "w= "<< avctx->width << ", h= " << avctx->height;
        /* we need to config while first time launch VAAPI engine.
         * this is a callback , which is invoked by h264.c before 
         * decoding a real slice
         */
        ret = engine->ConfigHwEngine(avctx->width , avctx->height, VAAPI_H264, avctx->refs);
        if(0 != ret){
        LOG(INFO) << "func : <ConfigHwEngine> return err " << ret;
        return PIX_FMT_NONE;
        }
        //LOG(INFO) << "func : <ConfigHwEngine> return ok ";

        return *pix_fmt;
    }
    LOG(INFO) << "Unsupported format "<< *pix_fmt << "\n";
        pix_fmt ++;
    }
    return pix_fmt[0];
}

/*
 * func: initialization function.
 *  
 * description: 
 *            is called to do basic initilization for VAAPI calling. 
 *            and also query hw capability for decoding.
 *
 */


int FFmpegVideoDecodeEngine::InitializeHwEngine(void)
{
    VADisplayAttribute *display_attrs;
    VAStatus status;
    int va_major_version, va_minor_version;
    int i, max_image_formats, max_subpic_formats, max_profiles;
    int num_display_attrs, max_display_attrs;

    DEV2_TRACE();
    
    hw_context_ = (hw_context*)calloc(1, sizeof(hw_context));
    if (!hw_context_){
    DEV2_TRACE();
        return -1;
    }

#if 0
    /*enable for player_x11 to use player_x11 display*/
    mDisplay = g_display;
#else
    mDisplay = XOpenDisplay(":0.0");

#endif
    hw_context_->display = vaGetDisplay(mDisplay);
    if (!hw_context_->display)
        return -1;
    
    /*vaInitialize*/
#ifdef _DEV2_DEBUG_
    LOG(INFO)<< " InitializeHwEngine(): VA display " << hw_context_->display;
#endif
    status = vaInitialize(hw_context_->display, &va_major_version, &va_minor_version);
    if (CheckStatus(status, "vaInitialize()")){
        DEV2_TRACE();
        return -1;
    }

#ifdef _DEV2_DEBUG_
    /*Query Image Formats*/
    LOG(INFO) <<  " InitializeHwEngine(): VA API version " << va_major_version << va_minor_version;
    max_image_formats = vaMaxNumImageFormats(hw_context_->display);
    hw_image_formats_ = (VAImageFormat*)calloc(max_image_formats, sizeof(VAImageFormat));
    if (!hw_image_formats_){
        return -1;
    }
    status = vaQueryImageFormats(hw_context_->display, hw_image_formats_, &hw_num_image_formats_);
    if (CheckStatus(status, "vaQueryImageFormats()")){
        DEV2_TRACE();
        return -1;
    }

    LOG(INFO) <<  " InitializeHwEngine():" << hw_num_image_formats_ << "image formats available";

    for (i = 0; i < hw_num_image_formats_; i++){
        LOG(INFO) << " VAIMAGE FORMAT : " << VAImageFormat2String(&hw_image_formats_[i]);
    }

    /*Query Sub Picture Formats.*/
    max_subpic_formats = (int)vaMaxNumSubpictureFormats(hw_context_->display);
    hw_subpic_formats_ = (VAImageFormat*)calloc(max_subpic_formats, sizeof(VAImageFormat));
    if (!hw_subpic_formats_){
        return -1;
    }
    hw_subpic_flags_ = (unsigned int*)calloc(max_subpic_formats, sizeof(unsigned int));
    if (!hw_subpic_flags_){
        return -1;
    }

    status = vaQuerySubpictureFormats(hw_context_->display, hw_subpic_formats_, hw_subpic_flags_, (unsigned int*)&hw_num_subpic_formats_);
    if (CheckStatus(status, "vaQuerySubpictureFormats()")){
        hw_num_subpic_formats_ = 0; /* XXX: don't error out for IEGD */
    }

    LOG(INFO) <<  " InitializeHwEngine(): "<< hw_num_subpic_formats_ << " Subpicture Formats Available\n" ;
    for (i = 0; i < hw_num_subpic_formats_; i++){
        LOG(INFO) << " Subpic Formats " << VAImageFormat2String(&hw_subpic_formats_[i]) << " flags 0x"  << hw_subpic_flags_[i];
    }

    DEV2FreeCalloc(hw_subpic_formats_);
 
    /*Query Config Profiles*/
    max_profiles = vaMaxNumProfiles(hw_context_->display);
    hw_profiles_ = (VAProfile*)calloc(max_profiles, sizeof(*hw_profiles_));
    if (!hw_profiles_)
        return -1;

    status = vaQueryConfigProfiles(hw_context_->display, hw_profiles_, &hw_num_profiles_);
    if (CheckStatus(status, "vaQueryConfigProfiles()")){
        DEV2_TRACE();
        return -1;
    }
    LOG(INFO) <<  "InitializeHwEngine(): " << hw_num_profiles_ << " Profiles Available" ;

    for (i = 0; i < hw_num_profiles_; i++){
        LOG(INFO) << " Profiles " << VAProfile2String(hw_profiles_[i]);
    }


    /*Query Displayer Attributes*/
    max_display_attrs = vaMaxNumDisplayAttributes(hw_context_->display);
    display_attrs = (VADisplayAttribute *)calloc(max_display_attrs, sizeof(*display_attrs));

    if (display_attrs) {
        num_display_attrs = 0;
        status = vaQueryDisplayAttributes(hw_context_->display, display_attrs, &num_display_attrs);
        if (CheckStatus(status, "vaQueryDisplayAttributes()") == 0) {
            LOG(INFO) <<  " vaQueryDisplayAttributes(): " << num_display_attrs ;
#if 0
            for (i = 0; i < num_display_attrs; i++) {
                VADisplayAttribute *attr;
                switch (display_attrs[i].type) {
                case VADisplayAttribBrightness:
                    attr = &hw_equalizer_.brightness;
                    break;
                case VADisplayAttribContrast:
                    attr = &hw_equalizer_.contrast;
                    break;
                case VADisplayAttribHue:
                    attr = &hw_equalizer_.hue;
                    break;
                case VADisplayAttribSaturation:
                    attr = &hw_equalizer_.saturation;
                    break;
                default:
                    attr = NULL;
                    break;
                }
                if (attr)
                    *attr = display_attrs[i];
            }
#endif
        }
        free(display_attrs);
    }
  
#endif

    return 0;

}

/*
 * func: config hw engine
 *  
 * description:
 *       create surfaces , config, context, only H264 here, so no "format" parameters
 *
 */

int FFmpegVideoDecodeEngine::ConfigHwEngine(uint32_t width, uint32_t height, uint32_t format, uint32_t refs)
{
    VAConfigAttrib attrib;
    VAStatus status;
    int i, j,  max_entrypoints ;
    struct VaapiSurface *surface = NULL;
    VAEntrypoint entrypoint = VAEntrypointVLD;
    VAProfile profile = VAProfileH264High;

    /*reset */
    //hw_num_surfaces_ = NUM_VIDEO_SURFACES_H264;
    hw_num_surfaces_ = ((refs + 5) < NUM_VIDEO_SURFACES_H264) ? (refs + 5) : NUM_VIDEO_SURFACES_H264;

    hw_free_surfaces_head_index_ = 0;
    hw_free_surfaces_[hw_num_surfaces_] = NULL;

    /*FIXED : H264 only, alignment input is needed, I do not why add limitation to this API??*/
    width = (width + 3) & (~3);

    /* Create video surfaces */
    status = vaCreateSurfaces(hw_context_->display, width, height, VA_RT_FORMAT_YUV420,
                              hw_num_surfaces_, hw_surface_ids_);
    if (CheckStatus(status, "vaCreateSurfaces():")){
    DEV2_TRACE();
    return -1;
    }

    for(i = 0; i < (hw_num_surfaces_); i ++){

    /*surface structure used by wrapping code*/
        surface = (VaapiSurface *)calloc(1, sizeof(*surface));
    if(surface == NULL){
        DEV2_TRACE();
        return -1;
    }
    
    hw_free_surfaces_[i] = surface;
    surface->id = hw_surface_ids_[i];
    surface->used = 0;
    surface->image.image_id = VA_INVALID_ID;
    surface->image.buf = VA_INVALID_ID;
    }
    /*indicate surface allocate/unallocate status*/
    hw_surface_ids_[hw_num_surfaces_] = 1;
    hw_free_surfaces_[hw_num_surfaces_] = (VaapiSurface *)0x1;

    /* Allocate VA images */
#ifdef _DEV2_DEBUG_
    /*IMG FMT IS VAAPI*/
    LOG(INFO) <<  " FFmpegVideoDecodeEngine::ConfigHwEngine , all surface:"<< hw_num_surfaces_;

    /* Check entry-point (only VLD for now) */
    max_entrypoints = vaMaxNumEntrypoints(hw_context_->display);
    hw_entrypoints_ = (VAEntrypoint*)calloc(max_entrypoints, sizeof(*hw_entrypoints_));
    if (!hw_entrypoints_){
    DEV2_TRACE();
        return -1;
    }

    /*check profile*/
    profile = CheckVAProfile();
    if(-1 == (int)profile){
    DEV2_TRACE();
        return -1;
    }

    status = vaQueryConfigEntrypoints(hw_context_->display, profile,
                                      hw_entrypoints_, &hw_num_entrypoints_);
    if (CheckStatus(status, "vaQueryConfigEntrypoints()")){
    DEV2_TRACE();
        return -1;
    }

    LOG(INFO) << " ConfigHwEngine("<< VAProfile2String(profile) << "): "<< hw_num_entrypoints_ << " entrypoints available";
    for (i = 0; i < hw_num_entrypoints_; i++){
        LOG(INFO) << "  num entrypoints ", VAEntryPoint2String(hw_entrypoints_[i]);
    }
#endif

    /* Config VA HW , profile, entrypoint, and render format--------------- */
    /* Check chroma format (only 4:2:0 for now) */
    attrib.type = VAConfigAttribRTFormat;
    status = vaGetConfigAttributes(hw_context_->display, profile, entrypoint, &attrib, 1);
    if (CheckStatus(status, "vaGetConfigAttributes()")){
    DEV2_TRACE();
        return -1;
    }
#ifdef _DEV2_DEBUG_
    LOG(INFO) << " ConfigHwEngine: Render Format:" << attrib.value ;
#endif
    if ((attrib.value & VA_RT_FORMAT_YUV420) == 0){
    DEV2_TRACE();
        return -1;
    }

    /* Create a configuration for the decode pipeline for h264*/
    status = vaCreateConfig(hw_context_->display, (VAProfile)profile, entrypoint, &attrib, 1, &hw_context_->config_id);
    if (CheckStatus(status, "vaCreateConfig()")){
    DEV2_TRACE();
        return -1;
    }

    /* Create a context for the decode pipeline */
    status = vaCreateContext(hw_context_->display, hw_context_->config_id,
                             width, height, VA_PROGRESSIVE,
                             hw_surface_ids_, hw_num_surfaces_,
                             &hw_context_->context_id);
    if (CheckStatus(status, "vaCreateContext()")){
    DEV2_TRACE();
        return -1;
    }

    return 0;
}

/*
 * func: UnInitializeHwEngine
 *  
 * description:
 *       free hw resource, such as context, surfaces, images,config and etc. 
 *
 */

void FFmpegVideoDecodeEngine::UnInitializeHwEngine(void)
{
    int i;

    DEV2_TRACE();

    if (hw_context_ && hw_context_->context_id) {
        vaDestroyContext(hw_context_->display, hw_context_->context_id);
        hw_context_->context_id = 0;
    }

    if (hw_free_surfaces_[hw_num_surfaces_]) {
    struct VaapiSurface *surface;
        for (i = 0; i < hw_num_surfaces_; i++) {
            if (!hw_free_surfaces_[i])
                continue;
        /*free VA surfaces*/
            if (hw_free_surfaces_[i]->image.image_id != VA_INVALID_ID) {
                vaDestroyImage(hw_context_->display,
                               hw_free_surfaces_[i]->image.image_id);
                hw_free_surfaces_[i]->image.image_id = VA_INVALID_ID;
            }
        /*free VA surfaces structure outside of VAAPI*/
            free(hw_free_surfaces_[i]);
            hw_free_surfaces_[i] = NULL;
        }

    /*freed by calloc*/
        hw_free_surfaces_head_index_ = 0;
    hw_free_surfaces_[hw_num_surfaces_] = NULL;
    }


    if (hw_surface_ids_[hw_num_surfaces_]) {
        vaDestroySurfaces(hw_context_->display, hw_surface_ids_, hw_num_surfaces_);
        hw_surface_ids_[hw_num_surfaces_] = 0;
        hw_num_surfaces_ = 0;
    }

    if (hw_context_ && hw_context_->config_id) {
        vaDestroyConfig(hw_context_->display, hw_context_->config_id);
        hw_context_->config_id = 0;
    }

    DEV2FreeCalloc(hw_context_);

#ifdef _DEV2_DEBUG_
    DEV2FreeCalloc(hw_image_formats_);
    DEV2FreeCalloc(hw_subpic_flags_);
    DEV2FreeCalloc(hw_profiles_);
    DEV2FreeCalloc(hw_entrypoints_);

    LOG(INFO) << "UnInitializeHwEngine:  END";
#endif

}

#endif
}  // namespace media

// Disable refcounting for this object because this object only lives
// on the video decoder thread and there's no need to refcount it.
DISABLE_RUNNABLE_METHOD_REFCOUNT(media::FFmpegVideoDecodeEngine);
