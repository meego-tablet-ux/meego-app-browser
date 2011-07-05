// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/render_messages.h"
#include "content/renderer/render_thread.h"

#include "webkit/glue/media/video_renderer_impl.h"

#include "media/base/video_frame.h"
#include "media/base/yuv_convert.h"
#include "webkit/glue/webmediaplayer_impl.h"

//#define CONTROL_UI_DEBUG

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
/*_DEV2_H264_*/
#endif

//#define DEF_ENABLE_DOUBLE_PIXMAP

namespace webkit_glue {

VideoRendererImpl::VideoRendererImpl(bool pts_logging, int routing_id)
    : last_converted_frame_(NULL),
      pts_logging_(pts_logging),
      routing_id_(routing_id),
      direct_paint_enabled_(false),
      direct_paint_inited_(false),
      direct_paint_init_tried_(false),
      paint_reset_(false),
      is_overlapped_(true)
{
  video_double_pixmap_[0] = 0;
  video_double_pixmap_[1] = 0;
}

VideoRendererImpl::~VideoRendererImpl() {
  ExitDirectPaint();
  }

void VideoRendererImpl::Send(IPC::Message* msg) {
  DCHECK(routing_id_ != MSG_ROUTING_NONE);
  DCHECK(routing_id_ == msg->routing_id());

  bool result = RenderThread::current()->Send(msg);
  LOG_IF(ERROR, !result) << "RenderThread::current()->Send(msg) failed";
}

bool VideoRendererImpl::DirectPaintInited()
{
  return ((direct_paint_init_tried_ == false) ? false : direct_paint_inited_);
}

void VideoRendererImpl::EnableDirectPaint(bool enable)
{
  if (direct_paint_enabled_ != enable)
  {
    direct_paint_enabled_ = enable;
    if (DirectPaintInited() == true)
    {
      Send(new ViewHostMsg_EnableVideoWidget(routing_id_, reinterpret_cast<unsigned int>(this), enable));
    }
  }
}

void VideoRendererImpl::FreeVideoPixmap(bool notify)
{
  if (notify == false)
  {
    Display* display = (Display*)video_display_;
    if (video_double_pixmap_[0] != 0)
    {
      XFreePixmap(display, video_double_pixmap_[0]);

    }
#if defined(DEF_ENABLE_DOUBLE_PIXMAP)
    if (video_double_pixmap_[1] != 0)
    {
      XFreePixmap(display, video_double_pixmap_[1]);
    }
#endif
  }
  else
  {
    Send(new ViewHostMsg_UpdateVideoWidget(routing_id_, reinterpret_cast<unsigned int>(this), 0, video_rect_));
    Send(new ViewHostMsg_DestroyVideoWidgetPixmap(routing_id_, reinterpret_cast<unsigned int>(this), video_double_pixmap_[0]));
#if defined(DEF_ENABLE_DOUBLE_PIXMAP)
    Send(new ViewHostMsg_DestroyVideoWidgetPixmap(routing_id_, reinterpret_cast<unsigned int>(this), video_double_pixmap_[1]));
#endif
  }

  video_double_pixmap_[0] = 0;
#if defined(DEF_ENABLE_DOUBLE_PIXMAP)
  video_double_pixmap_[1] = 0;
#endif
}

int VideoRendererImpl::GetVideoPixmap(bool create)
{
  if (create == true)
  {
    Display *display = (Display*)video_display_;
    int screen = DefaultScreen(display);
    int root_window = RootWindow(display, screen);
    Window root = root_window;
    XWindowAttributes attr;
    XGetWindowAttributes (display, root, &attr);

    video_double_pixmap_[0] = XCreatePixmap(display, root, video_rect_.width(), video_rect_.height(), attr.depth);
#if defined(DEF_ENABLE_DOUBLE_PIXMAP)
    video_double_pixmap_[1] = XCreatePixmap(display, root, video_rect_.width(), video_rect_.height(), attr.depth);
    if ((video_double_pixmap_[0] == 0) || (video_double_pixmap_[1] == 0))
#else
    if (video_double_pixmap_[0] == 0)
#endif
    {
      FreeVideoPixmap(false);
    }
    video_seq_ = 0;
  }
#if defined(DEF_ENABLE_DOUBLE_PIXMAP)
  video_seq_ ++;
  return (video_double_pixmap_[video_seq_ & 0x01]);
#else
  return (video_double_pixmap_[0]);
#endif
}

void VideoRendererImpl::InitDirectPaint(const gfx::Rect& dest_rect)
{
#if defined(CONTROL_UI_DEBUG)
  if (direct_paint_init_tried_ == true)
  {
    return;
  }

  direct_paint_inited_ = true;
  direct_paint_init_tried_ = true;
  video_rect_ = dest_rect;
#else
  bool size_changed = (video_rect_.width() != dest_rect.width()) || (video_rect_.height() != dest_rect.height());
  video_rect_ = dest_rect;
  // We only try init once.
  if (direct_paint_init_tried_ == true)
  {
    if (direct_paint_inited_ == false)
    {
      return;
    }

    // If pixmap size changed, update them
    if (size_changed == true)
    {
      FreeVideoPixmap(true);
      if (GetVideoPixmap(true) == 0)
      {
        ExitDirectPaint();
      }
    }
    return;
  }

  scoped_refptr<media::VideoFrame> frame;
  GetCurrentFrame(&frame);
  if ((frame == NULL) || (frame->data_[1] == NULL))
  {
    PutCurrentFrame(frame);
    return;
  }

  VA_Buffer *pVaBuf = (VA_Buffer*)frame->data_[1];

  if (pVaBuf->IsH264 == 0x264)
  {
    video_display_ = pVaBuf->mDisplay;

    if (GetVideoPixmap(true) > 0)
    {
      direct_paint_inited_ = true;
    }
  }

  PutCurrentFrame(frame);
  direct_paint_init_tried_ = true;
#endif

  if (direct_paint_inited_ == true)
  {
    // Notify browser create video widget
    Send(new ViewHostMsg_CreateVideoWidget(routing_id_, reinterpret_cast<unsigned int>(this), video_size_));
    Send(new ViewHostMsg_EnableVideoWidget(routing_id_, reinterpret_cast<unsigned int>(this), direct_paint_enabled_));
  }
}

void VideoRendererImpl::DirectPaint()
{
#if !defined(CONTROL_UI_DEBUG) // controls UI test
  DCHECK(MessageLoop::current() == proxy_->message_loop());

  if (video_rect_.IsEmpty() == true)
  {
    return;
  }

  scoped_refptr<media::VideoFrame> frame;
  GetCurrentFrame(&frame);
  if (frame == NULL)
  {
    PutCurrentFrame(frame);
    return;
  }

  CHECK(frame->width() == static_cast<size_t>(video_size_.width()));
  CHECK(frame->height() == static_cast<size_t>(video_size_.height()));

  unsigned int video_pixmap = GetVideoPixmap(false);
  if ((video_pixmap == 0)|| (frame->data_[1] == NULL))
  {
    PutCurrentFrame(frame);
    return;
  }
  VA_Buffer *pVaBuf = (VA_Buffer*)frame->data_[1];
  void *hw_ctx_display = pVaBuf->hwDisplay;
  VASurfaceID surface_id = (VASurfaceID)frame->idx_;
  VAStatus status;

  /*CC and Resize*/
  status = vaPutSurface(hw_ctx_display, surface_id, video_pixmap,
                        0, 0, frame->width(), frame->height(), /*src*/
                        0, 0, video_rect_.width(), video_rect_.height(), /*dst*/
                        NULL, 0,
                        VA_FRAME_PICTURE );

  PutCurrentFrame(frame);

  Send(new ViewHostMsg_UpdateVideoWidget(routing_id_,
                                         reinterpret_cast<unsigned int>(this),
                                         video_pixmap,
                                         video_rect_));
#else
  unsigned int video_pixmap = 0;
  Send(new ViewHostMsg_UpdateVideoWidget(routing_id_,
                                         reinterpret_cast<unsigned int>(this),
                                         video_pixmap,
                                         video_rect_));
#endif
  return;
}

void VideoRendererImpl::ExitDirectPaint()
{
  if (DirectPaintInited() == false)
  {
    direct_paint_inited_ = false;
    direct_paint_init_tried_ = false;
    return;
  }

  DCHECK(MessageLoop::current() == proxy_->message_loop());
  Send(new ViewHostMsg_DestroyVideoWidget(routing_id_, reinterpret_cast<unsigned int>(this)));
  FreeVideoPixmap(true);
  direct_paint_inited_ = false;
  direct_paint_init_tried_ = false;
}

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

void VideoRendererImpl::SetIsOverlapped(bool overlapped)
{
  is_overlapped_ = overlapped;
  if (overlapped)
  {
    EnableDirectPaint(false);
  }
  else
  {
    EnableDirectPaint(true);
  }
}


void VideoRendererImpl::OnStop(media::FilterCallback* callback) {
  proxy_->message_loop()->PostTask(FROM_HERE, NewRunnableMethod(this, &VideoRendererImpl::ExitDirectPaint));

  /*free hw pixmap*/
  H264FreePixmap(proxy_, (Display*)video_display_);

  if (callback) {
    callback->Run();
    delete callback;
  }
}

void VideoRendererImpl::OnFrameAvailable() {
  if ((paint_reset_ == false) && (direct_paint_enabled_ == true) && (DirectPaintInited() == true) && subwin == 0)
  {
    proxy_->message_loop()->PostTask(FROM_HERE, NewRunnableMethod(this, &VideoRendererImpl::DirectPaint));
    return;
  }

  paint_reset_ = false;
  proxy_->Repaint();
}

void VideoRendererImpl::SetWebMediaPlayerImplProxy(
    WebMediaPlayerImpl::Proxy* proxy) {
  proxy_ = proxy;
}

void VideoRendererImpl::SetRect(const gfx::Rect& rect) {
  if ((video_rect_.width() != rect.width()) || (video_rect_.height() != rect.height()))
  {
    paint_reset_ = true;
  }
}

// This method is always called on the renderer's thread.
void VideoRendererImpl::Paint(SkCanvas* canvas,
                              const gfx::Rect& dest_rect) {
  InitDirectPaint(dest_rect);

  if ((direct_paint_enabled_ == true) && (DirectPaintInited() == true))
  {
    DirectPaint();
    return;
  }

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
void VideoRendererImpl::H264FreePixmap(WebMediaPlayerImpl::Proxy* proxy, Display *dTmp)
{
  XImage * mXImage = proxy->m_ximage_;
  XShmSegmentInfo *shm = &proxy_->shminfo_ ;

  if((shm->shmid !=0) && shm->shmaddr){
    if(!dTmp){
      /*NULL*/
      return;
    }

    /*free share memory*/
    shmdt(shm->shmaddr);
    shmctl(shm->shmid, IPC_RMID, 0);
    shm->shmid = 0;
    shm->shmaddr = NULL;
  }

  /*free and reallocate*/
  if((dTmp != NULL) && proxy_->hw_pixmap_){
    XFreePixmap(dTmp, proxy_->hw_pixmap_);
    proxy_->hw_pixmap_ = 0;
    proxy_->pixmap_w_ = 0;
    proxy_->pixmap_h_ = 0;
  }

  return;
}
void VideoRendererImpl::H264CreateXImage(WebMediaPlayerImpl::Proxy* proxy, Display *dTmp, int w_, int h_, XShmSegmentInfo * shm_tmp)
{
  XImage * mXImage = NULL;
  XShmSegmentInfo *shm = &proxy_->shminfo_ ;

  mXImage = XShmCreateImage(dTmp, DefaultVisual(dTmp, DefaultScreen(dTmp)), 24, ZPixmap, NULL, shm_tmp, w_, h_);
  if(!mXImage){
     LOG(ERROR) << "XShm Create Error ";
     return ;
  }
  shm_tmp->shmaddr = mXImage->data = shm->shmaddr;
  shm_tmp->shmid = shm->shmid;

  if(!XShmAttach(dTmp, shm_tmp)){
     LOG(ERROR) << "XShmAttach Error" ;
     return ;
  };

  proxy->m_ximage_ = mXImage;

  return;
}

#define MAX_WIDTH 1280
#define MAX_HEIGHT 720 

/*return a mXImage, hw_pixmap, Shmmeory */
void VideoRendererImpl::H264GetPixmapAndmXImage(WebMediaPlayerImpl::Proxy* proxy, Display *dTmp, int w_, int h_, XShmSegmentInfo * shm_tmp)
{
  XImage * mXImage = proxy->m_ximage_;
  XShmSegmentInfo *shm = &proxy_->shminfo_ ;
  
  if((w_ == 0) || (h_ == 0)){
    return ;
  }

  if((proxy->pixmap_w_ == w_) && (proxy->pixmap_h_ == h_)){
    /*use last one, normal case*/
    /*create Image*/
    H264CreateXImage(proxy, dTmp, w_, h_, shm_tmp);
    
    return ;
  }else{

    /*1st frame pixmap*/
    if(!dTmp){
      return;
    }
    /*We free it firstly, Detach, Destroy, and free share memroy*/
    H264FreePixmap(proxy, dTmp);

    /*allocate shm, pixmap, mXImage*/
    int screen = DefaultScreen(dTmp);
    XWindowAttributes attr;
    int root_window = RootWindow(dTmp, screen);
    Window wTmp = root_window;
    XGetWindowAttributes (dTmp, wTmp, &attr);

    /*allocate something*/
    proxy->hw_pixmap_ = XCreatePixmap(dTmp, wTmp, w_, h_, attr.depth);
    if(proxy->hw_pixmap_ == 0){
      LOG(ERROR) << "XCreatePixmap  Error ";
      return ;
    }
    proxy->pixmap_w_ = w_;
    proxy->pixmap_h_ = h_;

#define _Shared_
#ifdef _Shared_
    int shmid = 0;
    //char* pShmaddr = NULL;
    if(!shm->shmaddr){
       //shmid = shmget(IPC_PRIVATE, ((w_ + 31) & (~31))*((h_ + 31) & (~31))*4, 0666);
       shm->shmid = shmget(IPC_PRIVATE, MAX_WIDTH*MAX_HEIGHT*4, 0666);
       shm->shmaddr = (char*)shmat(shm->shmid, NULL /* desired address */, 0 /* flags */);
       if(!shm->shmaddr){
         LOG(ERROR) << "XShm Alloc Error ";
         return ;
       }
    }

    H264CreateXImage(proxy, dTmp, w_, h_, shm_tmp);
#endif

    return ;
  }

}

void VideoRendererImpl::H264Paint(WebMediaPlayerImpl::Proxy* proxy, media::VideoFrame* video_frame, int dst_w, int dst_h, uint8 * pDst, int stride)
{
 
  int w = video_frame->width();
  int h = video_frame->height();
  
  int w_ = dst_w;
  int h_ = dst_h;
  
  WebMediaPlayerImpl * wp = proxy->GetMediaPlayer();

  if(!video_frame->data_[1]){
    return;
  }
  VA_Buffer *pVaBuf = (VA_Buffer*)video_frame->data_[1];
    
  void *hw_ctx_display = pVaBuf->hwDisplay;
  VASurfaceID surface_id = (VASurfaceID)video_frame->idx_;
  VAStatus status;
  Display *dpy = (Display*) pVaBuf->mDisplay;
  XShmSegmentInfo *shminfo = &proxy->shminfo_;

  base::AutoLock auto_lock(proxy->paint_lock_);

  if(wp->paused() && subwin){
       return;
  }
   /*if paused, but no subwin, it means preload*/
  if(subwin){
    /*if not paused , just render directly in full screen mode.*/
    return;

  }else{
    /*if paused , just copy to shm, and .*/

    Display *dTmp = (Display *)pVaBuf->mDisplay;

    XShmSegmentInfo shm = {0};
    /*get a pixmap and mxImage for RGBA data retrieve*/
    H264GetPixmapAndmXImage(proxy, dTmp, w_, h_, &shm);
   
    if((proxy->hw_pixmap_ == 0) || (proxy->m_ximage_ == NULL)){
     return;
    } 

    unsigned long pixmap = proxy->hw_pixmap_;
    
    //return ;
    if(proxy_->reload_){
      return;
    }
    /*CC and Resize*/
    status = vaPutSurface(hw_ctx_display, surface_id, pixmap,
                              0, 0, w, h, /*src*/
                              0, 0, w_, h_, /*dst*/
                              NULL, 0,
                              VA_FRAME_PICTURE );
    if (status != VA_STATUS_SUCCESS) {
      LOG(ERROR) << "vaPutsurface Error " ;
    }

    XShmSegmentInfo shminfo = {0};

    XImage * mXImage = proxy->m_ximage_;

#ifdef _Shared_

    if(!XShmGetImage(dTmp, pixmap, mXImage, 0, 0, AllPlanes) ){
       LOG(ERROR) << "XShmGetImage Error" ;
       return;
    } ;

#else
    mXImage = XGetImage(dTmp, pixmap, 0, 0, w_, h_, AllPlanes, ZPixmap);
#endif

    if(proxy->last_frame_){
        memset(mXImage->data, 0, w_*h_*4);
        proxy->last_frame_ = 0;
    }

#if 0
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
#else
   /*workaround for invalid ARGB*/
   int i = 0;
   int j = 0;
   unsigned int *pSrc = (unsigned int *)mXImage->data;
   unsigned int *pDst4 = (unsigned int *)pDst;
   for(i = 0; i <  mXImage->height; i ++){
     for(j = 0; j <  (mXImage->bytes_per_line>>2); j ++){
       /*set Alpha channle to FF, not transparents*/
       *(pDst4 + j) = (0xFF000000 | *(pSrc+ j));
     }
     pDst4 += (stride>>2);
     pSrc += (mXImage->bytes_per_line>>2);
   } 
#endif

#ifdef _Shared_
    if(!XShmDetach(dTmp, &shm)){
        LOG(ERROR) << "XShmDetach Error" ;
        return;
    }
    XDestroyImage(mXImage);
    proxy->m_ximage_ = NULL;

#else
    XDestroyImage(mXImage);
#endif


    //XFreePixmap(dTmp, pixmap);
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

  /*Skip not really resize paint*/
  if(local_matrix.getType() == 2){
    return;
  }

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

  VA_Buffer *pVaBuf = (VA_Buffer*)video_frame->data_[1];

  if(pVaBuf->IsH264 == 0x264){

    if(local_dest_irect.width() == dest_rect.width()){
      if(local_dest_irect.height() == dest_rect.height()){
        /*H264 Paint, Directly or not*/
        H264Paint(proxy_, video_frame, local_dest_irect.width(), local_dest_irect.height(),dest_rect_pointer,bitmap.rowBytes());
      }
    }

    bitmap.unlockPixels();

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
