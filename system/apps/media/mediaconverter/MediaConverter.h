
/*  Syllable Media Converter
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

#ifndef _MEDIACONVERTER_H_
#define _MEDIACONVERTER_H_

#include <gui/view.h>
#include <gui/window.h>
#include <gui/requesters.h>
#include <gui/dropdownmenu.h>
#include <gui/stringview.h>
#include <gui/button.h>
#include <gui/progressbar.h>
#include <gui/spinner.h>
#include <gui/textview.h>
#include <util/application.h>
#include <atheos/threads.h>
#include <atheos/kernel.h>
#include <media/inputselector.h>
#include <media/manager.h>
#include <media/input.h>
#include <media/output.h>
#include <media/codec.h>
#include <media/format.h>
#include <media/packet.h>

enum
{
	MC_IS_OPEN,
	MC_IS_CANCEL,

	MC_GUI_OUTPUT_CHANGE,
	MC_GUI_VIDEO_CHANGE,
	MC_GUI_AUDIO_CHANGE,
	MC_GUI_START,

	MC_START,
	MC_STOP
};


class MCWindow:public os::Window
{
      public:
	MCWindow( const os::Rect & cFrame, const std::string & cName, const std::string & cTitle, uint32 nFlags );
	 ~MCWindow();
	void FillLists();
	virtual void HandleMessage( os::Message * pcMessage );
	virtual bool OkToQuit();
	os::ProgressBar * GetProgress()
	{
		return ( m_pcProgress );
	}
	void SetTrackCount( int nTrackCount );
	void SetVideoAudio( os::MediaFormat_s sVideoFormat, os::MediaFormat_s sAudioFormat, bool bVideo, bool bAudio )
	{

		m_sVideoFormat = sVideoFormat;
		m_sAudioFormat = sAudioFormat;
		m_bVideo = bVideo;
		m_bAudio = bAudio;
		FillLists();
	}
      private:
	bool m_bEncode;
	bool m_bVideo;
	bool m_bAudio;

	int m_nTrackCount;
	os::MediaFormat_s m_sVideoFormat;
	os::MediaFormat_s m_sAudioFormat;
	os::TextView * m_pcFileInput;
	os::StringView * m_pcFileLabel;
	os::DropdownMenu * m_pcTrackList;
	os::StringView * m_pcTrackLabel;
	os::DropdownMenu * m_pcOutputList;
	os::StringView * m_pcOutputLabel;
	os::DropdownMenu * m_pcVideoList;
	os::StringView * m_pcVideoLabel;
	os::Spinner * m_pcVideoBitRate;
	os::StringView * m_pcVideoBitRateLabel;
	os::DropdownMenu * m_pcAudioList;
	os::StringView * m_pcAudioLabel;
	os::Spinner * m_pcAudioBitRate;
	os::StringView * m_pcAudioBitRateLabel;
	os::ProgressBar * m_pcProgress;
	os::Button * m_pcStartButton;
};

class MCApp:public os::Application
{
      public:
	MCApp( const char *pzMimeType, os::String zFileName, bool bLoad );
	 ~MCApp();
	virtual void HandleMessage( os::Message * pcMessage );
	virtual bool OkToQuit();
	virtual void Open( os::String zFile, os::String zInput );
	virtual void Encode();
      private:
	MCWindow * m_pcWin;
	os::MediaInputSelector * m_pcInputSelector;
	os::MediaManager * m_pcManager;
	os::MediaInput * m_pcInput;
	os::MediaCodec * m_pcInputVideo;
	os::MediaCodec * m_pcInputAudio;
	os::MediaCodec * m_pcOutputVideo;
	os::MediaCodec * m_pcOutputAudio;
	bool m_bVideo;
	bool m_bAudio;
	uint32 m_nVideoStream;
	uint32 m_nAudioStream;
	os::MediaFormat_s m_sVideoFormat;
	os::MediaFormat_s m_sAudioFormat;
	os::MediaOutput * m_pcOutput;
	os::String m_zFile;
	int32 m_nVideoOutputFormat;
	int32 m_nAudioOutputFormat;
	int32 m_nVideoBitRate;
	int32 m_nAudioBitRate;
	bool m_bEncode;
	thread_id m_hEncodeThread;
};

#endif
