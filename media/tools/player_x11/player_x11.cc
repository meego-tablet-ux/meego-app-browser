// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <signal.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>

#if defined (TOOLKIT_MEEGOTOUCH)
/*XA_WINDOW*/
#include <X11/Xatom.h>
#endif

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/file_path.h"
#include "base/memory/scoped_ptr.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread.h"
#include "media/base/callback.h"
#include "media/base/filter_collection.h"
#include "media/base/media.h"
#include "media/base/media_switches.h"
#include "media/base/message_loop_factory_impl.h"
#include "media/base/pipeline_impl.h"
#include "media/filters/adaptive_demuxer.h"
#include "media/filters/audio_renderer_impl.h"
#include "media/filters/ffmpeg_audio_decoder.h"
#include "media/filters/ffmpeg_demuxer_factory.h"
#include "media/filters/ffmpeg_video_decoder.h"
#include "media/filters/file_data_source_factory.h"
#include "media/filters/null_audio_renderer.h"
#include "media/filters/omx_video_decoder.h"

// TODO(jiesun): implement different video decode contexts according to
// these flags. e.g.
//     1.  system memory video decode context for x11
//     2.  gl texture video decode context for OpenGL.
//     3.  gles texture video decode context for OpenGLES.
// TODO(jiesun): add an uniform video renderer which take the video
//       decode context object and delegate renderer request to these
//       objects. i.e. Seperate "painter" and "pts scheduler".
#if defined(RENDERER_GL)
#include "media/tools/player_x11/gl_video_renderer.h"
typedef GlVideoRenderer Renderer;
#elif defined(RENDERER_GLES)
#include "media/tools/player_x11/gles_video_renderer.h"
typedef GlesVideoRenderer Renderer;
#elif defined(RENDERER_X11)
#include "media/tools/player_x11/x11_video_renderer.h"
typedef X11VideoRenderer Renderer;
#else
#error No video renderer defined.
#endif

Display* g_display = NULL;
Window g_window = 0;
bool g_running = false;

#if defined (TOOLKIT_MEEGOTOUCH)

#define _MENU_

#ifdef _MENU_
unsigned int g_menu_do = 0;
unsigned int g_play_do = 0;
long long g_pos = 0;
long long g_pos_total = 1;

extern void PaintPlayButton(Display *dpy, Window win, int play);
#endif

#endif

class MessageLoopQuitter {
 public:
  explicit MessageLoopQuitter(MessageLoop* loop) : loop_(loop) {}
  void Quit(media::PipelineStatus status) {
    loop_->PostTask(FROM_HERE, new MessageLoop::QuitTask());
    delete this;
  }
 private:
  MessageLoop* loop_;
  DISALLOW_COPY_AND_ASSIGN(MessageLoopQuitter);
};

// Initialize X11. Returns true if successful. This method creates the X11
// window. Further initialization is done in X11VideoRenderer.
bool InitX11() {
  g_display = XOpenDisplay(NULL);
  if (!g_display) {
    std::cout << "Error - cannot open display" << std::endl;
    return false;
  }

  // Get properties of the screen.
  int screen = DefaultScreen(g_display);
  int root_window = RootWindow(g_display, screen);

  // Creates the window.
  g_window = XCreateSimpleWindow(g_display, root_window, 1, 1, 100, 50, 0,
                                 BlackPixel(g_display, screen),
                                 BlackPixel(g_display, screen));

#if defined (TOOLKIT_MEEGOTOUCH)
       /*FIXME*/
       /*work around to Xorg/Mcompositor */
       /*resize to full screen*/
       XWindowAttributes attr;
       XGetWindowAttributes(g_display, g_window, &attr);
       XGetWindowAttributes(g_display, root_window, &attr);
       XResizeWindow(g_display, g_window, attr.width, attr.height);
  {

      long data[2] ;
      Atom property;
      data[0] = XInternAtom(g_display, "_KDE_NET_WM_WINDOW_TYPE_OVERRIDE", false);
      data[1] = XInternAtom(g_display, "_NET_WM_WINDOW_TYPE_NORMAL", false);
      property = XInternAtom(g_display, "_NET_WM_WINDOW_TYPE", false);
      XChangeProperty(g_display, g_window, property, XA_ATOM, 32, PropModeReplace, (unsigned char*)data, 2);
     
  }

#endif

  XStoreName(g_display, g_window, "X11 Media Player");

  XSelectInput(g_display, g_window,
               ExposureMask | ButtonPressMask | KeyPressMask);
  XMapWindow(g_display, g_window);
  return true;
}

media::FilterCollection* CreateCollection(MessageLoop* message_loop, 
                              		   bool enable_audio, 
                                           MessageLoop* paint_message_loop,
                                           media::MessageLoopFactory* message_loop_factory)
{

  media::FilterCollection* collection =  new media::FilterCollection() ;

  (collection)->SetDemuxerFactory(
      new media::AdaptiveDemuxerFactory(
          new media::FFmpegDemuxerFactory(
              new media::FileDataSourceFactory(), message_loop)));

  (collection)->AddAudioDecoder(new media::FFmpegAudioDecoder(
      message_loop_factory->GetMessageLoop("AudioDecoderThread")));

  if (CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableOpenMax)) {
    collection->AddVideoDecoder(new media::OmxVideoDecoder(
        message_loop_factory->GetMessageLoop("VideoDecoderThread"),
        NULL));
  } else {
    (collection)->AddVideoDecoder(new media::FFmpegVideoDecoder(
        message_loop_factory->GetMessageLoop("VideoDecoderThread"),
        NULL));
  }
  (collection)->AddVideoRenderer(new Renderer(g_display,
                                            g_window,
                                            paint_message_loop));

  if (enable_audio){
    (collection)->AddAudioRenderer(new media::AudioRendererImpl());
  }else{
    (collection)->AddAudioRenderer(new media::NullAudioRenderer());
  }

  /*No Need to free collection, it will be reset to pipeline_ internal scoped_ptr.*/
  /*refer to PipelineImpl::StartTask*/
  return collection;
}

int PipelineRestart(media::PipelineImpl* pipeline)
{
  /*create a new collection*/
  media::FilterCollection* collection = NULL;
  const char* filename = pipeline->filename;
  int ret = 0;

  collection = CreateCollection(pipeline->message_loop, pipeline->enable_audio, 
                   pipeline->paint_message_loop, pipeline->message_loop_factory);
  if(!collection){
    /*Fail to Create Collection*/
    std::cout << "Error in CreateCollection" << std::endl;
    return ret;
  }
  /*start*/
  media::PipelineStatusNotification note;
  (pipeline)->Start(collection, filename, note.Callback());

  // Wait until the pipeline is fully initialized.
  note.Wait();
  if (note.status() != media::PIPELINE_OK) {
    std::cout << "Start : " << note.status() << std::endl;
    (pipeline)->Stop(NULL);
    return ret;
  }

  /*check result*/
  if(pipeline->IsInitialized()){
    ret = 1;
  }else{
    ret = 0;
  }

  // And start the playback.
  (pipeline)->SetPlaybackRate(1.0f);

  return ret;
}

bool InitPipeline(MessageLoop* message_loop,
                  const char* filename, bool enable_audio,
                  scoped_refptr<media::PipelineImpl>* pipeline,
                  MessageLoop* paint_message_loop,
                  media::MessageLoopFactory* message_loop_factory) {
  // Initialize OpenMAX.
  if (CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableOpenMax) &&
      !media::InitializeOpenMaxLibrary(FilePath())) {
    std::cout << "Unable to initialize OpenMAX library."<< std::endl;
    return false;
  }

  // Load media libraries.
  if (!media::InitializeMediaLibrary(FilePath())) {
    std::cout << "Unable to initialize the media library." << std::endl;
    return false;
  }

  // Create our filter factories.
  scoped_ptr<media::FilterCollection> collection(
      new media::FilterCollection());
  collection->SetDemuxerFactory(
      new media::AdaptiveDemuxerFactory(
          new media::FFmpegDemuxerFactory(
              new media::FileDataSourceFactory(), message_loop)));

  collection->AddAudioDecoder(new media::FFmpegAudioDecoder(
      message_loop_factory->GetMessageLoop("AudioDecoderThread")));

  if (CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableOpenMax)) {
    collection->AddVideoDecoder(new media::OmxVideoDecoder(
        message_loop_factory->GetMessageLoop("VideoDecoderThread"),
        NULL));
  } else {
    collection->AddVideoDecoder(new media::FFmpegVideoDecoder(
        message_loop_factory->GetMessageLoop("VideoDecoderThread"),
        NULL));
  }
  collection->AddVideoRenderer(new Renderer(g_display,
                                            g_window,
                                            paint_message_loop));
  if (enable_audio){
    collection->AddAudioRenderer(new media::AudioRendererImpl());
  }else{
    collection->AddAudioRenderer(new media::NullAudioRenderer());
  }

  // Create the pipeline and start it.
  *pipeline = new media::PipelineImpl(message_loop);

  media::PipelineStatusNotification note;
  (*pipeline)->Start(collection.release(), filename, note.Callback());

  /*saving*/
  (*pipeline)->filename = filename ;
  (*pipeline)->message_loop = message_loop ;
  (*pipeline)->enable_audio = enable_audio;
  (*pipeline)->paint_message_loop = paint_message_loop;
  (*pipeline)->message_loop_factory = message_loop_factory;

  // Wait until the pipeline is fully initialized.
  note.Wait();
  if (note.status() != media::PIPELINE_OK) {
    std::cout << "InitPipeline: " << note.status() << std::endl;
    (*pipeline)->Stop(NULL);
    return false;
  }

  // And start the playback.
  (*pipeline)->SetPlaybackRate(1.0f);
  return true;
}

void TerminateHandler(int signal) {
  g_running = false;
}

void PeriodicalUpdate(
    media::PipelineImpl* pipeline,
    MessageLoop* message_loop,
    bool audio_only) {
  if (!g_running) {
    // interrupt signal was received during last time period.
    // Quit message_loop only when pipeline is fully stopped.
    MessageLoopQuitter* quitter = new MessageLoopQuitter(message_loop);
    pipeline->Stop(NewCallback(quitter, &MessageLoopQuitter::Quit));
    return;
  }

#if defined (TOOLKIT_MEEGOTOUCH)
#ifdef _MENU_
  while (XPending(g_display)) {
    XEvent e;
    XNextEvent(g_display, &e);
    //    ("\n  KeyCode:%d  Type:%d ",e.xkey.keycode, e.type);
    switch (e.type) {
      case Expose:
        if (!audio_only) {
          // Tell the renderer to paint.
          DCHECK(Renderer::instance());
          Renderer::instance()->Paint();
        }
        break;

    case MotionNotify:

      //(" <EXM:x y %d,%d> ", e.xmotion.x, e.xmotion.y);
      //g_curpos_x = e.xmotion.x; /// Qing
      //g_curpos_y = e.xmotion.y; /// Qing

      break;

      case ButtonPress:
        {
          Window window;
          int x, y;
          unsigned int width, height, border_width, depth;
          XGetGeometry(g_display,
                       g_window,
                       &window,
                       &x,
                       &y,
                       &width,
                       &height,
                       &border_width,
                       &depth);

          /*get playback status*/
      if (pipeline->GetPlaybackRate() < 0.01f){ // Check Paused
             g_play_do = 0;
      }else{
           /*playing*/
             g_play_do = 1;
      }
          
#define Button_W 80
#define Button_H 80
          /*check moving label position*/
      base::TimeDelta time5 = pipeline->GetMediaDuration();
      base::TimeDelta time6 = pipeline->GetCurrentTime();
          //("%lld.%lld\n", time5.ToInternalValue(), time6.ToInternalValue());
          g_pos = time6.InSeconds();
          g_pos_total = time5.InSeconds();

      if(g_menu_do && (e.xmotion.x > Button_W) && (e.xmotion.x < 1200) && e.xmotion.y > height - Button_H){
        //# Seek
        base::TimeDelta time = pipeline->GetMediaDuration();
       // base::TimeDelta time1 = pipeline->GetBufferedTime();
       // base::TimeDelta time2 = pipeline->GetCurrentTime();
        //pipeline->Seek(time*e.xbutton.x/width, NULL);
        pipeline->Seek(time*(e.xbutton.x-Button_W)/(width-Button_W), NULL);
            
           // ("buffer: %lld, current: %lld , duration: %lld,\n", time1, time2, time);

      }else if(g_menu_do && e.xmotion.x > 0 && e.xmotion.x <= Button_W && e.xmotion.y > height- Button_H){
            /*Play or Pause*/
        if (g_play_do == 0){ // Check Paused
              /*update button icon*/
              pipeline->SetPlaybackRate(1.0f); //# Set Play
          g_play_do = 1;
        }
            else{ //# Set Pause
              /*update button icon*/
              pipeline->SetPlaybackRate(0.0f);
          g_play_do = 0;
        }
            PaintPlayButton(g_display, g_window, g_play_do);
      }else if(g_menu_do && e.xmotion.x > 1200 && e.xmotion.y > 720){
        /*force quit*/
            exit(0);
      }else{
        g_menu_do = (g_menu_do + 1) & 0x1;
          }

        }
    //("\n <ButtonPre: %d [%d/%d]> ", e.xbutton.button,e.xmotion.x, e.xmotion.y);
    //g_curpos_x = e.xmotion.x; /// Qing
    //g_curpos_y = e.xmotion.y; /// Qing

    break;

      case ButtonRelease:

    //(" <ButtonRel: %d> ", e.xbutton.button);

    break;

      case KeyPress:
        {
          KeySym key = XKeycodeToKeysym(g_display, e.xkey.keycode, 0);
          if (key == XK_Escape) {
            g_running = false;
            // Quit message_loop only when pipeline is fully stopped.
            MessageLoopQuitter* quitter = new MessageLoopQuitter(message_loop);
            pipeline->Stop(NewCallback(quitter, &MessageLoopQuitter::Quit));
            return;

          } else if (key == XK_space) {
            if (pipeline->GetPlaybackRate() < 0.01f) // paused
              pipeline->SetPlaybackRate(1.0f);
            else
              pipeline->SetPlaybackRate(0.0f);
          }else if (key == XK_BackSpace){
            /*Stop and Restart*/
            if(pipeline->IsInitialized()){
              //MessageLoopQuitter* quitter = new MessageLoopQuitter(message_loop);
              //pipeline->Stop(NewCallback(quitter, &MessageLoopQuitter::Quit2));
              media::PipelineStatusNotification note;
              pipeline->Stop(note.Callback());
              note.Wait();

              if(pipeline->IsInitialized()){
                std::cout << "Fail To Stop Pipeline"<< std::endl;
              }else{
                std::cout << "Stop Pipeline"<< std::endl;
              }
            }else{
   
              if(PipelineRestart(pipeline)){
                std::cout<< "Restart Pipeline" << std::endl;
              }else{
                std::cout<< "Fail To Restart Pipeline" << std::endl;
              };

            }
            break;

          }
        }
        break;
      default:
        break;
    }
  }

#else
  // Consume all the X events
  while (XPending(g_display)) {
    XEvent e;
    XNextEvent(g_display, &e);
    switch (e.type) {
      case Expose:
        if (!audio_only) {
          // Tell the renderer to paint.
          DCHECK(Renderer::instance());
          Renderer::instance()->Paint();
        }
        break;
      case ButtonPress:
        {
          Window window;
          int x, y;
          unsigned int width, height, border_width, depth;
          XGetGeometry(g_display,
                       g_window,
                       &window,
                       &x,
                       &y,
                       &width,
                       &height,
                       &border_width,
                       &depth);
          base::TimeDelta time = pipeline->GetMediaDuration();
          pipeline->Seek(time*e.xbutton.x/width, NULL);
        }
        break;
      case KeyPress:
        {
          KeySym key = XKeycodeToKeysym(g_display, e.xkey.keycode, 0);
          if (key == XK_Escape) {
            g_running = false;
            // Quit message_loop only when pipeline is fully stopped.
            MessageLoopQuitter* quitter = new MessageLoopQuitter(message_loop);
            pipeline->Stop(NewCallback(quitter, &MessageLoopQuitter::Quit));
            return;
          } else if (key == XK_space) {
            if (pipeline->GetPlaybackRate() < 0.01f) // paused
              pipeline->SetPlaybackRate(1.0f);
            else
              pipeline->SetPlaybackRate(0.0f);
          }
        }
        break;
      default:
        break;
    }
  }

#endif
#endif

  message_loop->PostDelayedTask(FROM_HERE,
      NewRunnableFunction(PeriodicalUpdate, make_scoped_refptr(pipeline),
                          message_loop, audio_only), 10);
}

int main(int argc, char** argv) {
  // Read arguments.
  if (argc == 1) {
    std::cout << "Usage: " << argv[0] << " --file=FILE" << std::endl
              << std::endl
              << "Optional arguments:" << std::endl
              << "  [--enable-openmax]"
              << "  [--audio]"
              << "  [--alsa-device=DEVICE]" << std::endl
              << " Press [ESC] to stop" << std::endl
              << " Press [SPACE] to toggle pause/play" << std::endl
              << " Press [BackSpace] to toggle Pipeline Stop/Restart" << std::endl
              << " Press mouse left button to seek" << std::endl;
    return 1;
  }

  // Read command line.
  CommandLine::Init(argc, argv);
  std::string filename =
      CommandLine::ForCurrentProcess()->GetSwitchValueASCII("file");
  bool enable_audio = CommandLine::ForCurrentProcess()->HasSwitch("audio");
  bool audio_only = false;

  logging::InitLogging(
      NULL,
      logging::LOG_ONLY_TO_SYSTEM_DEBUG_LOG,
      logging::LOCK_LOG_FILE,  // Ignored.
      logging::DELETE_OLD_LOG_FILE,  // Ignored.
      logging::DISABLE_DCHECK_FOR_NON_OFFICIAL_RELEASE_BUILDS);

  // Install the signal handler.
  signal(SIGTERM, &TerminateHandler);
  signal(SIGINT, &TerminateHandler);

  // Initialize X11.
  if (!InitX11())
    return 1;

  // Initialize the pipeline thread and the pipeline.
  base::AtExitManager at_exit;
  scoped_ptr<media::MessageLoopFactory> message_loop_factory(
      new media::MessageLoopFactoryImpl());
  scoped_ptr<base::Thread> thread;
  scoped_refptr<media::PipelineImpl> pipeline;
  MessageLoop message_loop;
  thread.reset(new base::Thread("PipelineThread"));
  thread->Start();
  if (InitPipeline(thread->message_loop(), filename.c_str(),
                   enable_audio, &pipeline, &message_loop,
                   message_loop_factory.get())) {
    // Main loop of the application.
    g_running = true;

    // Check if video is present.
    audio_only = !pipeline->HasVideo();

    message_loop.PostTask(FROM_HERE,
        NewRunnableFunction(PeriodicalUpdate, pipeline,
                            &message_loop, audio_only));
    message_loop.Run();
  } else{
    std::cout << "Pipeline initialization failed..." << std::endl;
  }

  // Cleanup tasks.
  message_loop_factory.reset();

  thread->Stop();
  XDestroyWindow(g_display, g_window);
  XCloseDisplay(g_display);
  return 0;
}
