[![Build Status](https://travis-ci.org/kodi-pvr/pvr.vuplus.svg?branch=master)](https://travis-ci.org/kodi-pvr/pvr.vuplus)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/5120/badge.svg)](https://scan.coverity.com/projects/5120)

# Enigma2 PVR
PVR client addon for Kodi to control Enigma2-based TV-receivers/DVR/set-top boxes. 

Enigma2 is a open source TV-receiver/DVR platform which Linux-based firmware (OS images) can be loaded onto many Linux-based set-top boxes (satellite, terrestrial, cable or a combination of these) from different manufacturers. Such Enigma2-based TV-boxes include Amiko, DBox2, Dreambox, Gigablue, SAB, VuPlus/Vu+, WeTek, Zgemma, Xtrend, and more. 

For those interested; there are multiple open source projects and teams of independent developers out there making unofficial third-party firmware (OS) images for newer set-top box hardware that can run Enigma2. The Enigma2 framework/GUI and TV-zapping application running on Enigma2-based TV-boxes is written in Python (for LinuxTV DVB API) and its firmware OS images are based on OpenEmbedded is built with Yocto. 

## Build instructions

### Linux

1. `git clone https://github.com/xbmc/xbmc.git`
2. `git clone https://github.com/kodi-pvr/pvr.vuplus.git`
3. `cd pvr.vuplus && mkdir build && cd build`
4. `cmake -DADDONS_TO_BUILD=pvr.vuplus -DADDON_SRC_PREFIX=../.. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=../../xbmc/addons -DPACKAGE_ZIP=1 ../../xbmc/cmake/addons`
5. `make`

##### Useful links

* [Kodi's PVR user support] (http://forum.kodi.tv/forumdisplay.php?fid=167)
* [Kodi's PVR development support] (http://forum.kodi.tv/forumdisplay.php?fid=136)

## Configuring the addon

### Connection
Within this tab the connection options need to be configured before it can be successfully enabled.

* **Enigma2 hostname IP-address**: The hostname (DNS-name) or IP-address of your Enigma2-based set-top box.
* **Web interface port**: The port used to connect to the web interface
* **Use secure HTTP (https)**: Use https to connect to the web interface
* **Username**: If the webinterface of the settop box is protected with a username / password combination this needs to be set in this option.
* **Password**: If the webinterface of the settop box is protected with a username / password combination this needs to be set in this option.
* **Enable automatic configuration for live streams**: When enabled the stream URL will be read from an M3U file. When disabled it is constructed based on the filename.
* **Streaming port**: This option defines the streaming port the settop box uses to stream live tv. The default is 8001 which should be fine if the user did not define a custom port within the webinterface.
Webinterface Port: This option defines the port that should be used to access the webinterface of the settop box.

### General
Within this tab general options are configured.

* **Fetch picons from web interface**: Fetch the picons straight from the Vuplus/Enigma 2 STB
* **Use picons.eu file formate**: Assume all picons files fetched from the STB start with `1_1_1_` and end with `_0_0_0`
* **Icon path**: In order to have Kodi display channel logos you have to copy the picons from your settop box onto your OpenELEC machine. You then need to specify this path in this property.
* **Update interval**: As the settop box can also be used to modify timers, delete recordings etc. and the settop box does not notify the Kodi installation, the addon needs to regularly check for updates (new channels, new/changed/deletes timers, deleted recordings, etc.) This property defines how often the addon checks for updates.

### Channels & EPG
Within this tab options that refer to channel and EPG data can be set.

* **Fetch only one TV bouquet**: If this option is set than only the channels that are in one specified TV bouquet will be loaded, instead of fetching all available channels in all available bouquets.
* **TV bouquet**: If the previous option has been enabled you need to specify the TV bouquet to be fetched from the settop box. Please not that this is the bouquet-name as shown on the settop box (i.e. "Favourites (TV)"). This setting is case-sensitive.
* **Zap before channelswitch (i.e. for Single Tuner boxes)**: When using the addon with a single tuner box it may be necessary that the addon needs to be able to zap to another channel on the settop box. If this option is enabled each channel switch in Kodi will also result in a channel switch on the settop box. Please note that "allow channel switching" needs to be enabled in the webinterface on the settop box.
* **Extract genre, season and episode info where possible**: Have kodi check the description fields in the EPG data and attempt to extrac
t genre, season and episode info where possible. Currently supports OTA, OpenTV and Rytec XMLTV EPG data for UK and Ireland channels. If y
ou would like to support other formats please raise an issue at the github project https://github.com/kodi-pvr/pvr.vuplus. Genre support s
pecificially requires the use of Rytec XMLTV Epg data. There is a CPU overhead to enabling this setting. After changing this option you wi
ll need to clear the EPG cache `Settings->PVR & Live TV->Guide->Clear cache` for it to take effect.
* **Log missing genre mappings**: If you would like missing genre mappings to be logged so you can report them enable this option. Note, any genres found that don't have a mapping will still be extracted and sent to Kodi as strings. Currently genres are extracted by looking for text between square brackets, e.g. [TV Drama]

### Recordings & Timers

* **Recording folder on receiver**: Per default the addon does not specify the recording folder in newly created timers, so the default set in the settop box will be used. If you want to specify a different folder (i.e. because you want all recordings scheduled via Kodi to be stored in a separate folder), then you need to set this option.
* **Use only the DVB boxes' current recording path**: If this option is not set the addon will fetch all available recordings from all configured paths from the STB. If this option is set then it will only list recordings that are stored within the "current recording path" on the settop box.
* **Keep folder structure for records**: If enabled do not specify a recording folder, when disabled (defaut), check if the recording is in it's own folder or in the root of ht recording path
* **Enable generate repeat timers**: Repeat timers will display as timer rules. Enabling this will make Kodi generate regular timers to match the repeat timer rules so the UI can show what's scheduled and currently recording for each repeat timer.
* **Number of repeat timers to generate**: The number of Kodi PVR timers to generate.
* **Enable autotimers**: When this is enabled there are some settings required on the STB to enable linking of AutoTimers (Timer Rules) to Timers in the Kodi UI. On the STB enable the following options (note that this feature supports OpenWebIf 1.3.x and higher):
  1. Hit `Menu` on the remote and go to `Timers->AutoTimers`
  2. Hit `Menu` again and then select `6 Setup`
  3. Set the following to option to `yes`
    * `Include "AutoTimer" in tags`
    * `Include AutoTimer name in tags`
* **Automatic timerlist cleanup**: If this option is set then the addon will send the command to delete completed timers from the STB after each update interval.

### Timeshift
* **Enable timeshift**: What timeshift option do you want. `Disabled`, only timeshift `On pause` or timeshift `On Playback`.
* **Timeshift buffer path**: The path used to store the timeshoft buffer. The default is the addon data folder in userdata

### Advanced
Within this tab more uncommon and advanced options can be configured.

* **Put outline (e.g. sub-title) before plot**: By default plot outline (short description on Enigma2) is not displayed in the UI. Can be
 displayed in EPG, Recordings or both. After changing this option you will need to clear the EPG cache `Settings->PVR & Live TV->Guide->C
lear cache` for it to take effect.
* **Send deep standby command**: If this option is set then the addon will send the DeepStandby-Command to the settop box when Kodi will be closed (or the addon will be deactivated), causing the settop box to shutdown into DeepStandby.
* **Custom live TV timeout (0 to use default)**: The timemout to use when trying to read live streams
* **Stream read chunk size**: The chunk size used by Kodi for streams. Default 0 to leave it to Kodi to decide.
