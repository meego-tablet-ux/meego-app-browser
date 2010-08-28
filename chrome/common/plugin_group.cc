// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/plugin_group.h"

#include "base/linked_ptr.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "base/values.h"
#include "base/version.h"
#include "webkit/glue/plugins/plugin_list.h"
#include "webkit/glue/plugins/webplugininfo.h"

#if defined(OS_MACOSX)
// Plugin Groups for Mac.
// Plugins are listed here as soon as vulnerabilities and solutions
// (new versions) are published.
// TODO(panayiotis): Get the Real Player version on Mac, somehow.
static const PluginGroupDefinition kGroupDefinitions[] = {
  { "Quicktime", "QuickTime Plug-in", "", "", "7.6.6",
    "http://www.apple.com/quicktime/download/" },
  { "Java", "Java", "", "", "",
    "http://support.apple.com/kb/HT1338" },
  { "Flash", "Shockwave Flash", "", "", "10.1.82",
    "http://get.adobe.com/flashplayer/" },
  { "Silverlight 3", "Silverlight", "0", "4", "3.0.50106.0",
    "http://go.microsoft.com/fwlink/?LinkID=185927" },
  { "Silverlight 4", "Silverlight", "4", "5", "",
    "http://go.microsoft.com/fwlink/?LinkID=185927" },
  { "Flip4Mac", "Flip4Mac", "",  "", "2.2.1",
    "http://www.telestream.net/flip4mac-wmv/overview.htm" },
  { "Shockwave", "Shockwave for Director", "",  "", "11.5.7.609",
    "http://www.adobe.com/shockwave/download/" }
};

#elif defined(OS_WIN)
// TODO(panayiotis): We should group "RealJukebox NS Plugin" with the rest of
// the RealPlayer files.
static const PluginGroupDefinition kGroupDefinitions[] = {
  { "Quicktime", "QuickTime Plug-in", "", "", "7.6.7",
    "http://www.apple.com/quicktime/download/" },
  { "Java 6", "Java", "", "6", "6.0.200",
    "http://www.java.com/" },
  { "Adobe Reader 9", "Adobe Acrobat", "9", "10", "9.3.3",
    "http://get.adobe.com/reader/" },
  { "Adobe Reader 8", "Adobe Acrobat", "0", "9",  "8.2.3",
    "http://get.adobe.com/reader/" },
  { "Flash", "Shockwave Flash", "", "", "10.1.82",
    "http://get.adobe.com/flashplayer/" },
  { "Silverlight 3", "Silverlight", "0", "4", "3.0.50106.0",
    "http://go.microsoft.com/fwlink/?LinkID=185927" },
  { "Silverlight 4", "Silverlight", "4", "5", "",
    "http://go.microsoft.com/fwlink/?LinkID=185927" },
  { "Shockwave", "Shockwave for Director", "", "", "11.5.7.609",
    "http://www.adobe.com/shockwave/download/" },
  { "DivX Player", "DivX Web Player", "",  "", "1.4.3.4",
    "http://download.divx.com/divx/autoupdate/player/DivXWebPlayerInstaller.exe" },
  // These are here for grouping, no vulnerabilies known.
  { "Windows Media Player",  "Windows Media Player",  "",  "",   "", "" },
  { "Microsoft Office",  "Microsoft Office",  "",  "",   "", "" },
  // TODO(panayiotis): The vulnerable versions are
  //  (v >=  6.0.12.1040 && v <= 6.0.12.1663)
  //  || v == 6.0.12.1698  || v == 6.0.12.1741
  { "RealPlayer", "RealPlayer", "",  "",  "",
    "http://www.adobe.com/shockwave/download/" },
};

#else
static const PluginGroupDefinition kGroupDefinitions[] = {};
#endif

/*static*/
std::set<string16>* PluginGroup::policy_disabled_plugins_;

/*static*/
const PluginGroupDefinition* PluginGroup::GetPluginGroupDefinitions() {
  return kGroupDefinitions;
}

/*static*/
size_t PluginGroup::GetPluginGroupDefinitionsSize() {
  // TODO(viettrungluu): |arraysize()| doesn't work with zero-size arrays.
  return ARRAYSIZE_UNSAFE(kGroupDefinitions);
}

/*static*/
void PluginGroup::SetPolicyDisabledPluginSet(const std::set<string16>& set) {
  if (!policy_disabled_plugins_)
    policy_disabled_plugins_ = new std::set<string16>(set);
}

/*static*/
bool PluginGroup::IsPluginNameDisabledByPolicy(const string16& plugin_name) {
  return policy_disabled_plugins_ &&
      policy_disabled_plugins_->find(plugin_name) !=
          policy_disabled_plugins_->end();
}

/*static*/
bool PluginGroup::IsPluginPathDisabledByPolicy(const FilePath& plugin_path) {
  std::vector<WebPluginInfo> plugins;
  NPAPI::PluginList::Singleton()->GetPlugins(false, &plugins);
  for (std::vector<WebPluginInfo>::const_iterator it = plugins.begin();
       it != plugins.end();
       ++it) {
    if (FilePath::CompareEqualIgnoreCase(it->path.value(),
        plugin_path.value()) && IsPluginNameDisabledByPolicy(it->name)) {
      return true;
    }
  }
  return false;
}

PluginGroup::PluginGroup(const string16& group_name,
                         const string16& name_matcher,
                         const std::string& version_range_low,
                         const std::string& version_range_high,
                         const std::string& min_version,
                         const std::string& update_url) {
  group_name_ = group_name;
  name_matcher_ = name_matcher;
  version_range_low_str_ = version_range_low;
  if (!version_range_low.empty()) {
    version_range_low_.reset(
        Version::GetVersionFromString(version_range_low));
  }
  version_range_high_str_ = version_range_high;
  if (!version_range_high.empty()) {
    version_range_high_.reset(
        Version::GetVersionFromString(version_range_high));
  }
  min_version_str_ = min_version;
  if (!min_version.empty()) {
    min_version_.reset(Version::GetVersionFromString(min_version));
  }
  update_url_ = update_url;
  enabled_ = false;
  version_.reset(Version::GetVersionFromString("0"));
}

PluginGroup* PluginGroup::FromPluginGroupDefinition(
    const PluginGroupDefinition& definition) {
  return new PluginGroup(ASCIIToUTF16(definition.name),
                         ASCIIToUTF16(definition.name_matcher),
                         definition.version_matcher_low,
                         definition.version_matcher_high,
                         definition.min_version,
                         definition.update_url);
}

PluginGroup::~PluginGroup() { }

PluginGroup* PluginGroup::FromWebPluginInfo(const WebPluginInfo& wpi) {
  // Create a matcher from the name of this plugin.
  return new PluginGroup(wpi.name, wpi.name,
                         "", "", "", "");
}

PluginGroup* PluginGroup::FindHardcodedPluginGroup(const WebPluginInfo& info) {
  static std::vector<linked_ptr<PluginGroup> >* hardcoded_plugin_groups = NULL;
  if (!hardcoded_plugin_groups) {
    std::vector<linked_ptr<PluginGroup> >* groups =
        new std::vector<linked_ptr<PluginGroup> >();
    const PluginGroupDefinition* definitions = GetPluginGroupDefinitions();
    for (size_t i = 0; i < GetPluginGroupDefinitionsSize(); ++i) {
      PluginGroup* definition_group = PluginGroup::FromPluginGroupDefinition(
          definitions[i]);
      groups->push_back(linked_ptr<PluginGroup>(definition_group));
    }
    hardcoded_plugin_groups = groups;
  }

  // See if this plugin matches any of the hardcoded groups.
  PluginGroup* hardcoded_group = FindGroupMatchingPlugin(
      *hardcoded_plugin_groups, info);
  if (hardcoded_group) {
    // Make a copy.
    return hardcoded_group->Copy();
  } else {
    // Not found in our hardcoded list, create a new one.
    return PluginGroup::FromWebPluginInfo(info);
  }
}

PluginGroup* PluginGroup::FindGroupMatchingPlugin(
    std::vector<linked_ptr<PluginGroup> >& plugin_groups,
    const WebPluginInfo& plugin) {
  for (std::vector<linked_ptr<PluginGroup> >::iterator it =
       plugin_groups.begin();
       it != plugin_groups.end();
       ++it) {
    if ((*it)->Match(plugin)) {
      return it->get();
    }
  }
  return NULL;
}

bool PluginGroup::Match(const WebPluginInfo& plugin) const {
  if (name_matcher_.empty()) {
    return false;
  }

  // Look for the name matcher anywhere in the plugin name.
  if (plugin.name.find(name_matcher_) == string16::npos) {
    return false;
  }

  if (version_range_low_.get() == NULL ||
      version_range_high_.get() == NULL) {
    return true;
  }

  // There's a version range, we must be in it.
  scoped_ptr<Version> plugin_version(
      Version::GetVersionFromString(UTF16ToWide(plugin.version)));
  if (plugin_version.get() == NULL) {
    // No version could be extracted, assume we don't match the range.
    return false;
  }

  // We match if we are in the range: [low, high)
  return (version_range_low_->CompareTo(*plugin_version) <= 0 &&
          version_range_high_->CompareTo(*plugin_version) > 0);
}

Version* PluginGroup::CreateVersionFromString(const string16& version_string) {
  // Remove spaces and ')' from the version string,
  // Replace any instances of 'r', ',' or '(' with a dot.
  std::wstring version = UTF16ToWide(version_string);
  RemoveChars(version, L") ", &version);
  std::replace(version.begin(), version.end(), 'r', '.');
  std::replace(version.begin(), version.end(), ',', '.');
  std::replace(version.begin(), version.end(), '(', '.');

  return Version::GetVersionFromString(version);
}

void PluginGroup::UpdateActivePlugin(const WebPluginInfo& plugin) {
  // A group is enabled if any of the files are enabled.
  if (plugin.enabled) {
    if (!enabled_) {
      // If this is the first enabled plugin, use its description.
      enabled_ = true;
      UpdateDescriptionAndVersion(plugin);
    }
  } else {
    // If this is the first plugin and it's disabled,
    // use its description for now.
    if (description_.empty())
      UpdateDescriptionAndVersion(plugin);
  }
}

void PluginGroup::UpdateDescriptionAndVersion(const WebPluginInfo& plugin) {
  description_ = plugin.desc;
  if (Version* new_version = CreateVersionFromString(plugin.version))
    version_.reset(new_version);
  else
    version_.reset(Version::GetVersionFromString("0"));
}

void PluginGroup::AddPlugin(const WebPluginInfo& plugin, int position) {
  web_plugin_infos_.push_back(plugin);
  // The position of this plugin relative to the global list of plugins.
  web_plugin_positions_.push_back(position);
  UpdateActivePlugin(plugin);
}

DictionaryValue* PluginGroup::GetSummary() const {
  DictionaryValue* result = new DictionaryValue();
  result->SetString("name", group_name_);
  result->SetBoolean("enabled", enabled_);
  return result;
}

DictionaryValue* PluginGroup::GetDataForUI() const {
  DictionaryValue* result = new DictionaryValue();
  result->SetString("name", group_name_);
  result->SetString("description", description_);
  result->SetString("version", version_->GetString());
  result->SetString("update_url", update_url_);
  result->SetBoolean("critical", IsVulnerable());

  bool group_disabled_by_policy = IsPluginNameDisabledByPolicy(group_name_);
  ListValue* plugin_files = new ListValue();
  bool all_plugins_disabled_by_policy = true;
  for (size_t i = 0; i < web_plugin_infos_.size(); ++i) {
    const WebPluginInfo& web_plugin = web_plugin_infos_[i];
    int priority = web_plugin_positions_[i];
    DictionaryValue* plugin_file = new DictionaryValue();
    plugin_file->SetString("name", web_plugin.name);
    plugin_file->SetString("description", web_plugin.desc);
    plugin_file->SetString("path", web_plugin.path.value());
    plugin_file->SetString("version", web_plugin.version);
    bool plugin_disabled_by_policy = group_disabled_by_policy ||
        IsPluginNameDisabledByPolicy(web_plugin.name);
    if (plugin_disabled_by_policy) {
      plugin_file->SetString("enabledMode", "disabledByPolicy");
    } else {
      all_plugins_disabled_by_policy = false;
      plugin_file->SetString("enabledMode",
                             web_plugin.enabled ? "enabled" : "disabledByUser");
    }
    plugin_file->SetInteger("priority", priority);

    ListValue* mime_types = new ListValue();
    for (std::vector<WebPluginMimeType>::const_iterator type_it =
         web_plugin.mime_types.begin();
         type_it != web_plugin.mime_types.end();
         ++type_it) {
      DictionaryValue* mime_type = new DictionaryValue();
      mime_type->SetString("mimeType", type_it->mime_type);
      mime_type->SetString("description", type_it->description);

      ListValue* file_extensions = new ListValue();
      for (std::vector<std::string>::const_iterator ext_it =
           type_it->file_extensions.begin();
           ext_it != type_it->file_extensions.end();
           ++ext_it) {
        file_extensions->Append(new StringValue(*ext_it));
      }
      mime_type->Set("fileExtensions", file_extensions);

      mime_types->Append(mime_type);
    }
    plugin_file->Set("mimeTypes", mime_types);

    plugin_files->Append(plugin_file);
  }

  if (group_disabled_by_policy || all_plugins_disabled_by_policy) {
    result->SetString("enabledMode", "disabledByPolicy");
  } else {
    result->SetString("enabledMode", enabled_ ? "enabled" : "disabledByUser");
  }
  result->Set("plugin_files", plugin_files);

  return result;
}

// Returns true if the latest version of this plugin group is vulnerable.
bool PluginGroup::IsVulnerable() const {
  if (min_version_.get() == NULL || version_->GetString() == "0") {
    return false;
  }
  return version_->CompareTo(*min_version_) < 0;
}

void PluginGroup::DisableOutdatedPlugins() {
  if (!min_version_.get())
    return;

  description_ = string16();
  enabled_ = false;

  for (std::vector<WebPluginInfo>::iterator it =
           web_plugin_infos_.begin();
       it != web_plugin_infos_.end(); ++it) {
    scoped_ptr<Version> version(CreateVersionFromString(it->version));
    if (version.get() && version->CompareTo(*min_version_) < 0) {
      it->enabled = false;
      NPAPI::PluginList::Singleton()->DisablePlugin(it->path);
    }
    UpdateActivePlugin(*it);
  }
}

void PluginGroup::Enable(bool enable) {
  for (std::vector<WebPluginInfo>::const_iterator it =
       web_plugin_infos_.begin();
       it != web_plugin_infos_.end(); ++it) {
    if (enable && !IsPluginNameDisabledByPolicy(it->name)) {
      NPAPI::PluginList::Singleton()->EnablePlugin(it->path);
    } else {
      NPAPI::PluginList::Singleton()->DisablePlugin(it->path);
    }
  }
}
