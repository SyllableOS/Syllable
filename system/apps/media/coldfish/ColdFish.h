/*  ColdFish Music Player
 *  Copyright (C) 2003 Kristian Van Der Vliet
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

#ifndef _COLDFISH_H_
#define _COLDFISH_H_

#include <atheos/time.h>
#include <gui/view.h>
#include <gui/window.h>
#include <gui/imagebutton.h>
#include <gui/requesters.h>
#include <gui/menu.h>
#include <gui/desktop.h>
#include <gui/layoutview.h>
#include <gui/listview.h>
#include <gui/filerequester.h>
#include <util/application.h>
#include <util/settings.h>
#include <util/locale.h>
#include <atheos/threads.h>
#include <media/manager.h>
#include <media/input.h>
#include <media/output.h>
#include <media/codec.h>
#include <media/format.h>
#include <media/packet.h>
#include <media/inputselector.h>
#include <storage/path.h>
#include "resources/coldfish.h"
#include "messages.h"
#include "seekslider.h"
#include "lcd.h"
#include "SelectWin.h"


enum
{
	CF_STATE_STOPPED,
	CF_STATE_PLAYING,
	CF_STATE_PAUSED,
};

class CFPlaylist:public os::ListView
{
      public:
	CFPlaylist( const os::Rect & cFrame );
	 ~CFPlaylist()
	{
	}
	virtual void MouseUp( const os::Point & cPos, uint32 nButtons, os::Message * pcData );
};

class CFWindow:public os::Window
{
      public:
	CFWindow( const os::Rect & cFrame, const os::String & cName, const os::String & cTitle, uint32 nFlags );
	 ~CFWindow();
	virtual void HandleMessage( os::Message * pcMessage );
	virtual bool OkToQuit();
	CFPlaylist *GetPlaylist()
	{
		return ( m_pcPlaylist );
	}
	os::Lcd * GetLCD()
	{
		return ( m_pcLCD );
	}
	os::SeekSlider * GetSlider()
	{
		return ( m_pcSlider );
	}
	os::Menu * GetMenuBar()
	{
		return ( m_pcMenuBar );
	}
	void SetState( uint8 nState )
	{
		m_nState = nState;
	}
      private:
	uint8 m_nState;

	os::LayoutView * m_pcRoot;
	os::HLayoutNode * m_pcControls;
	os::Menu* m_pcMenuBar;
	os::Lcd * m_pcLCD;
	os::ImageButton * m_pcPlay;
	os::ImageButton * m_pcPause;
	os::ImageButton * m_pcStop;
	os::SeekSlider * m_pcSlider;
	os::ImageButton * m_pcShowList;

	CFPlaylist *m_pcPlaylist;
	os::FileRequester* m_pcFileDialog;
};

class CFApp:public os::Application
{
      public:
	CFApp( const char *pzMimeType, os::String zFileName, bool bLoad );
	 ~CFApp();
	virtual void HandleMessage( os::Message * pcMessage );
	virtual bool OkToQuit();

	bool OpenList( os::String zFileName );
	void SaveList();

	void AddFile( os::String zFileName );
	int OpenFile( os::String zFileName, uint32 nTrack, uint32 nStream );
	void CloseCurrentFile();


	void PlayThread();
	void PlayNext();
      private:

	CFWindow * m_pcWin;
	uint8 m_nState;
	os::Rect m_cSavedFrame;

	os::String m_zListName;

	os::String m_zAudioFile;
	uint32 m_nAudioTrack;
	uint32 m_nAudioStream;

	os::MediaManager * m_pcManager;
	os::MediaInput * m_pcInput;
	bool m_bPacket;
	bool m_bStream;
	os::MediaFormat_s m_sAudioFormat;
	os::MediaCodec * m_pcAudioCodec;
	os::MediaOutput * m_pcAudioOutput;

	bool m_bPlayThread;
	thread_id m_hPlayThread;
	uint64 m_nLastPosition;
};

#endif

