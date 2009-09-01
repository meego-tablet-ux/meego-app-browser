/*
 * Copyright 2009, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// This file contains the declarations for Texture2DStub and TextureCUBEStub.

#ifndef O3D_CONVERTER_CROSS_TEXTURE_STUB_H_
#define O3D_CONVERTER_CROSS_TEXTURE_STUB_H_

#include "core/cross/bitmap.h"
#include "core/cross/texture.h"
#include "core/cross/types.h"

namespace o3d {

// Texture2DStub implements the stub Texture2D interface for the converter.
class Texture2DStub : public Texture2D {
 public:
  typedef SmartPointer<Texture2DStub> Ref;

  Texture2DStub(ServiceLocator* service_locator,
                int width,
                int height,
                Texture::Format format,
                int levels,
                bool enable_render_surfaces)
      : Texture2D(service_locator,
                  width,
                  height,
                  format,
                  levels,
                  enable_render_surfaces) {}
  virtual ~Texture2DStub() {}

  // Overridden from Texture2D
  virtual void SetRect(int level,
                       unsigned left,
                       unsigned top,
                       unsigned width,
                       unsigned height,
                       const void* src_data,
                       int src_pitch) {
  }

  // Returns the implementation-specific texture handle for this texture.
  void* GetTextureHandle() const {
    return NULL;
  }

  // Gets a RGBASwizzleIndices that contains a mapping from
  // RGBA to the internal format used by the rendering API.
  virtual const RGBASwizzleIndices& GetABGR32FSwizzleIndices();

 protected:
  // Overridden from Texture2D
  virtual bool PlatformSpecificLock(
      int level, void** texture_data, int* pitch, AccessMode mode) {
    return false;
  }

  // Overridden from Texture2D
  virtual bool PlatformSpecificUnlock(int level) { return true; }

  // Overridden from Texture2D
  virtual RenderSurface::Ref PlatformSpecificGetRenderSurface(int mip_level) {
    return RenderSurface::Ref(NULL);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(Texture2DStub);
};


// TextureCUBEStub implements the TextureCUBE interface for the converter stub.
class TextureCUBEStub : public TextureCUBE {
 public:
  typedef SmartPointer<TextureCUBEStub> Ref;
  TextureCUBEStub(ServiceLocator* service_locator,
                  int edge_length,
                  Texture::Format format,
                  int levels,
                  bool enable_render_surfaces)
      : TextureCUBE(service_locator,
                    edge_length,
                    format,
                    levels,
                    enable_render_surfaces) {}
  virtual ~TextureCUBEStub() {}

  // Overridden from TextureCUBE
  virtual void SetRect(CubeFace face,
                       int level,
                       unsigned dst_left,
                       unsigned dst_top,
                       unsigned width,
                       unsigned height,
                       const void* src_data,
                       int src_pitch) {
  };

  // Returns the implementation-specific texture handle for this texture.
  void* GetTextureHandle() const {
    return NULL;
  }

  // Gets a RGBASwizzleIndices that contains a mapping from
  // RGBA to the internal format used by the rendering API.
  virtual const RGBASwizzleIndices& GetABGR32FSwizzleIndices();

 protected:
  // Overridden from TextureCUBE
  virtual bool PlatformSpecificLock(
      CubeFace face, int level, void** texture_data, int* pitch,
      AccessMode mode) {
    return false;
  }

  // Overridden from TextureCUBE
  virtual bool PlatformSpecificUnlock(CubeFace face, int level) { return true; }

  // Overridden from TextureCUBE
  virtual RenderSurface::Ref PlatformSpecificGetRenderSurface(CubeFace face,
                                                              int mip_level) {
    return RenderSurface::Ref(NULL);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TextureCUBEStub);
};

}  // namespace o3d

#endif  // O3D_CONVERTER_CROSS_TEXTURE_STUB_H_
