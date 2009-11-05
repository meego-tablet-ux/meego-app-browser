// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PROCESS_INFO_SNAPSHOT_H_
#define CHROME_BROWSER_PROCESS_INFO_SNAPSHOT_H_

#include <sys/types.h>

#include <map>
#include <string>
#include <vector>

#include "base/process_util.h"

// A class which captures process information at a given point in time when its
// |Sample()| method is called. This information can then be probed by PID.
// |Sample()| may take a while to complete, so if calling from the browser
// process, only do so from the file thread. The current implementation, only on
// Mac, pulls information from /bin/ps. /usr/bin/top provides much more
// information about memory, but it has changed greatly from Mac OS 10.5.x to
// 10.6.x, thereby raising future compatibility concerns. Moreover, the 10.6.x
// version is less capable in terms of configuring output and its output is
// harder to parse.
// TODO(viettrungluu): This is currently only implemented and used on Mac, so
// things are very Mac-specific. If this is ever implemented for other
// platforms, we should subclass and add opaqueness (probably |ProcInfoEntry|
// should be considered opaque).
class ProcessInfoSnapshot {
 public:
  ProcessInfoSnapshot();
  ~ProcessInfoSnapshot();

  // Capture a snapshot of process memory information (by running ps) for the
  // given list of PIDs. Call only from the file thread.
  //   |pid_list| - list of |ProcessId|s on which to capture information.
  //   returns - |true| if okay, |false| on error.
  bool Sample(std::vector<base::ProcessId> pid_list);

  // Reset all statistics (deallocating any memory allocated).
  void Reset();

  // Our basic structure for storing information about a process (the names are
  // mostly self-explanatory). Note that |command| may not actually reflect the
  // actual executable name; never trust it absolutely, and only take it
  // half-seriously when it begins with '/'.
  struct ProcInfoEntry {
    base::ProcessId pid;
    base::ProcessId ppid;
    uid_t uid;
    uid_t euid;
    size_t rss;
    size_t vsize;
    std::string command;
  };

  // Get process information for a given PID.
  //   |pid| - self-explanatory.
  //   |proc_info| - place to put the process information.
  //   returns - |true| if okay, |false| on error (including PID not found).
  bool GetProcInfo(int pid,
                   ProcInfoEntry* proc_info) const;

  // Fills a |CommittedKBytes| with both resident and paged memory usage, as per
  // its definition (or as close as we can manage). In the current (Mac)
  // implementation, we map:
  //                              vsize --> comm_priv,
  //                                  0 --> comm_mapped,
  //                                  0 --> comm_image;
  //   in about:memory: virtual:private  =  comm_priv,
  //                     virtual:mapped  =  comm_mapped.
  // TODO(viettrungluu): Doing such a mapping is kind of ugly.
  //   |pid| - self-explanatory.
  //   |usage| - pointer to |CommittedBytes| to fill; zero-ed on error.
  //   returns - |true| on success, |false| on error (including PID not found).
  bool GetCommittedKBytesOfPID(int pid,
                               base::CommittedKBytes* usage) const;

  // Fills a |WorkingSetKBytes| containing resident private and shared memory,
  // as per its definition (or as close as we can manage). In the current (Mac)
  // implementation, we map:
  //                              0 --> ws_priv,
  //                            rss --> ws_shareable,
  //                              0 --> ws_shared;
  //   in about:memory: res:private  =  ws_priv + ws_shareable - ws_shared,
  //                     res:shared  =  ws_shared / num_procs,
  //                      res:total  =  res:private + res:shared.
  // TODO(viettrungluu): Doing such a mapping is kind of ugly.
  //   |pid| - self-explanatory.
  //   |ws_usage| - pointer to |WorkingSetKBytes| to fill; zero-ed on error.
  //   returns - |true| on success, |false| on error (including PID not found).
  bool GetWorkingSetKBytesOfPID(int pid,
                                base::WorkingSetKBytes* ws_usage) const;

  // TODO(viettrungluu): Maybe we should also have the following (again, for
  // "compatibility"):
  //    size_t GetWorkingSetSizeOfPID(int pid) const;
  //    size_t GetPeakWorkingSetSizeOfPID(int pid) const;
  //    size_t GetPrivateBytesOfPID(int pid) const;

 private:
  // map from |int| (PID) to |ProcInfoEntry|
  std::map<int,ProcInfoEntry> proc_info_entries_;
};

#endif  // CHROME_BROWSER_PROCESS_INFO_SNAPSHOT_H_
