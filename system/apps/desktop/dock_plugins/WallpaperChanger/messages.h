#ifndef _MESSAGES_H
#define _MESSAGES_H

#define PLUGIN_NAME       "WallpaperChanger©"
#define PLUGIN_VERSION    "1.0"
#define PLUGIN_DESC       "A Wallpaper Changer Plugin for Dock"
#define PLUGIN_AUTHOR     "Rick Caudill"

enum
{
	M_PREFS=1,
	M_PREFS_SEND_TO_PARENT=2,
	M_PARENT_SEND_TO_PREFS=3,
	M_PREFS_CANCEL=4,
	M_WALLPAPERCHANGER_ABOUT=5,
	M_CHANGE_FILE = 6,
	M_HELP = 7
};

#endif
