// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This provides a way to access the application's current preferences.

#ifndef CHROME_BROWSER_PREFS_PREF_SERVICE_H_
#define CHROME_BROWSER_PREFS_PREF_SERVICE_H_
#pragma once

#include <set>
#include <string>

#include "base/non_thread_safe.h"
#include "base/ref_counted.h"
#include "base/scoped_ptr.h"
#include "base/values.h"
#include "chrome/common/pref_store.h"

class DefaultPrefStore;
class FilePath;
class NotificationObserver;
class PrefChangeObserver;
class PrefNotifier;
class PrefNotifierImpl;
class PrefValueStore;
class Profile;

namespace subtle {
class PrefMemberBase;
};

class PrefService : public NonThreadSafe {
 public:
  // A helper class to store all the information associated with a preference.
  class Preference {
   public:

    // The type of the preference is determined by the type with which it is
    // registered. This type needs to be a boolean, integer, real, string,
    // dictionary (a branch), or list.  You shouldn't need to construct this on
    // your own; use the PrefService::Register*Pref methods instead.
    Preference(const PrefService* service,
               const char* name);
    ~Preference() {}

    // Returns the name of the Preference (i.e., the key, e.g.,
    // browser.window_placement).
    const std::string name() const { return name_; }

    // Returns the registered type of the preference.
    Value::ValueType GetType() const;

    // Returns the value of the Preference, falling back to the registered
    // default value if no other has been set.
    const Value* GetValue() const;

    // Returns true if the Preference is managed, i.e. set by an admin policy.
    // Since managed prefs have the highest priority, this also indicates
    // whether the pref is actually being controlled by the policy setting.
    bool IsManaged() const;

    // Returns true if the Preference has a value set by an extension, even if
    // that value is being overridden by a higher-priority source.
    bool HasExtensionSetting() const;

    // Returns true if the Preference has a user setting, even if that value is
    // being overridden by a higher-priority source.
    bool HasUserSetting() const;

    // Returns true if the Preference value is currently being controlled by an
    // extension, and not by any higher-priority source.
    bool IsExtensionControlled() const;

    // Returns true if the Preference value is currently being controlled by a
    // user setting, and not by any higher-priority source.
    bool IsUserControlled() const;

    // Returns true if the Preference is currently using its default value,
    // and has not been set by any higher-priority source (even with the same
    // value).
    bool IsDefaultValue() const;

    // Returns true if the user can change the Preference value, which is the
    // case if no higher-priority source than the user store controls the
    // Preference.
    bool IsUserModifiable() const;

   private:
    friend class PrefService;

    std::string name_;

    // Reference to the PrefService in which this pref was created.
    const PrefService* pref_service_;

    DISALLOW_COPY_AND_ASSIGN(Preference);
  };

  // Factory method that creates a new instance of a PrefService with the
  // applicable PrefStores. The |pref_filename| points to the user preference
  // file. The |profile| is the one to which these preferences apply; it may be
  // NULL if we're dealing with the local state. This is the usual way to create
  // a new PrefService.
  static PrefService* CreatePrefService(const FilePath& pref_filename,
                                        Profile* profile);

  // Convenience factory method for use in unit tests. Creates a new
  // PrefService that uses a PrefValueStore with user preferences at the given
  // |pref_filename|, a default PrefStore, and no other PrefStores (i.e., no
  // other types of preferences).
  static PrefService* CreateUserPrefService(const FilePath& pref_filename);

  virtual ~PrefService();

  // Reloads the data from file. This should only be called when the importer
  // is running during first run, and the main process may not change pref
  // values while the importer process is running. Returns true on success.
  bool ReloadPersistentPrefs();

  // Returns true if the preference for the given preference name is available
  // and is managed.
  bool IsManagedPreference(const char* pref_name) const;

  // Writes the data to disk. The return value only reflects whether
  // serialization was successful; we don't know whether the data actually made
  // it on disk (since it's on a different thread).  This should only be used if
  // we need to save immediately (basically, during shutdown).  Otherwise, you
  // should use ScheduleSavePersistentPrefs.
  bool SavePersistentPrefs();

  // Serializes the data and schedules save using ImportantFileWriter.
  void ScheduleSavePersistentPrefs();

  // Make the PrefService aware of a pref.
  void RegisterBooleanPref(const char* path, bool default_value);
  void RegisterIntegerPref(const char* path, int default_value);
  void RegisterRealPref(const char* path, double default_value);
  void RegisterStringPref(const char* path, const std::string& default_value);
  void RegisterFilePathPref(const char* path, const FilePath& default_value);
  void RegisterListPref(const char* path);
  void RegisterDictionaryPref(const char* path);

  // These varients use a default value from the locale dll instead.
  void RegisterLocalizedBooleanPref(const char* path,
                                    int locale_default_message_id);
  void RegisterLocalizedIntegerPref(const char* path,
                                    int locale_default_message_id);
  void RegisterLocalizedRealPref(const char* path,
                                 int locale_default_message_id);
  void RegisterLocalizedStringPref(const char* path,
                                   int locale_default_message_id);

  // If the path is valid and the value at the end of the path matches the type
  // specified, it will return the specified value.  Otherwise, the default
  // value (set when the pref was registered) will be returned.
  bool GetBoolean(const char* path) const;
  int GetInteger(const char* path) const;
  double GetReal(const char* path) const;
  std::string GetString(const char* path) const;
  FilePath GetFilePath(const char* path) const;

  // Returns the branch if it exists.  If it's not a branch or the branch does
  // not exist, returns NULL.
  const DictionaryValue* GetDictionary(const char* path) const;
  const ListValue* GetList(const char* path) const;

  // Removes a user pref and restores the pref to its default value.
  void ClearPref(const char* path);

  // If the path is valid (i.e., registered), update the pref value in the user
  // prefs. Seting a null value on a preference of List or Dictionary type is
  // equivalent to removing the user value for that preference, allowing the
  // default value to take effect unless another value takes precedence.
  void Set(const char* path, const Value& value);
  void SetBoolean(const char* path, bool value);
  void SetInteger(const char* path, int value);
  void SetReal(const char* path, double value);
  void SetString(const char* path, const std::string& value);
  void SetFilePath(const char* path, const FilePath& value);

  // Int64 helper methods that actually store the given value as a string.
  // Note that if obtaining the named value via GetDictionary or GetList, the
  // Value type will be TYPE_STRING.
  void SetInt64(const char* path, int64 value);
  int64 GetInt64(const char* path) const;
  void RegisterInt64Pref(const char* path, int64 default_value);

  // Used to set the value of dictionary or list values in the pref tree.  This
  // will create a dictionary or list if one does not exist in the pref tree.
  // This method returns NULL only if you're requesting an unregistered pref or
  // a non-dict/non-list pref.
  // WARNING: Changes to the dictionary or list will not automatically notify
  // pref observers.
  // Use a ScopedPrefUpdate to update observers on changes.
  // These should really be GetUserMutable... since we will only ever get
  // a mutable from the user preferences store.
  DictionaryValue* GetMutableDictionary(const char* path);
  ListValue* GetMutableList(const char* path);

  // Returns true if a value has been set for the specified path.
  // NOTE: this is NOT the same as FindPreference. In particular
  // FindPreference returns whether RegisterXXX has been invoked, where as
  // this checks if a value exists for the path.
  bool HasPrefPath(const char* path) const;

  class PreferencePathComparator {
   public:
    bool operator() (Preference* lhs, Preference* rhs) const {
      return lhs->name() < rhs->name();
    }
  };
  typedef std::set<Preference*, PreferencePathComparator> PreferenceSet;
  const PreferenceSet& preference_set() const { return prefs_; }

  // A helper method to quickly look up a preference.  Returns NULL if the
  // preference is not registered.
  const Preference* FindPreference(const char* pref_name) const;

  bool ReadOnly() const;

  // TODO(mnissler): This should not be public. Change client code to call a
  // preference setter or use ScopedPrefUpdate.
  PrefNotifier* pref_notifier() const;

  // Get the extension PrefStore.
  PrefStore* GetExtensionPrefStore();

 protected:
  // Construct a new pref service, specifying the pref sources as explicit
  // PrefStore pointers. This constructor is what CreatePrefService() ends up
  // calling. It's also used for unit tests.
  PrefService(PrefStore* managed_platform_prefs,
              PrefStore* device_management_prefs,
              PrefStore* extension_prefs,
              PrefStore* command_line_prefs,
              PrefStore* user_prefs,
              PrefStore* recommended_prefs,
              Profile* profile);

  // The PrefNotifier handles registering and notifying preference observers.
  // It is created and owned by this PrefService. Subclasses may access it for
  // unit testing.
  scoped_ptr<PrefNotifierImpl> pref_notifier_;

 private:
  friend class TestingPrefService;

  // Registration of pref change observers must be done using the
  // PrefChangeRegistrar, which is declared as a friend here to grant it
  // access to the otherwise protected members Add/RemovePrefObserver.
  // PrefMember registers for preferences changes notification directly to
  // avoid the storage overhead of the registrar, so its base class must be
  // declared as a friend, too.
  friend class PrefChangeRegistrar;
  friend class subtle::PrefMemberBase;

  // If the pref at the given path changes, we call the observer's Observe
  // method with PREF_CHANGED. Note that observers should not call these methods
  // directly but rather use a PrefChangeRegistrar to make sure the observer
  // gets cleaned up properly.
  virtual void AddPrefObserver(const char* path, NotificationObserver* obs);
  virtual void RemovePrefObserver(const char* path, NotificationObserver* obs);

  // Add a preference to the PreferenceMap.  If the pref already exists, return
  // false.  This method takes ownership of |default_value|.
  void RegisterPreference(const char* path, Value* default_value);

  // Returns a copy of the current pref value.  The caller is responsible for
  // deleting the returned object.
  Value* GetPrefCopy(const char* pref_name);

  // Sets the value for this pref path in the user pref store and informs the
  // PrefNotifier of the change.
  void SetUserPrefValue(const char* path, Value* new_value);

  // Load from disk.  Returns a non-zero error code on failure.
  PrefStore::PrefReadError LoadPersistentPrefs();

  // Load preferences from storage, attempting to diagnose and handle errors.
  // This should only be called from the constructor.
  void InitFromStorage();

  // The PrefValueStore provides prioritized preference values. It is created
  // and owned by this PrefService. Subclasses may access it for unit testing.
  scoped_refptr<PrefValueStore> pref_value_store_;

  // The extension pref store registered with the PrefValueStore.
  PrefStore* extension_store_;

  // Points to the default pref store we passed to the PrefValueStore.
  PrefStore* default_store_;

  // A set of all the registered Preference objects.
  PreferenceSet prefs_;

  DISALLOW_COPY_AND_ASSIGN(PrefService);
};

#endif  // CHROME_BROWSER_PREFS_PREF_SERVICE_H_
