// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/input_window_dialog.h"

#include "base/message_loop.h"
#include "base/scoped_ptr.h"
#include "base/string_piece.h"
#include "base/utf_string_conversions.h"

class QtInputWindowDialog : public InputWindowDialog {
 public:
  // Creates a dialog. Takes ownership of |delegate|.
  QtInputWindowDialog(gfx::NativeWindow parent,
                       const std::string& window_title,
                       const std::string& label,
                       const std::string& contents,
                       Delegate* delegate);
  virtual ~QtInputWindowDialog();

  virtual void Show();
  virtual void Close();
};


QtInputWindowDialog::QtInputWindowDialog(gfx::NativeWindow parent,
                                           const std::string& window_title,
                                           const std::string& label,
                                           const std::string& contents,
                                           Delegate* delegate) {
  DNOTIMPLEMENTED();
}

QtInputWindowDialog::~QtInputWindowDialog() {
  DNOTIMPLEMENTED();
}

void QtInputWindowDialog::Show() {
  DNOTIMPLEMENTED();
}

void QtInputWindowDialog::Close() {
  DNOTIMPLEMENTED();
}

// static
InputWindowDialog* InputWindowDialog::Create(gfx::NativeWindow parent,
                                             const std::wstring& window_title,
                                             const std::wstring& label,
                                             const std::wstring& contents,
                                             Delegate* delegate) {
  return new QtInputWindowDialog(parent,
                                  WideToUTF8(window_title),
                                  WideToUTF8(label),
                                  WideToUTF8(contents),
                                  delegate);
}
