/*
    A Wallpaper Changer utility for Dock

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
                                                                                                                                                       

#include <vector>
#include <gui/button.h>
#include <gui/imagebutton.h>
#include <gui/layoutview.h>
#include <gui/frameview.h>
#include <gui/stringview.h>
#include <util/resources.h>
#include <util/settings.h>
#include <gui/requesters.h>
#include <gui/dropdownmenu.h>
#include <gui/menu.h>
#include <util/datetime.h>
#include <gui/desktop.h>
#include <gui/image.h>
#include <storage/file.h>
#include <atheos/time.h>
#include <storage/streamableio.h>
#include <iostream>

#include <appserver/dockplugin.h>
#include "WallpaperChangerSettings.h"
#include "WallpaperChangerLooper.h"

using namespace os;
using namespace std;                                                                                                                                                                                                     
                                                                                                                                                                                                        
class DockWallpaperChanger : public View
{
	public:
		DockWallpaperChanger( os::Path cPath, os::Looper* pcDock );
		~DockWallpaperChanger();
		
		Point GetPreferredSize( bool bLargest ) const;
		os::Path GetPath() { return( m_cPath ); }
		
        virtual void Paint( const Rect &cUpdateRect );
		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		virtual void MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData );
		virtual void MouseUp( const os::Point & cPosition, uint32 nButton, os::Message * pcData );
		virtual void MouseDown( const os::Point& cPosition, uint32 nButtons );
		virtual void HandleMessage(Message* pcMessage);
		void ChangeFile();
		void ConvertTime();
		void UpdateImage();    
	private:
		Message* m_pcPassedToPrefsMessage;
		Bitmap* cm_pcBitmap;
		
		void LoadSettings();
		void SaveSettings();

		void ShowPrefs();
		
		os::Path m_cPath;
		os::Looper* m_pcDock;
		bool m_bCanDrag;
		bool m_bDragging;
		bool bRandom;
		int32 nTime;
		String cCurrentImage;
		Menu* pcContextMenu;
		bool m_bHover;
		BitmapImage* m_pcIcon;
		BitmapImage* m_pcHoverIcon;
		os::File* pcFile;
		os::ResStream *pcStream;
		void RefreshIcons();
		BitmapImage* pcPaint;
		WallpaperChangerSettings* pcPrefsWin;
		DockWallpaperChangerLooper* pcChangeLooper;
		bigtime_t nTimerTime;
};






