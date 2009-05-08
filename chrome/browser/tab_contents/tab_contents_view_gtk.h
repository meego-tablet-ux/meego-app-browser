// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TAB_CONTENTS_TAB_CONTENTS_VIEW_GTK_H_
#define CHROME_BROWSER_TAB_CONTENTS_TAB_CONTENTS_VIEW_GTK_H_

#include "base/scoped_ptr.h"
#include "chrome/browser/tab_contents/tab_contents_view.h"
#include "chrome/common/notification_observer.h"
#include "chrome/common/notification_registrar.h"
#include "chrome/common/owned_widget_gtk.h"

class RenderViewContextMenuGtk;
class SadTabGtk;

class TabContentsViewGtk : public TabContentsView,
                           public NotificationObserver {
 public:
  // The corresponding TabContents is passed in the constructor, and manages our
  // lifetime. This doesn't need to be the case, but is this way currently
  // because that's what was easiest when they were split.
  explicit TabContentsViewGtk(TabContents* tab_contents);
  virtual ~TabContentsViewGtk();

  // TabContentsView implementation --------------------------------------------

  virtual void CreateView();
  virtual RenderWidgetHostView* CreateViewForWidget(
      RenderWidgetHost* render_widget_host);

  virtual gfx::NativeView GetNativeView() const;
  virtual gfx::NativeView GetContentNativeView() const;
  virtual gfx::NativeWindow GetTopLevelNativeWindow() const;
  virtual void GetContainerBounds(gfx::Rect* out) const;
  virtual void OnContentsDestroy();
  virtual void SetPageTitle(const std::wstring& title);
  virtual void Invalidate();
  virtual void SizeContents(const gfx::Size& size);
  virtual void FindInPage(const Browser& browser,
                          bool find_next, bool forward_direction);
  virtual void HideFindBar(bool end_session);
  virtual void ReparentFindWindow(Browser* new_browser) const;
  virtual bool GetFindBarWindowInfo(gfx::Point* position,
                                    bool* fully_visible) const;
  virtual void Focus();
  virtual void SetInitialFocus();
  virtual void StoreFocus();
  virtual void RestoreFocus();

  // Backend implementation of RenderViewHostDelegate::View.
  virtual void ShowContextMenu(const ContextMenuParams& params);
  virtual void StartDragging(const WebDropData& drop_data);
  virtual void UpdateDragCursor(bool is_drop_target);
  virtual void TakeFocus(bool reverse);
  virtual void HandleKeyboardEvent(const NativeWebKeyboardEvent& event);
  virtual void OnFindReply(int request_id,
                           int number_of_matches,
                           const gfx::Rect& selection_rect,
                           int active_match_ordinal,
                           bool final_update);

  // NotificationObserver implementation ---------------------------------------

  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details);

 private:
  // We keep track of the timestamp of the latest mousedown event.
  static gboolean OnMouseDown(GtkWidget* widget,
                              GdkEventButton* event, TabContentsViewGtk* view);

  // The native widget for the tab.
  OwnedWidgetGtk vbox_;

  // The context menu is reset every time we show it, but we keep a pointer to
  // between uses so that it won't go out of scope before we're done with it.
  scoped_ptr<RenderViewContextMenuGtk> context_menu_;

  // The event time for the last mouse down we handled. We need this to properly
  // show context menus.
  guint32 last_mouse_down_time_;

  // Used to get notifications about renderers coming and going.
  NotificationRegistrar registrar_;

  scoped_ptr<SadTabGtk> sad_tab_;

  // The widget that was focused the last time we were asked to store focus.
  GtkWidget* stored_focus_widget_;

  // The widget for which we've stored focus might be destroyed by the time we
  // want to restore focus. Thus we connect to the "destroy" signal on that
  // widget. This is the handler ID for the destroy handler.
  guint destroy_handler_id_;

  DISALLOW_COPY_AND_ASSIGN(TabContentsViewGtk);
};

#endif  // CHROME_BROWSER_TAB_CONTENTS_TAB_CONTENTS_VIEW_GTK_H_
