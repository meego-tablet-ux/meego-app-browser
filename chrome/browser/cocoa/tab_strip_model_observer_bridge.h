// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_COCOA_TAB_STRIP_MODEL_OBSERVER_BRIDGE_H_
#define CHROME_BROWSER_COCOA_TAB_STRIP_MODEL_OBSERVER_BRIDGE_H_
#pragma once

#import <Foundation/Foundation.h>

#include "chrome/browser/tabs/tab_strip_model_observer.h"

class TabContents;
class TabStripModel;

// A C++ bridge class to handle receiving notifications from the C++ tab strip
// model. When the caller allocates a bridge, it automatically registers for
// notifications from |model| and passes messages to |controller| via the
// informal protocol below. The owner of this object is responsible for deleting
// it (and thus unhooking notifications) before |controller| is destroyed.
class TabStripModelObserverBridge : public TabStripModelObserver {
 public:
  TabStripModelObserverBridge(TabStripModel* model, id controller);
  virtual ~TabStripModelObserverBridge();

  // Overridden from TabStripModelObserver
  virtual void TabInsertedAt(TabContents* contents,
                             int index,
                             bool foreground);
  virtual void TabClosingAt(TabStripModel* tab_strip_model,
                            TabContents* contents,
                            int index);
  virtual void TabDetachedAt(TabContents* contents, int index);
  virtual void TabSelectedAt(TabContents* old_contents,
                             TabContents* new_contents,
                             int index,
                             bool user_gesture);
  virtual void TabMoved(TabContents* contents,
                        int from_index,
                        int to_index);
  virtual void TabChangedAt(TabContents* contents, int index,
                            TabChangeType change_type);
  virtual void TabReplacedAt(TabContents* old_contents,
                             TabContents* new_contents,
                             int index);
  virtual void TabMiniStateChanged(TabContents* contents, int index);
  virtual void TabStripEmpty();
  virtual void TabStripModelDeleted();

 private:
  id controller_;  // weak, owns me
  TabStripModel* model_;  // weak, owned by Browser
};

// A collection of methods which can be selectively implemented by any
// Cocoa object to receive updates about changes to a tab strip model. It is
// ok to not implement them, the calling code checks before calling.
@interface NSObject(TabStripModelBridge)
- (void)insertTabWithContents:(TabContents*)contents
                      atIndex:(NSInteger)index
                 inForeground:(bool)inForeground;
- (void)tabClosingWithContents:(TabContents*)contents
                       atIndex:(NSInteger)index;
- (void)tabDetachedWithContents:(TabContents*)contents
                        atIndex:(NSInteger)index;
- (void)selectTabWithContents:(TabContents*)newContents
             previousContents:(TabContents*)oldContents
                      atIndex:(NSInteger)index
                  userGesture:(bool)wasUserGesture;
- (void)tabMovedWithContents:(TabContents*)contents
                    fromIndex:(NSInteger)from
                      toIndex:(NSInteger)to;
- (void)tabChangedWithContents:(TabContents*)contents
                       atIndex:(NSInteger)index
                    changeType:(TabStripModelObserver::TabChangeType)change;
- (void)tabReplacedWithContents:(TabContents*)newContents
               previousContents:(TabContents*)oldContents
                        atIndex:(NSInteger)index;
- (void)tabMiniStateChangedWithContents:(TabContents*)contents
                                atIndex:(NSInteger)index;
- (void)tabStripEmpty;
- (void)tabStripModelDeleted;
@end

#endif  // CHROME_BROWSER_COCOA_TAB_STRIP_MODEL_OBSERVER_BRIDGE_H_
