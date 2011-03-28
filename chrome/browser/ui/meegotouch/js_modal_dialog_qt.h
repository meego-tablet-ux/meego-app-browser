// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Copyright (c) 2010, Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_QT_JS_MODAL_DIALOG_QT_H_
#define CHROME_BROWSER_QT_JS_MODAL_DIALOG_QT_H_
#pragma once

#include <QObject>
#include <QString>
#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "chrome/browser/ui/app_modal_dialogs/native_app_modal_dialog.h"
#include "chrome/browser/ui/meegotouch/dialog_qt.h"
#include "ui/gfx/native_widget_types.h"

class JavaScriptAppModalDialog;

/* This is the implement class for JavaScript alert, confirm, prompt, and
 * onbeforeunload dialog boxes for Meego.
 */
class JSModalDialogQt : public NativeAppModalDialog, 
			 public DialogQtResultListener {
 public:
  JSModalDialogQt(JavaScriptAppModalDialog* dialog,
                   gfx::NativeWindow parent_window);
  virtual ~JSModalDialogQt();

  // Overridden from NativeAppModalDialog:
  virtual int GetAppModalDialogButtons() const;
  virtual void ShowAppModalDialog();
  virtual void ActivateAppModalDialog();
  virtual void CloseAppModalDialog();
  virtual void AcceptAppModalDialog();
  virtual void CancelAppModalDialog();

  virtual void OnDialogResponse(int result, QString input1, QString input2, bool isSuppress);
 private:
  void HandleDialogResponse(int response_id, QString input, bool isSuppress=false);

  scoped_ptr<JavaScriptAppModalDialog> jsDialog_;
  DialogQtModel *qDlgModel_;
};

#endif  // CHROME_BROWSER_QT_JS_MODAL_DIALOG_QT_H_

