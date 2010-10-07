// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ACCESSIBILITY_BROWSER_ACCESSIBILITY_MANAGER_H_
#define CHROME_BROWSER_ACCESSIBILITY_BROWSER_ACCESSIBILITY_MANAGER_H_
#pragma once

#include <vector>

#include "base/hash_tables.h"
#include "base/scoped_ptr.h"
#include "build/build_config.h"
#include "chrome/common/render_messages_params.h"
#include "gfx/native_widget_types.h"
#include "webkit/glue/webaccessibility.h"


class BrowserAccessibility;
#if defined(OS_WIN)
class BrowserAccessibilityManagerWin;
#endif

using webkit_glue::WebAccessibility;

// Class that can perform actions on behalf of the BrowserAccessibilityManager.
class BrowserAccessibilityDelegate {
 public:
  virtual ~BrowserAccessibilityDelegate() {}
  virtual void SetAccessibilityFocus(int acc_obj_id) = 0;
  virtual void AccessibilityDoDefaultAction(int acc_obj_id) = 0;
  virtual bool HasFocus() = 0;
};

class BrowserAccessibilityFactory {
 public:
  virtual ~BrowserAccessibilityFactory() {}

  // Create an instance of BrowserAccessibility and return a new
  // reference to it.
  virtual BrowserAccessibility* Create();
};

// Manages a tree of BrowserAccessibility objects.
class BrowserAccessibilityManager {
 public:
  // Creates the platform specific BrowserAccessibilityManager. Ownership passes
  // to the caller.
  static BrowserAccessibilityManager* Create(
    gfx::NativeView parent_view,
    const WebAccessibility& src,
    BrowserAccessibilityDelegate* delegate,
    BrowserAccessibilityFactory* factory = new BrowserAccessibilityFactory());

  virtual ~BrowserAccessibilityManager();

  virtual void NotifyAccessibilityEvent(
      ViewHostMsg_AccessibilityNotification_Params::NotificationType n,
      BrowserAccessibility* node) = 0;

  // Returns the next unique child id.
  static int32 GetNextChildID();

  // Return a pointer to the root of the tree, does not make a new reference.
  BrowserAccessibility* GetRoot();

  // Removes the BrowserAccessibility child_id from the manager.
  void Remove(int32 child_id);

  // Return a pointer to the object corresponding to the given child_id,
  // does not make a new reference.
  BrowserAccessibility* GetFromChildID(int32 child_id);

  // Called to notify the accessibility manager that its associated native
  // view got focused.
  void GotFocus();

  // Tell the renderer to set focus to this node.
  void SetFocus(const BrowserAccessibility& node);

  // Tell the renderer to do the default action for this node.
  void DoDefaultAction(const BrowserAccessibility& node);

  // Called when the renderer process has notified us of about tree changes.
  // Send a notification to MSAA clients of the change.
  void OnAccessibilityNotifications(
    const std::vector<ViewHostMsg_AccessibilityNotification_Params>& params);

  gfx::NativeView GetParentView();

#if defined(OS_WIN)
  BrowserAccessibilityManagerWin* toBrowserAccessibilityManagerWin();
#endif

  // Return the object that has focus, if it's a descandant of the
  // given root (inclusive). Does not make a new reference.
  BrowserAccessibility* GetFocus(BrowserAccessibility* root);

 protected:
  BrowserAccessibilityManager(
      gfx::NativeView parent_view,
      const WebAccessibility& src,
      BrowserAccessibilityDelegate* delegate,
      BrowserAccessibilityFactory* factory);

 private:
  void OnAccessibilityObjectStateChange(
      const WebAccessibility& acc_obj);
  void OnAccessibilityObjectChildrenChange(
      const WebAccessibility& acc_obj);
  void OnAccessibilityObjectFocusChange(
      const WebAccessibility& acc_obj);
  void OnAccessibilityObjectLoadComplete(
      const WebAccessibility& acc_obj);
  void OnAccessibilityObjectValueChange(
      const WebAccessibility& acc_obj);
  void OnAccessibilityObjectTextChange(
      const WebAccessibility& acc_obj);

  // Recursively compare the IDs of our subtree to a new subtree received
  // from the renderer and return true if their IDs match exactly.
  bool CanModifyTreeInPlace(
      BrowserAccessibility* current_root,
      const WebAccessibility& new_root);

  // Recursively modify a subtree (by reinitializing) to match a new
  // subtree received from the renderer process. Should only be called
  // if CanModifyTreeInPlace returned true.
  void ModifyTreeInPlace(
      BrowserAccessibility* current_root,
      const WebAccessibility& new_root);

  // Update the accessibility tree with an updated WebAccessibility tree or
  // subtree received from the renderer process. First attempts to modify
  // the tree in place, and if that fails, replaces the entire subtree.
  // Returns the updated node or NULL if no node was updated.
  BrowserAccessibility* UpdateTree(
      const WebAccessibility& acc_obj);

  // Recursively build a tree of BrowserAccessibility objects from
  // the WebAccessibility tree received from the renderer process.
  BrowserAccessibility* CreateAccessibilityTree(
      BrowserAccessibility* parent,
      int child_id,
      const WebAccessibility& src,
      int index_in_parent);

 protected:
  // The next unique id for a BrowserAccessibility instance.
  static int32 next_child_id_;

  // The parent view.
  gfx::NativeView parent_view_;

  // The object that can perform actions on our behalf.
  BrowserAccessibilityDelegate* delegate_;

  // Factory to create BrowserAccessibility objects (for dependency injection).
  scoped_ptr<BrowserAccessibilityFactory> factory_;

  // The root of the tree of IAccessible objects and the element that
  // currently has focus, if any.
  BrowserAccessibility* root_;
  BrowserAccessibility* focus_;

  // A mapping from the IDs of objects in the renderer, to the child IDs
  // we use internally here.
  base::hash_map<int, int32> renderer_id_to_child_id_map_;

  // A mapping from child IDs to BrowserAccessibility objects.
  base::hash_map<int32, BrowserAccessibility*> child_id_map_;

  DISALLOW_COPY_AND_ASSIGN(BrowserAccessibilityManager);
};

#endif  // CHROME_BROWSER_ACCESSIBILITY_BROWSER_ACCESSIBILITY_MANAGER_H_
