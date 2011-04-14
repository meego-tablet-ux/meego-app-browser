// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/glue/media/video_renderer_impl.h"

#include "media/base/video_frame.h"
#include "media/base/yuv_convert.h"
#include "webkit/glue/webmediaplayer_impl.h"

#if defined (TOOLKIT_MEEGOTOUCH)
/*_DEV2_H264_*/
#include <sys/syscall.h>
/*optimize render of LIBVA H264*/
/*_DEV2_H264_*/
#define _DEV2_H264_

#include <va/va.h>
#include <va/va_x11.h>
/*XA_WINDOW*/
#include <X11/Xatom.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>

#include <X11/X.h>
extern Window subwin;
//extern double GetTick(void);
extern int shmkey;
/*_DEV2_H264_*/
#endif

namespace webkit_glue {

VideoRendererImpl::VideoRendererImpl(bool pts_logging)
    : last_converted_frame_(NULL),
      pts_logging_(pts_logging) {
}

VideoRendererImpl::~VideoRendererImpl() {}

bool VideoRendererImpl::OnInitialize(media::VideoDecoder* decoder) {
  video_size_.SetSize(width(), height());
  bitmap_.setConfig(SkBitmap::kARGB_8888_Config, width(), height());
  if (bitmap_.allocPixels(NULL, NULL)) {
    bitmap_.eraseRGB(0x00, 0x00, 0x00);
    return true;
  }

  NOTREACHED();
  return false;
}

void VideoRendererImpl::OnStop(media::FilterCallback* callback) {
  if (callback) {
    callback->Run();
    delete callback;
  }
}

void VideoRendererImpl::OnFrameAvailable() {
  proxy_->Repaint();
}

void VideoRendererImpl::SetWebMediaPlayerImplProxy(
    WebMediaPlayerImpl::Proxy* proxy) {
  proxy_ = proxy;
}

void VideoRendererImpl::SetRect(const gfx::Rect& rect) {
}

// This method is always called on the renderer's thread.
void VideoRendererImpl::Paint(SkCanvas* canvas,
                              const gfx::Rect& dest_rect) {
  scoped_refptr<media::VideoFrame> video_frame;
  GetCurrentFrame(&video_frame);
  if (!video_frame) {
    SkPaint paint;
    paint.setColor(SK_ColorBLACK);
    canvas->drawRectCoords(
        static_cast<float>(dest_rect.x()),
        static_cast<float>(dest_rect.y()),
        static_cast<float>(dest_rect.right()),
        static_cast<float>(dest_rect.bottom()),
        paint);
  } else {
    if (CanFastPaint(canvas, dest_rect)) {
      FastPaint(video_frame, canvas, dest_rect);
    } else {
      SlowPaint(video_frame, canvas, dest_rect);
    }

    // Presentation timestamp logging is primarily used to measure performance
    // on low-end devices.  When profiled on an Intel Atom N280 @ 1.66GHz this
    // code had a ~63 microsecond perf hit when logging to a file (not stdout),
    // which is neglible enough for measuring playback performance.
    if (pts_logging_)
      VLOG(1) << "pts=" << video_frame->GetTimestamp().InMicroseconds();
  }

  PutCurrentFrame(video_frame);
}

void VideoRendererImpl::GetCurrentFrame(
    scoped_refptr<media::VideoFrame>* frame_out) {
  VideoRendererBase::GetCurrentFrame(frame_out);
}

void VideoRendererImpl::PutCurrentFrame(
    scoped_refptr<media::VideoFrame> frame) {
  VideoRendererBase::PutCurrentFrame(frame);
}

// CanFastPaint is a helper method to determine the conditions for fast
// painting. The conditions are:
// 1. No skew in canvas matrix.
// 2. No flipping nor mirroring.
// 3. Canvas has pixel format ARGB8888.
// 4. Canvas is opaque.
// TODO(hclam): The fast paint method should support flipping and mirroring.
// Disable the flipping and mirroring checks once we have it.
bool VideoRendererImpl::CanFastPaint(SkCanvas* canvas,
                                     const gfx::Rect& dest_rect) {
  // Fast paint does not handle opacity value other than 1.0. Hence use slow
  // paint if opacity is not 1.0. Since alpha = opacity * 0xFF, we check that
  // alpha != 0xFF.
  //
  // Additonal notes: If opacity = 0.0, the chrome display engine does not try
  // to render the video. So, this method is never called. However, if the
  // opacity = 0.0001, alpha is again 0, but the display engine tries to render
  // the video. If we use Fast paint, the video shows up with opacity = 1.0.
  // Hence we use slow paint also in the case where alpha = 0. It would be ideal
  // if rendering was never called even for cases where alpha is 0. Created
  // bug 48090 for this.
  SkCanvas::LayerIter layer_iter(canvas, false);
  SkColor sk_color = layer_iter.paint().getColor();
  SkAlpha sk_alpha = SkColorGetA(sk_color);
  if (sk_alpha != 0xFF) {
    return false;
  }

  const SkMatrix& total_matrix = canvas->getTotalMatrix();
  // Perform the following checks here:
  // 1. Check for skewing factors of the transformation matrix. They should be
  //    zero.
  // 2. Check for mirroring and flipping. Make sure they are greater than zero.
  if (SkScalarNearlyZero(total_matrix.getSkewX()) &&
      SkScalarNearlyZero(total_matrix.getSkewY()) &&
      total_matrix.getScaleX() > 0 &&
      total_matrix.getScaleY() > 0) {
    // Get the properties of the SkDevice and the clip rect.
    SkDevice* device = canvas->getDevice();

    // Get the boundary of the device.
    SkIRect device_rect;
    device->getBounds(&device_rect);

    // Get the pixel config of the device.
    const SkBitmap::Config config = device->config();
    // Get the total clip rect associated with the canvas.
    const SkRegion& total_clip = canvas->getTotalClip();

    SkIRect dest_irect;
    TransformToSkIRect(canvas->getTotalMatrix(), dest_rect, &dest_irect);

    if (config == SkBitmap::kARGB_8888_Config && device->isOpaque() &&
        device_rect.contains(total_clip.getBounds())) {
      return true;
    }
  }

  return false;
}

void VideoRendererImpl::SlowPaint(media::VideoFrame* video_frame,
                                  SkCanvas* canvas,
                                  const gfx::Rect& dest_rect) {
  // 1. Convert YUV frame to RGB.
  base::TimeDelta timestamp = video_frame->GetTimestamp();
  if (video_frame != last_converted_frame_ ||
      timestamp != last_converted_timestamp_) {
    last_converted_frame_ = video_frame;
    last_converted_timestamp_ = timestamp;
    DCHECK(video_frame->format() == media::VideoFrame::YV12 ||
           video_frame->format() == media::VideoFrame::YV16);
    DCHECK(video_frame->stride(media::VideoFrame::kUPlane) ==
           video_frame->stride(media::VideoFrame::kVPlane));
    DCHECK(video_frame->planes() == media::VideoFrame::kNumYUVPlanes);
    bitmap_.lockPixels();
    media::YUVType yuv_type =
        (video_frame->format() == media::VideoFrame::YV12) ?
        media::YV12 : media::YV16;
    media::ConvertYUVToRGB32(video_frame->data(media::VideoFrame::kYPlane),
                             video_frame->data(media::VideoFrame::kUPlane),
                             video_frame->data(media::VideoFrame::kVPlane),
                             static_cast<uint8*>(bitmap_.getPixels()),
                             video_frame->width(),
                             video_frame->height(),
                             video_frame->stride(media::VideoFrame::kYPlane),
                             video_frame->stride(media::VideoFrame::kUPlane),
                             bitmap_.rowBytes(),
                             yuv_type);
    bitmap_.unlockPixels();
  }

  // 2. Paint the bitmap to canvas.
  SkMatrix matrix;
  matrix.setTranslate(static_cast<SkScalar>(dest_rect.x()),
                      static_cast<SkScalar>(dest_rect.y()));
  if (dest_rect.width()  != video_size_.width() ||
      dest_rect.height() != video_size_.height()) {
    matrix.preScale(SkIntToScalar(dest_rect.width()) /
                    SkIntToScalar(video_size_.width()),
                    SkIntToScalar(dest_rect.height()) /
                    SkIntToScalar(video_size_.height()));
  }
  SkPaint paint;
  paint.setFlags(SkPaint::kFilterBitmap_Flag);
  canvas->drawBitmapMatrix(bitmap_, matrix, &paint);
}


#if defined (TOOLKIT_MEEGOTOUCH)
/*_DEV2_H264_*/
/*
  Paint VAAPI H264 to Subwin or Share memory.
  it adopts automatically on context.
*/

void VideoRendererImpl::H264Paint(WebMediaPlayerImpl::Proxy* proxy, media::VideoFrame* video_frame, int dst_w, int dst_h, uint8 * pDst, int stride)
{

    int w = video_frame->width();
    int h = video_frame->height();
  
    int w_ = dst_w;
   int h_ = dst_h;
    WebMediaPlayerImpl * wp = proxy->GetMediaPlayer();
    
    void *hw_ctx_display = (void*)video_frame->data_[2];
    VASurfaceID surface_id = (VASurfaceID)video_frame->idx_;
   VAStatus status;
    Display *dpy = (Display*) video_frame->data_[0];

    if(wp->paused() && subwin){
        //("skip paint for chromium default render\n");
       //("paint %d \n", syscall(__NR_gettid));
       return;
    }
   /*if paused, but no subwin, it means preload*/
    //("h264 p in, surface id :%d, win: %d, last frm: %d\n", surface_id, subwin, proxy->last_frame_);


if(/*(wp->paused() == 0) ||*/ subwin){
    /*if not paused , just render directly in full screen mode.*/
    //base::TimeDelta time = video_frame->GetDuration();
    return;

#if 0
    int w_ = WIDTH, h_ = HEIGHT ;
    /*resize of not while menu is enabled*/
    if(proxy->menu_on_){
        /*moving label*/
        h_ -= 84;
    }
   status = vaPutSurface(hw_ctx_display, surface_id, subwin,
                              0, 0, w, h, /*src*/
                              0, 0, w_, h_, /*dst*/
                              NULL, 0,
                              VA_FRAME_PICTURE | VA_SRC_BT601);
    if(proxy->menu_on_ && subwin){
        proxy->PaintControlBar();
    }
#endif

#if 0
}else{
    /*if paused , just copy to shm, and .*/
    //("render default\n");

    Display *dTmp = (Display *)video_frame->data_[0];

    int screen = DefaultScreen(dTmp);
    int root_window = RootWindow(dTmp, screen);
    Window wTmp = root_window;

    XWindowAttributes attr;
   XGetWindowAttributes (dTmp, wTmp, &attr);
    // Creates a pixmap and uploads from the XImage.
    unsigned long pixmap = XCreatePixmap(dTmp,
                                         wTmp,
                                         w,
                                         h,
                                         attr.depth);
    
    /*CC and Resize*/
    status = vaPutSurface(hw_ctx_display, surface_id, pixmap,
                             0, 0, w, h, /*src*/
                              0, 0, w_, h_, /*dst*/
                              NULL, 0,
                              VA_FRAME_PICTURE | VA_SRC_BT601);
    if(proxy->menu_on_ && subwin){
        proxy->PaintControlBar();
    }
#endif

}else{
    /*if paused , just copy to shm, and .*/
    //("render default\n");

    Display *dTmp = (Display *)video_frame->data_[0];

    int screen = DefaultScreen(dTmp);
    int root_window = RootWindow(dTmp, screen);
    Window wTmp = root_window;

   XWindowAttributes attr;
    XGetWindowAttributes (dTmp, wTmp, &attr);
    // Creates a pixmap and uploads from the XImage.
    unsigned long pixmap = XCreatePixmap(dTmp,
                                         wTmp,
                                         w,
                                        h,
                                         attr.depth);
    
    /*CC and Resize*/
    status = vaPutSurface(hw_ctx_display, surface_id, pixmap,
                              0, 0, w, h, /*src*/
                              0, 0, w_, h_, /*dst*/
                              NULL, 0,
                              VA_FRAME_PICTURE );

   XImage * mXImage;

//#define _Shared_
#ifdef _Shared_
    static int shared = 0;
    static char* pbuffer = NULL;
    /*if w_, h_ is changed, shm has to be reallocated.*/
    if(!shared){
       shared = 1;
       shmkey = shmget(IPC_PRIVATE, 1280*720*4, 0666);
      pbuffer = (char*)shmat(shmkey, NULL /* desired address */, 0 /* flags */);
    }
        
    XShmSegmentInfo shminfo = {0};

    mXImage = XShmCreateImage(dTmp, DefaultVisual(dTmp, DefaultScreen(dTmp)), 24, ZPixmap, NULL, &shminfo, w_, h_);
 // ("mXImage->bytes_per_line: %d, mXImage->height: %d\n", mXImage->bytes_per_line, mXImage->height);

    shminfo.shmaddr = mXImage->data = pbuffer;
   shminfo.shmid = shmkey;

    if(!XShmAttach(dTmp, &shminfo)){
       LOG(ERROR) << "XShmAttach Error" ;
    };

    if(! XShmGetImage(dTmp, pixmap, mXImage, 0, 0, AllPlanes) ){
        LOG(ERROR) << "XShmGetImage Error" ;
    } ;

#else
    mXImage = XGetImage(dTmp, pixmap, 0, 0, w_, h_, AllPlanes, ZPixmap);
#endif
    //("mXImage: %x, %x, %x, %x, %x\n", mXImage->data, mXImage->bytes_per_line, mXImage->width, mXImage->height, mXImage->depth);

    if(proxy->last_frame_){
        memset(mXImage->data, 0, w_*h_*4);
        proxy->last_frame_ = 0;
    }
    
    int k = 0;
   char *pSrc = mXImage->data;
    //("stride : %d, w.h: %d.%d\n", mXImage->bytes_per_line, w_, h_);

    /*No Memory Share between two process, only Copy*/
   for( k = 0; k < mXImage->height; k ++){
       memcpy(pDst, pSrc, mXImage->bytes_per_line);
        //memset(pDst, 0, mXImage->bytes_per_line);
       pDst += stride;
       pSrc += mXImage->bytes_per_line;
    }

    XFreePixmap(dTmp, pixmap);
}/*not full screen ?*/

    return;
}

#endif



void VideoRendererImpl::FastPaint(media::VideoFrame* video_frame,
                                  SkCanvas* canvas,
                                  const gfx::Rect& dest_rect) {
  DCHECK(video_frame->format() == media::VideoFrame::YV12 ||
         video_frame->format() == media::VideoFrame::YV16);
  DCHECK(video_frame->stride(media::VideoFrame::kUPlane) ==
         video_frame->stride(media::VideoFrame::kVPlane));
  DCHECK(video_frame->planes() == media::VideoFrame::kNumYUVPlanes);
  const SkBitmap& bitmap = canvas->getDevice()->accessBitmap(true);
  media::YUVType yuv_type = (video_frame->format() == media::VideoFrame::YV12) ?
                            media::YV12 : media::YV16;
  int y_shift = yuv_type;  // 1 for YV12, 0 for YV16.

  // Create a rectangle backed by SkScalar.
  SkRect scalar_dest_rect;
  scalar_dest_rect.iset(dest_rect.x(), dest_rect.y(),
                        dest_rect.right(), dest_rect.bottom());

  // Transform the destination rectangle to local coordinates.
  const SkMatrix& local_matrix = canvas->getTotalMatrix();
  SkRect local_dest_rect;
  local_matrix.mapRect(&local_dest_rect, scalar_dest_rect);

  // After projecting the destination rectangle to local coordinates, round
  // the projected rectangle to integer values, this will give us pixel values
  // of the rectangle.
  SkIRect local_dest_irect, local_dest_irect_saved;
  local_dest_rect.round(&local_dest_irect);
  local_dest_rect.round(&local_dest_irect_saved);

  // Only does the paint if the destination rect intersects with the clip
  // rect.
  if (local_dest_irect.intersect(canvas->getTotalClip().getBounds())) {
    // At this point |local_dest_irect| contains the rect that we should draw
    // to within the clipping rect.

    // Calculate the address for the top left corner of destination rect in
    // the canvas that we will draw to. The address is obtained by the base
    // address of the canvas shifted by "left" and "top" of the rect.
    uint8* dest_rect_pointer = static_cast<uint8*>(bitmap.getPixels()) +
        local_dest_irect.fTop * bitmap.rowBytes() +
        local_dest_irect.fLeft * 4;

    // Project the clip rect to the original video frame, obtains the
    // dimensions of the projected clip rect, "left" and "top" of the rect.
    // The math here are all integer math so we won't have rounding error and
    // write outside of the canvas.
    // We have the assumptions of dest_rect.width() and dest_rect.height()
    // being non-zero, these are valid assumptions since finding intersection
    // above rejects empty rectangle so we just do a DCHECK here.
    DCHECK_NE(0, dest_rect.width());
    DCHECK_NE(0, dest_rect.height());
    size_t frame_clip_width = local_dest_irect.width() *
        video_frame->width() / local_dest_irect_saved.width();
    size_t frame_clip_height = local_dest_irect.height() *
        video_frame->height() / local_dest_irect_saved.height();

    // Project the "left" and "top" of the final destination rect to local
    // coordinates of the video frame, use these values to find the offsets
    // in the video frame to start reading.
    size_t frame_clip_left =
        (local_dest_irect.fLeft - local_dest_irect_saved.fLeft) *
        video_frame->width() / local_dest_irect_saved.width();
    size_t frame_clip_top =
        (local_dest_irect.fTop - local_dest_irect_saved.fTop) *
        video_frame->height() / local_dest_irect_saved.height();

    // Use the "left" and "top" of the destination rect to locate the offset
    // in Y, U and V planes.
    size_t y_offset = video_frame->stride(media::VideoFrame::kYPlane) *
        frame_clip_top + frame_clip_left;
    // For format YV12, there is one U, V value per 2x2 block.
    // For format YV16, there is one u, V value per 2x1 block.
    size_t uv_offset = (video_frame->stride(media::VideoFrame::kUPlane) *
                        (frame_clip_top >> y_shift)) + (frame_clip_left >> 1);
    uint8* frame_clip_y =
        video_frame->data(media::VideoFrame::kYPlane) + y_offset;
    uint8* frame_clip_u =
        video_frame->data(media::VideoFrame::kUPlane) + uv_offset;
    uint8* frame_clip_v =
        video_frame->data(media::VideoFrame::kVPlane) + uv_offset;
    bitmap.lockPixels();

#if defined (TOOLKIT_MEEGOTOUCH)
// _DEV2_H264_

/*
    ("src: %d.%d, dst: %d.%d, row: %d. left: %d, top: %d\n", video_frame->width(),video_frame->height(),local_dest_irect.width(),
                       local_dest_irect.height(), bitmap.rowBytes(), 
                      local_dest_irect.fLeft , local_dest_irect.fTop);
*/

  //("Painting\n");
 if(video_frame->data_[1] == (uint8_t*)0x264){

     /*H264 Paint, Directly or not*/
     H264Paint(proxy_, video_frame, local_dest_irect.width(), local_dest_irect.height(),dest_rect_pointer,bitmap.rowBytes());

    return ;

  }/*H.264 Painting*/

#endif

    // TODO(hclam): do rotation and mirroring here.
    // TODO(fbarchard): switch filtering based on performance.
    media::ScaleYUVToRGB32(frame_clip_y,
                           frame_clip_u,
                           frame_clip_v,
                           dest_rect_pointer,
                           frame_clip_width,
                           frame_clip_height,
                           local_dest_irect.width(),
                           local_dest_irect.height(),
                           video_frame->stride(media::VideoFrame::kYPlane),
                           video_frame->stride(media::VideoFrame::kUPlane),
                           bitmap.rowBytes(),
                           yuv_type,
                           media::ROTATE_0,
                           media::FILTER_BILINEAR);
    bitmap.unlockPixels();
  }
}

void VideoRendererImpl::TransformToSkIRect(const SkMatrix& matrix,
                                           const gfx::Rect& src_rect,
                                           SkIRect* dest_rect) {
    // Transform destination rect to local coordinates.
    SkRect transformed_rect;
    SkRect skia_dest_rect;
    skia_dest_rect.iset(src_rect.x(), src_rect.y(),
                        src_rect.right(), src_rect.bottom());
    matrix.mapRect(&transformed_rect, skia_dest_rect);
    transformed_rect.round(dest_rect);
}

}  // namespace webkit_glue
