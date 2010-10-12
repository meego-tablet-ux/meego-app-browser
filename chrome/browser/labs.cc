// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/labs.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <set>

#include "app/l10n_util.h"
#include "base/command_line.h"
#include "base/values.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "grit/generated_resources.h"

namespace about_labs {

enum { kOsMac = 1 << 0, kOsWin = 1 << 1, kOsLinux = 1 << 2 };

unsigned kOsAll = kOsMac | kOsWin | kOsLinux;

struct Experiment {
  // The internal name of the experiment. This is never shown to the user.
  // It _is_ however stored in the prefs file, so you shouldn't change the
  // name of existing labs.
  const char* internal_name;

  // String id of the message containing the experiment's name.
  int visible_name_id;

  // String id of the message containing the experiment's description.
  int visible_description_id;

  // The platforms the experiment is available on
  // Needs to be more than a compile-time #ifdef because of profile sync.
  unsigned supported_platforms;  // bitmask

  // The commandline parameter that's added when this lab is active. This is
  // different from |internal_name| so that the commandline flag can be
  // renamed without breaking the prefs file.
  const char* command_line;
};

const Experiment kExperiments[] = {
  {
    "expose-for-tabs",  // Do not change; see above.
    IDS_LABS_TABPOSE_NAME,
    IDS_LABS_TABPOSE_DESCRIPTION,
    kOsMac,
#if defined(OS_MACOSX)
    // The switch exists only on OS X.
    switches::kEnableExposeForTabs
#else
    ""
#endif
  },
  {
    "vertical-tabs",  // Do not change; see above.
    IDS_LABS_SIDE_TABS_NAME,
    IDS_LABS_SIDE_TABS_DESCRIPTION,
    kOsWin,
    switches::kEnableVerticalTabs
  },
  {
    "tabbed-options",  // Do not change; see above.
    IDS_LABS_TABBED_OPTIONS_NAME,
    IDS_LABS_TABBED_OPTIONS_DESCRIPTION,
    kOsAll,
    switches::kEnableTabbedOptions
  },
  {
    "remoting",  // Do not change; see above.
    IDS_LABS_REMOTING_NAME,
#if defined(OS_WIN)
    // Windows only supports host functionality at the moment.
    IDS_LABS_REMOTING_HOST_DESCRIPTION,
#elif defined(OS_LINUX)
    // Linux only supports client functionality at the moment.
    IDS_LABS_REMOTING_CLIENT_DESCRIPTION,
#else
    // On other platforms, this lab isn't available at all.
    0,
#endif
    kOsWin | kOsLinux,
    switches::kEnableRemoting
  },
  {
    "disable-outdated-plugins",  // Do not change; see above.
    IDS_LABS_DISABLE_OUTDATED_PLUGINS_NAME,
    IDS_LABS_DISABLE_OUTDATED_PLUGINS_DESCRIPTION,
    kOsAll,
    switches::kDisableOutdatedPlugins
  },
  {
    "xss-auditor",  // Do not change; see above.
    IDS_LABS_XSS_AUDITOR_NAME,
    IDS_LABS_XSS_AUDITOR_DESCRIPTION,
    kOsAll,
    switches::kEnableXSSAuditor
  },
  {
    "background-webapps", // Do not change; see above
    IDS_LABS_BACKGROUND_WEBAPPS_NAME,
    IDS_LABS_BACKGROUND_WEBAPPS_DESCRIPTION,
    kOsAll,
    switches::kEnableBackgroundMode
  },
  {
    "cloud-print-proxy", // Do not change; see above.
    IDS_LABS_CLOUD_PRINT_PROXY_NAME,
    IDS_LABS_CLOUD_PRINT_PROXY_DESCRIPTION,
#if defined(GOOGLE_CHROME_BUILD)
    // For a Chrome build, we know we have a PDF plug-in, and so we'll
    // enable by platform as we get things working.
    0,
#else
    // Otherwise, where we know it could be working if a viable PDF
    // plug-in could be supplied, we'll keep the lab enabled.
    kOsWin,
#endif
    switches::kEnableCloudPrintProxy
  },
  // FIXME(scheib): Add Labs entry for accelerated Compositing, 
  // or pull it and the strings in generated_resources.grd by Dec 2010
  //{
  //  "gpu-compositing", // Do not change; see above
  //  IDS_LABS_ACCELERATED_COMPOSITING_NAME,
  //  IDS_LABS_ACCELERATED_COMPOSITING_DESCRIPTION,
  //  kOsAll,
  //  switches::kDisableAcceleratedCompositing
  //},
  {
    "gpu-canvas-2d", // Do not change; see above
    IDS_LABS_ACCELERATED_CANVAS_2D_NAME,
    IDS_LABS_ACCELERATED_CANVAS_2D_DESCRIPTION,
    kOsWin | kOsLinux,
    switches::kEnableAccelerated2dCanvas
  }
  // FIXME(scheib): Add Labs entry for WebGL,
  // or pull it and the strings in generated_resources.grd by Dec 2010
  //{
  //  "webgl", // Do not change; see above
  //  IDS_LABS_WEBGL_NAME,
  //  IDS_LABS_WEBGL_DESCRIPTION,
  //  kOsAll,
  //  switches::kDisableExperimentalWebGL
  //}
};

// Extracts the list of enabled lab experiments from preferences and stores them
// in a set.
void GetEnabledLabs(const PrefService* prefs, std::set<std::string>* result) {
  const ListValue* enabled_experiments = prefs->GetList(
      prefs::kEnabledLabsExperiments);
  if (!enabled_experiments)
    return;

  for (ListValue::const_iterator it = enabled_experiments->begin();
       it != enabled_experiments->end();
       ++it) {
    std::string experiment_name;
    if (!(*it)->GetAsString(&experiment_name)) {
      LOG(WARNING) << "Invalid entry in " << prefs::kEnabledLabsExperiments;
      continue;
    }
    result->insert(experiment_name);
  }
}

// Takes a set of enabled lab experiments
void SetEnabledLabs(
    PrefService* prefs, const std::set<std::string>& enabled_experiments) {
  ListValue* experiments_list = prefs->GetMutableList(
      prefs::kEnabledLabsExperiments);
  if (!experiments_list)
    return;

  experiments_list->Clear();
  for (std::set<std::string>::const_iterator it = enabled_experiments.begin();
       it != enabled_experiments.end();
       ++it) {
    experiments_list->Append(new StringValue(*it));
  }
}

// Removes all experiments from prefs::kEnabledLabsExperiments that are
// unknown, to prevent this list to become very long as experiments are added
// and removed.
void SanitizeList(PrefService* prefs) {
  std::set<std::string> known_experiments;
  for (size_t i = 0; i < arraysize(kExperiments); ++i)
    known_experiments.insert(kExperiments[i].internal_name);

  std::set<std::string> enabled_experiments;
  GetEnabledLabs(prefs, &enabled_experiments);

  std::set<std::string> new_enabled_experiments;
  std::set_intersection(
      known_experiments.begin(), known_experiments.end(),
      enabled_experiments.begin(), enabled_experiments.end(),
      std::inserter(new_enabled_experiments, new_enabled_experiments.begin()));

  SetEnabledLabs(prefs, new_enabled_experiments);
}

void GetSanitizedEnabledLabs(
    PrefService* prefs, std::set<std::string>* result) {
  SanitizeList(prefs);
  GetEnabledLabs(prefs, result);
}

int GetCurrentPlatform() {
#if defined(OS_MACOSX)
  return kOsMac;
#elif defined(OS_WIN)
  return kOsWin;
#elif defined(OS_LINUX)
  return kOsLinux;
#else
#error Unknown platform
#endif
}

bool IsEnabled() {
#if defined(OS_CHROMEOS)
  // ChromeOS uses a different mechanism for about:labs; integrated with their
  // dom ui options.
  // TODO(thakis): Port about:labs to chromeos -- http://crbug.com/57634
  return false;
#else
  return true;
#endif
}

void ConvertLabsToSwitches(PrefService* prefs, CommandLine* command_line) {
  if (!IsEnabled())
    return;

  std::set<std::string> enabled_experiments;
  GetSanitizedEnabledLabs(prefs, &enabled_experiments);

  std::map<std::string, const Experiment*> experiments;
  for (size_t i = 0; i < arraysize(kExperiments); ++i)
    experiments[kExperiments[i].internal_name] = &kExperiments[i];

  for (std::set<std::string>::iterator it = enabled_experiments.begin();
       it != enabled_experiments.end();
       ++it) {
    const std::string& experiment_name = *it;
    std::map<std::string, const Experiment*>::iterator experiment =
        experiments.find(experiment_name);
    DCHECK(experiment != experiments.end());
    if (experiment == experiments.end())
      continue;

    command_line->AppendSwitch(experiment->second->command_line);
  }
}

ListValue* GetLabsExperimentsData(PrefService* prefs) {
  std::set<std::string> enabled_experiments;
  GetSanitizedEnabledLabs(prefs, &enabled_experiments);

  int current_platform = GetCurrentPlatform();

  ListValue* experiments_data = new ListValue();
  for (size_t i = 0; i < arraysize(kExperiments); ++i) {
    const Experiment& experiment = kExperiments[i];
    if (!(experiment.supported_platforms & current_platform))
      continue;

    DictionaryValue* data = new DictionaryValue();
    data->SetString("internal_name", experiment.internal_name);
    data->SetString("name",
                    l10n_util::GetStringUTF16(experiment.visible_name_id));
    data->SetString("description",
                    l10n_util::GetStringUTF16(
                        experiment.visible_description_id));
    data->SetBoolean("enabled",
                      enabled_experiments.count(experiment.internal_name) > 0);

    experiments_data->Append(data);
  }
  return experiments_data;
}

static bool needs_restart_ = false;

bool IsRestartNeededToCommitChanges() {
  return needs_restart_;
}

void SetExperimentEnabled(
    PrefService* prefs, const std::string& internal_name, bool enable) {
  needs_restart_ = true;

  std::set<std::string> enabled_experiments;
  GetSanitizedEnabledLabs(prefs, &enabled_experiments);

  if (enable)
    enabled_experiments.insert(internal_name);
  else
    enabled_experiments.erase(internal_name);

  SetEnabledLabs(prefs, enabled_experiments);
}

}  // namespace Labs
