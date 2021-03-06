// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_PLUGINS_NPAPI_PLUGIN_LIB_H_
#define WEBKIT_PLUGINS_NPAPI_PLUGIN_LIB_H_

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/memory/ref_counted.h"
#include "base/native_library.h"
#include "build/build_config.h"
#include "webkit/plugins/npapi/plugin_list.h"
#include "webkit/plugins/npapi/webplugin.h"

class FilePath;
struct WebPluginInfo;

namespace webkit {
namespace npapi {

class PluginInstance;

// A PluginLib is a single NPAPI Plugin Library, and is the lifecycle
// manager for new PluginInstances.
class PluginLib : public base::RefCounted<PluginLib> {
 public:
  static PluginLib* CreatePluginLib(const FilePath& filename);

  // Creates a WebPluginInfo structure given a plugin's path.  On success
  // returns true, with the information being put into "info".
  // Returns false if the library couldn't be found, or if it's not a plugin.
  static bool ReadWebPluginInfo(const FilePath& filename, WebPluginInfo* info);

#if defined(OS_POSIX) && !defined(OS_MACOSX)
  // Parse the result of an NP_GetMIMEDescription() call.
  // This API is only used on Unixes, and is exposed here for testing.
  static void ParseMIMEDescription(const std::string& description,
                                   std::vector<WebPluginMimeType>* mime_types);

  // Extract a version number from a description string.
  // This API is only used on Unixes, and is exposed here for testing.
  static void ExtractVersionString(const std::string& version,
                                   WebPluginInfo* info);
#endif

  // Unloads all the loaded plugin libraries and cleans up the plugin map.
  static void UnloadAllPlugins();

  // Shuts down all loaded plugin instances.
  static void ShutdownAllPlugins();

#if defined(TOOLKIT_MEEGOTOUCH)
  static void OnOrientationChanged(int orientation);
#endif

  // Get the Plugin's function pointer table.
  NPPluginFuncs* functions();

  // Creates a new instance of this plugin.
  PluginInstance* CreateInstance(const std::string& mime_type);

  // Called by the instance when the instance is tearing down.
  void CloseInstance();

  // Gets information about this plugin and the mime types that it
  // supports.
  const WebPluginInfo& plugin_info() { return web_plugin_info_; }

  bool internal() { return internal_; }

  //
  // NPAPI functions
  //

  // NPAPI method to initialize a Plugin.
  // Initialize can be safely called multiple times
  NPError NP_Initialize();

  // NPAPI method to shutdown a Plugin.
  void NP_Shutdown(void);

  // NPAPI method to clear locally stored data (LSO's or "Flash cookies").
  NPError NP_ClearSiteData(const char* site, uint64 flags, uint64 max_age);

  // NPAPI method to get a NULL-terminated list of all sites under which data
  // is stored.
  char** NP_GetSitesWithData();

#if defined(TOOLKIT_MEEGOTOUCH)
  // NPAPI method to set orientation for all instances
  void NP_SetOrientation(int orientation);
#endif

  int instance_count() const { return instance_count_; }
#if defined(TOOLKIT_MEEGOTOUCH)
  int flash_playing_count() const { return flash_playing_count_; }
#endif

  // Prevents the library code from being unload when Unload() is called (since
  // some plugins crash if unloaded).
  void PreventLibraryUnload();

#if defined(TOOLKIT_MEEGOTOUCH)
  // Trigger screen saver if needed
  void OnFlashInstancePaused(bool paused);
#endif

  // protected for testability.
 protected:
  friend class base::RefCounted<PluginLib>;

  // Creates a new PluginLib.
  // |entry_points| is non-NULL for internal plugins.
  PluginLib(const WebPluginInfo& info,
            const PluginEntryPoints* entry_points);

  virtual ~PluginLib();

  // Attempts to load the plugin from the library.
  // Returns true if it is a legitimate plugin, false otherwise
  bool Load();

  // Unloads the plugin library.
  void Unload();

  // Shutdown the plugin library.
  void Shutdown();

 private:
  bool internal_;  // True for plugins that are built-in into chrome binaries.
  WebPluginInfo web_plugin_info_;  // Supported mime types, description
  base::NativeLibrary library_;  // The opened library reference.
  NPPluginFuncs plugin_funcs_;  // The struct of plugin side functions.
  bool initialized_;  // Is the plugin initialized?
  NPSavedData *saved_data_;  // Persisted plugin info for NPAPI.
  int instance_count_;  // Count of plugins in use.
#if defined(TOOLKIT_MEEGOTOUCH)
  int flash_playing_count_;  // Count of flash plugins is playing.
#endif
  bool skip_unload_;  // True if library_ should not be unloaded.

  // Function pointers to entry points into the plugin.
  PluginEntryPoints entry_points_;

  DISALLOW_COPY_AND_ASSIGN(PluginLib);
};

}  // namespace npapi
}  // namespace webkit

#endif  // WEBKIT_PLUGINS_NPAPI_PLUGIN_LIB_H_
