// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TAB_CONTENTS_WEB_CONTENTS_VIEW_MAC_H_
#define CHROME_BROWSER_TAB_CONTENTS_WEB_CONTENTS_VIEW_MAC_H_

#import <Cocoa/Cocoa.h>

#include "base/gfx/size.h"
#include "base/scoped_ptr.h"
#include "base/scoped_nsobject.h"
#include "chrome/browser/cocoa/base_view.h"
#include "chrome/browser/tab_contents/web_contents_view.h"
#include "chrome/common/notification_registrar.h"

class FindBarMac;
@class SadTabView;
class WebContentsViewMac;

@interface WebContentsViewCocoa : BaseView {
 @private
  WebContentsViewMac* webContentsView_;  // WEAK; owns us
}

@end

// Mac-specific implementation of the WebContentsView. It owns an NSView that
// contains all of the contents of the tab and associated child views.
class WebContentsViewMac : public WebContentsView,
                           public NotificationObserver {
 public:
  // The corresponding WebContents is passed in the constructor, and manages our
  // lifetime. This doesn't need to be the case, but is this way currently
  // because that's what was easiest when they were split.
  explicit WebContentsViewMac(WebContents* web_contents);
  virtual ~WebContentsViewMac();

  // WebContentsView implementation --------------------------------------------

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
  virtual bool GetFindBarWindowInfo(gfx::Point* position,
                                    bool* fully_visible) const;
  virtual void SetInitialFocus();
  virtual void StoreFocus();
  virtual void RestoreFocus();

  // Backend implementation of RenderViewHostDelegate::View.
  virtual WebContents* CreateNewWindowInternal(
      int route_id, base::WaitableEvent* modal_dialog_event);
  virtual void ShowCreatedWindowInternal(WebContents* new_web_contents,
                                         WindowOpenDisposition disposition,
                                         const gfx::Rect& initial_pos,
                                         bool user_gesture);
  virtual RenderWidgetHostView* CreateNewWidgetInternal(int route_id,
                                                        bool activatable);
  virtual void ShowCreatedWidgetInternal(RenderWidgetHostView* widget_host_view,
                                         const gfx::Rect& initial_pos);
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
  // ---------------------------------------------------------------------------

  // The Cocoa NSView that lives in the view hierarchy.
  scoped_nsobject<WebContentsViewCocoa> cocoa_view_;

  // For find in page. This may be NULL if there is no find bar, and if it is
  // non-NULL, it may or may not be visible.
  scoped_ptr<FindBarMac> find_bar_;

  // Used to get notifications about renderers coming and going.
  NotificationRegistrar registrar_;

  // Used to render the sad tab. This will be non-NULL only when the sad tab is
  // visible.
  scoped_nsobject<SadTabView> sad_tab_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsViewMac);
};

#endif  // CHROME_BROWSER_TAB_CONTENTS_WEB_CONTENTS_VIEW_MAC_H_
