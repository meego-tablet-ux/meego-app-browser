// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VIEWS_WIDGET_NATIVE_WINDOW_DELEGATE_H_
#define VIEWS_WIDGET_NATIVE_WINDOW_DELEGATE_H_
#pragma once

namespace views {
namespace internal {

////////////////////////////////////////////////////////////////////////////////
// NativeWindowDelegate interface
//
//  An interface implemented by an object that receives notifications from a
//  NativeWindow implementation.
//
class NativeWindowDelegate {
 public:
  virtual ~NativeWindowDelegate() {}
};

}  // namespace internal
}  // namespace views

#endif  // VIEWS_WIDGET_NATIVE_WINDOW_DELEGATE_H_
