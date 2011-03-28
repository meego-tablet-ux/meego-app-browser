/*
 * Copyright (c) 2010, Intel Corporation. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are 
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright 
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above 
 * copyright notice, this list of conditions and the following disclaimer 
 * in the documentation and/or other materials provided with the 
 * distribution.
 *     * Neither the name of Intel Corporation nor the names of its 
 * contributors may be used to endorse or promote products derived from 
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef CHROME_BROWSER_QT_POPUP_LIST_QT_H_
#define CHROME_BROWSER_QT_POPUP_LIST_QT_H_
#pragma once

#include "base/basictypes.h"
#include "chrome/browser/renderer_host/render_widget_host_view_qt.h"

class BrowserWindowQt;
class WebMenuItem;
class PopupListQtImpl;
class RenderWidgerHostViewQt;

class PopupListQt {
 friend class PopupListQtImpl;

 public:
  explicit PopupListQt(BrowserWindowQt* window);
  virtual ~PopupListQt();
  void PopulateMenuItemData(int selected_item, const std::vector<WebMenuItem>& items);
  void SetHeaderBounds(gfx::Rect bounds);
  void setCurrentView(RenderWidgetHostViewQt* view) { view_ = view; }
  RenderWidgetHostViewQt* currentView() { return view_; }
  void show();

 private:
  RenderWidgetHostViewQt* view_;
  BrowserWindowQt* window_;
  PopupListQtImpl* impl_;
  QRect header_bounds_;

  DISALLOW_COPY_AND_ASSIGN(PopupListQt);
};

#endif  // CHROME_BROWSER_QT_POPUP_LIST_QT_H_
