// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VIEWS_WIDGET_DROP_TARGET_WIN_H_
#define VIEWS_WIDGET_DROP_TARGET_WIN_H_
#pragma once

#include "app/win/drop_target.h"
#include "views/widget/drop_helper.h"

namespace views {

class RootView;
class View;

// DropTargetWin takes care of managing drag and drop for WidgetWin. It
// converts Windows OLE drop messages into Views drop messages.
//
// DropTargetWin uses DropHelper to manage the appropriate view to target
// drop messages at.
class DropTargetWin : public app::win::DropTarget {
 public:
  explicit DropTargetWin(RootView* root_view);
  virtual ~DropTargetWin();

  // If a drag and drop is underway and view is the current drop target, the
  // drop target is set to null.
  // This is invoked when a View is removed from the RootView to make sure
  // we don't target a view that was removed during dnd.
  void ResetTargetViewIfEquals(View* view);

 protected:
  virtual DWORD OnDragOver(IDataObject* data_object,
                           DWORD key_state,
                           POINT cursor_position,
                           DWORD effect);

  virtual void OnDragLeave(IDataObject* data_object);

  virtual DWORD OnDrop(IDataObject* data_object,
                       DWORD key_state,
                       POINT cursor_position,
                       DWORD effect);

 private:
  views::DropHelper helper_;

  DISALLOW_COPY_AND_ASSIGN(DropTargetWin);
};

}  // namespace views

#endif  // VIEWS_WIDGET_DROP_TARGET_WIN_H_
