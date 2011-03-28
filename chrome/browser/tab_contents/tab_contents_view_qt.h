// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TAB_CONTENTS_TAB_CONTENTS_VIEW_QT_H_
#define CHROME_BROWSER_TAB_CONTENTS_TAB_CONTENTS_VIEW_QT_H_

//#include <gtk/gtk.h>
//#include "ui/base/gtk/gtk_signal.h"
#include "base/scoped_ptr.h"
//#include "chrome/browser/ui/gtk/focus_store_gtk.h"
#include "content/browser/tab_contents/tab_contents_view.h"
#include "content/common/notification_observer.h"
#include "content/common/notification_registrar.h"
//#include "chrome/common/owned_widget_gtk.h"

class ConstrainedWindowQt;
class QtThemeProperties;
class RenderViewContextMenuQt;
class SadTabQt;
class TabContentsDragSource;
class WebDragDestQt;
class RenderWidgetHostViewQt;

class TabContentsViewQt : public TabContentsView,
                           public NotificationObserver {
 public:
  // The corresponding TabContents is passed in the constructor, and manages our
  // lifetime. This doesn't need to be the case, but is this way currently
  // because that's what was easiest when they were split.
  explicit TabContentsViewQt(TabContents* tab_contents);
  virtual ~TabContentsViewQt();

  // Unlike Windows, ConstrainedWindows need to collaborate with the
  // TabContentsViewQt to position the dialogs.
  void AttachConstrainedWindow(ConstrainedWindowQt* constrained_window);
  void RemoveConstrainedWindow(ConstrainedWindowQt* constrained_window);

  // TabContentsView implementation --------------------------------------------

  virtual void CreateView(const gfx::Size& initial_size);
  virtual RenderWidgetHostView* CreateViewForWidget(
      RenderWidgetHost* render_widget_host);

  virtual gfx::NativeView GetNativeView() const;
  virtual gfx::NativeView GetContentNativeView() const;
  virtual gfx::NativeWindow GetTopLevelNativeWindow() const;
  virtual void GetContainerBounds(gfx::Rect* out) const;
  virtual void SetPageTitle(const std::wstring& title);
  virtual void OnTabCrashed(base::TerminationStatus status,
                            int error_code);
  virtual void SizeContents(const gfx::Size& size);
  virtual void Focus();
  virtual void SetInitialFocus();
  virtual void StoreFocus();
  virtual void RestoreFocus();
  virtual void GetViewBounds(gfx::Rect* out) const;

  // Backend implementation of RenderViewHostDelegate::View.
  virtual void ShowContextMenu(const ContextMenuParams& params);
  virtual void ShowPopupMenu(const gfx::Rect& bounds,
                             int item_height,
                             double item_font_size,
                             int selected_item,
                             const std::vector<WebMenuItem>& items,
                             bool right_aligned);
  virtual void StartDragging(const WebDropData& drop_data,
                             WebKit::WebDragOperationsMask allowed_ops,
                             const SkBitmap& image,
                             const gfx::Point& image_offset);
  virtual void UpdateDragCursor(WebKit::WebDragOperation operation);
  virtual void GotFocus();
  virtual void TakeFocus(bool reverse);

  // NotificationObserver implementation ---------------------------------------

  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details);

  // MExpandedContainerDelegate implementation
  virtual void OnResize();


 void InsertIntoContentArea(QGraphicsWidget* widget);

 private:
  QGraphicsWidget* container_;

  RenderWidgetHostViewQt* rwhv_;

  QGraphicsWidget* rwhv_view_;
  gfx::Size requested_size_;

  scoped_ptr<RenderViewContextMenuQt> context_menu_;

  DISALLOW_COPY_AND_ASSIGN(TabContentsViewQt);
};

#endif  // CHROME_BROWSER_TAB_CONTENTS_TAB_CONTENTS_VIEW_QT_H_
