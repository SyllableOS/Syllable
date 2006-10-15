
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
#include <gui/separator.h>
#include <gui/splitter.h>
#include <gui/treeview.h>
#include <gui/checkbox.h>
#include <media/output.h>
#include <media/manager.h>
#include <media/server.h>
#include <vector>

class SettingsTree : public os::TreeView
{
public:
	SettingsTree();
	~SettingsTree();
	os::Point GetPreferredSize( bool bLargets ) const; 
};


class InputPrefs : public os::VLayoutNode
{
public:
	InputPrefs( os::MediaInput* pcInput );
	~InputPrefs();
private:
	os::View* m_pcConfigView;
	os::MediaInput* m_pcInput;
};


class CodecPrefs : public os::VLayoutNode
{
public:
	CodecPrefs( os::MediaCodec* pcCodec );
	~CodecPrefs();
private:
	os::View* m_pcConfigView;
	os::MediaCodec* m_pcCodec;
};


class OutputPrefs : public os::VLayoutNode
{
public:
	OutputPrefs( os::MediaOutput* pcOutput );
	~OutputPrefs();
private:
	os::View* m_pcConfigView;
	os::MediaOutput* m_pcOutput;
};


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
	void DeviceChanged();
	void InputChange();
	void VideoOutputChange();
	void AudioOutputChange();

	// Default outputs
	os::String cCurrentVideo;
	os::String cUndoVideo;

	os::String cCurrentAudio;
	os::String cUndoAudio;
	
	os::String cCurrentInput;
	os::String cUndoInput;

	// Refresh flag/custom or not

	os::LayoutView * pcLRoot;
	os::VLayoutNode * pcVLRoot;
	os::LayoutView* pcSettingsView;
	os::Splitter *pcSplitter;
	os::DropdownMenu* pcDevice;
	os::DropdownMenu* pcDefaultInput;
	os::DropdownMenu* pcDefaultAudioOut;
	os::DropdownMenu* pcDefaultVideoOut;
	os::HLayoutNode * pcHLButtons;
	os::Button * pcBDefault, *pcBApply, *pcBUndo, *pcBControls;
	
	// State
	
	int m_nDeviceSelect;
	
	int m_nOutputStart;
	int m_nInputStart;
	int m_nCodecStart;

};

#endif /* __MAINWINDOW_H_ */


