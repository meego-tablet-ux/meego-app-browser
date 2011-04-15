// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_C_PRIVATE_PROXY_PRIVATE_H_
#define PPAPI_C_PRIVATE_PROXY_PRIVATE_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_module.h"
#include "ppapi/c/pp_resource.h"

#define PPB_PROXY_PRIVATE_INTERFACE "PPB_Proxy_Private;4"

// Exposes functions needed by the out-of-process proxy to call into the
// renderer PPAPI implementation.
struct PPB_Proxy_Private {
  // Called when the given plugin process has crashed.
  void (*PluginCrashed)(PP_Module module);

  // Returns the instance for the given resource, or 0 on failure.
  PP_Instance (*GetInstanceForResource)(PP_Resource resource);

  // Sets a callback that will be used to make sure that PP_Instance IDs
  // are unique in the plugin.
  //
  // Since the plugin may be shared between several browser processes, we need
  // to do extra work to make sure that an instance ID is globally unqiue. The
  // given function will be called and will return true if the given
  // PP_Instance is OK to use in the plugin. It will then be marked as "in use"
  // On failure (returns false), the host implementation will generate a new
  // instance ID and try again.
  void (*SetReserveInstanceIDCallback)(
      PP_Module module,
      PP_Bool (*is_seen)(PP_Module, PP_Instance));

  // Returns the number of bytes synchronously readable out of the URLLoader's
  // buffer. Returns 0 on failure or if the url loader doesn't have any data
  // now.
  int32_t (*GetURLLoaderBufferedBytes)(PP_Resource url_loader);
};

#endif  // PPAPI_C_PRIVATE_PROXY_PRIVATE_H_
