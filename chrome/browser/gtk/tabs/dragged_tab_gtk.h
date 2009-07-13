// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GTK_TABS_DRAGGED_TAB_GTK_H_
#define CHROME_BROWSER_GTK_TABS_DRAGGED_TAB_GTK_H_

#include <gtk/gtk.h>

#include "app/gfx/canvas.h"
#include "app/slide_animation.h"
#include "base/gfx/point.h"
#include "base/gfx/rect.h"
#include "base/gfx/size.h"
#include "base/scoped_ptr.h"
#include "base/task.h"
#include "chrome/browser/renderer_host/backing_store.h"

class TabContents;
class TabRendererGtk;

class DraggedTabGtk : public AnimationDelegate {
 public:
  DraggedTabGtk(TabContents* datasource,
                const gfx::Point& mouse_tab_offset,
                const gfx::Size& contents_size);
  virtual ~DraggedTabGtk();

  // Moves the dragged tab to the appropriate location given the mouse
  // pointer at |screen_point|.
  void MoveTo(const gfx::Point& screen_point);

  // Sets the offset of the mouse from the upper left corner of the tab.
  void set_mouse_tab_offset(const gfx::Point& offset) {
    mouse_tab_offset_ = offset;
  }

  // Sets the non-pinned tab width. The actual width of the dragged tab is the
  // value last past to Attach or Resize. |tab_width| is used when Detach is
  // invoked (which triggers resizing to |tab_width|), or when dragging within
  // a tab strip and the dragged tab changes state from pinned to non-pinned.
  void set_tab_width(int tab_width) { tab_width_ = tab_width; }
  int tab_width() const { return tab_width_; }

  // Notifies the dragged tab that it has become attached to a tabstrip.
  void Attach(int selected_width);

  // Resizes the dragged tab to a width of |width|.
  void Resize(int width);

  // Sets whether the tab is rendered pinned or not.
  void set_pinned(bool pinned);
  bool is_pinned() const;

  // Notifies the dragged tab that it has been detached from a tabstrip.
  // |contents| is the widget that contains the dragged tab contents, while
  // |backing_store| is the backing store that holds a server-side bitmap of the
  // visual representation of |contents|.
  void Detach(GtkWidget* contents, BackingStore* backing_store);

  // Notifies the dragged tab that it should update itself.
  void Update();

  // Animates the dragged tab to the specified bounds, then calls back to
  // |callback|.
  typedef Callback0::Type AnimateToBoundsCallback;
  void AnimateToBounds(const gfx::Rect& bounds,
                       AnimateToBoundsCallback* callback);

  // Returns the size of the dragged tab. Used when attaching to a tabstrip
  // to determine where to place the tab in the attached tabstrip.
  const gfx::Size& attached_tab_size() const { return attached_tab_size_; }

  GtkWidget* widget() const { return container_; }

 private:
  // Overridden from AnimationDelegate:
  virtual void AnimationProgressed(const Animation* animation);
  virtual void AnimationEnded(const Animation* animation);
  virtual void AnimationCanceled(const Animation* animation);

  // Arranges the contents of the dragged tab.
  void Layout();

  // Gets the preferred size of the dragged tab.
  gfx::Size GetPreferredSize();

  // Resizes the container to fit the content for the current attachment mode.
  void ResizeContainer();

  // Utility for scaling a size by the current scaling factor.
  int ScaleValue(int value);

  // Returns the bounds of the container window.
  gfx::Rect bounds() const;

  // Sets the color map of the container window to allow the window to be
  // transparent.
  void SetContainerColorMap();

  // Sets full transparency for the container window.  This is used if
  // compositing is available for the screen.
  void SetContainerTransparency();

  // Sets the shape mask for the container window to emulate a transparent
  // container window.  This is used if compositing is not available for the
  // screen.
  // |pixbuf| is the pixbuf for the tab only (not the render view).
  void SetContainerShapeMask(GdkPixbuf* pixbuf);

  // Paints the tab. The returned pixbuf belongs to the caller.
  GdkPixbuf* PaintTab();

  // expose-event handler that notifies when the tab needs to be redrawn.
  static gboolean OnExposeEvent(GtkWidget* widget, GdkEventExpose* event,
                                DraggedTabGtk* dragged_tab);

  // The window that contains the dragged tab or tab contents.
  GtkWidget* container_;

  // The native view of the tab contents.
  GtkWidget* contents_;

  // The backing store used to create a screenshot of the dragged contents.
  // Owned by the RWH.
  BackingStore* backing_store_;

  // The renderer that paints the dragged tab.
  scoped_ptr<TabRendererGtk> renderer_;

  // True if the view is currently attached to a tabstrip. Controls rendering
  // and sizing modes.
  bool attached_;

  // The unscaled offset of the mouse from the top left of the dragged tab.
  // This is used to maintain an appropriate offset for the mouse pointer when
  // dragging scaled and unscaled representations, and also to calculate the
  // position of detached windows.
  gfx::Point mouse_tab_offset_;

  // The size of the tab renderer when the dragged tab is attached to a
  // tabstrip.
  gfx::Size attached_tab_size_;

  // The dimensions of the TabContents being dragged.
  gfx::Size contents_size_;

  // The animation used to slide the attached tab to its final location.
  SlideAnimation close_animation_;

  // A callback notified when the animation is complete.
  scoped_ptr<Callback0::Type> animation_callback_;

  // The start and end bounds of the animation sequence.
  gfx::Rect animation_start_bounds_;
  gfx::Rect animation_end_bounds_;

  // Non-pinned tab width. See description above setter for how this is used.
  int tab_width_;

  DISALLOW_COPY_AND_ASSIGN(DraggedTabGtk);
};

#endif  // CHROME_BROWSER_GTK_TABS_DRAGGED_TAB_GTK_H_
