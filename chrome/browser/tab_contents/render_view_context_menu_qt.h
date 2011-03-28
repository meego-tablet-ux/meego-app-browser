// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TAB_CONTENTS_RENDER_VIEW_CONTEXT_MENU_QT_H_
#define CHROME_BROWSER_TAB_CONTENTS_RENDER_VIEW_CONTEXT_MENU_QT_H_

#include <map>
#include <string>
#include <vector>

#include "base/scoped_ptr.h"
#include "chrome/browser/tab_contents/render_view_context_menu_simple.h"
#include "ui/gfx/point.h"
#include "chrome/browser/ui/meegotouch/menu_qt.h"

class RenderWidgetHostView;
struct ContextMenuParams;

class RenderViewContextMenuQt : public RenderViewContextMenuSimple
{
 public:
  RenderViewContextMenuQt(TabContents* web_contents,
                           const ContextMenuParams& params,
                           unsigned int triggering_event_time);

  ~RenderViewContextMenuQt();

  void Popup();
  // Show the menu at the given location.
  void Popup(const gfx::Point& point);

  // Menu::Delegate implementation ---------------------------------------------
  virtual void StoppedShowing();

 protected:
  // RenderViewContextMenu implementation --------------------------------------
  virtual void PlatformInit();
  // TODO(port): implement.
  virtual bool GetAcceleratorForCommandId(
      int command_id,
      ui::Accelerator* accelerator) {
    return false;
  }

 private:
  scoped_ptr<MenuQt> menu_;
  uint32_t triggering_event_time_;
};

#endif  // CHROME_BROWSER_TAB_CONTENTS_RENDER_VIEW_CONTEXT_MENU_QT_H_
