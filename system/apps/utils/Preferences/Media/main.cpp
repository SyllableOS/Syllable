
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

#include <unistd.h>
#include <iostream>
#include <gui/desktop.h>
#include <gui/requesters.h>
#include "resources/Media.h"
#include "main.h"

using namespace os;

int main( int argc, char *argv[] )
{
	PrefsMediaApp *pcPrefsMediaApp;

	pcPrefsMediaApp = new PrefsMediaApp( argc, argv );
	pcPrefsMediaApp->Run();
}

PrefsMediaApp::PrefsMediaApp( int argc, char* argv[] ) : os::Application( "application/x-vnd-MediaPreferences" )
{
	SetCatalog("Media.catalog");

	m_bConsoleMode = false;
	m_bShowHelp = false;
	m_bSetDefaultDevices = false;

	if( argc > 1 )
	{
		if( !strcmp( argv[1], "--help" ) )
		{
			m_bConsoleMode = true;
			m_bShowHelp = true;
		}
		else if( !strcmp( argv[1], "--setupdefaults" ) )
		{
			m_bConsoleMode = true;
			m_bSetDefaultDevices = true;
		}
	}
}

thread_id PrefsMediaApp::Run( void )
{
	// Create media manager 
	m_pcManager = new os::MediaManager();

	if ( !m_pcManager->IsValid() )
	{
		std::cout << "Media prefs: Could not establish connection to media server!" << std::endl;

		if( !m_bConsoleMode ) {
			os::Alert* pcAlert = new os::Alert( MSG_ALERTWND_NOMEDIASERVER_TITLE, MSG_ALERTWND_NOMEDIASERVER_TEXT, Alert::ALERT_WARNING,0, MSG_ALERTWND_NOMEDIASERVER_OK.c_str(),NULL);
			pcAlert->Go();
		}

		Quit();
		return( -1 );
	}

	
	if( m_bConsoleMode )
	{
		/* Running in console mode. Perform the appropriate actions */
		if( m_bShowHelp )
		{
			std::cout << "Valid options are:\n\t--help\t\tshow this help\n\t--setupdefaults\tautomatically choose default input and output devices\n\nAny other options launch the media prefs window.\n";
		}
		else if( m_bSetDefaultDevices )
		{
			SetupDefaultDevices();
		}
		
		Quit();
		return( -1 );
	}
	else
	{
		/* Launch the Prefs window */
		
		// Get screen width and height to centre the window
		os::Desktop * pcDesktop = new os::Desktop();
		
		// Show main window
		pcWindow = new MainWindow( os::Rect( 0, 0, 600, 350 ) );
		pcWindow->CenterInScreen();
		pcWindow->Show();
		pcWindow->MakeFocus();
		return( Application::Run() );
	}
}

void PrefsMediaApp::SetupDefaultDevices()
{
	
	/* Setup default input device */
	os::MediaInput* pcInput;
	os::String cBestInput = "";
	uint32 nBestType = MEDIA_PHY_UNKNOWN;
	uint32 nIndex = 0;
	char* pzDescription = "unknown";
	
	while ( ( pcInput = os::MediaManager::GetInstance()->GetInput( nIndex ) ) != NULL )
	{
		uint32 nType = pcInput->GetPhysicalType();
		
//		printf( "Examining input: '%s' 0x%x\n", pcInput->GetIdentifier().c_str(), nType );
		
		if( pcInput->FileNameRequired() ) goto skip1;
		
		/* Use the first soundcard line-in, if present; otherwise use the first soundcard SPDIF-in, if present. */
		if( nType == MEDIA_PHY_SOUNDCARD_MIC_IN )
		{
			if( nBestType != MEDIA_PHY_SOUNDCARD_MIC_IN )
			{
				cBestInput = pcInput->GetIdentifier();
				nBestType = nType;
				pzDescription = "microphone input";
				pcInput->Release();
				break;		/* Found a microphone input, so stop searching */
			}
		}
		else if( nType == MEDIA_PHY_SOUNDCARD_LINE_IN )
		{
			if( nBestType != MEDIA_PHY_SOUNDCARD_MIC_IN && nBestType != MEDIA_PHY_SOUNDCARD_LINE_IN )
			{
				cBestInput = pcInput->GetIdentifier();
				nBestType = nType;
				pzDescription = "line in";
			}
		}
		else if( nType == MEDIA_PHY_SOUNDCARD_SPDIF_IN )
		{
			if( nBestType != MEDIA_PHY_SOUNDCARD_MIC_IN && nBestType != MEDIA_PHY_SOUNDCARD_LINE_IN && nBestType != MEDIA_PHY_SOUNDCARD_SPDIF_IN )
			{
				cBestInput = pcInput->GetIdentifier();
				nBestType = nType;
				pzDescription = "SPDIF input";
			}
		}
		
skip1:

		pcInput->Release();
		nIndex++;
	}
	if( cBestInput == "" )
	{
		std::cout << "No appropriate audio input devices found! Please set default input device manually.\n";
	}
	else
	{
		std::cout << "Found " << pzDescription << " '" << cBestInput.str() << "'.\n";
		os::MediaManager::GetInstance()->SetDefaultInput( cBestInput );
	}

	/* Setup default audio and video output device */
	os::MediaOutput* pcOutput;
	os::String cBestAudioOutput = "";
	os::String cBestVideoOutput = "";
	uint32 nBestAudioType = MEDIA_PHY_UNKNOWN;
	uint32 nBestVideoType = MEDIA_PHY_UNKNOWN;
	char* pzAudioDescription = "unknown";
	char* pzVideoDescription = "unknown";
	nIndex = 0;

	while ( ( pcOutput = os::MediaManager::GetInstance()->GetOutput( nIndex ) ) != NULL )
	{
		if( pcOutput->FileNameRequired() ) goto skip2;

		for( int i = 0; i < pcOutput->GetOutputFormatCount(); i++ )
		{
			os::MediaFormat_s sFormat = pcOutput->GetOutputFormat( i );
			if( sFormat.nType == os::MEDIA_TYPE_AUDIO )
			{
				uint32 nType = pcOutput->GetPhysicalType();
//				printf( "Examining audio output: '%s' 0x%x\n", pcOutput->GetIdentifier().c_str(), nType );
				if( nType == MEDIA_PHY_SOUNDCARD_LINE_OUT )
				{
					/* First choice is a line out */
					if( nBestAudioType != MEDIA_PHY_SOUNDCARD_LINE_OUT )
					{
						cBestAudioOutput = pcOutput->GetIdentifier();
						nBestAudioType = nType;
						pzAudioDescription = "line out";
					}
				}
				else if( nType == MEDIA_PHY_SOUNDCARD_SPDIF_OUT )
				{
					/* Second choice is an SPDIF out */
					if( nBestAudioType != MEDIA_PHY_SOUNDCARD_LINE_OUT && nBestAudioType != MEDIA_PHY_SOUNDCARD_SPDIF_OUT )
					{
						cBestAudioOutput = pcOutput->GetIdentifier();
						nBestAudioType = nType;
						pzAudioDescription = "SPDIF output";
					}
				}
				else if( cBestAudioOutput == "" && strncmp( pcOutput->GetIdentifier().c_str(), "Media Server - ", 15 ) == 0 )
//				else if( cBestAudioOutput == "" && pcOutput->GetIdentifier().substr(0,), "Media Server - ", sizeof("Media Server - ") ) == 0 )
				{
					/* Fall back on any output starting with "Media Server -" - a hack until the media server correctly identifies device types. */
					cBestAudioOutput = pcOutput->GetIdentifier();
					nBestAudioType = nType;
					pzAudioDescription = "generic input";
				}
				break;
			}
			
			if( sFormat.nType == os::MEDIA_TYPE_VIDEO )
			{
				uint32 nType = pcOutput->GetPhysicalType();
//				printf( "Examining video output: '%s' 0x%x\n", pcOutput->GetIdentifier().c_str(), nType );
				if( nType == MEDIA_PHY_SCREEN_OUT )
				{
					if( nBestVideoType != MEDIA_PHY_SCREEN_OUT )
					{
						cBestVideoOutput = pcOutput->GetIdentifier();
						nBestVideoType = nType;
						pzVideoDescription = "screen video output";
					}
					break;
				}
			}
		}
	
skip2:
		pcOutput->Release();
		nIndex++;
	}
	
	if( cBestAudioOutput == "" )
	{
		std::cout << "No appropriate audio output devices found! Please set default audio output device manually.\n";
	}
	else
	{
		std::cout << "Found " << pzAudioDescription << " '" << cBestAudioOutput.str() << "'.\n";
		os::MediaManager::GetInstance()->SetDefaultAudioOutput( cBestAudioOutput );
	}
	
	if( cBestVideoOutput == "" )
	{
		std::cout << "No appropriate video output devices found! Please set default video output device manually.\n";
	}
	else
	{
		std::cout << "Found " << pzVideoDescription << " '" << cBestVideoOutput.str() << "'.\n";
		os::MediaManager::GetInstance()->SetDefaultVideoOutput( cBestVideoOutput );
	}
}

bool PrefsMediaApp::OkToQuit()
{
	return ( true );
}

PrefsMediaApp::~PrefsMediaApp()
{
	m_pcManager->Put();
}

