// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/tools/player_x11/x11_video_renderer.h"

#include <dlfcn.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xcomposite.h>

#include "base/message_loop.h"
#include "media/base/video_frame.h"
#include "media/base/yuv_convert.h"

#if defined (TOOLKIT_MEEGOTOUCH)
/*optimize the render for FFMPEG/LIBVA*/
#define _DEV2_OPT_

#ifdef _DEV2_OPT_
#include <va/va.h>
#include <va/va_x11.h>
/*XA_WINDOW*/
#include <X11/Xatom.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>

#endif
#endif


X11VideoRenderer* X11VideoRenderer::instance_ = NULL;

// Returns the picture format for ARGB.
// This method is originally from chrome/common/x11_util.cc.
static XRenderPictFormat* GetRenderARGB32Format(Display* dpy) {
  static XRenderPictFormat* pictformat = NULL;
  if (pictformat)
    return pictformat;

  // First look for a 32-bit format which ignores the alpha value.
  XRenderPictFormat templ;
  templ.depth = 32;
  templ.type = PictTypeDirect;
  templ.direct.red = 16;
  templ.direct.green = 8;
  templ.direct.blue = 0;
  templ.direct.redMask = 0xff;
  templ.direct.greenMask = 0xff;
  templ.direct.blueMask = 0xff;
  templ.direct.alphaMask = 0;

  static const unsigned long kMask =
      PictFormatType | PictFormatDepth |
      PictFormatRed | PictFormatRedMask |
      PictFormatGreen | PictFormatGreenMask |
      PictFormatBlue | PictFormatBlueMask |
      PictFormatAlphaMask;

  pictformat = XRenderFindFormat(dpy, kMask, &templ, 0 /* first result */);

  if (!pictformat) {
    // Not all X servers support xRGB32 formats. However, the XRENDER spec
    // says that they must support an ARGB32 format, so we can always return
    // that.
    pictformat = XRenderFindStandardFormat(dpy, PictStandardARGB32);
    CHECK(pictformat) << "XRENDER ARGB32 not supported.";
  }

  return pictformat;
}

X11VideoRenderer::X11VideoRenderer(Display* display, Window window,
                                   MessageLoop* message_loop)
    : display_(display),
      window_(window),
      image_(NULL),
      picture_(0),
      use_render_(false),
      glx_thread_message_loop_(message_loop) {
}

X11VideoRenderer::~X11VideoRenderer() {
}

void X11VideoRenderer::OnStop(media::FilterCallback* callback) {
  if (image_) {
    XDestroyImage(image_);
  }
  XRenderFreePicture(display_, picture_);
  if (callback) {
    callback->Run();
    delete callback;
  }
}

bool X11VideoRenderer::OnInitialize(media::VideoDecoder* decoder) {
  LOG(INFO) << "Initializing X11 Renderer...";

  // Resize the window to fit that of the video.
#if defined (TOOLKIT_MEEGOTOUCH)
  //XResizeWindow(display_, window_, width(), height());
#else
  XResizeWindow(display_, window_, width(), height());
#endif

  // Testing XRender support. We'll use the very basic of XRender
  // so if it presents it is already good enough. We don't need
  // to check its version.
  int dummy;
  use_render_ = XRenderQueryExtension(display_, &dummy, &dummy);

  if (use_render_) {
    // If we are using XRender, we'll create a picture representing the
    // window.
    XWindowAttributes attr;
    XGetWindowAttributes(display_, window_, &attr);

    XRenderPictFormat* pictformat = XRenderFindVisualFormat(
        display_,
        attr.visual);
    CHECK(pictformat) << "XRENDER does not support default visual";

    picture_ = XRenderCreatePicture(display_, window_, pictformat, 0, NULL);
    CHECK(picture_) << "Backing picture not created";
  }

  // Initialize the XImage to store the output of YUV -> RGB conversion.
  image_ = XCreateImage(display_,
                        DefaultVisual(display_, DefaultScreen(display_)),
                        DefaultDepth(display_, DefaultScreen(display_)),
                        ZPixmap,
                        0,
                        static_cast<char*>(malloc(width() * height() * 4)),
                        width(),
                        height(),
                        32,
                        width() * 4);
  DCHECK(image_);

  // Save this instance.
  DCHECK(!instance_);
  instance_ = this;
  return true;
}

void X11VideoRenderer::OnFrameAvailable() {
  if (glx_thread_message_loop()) {
    glx_thread_message_loop()->PostTask(FROM_HERE,
        NewRunnableMethod(this, &X11VideoRenderer::Paint));
  }
}
#if defined (TOOLKIT_MEEGOTOUCH)
#ifdef _DEV2_OPT_
extern double  GetTick(void);
extern int shmkey;
#endif

#define _MENU_

#ifdef _MENU_

#define G_WIDTH  1280
#define G_HEIGHT 800 
extern unsigned int g_menu_do;
extern unsigned int g_play_do;
extern long long g_pos;
extern long long g_pos_total;

void PaintPlayButton(Display *dpy, Window win, int play);

void PaintPlayButton(Display *dpy, Window win, int play)
{

  int w , h ;
  int x , y ;
  GC gc = XCreateGC(dpy, win, 0, NULL);

  /*flush background*/
  XSetForeground(dpy, gc, 0xff000000);
  x = 0; y = G_HEIGHT - 80;
  w = h = 80;
  XDrawRectangle(dpy, win, gc, x, y , w, h);
  XFillRectangle(dpy, win, gc, x, y , w, h);

  if(play == 0){
  /*Play buttone Triangle*/
  //XSetForeground(dpy, gc, 0xff4295e1);
  XSetForeground(dpy, gc, 0xff606060);
  XPoint points[] = {
    {10,G_HEIGHT - 70},
    {70,G_HEIGHT - 40},
    {10,G_HEIGHT - 10},
    {10,G_HEIGHT - 70},
     
  };
  XDrawLines(dpy, win, gc, points, 4, CoordModeOrigin);
  XFillPolygon(dpy, win, gc, points, 4, Convex ,CoordModeOrigin);
  }else{
  /*Pause button*/
  XSetForeground(dpy, gc, 0xff606060);
  x = 10;
  y = G_HEIGHT - 70;
  XDrawRectangle(dpy, win, gc, x, y , 20, 60);
  XFillRectangle(dpy, win, gc, x, y , 20, 60);
  x = 40;
  XDrawRectangle(dpy, win, gc, x, y , 20, 60);
  XFillRectangle(dpy, win, gc, x, y , 20, 60);
  }

}

int GetPosition()
{
    /*framerate = 24*/
    int fr = 24;
    int position = 0;

    position = 80 + g_pos * (G_WIDTH - 80*2)/g_pos_total ;
  
    return position;
} 

int PaintExitButton(Display *dpy, Window win, GC gc)
{

    XSetForeground(dpy, gc, 0xff505050);
    XSetLineAttributes(dpy, gc, 8, LineSolid, CapNotLast, JoinMiter);
    XSegment seg[]= {
        {G_WIDTH - 70, G_HEIGHT - 70, G_WIDTH - 10, G_HEIGHT - 10},
        {G_WIDTH - 70, G_HEIGHT - 10, G_WIDTH - 10, G_HEIGHT - 70},
    };
    XDrawSegments(dpy, win, gc, seg, 2);

}

void PaintControlBar(Display *dpy, Window win)
{

  GC gc = XCreateGC(dpy, win, 0, NULL);

  int w , h ;
  int x , y ;
  int seek_h;
  
  XSetFillStyle(dpy, gc, FillSolid);

  /*Paint seek */
  seek_h = 78;
  w = G_WIDTH - 84 - 80;

  h = seek_h;
  x = 80; y = G_HEIGHT - seek_h - 2;
  /*seek bar*/
  XSetForeground(dpy, gc, 0xff4295e1);
  XDrawRectangle(dpy, win, gc, x, y , w, h);
  XFillRectangle(dpy, win, gc, x, y , w, h);

  x = GetPosition();

  /*Paint moving label*/
  //XSetForeground(dpy, gc, 0xff101010);
  //XSetForeground(dpy, gc, 0xff505050);
  XSetForeground(dpy, gc, 0xff000050);
  XDrawRectangle(dpy, win, gc, x, y , 12, h);
  //XFillRectangle(dpy, win, gc, x, y , 12, h);

  if(g_play_do == 0){
  /*Play buttone Triangle*/
  //XSetForeground(dpy, gc, 0xff4295e1);
  XSetForeground(dpy, gc, 0xff606060);
  XPoint points[] = {
    {10,G_HEIGHT - 70},
    {70,G_HEIGHT - 40},
    {10,G_HEIGHT - 10},
    {10,G_HEIGHT - 70},
     
  };

  XDrawLines(dpy, win, gc, points, 4, CoordModeOrigin);
  XFillPolygon(dpy, win, gc, points, 4, Convex ,CoordModeOrigin);
  }else{

  /*Pause button*/
  XSetForeground(dpy, gc, 0xff606060);
  x = 10;
  y = G_HEIGHT - 70;
  XDrawRectangle(dpy, win, gc, x, y , 20, 60);
  XFillRectangle(dpy, win, gc, x, y , 20, 60);
  x = 40;
  XDrawRectangle(dpy, win, gc, x, y , 20, 60);
  XFillRectangle(dpy, win, gc, x, y , 20, 60);
  }

  /*exit button*/
  PaintExitButton(dpy, win, gc);

  return;
}

//#define _MENU_ in player_x11.c
/*FULLSCREEN*/
void X11VideoRenderer::Paint() {
  scoped_refptr<media::VideoFrame> video_frame;
  GetCurrentFrame(&video_frame);
  int w_ = G_WIDTH, h_ = G_HEIGHT ;

  /*resize of not while menu is enabled*/
  if(g_menu_do){
     h_ -= 84;
  }

  if(width() < 720){
    w_ = width();
    h_ = height();
  }

  if (!image_ || !video_frame) {
    // TODO(jiesun): Use color fill rather than create black frame then scale.
    PutCurrentFrame(video_frame);
    return;
  }

  if(video_frame->data_[1] == (uint8_t*)0x264){
   
    int w = video_frame->width();
    int h = video_frame->height();


    void* hw_ctx_display = (void*)video_frame->data_[2];
    VASurfaceID surface_id = (VASurfaceID)video_frame->idx_;
    VAStatus status;
    //Display *display2 = display_; 
    Display *display2 = (Display *)video_frame->data_[0];

    /*do render here for ffmpeg/libva case*/
    Display *dTmp = display_;
    Window wTmp = window_;

    XWindowAttributes attr;
    XGetWindowAttributes (dTmp, wTmp, &attr);

    status = vaPutSurface(hw_ctx_display, surface_id, window_,
                              0, 0, width(), height(),
                              0, 0, w_, h_,
                              NULL, 0,
                              VA_FRAME_PICTURE );
    
    base::TimeDelta time = video_frame->GetDuration();
    base::TimeDelta time2 = video_frame->GetTimestamp();
    g_pos = time2.InSeconds();
    //g_pos = time2.InMilliseconds();
    //("Cur Duration: %lld TimeStamp: %lld: \n", time.ToInternalValue(),time2.InMilliseconds());

    PutCurrentFrame(video_frame);

    /*add control bar*/
    if(g_menu_do){
        PaintControlBar(dTmp, window_);
    }

    return;
  }

  media::YUVType yuv_type =
      (video_frame->format() == media::VideoFrame::YV12) ?
      media::YV12 : media::YV16;
/*
  media::ConvertYUVToRGB32(video_frame->data(media::VideoFrame::kYPlane),
                           video_frame->data(media::VideoFrame::kUPlane),
                           video_frame->data(media::VideoFrame::kVPlane),
                           (uint8*)image_->data,
                           video_frame->width(),
                           video_frame->height(),
                           video_frame->stride(media::VideoFrame::kYPlane),
                           video_frame->stride(media::VideoFrame::kUPlane),
                           image_->bytes_per_line,
                           yuv_type);
*/

  w_ = video_frame->width();
  h_ = video_frame->height();
  /*resize*/
  media::ScaleYUVToRGB32(video_frame->data(media::VideoFrame::kYPlane),
                           video_frame->data(media::VideoFrame::kUPlane),
                           video_frame->data(media::VideoFrame::kVPlane),
                           (uint8*)image_->data,
                           video_frame->width(),
                           video_frame->height(),
                           w_,
                           h_,
                           video_frame->stride(media::VideoFrame::kYPlane),
                           video_frame->stride(media::VideoFrame::kUPlane),
                           image_->bytes_per_line,
                           yuv_type,
                           media::ROTATE_0,
                           media::FILTER_BILINEAR);

  PutCurrentFrame(video_frame);

  if (use_render_) {
    // If XRender is used, we'll upload the image to a pixmap. And then
    // creats a picture from the pixmap and composite the picture over
    // the picture represending the window.

    // Creates a XImage.
    XImage image;
    memset(&image, 0, sizeof(image));
    image.width = w_;
    image.height = h_;
    image.depth = 32;
    image.bits_per_pixel = 32;
    image.format = ZPixmap;
    image.byte_order = LSBFirst;
    image.bitmap_unit = 8;
    image.bitmap_bit_order = LSBFirst;
    image.bytes_per_line = image_->bytes_per_line;
    image.red_mask = 0xff;
    image.green_mask = 0xff00;
    image.blue_mask = 0xff0000;
    image.data = image_->data;

    // Creates a pixmap and uploads from the XImage.
    unsigned long pixmap = XCreatePixmap(display_,
                                         window_,
                                         w_,
                                         h_,
                                         32);

    //("Xcreate pixmap: %x, %x, %x, %x\n", display_, window_, width(), height());
    GC gc = XCreateGC(display_, pixmap, 0, NULL);
    XPutImage(display_, pixmap, gc, &image,
              0, 0, 0, 0,
              w_, h_);
    XFreeGC(display_, gc);

    // Creates the picture representing the pixmap.
    unsigned long picture = XRenderCreatePicture(
        display_, pixmap, GetRenderARGB32Format(display_), 0, NULL);

    //("XRenderCreatePicture, %x, %x, %x\n", display_, pixmap, GetRenderARGB32Format(display_));

    // Composite the picture over the picture representing the window.
    XRenderComposite(display_, PictOpSrc, picture, 0,
                     picture_, 0, 0, 0, 0, 0, 0,
                     w_, h_);

    XRenderFreePicture(display_, picture);
    XFreePixmap(display_, pixmap);

    return;
  }

  // If XRender is not used, simply put the image to the server.
  // This will have a tearing effect but this is OK.
  // TODO(hclam): Upload the image to a pixmap and do XCopyArea()
  // to the window.
  GC gc = XCreateGC(display_, window_, 0, NULL);
  XPutImage(display_, window_, gc, image_,
            0, 0, 0, 0, width(), height());
  XFlush(display_);
  XFreeGC(display_, gc);

}

#else
/*to disable _MENU_ in player_x11.cc*/
void X11VideoRenderer::Paint() {

  scoped_refptr<media::VideoFrame> video_frame;
  GetCurrentFrame(&video_frame);
  int w_ = 720, h_ = 480;

  if(width() < 720){
    w_ = width();
    h_ = height();
  }

  if (!image_ || !video_frame) {
    // TODO(jiesun): Use color fill rather than create black frame then scale.
    PutCurrentFrame(video_frame);
    return;
  }

#ifdef _DEV2_OPT_
  if(video_frame->data_[1] == (uint8_t*)0x264){
   
    int w = video_frame->width();
    int h = video_frame->height();


    void* hw_ctx = (void*)video_frame->data_[2];
    VASurfaceID surface_id = (VASurfaceID)video_frame->idx_;
    VAStatus status;
    Display *display2 = (Display *)video_frame->data_[0];

    /*do render here for FFMPEG/LIBVA Case*/
#if 0
#else

#define _ANOTHER_DISPLAY_

#ifdef _ANOTHER_DISPLAY_
    Display *dTmp = display2;
    int screen = DefaultScreen(dTmp);
    int root_window = RootWindow(dTmp, screen);
    Window wTmp = root_window;
#else
    Display *dTmp = display_;
    Window wTmp = window_;
#endif


{

    XWindowAttributes attr;
    XGetWindowAttributes (dTmp, wTmp, &attr);
    // Creates a pixmap and uploads from the XImage.
    //unsigned long pixmap = XCreatePixmap(display_,
    unsigned long pixmap = XCreatePixmap(dTmp,
                                         wTmp,
                                         width(),
                                         height(),
                                         attr.depth);
    
    //("Xcreate pixmap-hw: %x, %x, %x, %x\n", display_, window_, width(), height());

    status = vaPutSurface(hw_ctx_display, surface_id, pixmap,
                              0, 0, width(), height(),
                              0, 0, w_, h_,
                              NULL, 0,
                              VA_FRAME_PICTURE );

                              //0, 0, width(), height(),
#if 1
    /*Copy Directly from pixmap to window, mush with-the-same-root-window for drawables.*/
    GC gc = XCreateGC(display_, window_, 0, NULL);
    //("XCopyArea: %x, %x, %x, %x, w.h: 480.320, attr.depth: %x\n", display_, pixmap, window_, gc, attr.depth);
    XCopyArea(display_, pixmap, window_,  gc, 0, 0, w_, h_, 16, 16);
    XFreeGC(display_, gc);
#else

    XImage * mXImage;
#define _Shared_
#ifdef _Shared_
    static int shared = 0;
    //static int shmkey;
    static char* pbuffer = NULL;
    /*if w_, h_ is changed, shm has to be reallocated.*/
    if(!shared){
    shared = 1;
        shmkey = shmget(IPC_PRIVATE, w_*h_*4, 0666);
        pbuffer = (char*)shmat(shmkey, NULL /* desired address */, 0 /* flags */);
    }
    {
        XShmSegmentInfo shminfo = {0};

        mXImage = XShmCreateImage(dTmp, DefaultVisual(dTmp, DefaultScreen(dTmp)), 24, ZPixmap, NULL, &shminfo, w_, h_);
 //       ("mXImage->bytes_per_line: %d, mXImage->height: %d\n", mXImage->bytes_per_line, mXImage->height);

        //const int shmkey = shmget(IPC_PRIVATE, w_*h_*4, 0666);
        //char* pbuffer = (char*)shmat(shmkey, NULL /* desired address */, 0 /* flags */);
        shminfo.shmaddr = mXImage->data = pbuffer;
        shminfo.shmid = shmkey;
        //shminfo.shmseg = 20;

       if(!XShmAttach(dTmp, &shminfo)){
            LOG(ERROR) << "XShmAttach Error" ;
    };
    }
//    ("w_.h_: %d.%d, byte.row: %d, buf: %x\n", w_, h_, mXImage->bytes_per_line, mXImage->data);
    if(! XShmGetImage(dTmp, pixmap, mXImage, 0, 0, AllPlanes) ){
        LOG(ERROR) << "XShmGetImage Error" ;
    } ;
#else

    mXImage = XGetImage(dTmp, pixmap, 0, 0, w_, h_, AllPlanes, ZPixmap);
#endif

    unsigned long pixmap2 = XCreatePixmap(display_,
                                         window_,
                                         w_,
                                         h_,
                                         attr.depth);

    GC gc2 = XCreateGC(display_, pixmap2, 0, NULL);

    XPutImage(display_, pixmap2, gc2, mXImage, 0, 0, 0, 0, w_, h_);

    XFreeGC(display_, gc2);

    XRenderPictFormat *pictformat = NULL;

    pictformat = XRenderFindStandardFormat(display_, PictStandardRGB24); 
    // Creates the picture representing the pixmap.
    unsigned long picture = XRenderCreatePicture(display_, pixmap2, pictformat, 0, NULL);

    //("XRenderCreatePicture-hw, %x, %x, %x\n", display_, pixmap, pictformat);

    // Composite the picture over the picture representing the window.
    XRenderComposite(display_, PictOpSrc, picture, 0,
                     picture_, 0, 0, 0, 0, 0, 0,
                     w_, h_);
                     //width(), height());
    XRenderFreePicture(display_, picture);
    XFreePixmap(display_, pixmap2);

#ifdef Shared
    XShmDetach(dTmp, shminfo);
    XDestroyImage(mXImage);
    shmdt(shminfo->shmaddr);
#endif

#endif /*rend to where*/

    PutCurrentFrame(video_frame);

    XFreePixmap(dTmp, pixmap);
    return;
}

#endif

  }
#endif

  media::YUVType yuv_type =
      (video_frame->format() == media::VideoFrame::YV12) ?
      media::YV12 : media::YV16;
/*
  media::ConvertYUVToRGB32(video_frame->data(media::VideoFrame::kYPlane),
                           video_frame->data(media::VideoFrame::kUPlane),
                           video_frame->data(media::VideoFrame::kVPlane),
                           (uint8*)image_->data,
                           video_frame->width(),
                           video_frame->height(),
                           video_frame->stride(media::VideoFrame::kYPlane),
                           video_frame->stride(media::VideoFrame::kUPlane),
                           image_->bytes_per_line,
                           yuv_type);
*/

  /*resize*/
  media::ScaleYUVToRGB32(video_frame->data(media::VideoFrame::kYPlane),
                           video_frame->data(media::VideoFrame::kUPlane),
                           video_frame->data(media::VideoFrame::kVPlane),
                           (uint8*)image_->data,
                           video_frame->width(),
                           video_frame->height(),
                           w_,
                           h_,
                           video_frame->stride(media::VideoFrame::kYPlane),
                           video_frame->stride(media::VideoFrame::kUPlane),
                           image_->bytes_per_line,
                           yuv_type,
                           media::ROTATE_0,
                           media::FILTER_BILINEAR);

  PutCurrentFrame(video_frame);

  if (use_render_) {
    // If XRender is used, we'll upload the image to a pixmap. And then
    // creats a picture from the pixmap and composite the picture over
    // the picture represending the window.

    // Creates a XImage.
    XImage image;
    memset(&image, 0, sizeof(image));
/*
    image.width = width();
    image.height = height();
*/
    image.width = w_;
    image.height = h_;
    image.depth = 32;
    image.bits_per_pixel = 32;
    image.format = ZPixmap;
    image.byte_order = LSBFirst;
    image.bitmap_unit = 8;
    image.bitmap_bit_order = LSBFirst;
    image.bytes_per_line = image_->bytes_per_line;
    image.red_mask = 0xff;
    image.green_mask = 0xff00;
    image.blue_mask = 0xff0000;
    image.data = image_->data;

    // Creates a pixmap and uploads from the XImage.
    unsigned long pixmap = XCreatePixmap(display_,
                                         window_,
                                         w_,
                                         h_,
                                         32);

    //("Xcreate pixmap: %x, %x, %x, %x\n", display_, window_, width(), height());
    GC gc = XCreateGC(display_, pixmap, 0, NULL);
    XPutImage(display_, pixmap, gc, &image,
              0, 0, 0, 0,
              w_, h_);
    XFreeGC(display_, gc);

    // Creates the picture representing the pixmap.
    unsigned long picture = XRenderCreatePicture(
        display_, pixmap, GetRenderARGB32Format(display_), 0, NULL);

    // Composite the picture over the picture representing the window.
    XRenderComposite(display_, PictOpSrc, picture, 0,
                     picture_, 0, 0, 0, 0, 0, 0,
                     w_, h_);

    XRenderFreePicture(display_, picture);
    XFreePixmap(display_, pixmap);

    return;
  }

  // If XRender is not used, simply put the image to the server.
  // This will have a tearing effect but this is OK.
  // TODO(hclam): Upload the image to a pixmap and do XCopyArea()
  // to the window.
  GC gc = XCreateGC(display_, window_, 0, NULL);
  XPutImage(display_, window_, gc, image_,
            0, 0, 0, 0, width(), height());
  XFlush(display_);
  XFreeGC(display_, gc);
}
#endif
#endif
