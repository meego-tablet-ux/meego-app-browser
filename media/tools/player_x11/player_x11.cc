// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <signal.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/file_path.h"
#include "base/scoped_ptr.h"
#include "base/thread.h"
#include "media/base/callback.h"
#include "media/base/media.h"
#include "media/base/media_switches.h"
#include "media/base/pipeline_impl.h"
#include "media/filters/audio_renderer_impl.h"
#include "media/filters/ffmpeg_audio_decoder.h"
#include "media/filters/ffmpeg_demuxer.h"
#include "media/filters/ffmpeg_video_decoder.h"
#include "media/filters/file_data_source.h"
#include "media/filters/null_audio_renderer.h"
#include "media/filters/omx_video_decoder.h"

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

void Quit(MessageLoop* message_loop) {
  message_loop->PostTask(FROM_HERE, new MessageLoop::QuitTask());
}

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
  XStoreName(g_display, g_window, "X11 Media Player");

  XSelectInput(g_display, g_window,
               ExposureMask | ButtonPressMask | KeyPressMask);
  XMapWindow(g_display, g_window);
  return true;
}

bool InitPipeline(MessageLoop* message_loop,
                  const char* filename, bool enable_audio,
                  scoped_refptr<media::PipelineImpl>* pipeline,
                  MessageLoop* paint_message_loop) {
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
  scoped_refptr<media::FilterFactoryCollection> factories =
      new media::FilterFactoryCollection();
  factories->AddFactory(media::FileDataSource::CreateFactory());
  factories->AddFactory(media::FFmpegAudioDecoder::CreateFactory());
  factories->AddFactory(media::FFmpegDemuxer::CreateFilterFactory());
  if (CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableOpenMax)) {
    factories->AddFactory(media::OmxVideoDecoder::CreateFactory());
  }
  factories->AddFactory(media::FFmpegVideoDecoder::CreateFactory());
  factories->AddFactory(Renderer::CreateFactory(g_display, g_window,
                                                paint_message_loop));

  if (enable_audio) {
    factories->AddFactory(media::AudioRendererImpl::CreateFilterFactory());
  } else {
    factories->AddFactory(media::NullAudioRenderer::CreateFilterFactory());
  }

  // Creates the pipeline and start it.
  *pipeline = new media::PipelineImpl(message_loop);
  (*pipeline)->Start(factories, filename, NULL);

  // Wait until the pipeline is fully initialized.
  while (true) {
    PlatformThread::Sleep(100);
    if ((*pipeline)->IsInitialized())
      break;
    if ((*pipeline)->GetError() != media::PIPELINE_OK) {
      (*pipeline)->Stop(NULL);
      return false;
    }
  }

  // And starts the playback.
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
    // interrupt signal is received during lat time period.
    // Quit message_loop only when pipeline is fully stopped.
    pipeline->Stop(media::TaskToCallbackAdapter::NewCallback(
        NewRunnableFunction(Quit, message_loop)));
    return;
  }

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
            pipeline->Stop(media::TaskToCallbackAdapter::NewCallback(
                NewRunnableFunction(Quit, message_loop)));
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

  message_loop->PostDelayedTask(FROM_HERE,
      NewRunnableFunction(PeriodicalUpdate, pipeline,
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
              << " Press mouse left button to seek" << std::endl;
    return 1;
  }

  // Read command line.
  CommandLine::Init(argc, argv);
  std::string filename =
      CommandLine::ForCurrentProcess()->GetSwitchValueASCII("file");
  bool enable_audio = CommandLine::ForCurrentProcess()->HasSwitch("audio");
  bool audio_only = false;

  // Install the signal handler.
  signal(SIGTERM, &TerminateHandler);
  signal(SIGINT, &TerminateHandler);

  // Initialize X11.
  if (!InitX11())
    return 1;

  // Initialize the pipeline thread and the pipeline.
  base::AtExitManager at_exit;
  scoped_ptr<base::Thread> thread;
  scoped_refptr<media::PipelineImpl> pipeline;
  MessageLoop message_loop;
  thread.reset(new base::Thread("PipelineThread"));
  thread->Start();
  if (InitPipeline(thread->message_loop(), filename.c_str(),
                   enable_audio, &pipeline, &message_loop)) {
    // Main loop of the application.
    g_running = true;

    // Check if video is present.
    audio_only = !pipeline->IsRendered(media::mime_type::kMajorTypeVideo);

    message_loop.PostTask(FROM_HERE,
        NewRunnableFunction(PeriodicalUpdate, pipeline.get(),
                            &message_loop, audio_only));
    message_loop.Run();
  } else{
    std::cout << "Pipeline initialization failed..." << std::endl;
  }

  // Cleanup tasks.
  thread->Stop();
  XDestroyWindow(g_display, g_window);
  XCloseDisplay(g_display);
  return 0;
}
