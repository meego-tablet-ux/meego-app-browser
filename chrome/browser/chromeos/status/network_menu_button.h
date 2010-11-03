// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_STATUS_NETWORK_MENU_BUTTON_H_
#define CHROME_BROWSER_CHROMEOS_STATUS_NETWORK_MENU_BUTTON_H_
#pragma once

#include "app/throb_animation.h"
#include "base/timer.h"
#include "chrome/browser/chromeos/cros/network_library.h"
#include "chrome/browser/chromeos/status/network_menu.h"
#include "chrome/browser/chromeos/status/status_area_button.h"

namespace gfx {
class Canvas;
}

namespace chromeos {

class StatusAreaHost;

// The network menu button in the status area.
// This class will handle getting the wifi networks and populating the menu.
// It will also handle the status icon changing and connecting to another
// wifi/cellular network.
//
// The network menu looks like this:
//
// <icon>  Ethernet
// <icon>  Wifi Network A
// <icon>  Wifi Network B
// <icon>  Wifi Network C
// <icon>  Cellular Network A
// <icon>  Cellular Network B
// <icon>  Cellular Network C
// <icon>  Other...
// --------------------------------
//         Disable Wifi
//         Disable Celluar
// --------------------------------
//         <IP Address>
//         Network settings...
//
// <icon> will show the strength of the wifi/cellular networks.
// The label will be BOLD if the network is currently connected.
class NetworkMenuButton : public StatusAreaButton,
                          public NetworkMenu,
                          public NetworkLibrary::Observer {
 public:
  explicit NetworkMenuButton(StatusAreaHost* host);
  virtual ~NetworkMenuButton();

  // AnimationDelegate implementation.
  virtual void AnimationProgressed(const Animation* animation);

  // NetworkLibrary::Observer implementation.
  virtual void NetworkChanged(NetworkLibrary* obj);
  virtual void CellularDataPlanChanged(NetworkLibrary* obj);

  // Sets the badge icon.
  void SetBadge(const SkBitmap& badge) { badge_ = badge; }
  SkBitmap badge() const { return badge_; }

 protected:
  // StatusAreaButton implementation.
  virtual void DrawIcon(gfx::Canvas* canvas);

  // NetworkMenu implementation:
  virtual bool IsBrowserMode() const;
  virtual gfx::NativeWindow GetNativeWindow() const;
  virtual void OpenButtonOptions() const;
  virtual bool ShouldOpenButtonOptions() const;

 private:
  // The status area host,
  StatusAreaHost* host_;

  // A badge icon displayed on top of the icon.
  SkBitmap badge_;

  // The throb animation that does the wifi connecting animation.
  ThrobAnimation animation_connecting_;

  // The duration of the icon throbbing in milliseconds.
  static const int kThrobDuration;

  DISALLOW_COPY_AND_ASSIGN(NetworkMenuButton);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_STATUS_NETWORK_MENU_BUTTON_H_
