[![License: GPL-2.0-or-later](https://img.shields.io/badge/License-GPL%20v2+-blue.svg)](LICENSE.md)
[![Build Status](https://travis-ci.org/kodi-pvr/pvr.vuplus.svg?branch=Matrix)](https://travis-ci.org/kodi-pvr/pvr.vuplus/branches)
[![Build Status](https://dev.azure.com/teamkodi/kodi-pvr/_apis/build/status/kodi-pvr.pvr.vuplus?branchName=Matrix)](https://dev.azure.com/teamkodi/kodi-pvr/_build/latest?definitionId=68&branchName=Matrix)
[![Build Status](https://jenkins.kodi.tv/view/Addons/job/kodi-pvr/job/pvr.vuplus/job/Matrix/badge/icon)](https://jenkins.kodi.tv/blue/organizations/jenkins/kodi-pvr%2Fpvr.vuplus/branches/)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/5120/badge.svg)](https://scan.coverity.com/projects/5120)

# Enigma2 PVR
Enigma2 PVR client addon for [Kodi](https://kodi.tv)

## Overview
Enigma2 is a open source TV-receiver/DVR platform which Linux-based firmware (OS images) can be loaded onto many Linux-based set-top boxes (satellite, terrestrial, cable or a combination of these) from different manufacturers.

This addon leverages the OpenWebIf project to interact with the Enigma2 device via Restful APIs: (https://github.com/E2OpenPlugins/e2openplugin-OpenWebif)

### Compatibility

**Note:** Some images do not use OpenWebIf as the default web interface. In these images some standard functionality may still work but is not guaranteed. Some features that may not function include:
* Autotimers
* Drive Space Reporting
* Embedded EPG Genre IDs
* Full Tuner Signal Support (Including Service Providers)
* Timer and Recording descriptions: If your provider only uses short description (plot outline) instead of long descrption (plot) then info will not be displayed pertaining to the shows in question. For OpenWebIf clients a JSON API is available to populate the missing data.
* Edit recording name, last played position and play count for recordings
* Bouquet (and group specific) Backend Channel Numbers

Some features are only available with at least certain OpenWebIf versions:
* 1.3.0
  * AutoTimers
* 1.3.5
  * Embedded EPG Genres
  * Tuner Details
  * Provider Name
  * Picon URLs
* 1.3.6
  * Editing Recordings
* 1.3.7
  * Backend Channel Numbers

### IPTV Streams

The majority of Enigma2 devices support viewing and recording IPTV streams. They do not however support streaming IPTV from the device. The addon supports IPTV by simply passing the URL used on the device to Kodi PVR. This means that timeshifting cannot be used (and will not be supported in the future). Note that if your IPTV provider restricts the number of active streams each kodi instance viewing it will count as an active stream.

The option `Enable automatic configuration for live streams` is ignored for channels that are IPTV Streams.

## Build instructions

### Linux

1. `git clone --branch master https://github.com/xbmc/xbmc.git`
2. `git clone https://github.com/kodi-pvr/pvr.vuplus.git`
3. `cd pvr.vuplus && mkdir build && cd build`
4. `cmake -DADDONS_TO_BUILD=pvr.vuplus -DADDON_SRC_PREFIX=../.. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=../../xbmc/build/addons -DPACKAGE_ZIP=1 ../../xbmc/cmake/addons`
5. `make`

The addon files will be placed in `../../xbmc/build/addons` so if you build Kodi from source and run it directly the addon will be available as a system addon.

As an alternative to step 4 the following command can be run whic is addon agnostic:
 - `cmake -DADDONS_TO_BUILD=$(basename $(dirname $(pwd))) -DADDON_SRC_PREFIX=../.. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=../../xbmc/addons -DPACKAGE_ZIP=1 ../../xbmc/cmake/addons`

### Mac OSX

In order to build the addon on mac the steps are different to Linux and Windows as the cmake command above will not produce an addon that will run in kodi. Instead using make directly as per the supported build steps for kodi on mac we can build the tools and just the addon on it's own. Following this we copy the addon into kodi. Note that we checkout kodi to a separate directory as this repo will only only be used to build the addon and nothing else.

#### Build tools and initial addon build

1. Get the repos
 * `cd $HOME`
 * `git clone https://github.com/xbmc/xbmc xbmc-addon`
 * `git clone https://github.com/kodi-pvr/pvr.vuplus`
2. Build the kodi tools
 * `cd $HOME/xbmc-addon/tools/depends`
 * `./bootstrap`
 * `./configure --host=x86_64-apple-darwin`
 * `make -j$(getconf _NPROCESSORS_ONLN)`
3. Build the addon
 * `cd $HOME/xbmc-addon`
 * `make -j$(getconf _NPROCESSORS_ONLN) -C tools/depends/target/binary-addons ADDONS="pvr.vuplus" ADDON_SRC_PREFIX=$HOME`

Note that the steps in the following section need to be performed before the addon is installed and you can run it in Kodi.

#### To rebuild the addon and copy to kodi after changes (after the initial addon build)

1. `cd $HOME/pvr.vuplus`
2. `./build-install-mac.sh ../xbmc-addon`

If you would prefer to run the rebuild steps manually instead of using the above helper script check the appendix [here](#manual-steps-to-rebuild-the-addon-on-macosx)

## Support

### Useful links

* [Kodi's PVR user support forum](https://forum.kodi.tv/forumdisplay.php?fid=167)
* [Report an issue on Github](https://github.com/kodi-pvr/pvr.vuplus/issues)
* [Kodi's PVR development support forum](https://forum.kodi.tv/forumdisplay.php?fid=136)

### Logging

When reporting issues a debug log should always be supplied. You can use the following guide: [Easy way to submit Kodi debug logs](https://kodi.wiki/view/Log_file/Easy)

For more detailed info on logging please see the appendix [here](#logging-detailed)

## Configuring the addon

### Settings Levels
In Kodi 18.2 the level of settings shown will correspond to the level set in the main kodi settings UI: `Basic`, `Standard`, `Advanced` and `Expert`. From Kodi 19 it will be possible to change the settingds level from within the addon settings itself.

### Connection
Within this tab the connection options need to be configured before it can be successfully enabled.

* **Enigma2 hostname or IP address**: The IP address or hostname of your enigma2 based set-top box.
* **Web interface port**: The port used to connect to the web interface.
* **Use secure HTTP (https)**: Use https to connect to the web interface.
* **Username**: If the webinterface of the set-top box is protected with a username/password combination this needs to be set in this option.
* **Password**: If the webinterface of the set-top box is protected with a username/password combination this needs to be set in this option.
* **Enable automatic configuration for live streams**: When enabled the stream URL will be read from an M3U8 file. When disabled it is constructed based on the service reference of the channel. This option is rarely required and should not be enbaled unless you have a special use case. If viewing an IPTV Stream this option has no effect on those channels.
* **Streaming port**: This option defines the streaming port the set-top box uses to stream live tv. The default is 8001 which should be fine if the user did not define a custom port within the webinterface.
* **Use secure HTTP (https) for streams**: Use https to connect to streams.
* **Use login for streams**: Use the login username and password for streams.
* **Connection check timeout**: The value in seconds to wait for a connection check to complete before failure. Useful for tuning on older Enigma2 devices. Note, this setting should rarely need to be changed. It's more likely the `Connection check interval` will have the desired effect. Default is 30 seconds.
* **Connection check interval**: The value in seconds to wait between connection checks. Useful for tuning on older Enigma2 devices. Default is 10 seconds.

### General
Within this tab general options are configured.

* **Set program id for live channel streams**: Some TV Providers (e.g. Nos - Portugal) using MPTS send extra program stream information. Setting the program id allows kodi to select the correct stream and therefore makes the channel/recording playable. Note that it takes approx 33% longer to open any stream with this option enabled.
* **Fetch picons from web interface**: Fetch the picons straight from the Enigma 2 set-top box.
* **Use picons.eu file format**: Assume all picons files fetched from the set-top box start with `1_1_1_` and end with `_0_0_0`.
* **Use OpenWebIf picon path**: Fetch the picon path from OpenWebIf instead of constructing from ServiceRef. Requires OpenWebIf 1.3.5 or higher. There is no effect if used on a lower version of OpenWebIf.
* **Icon path**: In order to have Kodi display channel logos you have to copy the picons from your set-top box onto your OpenELEC machine. You then need to specify this path in this property.
* **Update interval**: As the set-top box can also be used to modify timers, delete recordings etc. and the set-top box does not notify the Kodi installation, the addon needs to regularly check for updates (new channels, new/changed/deletes timers, deleted recordings, etc.) This property defines how often the addon checks for updates. Please note that updating the recordings frequently can keep your receiver and it's harddisk from entering standby automatically.
* **Update mode**: The mode used when the update interval is reached. Note that if there is any timer change detected a recordings update will always occur regardless of the update mode. Choose from one of the following two modes:
    - `Timers and Recordings` - Update all timers and recordings.
    - `Timers only` - Only update the timers. If it's important to not spin up the HDD on your STB use this option. The HDD should then only spin up when a timer event occurs.
* **Channel and groups update mode**: The mode used when the hour in the next settings is reached. Choose from one of the following three modes:
    - `Disabled` - Never check for channel and group changes.
    - `Notify on UI and Log` - Display a notice in the UI and log the fact that a change was detectetd.
    - `Reload Channels and Groups` - Disconnect and reconnect with E2 device to reload channels only if a change is detected.
* **Channel and group update hour (24h)**: The hour of the day when the check for new channels should occur. Default is 4h as the Auto Bouquet Maker (ABM) on the E2 device defaults to 3AM.

### Channels
Within this tab options that refer to channel data can be set. When changing bouquets you may need to clear the channel cache to the settings to take effect. You can do this by going to the following in Kodi settings: `Settings->PVR & Live TV->General->Clear cache`.

Note that channel numbers can be set in the addon based on either:
  1. Their first occurence when loaded, i.e. if a channel appears in multiple bouqets the channel number will be taken from the first bouquet in which it is loaded, any subsequent channel numbers will be ignored. This is useful as regardless of what group you are using in Kodi-PVR you can jump to a channel easily as it will always have the same number.
  2. Bouquet specific channel numbering, i.e. exactly as they appear in the backend. This can be useful when bouquet are used as differnt users (profile-like) lists of channels.

If Kodi PVR is set to use the channel numbers from the backend the numbers will match those on your STB. If this is not enabled each unique instance of a channel will be given the next free number starting from 1 (i.e. the 17th unique channel will be channel 17). Backend channel numbers will only work for OpenWebIf 1.3.5 and later and they have been tested using ABM (AutoBouquetsMaker).

* **Zap before channelswitch (i.e. for Single Tuner boxes)**: When using the addon with a single tuner box it may be necessary that the addon needs to be able to zap to another channel on the set-top box. If this option is enabled each channel switch in Kodi will also result in a channel switch on the set-top box. Please note that "allow channel switching" needs to be enabled in the webinterface on the set-top box.
* **Use group specific channel numbers from backend**: If this option is enabled then each group in kodi will match the exact channel numbers used on the backend bouquets. If disabled (default) each channel will only have a single backend channel number (first occurence when loaded).
* **Use standard channel service reference**: Usually service reference's for the channels are in a standard format like `1:0:1:27F6:806:2:11A0000:0:0:0:`. On occasion depending on provider they can be extended with some text e.g. `1:0:1:27F6:806:2:11A0000:0:0:0::UTV` or `1:0:1:27F6:806:2:11A0000:0:0:0::UTV + 1`. If this option is enabled then all read service reference's will be read as standard. This is default behaviour. Functionality like autotimers will always convert to a standard reference.
* **Retrieve provider name for channels**: Retrieve provider name from the backend when fetching channels. Default is enabled but disabling can speed up fetch times on older devices.
* **TV bouquet fetch mode**: Choose from one of the following three modes:
    - `All bouquets` - Fetch all TV bouquets from the set-top box.
    - `Some bouquets` - Only fetch the bouquet specified in the next option
    - `Favourites bouquet` - Only fetch the system bouquet for TV favourites.
    - `Custom bouquets` - Fetch a set of bouquets from the Set-top box whose names are loaded from an XML file.
* **Number of TV bouquets**: The number of TV bouquets to load when `Some bouquets` is the selected fetch mode. Up to 5 can be chosen. If more than 5 are required the `Custom bouquets` fetch mode should be used instead.
* **TV bouquet 1-5**: If the previous option has been has been set to `Some bouquets` you need to specify a TV bouquet to be fetched from the set-top box. Please not that this is the bouquet-name as shown on the set-top box (i.e. "Favourites (TV)"). This setting is case-sensitive.
* **Custom TV Groups file**: The file used to load the custom TV bouquets (groups). If no groups can be matched the channel list will default to 'Last Scanned (TV)'. The default file is `customTVGroups-example.xml`. Details on how to customise can be found in the next section of the README.
* **Fetch TV favourites bouquet**: If the fetch mode is `All bouquets` or `Only one bouquet` depending on your Enigma2 image you may need to explicitly fetch favourites if you require them. The options are:
    - `Disabled` - Don't explicitly fetch TV favourites.
    - `As first bouquet` - Explicitly fetch them as the first bouquet.
    - `As last bouquet` - Explicitly fetch them as the last bouquet.
* **Exclude last scanned bouquet**: Last scanned is a system bouquet containing all the TV and Radio channels found in the last scan. Any TV channels found in the Last Scanned bouquet can be displayed as a group called ```Last Scanned (TV)``` in Kodi. For TV this group is excluded by default. Disable this option to exclude this group. Note that if no TV groups are loaded the Last Scanned group for TV will be loaded by default regardless of this setting.
* **Radio bouquet fetch mode**: Choose from one of the following three modes:
    - `All bouquets` - Fetch all Radio bouquets from the set-top box.
    - `Some bouquets` - Only fetch the bouquet specified in the next option
    - `Favourites bouquet` - Only fetch the system bouquet for Radio favourites.
    - `Custom bouquets` - Fetch a set of bouquets from the Set-top box whose names are loaded from an XML file.
* **Number of radio bouquets**: The number of Radio bouquets to load when `Some bouquets` is the selected fetch mode. Up to 5 can be chosen. If more than 5 are required the `Custom bouquets` fetch mode should be used instead.
* **Radio bouquet 1-5**: If the previous option has been has been set to `Some bouquets` you need to specify a Radio bouquet to be fetched from the set-top box. Please not that this is the bouquet-name as shown on the set-top box (i.e. "Favourites (Radio)"). This setting is case-sensitive.
* **Custom Radio Groups file**: The file used to load the custom Radio bouquets (groups). If no groups can be matched the channel list will default to 'Last Scanned (Radio)'. The default file is `customRadioGroups-example.xml`. Details on how to customise can be found in the next section of the README.
* **Fetch Radio favourites bouquet**: If the fetch mode is `All bouquets` or `Only one bouquet` depending on your Enigma2 image you may need to explicitly fetch favourites if you require them. The options are:
    - `Disabled` - Don't explicitly fetch Radio favourites.
    - `As first bouquet` - Explicitly fetch them as the first bouquet.
    - `As last bouquet` - Explicitly fetch them as the last bouquet.
* **Exclude last scanned bouquet**: Last scanned is a system bouquet containing all the TV and Radio channels found in the last scan. Any Radio channels found in the Last Scanned bouquet can be displayed as a group called ```Last Scanned (Radio)``` in Kodi. For Radio this group is excluded by default. Disable this option to show this group.

### EPG
Within this tab options that refer to EPG data can be set. Excluding logging missing genre text mappings all other options will require clearing the EPG cache to take effect. This can be done by going to `Settings->PVR & Live TV->Guide->Clear cache` in Kodi after the addon restarts.

Information on customising the extraction and mapper configs can be found in the next section of the README.

* **Extract season, episode and year info where possible**: Check the description fields in the EPG data and attempt to extract season, episode and year info where possible. In addtion can also extract properties like new, live and premiere info.
* **Extract show info file**: The config used to extract season, episode and year information. The default file is `English-ShowInfo.xml`.
* **Enable genre ID mappings**: If the genre IDs sent with EPG data from your set-top box are not using the DVB standard, map from these to the DVB standard IDs. Sky UK for instance uses OpenTV, in that case without this option set the genre colouring and text would be incorrect in Kodi.
* **Genre ID mappings file**: The config used to map set-top box EPG genre IDs to DVB standard IDs. The default file is `Sky-UK.xml`.
* **Enable Rytec genre text mappings**: If you use Rytec XMLTV EPG data this option can be used to map the text genres to DVB standard IDs.
* **Rytec genre text mappings file**: The config used to map Rytec Genre Text to DVB IDs. The default file is `Rytec-UK-Ireland.xml`.
* **Log missing genre text mappings**: If you would like missing genre mappings to be logged so you can report them enable this option. Note: any genres found that don't have a mapping will still be extracted and sent to Kodi as strings. Currently genres are extracted by looking for text between square brackets, e.g. [TV Drama], or for major, minor genres using a dot (.) to separate [TV Drama. Soap Opera]
* **EPG update delay per channel**: For older Enigma2 devices EPG updates can effect streaming quality (such as buffer timeouts). A delay of between 250ms and 5000ms can be introduced to improve quality. Only recommended for older devices. Choose the lowest value that avoids buffer timeouts.
* **Skip intial EPG load**: Ignore the intial EPG load (now and next). Enabled by default to prevent crash issues on LibreElec/CoreElec.

### Recordings
The following configuration is available on the Recordings tab of the addon settings.

* **Store last played/play count on the backend**: Store last played position and count on the backend so they can be shared across kodi instances. Only supported on OpenWebIf version 1.3.6+.
* **Share last played across**: The options are:
    - `Kodi instances` - Only use the value in kodi and will not affect last played on the E2 device.
    - `Kodi/E2 instances` - Use the value across kodi and the E2 device so they stay in sync. Last played will be synced with the E2 device once every 5-10 minutes per recording if the PVR menus are in use. Note that only a single kodi instance is required to have this option enabled.
* **Recording folder on receiver**: Per default the addon does not specify the recording folder in newly created timers, so the default set in the set-top box will be used. If you want to specify a different folder (i.e. because you want all recordings scheduled via Kodi to be stored in a separate folder), then you need to set this option.
* **Use only the DVB boxes' current recording path**: If this option is not set the addon will fetch all available recordings from all configured paths from the set-top box. If this option is set then it will only list recordings that are stored within the "current recording path" on the set-top box.
* **Keep folder structure for records**: If enabled do not specify a recording folder, when disabled (defaut), check if the recording is in it's own folder or in the root of the recording path.
* **Use recursive listing for recording locations**By default only the root of the location will be used to list recordings. When enabled the contents of child folders will be included.
* **Enable EDLs support**: EDLs are used to define commericals etc. in recordings. If a tool like [Comskip]() is used to generate EDL files enabling this will allow Kodi PVR to use them. E.g. if there is a file called ```my recording.ts``` the EDL file should be call ```my recording.edl```. Note: enabling this setting has no effect if the files are not present.
* **EDL start time padding**: Padding to use at an EDL stop. I.e. use a negative number to start the cut earlier and positive to start the cut later. Default 0.
* **EDL stop time padding**: Padding to use at an EDL stop. I.e. use a negative number to stop the cut earlier and positive to stop the cut later. Default 0.

### Timers

**Using Padding for Timers in Kodi PVR**

Using padding for timers allows you to start a recording some time earlier and finish later in case the actual start and/or run time of a show is incorrect. It's important to note that Enigma2 devices allow you set a padding for all timers, both regular timers and autotimers but it only applies to timers created on the Enigma2 device directly. In the case of autotimers it does not matter if they are created from Kodi or directly on the Enigma2 device as new once off timers are generated on the device so will use this default device padding. It is not possible to set padding for autotimers directly in Kodi PVR at present. You can change these settings on your Enigma2 device as follows:
  1. Hit `Menu` on the remote and go to `Setup->Recordings, playback & timeshift->Recording & playback`
  2. Set the following options to your chosen values:
    * `Margin before recording (minutes)`
    * `Margin after recording (minutes)`

If setting padding in Kodi PVR it's only supported on certain timer types, i.e. `Once off time/channel based`, `Repeating time/channel based` and `One time guide-based`. As the Enigma2 device does not support padding per Timer the `tags` field is used to store the padding set in Kodi PVR. If a padding value is not set for these timer types the addon will use the Enigma2 devices default padding value instead. So if you have set a value on the Enigma2 device, this can be overidden in the Kodi UI (note, the default from the Enigma2 device does not display in the Add Timer UI, it won't show until after the timer is created and only if no padding values are set).

**Timer Types**

The addon provides the following types of timers and timer rules that the user can create:

* **Once off timer (channel)**: This timer can be created from the add timer UI on the main PVR screen (It cannot be selected from the EPG UI). If running OpenWebIf the timer will be populated with the EPG Entry (if available) at the start time for that channel.
* **Repeating time/channel based**: This timer can be created from the add timer UI on the main PVR screen (It cannot be selected from the EPG UI). This type is a timer rule and generates timers. The timers that are generated cannot be edited and will be of the type `Once off timer (repeating)`.
* **One time guide-based**: This timer can be created from the EPG UI as well as when playing a channel. It is the timer used when the user selects `Record` when accessing an EPG entry. It will create a timer that starts and ends as per the EPG entry. If playing back a channel it will start from now until the end of the current show. If using OpenWebIf, for providers that only use short description (plot outline) the addon will retrieve the correct description if available and use it in both the timer and resulting recording.
* **Auto guide-based**: This is a search based timer rule, using the show name and other factors the Enigma2 device will create timers for EPG entries that satisfy the search. The timers it creates are not editable and will be of the type `Once off timer (auto)`.

In addition there are some timers that can only be created by the addon and are read only:

* **Once off timer (repeating)**: Timer generated by a `Repeating time/channel based` timer rule. Will contain padding that can't be modified the same as the parent timer rule.
* **Once off timer (auto)**: Timer created by an `Auto guide-based` timer rule.
* **Repeating guide-based**: This type can only be created directly on the Enigma2 device. The type exists to allow users to view the timers in the PVR UI.

**Timer settings**

The following are the settings in the Timers tab:

* **Enable generate repeat timers**: Repeat timers will display as timer rules. Enabling this will make Kodi generate regular timers to match the repeat timer rules so the UI can show what's scheduled and currently recording for each repeat timer.
* **Number of repeat timers to generate**: The number of Kodi PVR timers to generate.
* **Automatic timerlist cleanup**: If this option is set then the addon will send the command to delete completed timers from the set-top box after each update interval.
* **Enable autotimers**: When this is enabled there are some settings required on the set-top box to enable linking of AutoTimers (Timer Rules) to Timers in the Kodi UI. The addon attempts to set these automatically on boot. To set manually on the set-top box enable the following options (note that this feature supports OpenWebIf 1.3.x and higher):
  1. Hit `Menu` on the remote and go to `Timers->AutoTimers`
  2. Hit `Menu` again and then select `6 Setup`
  3. Set the following to option to `yes`
    * `Include "AutoTimer" in tags`
    * `Include AutoTimer name in tags`
* **Limit 'Any Channel' autotimers to TV or Radio**: If last scanned groups are excluded attempt to limit new autotimers to either TV or Radio (dependent on channel used to create the autotimer). Note that if last scanned groups are enabled this is not possible and the setting will be ignored.
* **Limit to groups of original EPG channel**: For the channel used to create the autotimer limit to channel groups that this channel is a member of.

### Timeshift
Timeshifting allows you to pause live TV as well as move back and forward from your current position similar to playing back a recording. The buffer is deleted each time a channel is changed or stopped.

* **Enable timeshift**: What timeshift option do you want:
    - `Disabled` - No timeshifting
    - `On Pause` - Timeshifting starts when a live stream is paused. E.g. you want to continue from where you were at after pausing.
    - `On Playback` - Timeshifting starts when a live stream is opened. E.g. You can go to any point in the stream since it was opened.
* **Timeshift buffer path**: The path used to store the timeshift buffer. The default is the `addon_data/pvr.vuplus` folder in userdata.
* **Enable timeshift for IPTV streams**: Enable the timeshift feature for HTTP IPTV streams using `inputstream.ffmpegdirect`. Note that this feature only works `On playback` and will ignore the timeshift mode used for regular channel playback.
* **Use FFMpeg http reconnect options if possible**: Note this can only apply to http/https streams that are processed by libavformat (e.g. M3u8/HLS).
* **Use mpegts MIME type for unknown streams**: If the type of stream cannot be determined assume it's an MPEG TS stream.
* **Modify inputstream.ffmpegdirect settings..**: Open settings dialog for inputstream.ffmpegdirect for modification of timeshift and other settings.

### Backend
This category contains information and settings on/about the Enigma2 STB.

* **Wake on LAN MAC**: The MAC address of the Engima2 STB to be used for WoL (Wake On LAN).
* **Send powerstate mode on addon exit**: If this option is set to a value other than `DISABLED` then the addon will send a Powerstate command to the set-top box when Kodi will be closed (or the addon will be deactivated).
    - `Disabled` - No command sent when the addon exits
    - `Standby` - Send the standby command on exit
    - `Deep standby` - Send the deep standby command on exit. Note, the set-top box will not respond to Kodi after this command is sent.
    - `Wakeup, then standby` - Similar to standby but first sends a wakeup command. Can be useful if you want to ensure all streams have stopped. Note: if you use CEC this could cause your TV to wake.

### Advanced
Within this tab more uncommon and advanced options can be configured.

* **Put outline (e.g. sub-title) before plot**: By default plot outline (short description on Enigma2) is not displayed in the UI. Can be displayed in EPG, Recordings or both. After changing this option you will need to clear the EPG cache `Settings->PVR & Live TV->Guide->Clear cache` for it to take effect.
* **Custom live TV timeout (0 to use default)**: The timemout to use when trying to read live streams. Default for live streams is 0. Default for timeshifting is 10 seconds.
* **Stream read chunk size**: The chunk size used by Kodi for streams. Default at 0 to leave it to Kodi to decide. Can be useful to set manually when viewing streams remotely where buffering can occur as PVR is optimised for a local network.
* **Ignore debug logging in debug mode**: Debug log statements will not be displayed for the addon even though debug logging is enabled in Kodi. This can be useful when trying to debug an issue in Kodi which is not addon related.
* **Enable debug logging in normal mode**: Debug log statements will display for the addon even though debug logging may not be enabled in Kodi. Note that all debug log statements will display at NOTICE level.
* **Enable trace logging in debug mode**: Very detailed and verbose log statements will display in addition to standard debug statements. If enabled along with `Enable debug logging in normal mode` both trace and debug will display without debug logging enabled. In this case both debug and trace log statements will display at NOTICE level.

## Customising Config Files

The various config files have examples allowing users to create their own, making it possible to support custom config, other languages and formats. Each different type of config file is detailed below. Best way to learn about them is to read the config files themselves. Each contains details of how the config file works.

All of the files listed below are overwritten each time the addon starts. Therefore if you are customising files please create new copies with different file names. Note: that only the files below are overwritten any new files you create will not be touched.

After adding and selecting new config files you will need to clear the EPG cache `Settings->PVR & Live TV->Guide->Clear cache` for it to take effect in the case of EPG relatd config and for channel related config will need to clear the full cache `Settings->PVR & Live TV->General->Clear cache`.

If you would like to support other formats/languages please raise an issue at the github project https://github.com/kodi-pvr/pvr.vuplus, where you can either create a PR or request your new configs be shipped with the addon.

There is one config file located here: `userdata/addon_data/pvr.vuplus/genres/kodiDvbGenres.xml`. This simply contains the DVB genre IDs that Kodi supports. Can be a useful reference if creating your own configs. This file is also overwritten each time the addon restarts.

### Custom Channel Groups (Channels)

Config files are located in the `userdata/addon_data/pvr.vuplus/channelGroups` folder.

The following files are currently available with the addon:
    - `customTVGroups-example.xml`
    - `customRadioGroups-example.xml`

Note that both these files are provided as examples and are overwritten each time the addon starts. Therefore you should make copies and use those for your custom config.

The format is quite simple, containing a number of channel group/bouquet names.

### Season, Episode and Year Show Info (EPG_)

Config files are located in the `userdata/addon_data/pvr.vuplus/showInfo` folder.

The following files are currently available with the addon:
    - `English-ShowInfo.xml`

Note: the config file can contain as many pattern matches as are required. So if you need to support multiple languages in a single file that is possible. However there must be at least one <seasonEpisode> pattern and at least one <year> pattern. Proficiency in regular exressions is required!

### Genre ID Mappings (EPG)

Config files are located in the `userdata/addon_data/pvr.vuplus/genres/genreIdMappings` folder.

The following files are currently available with the addon:
    - `AU-SAT.xml`
    - `Sky-IT.xml`
    - `Sky-NZ.xml`
    - `Sky-UK.xml`

Note: that each source genre ID can be mapped to a DVB ID. However multiple source IDs can be mapped to the same DVB ID. Therefore there are exactly 256 <mapping> elements in each file as a genre ID is 8 bits. All values are in Hex. The first fours bits are the genreType in Kodi PVR and the last four bits are the genreSubType.

### Rytec Genre Text Mappings (EPG)

Config files are located in the `userdata/addon_data/pvr.vuplus/genres/genreRytecTextMappings` folder.

The following files are currently available with the addon:
    - `Rytec-UK-Ireland.xml`

Note: the config file can contain as many mappings as is required. Currently genres are extracted by looking for text between square brackets, e.g. [TV Drama], or for major, minor genres using a dot (.) to separate [TV Drama. Soap Opera]. The config file maps the text to a kodi DVB genre ID. If the full text cannot be matched it attempts to match just the major genre, i.e. "TV Drama" in the previous example. If a mapping cannot be found the text between the brackets will be used instead. However there will be no colouring in the Kodi EPG in this case.

## Appendix

### Manual Steps to rebuild the addon on MacOSX

The following steps can be followed manually instead of using the `build-install-mac.sh` in the root of the addon repo after the [initial addon build](#build-tools-and-initial-addon-build) has been completed.

**To rebuild the addon after changes**

1. `rm tools/depends/target/binary-addons/.installed-macosx*`
2. `make -j$(getconf _NPROCESSORS_ONLN) -C tools/depends/target/binary-addons ADDONS="pvr.vuplus" ADDON_SRC_PREFIX=$HOME`

or

1. `cd tools/depends/target/binary-addons/macosx*`
2. `make`

**Copy the addon to the Kodi addon directory on Mac**

1. `rm -rf "$HOME/Library/Application Support/Kodi/addons/pvr.vuplus"`
2. `cp -rf $HOME/xbmc-addon/addons/pvr.vuplus "$HOME/Library/Application Support/Kodi/addons"`

### Logging detailed

Some of the most useful information in your log are the details output at the start of the log, both for Kodi and the addon.

The first 5 lines of the log give details on the exact kodi flavour and version you are running:

```
01:10:45.744 T:140736264741760  NOTICE: -----------------------------------------------------------------------
01:10:45.744 T:140736264741760  NOTICE: Starting Kodi (18.0-BETA2 Git:20180903-5d7e1dbd9c-dirty). Platform: OS X x86 64-bit
01:10:45.744 T:140736264741760  NOTICE: Using Debug Kodi x64 build
01:10:45.744 T:140736264741760  NOTICE: Kodi compiled Sep  5 2018 by Clang 9.1.0 (clang-902.0.39.2) for OS X x86 64-bit version 10.9.0 (1090)
01:10:45.745 T:140736264741760  NOTICE: Running on Apple Inc. MacBook10,1 with OS X 10.13.6, kernel: Darwin x86 64-bit version 17.7.0
```

Secondly all the information of the Enigma2 image, web interface version etc. is also output:

```
12:36:55.888 T:123145422725120  NOTICE: AddOnLog: Enigma2 Client: pvr.vuplus - Open - VU+ Addon Configuration options
12:36:55.888 T:123145422725120  NOTICE: AddOnLog: Enigma2 Client: pvr.vuplus - Open - Hostname: '192.168.1.201'
12:36:55.888 T:123145422725120  NOTICE: AddOnLog: Enigma2 Client: pvr.vuplus - Open - WebPort: '80'
12:36:55.888 T:123145422725120  NOTICE: AddOnLog: Enigma2 Client: pvr.vuplus - Open - StreamPort: '8001'
12:36:55.888 T:123145422725120  NOTICE: AddOnLog: Enigma2 Client: pvr.vuplus - Open Use HTTPS: 'false'
12:36:56.308 T:123145422725120  NOTICE: AddOnLog: Enigma2 Client: pvr.vuplus - LoadDeviceInfo - DeviceInfo
12:36:56.308 T:123145422725120  NOTICE: AddOnLog: Enigma2 Client: pvr.vuplus - LoadDeviceInfo - E2EnigmaVersion: 2019-01-03
12:36:56.308 T:123145422725120  NOTICE: AddOnLog: Enigma2 Client: pvr.vuplus - LoadDeviceInfo - E2ImageVersion: 5.2.022
12:36:56.309 T:123145422725120  NOTICE: AddOnLog: Enigma2 Client: pvr.vuplus - LoadDeviceInfo - E2DistroVersion: openvix
12:36:56.309 T:123145422725120  NOTICE: AddOnLog: Enigma2 Client: pvr.vuplus - LoadDeviceInfo - E2WebIfVersion: OWIF 1.3.5
12:36:56.309 T:123145422725120  NOTICE: AddOnLog: Enigma2 Client: pvr.vuplus - LoadDeviceInfo - E2DeviceName: Ultimo4K
```

#### Description of Log levels

* **Error**: Something that will require intervention to resolve
* **Notice**: Could be an important piece of information or a warning that something has occurred that might be undesirable but has not affected normal operation
* **Info**: Some information on normal operation
* **Debug**: More detailed information that can aid in diagnosing issues.
* **Trace**: Extremely verbose logging, should rarely be required. Note: can only be enabled from the addon settings in the ```Advanced``` section.

#### Cleaning up the log

(If you have a fresh install of version 3.15.0 or later you can ignore this)

As the addon was upgraded over time some old settings are no longer required. These old settings can create a lot of extra logging on startup. To date there are four of these and they will present like this in your log:

```
23:17:22.696 T:3990016880   DEBUG: CSettingsManager: requested setting (extracteventinfo) was not found.
23:17:22.696 T:3990016880   DEBUG: CAddonSettings[pvr.vuplus]: failed to find definition for setting extracteventinfo. Creating a setting on-the-fly...
23:17:22.696 T:3990016880   DEBUG: CSettingsManager: requested setting (onegroup) was not found.
23:17:22.696 T:3990016880   DEBUG: CAddonSettings[pvr.vuplus]: failed to find definition for setting onegroup. Creating a setting on-the-fly...
23:17:22.696 T:3990016880   DEBUG: CSettingsManager: requested setting (onlyonegroup) was not found.
23:17:22.696 T:3990016880   DEBUG: CAddonSettings[pvr.vuplus]: failed to find definition for setting onlyonegroup. Creating a setting on-the-fly...
23:17:22.697 T:3990016880   DEBUG: CSettingsManager: requested setting (setpowerstate) was not found.
23:17:22.697 T:3990016880   DEBUG: CAddonSettings[pvr.vuplus]: failed to find definition for setting setpowerstate. Creating a setting on-the-fly...
```

If you see these in debug mode and would like to clean them up you can either:

1. Remove the offending lines from you settings.xml file OR
2. Delete you settings.xml and restart kodi. Note that if you use this option you will need to reconfigure the addon from scratch.

Your settings.xml file can be found here: ```userdata/addon_data/pvr.vuplus/settings.xml```
