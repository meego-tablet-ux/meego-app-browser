// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_SERVICE_GL_CONTEXT_H_
#define GPU_COMMAND_BUFFER_SERVICE_GL_CONTEXT_H_

#include "build/build_config.h"
#include "gfx/native_widget_types.h"
#include "gfx/size.h"
#include "gpu/command_buffer/common/logging.h"

namespace gpu {

bool InitializeGLEW();

// Encapsulates an OpenGL context, hiding platform specific management.
class GLContext {
 public:
  GLContext();
  virtual ~GLContext();

  // Destroys the GL context.
  virtual void Destroy() = 0;

  // Makes the GL context current on the current thread.
  virtual bool MakeCurrent() = 0;

  // Returns true if this context is current.
  virtual bool IsCurrent() = 0;

  // Returns true if this context is offscreen.
  virtual bool IsOffscreen() = 0;

  // Swaps front and back buffers. This has no effect for off-screen
  // contexts.
  virtual void SwapBuffers() = 0;

  // Get the size of the back buffer.
  virtual gfx::Size GetSize() = 0;

  // Get the underlying platform specific GL context "handle".
  virtual void* GetHandle() = 0;

#if !defined(OS_MACOSX)
  // Create a GL context that renders directly to a view.
  static GLContext* CreateViewGLContext(gfx::PluginWindowHandle window,
                                        bool multisampled);
#endif

  // Create a GL context used for offscreen rendering. It is initially backed by
  // a 1x1 pbuffer. Use it to create an FBO to do useful rendering.
  static GLContext* CreateOffscreenGLContext(void* shared_handle);

 protected:
  bool InitializeCommon();

 private:
  DISALLOW_COPY_AND_ASSIGN(GLContext);
};

}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_SERVICE_GL_CONTEXT_H_
