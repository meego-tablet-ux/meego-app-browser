// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_CROS_MOUNT_LIBRARY_H_
#define CHROME_BROWSER_CHROMEOS_CROS_MOUNT_LIBRARY_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/singleton.h"
#include "base/time.h"
#include "cros/chromeos_mount.h"

namespace chromeos {

// This class handles the interaction with the ChromeOS mount library APIs.
// Classes can add themselves as observers. Users can get an instance of this
// library class like this: MountLibrary::Get().
class MountLibrary {
 public:
  // Used to house an instance of each found mount device.
  struct Disk {
    Disk() {}
    Disk(const std::string& devicepath,
         const std::string& mountpath,
         const std::string& systempath,
         bool isparent,
         bool hasmedia)
        : device_path(devicepath),
          mount_path(mountpath),
          system_path(systempath),
          is_parent(isparent),
          has_media(hasmedia) {}
    // The path of the device, used by devicekit-disks.
    std::string device_path;
    // The path to the mount point of this device. Will be empty if not mounted.
    std::string mount_path;
    // The path of the device according to the udev system.
    std::string system_path;
    // if the device is a parent device (i.e. sdb rather than sdb1)
    bool is_parent;
    // if the device has media currently
    bool has_media;
  };
  typedef std::vector<Disk> DiskVector;

  class Observer {
   public:
    virtual void MountChanged(MountLibrary* obj,
                              MountEventType evt,
                              const std::string& path) = 0;
  };

  virtual ~MountLibrary() {}
  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;
  virtual const DiskVector& disks() const = 0;
  virtual bool MountPath(const char* device_path) = 0;
};

// This class handles the interaction with the ChromeOS mount library APIs.
// Classes can add themselves as observers. Users can get an instance of this
// library class like this: MountLibrary::Get().
class MountLibraryImpl : public MountLibrary {
 public:
  MountLibraryImpl();
  virtual ~MountLibraryImpl();

  // MountLibrary overrides.
  virtual void AddObserver(Observer* observer);
  virtual void RemoveObserver(Observer* observer);
  virtual const DiskVector& disks() const { return disks_; }
  virtual bool MountPath(const char* device_path);
 private:
  void ParseDisks(const MountStatus& status);

  // This method is called when there's a change in mount status.
  // This method is called the UI Thread.
  static void MountStatusChangedHandler(void* object,
                                        const MountStatus& status,
                                        MountEventType evt,
                                        const char* path);

  // This methods starts the monitoring of mount changes.
  // It should be called on the UI Thread.
  void Init();

  // Called by the handler to update the mount status.
  // This will notify all the Observers.
  void UpdateMountStatus(const MountStatus& status,
                         MountEventType evt,
                         const std::string& path);

  ObserverList<Observer> observers_;

  // A reference to the  mount api, to allow callbacks when the mount
  // status changes.
  MountStatusConnection mount_status_connection_;

  // The list of disks found.
  DiskVector disks_;

  DISALLOW_COPY_AND_ASSIGN(MountLibraryImpl);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_CROS_MOUNT_LIBRARY_H_
