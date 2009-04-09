// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/gtk/tabs/tab_strip_gtk.h"

#include "base/gfx/gtk_util.h"
#include "base/gfx/point.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/tab_contents/tab_contents.h"
#include "chrome/common/gfx/chrome_canvas.h"
#include "chrome/common/l10n_util.h"
#include "chrome/common/resource_bundle.h"
#include "chrome/common/slide_animation.h"
#include "grit/generated_resources.h"
#include "grit/theme_resources.h"

namespace {

const int kDefaultAnimationDurationMs = 100;

const int kNewTabButtonHOffset = -5;
const int kNewTabButtonVOffset = 5;

// The horizontal offset from one tab to the next,
// which results in overlapping tabs.
const int kTabHOffset = -16;

SkBitmap* background = NULL;

inline int Round(double x) {
  return static_cast<int>(x + 0.5);
}

// widget->allocation is not guaranteed to be set.  After window creation,
// we pick up the normal bounds by connecting to the configure-event signal.
gfx::Rect GetInitialWidgetBounds(GtkWidget* widget) {
  GtkRequisition request;
  gtk_widget_size_request(widget, &request);
  return gfx::Rect(0, 0, request.width, request.height);
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
//
// TabAnimation
//
//  A base class for all TabStrip animations.
//
class TabStripGtk::TabAnimation : public AnimationDelegate {
 public:
  friend class TabStripGtk;

  // Possible types of animation.
  enum Type {
    INSERT,
    REMOVE,
    MOVE,
    RESIZE
  };

  TabAnimation(TabStripGtk* tabstrip, Type type)
      : tabstrip_(tabstrip),
        animation_(this),
        start_selected_width_(0),
        start_unselected_width_(0),
        end_selected_width_(0),
        end_unselected_width_(0),
        layout_on_completion_(false),
        type_(type) {
  }
  virtual ~TabAnimation() {}

  Type type() const { return type_; }

  void Start() {
    animation_.SetSlideDuration(GetDuration());
    animation_.SetTweenType(SlideAnimation::EASE_OUT);
    if (!animation_.IsShowing()) {
      animation_.Reset();
      animation_.Show();
    }
  }

  void Stop() {
    animation_.Stop();
  }

  void set_layout_on_completion(bool layout_on_completion) {
    layout_on_completion_ = layout_on_completion;
  }

  // Retrieves the width for the Tab at the specified index if an animation is
  // active.
  static double GetCurrentTabWidth(TabStripGtk* tabstrip,
                                   TabStripGtk::TabAnimation* animation,
                                   int index) {
    double unselected, selected;
    tabstrip->GetCurrentTabWidths(&unselected, &selected);
    TabGtk* tab = tabstrip->GetTabAt(index);
    double tab_width = tab->IsSelected() ? selected : unselected;

    if (animation) {
      double specified_tab_width = animation->GetWidthForTab(index);
      if (specified_tab_width != -1)
        tab_width = specified_tab_width;
    }

    return tab_width;
  }

  // Overridden from AnimationDelegate:
  virtual void AnimationProgressed(const Animation* animation) {
    tabstrip_->AnimationLayout(end_unselected_width_);
  }

  virtual void AnimationEnded(const Animation* animation) {
    tabstrip_->FinishAnimation(this, layout_on_completion_);
    // This object is destroyed now, so we can't do anything else after this.
  }

  virtual void AnimationCanceled(const Animation* animation) {
    AnimationEnded(animation);
  }

 protected:
  // Returns the duration of the animation.
  virtual int GetDuration() const {
    return kDefaultAnimationDurationMs;
  }

  // Subclasses override to return the width of the Tab at the specified index
  // at the current animation frame. -1 indicates the default width should be
  // used for the Tab.
  virtual double GetWidthForTab(int index) const {
    return -1;  // Use default.
  }

  // Figure out the desired start and end widths for the specified pre- and
  // post- animation tab counts.
  void GenerateStartAndEndWidths(int start_tab_count, int end_tab_count) {
    tabstrip_->GetDesiredTabWidths(start_tab_count, &start_unselected_width_,
                                   &start_selected_width_);
    double standard_tab_width =
        static_cast<double>(TabRendererGtk::GetStandardSize().width());

    if (start_tab_count < end_tab_count &&
        start_unselected_width_ < standard_tab_width) {
      double minimum_tab_width = static_cast<double>(
          TabRendererGtk::GetMinimumUnselectedSize().width());
      start_unselected_width_ -= minimum_tab_width / start_tab_count;
    }

    tabstrip_->GenerateIdealBounds();
    tabstrip_->GetDesiredTabWidths(end_tab_count,
                                   &end_unselected_width_,
                                   &end_selected_width_);
  }

  TabStripGtk* tabstrip_;
  SlideAnimation animation_;

  double start_selected_width_;
  double start_unselected_width_;
  double end_selected_width_;
  double end_unselected_width_;

 private:
  // True if a complete re-layout is required upon completion of the animation.
  // Subclasses set this if they don't perform a complete layout
  // themselves and canceling the animation may leave the strip in an
  // inconsistent state.
  bool layout_on_completion_;

  const Type type_;

  DISALLOW_EVIL_CONSTRUCTORS(TabAnimation);
};

///////////////////////////////////////////////////////////////////////////////

// Handles insertion of a Tab at |index|.
class InsertTabAnimation : public TabStripGtk::TabAnimation {
 public:
  explicit InsertTabAnimation(TabStripGtk* tabstrip, int index)
      : TabAnimation(tabstrip, INSERT),
        index_(index) {
    int tab_count = tabstrip->GetTabCount();
    GenerateStartAndEndWidths(tab_count - 1, tab_count);
  }
  virtual ~InsertTabAnimation() {}

 protected:
  // Overridden from TabStripGtk::TabAnimation:
  virtual double GetWidthForTab(int index) const {
    if (index == index_) {
      bool is_selected = tabstrip_->model()->selected_index() == index;
      double target_width =
          is_selected ? end_unselected_width_ : end_selected_width_;
      double start_width =
          is_selected ? TabGtk::GetMinimumSelectedSize().width() :
                        TabGtk::GetMinimumUnselectedSize().width();

      double delta = target_width - start_width;
      if (delta > 0)
        return start_width + (delta * animation_.GetCurrentValue());

      return start_width;
    }

    if (tabstrip_->GetTabAt(index)->IsSelected()) {
      double delta = end_selected_width_ - start_selected_width_;
      return start_selected_width_ + (delta * animation_.GetCurrentValue());
    }

    double delta = end_unselected_width_ - start_unselected_width_;
    return start_unselected_width_ + (delta * animation_.GetCurrentValue());
  }

 private:
  int index_;

  DISALLOW_EVIL_CONSTRUCTORS(InsertTabAnimation);
};

///////////////////////////////////////////////////////////////////////////////

// Handles removal of a Tab from |index|
class RemoveTabAnimation : public TabStripGtk::TabAnimation {
 public:
  RemoveTabAnimation(TabStripGtk* tabstrip, int index, TabContents* contents)
      : TabAnimation(tabstrip, REMOVE),
        index_(index) {
    int tab_count = tabstrip->GetTabCount();
    GenerateStartAndEndWidths(tab_count, tab_count - 1);
  }

  virtual ~RemoveTabAnimation() { }

  // Returns the index of the tab being removed.
  int index() const { return index_; }

 protected:
  // Overridden from TabStripGtk::TabAnimation:
  virtual double GetWidthForTab(int index) const {
    TabGtk* tab = tabstrip_->GetTabAt(index);

    if (index == index_) {
      // The tab(s) being removed are gradually shrunken depending on the state
      // of the animation.
      // Removed animated Tabs are never selected.
      double start_width = start_unselected_width_;
      // Make sure target_width is at least abs(kTabHOffset), otherwise if
      // less than kTabHOffset during layout tabs get negatively offset.
      double target_width =
          std::max(abs(kTabHOffset),
                   TabGtk::GetMinimumUnselectedSize().width() + kTabHOffset);
      double delta = start_width - target_width;
      return start_width - (delta * animation_.GetCurrentValue());
    }

    if (tabstrip_->available_width_for_tabs_ != -1 &&
        index_ != tabstrip_->GetTabCount() - 1) {
      return TabStripGtk::TabAnimation::GetWidthForTab(index);
    }

    // All other tabs are sized according to the start/end widths specified at
    // the start of the animation.
    if (tab->IsSelected()) {
      double delta = end_selected_width_ - start_selected_width_;
      return start_selected_width_ + (delta * animation_.GetCurrentValue());
    }

    double delta = end_unselected_width_ - start_unselected_width_;
    return start_unselected_width_ + (delta * animation_.GetCurrentValue());
  }

  virtual void AnimationEnded(const Animation* animation) {
    tabstrip_->RemoveTabAt(index_);
    HighlightCloseButton();
    TabStripGtk::TabAnimation::AnimationEnded(animation);
  }

 private:
  // When the animation completes, we send the Container a message to simulate
  // a mouse moved event at the current mouse position. This tickles the Tab
  // the mouse is currently over to show the "hot" state of the close button.
  void HighlightCloseButton() {
    if (tabstrip_->available_width_for_tabs_ == -1) {
      // This function is not required (and indeed may crash!) for removes
      // spawned by non-mouse closes and drag-detaches.
      return;
    }

    /* get default display and screen */
    GdkDisplay* display = gdk_display_get_default();
    GdkScreen* screen = gdk_display_get_default_screen(display);

    /* get cursor position */
    int x, y;
    gdk_display_get_pointer(display, NULL, &x, &y, NULL);

    /* reset cusor position */
    gdk_display_warp_pointer(display, screen, x, y);
  }

  int index_;

  DISALLOW_EVIL_CONSTRUCTORS(RemoveTabAnimation);
};

////////////////////////////////////////////////////////////////////////////////
// TabStripGtk, public:

TabStripGtk::TabStripGtk(TabStripModel* model)
    : current_unselected_width_(TabGtk::GetStandardSize().width()),
      current_selected_width_(TabGtk::GetStandardSize().width()),
      available_width_for_tabs_(-1),
      resize_layout_scheduled_(false),
      model_(model),
      hover_index_(-1) {
}

TabStripGtk::~TabStripGtk() {
  model_->RemoveObserver(this);
  tabstrip_.Destroy();
}

void TabStripGtk::Init() {
  model_->AddObserver(this);

  if (!background) {
    ResourceBundle &rb = ResourceBundle::GetSharedInstance();
    background = rb.GetBitmapNamed(IDR_WINDOW_TOP_CENTER);
  }

  tabstrip_.Own(gtk_drawing_area_new());
  gtk_widget_set_size_request(tabstrip_.get(), -1,
                              TabGtk::GetMinimumUnselectedSize().height());
  gtk_widget_set_app_paintable(tabstrip_.get(), TRUE);
  g_signal_connect(G_OBJECT(tabstrip_.get()), "expose-event",
                   G_CALLBACK(OnExpose), this);
  g_signal_connect(G_OBJECT(tabstrip_.get()), "configure-event",
                   G_CALLBACK(OnConfigure), this);
  g_signal_connect(G_OBJECT(tabstrip_.get()), "motion-notify-event",
                   G_CALLBACK(OnMotionNotify), this);
  g_signal_connect(G_OBJECT(tabstrip_.get()), "button-press-event",
                   G_CALLBACK(OnMousePress), this);
  g_signal_connect(G_OBJECT(tabstrip_.get()), "button-release-event",
                   G_CALLBACK(OnMouseRelease), this);
  g_signal_connect(G_OBJECT(tabstrip_.get()), "leave-notify-event",
                   G_CALLBACK(OnLeaveNotify), this);
  gtk_widget_add_events(tabstrip_.get(),
      GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK |
      GDK_BUTTON_RELEASE_MASK |GDK_LEAVE_NOTIFY_MASK);
  gtk_widget_show_all(tabstrip_.get());

  bounds_ = GetInitialWidgetBounds(tabstrip_.get());
}

void TabStripGtk::AddTabStripToBox(GtkWidget* box) {
  gtk_box_pack_start(GTK_BOX(box), tabstrip_.get(), FALSE, FALSE, 0);
}

void TabStripGtk::Layout() {
  // Called from:
  // - window resize
  // - animation completion
  if (active_animation_.get())
    active_animation_->Stop();

  GenerateIdealBounds();
  int tab_count = GetTabCount();
  for (int i = 0; i < tab_count; ++i) {
    const gfx::Rect& bounds = tab_data_.at(i).ideal_bounds;
    GetTabAt(i)->SetBounds(bounds);
  }

  gtk_widget_queue_draw(tabstrip_.get());
}

void TabStripGtk::UpdateLoadingAnimations() {
  for (int i = 0, index = 0; i < GetTabCount(); ++i, ++index) {
    TabGtk* current_tab = GetTabAt(i);
    if (current_tab->closing()) {
      --index;
    } else {
      TabContents* contents = model_->GetTabContentsAt(index);
      if (!contents || !contents->is_loading()) {
        current_tab->ValidateLoadingAnimation(TabGtk::ANIMATION_NONE);
      } else if (contents->waiting_for_response()) {
        current_tab->ValidateLoadingAnimation(TabGtk::ANIMATION_WAITING);
      } else {
        current_tab->ValidateLoadingAnimation(TabGtk::ANIMATION_LOADING);
      }
    }
  }

  gtk_widget_queue_draw(tabstrip_.get());
}

bool TabStripGtk::IsAnimating() const {
  return active_animation_.get() != NULL;
}

////////////////////////////////////////////////////////////////////////////////
// TabStripGtk, TabStripModelObserver implementation:

void TabStripGtk::TabInsertedAt(TabContents* contents,
                                int index,
                                bool foreground) {
  DCHECK(contents);
  DCHECK(index == TabStripModel::kNoTab || model_->ContainsIndex(index));

  if (active_animation_.get())
    active_animation_->Stop();

  TabGtk* tab = new TabGtk(this);

  // Only insert if we're not already in the list.
  if (index == TabStripModel::kNoTab) {
    TabData d = { tab, gfx::Rect() };
    tab_data_.push_back(d);
    tab->UpdateData(contents, false);
  } else {
    TabData d = { tab, gfx::Rect() };
    tab_data_.insert(tab_data_.begin() + index, d);
    tab->UpdateData(contents, false);
  }

  // Don't animate the first tab; it looks weird.
  if (GetTabCount() > 1) {
    StartInsertTabAnimation(index);
  } else {
    Layout();
  }
}

void TabStripGtk::TabDetachedAt(TabContents* contents, int index) {
  if (CanUpdateDisplay()) {
    GenerateIdealBounds();
    StartRemoveTabAnimation(index, contents);
    // Have to do this _after_ calling StartRemoveTabAnimation, so that any
    // previous remove is completed fully and index is valid in sync with the
    // model index.
    GetTabAt(index)->set_closing(true);
  }
}

void TabStripGtk::TabSelectedAt(TabContents* old_contents,
                                TabContents* new_contents,
                                int index,
                                bool user_gesture) {
  DCHECK(index >= 0 && index < static_cast<int>(GetTabCount()));

  if (CanUpdateDisplay()) {
    // We have "tiny tabs" if the tabs are so tiny that the unselected ones are
    // a different size to the selected ones.
    bool tiny_tabs = current_unselected_width_ != current_selected_width_;
    if (!IsAnimating() && (!resize_layout_scheduled_ || tiny_tabs)) {
      Layout();
    } else {
      gtk_widget_queue_draw(tabstrip_.get());
    }
  }
}

void TabStripGtk::TabMoved(TabContents* contents,
                           int from_index,
                           int to_index) {
  TabGtk* tab = GetTabAt(from_index);
  tab_data_.erase(tab_data_.begin() + from_index);
  TabData data = {tab, gfx::Rect()};
  tab_data_.insert(tab_data_.begin() + to_index, data);
  GenerateIdealBounds();
  // TODO(jhawkins): Remove layout call when animations are hooked up.
  Layout();
}

void TabStripGtk::TabChangedAt(TabContents* contents, int index,
                               bool loading_only) {
  // Index is in terms of the model. Need to make sure we adjust that index in
  // case we have an animation going.
  TabGtk* tab = GetTabAt(index);
  tab->UpdateData(contents, loading_only);
  tab->UpdateFromModel();
  gtk_widget_queue_draw(tabstrip_.get());
}

///////////////////////////////////////////////////////////////////////////////
// TabStripGtk, TabGtk::Delegate implementation:

bool TabStripGtk::IsTabSelected(const TabGtk* tab) const {
  if (tab->closing())
    return false;

  int tab_count = GetTabCount();
  for (int i = 0, index = 0; i < tab_count; ++i, ++index) {
    TabGtk* current_tab = GetTabAt(i);
    if (current_tab->closing())
      --index;
    if (current_tab == tab)
      return index == model_->selected_index();
  }
  return false;
}

void TabStripGtk::GetCurrentTabWidths(double* unselected_width,
                                      double* selected_width) const {
  *unselected_width = current_unselected_width_;
  *selected_width = current_selected_width_;
}

void TabStripGtk::SelectTab(TabGtk* tab) {
  int index = GetIndexOfTab(tab);
  if (model_->ContainsIndex(index))
    model_->SelectTabContentsAt(index, true);
}

void TabStripGtk::CloseTab(TabGtk* tab) {
  int tab_index = GetIndexOfTab(tab);
  if (model_->ContainsIndex(tab_index)) {
    TabGtk* last_tab = GetTabAt(GetTabCount() - 1);
    // Limit the width available to the TabStrip for laying out Tabs, so that
    // Tabs are not resized until a later time (when the mouse pointer leaves
    // the TabStrip).
    available_width_for_tabs_ = GetAvailableWidthForTabs(last_tab);
    resize_layout_scheduled_ = true;
    model_->CloseTabContentsAt(tab_index);
  }
}

bool TabStripGtk::IsCommandEnabledForTab(
    TabStripModel::ContextMenuCommand command_id, const TabGtk* tab) const {
  int index = GetIndexOfTab(tab);
  if (model_->ContainsIndex(index))
    return model_->IsContextMenuCommandEnabled(index, command_id);
  return false;
}

void TabStripGtk::ExecuteCommandForTab(
    TabStripModel::ContextMenuCommand command_id, TabGtk* tab) {
  int index = GetIndexOfTab(tab);
  if (model_->ContainsIndex(index))
    model_->ExecuteContextMenuCommand(index, command_id);
}

void TabStripGtk::StartHighlightTabsForCommand(
    TabStripModel::ContextMenuCommand command_id, TabGtk* tab) {
  if (command_id == TabStripModel::CommandCloseTabsOpenedBy) {
    int index = GetIndexOfTab(tab);
    if (model_->ContainsIndex(index)) {
      std::vector<int> indices = model_->GetIndexesOpenedBy(index);
      std::vector<int>::const_iterator iter = indices.begin();
      for (; iter != indices.end(); ++iter) {
        int current_index = *iter;
        DCHECK(current_index >= 0 && current_index < GetTabCount());
      }
    }
  }
}

void TabStripGtk::StopHighlightTabsForCommand(
    TabStripModel::ContextMenuCommand command_id, TabGtk* tab) {
  if (command_id == TabStripModel::CommandCloseTabsOpenedBy ||
      command_id == TabStripModel::CommandCloseTabsToRight ||
      command_id == TabStripModel::CommandCloseOtherTabs) {
    // Just tell all Tabs to stop pulsing - it's safe.
    StopAllHighlighting();
  }
}

void TabStripGtk::StopAllHighlighting() {
  // TODO(jhawkins): Hook up animations.
}

bool TabStripGtk::EndDrag(bool canceled) {
  // TODO(jhawkins): Tab dragging.
  return true;
}

bool TabStripGtk::HasAvailableDragActions() const {
  return model_->delegate()->GetDragActions() != 0;
}

////////////////////////////////////////////////////////////////////////////////
// TabStripGtk, private:

int TabStripGtk::GetTabCount() const {
  return static_cast<int>(tab_data_.size());
}

int TabStripGtk::GetAvailableWidthForTabs(TabGtk* last_tab) const {
  return last_tab->x() + last_tab->width();
}

int TabStripGtk::GetIndexOfTab(const TabGtk* tab) const {
  for (int i = 0, index = 0; i < GetTabCount(); ++i, ++index) {
    TabGtk* current_tab = GetTabAt(i);
    if (current_tab->closing()) {
      --index;
    } else if (current_tab == tab) {
      return index;
    }
  }
  return -1;
}

TabGtk* TabStripGtk::GetTabAt(int index) const {
  DCHECK_GE(index, 0);
  DCHECK_LT(index, GetTabCount());
  return tab_data_.at(index).tab;
}

void TabStripGtk::RemoveTabAt(int index) {
  tab_data_.erase(tab_data_.begin() + index);
  Layout();
}

void TabStripGtk::GenerateIdealBounds() {
  int tab_count = GetTabCount();
  double unselected, selected;
  GetDesiredTabWidths(tab_count, &unselected, &selected);

  current_unselected_width_ = unselected;
  current_selected_width_ = selected;

  // NOTE: This currently assumes a tab's height doesn't differ based on
  // selected state or the number of tabs in the strip!
  int tab_height = TabGtk::GetStandardSize().height();
  double tab_x = 0;
  for (int i = 0; i < tab_count; ++i) {
    TabGtk* tab = GetTabAt(i);
    double tab_width = unselected;
    if (tab->IsSelected())
      tab_width = selected;
    double end_of_tab = tab_x + tab_width;
    int rounded_tab_x = Round(tab_x);
    gfx::Rect state(rounded_tab_x, 0, Round(end_of_tab) - rounded_tab_x,
                    tab_height);
    tab_data_.at(i).ideal_bounds = state;
    tab_x = end_of_tab + kTabHOffset;
  }
}

void TabStripGtk::GetDesiredTabWidths(int tab_count,
                                      double* unselected_width,
                                      double* selected_width) const {
  const double min_unselected_width =
      TabGtk::GetMinimumUnselectedSize().width();
  const double min_selected_width =
      TabGtk::GetMinimumSelectedSize().width();

  if (tab_count == 0) {
    // Return immediately to avoid divide-by-zero below.
    *unselected_width = min_unselected_width;
    *selected_width = min_selected_width;
    return;
  }

  // Determine how much space we can actually allocate to tabs.
  int available_width = tabstrip_.get()->allocation.width;
  // TODO(jhawkins): Implement new tab button.

  // Calculate the desired tab widths by dividing the available space into equal
  // portions.  Don't let tabs get larger than the "standard width" or smaller
  // than the minimum width for each type, respectively.
  const int total_offset = kTabHOffset * (tab_count - 1);
  const double desired_tab_width = std::min(
      (static_cast<double>(available_width - total_offset) /
       static_cast<double>(tab_count)),
      static_cast<double>(TabGtk::GetStandardSize().width()));
  *unselected_width = std::max(desired_tab_width, min_unselected_width);
  *selected_width = std::max(desired_tab_width, min_selected_width);

  // When there are multiple tabs, we'll have one selected and some unselected
  // tabs.  If the desired width was between the minimum sizes of these types,
  // try to shrink the tabs with the smaller minimum.  For example, if we have
  // a strip of width 10 with 4 tabs, the desired width per tab will be 2.5.  If
  // selected tabs have a minimum width of 4 and unselected tabs have a minimum
  // width of 1, the above code would set *unselected_width = 2.5,
  // *selected_width = 4, which results in a total width of 11.5.  Instead, we
  // want to set *unselected_width = 2, *selected_width = 4, for a total width
  // of 10.
  if (tab_count > 1) {
    if ((min_unselected_width < min_selected_width) &&
        (desired_tab_width < min_selected_width)) {
      double calc_width =
          static_cast<double>(
              available_width - total_offset - min_selected_width) /
          static_cast<double>(tab_count - 1);
      *unselected_width = std::max(calc_width, min_unselected_width);
    } else if ((min_unselected_width > min_selected_width) &&
               (desired_tab_width < min_unselected_width)) {
      *selected_width = std::max(available_width - total_offset -
          (min_unselected_width * (tab_count - 1)), min_selected_width);
    }
  }
}

// Called from:
// - animation tick
void TabStripGtk::AnimationLayout(double unselected_width) {
  int tab_height = TabGtk::GetStandardSize().height();
  double tab_x = 0;
  for (int i = 0; i < GetTabCount(); ++i) {
    TabAnimation* animation = active_animation_.get();
    double tab_width = TabAnimation::GetCurrentTabWidth(this, animation, i);
    double end_of_tab = tab_x + tab_width;
    int rounded_tab_x = Round(tab_x);
    TabGtk* tab = GetTabAt(i);
    gfx::Rect bounds(rounded_tab_x, 0, Round(end_of_tab) - rounded_tab_x,
                     tab_height);
    tab->SetBounds(bounds);
    tab_x = end_of_tab + kTabHOffset;
  }
  // TODO(jhawkins): Layout new tab button.
  gtk_widget_queue_draw(tabstrip_.get());
}

void TabStripGtk::StartInsertTabAnimation(int index) {
  // The TabStrip can now use its entire width to lay out Tabs.
  available_width_for_tabs_ = -1;
  if (active_animation_.get())
    active_animation_->Stop();
  active_animation_.reset(new InsertTabAnimation(this, index));
  active_animation_->Start();
}

void TabStripGtk::StartRemoveTabAnimation(int index, TabContents* contents) {
  if (active_animation_.get()) {
    // Some animations (e.g. MoveTabAnimation) cause there to be a Layout when
    // they're completed (which includes canceled). Since |tab_data_| is now
    // inconsistent with TabStripModel, doing this Layout will crash now, so
    // we ask the MoveTabAnimation to skip its Layout (the state will be
    // corrected by the RemoveTabAnimation we're about to initiate).
    active_animation_->set_layout_on_completion(false);
    active_animation_->Stop();
  }

  active_animation_.reset(new RemoveTabAnimation(this, index, contents));
  active_animation_->Start();
}

bool TabStripGtk::CanUpdateDisplay() {
  // Don't bother laying out/painting when we're closing all tabs.
  if (model_->closing_all()) {
    // Make sure any active animation is ended, too.
    if (active_animation_.get())
      active_animation_->Stop();
    return false;
  }
  return true;
}

void TabStripGtk::FinishAnimation(TabStripGtk::TabAnimation* animation,
                                  bool layout) {
  active_animation_.reset(NULL);
  if (layout)
    Layout();
}

// static
gboolean TabStripGtk::OnExpose(GtkWidget* widget, GdkEventExpose* event,
                               TabStripGtk* tabstrip) {
  ChromeCanvasPaint canvas(event);
  if (canvas.isEmpty())
    return TRUE;

  canvas.TileImageInt(*background, 0, 0, tabstrip->bounds_.width(),
                      tabstrip->bounds_.height());

  // Paint the tabs in reverse order, so they stack to the left.
  TabGtk* selected_tab = NULL;
  int tab_count = tabstrip->GetTabCount();
  for (int i = tab_count - 1; i >= 0; --i) {
    TabGtk* tab = tabstrip->GetTabAt(i);
    // We must ask the _Tab's_ model, not ourselves, because in some situations
    // the model will be different to this object, e.g. when a Tab is being
    // removed after its TabContents has been destroyed.
    if (!tab->IsSelected()) {
      tab->Paint(&canvas);
    } else {
      selected_tab = tab;
    }
  }

  // Paint the selected tab last, so it overlaps all the others.
  if (selected_tab)
    selected_tab->Paint(&canvas);

  return TRUE;
}

// static
gboolean TabStripGtk::OnConfigure(GtkWidget* widget, GdkEventConfigure* event,
                                  TabStripGtk* tabstrip) {
  gfx::Rect bounds = gfx::Rect(event->x, event->y, event->width, event->height);
  tabstrip->SetBounds(bounds);
  tabstrip->Layout();
  return TRUE;
}

// static
gboolean TabStripGtk::OnMotionNotify(GtkWidget* widget, GdkEventMotion* event,
                                     TabStripGtk* tabstrip) {
  int old_hover_index = tabstrip->hover_index_;
  tabstrip->hover_index_ = -1;

  gfx::Point point(event->x, event->y);
  // Get a rough estimate for which tab the mouse is over.
  int index = event->x / (tabstrip->current_unselected_width_ + kTabHOffset);

  int tab_count = tabstrip->GetTabCount();
  if (index >= tab_count) {
    if (old_hover_index != -1 && old_hover_index < tab_count) {
      tabstrip->GetTabAt(old_hover_index)->SetHovering(false);
      gtk_widget_queue_draw(tabstrip->tabstrip_.get());
    }

    return TRUE;
  }

  // Tab hovering calcuation.
  // Using the rough estimate tab index, we check the tab bounds in a smart
  // order to reduce the number of tabs we need to check.  If the tab at the
  // estimated index is selected, check it first as it covers both tabs below
  // it.  Otherwise, check the tab to the left, then the estimated tab, and
  // finally the tab to the right (tabs stack to the left.)

  if (tabstrip->model()->selected_index() == index &&
      tabstrip->GetTabAt(index)->IsPointInBounds(point)) {
    tabstrip->hover_index_ = index;
  } else if (index > 0 &&
             tabstrip->GetTabAt(index - 1)->IsPointInBounds(point)) {
    tabstrip->hover_index_ = index - 1;
  } else if (tabstrip->model()->selected_index() != index &&
             tabstrip->GetTabAt(index)->IsPointInBounds(point)) {
    tabstrip->hover_index_ = index;
  } else if (index < tab_count - 1 &&
             tabstrip->GetTabAt(index + 1)->IsPointInBounds(point)) {
    tabstrip->hover_index_ = index + 1;
  }

  // Nothing to do if the indexes are the same.
  if (tabstrip->hover_index_ != old_hover_index) {
    if (tabstrip->hover_index_ != -1)
      tabstrip->GetTabAt(tabstrip->hover_index_)->SetHovering(true);

    if (old_hover_index != -1 && old_hover_index < tab_count)
      tabstrip->GetTabAt(old_hover_index)->SetHovering(false);

    gtk_widget_queue_draw(tabstrip->tabstrip_.get());
  }

  // Forward the mouse movement to the tab.  Used to handle close button input.
  if (tabstrip->hover_index_ != -1) {
    if (tabstrip->GetTabAt(tabstrip->hover_index_)->OnMotionNotify(point))
      gtk_widget_queue_draw(tabstrip->tabstrip_.get());
  }

  return TRUE;
}

// static
gboolean TabStripGtk::OnMousePress(GtkWidget* widget, GdkEventButton* event,
                                   TabStripGtk* tabstrip) {
  // TODO(jhawkins): Handle middle and right-click.
  // TODO(jhawkins): Are there no gdk constants for event->button?
  if (tabstrip->hover_index_ == -1 || event->button != 1)
    return TRUE;

  if (tabstrip->GetTabAt(tabstrip->hover_index_)->OnMousePress())
    gtk_widget_queue_draw(tabstrip->tabstrip_.get());
  else if (tabstrip->hover_index_ != tabstrip->model()->selected_index())
    tabstrip->model()->SelectTabContentsAt(tabstrip->hover_index_, true);

  return TRUE;
}

// static
gboolean TabStripGtk::OnMouseRelease(GtkWidget* widget, GdkEventButton* event,
                                     TabStripGtk* tabstrip) {
  if (event->button != 1)
    return TRUE;

  if (tabstrip->hover_index_ != -1)
    tabstrip->GetTabAt(tabstrip->hover_index_)->OnMouseRelease();

  return TRUE;
}

// static
gboolean TabStripGtk::OnLeaveNotify(GtkWidget* widget, GdkEventCrossing* event,
                                    TabStripGtk* tabstrip) {
  // A leave-notify-event is generated on mouse click, which sets the mode to
  // GDK_CROSSING_GRAB.  Ignore this event because it doesn't meant the mouse
  // has left the tabstrip.
  if (tabstrip->hover_index_ != -1 && event->mode != GDK_CROSSING_GRAB) {
    tabstrip->GetTabAt(tabstrip->hover_index_)->SetHovering(false);
    tabstrip->hover_index_ = -1;
    gtk_widget_queue_draw(tabstrip->tabstrip_.get());
  }

  return TRUE;
}
