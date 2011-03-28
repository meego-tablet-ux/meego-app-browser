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

#ifndef CHROME_BROWSER_MENU_QT_H_
#define CHROME_BROWSER_MENU_QT_H_

#include "ui/gfx/point.h"
#include "ui/base/models/menu_model.h"

class BrowserWindowQt;

class MenuQtImpl;

class MenuQt
{
  friend class MenuQtImpl;
public:
  MenuQt(BrowserWindowQt* window);
  ~MenuQt();

  void SetModel(ui::MenuModel* model);
  
  void Popup();
  // Displays the menu at the given coords. |point| is intentionally not const.
  void PopupAt(gfx::Point point);
  void PopupAsContextAt(unsigned int event_time, gfx::Point point);

  // Closes the menu.
  void CloseMenu();

private:
  // If non-NULL, the MenuModel that we use to populate and control the GTK
  // menu (overriding the delegate as a controller).
  ui::MenuModel* model_;

  BrowserWindowQt* window_;

  MenuQtImpl* impl_;
};
#endif
