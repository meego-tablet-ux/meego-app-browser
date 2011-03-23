// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_RENDERER_WEBAUDIODEVICE_IMPL_H_
#define CONTENT_RENDERER_RENDERER_WEBAUDIODEVICE_IMPL_H_

#include <vector>

#include "base/scoped_ptr.h"
#include "content/renderer/audio_device.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebAudioDevice.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebVector.h"

class RendererWebAudioDeviceImpl : public WebKit::WebAudioDevice,
                                   public AudioDevice::RenderCallback {
 public:
  RendererWebAudioDeviceImpl(size_t buffer_size,
                             int channels,
                             double sample_rate,
                             WebKit::WebAudioDevice::RenderCallback* callback);
  virtual ~RendererWebAudioDeviceImpl();

  // WebKit::WebAudioDevice implementation.
  virtual void start();
  virtual void stop();
  virtual double sampleRate() { return 44100.0; }

  // AudioDevice::RenderCallback implementation.
  virtual void Render(const std::vector<float*>& audio_data,
                      size_t number_of_frames);

 private:
  scoped_ptr<AudioDevice> audio_device_;

  // Weak reference to the callback into WebKit code.
  WebKit::WebAudioDevice::RenderCallback* client_callback_;

  DISALLOW_COPY_AND_ASSIGN(RendererWebAudioDeviceImpl);
};

#endif  // CONTENT_RENDERER_RENDERER_WEBAUDIODEVICE_IMPL_H_
