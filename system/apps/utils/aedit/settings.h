// AEdit -:-  (C)opyright 2000-2002 Kristian Van Der Vliet
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef __AEDIT_SETTINGS_H_
#define __AEDIT_SETTINGS_H_

#include <gui/rect.h>

#include <string>

using namespace os;

class AEditSettings
{
	public:
		AEditSettings(void);
		~AEditSettings();
		void SetWindowPos(Rect cPosition);
		Rect GetWindowPos(void);
		void SetSaveOnExit(bool bSave);
		bool GetSaveOnExit(void);

		int Load(void);
		int Save(void);
		
	private:
		std::string zSettingsFile;

		Rect cWindowPosition;
		bool bSaveOnExit;
};

#define AEDIT_SETTINGS_PATH "/config/aedit.cfg"

#define ESETTINGSCREATE	1
#define ESETTINGSWRITE 	2
#define ESETTINGSSTAT	3
#define ESETTINGSREAD	4

#endif

