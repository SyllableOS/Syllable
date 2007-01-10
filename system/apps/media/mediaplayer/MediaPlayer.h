/*  Syllable Media Player
 *  Copyright (C) 2003 Arno Klenke
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
 
#ifndef _MEDIAPLAYER_H_
#define _MEDIAPLAYER_H_ 

#include <atheos/time.h>
#include <gui/view.h>
#include <gui/window.h>
#include <gui/imagebutton.h>
#include <gui/requesters.h>
#include <gui/menu.h>
#include <gui/desktop.h>
#include <gui/filerequester.h>
#include <util/application.h>
#include <util/locale.h>
#include <atheos/threads.h>
#include <storage/path.h>
#include <media/manager.h>
#include <media/input.h>
#include <media/output.h>
#include <media/codec.h>
#include <media/format.h>
#include <media/packet.h>
#include <media/inputselector.h>
#include "seekslider.h"
#include "cimagebutton.h"
#include "resources/mediaplayer.h"

/* Messages */
enum {
	/* Button / menu messages */
	MP_GUI_PLAY_PAUSE,
	MP_GUI_STOP,
	MP_GUI_SEEK,
	MP_GUI_TRACK_SELECTED,
	MP_GUI_FULLSCREEN,
	MP_GUI_ABOUT,
	MP_GUI_QUIT,
	MP_GUI_OPEN,
	MP_GUI_OPEN_INPUT,
	MP_GUI_FILE_INFO,
		
	/* Input selector messages */
	MP_IS_OPEN,
	MP_IS_CANCEL,
	
	/* Internal messages */
	MP_CREATE_VIDEO,
	MP_DELETE_VIDEO,
	MP_UPDATE_VIDEO,
	MP_STATE_CHANGED
};

enum {
	MP_STATE_STOPPED,
	MP_STATE_PLAYING,
	MP_STATE_PAUSED,
};

class MPWindow : public os::Window
{
public:
	MPWindow( const os::Rect& cFrame, const std::string& cName, const std::string& cTitle );
	~MPWindow();
	void HandleMessage( os::Message* pcMessage );
	bool OkToQuit();
	void FrameMoved( const os::Point& cDelta );
	void FrameSized( const os::Point& cDelta );
	void SetState( uint8 nState ) { m_nState = nState; }
	void SetVideo( os::View* pcView ) { m_pcVideo = pcView; }
	os::View* GetControlView() { return( m_pcControls ); }
	os::SeekSlider* GetSlider() { return( m_pcSlider ); } 
	os::Menu* GetTracksMenu() { return( m_pcTracksMenu ); }
private:
	uint8 m_nState;
	os::Menu* m_pcMenuBar;
	os::Menu* m_pcTracksMenu;
	os::View* m_pcVideoView;
	os::View* m_pcControls;
	os::View* m_pcVideo;
	os::IPoint m_cVideoSize;
	os::CImageButton* m_pcPlayPause;
	os::CImageButton* m_pcStop;
	os::CImageButton* m_pcFullScreen;
	os::SeekSlider* m_pcSlider;
	os::FileRequester* m_pcFileDialog;
};

class MPApp : public os::Application
{
public:
	MPApp( const char* pzMimeType, os::String zFileName, bool bLoad );
	~MPApp();
	void HandleMessage( os::Message* pcMessage );
	bool OkToQuit();
	void Open( os::String zFileName, os::String zInput );
	void Close();
	void PlayThread();
	void ChangeFullScreen();
private:
	MPWindow* m_pcWin;
	os::Rect  m_sSavedFrame;
	bool m_bFullScreen;
	uint8 m_nState;
	bool m_bFileLoaded;
	os::String m_zFileName;
	os::MediaInputSelector* m_pcInputSelector;
	os::MediaManager* m_pcManager;
	os::MediaInput* m_pcInput;
	bool m_bPacket;
	bool m_bStream;
	uint32 m_nTrackCount;
	uint32 m_nTrack;
	bool m_bAudio;
	bool m_bVideo;
	uint32 m_nAudioStream;
	uint32 m_nVideoStream;
	os::MediaFormat_s m_sAudioFormat;
	os::MediaFormat_s m_sVideoFormat;
	os::MediaCodec* m_pcAudioCodec;
	os::MediaCodec* m_pcVideoCodec;
	os::MediaOutput* m_pcAudioOutput;
	os::MediaFormat_s m_sAudioOutFormat;
	os::MediaOutput* m_pcVideoOutput;
	
	bool m_bPlayThread;
	thread_id m_hPlayThread;
	uint64 m_nLastPosition;
	
};

#endif






















