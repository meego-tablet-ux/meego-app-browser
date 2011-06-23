// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_NIX_XDG_UTIL_H_
#define BASE_NIX_XDG_UTIL_H_
#pragma once

// XDG refers to http://en.wikipedia.org/wiki/Freedesktop.org .
// This file contains utilities found across free desktop environments.
//
// TODO(brettw) this file should be in app/x11, but is currently used by
// net. We should have a net API to allow the embedder to specify the behavior
// that it uses XDG for, and then move this file.

#ifdef nix
#error asdf
#endif

class FilePath;

namespace base {

class Environment;

namespace nix {

// Utility function for getting XDG directories.
// |env_name| is the name of an environment variable that we want to use to get
// a directory path. |fallback_dir| is the directory relative to $HOME that we
// use if |env_name| cannot be found or is empty. |fallback_dir| may be NULL.
// Examples of |env_name| are XDG_CONFIG_HOME and XDG_DATA_HOME.
FilePath GetXDGDirectory(Environment* env, const char* env_name,
                         const char* fallback_dir);

// Wrapper around xdg_user_dir_lookup() from src/base/third_party/xdg-user-dirs
// This looks up "well known" user directories like the desktop and music
// folder. Examples of |dir_name| are DESKTOP and MUSIC.
FilePath GetXDGUserDirectory(Environment* env, const char* dir_name,
                             const char* fallback_dir);

enum DesktopEnvironment {
  DESKTOP_ENVIRONMENT_OTHER,
  DESKTOP_ENVIRONMENT_GNOME,
  // KDE3 and KDE4 are sufficiently different that we count
  // them as two different desktop environments here.
  DESKTOP_ENVIRONMENT_KDE3,
  DESKTOP_ENVIRONMENT_KDE4,
  DESKTOP_ENVIRONMENT_XFCE,
  DESKTOP_ENVIRONMENT_MEEGO,
};

// Return an entry from the DesktopEnvironment enum with a best guess
// of which desktop environment we're using.  We use this to know when
// to attempt to use preferences from the desktop environment --
// proxy settings, password manager, etc.
DesktopEnvironment GetDesktopEnvironment(Environment* env);

// Return a string representation of the given desktop environment.
// May return NULL in the case of DESKTOP_ENVIRONMENT_OTHER.
const char* GetDesktopEnvironmentName(DesktopEnvironment env);
// Convenience wrapper that calls GetDesktopEnvironment() first.
const char* GetDesktopEnvironmentName(Environment* env);

}  // namespace nix
}  // namespace base

#endif  // BASE_NIX_XDG_UTIL_H_
