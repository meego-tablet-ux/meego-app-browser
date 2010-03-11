// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/renderer_host/render_view_host_delegate.h"

#include "base/gfx/rect.h"
#include "base/singleton.h"
#include "chrome/common/renderer_preferences.h"
#include "googleurl/src/gurl.h"
#include "webkit/glue/webpreferences.h"

#if defined(OS_LINUX)
#include "chrome/browser/gtk/gtk_util.h"
#endif

RenderViewHostDelegate::View* RenderViewHostDelegate::GetViewDelegate() {
  return NULL;
}

RenderViewHostDelegate::RendererManagement*
RenderViewHostDelegate::GetRendererManagementDelegate() {
  return NULL;
}

RenderViewHostDelegate::BrowserIntegration*
RenderViewHostDelegate::GetBrowserIntegrationDelegate() {
  return NULL;
}

RenderViewHostDelegate::Resource*
RenderViewHostDelegate::GetResourceDelegate() {
  return NULL;
}

RenderViewHostDelegate::Save* RenderViewHostDelegate::GetSaveDelegate() {
  return NULL;
}

RenderViewHostDelegate::Printing*
RenderViewHostDelegate::GetPrintingDelegate() {
  return NULL;
}

RenderViewHostDelegate::FavIcon*
RenderViewHostDelegate::GetFavIconDelegate() {
  return NULL;
}

RenderViewHostDelegate::Autocomplete*
RenderViewHostDelegate::GetAutocompleteDelegate() {
  return NULL;
}

RenderViewHostDelegate::AutoFill*
RenderViewHostDelegate::GetAutoFillDelegate() {
  return NULL;
}

RenderViewHostDelegate::BookmarkDrag*
RenderViewHostDelegate::GetBookmarkDragDelegate() {
  return NULL;
}

const GURL& RenderViewHostDelegate::GetURL() const {
  return GURL::EmptyGURL();
}

TabContents* RenderViewHostDelegate::GetAsTabContents() {
  return NULL;
}

GURL RenderViewHostDelegate::GetAlternateErrorPageURL() const {
  return GURL();
}

WebPreferences RenderViewHostDelegate::GetWebkitPrefs() {
  return WebPreferences();
}

bool RenderViewHostDelegate::CanBlur() const {
  return true;
}

gfx::Rect RenderViewHostDelegate::GetRootWindowResizerRect() const {
  return gfx::Rect();
}

bool RenderViewHostDelegate::IsExternalTabContainer() const {
  return false;
}
