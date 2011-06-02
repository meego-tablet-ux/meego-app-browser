// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_GLUE_PLUGINS_QT_PLUGIN_CONTAINER_MANAGER_HOST_DELEGATE_H_
#define WEBKIT_GLUE_PLUGINS_QT_PLUGIN_CONTAINER_MANAGER_HOST_DELEGATE_H_

#include "ui/gfx/native_widget_types.h"
// used by QtPluginContainerManager to call back to it's owner.

namespace webkit {
namespace npapi {

class QtPluginContainerManagerHostDelegate {
 public:
  virtual void OnCloseFSPluginWindow(gfx::PluginWindowHandle id) = 0;
};

}
}

#endif // WEBKIT_GLUE_PLUGINS_QT_PLUGIN_CONTAINER_MANAGER_HOST_DELEGATE_H_
