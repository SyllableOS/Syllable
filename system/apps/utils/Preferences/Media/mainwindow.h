
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
	OutputPrefs( os::MediaOutput* pcOutput, bool bDefaultVideo, bool bDefaultAudio );
	~OutputPrefs();
private:
	os::CheckBox* m_pcDefaultVideo;
	os::CheckBox* m_pcDefaultAudio;
	os::View* m_pcConfigView;
	os::MediaOutput* m_pcOutput;
};


class SoundPrefs : public os::VLayoutNode
{
public:
	SoundPrefs( os::Window* pcParent,os::String zCurrentStartup );
	~SoundPrefs();
	os::String GetString() { return( m_pcStartupSound->GetCurrentString() ); }
private:
	os::DropdownMenu* m_pcStartupSound;
	
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
	void Treeview();
	void VideoOutputChange();
	void AudioOutputChange();
	void StartupSoundChange();

	// Default outputs
	os::String cCurrentVideo;
	os::String cUndoVideo;

	os::String cCurrentAudio;
	os::String cUndoAudio;

	os::String cCurrentStartupSound;
	os::String cUndoStartupSound;

	// Refresh flag/custom or not

	os::LayoutView * pcLRoot;
	os::VLayoutNode * pcVLRoot;
	os::HLayoutNode* pcHLSettings;
	os::Splitter *pcSplitter;
	SettingsTree* m_pcTree;
	os::VLayoutNode* m_pcPrefs;
	os::HLayoutNode * pcHLButtons;
	os::Button * pcBDefault, *pcBApply, *pcBUndo, *pcBControls;
	
	// State
	
	int m_nTreeSelect;
	
	int m_nOutputStart;
	int m_nInputStart;
	int m_nCodecStart;

};

#endif /* __MAINWINDOW_H_ */


