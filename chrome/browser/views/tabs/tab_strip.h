// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VIEWS_TABS_TAB_STRIP_H_
#define CHROME_BROWSER_VIEWS_TABS_TAB_STRIP_H_

#include "app/animation_container.h"
#include "base/message_loop.h"
#include "base/ref_counted.h"
#include "base/timer.h"
#include "chrome/browser/tabs/tab_strip_model.h"
#include "chrome/browser/views/tabs/base_tab_strip.h"
#include "chrome/browser/views/tabs/tab.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "views/animation/bounds_animator.h"
#include "views/controls/button/image_button.h"

class DraggedTabController;
class ScopedMouseCloseWidthCalculator;
class TabStripModel;

namespace views {
class ImageView;
#if defined(OS_LINUX)
class WidgetGtk;
#elif defined(OS_WIN)
class WidgetWin;
#endif
}

///////////////////////////////////////////////////////////////////////////////
//
// TabStrip
//
//  A View that represents the TabStripModel. The TabStrip has the
//  following responsibilities:
//    - It implements the TabStripModelObserver interface, and acts as a
//      container for Tabs, and is also responsible for creating them.
//    - It takes part in Tab Drag & Drop with Tab, TabDragHelper and
//      DraggedTab, focusing on tasks that require reshuffling other tabs
//      in response to dragged tabs.
//
///////////////////////////////////////////////////////////////////////////////
class TabStrip : public BaseTabStrip,
                 public TabStripModelObserver,
                 public Tab::TabDelegate,
                 public views::ButtonListener,
                 public MessageLoopForUI::Observer,
                 public views::BoundsAnimatorObserver {
 public:
  explicit TabStrip(TabStripModel* model);
  virtual ~TabStrip();

  // Returns true if the TabStrip can accept input events. This returns false
  // when the TabStrip is animating to a new state and as such the user should
  // not be allowed to interact with the TabStrip.
  bool CanProcessInputEvents() const;

  // Accessors for the model and individual Tabs.
  TabStripModel* model() const { return model_; }

  // Destroys the active drag controller.
  void DestroyDragController();

  // Removes the drag source Tab from this TabStrip, and deletes it.
  void DestroyDraggedSourceTab(Tab* tab);

  // Retrieves the ideal bounds for the Tab at the specified index.
  gfx::Rect GetIdealBounds(int tab_data_index);

  // Returns the currently selected tab.
  Tab* GetSelectedTab() const;

  // Creates the new tab button.
  void InitTabStripButtons();

  // Return true if this tab strip is compatible with the provided tab strip.
  // Compatible tab strips can transfer tabs during drag and drop.
  bool IsCompatibleWith(TabStrip* other) const;

  // Returns the bounds of the new tab button.
  gfx::Rect GetNewTabButtonBounds();

  // Populates the BaseTabStrip implementation from its model. This is primarily
  // useful when switching between display types and there are existing tabs.
  // Upon initial creation the TabStrip is empty.
  void InitFromModel();

  // BaseTabStrip implementation:
  virtual int GetPreferredHeight();
  virtual void SetBackgroundOffset(const gfx::Point& offset);
  virtual bool IsPositionInWindowCaption(const gfx::Point& point);
  virtual void SetDraggedTabBounds(int tab_index,
                                   const gfx::Rect& tab_bounds);
  virtual bool IsDragSessionActive() const;
  virtual void UpdateLoadingAnimations();
  virtual bool IsAnimating() const;
  virtual TabStrip* AsTabStrip();

  // views::View overrides:
  virtual void PaintChildren(gfx::Canvas* canvas);
  virtual views::View* GetViewByID(int id) const;
  virtual void Layout();
  virtual gfx::Size GetPreferredSize();
  // NOTE: the drag and drop methods are invoked from FrameView. This is done to
  // allow for a drop region that extends outside the bounds of the TabStrip.
  virtual void OnDragEntered(const views::DropTargetEvent& event);
  virtual int OnDragUpdated(const views::DropTargetEvent& event);
  virtual void OnDragExited();
  virtual int OnPerformDrop(const views::DropTargetEvent& event);
  virtual bool GetAccessibleRole(AccessibilityTypes::Role* role);
  virtual views::View* GetViewForPoint(const gfx::Point& point);
  virtual void ThemeChanged();

  // BoundsAnimator::Observer overrides:
  virtual void OnBoundsAnimatorDone(views::BoundsAnimator* animator);

 protected:
  // Creates a new tab.
  virtual Tab* CreateTab();

  // views::View implementation:
  virtual void ViewHierarchyChanged(bool is_add,
                                    views::View* parent,
                                    views::View* child);

  // TabStripModelObserver implementation:
  virtual void TabInsertedAt(TabContents* contents,
                             int model_index,
                             bool foreground);
  virtual void TabDetachedAt(TabContents* contents, int model_index);
  virtual void TabSelectedAt(TabContents* old_contents,
                             TabContents* contents,
                             int model_index,
                             bool user_gesture);
  virtual void TabMoved(TabContents* contents,
                        int from_model_index,
                        int to_model_index);
  virtual void TabChangedAt(TabContents* contents,
                            int model_index,
                            TabChangeType change_type);
  virtual void TabReplacedAt(TabContents* old_contents,
                             TabContents* new_contents,
                             int model_index);
  virtual void TabMiniStateChanged(TabContents* contents, int model_index);
  virtual void TabBlockedStateChanged(TabContents* contents, int model_index);

  // Tab::Delegate implementation:
  virtual bool IsTabSelected(const Tab* tab) const;
  virtual bool IsTabPinned(const Tab* tab) const;
  virtual void SelectTab(Tab* tab);
  virtual void CloseTab(Tab* tab);
  virtual bool IsCommandEnabledForTab(
      TabStripModel::ContextMenuCommand command_id, const Tab* tab) const;
  virtual bool IsCommandCheckedForTab(
      TabStripModel::ContextMenuCommand command_id, const Tab* tab) const;
  virtual void ExecuteCommandForTab(
      TabStripModel::ContextMenuCommand command_id, Tab* tab);
  virtual void StartHighlightTabsForCommand(
      TabStripModel::ContextMenuCommand command_id, Tab* tab);
  virtual void StopHighlightTabsForCommand(
      TabStripModel::ContextMenuCommand command_id, Tab* tab);
  virtual void StopAllHighlighting();
  virtual void MaybeStartDrag(Tab* tab, const views::MouseEvent& event);
  virtual void ContinueDrag(const views::MouseEvent& event);
  virtual bool EndDrag(bool canceled);
  virtual bool HasAvailableDragActions() const;

  // views::ButtonListener implementation:
  virtual void ButtonPressed(views::Button* sender, const views::Event& event);

  // MessageLoop::Observer implementation:
#if defined(OS_WIN)
  virtual void WillProcessMessage(const MSG& msg);
  virtual void DidProcessMessage(const MSG& msg);
#else
  virtual void WillProcessEvent(GdkEvent* event);
  virtual void DidProcessEvent(GdkEvent* event);
#endif

  // Horizontal gap between mini and non-mini-tabs.
  static const int mini_to_non_mini_gap_;

 private:
  class RemoveTabDelegate;

  friend class DraggedTabController;

  // AnimationType used for tracking animations that require additional
  // state beyond just animating the bounds of a view.
  //
  // Currently the only animation special cased is that of inserting the new tab
  // page at the end of the tab strip. Here's the steps that take place when
  // this happens.
  // . The newly inserted tab is set to render for the new tab animation
  //   |set_render_as_new_tab|. The timer new_tab_timer_ is used to determine
  //   when to turn this off. This is represented by state ANIMATION_NEW_TAB_1.
  // . The new tab is rendered in the background with an ever increasing alpha
  //   value and the tab goes slightly past the new tab button. The new tab
  //   button is not visible during this animation. This is represented by the
  //   state ANIMATION_NEW_TAB_2.
  // . The new tab is animated to its final position and the new tab button is
  //   rendered beneath the selected tab. This is represented by the state
  //   ANIMATION_NEW_TAB_3.
  enum AnimationType {
    ANIMATION_DEFAULT,

    ANIMATION_NEW_TAB_1,
    ANIMATION_NEW_TAB_2,
    ANIMATION_NEW_TAB_3
  };

  // Used during a drop session of a url. Tracks the position of the drop as
  // well as a window used to highlight where the drop occurs.
  struct DropInfo {
    DropInfo(int index, bool drop_before, bool paint_down);
    ~DropInfo();

    // Index of the tab to drop on. If drop_before is true, the drop should
    // occur between the tab at drop_index - 1 and drop_index.
    // WARNING: if drop_before is true it is possible this will == tab_count,
    // which indicates the drop should create a new tab at the end of the tabs.
    int drop_index;
    bool drop_before;

    // Direction the arrow should point in. If true, the arrow is displayed
    // above the tab and points down. If false, the arrow is displayed beneath
    // the tab and points up.
    bool point_down;

    // Renders the drop indicator.
    // TODO(beng): should be views::Widget.
#if defined(OS_WIN)
    views::WidgetWin* arrow_window;
#else
    views::WidgetGtk* arrow_window;
#endif
    views::ImageView* arrow_view;

   private:
    DISALLOW_COPY_AND_ASSIGN(DropInfo);
  };

  // The Tabs we contain, and their last generated "good" bounds.
  struct TabData {
    Tab* tab;
    gfx::Rect ideal_bounds;
  };

  TabStrip();
  void Init();

  // Set the images for the new tab button.
  void LoadNewTabButtonImage();

  // Retrieves the Tab at the specified index. Remember, the specified index
  // is in terms of tab_data, *not* the model.
  Tab* GetTabAtTabDataIndex(int tab_data_index) const;

  // Returns the tab at the specified index. If a remove animation is on going
  // and the index is >= the index of the tab being removed, the index is
  // incremented. While a remove operation is on going the indices of the model
  // do not line up with the indices of the view. This method adjusts the index
  // accordingly.
  //
  // Use this instead of GetTabAtTabDataIndex if the index comes from the model.
  Tab* GetTabAtModelIndex(int model_index) const;

  // Gets the number of Tabs in the collection.
  // WARNING: this is the number of tabs displayed by the tabstrip, which if
  // an animation is ongoing is not necessarily the same as the number of tabs
  // in the model.
  int GetTabCount() const;

  // Returns the number of mini-tabs.
  int GetMiniTabCount() const;

  // -- Tab Resize Layout -----------------------------------------------------

  // Returns the exact (unrounded) current width of each tab.
  void GetCurrentTabWidths(double* unselected_width,
                           double* selected_width) const;

  // Returns the exact (unrounded) desired width of each tab, based on the
  // desired strip width and number of tabs.  If
  // |width_of_tabs_for_mouse_close_| is nonnegative we use that value in
  // calculating the desired strip width; otherwise we use the current width.
  // |mini_tab_count| gives the number of mini-tabs, and |tab_count| the
  // number of mini and non-mini-tabs.
  void GetDesiredTabWidths(int tab_count,
                           int mini_tab_count,
                           double* unselected_width,
                           double* selected_width) const;

  // Perform an animated resize-relayout of the TabStrip immediately.
  void ResizeLayoutTabs();

  // Returns whether or not the cursor is currently in the "tab strip zone"
  // which is defined as the region above the TabStrip and a bit below it.
  bool IsCursorInTabStripZone() const;

  // Ensure that the message loop observer used for event spying is added and
  // removed appropriately so we can tell when to resize layout the tab strip.
  void AddMessageLoopObserver();
  void RemoveMessageLoopObserver();

  // -- Link Drag & Drop ------------------------------------------------------

  // Returns the bounds to render the drop at, in screen coordinates. Sets
  // |is_beneath| to indicate whether the arrow is beneath the tab, or above
  // it.
  gfx::Rect GetDropBounds(int drop_index, bool drop_before, bool* is_beneath);

  // Updates the location of the drop based on the event.
  void UpdateDropIndex(const views::DropTargetEvent& event);

  // Sets the location of the drop, repainting as necessary.
  void SetDropIndex(int tab_data_index, bool drop_before);

  // Returns the drop effect for dropping a URL on the tab strip. This does
  // not query the data in anyway, it only looks at the source operations.
  int GetDropEffect(const views::DropTargetEvent& event);

  // Returns the image to use for indicating a drop on a tab. If is_down is
  // true, this returns an arrow pointing down.
  static SkBitmap* GetDropArrowImage(bool is_down);

  // -- Animations ------------------------------------------------------------

  // Generates the ideal bounds of the TabStrip when all Tabs have finished
  // animating to their desired position/bounds. This is used by the standard
  // Layout method and other callers like the DraggedTabController that need
  // stable representations of Tab positions.
  void GenerateIdealBounds();

  // Both of these are invoked when a part of the new tab animation completes.
  // They configure state for the next step in the animation and start it.
  void NewTabAnimation1Done();
  void NewTabAnimation2Done();

  // Animates all the views to their ideal bounds.
  // NOTE: this does *not* invoke GenerateIdealBounds, it uses the bounds
  // currently set in ideal_bounds.
  void AnimateToIdealBounds();

  // Returns true if a new tab inserted at specified index should start the
  // new tab animation. See description above AnimationType for details on
  // this animation.
  bool ShouldStartIntertTabAnimationAtEnd(int model_index, bool foreground);

  // Starts various types of TabStrip animations.
  void StartResizeLayoutAnimation();
  void StartInsertTabAnimationAtEnd();
  void StartInsertTabAnimation(int model_index);
  void StartRemoveTabAnimation(int model_index);
  void StartMoveTabAnimation(int from_model_index,
                             int to_model_index);
  void StartMiniTabAnimation();

  // Stops any ongoing animations. If |layout| is true and an animation is
  // ongoing this does a layout.
  void StopAnimating(bool layout);

  // Resets all state related to animations. This is invoked when an animation
  // completes, prior to starting an animation or when we cancel an animation.
  // If |stop_new_tab_timer| is true, |new_tab_timer_| is stopped.
  void ResetAnimationState(bool stop_new_tab_timer);

  // Calculates the available width for tabs, assuming a Tab is to be closed.
  int GetAvailableWidthForTabs(Tab* last_tab) const;

  // Returns true if the specified point in TabStrip coords is within the
  // hit-test region of the specified Tab.
  bool IsPointInTab(Tab* tab, const gfx::Point& point_in_tabstrip_coords);

  // Cleans up the Tab from the TabStrip. This is called from the tab animation
  // code and is not a general-purpose method.
  void RemoveTab(Tab* tab);

  // Called from the message loop observer when a mouse movement has occurred
  // anywhere over our containing window.
  void HandleGlobalMouseMoveEvent();

  // Returns true if any of the tabs are phantom.
  bool HasPhantomTabs() const;

  // Returns the index of the specified tab in the model coordiate system, or
  // -1 if tab is closing or not in |tab_data_|.
  int GetModelIndexOfTab(const Tab* tab) const;

  // Returns the index into |tab_data_| corresponding to the index from the
  // TabStripModel, or |tab_data_.size()| if there is no tab representing
  // |model_index|.
  int ModelIndexToTabDataIndex(int model_index) const;

  // Returns the index into |tab_data_| corresponding to the specified tab, or
  // -1 if the tab isn't in |tab_data_|.
  int TabDataIndexOfTab(Tab* tab) const;

  // -- Member Variables ------------------------------------------------------

  // Our model.
  TabStripModel* model_;

  // A factory that is used to construct a delayed callback to the
  // ResizeLayoutTabsNow method.
  ScopedRunnableMethodFactory<TabStrip> resize_layout_factory_;

  // True if the TabStrip has already been added as a MessageLoop observer.
  bool added_as_message_loop_observer_;

  // True if a resize layout animation should be run a short delay after the
  // mouse exits the TabStrip.
  bool needs_resize_layout_;

  // The "New Tab" button.
  views::ImageButton* newtab_button_;

  // Ideal bounds of the new tab button.
  gfx::Rect newtab_button_bounds_;

  // The current widths of various types of tabs.  We save these so that, as
  // users close tabs while we're holding them at the same size, we can lay out
  // tabs exactly and eliminate the "pixel jitter" we'd get from just leaving
  // them all at their existing, rounded widths.
  double current_unselected_width_;
  double current_selected_width_;

  // If this value is nonnegative, it is used in GetDesiredTabWidths() to
  // calculate how much space in the tab strip to use for tabs.  Most of the
  // time this will be -1, but while we're handling closing a tab via the mouse,
  // we'll set this to the edge of the last tab before closing, so that if we
  // are closing the last tab and need to resize immediately, we'll resize only
  // back to this width, thus once again placing the last tab under the mouse
  // cursor.
  int available_width_for_tabs_;

  // Storage of strings needed for accessibility.
  std::wstring accessible_name_;

  // The size of the new tab button must be hardcoded because we need to be
  // able to lay it out before we are able to get its image from the
  // ThemeProvider.  It also makes sense to do this, because the size of the
  // new tab button should not need to be calculated dynamically.
  static const int kNewTabButtonWidth = 28;
  static const int kNewTabButtonHeight = 18;

  // Valid for the lifetime of a drag over us.
  scoped_ptr<DropInfo> drop_info_;

  // The controller for a drag initiated from a Tab. Valid for the lifetime of
  // the drag session.
  scoped_ptr<DraggedTabController> drag_controller_;

  std::vector<TabData> tab_data_;

  // To ensure all tabs pulse at the same time they share the same animation
  // container. This is that animation container.
  scoped_refptr<AnimationContainer> animation_container_;

  views::BoundsAnimator bounds_animator_;

  // Used for stage 1 of new tab animation.
  base::OneShotTimer<TabStrip> new_tab_timer_;

  // Set for special animations.
  AnimationType animation_type_;

  DISALLOW_COPY_AND_ASSIGN(TabStrip);
};

#endif  // CHROME_BROWSER_VIEWS_TABS_TAB_STRIP_H_
