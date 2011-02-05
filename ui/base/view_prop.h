// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_VIEW_PROP_H_
#define UI_BASE_VIEW_PROP_H_
#pragma once

#include "base/basictypes.h"
#include "base/ref_counted.h"
#include "ui/gfx/native_widget_types.h"

namespace ui {

// ViewProp maintains a key/value pair for a particular view. ViewProp is
// designed as a replacement for the Win32's SetProp, but does not make use of
// window manager memory. ViewProp shares similar semantics as SetProp, the
// value for a particular view/key pair comes from the last ViewProp created.
class ViewProp {
 public:
  // Associates data with a view/key pair. If a ViewProp has already been
  // created for the specified pair |data| replaces the current value.
  //
  // ViewProp does *not* make a copy of the char*, the pointer is used for
  // sorting.
  ViewProp(gfx::NativeView view, const char* key, void* data);
  ~ViewProp();

  // Returns the value associated with the view/key pair, or NULL if there is
  // none.
  static void* GetValue(gfx::NativeView view, const char* key);

  // Returns the key.
  const char* Key() const;

 private:
  class Data;

  // Stores the actual data.
  scoped_refptr<Data> data_;

  DISALLOW_COPY_AND_ASSIGN(ViewProp);
};

}  // namespace ui

#endif  // UI_BASE_VIEW_PROP_H_
