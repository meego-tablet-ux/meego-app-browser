// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_GL_GL_SURFACE_EGL_H_
#define UI_GFX_GL_GL_SURFACE_EGL_H_
#pragma once

#include "ui/gfx/gl/gl_surface.h"
#include "ui/gfx/size.h"

typedef void* EGLConfig;
typedef void* EGLDisplay;
typedef void* EGLSurface;

namespace gfx {

// Interface for EGL contexts.
class GLSurfaceEGL : public GLSurface {
 public:
  GLSurfaceEGL();
  virtual ~GLSurfaceEGL();

  static bool InitializeOneOff();
  static EGLDisplay GetDisplay();
  static EGLConfig GetConfig();

 private:
  DISALLOW_COPY_AND_ASSIGN(GLSurfaceEGL);
};

// Encapsulates an EGL surface bound to a view.
class NativeViewGLSurfaceEGL : public GLSurfaceEGL {
 public:
  explicit NativeViewGLSurfaceEGL(void* window);
  virtual ~NativeViewGLSurfaceEGL();

  // Initialize an EGL context.
  bool Initialize();

  // Implement GLSurface.
  virtual void Destroy();
  virtual bool IsOffscreen();
  virtual bool SwapBuffers();
  virtual gfx::Size GetSize();
  virtual EGLSurface GetHandle();

 private:
  void* window_;
  EGLSurface surface_;

  DISALLOW_COPY_AND_ASSIGN(NativeViewGLSurfaceEGL);
};

// Encapsulates a pbuffer EGL surface.
class PbufferGLSurfaceEGL : public GLSurfaceEGL {
 public:
  explicit PbufferGLSurfaceEGL(const gfx::Size& size);
  virtual ~PbufferGLSurfaceEGL();

  // Initialize an EGL context that shares a namespace with another.
  bool Initialize();

  // Implement GLSurface.
  virtual void Destroy();
  virtual bool IsOffscreen();
  virtual bool SwapBuffers();
  virtual gfx::Size GetSize();
  virtual EGLSurface GetHandle();

 private:
  gfx::Size size_;
  EGLSurface surface_;

  DISALLOW_COPY_AND_ASSIGN(PbufferGLSurfaceEGL);
};

}  // namespace gfx

#endif  // UI_GFX_GL_GL_SURFACE_EGL_H_
