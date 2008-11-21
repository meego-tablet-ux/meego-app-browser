# Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Tool module for adding, to a construction environment, Chromium-specific
wrappers around Hammer builders.  This gives us a central place for any
customization we need to make to the different things we build.
"""

def generate(env):
  def ChromeProgram(env, *args, **kw):
    return env.ComponentProgram(*args, **kw)
  env.AddMethod(ChromeProgram)

  def ChromeTestProgram(env, *args, **kw):
    return env.ComponentTestProgram(*args, **kw)
  env.AddMethod(ChromeTestProgram)

  def ChromeStaticLibrary(env, *args, **kw):
    kw['COMPONENT_STATIC'] = True
    return env.ComponentLibrary(*args, **kw)
  env.AddMethod(ChromeStaticLibrary)

  def ChromeSharedLibrary(env, *args, **kw):
    kw['COMPONENT_STATIC'] = False
    return [env.ComponentLibrary(*args, **kw)[0]]
  env.AddMethod(ChromeSharedLibrary)

  def ChromeObject(env, *args, **kw):
    return env.ComponentObject(*args, **kw)
  env.AddMethod(ChromeObject)

def exists(env):
  return True
