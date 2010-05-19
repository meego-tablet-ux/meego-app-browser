// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/support/platform_support.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#import <objc/objc-runtime.h>

#include "base/data_pack.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/mac_util.h"
#include "base/path_service.h"
#include "base/string16.h"
#include "grit/webkit_resources.h"
#include "third_party/WebKit/WebKit/mac/WebCoreSupport/WebSystemInterface.h"
#import "webkit/tools/test_shell/mac/DumpRenderTreePasteboard.h"

static base::DataPack* g_resource_data_pack = NULL;

namespace webkit_support {

static NSAutoreleasePool* autorelease_pool;

void BeforeInitialize() {
  // Need to initialize NSAutoreleasePool before InitWebCoreSystemInterface().
  autorelease_pool = [[NSAutoreleasePool alloc] init];
  DCHECK(autorelease_pool);
  InitWebCoreSystemInterface();
}

#if OBJC_API_VERSION == 2
static void SwizzleAllMethods(Class imposter, Class original) {
  unsigned int imposterMethodCount = 0;
  Method* imposterMethods =
      class_copyMethodList(imposter, &imposterMethodCount);

  unsigned int originalMethodCount = 0;
  Method* originalMethods =
      class_copyMethodList(original, &originalMethodCount);

  for (unsigned int i = 0; i < imposterMethodCount; i++) {
    SEL imposterMethodName = method_getName(imposterMethods[i]);

    // Attempt to add the method to the original class.  If it fails, the method
    // already exists and we should instead exchange the implementations.
    if (class_addMethod(original,
                        imposterMethodName,
                        method_getImplementation(originalMethods[i]),
                        method_getTypeEncoding(originalMethods[i]))) {
      continue;
    }

    unsigned int j = 0;
    for (; j < originalMethodCount; j++) {
      SEL originalMethodName = method_getName(originalMethods[j]);
      if (sel_isEqual(imposterMethodName, originalMethodName)) {
        break;
      }
    }

    // If class_addMethod failed above then the method must exist on the
    // original class.
    DCHECK(j < originalMethodCount) << "method wasn't found?";
    method_exchangeImplementations(imposterMethods[i], originalMethods[j]);
  }

  if (imposterMethods) {
    free(imposterMethods);
  }
  if (originalMethods) {
    free(originalMethods);
  }
}
#endif

static void SwizzleNSPasteboard() {
  // We replace NSPaseboard w/ the shim (from WebKit) that avoids having
  // sideeffects w/ whatever the user does at the same time.

  Class imposterClass = objc_getClass("DumpRenderTreePasteboard");
  Class originalClass = objc_getClass("NSPasteboard");
#if OBJC_API_VERSION == 0
  class_poseAs(imposterClass, originalClass);
#else
  // Swizzle instance methods...
  SwizzleAllMethods(imposterClass, originalClass);
  // and then class methods.
  SwizzleAllMethods(object_getClass(imposterClass),
                    object_getClass(originalClass));
#endif
}

void AfterInitialize() {
  // Load a data pack.
  g_resource_data_pack = new base::DataPack;
  NSString* resource_path =
      [mac_util::MainAppBundle() pathForResource:@"DumpRenderTree"
                                          ofType:@"pak"];
  FilePath resources_pak_path([resource_path fileSystemRepresentation]);
  if (!g_resource_data_pack->Load(resources_pak_path)) {
    LOG(FATAL) << "failed to load DumpRenderTree.pak";
  }

  // Load font files in the resource folder.
  static const char* const fontFileNames[] = {
      "AHEM____.TTF",
      "WebKitWeightWatcher100.ttf",
      "WebKitWeightWatcher200.ttf",
      "WebKitWeightWatcher300.ttf",
      "WebKitWeightWatcher400.ttf",
      "WebKitWeightWatcher500.ttf",
      "WebKitWeightWatcher600.ttf",
      "WebKitWeightWatcher700.ttf",
      "WebKitWeightWatcher800.ttf",
      "WebKitWeightWatcher900.ttf",
  };

  NSString* resources = [[NSBundle mainBundle] resourcePath];
  for (unsigned i = 0; i < arraysize(fontFileNames); ++i) {
    const char* resource_path = [[resources stringByAppendingPathComponent:
        [NSString stringWithUTF8String:fontFileNames[i]]] UTF8String];
    FSRef resource_ref;
    const UInt8* uint8_resource_path
        = reinterpret_cast<const UInt8*>(resource_path);
    if (FSPathMakeRef(uint8_resource_path, &resource_ref, nil) != noErr) {
      DLOG(FATAL) << "Fail to open " << resource_path;
    }
    if (ATSFontActivateFromFileReference(&resource_ref, kATSFontContextLocal,
        kATSFontFormatUnspecified, 0, kATSOptionFlagsDefault, 0) != noErr) {
      DLOG(FATAL) << "Fail to activate font: %s" << resource_path;
    }
  }

  SwizzleNSPasteboard();
}

void BeforeShutdown() {
}

void AfterShutdown() {
  [DumpRenderTreePasteboard releaseLocalPasteboards];
  [autorelease_pool drain];
  delete g_resource_data_pack;
}

}  // namespace webkit_support

namespace webkit_glue {

string16 GetLocalizedString(int message_id) {
  base::StringPiece res;
  if (!g_resource_data_pack->GetStringPiece(message_id, &res)) {
    LOG(FATAL) << "failed to load webkit string with id " << message_id;
  }
  return string16(reinterpret_cast<const char16*>(res.data()),
                  res.length() / 2);
}

// Helper method for getting the path to the test shell resources directory.
static FilePath GetResourcesFilePath() {
  FilePath path;
  // We assume the application is bundled.
  if (!mac_util::AmIBundled()) {
    LOG(FATAL) << "Failed to locate resources. The applicaiton is not bundled.";
  }
  PathService::Get(base::DIR_EXE, &path);
  path = path.Append(FilePath::kParentDirectory);
  return path.AppendASCII("Resources");
}

base::StringPiece GetDataResource(int resource_id) {
  switch (resource_id) {
  case IDR_BROKENIMAGE: {
    // Use webkit's broken image icon (16x16)
    static std::string broken_image_data;
    if (broken_image_data.empty()) {
      FilePath path = GetResourcesFilePath();
      // In order to match WebKit's colors for the missing image, we have to
      // use a PNG. The GIF doesn't have the color range needed to correctly
      // match the TIFF they use in Safari.
      path = path.AppendASCII("missingImage.png");
      bool success = file_util::ReadFileToString(path.ToWStringHack(),
                                                 &broken_image_data);
      if (!success) {
        LOG(FATAL) << "Failed reading: " << path.value();
      }
    }
    return broken_image_data;
  }
  case IDR_FEED_PREVIEW:
    // It is necessary to return a feed preview template that contains
    // a {{URL}} substring where the feed URL should go; see the code
    // that computes feed previews in feed_preview.cc:MakeFeedPreview.
    // This fixes issue #932714.
    return "Feed preview for {{URL}}";
  case IDR_TEXTAREA_RESIZER: {
    // Use webkit's text area resizer image.
    static std::string resize_corner_data;
    if (resize_corner_data.empty()) {
      FilePath path = GetResourcesFilePath();
      path = path.AppendASCII("textAreaResizeCorner.png");
      bool success = file_util::ReadFileToString(path.ToWStringHack(),
                                                 &resize_corner_data);
      if (!success) {
        LOG(FATAL) << "Failed reading: " << path.value();
      }
    }
    return resize_corner_data;
  }

  case IDR_SEARCH_CANCEL:
  case IDR_SEARCH_CANCEL_PRESSED:
  case IDR_SEARCH_MAGNIFIER:
  case IDR_SEARCH_MAGNIFIER_RESULTS:
  case IDR_MEDIA_PAUSE_BUTTON:
  case IDR_MEDIA_PLAY_BUTTON:
  case IDR_MEDIA_PLAY_BUTTON_DISABLED:
  case IDR_MEDIA_SOUND_FULL_BUTTON:
  case IDR_MEDIA_SOUND_NONE_BUTTON:
  case IDR_MEDIA_SOUND_DISABLED:
  case IDR_MEDIA_SLIDER_THUMB:
  case IDR_MEDIA_VOLUME_SLIDER_THUMB: {
    base::StringPiece res;
    g_resource_data_pack->GetStringPiece(resource_id, &res);
    return res;
  }

  default:
    break;
  }
  return base::StringPiece();
}

}  // namespace webkit_glue
