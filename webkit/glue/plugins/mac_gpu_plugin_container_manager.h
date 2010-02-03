// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_GLUE_PLUGINS_MAC_GPU_PLUGIN_CONTAINER_MANAGER_H_
#define WEBKIT_GLUE_PLUGINS_MAC_GPU_PLUGIN_CONTAINER_MANAGER_H_

#include <OpenGL/OpenGL.h>
#include <map>
#include <vector>

#include "app/gfx/native_widget_types.h"
#include "base/basictypes.h"

namespace webkit_glue {
struct WebPluginGeometry;
}

class MacGPUPluginContainer;

// Helper class that manages the backing store and on-screen rendering
// of instances of the GPU plugin on the Mac.
class MacGPUPluginContainerManager {
 public:
  MacGPUPluginContainerManager();

  // Allocates a new "fake" PluginWindowHandle, which is used as the
  // key for the other operations.
  gfx::PluginWindowHandle AllocateFakePluginWindowHandle();

  // Destroys a fake PluginWindowHandle and associated storage.
  void DestroyFakePluginWindowHandle(gfx::PluginWindowHandle id);

  // Sets the size and backing store of the plugin instance.
  void SetSizeAndBackingStore(gfx::PluginWindowHandle id,
                              int32 width,
                              int32 height,
                              uint64 io_surface_identifier);

  // Takes an update from WebKit about a plugin's position and size and moves
  // the plugin accordingly.
  void MovePluginContainer(const webkit_glue::WebPluginGeometry& move);

  // Draws all of the managed plugin containers into the given OpenGL
  // context, which must already be current.
  void Draw(CGLContextObj context);

  // Called by the container to enqueue its OpenGL texture objects for
  // deletion.
  void EnqueueTextureForDeletion(GLuint texture);

 private:
  uint32 current_id_;

  // Maps a "fake" plugin window handle to the corresponding container.
  MacGPUPluginContainer* MapIDToContainer(gfx::PluginWindowHandle id);

  // A map that associates plugin window handles with their containers.
  typedef std::map<gfx::PluginWindowHandle, MacGPUPluginContainer*>
      PluginWindowToContainerMap;
  PluginWindowToContainerMap plugin_window_to_container_map_;

  // A list of OpenGL textures waiting to be deleted
  std::vector<GLuint> textures_pending_deletion_;

  DISALLOW_COPY_AND_ASSIGN(MacGPUPluginContainerManager);
};

#endif  // WEBKIT_GLUE_PLUGINS_MAC_GPU_PLUGIN_CONTAINER_MANAGER_H_

