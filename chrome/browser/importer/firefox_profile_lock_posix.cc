// Copyright (c) 2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/importer/firefox_profile_lock.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "base/file_util.h"

// This class is based on Firefox code in:
//   profile/dirserviceprovider/src/nsProfileLock.cpp
// The license block is:

/* ***** BEGIN LICENSE BLOCK *****
* Version: MPL 1.1/GPL 2.0/LGPL 2.1
*
* The contents of this file are subject to the Mozilla Public License Version
* 1.1 (the "License"); you may not use this file except in compliance with
* the License. You may obtain a copy of the License at
* http://www.mozilla.org/MPL/
*
* Software distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
* for the specific language governing rights and limitations under the
* License.
*
* The Original Code is mozilla.org code.
*
* The Initial Developer of the Original Code is
* Netscape Communications Corporation.
* Portions created by the Initial Developer are Copyright (C) 2002
* the Initial Developer. All Rights Reserved.
*
* Contributor(s):
*   Conrad Carlen <ccarlen@netscape.com>
*   Brendan Eich <brendan@mozilla.org>
*   Colin Blake <colin@theblakes.com>
*   Javier Pedemonte <pedemont@us.ibm.com>
*   Mats Palmgren <mats.palmgren@bredband.net>
*
* Alternatively, the contents of this file may be used under the terms of
* either the GNU General Public License Version 2 or later (the "GPL"), or
* the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
* in which case the provisions of the GPL or the LGPL are applicable instead
* of those above. If you wish to allow use of your version of this file only
* under the terms of either the GPL or the LGPL, and not to allow others to
* use your version of this file under the terms of the MPL, indicate your
* decision by deleting the provisions above and replace them with the notice
* and other provisions required by the GPL or the LGPL. If you do not delete
* the provisions above, a recipient may use your version of this file under
* the terms of any one of the MPL, the GPL or the LGPL.
*
* ***** END LICENSE BLOCK ***** */

void FirefoxProfileLock::Init() {
  lock_fd_ = -1;
}

void FirefoxProfileLock::Lock() {
  if (HasAcquired())
    return;
  lock_fd_ = open(lock_file_.value().c_str(), O_CREAT | O_EXCL, 0644);
}

void FirefoxProfileLock::Unlock() {
  if (!HasAcquired())
    return;
  close(lock_fd_);
  lock_fd_ = -1;
}

bool FirefoxProfileLock::HasAcquired() {
  return (lock_fd_ >= 0);
}


