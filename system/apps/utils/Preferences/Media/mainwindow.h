
/*  Syllable Media Preferences
 *  Copyright (C) 2003 Arno Klenke
 *  Based on the preferences code by Daryl Dudey
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */

#ifndef __MAINWINDOW_H_
#define __MAINWINDOW_H_

#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/frameview.h>
#include <gui/button.h>
#include <gui/stringview.h>
#include <gui/dropdownmenu.h>
#include <gui/slider.h>
#include <media/output.h>
#include <media/manager.h>
#include <media/server.h>
#include <vector.h>

class MainWindow:public os::Window
{
      public:
	MainWindow( const os::Rect & cFrame );
	 ~MainWindow();
	virtual void HandleMessage( os::Message * pcMessage );

	bool OkToQuit( void );

      private:
	void ShowData();
	void Apply();
	void Undo();
	void Default();
	void Plugins();
	void OutputChange();

	// Default outputs
	os::String cCurrentVideo;
	os::String cUndoVideo;

	os::String cCurrentAudio;
	os::String cUndoAudio;

	os::String cCurrentStartupSound;
	os::String cUndoStartupSound;

	// Refresh flag/custom or not

	os::LayoutView * pcLRoot;
	os::VLayoutNode * pcVLRoot, *pcVLSettings, *pcVLSounds;
	os::HLayoutNode * pcHLButtons, *pcHLVideoOutput, *pcHLAudioOutput, *pcHLStartupSound;
	os::FrameView * pcFVSettings, *pcFVSounds;
	os::Button * pcBDefault, *pcBApply, *pcBUndo, *pcBPlugins, *pcBControls;
	os::DropdownMenu * pcDDMVideoOutput, *pcDDMAudioOutput, *pcDDMStartupSound;

};

#endif /* __MAINWINDOW_H_ */

