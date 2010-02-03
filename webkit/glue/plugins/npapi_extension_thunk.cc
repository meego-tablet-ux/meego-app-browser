// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/glue/plugins/npapi_extension_thunk.h"

#include "base/logging.h"
#include "third_party/npapi/bindings/npapi_extensions.h"
#include "webkit/glue/plugins/plugin_instance.h"
#include "webkit/glue/webplugin.h"
#include "webkit/glue/webplugin_delegate.h"


// FindInstance()
// Finds a PluginInstance from an NPP.
// The caller must take a reference if needed.
static NPAPI::PluginInstance* FindInstance(NPP id) {
  if (id == NULL) {
    NOTREACHED();
    return NULL;
  }
  return static_cast<NPAPI::PluginInstance*>(id->ndata);
}

// 2D device API ---------------------------------------------------------------

static NPError Device2DQueryCapability(NPP id, int32 capability, int32* value) {
  scoped_refptr<NPAPI::PluginInstance> plugin = FindInstance(id);
  if (plugin) {
    plugin->webplugin()->delegate()->Device2DQueryCapability(capability, value);
    return NPERR_NO_ERROR;
  } else {
    return NPERR_GENERIC_ERROR;
  }
}

static NPError Device2DQueryConfig(NPP id,
                                   const NPDeviceConfig* request,
                                   NPDeviceConfig* obtain) {
  scoped_refptr<NPAPI::PluginInstance> plugin = FindInstance(id);
  if (plugin) {
    return plugin->webplugin()->delegate()->Device2DQueryConfig(
        static_cast<const NPDeviceContext2DConfig*>(request),
        static_cast<NPDeviceContext2DConfig*>(obtain));
  }
  return NPERR_GENERIC_ERROR;
}

static NPError Device2DInitializeContext(NPP id,
                                         const NPDeviceConfig* config,
                                         NPDeviceContext* context) {
  scoped_refptr<NPAPI::PluginInstance> plugin = FindInstance(id);
  if (plugin) {
    return plugin->webplugin()->delegate()->Device2DInitializeContext(
        static_cast<const NPDeviceContext2DConfig*>(config),
        static_cast<NPDeviceContext2D*>(context));
  }
  return NPERR_GENERIC_ERROR;
}

static NPError Device2DSetStateContext(NPP id,
                                       NPDeviceContext* context,
                                       int32 state,
                                       intptr_t value) {
  scoped_refptr<NPAPI::PluginInstance> plugin = FindInstance(id);
  if (plugin) {
    return plugin->webplugin()->delegate()->Device2DSetStateContext(
        static_cast<NPDeviceContext2D*>(context), state, value);
  }
  return NPERR_GENERIC_ERROR;
}

static NPError Device2DGetStateContext(NPP id,
                                       NPDeviceContext* context,
                                       int32 state,
                                       intptr_t* value) {
  scoped_refptr<NPAPI::PluginInstance> plugin = FindInstance(id);
  if (plugin) {
    return plugin->webplugin()->delegate()->Device2DGetStateContext(
        static_cast<NPDeviceContext2D*>(context), state, value);
  }
  return NPERR_GENERIC_ERROR;
}

static NPError Device2DFlushContext(NPP id,
                                    NPDeviceContext* context,
                                    NPDeviceFlushContextCallbackPtr callback,
                                    void* user_data) {
  scoped_refptr<NPAPI::PluginInstance> plugin = FindInstance(id);
  if (plugin) {
    NPError err = plugin->webplugin()->delegate()->Device2DFlushContext(
        id, static_cast<NPDeviceContext2D*>(context), callback, user_data);

    // Invoke the callback to inform the caller the work was done.
    // TODO(brettw) this is probably not how we want this to work, this should
    // happen when the frame is painted so the plugin knows when it can draw
    // the next frame.
    if (callback != NULL)
      (*callback)(id, context, err, user_data);

    // Return any errors.
    return err;
  }
  return NPERR_GENERIC_ERROR;
}

static NPError Device2DDestroyContext(NPP id,
                                      NPDeviceContext* context) {
  scoped_refptr<NPAPI::PluginInstance> plugin = FindInstance(id);
  if (plugin) {
    return plugin->webplugin()->delegate()->Device2DDestroyContext(
        static_cast<NPDeviceContext2D*>(context));
  }
  return NPERR_GENERIC_ERROR;
}

static NPError Device2DCreateBuffer(NPP id,
                                    NPDeviceContext* context,
                                    size_t size,
                                    int32* buffer_id) {
  return NPERR_GENERIC_ERROR;
}

static NPError Device2DDestroyBuffer(NPP id,
                                     NPDeviceContext* context,
                                     int32 buffer_id) {
  return NPERR_GENERIC_ERROR;
}

static NPError Device2DMapBuffer(NPP id,
                                 NPDeviceContext* context,
                                 int32 buffer_id,
                                 NPDeviceBuffer* buffer) {
  return NPERR_GENERIC_ERROR;
}

// 3D device API ---------------------------------------------------------------

static NPError Device3DQueryCapability(NPP id, int32 capability, int32* value) {
  scoped_refptr<NPAPI::PluginInstance> plugin = FindInstance(id);
  if (plugin) {
    plugin->webplugin()->delegate()->Device3DQueryCapability(capability, value);
    return NPERR_NO_ERROR;
  } else {
    return NPERR_GENERIC_ERROR;
  }
}

static NPError Device3DQueryConfig(NPP id,
                                   const NPDeviceConfig* request,
                                   NPDeviceConfig* obtain) {
  scoped_refptr<NPAPI::PluginInstance> plugin = FindInstance(id);
  if (plugin) {
    return plugin->webplugin()->delegate()->Device3DQueryConfig(
        static_cast<const NPDeviceContext3DConfig*>(request),
        static_cast<NPDeviceContext3DConfig*>(obtain));
  }
  return NPERR_GENERIC_ERROR;
}

static NPError Device3DInitializeContext(NPP id,
                                         const NPDeviceConfig* config,
                                         NPDeviceContext* context) {
  scoped_refptr<NPAPI::PluginInstance> plugin = FindInstance(id);
  if (plugin) {
    return plugin->webplugin()->delegate()->Device3DInitializeContext(
        static_cast<const NPDeviceContext3DConfig*>(config),
        static_cast<NPDeviceContext3D*>(context));
  }
  return NPERR_GENERIC_ERROR;
}

static NPError Device3DSetStateContext(NPP id,
                                       NPDeviceContext* context,
                                       int32 state,
                                       intptr_t value) {
  scoped_refptr<NPAPI::PluginInstance> plugin = FindInstance(id);
  if (plugin) {
    return plugin->webplugin()->delegate()->Device3DSetStateContext(
        static_cast<NPDeviceContext3D*>(context), state, value);
  }
  return NPERR_GENERIC_ERROR;
}

static NPError Device3DGetStateContext(NPP id,
                                       NPDeviceContext* context,
                                       int32 state,
                                       intptr_t* value) {
  scoped_refptr<NPAPI::PluginInstance> plugin = FindInstance(id);
  if (plugin) {
    return plugin->webplugin()->delegate()->Device3DGetStateContext(
        static_cast<NPDeviceContext3D*>(context), state, value);
  }
  return NPERR_GENERIC_ERROR;
}

static NPError Device3DFlushContext(NPP id,
                                    NPDeviceContext* context,
                                    NPDeviceFlushContextCallbackPtr callback,
                                    void* user_data) {
  scoped_refptr<NPAPI::PluginInstance> plugin = FindInstance(id);
  if (plugin) {
    return plugin->webplugin()->delegate()->Device3DFlushContext(
        id, static_cast<NPDeviceContext3D*>(context), callback, user_data);
  }
  return NPERR_GENERIC_ERROR;
}

static NPError Device3DDestroyContext(NPP id,
                                      NPDeviceContext* context) {
  scoped_refptr<NPAPI::PluginInstance> plugin = FindInstance(id);
  if (plugin) {
    return plugin->webplugin()->delegate()->Device3DDestroyContext(
        static_cast<NPDeviceContext3D*>(context));
  }
  return NPERR_GENERIC_ERROR;
}

static NPError Device3DCreateBuffer(NPP id,
                                    NPDeviceContext* context,
                                    size_t size,
                                    int32* buffer_id) {
  scoped_refptr<NPAPI::PluginInstance> plugin = FindInstance(id);
  if (plugin) {
    return plugin->webplugin()->delegate()->Device3DCreateBuffer(
        static_cast<NPDeviceContext3D*>(context), size, buffer_id);
  }
  return NPERR_GENERIC_ERROR;
}

static NPError Device3DDestroyBuffer(NPP id,
                                     NPDeviceContext* context,
                                     int32 buffer_id) {
  scoped_refptr<NPAPI::PluginInstance> plugin = FindInstance(id);
  if (plugin) {
    return plugin->webplugin()->delegate()->Device3DDestroyBuffer(
        static_cast<NPDeviceContext3D*>(context), buffer_id);
  }
  return NPERR_GENERIC_ERROR;
}

static NPError Device3DMapBuffer(NPP id,
                                 NPDeviceContext* context,
                                 int32 buffer_id,
                                 NPDeviceBuffer* buffer) {
  scoped_refptr<NPAPI::PluginInstance> plugin = FindInstance(id);
  if (plugin) {
    return plugin->webplugin()->delegate()->Device3DMapBuffer(
        static_cast<NPDeviceContext3D*>(context), buffer_id, buffer);
  }
  return NPERR_GENERIC_ERROR;
}

// Audio device API ------------------------------------------------------------

static NPError DeviceAudioQueryCapability(NPP id, int32 capability,
                                          int32* value) {
  scoped_refptr<NPAPI::PluginInstance> plugin = FindInstance(id);
  if (plugin) {
    plugin->webplugin()->delegate()->DeviceAudioQueryCapability(capability,
                                                                value);
    return NPERR_NO_ERROR;
  } else {
    return NPERR_GENERIC_ERROR;
  }
}

static NPError DeviceAudioQueryConfig(NPP id,
                                      const NPDeviceConfig* request,
                                      NPDeviceConfig* obtain) {
  scoped_refptr<NPAPI::PluginInstance> plugin = FindInstance(id);
  if (plugin) {
    return plugin->webplugin()->delegate()->DeviceAudioQueryConfig(
        static_cast<const NPDeviceContextAudioConfig*>(request),
        static_cast<NPDeviceContextAudioConfig*>(obtain));
  }
  return NPERR_GENERIC_ERROR;
}

static NPError DeviceAudioInitializeContext(NPP id,
                                            const NPDeviceConfig* config,
                                            NPDeviceContext* context) {
  scoped_refptr<NPAPI::PluginInstance> plugin = FindInstance(id);
  if (plugin) {
    return plugin->webplugin()->delegate()->DeviceAudioInitializeContext(
        static_cast<const NPDeviceContextAudioConfig*>(config),
        static_cast<NPDeviceContextAudio*>(context));
  }
  return NPERR_GENERIC_ERROR;
}

static NPError DeviceAudioSetStateContext(NPP id,
                                          NPDeviceContext* context,
                                          int32 state,
                                          intptr_t value) {
  scoped_refptr<NPAPI::PluginInstance> plugin = FindInstance(id);
  if (plugin) {
    return plugin->webplugin()->delegate()->DeviceAudioSetStateContext(
        static_cast<NPDeviceContextAudio*>(context), state, value);
  }
  return NPERR_GENERIC_ERROR;
}

static NPError DeviceAudioGetStateContext(NPP id,
                                          NPDeviceContext* context,
                                          int32 state,
                                          intptr_t* value) {
  scoped_refptr<NPAPI::PluginInstance> plugin = FindInstance(id);
  return plugin->webplugin()->delegate()->DeviceAudioGetStateContext(
      static_cast<NPDeviceContextAudio*>(context), state, value);
}

static NPError DeviceAudioFlushContext(NPP id,
                                       NPDeviceContext* context,
                                       NPDeviceFlushContextCallbackPtr callback,
                                       void* user_data) {
  scoped_refptr<NPAPI::PluginInstance> plugin = FindInstance(id);
  return plugin->webplugin()->delegate()->DeviceAudioFlushContext(
      id, static_cast<NPDeviceContextAudio*>(context), callback, user_data);
}

static NPError DeviceAudioDestroyContext(NPP id,
                                         NPDeviceContext* context) {
  scoped_refptr<NPAPI::PluginInstance> plugin = FindInstance(id);
  return plugin->webplugin()->delegate()->DeviceAudioDestroyContext(
      static_cast<NPDeviceContextAudio*>(context));
}
// -----------------------------------------------------------------------------

static NPDevice* AcquireDevice(NPP id, NPDeviceID device_id) {
  static NPDevice device_2d = {
    Device2DQueryCapability,
    Device2DQueryConfig,
    Device2DInitializeContext,
    Device2DSetStateContext,
    Device2DGetStateContext,
    Device2DFlushContext,
    Device2DDestroyContext,
    Device2DCreateBuffer,
    Device2DDestroyBuffer,
    Device2DMapBuffer,
  };
  static NPDevice device_3d = {
    Device3DQueryCapability,
    Device3DQueryConfig,
    Device3DInitializeContext,
    Device3DSetStateContext,
    Device3DGetStateContext,
    Device3DFlushContext,
    Device3DDestroyContext,
    Device3DCreateBuffer,
    Device3DDestroyBuffer,
    Device3DMapBuffer,
  };
  static NPDevice device_audio = {
      DeviceAudioQueryCapability,
      DeviceAudioQueryConfig,
      DeviceAudioInitializeContext,
      DeviceAudioSetStateContext,
      DeviceAudioGetStateContext,
      DeviceAudioFlushContext,
      DeviceAudioDestroyContext,
  };

  switch (device_id) {
    case NPPepper2DDevice:
      return const_cast<NPDevice*>(&device_2d);
    case NPPepper3DDevice:
      return const_cast<NPDevice*>(&device_3d);
    case NPPepperAudioDevice:
      return const_cast<NPDevice*>(&device_audio);
    default:
      return NULL;
  }
}

namespace NPAPI {

NPError GetPepperExtensionsFunctions(void* value) {
  static const NPExtensions kExtensions = {
    &AcquireDevice,
  };

  // Return a pointer to the canonical function table.
  NPExtensions* extensions = const_cast<NPExtensions*>(&kExtensions);
  NPExtensions** exts = reinterpret_cast<NPExtensions**>(value);
  *exts = extensions;
  return NPERR_NO_ERROR;
}

}  // namespace NPAPI
