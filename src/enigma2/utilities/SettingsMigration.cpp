/*
 *  Copyright (C) 2005-2022 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "SettingsMigration.h"

#include "kodi/General.h"

#include <algorithm>
#include <utility>
#include <vector>

using namespace enigma2;
using namespace enigma2::utilities;

namespace
{
// <setting name, default value> maps
const std::vector<std::pair<const char*, const char*>> stringMap = {{"host", "127.0.0.1"},
                                                                    {"user", ""},
                                                                    {"pass", ""},
                                                                    {"iconpath", ""},
                                                                    {"defaultprovidername", ""},
                                                                    {"providermapfile", "special://userdata/addon_data/pvr.vuplus/providers/providerMappings.xml"},
                                                                    {"onetvgroup", ""},
                                                                    {"twotvgroup", ""},
                                                                    {"threetvgroup", ""},
                                                                    {"fourtvgroup", ""},
                                                                    {"fivetvgroup", ""},
                                                                    {"customtvgroupsfile", "special://userdata/addon_data/pvr.vuplus/channelGroups/customTVGroups-example.xml"},
                                                                    {"oneradiogroup", ""},
                                                                    {"tworadiogroup", ""},
                                                                    {"threeradiogroup", ""},
                                                                    {"fourradiogroup", ""},
                                                                    {"fiveradiogroup", ""},
                                                                    {"customradiogroupsfile", "special://userdata/addon_data/pvr.vuplus/channelGroups/customRadioGroups-example.xml"},
                                                                    {"extractshowinfofile", "special://userdata/addon_data/pvr.vuplus/showInfo/English-ShowInfo.xml"},
                                                                    {"genreidmapfile", "special://userdata/addon_data/pvr.vuplus/genres/genreIdMappings/Sky-UK.xml"},
                                                                    {"rytecgenretextmapfile", "special://userdata/addon_data/pvr.vuplus/genres/genreRytecTextMappings/Rytec-UK-Ireland.xml"},
                                                                    {"recordingpath", ""},
                                                                    {"timeshiftbufferpath", "special://userdata/addon_data/pvr.vuplus"},
                                                                    {"wakeonlanmac", ""}};

const std::vector<std::pair<const char*, int>> intMap = {{"webport", 80},
                                                         {"streamport", 8001},
                                                         {"connectionchecktimeout", 10},
                                                         {"connectioncheckinterval", 1},
                                                         {"updateint", 2},
                                                         {"updatemode", 0},
                                                         {"channelandgroupupdatemode", 2},
                                                         {"channelandgroupupdatehour", 4},
                                                         {"tvgroupmode", 0},
                                                         {"numtvgroups", 1},
                                                         {"tvfavouritesmode", 0},
                                                         {"radiogroupmode", 0},
                                                         {"numradiogroups", 1},
                                                         {"radiofavouritesmode", 0},
                                                         {"epgdelayperchannel", 0},
                                                         {"sharerecordinglastplayed", 0},
                                                         {"edlpaddingstart", 0},
                                                         {"edlpaddingstop", 0},
                                                         {"numgenrepeattimers", 1},
                                                         {"enabletimeshift", 0},
                                                         {"powerstatemode", 0},
                                                         {"globalstartpaddingstb", 0},
                                                         {"globalendpaddingstb", 0},
                                                         {"prependoutline", 0},
                                                         {"readtimeout", 0},
                                                         {"streamreadchunksize", 0}};

const std::vector<std::pair<const char*, float>> floatMap = {{"timeshiftdisklimit", 4.0}};

const std::vector<std::pair<const char*, bool>> boolMap = {{"use_secure", false},
                                                           {"autoconfig", false},
                                                           {"use_secure_stream", false},
                                                           {"use_login_stream", false},
                                                           {"setprogramid", false},
                                                           {"onlinepicons", true},
                                                           {"useopenwebifpiconpath", false},
                                                           {"usepiconseuformat", false},
                                                           {"zap", false},
                                                           {"usegroupspecificnumbers", false},
                                                           {"usestandardserviceref", true},
                                                           {"retrieveprovidername", true},
                                                           {"excludelastscannedtv", true},
                                                           {"excludelastscannedradio", true},
                                                           {"extractshowinfoenabled", true},
                                                           {"genreidmapenabled", true},
                                                           {"rytecgenretextmapenabled", false},
                                                           {"logmissinggenremapping", true},
                                                           {"storeextrarecordinginfo", true},
                                                           {"virtualfolders", true},
                                                           {"keepfolders", true},
                                                           {"keepfoldersomitlocation", true},
                                                           {"recordingsrecursive", true},
                                                           {"onlycurrent", false},
                                                           {"enablerecordingedls", false},
                                                           {"enablegenrepeattimers", true},
                                                           {"timerlistcleanup", false},
                                                           {"enableautotimers", true},
                                                           {"limitanychannelautotimers", true},
                                                           {"limitanychannelautotimerstogroups", true},
                                                           {"enabletimeshiftdisklimit", false},
                                                           {"timeshiftEnabledIptv", true},
                                                           {"useFFmpegReconnect", true},
                                                           {"useMpegtsForUnknownStreams", true}};

} // unnamed namespace

bool SettingsMigration::MigrateSettings(kodi::addon::IAddonInstance& target)
{
  std::string stringValue;
  bool boolValue{false};
  int intValue{0};

  if (target.CheckInstanceSettingString("kodi_addon_instance_name", stringValue) &&
      !stringValue.empty())
  {
    // Instance already has valid instance settings
    return false;
  }

  // Read pre-multi-instance settings from settings.xml, transfer to instance settings
  SettingsMigration mig(target);

  for (const auto& setting : stringMap)
    mig.MigrateStringSetting(setting.first, setting.second);

  for (const auto& setting : intMap)
    mig.MigrateIntSetting(setting.first, setting.second);

  for (const auto& setting : floatMap)
    mig.MigrateFloatSetting(setting.first, setting.second);

  for (const auto& setting : boolMap)
    mig.MigrateBoolSetting(setting.first, setting.second);

  if (mig.Changed())
  {
    // Set a title for the new instance settings
    std::string title;
    target.CheckInstanceSettingString("host", title);
    if (title.empty())
      title = "Migrated Add-on Config";

    target.SetInstanceSettingString("kodi_addon_instance_name", title);

    return true;
  }
  return false;
}

bool SettingsMigration::IsMigrationSetting(const std::string& key)
{
  return std::any_of(stringMap.cbegin(), stringMap.cend(),
                     [&key](const auto& entry) { return entry.first == key; }) ||
         std::any_of(intMap.cbegin(), intMap.cend(),
                     [&key](const auto& entry) { return entry.first == key; }) ||
         std::any_of(floatMap.cbegin(), floatMap.cend(),
                     [&key](const auto& entry) { return entry.first == key; }) ||
         std::any_of(boolMap.cbegin(), boolMap.cend(),
                     [&key](const auto& entry) { return entry.first == key; });
}

void SettingsMigration::MigrateStringSetting(const char* key, const std::string& defaultValue)
{
  std::string value;
  if (kodi::addon::CheckSettingString(key, value) && value != defaultValue)
  {
    m_target.SetInstanceSettingString(key, value);
    m_changed = true;
  }
}

void SettingsMigration::MigrateIntSetting(const char* key, int defaultValue)
{
  int value;
  if (kodi::addon::CheckSettingInt(key, value) && value != defaultValue)
  {
    m_target.SetInstanceSettingInt(key, value);
    m_changed = true;
  }
}

void SettingsMigration::MigrateFloatSetting(const char* key, float defaultValue)
{
  float value;
  if (kodi::addon::CheckSettingFloat(key, value) && value != defaultValue)
  {
    m_target.SetInstanceSettingFloat(key, value);
    m_changed = true;
  }
}

void SettingsMigration::MigrateBoolSetting(const char* key, bool defaultValue)
{
  bool value;
  if (kodi::addon::CheckSettingBoolean(key, value) && value != defaultValue)
  {
    m_target.SetInstanceSettingBoolean(key, value);
    m_changed = true;
  }
}