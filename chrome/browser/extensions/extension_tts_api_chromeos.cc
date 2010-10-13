// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_tts_api.h"

#include "base/singleton.h"
#include "base/string_number_conversions.h"
#include "chrome/browser/chromeos/cros/cros_library.h"
#include "chrome/browser/chromeos/cros/speech_synthesis_library.h"

namespace util = extension_tts_api_util;

using base::DoubleToString;

namespace {
const char kCrosLibraryNotLoadedError[] = "Cros shared library not loaded.";
};

class ExtensionTtsPlatformImplChromeOs : public ExtensionTtsPlatformImpl {
 public:
  virtual bool Speak(
      const std::string& utterance,
      const std::string& language,
      const std::string& gender,
      double rate,
      double pitch,
      double volume);

  virtual bool StopSpeaking();

  virtual bool IsSpeaking();

  // Get the single instance of this class.
  static ExtensionTtsPlatformImplChromeOs* GetInstance();

 private:
  ExtensionTtsPlatformImplChromeOs() {}
  virtual ~ExtensionTtsPlatformImplChromeOs() {}

  friend struct DefaultSingletonTraits<ExtensionTtsPlatformImplChromeOs>;

  DISALLOW_COPY_AND_ASSIGN(ExtensionTtsPlatformImplChromeOs);
};

// static
ExtensionTtsPlatformImpl* ExtensionTtsPlatformImpl::GetInstance() {
  return ExtensionTtsPlatformImplChromeOs::GetInstance();
}

bool ExtensionTtsPlatformImplChromeOs::Speak(
    const std::string& utterance,
    const std::string& language,
    const std::string& gender,
    double rate,
    double pitch,
    double volume) {
  chromeos::CrosLibrary* cros_library = chromeos::CrosLibrary::Get();
  if (!cros_library->EnsureLoaded()) {
    set_error(kCrosLibraryNotLoadedError);
    return false;
  }

  std::string options;

  if (!language.empty()) {
    util::AppendSpeakOption(
        std::string(util::kNameKey), language, &options);
  }

  if (!gender.empty()) {
    util::AppendSpeakOption(
        std::string(util::kGenderKey), gender, &options);
  }

  if (rate >= 0.0) {
    util::AppendSpeakOption(
        std::string(util::kRateKey), DoubleToString(rate * 5), &options);
  }

  if (pitch >= 0.0) {
    // The TTS service allows a range of 0 to 2 for speech pitch.
    util::AppendSpeakOption(
        std::string(util::kPitchKey), DoubleToString(pitch * 2), &options);
  }

  if (volume >= 0.0) {
    // The TTS service allows a range of 0 to 5 for speech volume.
    util::AppendSpeakOption(
        std::string(util::kVolumeKey), DoubleToString(volume * 5), &options);
  }

  if (!options.empty()) {
    cros_library->GetSpeechSynthesisLibrary()->SetSpeakProperties(
        options.c_str());
  }

  return cros_library->GetSpeechSynthesisLibrary()->Speak(utterance.c_str());
}

bool ExtensionTtsPlatformImplChromeOs::StopSpeaking() {
  if (chromeos::CrosLibrary::Get()->EnsureLoaded()) {
    return chromeos::CrosLibrary::Get()->GetSpeechSynthesisLibrary()->
        StopSpeaking();
  }

  set_error(kCrosLibraryNotLoadedError);
  return false;
}

bool ExtensionTtsPlatformImplChromeOs::IsSpeaking() {
  if (chromeos::CrosLibrary::Get()->EnsureLoaded()) {
    return chromeos::CrosLibrary::Get()->GetSpeechSynthesisLibrary()->
        IsSpeaking();
  }

  set_error(kCrosLibraryNotLoadedError);
  return false;
}

// static
ExtensionTtsPlatformImplChromeOs*
ExtensionTtsPlatformImplChromeOs::GetInstance() {
  return Singleton<ExtensionTtsPlatformImplChromeOs>::get();
}
