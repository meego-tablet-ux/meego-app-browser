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

#include "base/environment.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/process.h"
#include "base/string_util.h"
#include "base/string_number_conversions.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/memory_details.h"
#include "chrome/browser/memory_purger.h"
#include "chrome/browser/tabs/phantom_tab_manager.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "chrome/browser/task_manager/task_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tab_contents/tab_contents_wrapper.h"

#include <qobject.h>
#include <QTimer>

//KB
static const int kMemThreshold = 4000000;
static const int kMinProfileInterval = 100;
static const int kMinProfileTimeout = 500;
static const int kMaxProfileTimeout = 10000;

#define CHROME_MEM_THRESHOLD "CHROME_MEM_THRESHOLD"
#define CHROME_MEM_TRACE "CHROME_MEM_TRACE"

static ResourceMonitor* g_resource_monitor_ = NULL;

class ResourceMonitor : public TaskManagerModelObserver {
 public:
  ResourceMonitor(PhantomTabManager* tab_manager) :
      task_manager_(TaskManager::GetInstance()),
      model_(TaskManager::GetInstance()->model()),
      tab_manager_(tab_manager),
      log_file_(NULL)
  {
    log_file_ = fopen("memory_trace.log", "a");
    assert(log_file_ != NULL);

    model_->AddObserver(this);
    model_->StartUpdating();
  }

  virtual ~ResourceMonitor()
  {
    model_->RemoveObserver(this);
    task_manager_->OnWindowClosed();

    fclose(log_file_);
  }

  int GetTotalPrivateMemory()
  {
    int total_pri_mem = 0;
    for (int i = 0; i < resource_list_.size(); i++)
      total_pri_mem += resource_list_[i]->pri_mem;

    return total_pri_mem;
  }

 private:
  struct ResourceItem {
    int index;
    base::ProcessHandle pid;
    TabContents* tab_contents;
    std::string title;
    int pri_mem;
    int shd_mem;
    int phy_mem;
  };

  typedef std::vector<ResourceItem*> ResourceVect;

  void SetResourceItem(ResourceItem* item, int index)
  {
    item->index = index;
    item->pid = model_->GetResourceProcessHandle(index);
    item->tab_contents = model_->GetResourceTabContents(index);
    item->title = UTF16ToUTF8(model_->GetResourceTitle(index));
    size_t size;
    if (model_->GetPrivateMemory(index, &size))
      item->pri_mem = size;
    else
      item->pri_mem = -1;

    if (model_->GetSharedMemory(index, &size))
      item->shd_mem = size;
    else
      item->shd_mem = -1;

    if (model_->GetPhysicalMemory(index, &size))
      item->phy_mem = size;
    else
      item->phy_mem = -1;
  }

  void Dump()
  {
    fprintf(log_file_, ">>> %lld \n", base::Time::Now().ToInternalValue());
    int i = 0;
    int total_pri = 0;
    int total_shd = 0;
    int total_phy = 0;
    while(i < resource_list_.size())
    {
      std::string title;
      if (resource_list_[i]->tab_contents)
        title = "Render";
      else
        title = resource_list_[i]->title;
      fprintf(log_file_, "%i %s %i %i %i\n",
              resource_list_[i]->pid,
              title.c_str(),
              resource_list_[i]->pri_mem,
              resource_list_[i]->shd_mem,
              resource_list_[i]->phy_mem);

      if (model_->IsResourceFirstInGroup(i))
      {
        total_pri += resource_list_[i]->pri_mem;
        total_shd += resource_list_[i]->shd_mem;
        total_shd += resource_list_[i]->phy_mem;
        std::pair<int, int> group = model_->GetGroupRangeForResource(i);
        while(i < group.first + group.second)
        {
          if (resource_list_[i]->tab_contents)
            fprintf(log_file_, "\tPage %s %s\n",
                    resource_list_[i]->tab_contents->GetURL().spec().c_str(),
                    resource_list_[i]->tab_contents->is_loading() ? "loading" : "loaded");
          i++;
        }
      }
      else
      {
        i++;
      }
    }

    TabStripModel* model = tab_manager_->browser_->tabstrip_model();
    for(int i = 0; i < model->count(); i++)
    {
      if(model->IsPhantomTab(i)) {
        TabContents* tab = model->GetTabContentsAt(i)->tab_contents();
        fprintf(log_file_, "\tPage %s %s\n", tab->GetURL().spec().c_str(), "phantom");
      }
    }
    
    fprintf(log_file_, "<<< %i %i %i\n", total_pri, total_shd, total_phy);
    fflush(log_file_);
  }
  // TaskManagerModelObserver
  virtual void OnModelChanged()
  {
  }

  virtual void OnItemsChanged(int start, int length)
  {
    for ( int i = start; i < start + length; i++)
    {
      ResourceItem* item = resource_list_[i];
      SetResourceItem(item, i);
    }
    Dump();
  }

  virtual void OnItemsAdded(int start, int length)
  {
    ResourceVect::iterator itr = resource_list_.begin();
    itr += start;
    for (int i = start; i < start + length; i++)
    {
      ResourceItem* item = new ResourceItem;
      SetResourceItem(item, i);
      itr = resource_list_.insert(itr, item);
      itr++;
    }
    Dump();
  }

  virtual void OnItemsRemoved(int start, int length)
  {
    ResourceVect::iterator itr = resource_list_.begin();
    itr += start;
    for (int i = start; i < start + length; i++)
    {
      delete *itr;
      itr = resource_list_.erase(itr);
    }
    Dump();
  }

 private:
    // The task manager.
  TaskManager* task_manager_;

  // Our model.
  TaskManagerModel* model_;

  std::vector<ResourceItem*> resource_list_;

  PhantomTabManager* tab_manager_;

  FILE* log_file_;
};

class MemoryProfiler : public MemoryDetails {
 public:
  MemoryProfiler(PhantomTabManager* tab_manager):
      tab_manager_(tab_manager)
  {
  }

  virtual void OnDetailsAvailable() {
    const std::vector<ProcessData>& browser_processes = processes();

    std::wstring log_string;
    ProcessMemoryInformation aggregate;
    DLOG(INFO) << "+++++++ Memory Details Start +++++++ ";
    // Aggregate per-process data into browser summary data.
    for (size_t index = 0; index < browser_processes.size(); index++) {
      if (browser_processes[index].processes.size() == 0)
        continue;

      // Sum the information for the processes within this browser.
      ProcessMemoryInformationList::const_iterator iterator;
      iterator = browser_processes[index].processes.begin();
      aggregate.pid = iterator->pid;
      aggregate.version = iterator->version;
      while (iterator != browser_processes[index].processes.end()) {
        if (!iterator->is_diagnostics ||
            browser_processes[index].processes.size() == 1) {
          aggregate.working_set.priv += iterator->working_set.priv;
          aggregate.working_set.shared += iterator->working_set.shared;
          aggregate.working_set.shareable += iterator->working_set.shareable;
          aggregate.committed.priv += iterator->committed.priv;
          aggregate.committed.mapped += iterator->committed.mapped;
          aggregate.committed.image += iterator->committed.image;
          aggregate.num_processes++;
        }
        std::wstring process_log;
        process_log.append(L"[");
        process_log.append(UTF8ToWide(base::IntToString(iterator->pid)));
        process_log.append(L"] ");
        process_log.append(UTF8ToWide(ChildProcessInfo::GetTypeNameInEnglish(iterator->type)));
        process_log.append(L" (");
        for (int i = 0; i < iterator->titles.size(); i++)
        {
          process_log.append(UTF16ToWide(iterator->titles[i]));
          if ( i != iterator->titles.size() - 1)
            process_log.append(L", ");
        }
        process_log.append(L")");
        process_log.append(L": ");
        process_log.append(UTF8ToWide(
            base::Int64ToString(iterator->working_set.priv)));
        process_log.append(L", ");
        process_log.append(UTF8ToWide(
            base::Int64ToString(iterator->working_set.shared)));
        process_log.append(L", ");
        process_log.append(UTF8ToWide(
            base::Int64ToString(iterator->working_set.shareable)));
        DLOG(INFO) << process_log;

        ++iterator;
      }

      // We log memory info as we record it.
      if (log_string.size() != 0)
        log_string.append(L"\n");
      log_string.append(UTF16ToWide(browser_processes[index].name));
      log_string.append(L": ");
      log_string.append(UTF8ToWide(
          base::Int64ToString(aggregate.working_set.priv)));
      log_string.append(L", ");
      log_string.append(UTF8ToWide(
          base::Int64ToString(aggregate.working_set.shared)));
      log_string.append(L", ");
      log_string.append(UTF8ToWide(
          base::Int64ToString(aggregate.working_set.shareable)));
    }
    DLOG(INFO) << log_string;
    DLOG(INFO) << "+++++++ Memory Details End +++++++ ";

    tab_manager_->MemoryProfileDone(aggregate.working_set.priv);
  }

 private:
  ~MemoryProfiler(){}

  PhantomTabManager* tab_manager_;

  DISALLOW_COPY_AND_ASSIGN(MemoryProfiler);
};

class ProfileTimer : public QObject
{
 Q_OBJECT
 public:
  ProfileTimer(PhantomTabManager* tab_manager):
      tab_manager_(tab_manager)
  {
    timer_ = new QTimer(this);
    timer_->setSingleShot(true);
    QObject::connect(timer_, SIGNAL(timeout()), this, SLOT(onTimeout()));
  }

  ~ProfileTimer()
  {
    delete timer_;
  }

  void timeout(int msec)
  {
    if(timer_->isActive())
      timer_->stop();
    timer_->start(msec);
  }

  void stop()
  {
    timer_->stop();
  }

 public Q_SLOTS:
  void onTimeout()
  {
    tab_manager_->ProfileTimeout();
  }

 private:
  QTimer* timer_;
  PhantomTabManager* tab_manager_;
};

PhantomTabManager::PhantomTabManager(Browser* browser, int memory_threshold):
    browser_(browser),
    factory_(this),
    memory_threshold_(memory_threshold),
    last_profile_time_(base::Time::Now()),
    last_total_mem_(0),
    profile_timeout_scale_(1)
{
  if (memory_threshold_ < 1000 || memory_threshold_ > 4000000)
    memory_threshold_ = kMemThreshold;
  DLOG(INFO) << "Chrome memory threshold " << memory_threshold_;

  tab_strip_model_ = browser_->tabstrip_model();
  tab_strip_model_->AddObserver(this);

  scoped_ptr<base::Environment> env(base::Environment::Create());
  std::string trace_str;
  if (env->GetVar(CHROME_MEM_TRACE, &trace_str) && !trace_str.empty())
  {
    if (g_resource_monitor_ == NULL)
    {
      g_resource_monitor_ = new ResourceMonitor(this);
      DLOG(INFO) << "Chrome memory trace starts ";
    }
  }

  timer_ = new ProfileTimer(this);
}

PhantomTabManager::~PhantomTabManager()
{
  tab_strip_model_->RemoveObserver(this);
  if (g_resource_monitor_)
    delete g_resource_monitor_;

  delete timer_;
}

void PhantomTabManager::MemoryProfileDone(int total_priv_mem)
{
  base::TimeDelta delta = base::Time::Now() - last_profile_time_;
  if (delta.InMillisecondsRoundedUp() != 0)
    memory_change_rate_ = ::abs(total_priv_mem - last_total_mem_) / delta.InMillisecondsRoundedUp();

  last_profile_time_ = base::Time::Now();
  last_total_mem_ = total_priv_mem;

  if (total_priv_mem > memory_threshold_)
  {
    MakePhantomTab();
  }
}

void PhantomTabManager::TabInsertedAt(TabContents* contents,
                                      int index,
                                      bool foreground)
{
  TabItem* item = new TabItem;
  item->tab_contents = contents;
  item->last_visit_time = base::Time::Now();
  item->visit_count = 1;

  tab_map_.insert(TabMapItem(contents, item));
  tab_list_.push_back(item);

  SortTabList();

  StartMemoryProfile();
}

void PhantomTabManager::TabDetachedAt(TabContents* contents, int index)
{
  TabMap::iterator itr = tab_map_.find(contents);
  if (itr != tab_map_.end())
  {
    tab_map_.erase(itr);
  }

  TabList::iterator list_itr = tab_list_.begin();
  for (; list_itr != tab_list_.end(); list_itr++)
  {
    if ((*list_itr)->tab_contents == contents)
    {
      tab_list_.erase(list_itr);
      break;
    }
  }

  if (list_itr != tab_list_.end())
    delete *list_itr;

  SortTabList();

  StartMemoryProfile();

  RenderProcessHost* host = contents->GetRenderProcessHost();
  MessageLoop::current()->PostTask(FROM_HERE,
                                   factory_.NewRunnableMethod(&PhantomTabManager::PurgeRenderForHost, host));
}

void PhantomTabManager::TabSelectedAt(TabContents* old_contents,
                                      TabContents* contents,
                                      int index,
                                      bool user_gesture)
{
  TabMap::iterator itr = tab_map_.find(contents);
  if (itr != tab_map_.end())
  {
    (*itr).second->last_visit_time = base::Time::Now();
    (*itr).second->visit_count++;
  }

  SortTabList();

  StartMemoryProfile();
}

void PhantomTabManager::TabMoved(TabContents* contents,
                                 int from_index,
                                 int to_index)
{
}

void PhantomTabManager::TabChangedAt(TabContents* contents, int index,
                                     TabChangeType change_type)
{
  StartMemoryProfile();
}

void PhantomTabManager::TabReplacedAt(TabContents* old_contents,
                                      TabContents* new_contents,
                                      int index)
{
  TabMap::iterator itr = tab_map_.find(old_contents);
  TabItem* item = NULL;
  if (itr != tab_map_.end())
  {
    item = (*itr).second;
    item->tab_contents = new_contents;
    tab_map_.erase(itr);
    tab_map_.insert(TabMapItem(new_contents, item));
  }

  StartMemoryProfile();

  RenderProcessHost* host = old_contents->GetRenderProcessHost();
  MessageLoop::current()->PostTask(FROM_HERE,
                                   factory_.NewRunnableMethod(&PhantomTabManager::PurgeRenderForHost, host));
}

void PhantomTabManager::TabMiniStateChanged(TabContents* contents, int index)
{
}

void PhantomTabManager::TabBlockedStateChanged(TabContents* contents,
                                               int index)
{
}

void PhantomTabManager::TabStripEmpty()
{
}

void PhantomTabManager::TabStripModelDeleted()
{
}

void PhantomTabManager::MakePhantomTab()
{
  int index = SelectTabToPhantom();
  DLOG(INFO) << "MakePhantomTab: selected phantom tab is " << (index == -1 ? "NULL" : UTF16ToUTF8(tab_strip_model_->GetTabContentsAt(index)->tab_contents()->GetTitle()));
  if (index == -1)
    return;
  MessageLoop::current()->PostTask(FROM_HERE,
                                   factory_.NewRunnableMethod(&PhantomTabManager::ConvertTabToPhantom, index));
}

int PhantomTabManager::SelectTabToPhantom()
{
  TabList::iterator itr = tab_list_.begin();
  int index = -1;
  for(; itr != tab_list_.end(); itr++)
  {
    TabItem *item = (*itr);
    TabContentsWrapper  *tab_contents = new TabContentsWrapper(item->tab_contents);
    index = tab_strip_model_->GetIndexOfTabContents(tab_contents);
    if (!tab_strip_model_->IsPhantomTab(index))
    {
      break;
    }
  }

  if (itr == tab_list_.end())
    return -1;

  if (itr != tab_list_.end() &&
      tab_strip_model_->GetSelectedTabContents()->tab_contents() == (*itr)->tab_contents)
    return -1;

  return index;
}

bool compare_tab (TabItem* first, TabItem* second)
{
   return first->last_visit_time < second->last_visit_time;
}

void PhantomTabManager::SortTabList()
{
  tab_list_.sort(compare_tab);
}

void PhantomTabManager::ConvertTabToPhantom(int index)
{
  if (tab_strip_model_->ContainsIndex(index))
  {
    tab_strip_model_->SetTabToPhantom(index);
  }
}

void PhantomTabManager::PurgeRenderForHost(RenderProcessHost* host)
{
  MemoryPurger::PurgeRendererForHost(host);
}

void PhantomTabManager::StartMemoryProfile()
{
  base::TimeDelta delta = base::Time::Now() - last_profile_time_;
  if (delta.InMillisecondsRoundedUp() < kMinProfileInterval)
    return;

  scoped_refptr<MemoryProfiler>
      profiler(new MemoryProfiler(this));
  profiler->StartFetch();

  profile_timeout_scale_ = 1;
  timer_->timeout(kMinProfileTimeout * profile_timeout_scale_);
}

void PhantomTabManager::ProfileTimeout()
{
  profile_timeout_scale_ = profile_timeout_scale_ << 1;
  int msec = profile_timeout_scale_ * kMinProfileTimeout;
  if ( msec > kMaxProfileTimeout)
  {
    DLOG(INFO) << "ProfileTimeout: timer stop";
    timer_->stop();
  }
  else
  {
    DLOG(INFO) << "ProfileTimeout: reshedule " << msec;
    timer_->timeout(msec);
  }

  scoped_refptr<MemoryProfiler>
      profiler(new MemoryProfiler(this));
  profiler->StartFetch();
}

#include "moc_phantom_tab_manager.cc"
