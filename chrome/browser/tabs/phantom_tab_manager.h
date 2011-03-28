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

#ifndef PHANTOM_TAB_MANAGER_H
#define PHANTOM_TAB_MANAGER_H

#include <list>
#include <map>

#include "chrome/browser/tabs/tab_strip_model.h"
#include "content/common/notification_observer.h"
#include "base/task.h"
#include "base/time.h"

class Browser;
class ResourceMonitor;
class MemoryProfiler;
class RenderProcessHost;
class ProfileTimer;

struct TabItem {
  TabContents* tab_contents;
  base::Time last_visit_time;
  unsigned int visit_count;
};

class PhantomTabManager : public TabStripModelObserver
{
  friend class ResourceMonitor;
  friend class ProfileTimer;
 public:
  PhantomTabManager(Browser* browser, int threshold);
  virtual ~PhantomTabManager();

  // TabStripModelObserver implementation:
  virtual void TabInsertedAt(TabContents* contents,
                             int index,
                             bool foreground);
  virtual void TabDetachedAt(TabContents* contents, int index);
  virtual void TabSelectedAt(TabContents* old_contents,
                             TabContents* contents,
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
  virtual void TabBlockedStateChanged(TabContents* contents,
                                      int index);
  virtual void TabStripEmpty();
  virtual void TabStripModelDeleted();

  void MemoryProfileDone(int priv_mem);

  typedef std::list<TabItem*> TabList;
  typedef std::map<TabContents*, TabItem*> TabMap;
  typedef std::pair<TabContents*, TabItem*> TabMapItem;

  void ConvertTabToPhantom(int index);
  void SortTabList();

  void MakePhantomTab();
  int SelectTabToPhantom();

  void StartMemoryProfile();
  void PurgeRenderForHost(RenderProcessHost* host);

  void ProfileTimeout();
private:
  Browser* browser_;
  TabStripModel* tab_strip_model_;
  MemoryProfiler* memory_profiler_;

  TabList tab_list_;
  TabMap tab_map_;

  int memory_threshold_;
  int last_total_mem_;
  base::Time last_profile_time_;
  double memory_change_rate_;

  ProfileTimer* timer_;
  int profile_timeout_scale_;

  ScopedRunnableMethodFactory<PhantomTabManager> factory_;

  DISALLOW_COPY_AND_ASSIGN(PhantomTabManager);
};

#endif
