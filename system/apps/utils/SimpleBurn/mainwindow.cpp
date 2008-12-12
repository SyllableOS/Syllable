#include "mainwindow.h"
#include "messages.h"
#include <unistd.h>
#include <iostream>


//----------------------------------------------------------------------------
// NAME:
// DESC: Convert the time (seconds) to Hours, Minutes, Seconds.
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------
static inline void secs_to_hms( uint64 nTime, int32 *nH, int32 *nM, int32 *nS )
{
	if ( nTime > 3599 )										// Convert to Hours.
	{
		*nH = ( nTime / 3600 );
		*nM = ( ( nTime - ( *nH * 3600 ) ) / 60 );
		*nS = nTime - ( *nH * 3600 ) - ( *nM * 60 );
	}
	else if ( nTime < 3600 && nTime > 59 )					// Convert the rest of to Minutes.
	{
		*nH = 0;
		*nM = ( nTime / 60 );
		*nS = nTime - ( *nM * 60 );
	}
	else													// ...and what's left must be seconds.
	{
		*nH = 0;
		*nM = 0;
		*nS = ( uint32 )nTime;
	}
}


//----------------------------------------------------------------------------
// NAME: Flemming H. Sørensen
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------
MainWindow::MainWindow() : os::Window( os::Rect( 0, 0, 600, 500 ), "main_wnd", "SimpleBurn 0.4.6" )
{
	os::LayoutView* pcView = new os::LayoutView( GetBounds(), "layout_view" );
	#include "mainwindowLayout.cpp"
	#include "burnLayout.cpp"
	#include "dataLayout.cpp"
	#include "audioLayout.cpp"
	#include "videoLayout.cpp"
	#include "backupLayout.cpp"
	pcView->SetRoot( m_pcRoot );
	AddChild( pcView );

	// Set an application icon for Dock.
	os::BitmapImage *pcImage = new os::BitmapImage();
	os::Resources cRes( get_image_id() );
	pcImage->Load( cRes.GetResourceStream( "icon24x24.png" ) );
	SetIcon(pcImage->LockBitmap());
	m_pcIcon = pcImage;   // Set an alertbox icon, too.

	// Show filesystem-warning, when the user try to burn a backup.
	bBackupWarning = true;

	// Set the default dir. for where to find Audio and Video files.
	cAudioSelect = getenv( "HOME" );
	cAudioSelect += os::String( "/Music/" );
	cVideoSelect = getenv( "HOME" );
	cVideoSelect += os::String( "/Movies/" );

	// Load settings, scan for devices, etc.
	m_pcMenuMediatypeCD->SetChecked( true );
	m_pcMenuSaveOnExit->SetChecked( true );
	DeviceScan();
	LoadSettings();
	SetMediaType();
}


void MainWindow::HandleMessage( os::Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC: Instead of burning the content to a disk, we "burn" it to an ISO.
		// NOTE:
		// SEE ALSO: CopyDisk() and DataDisk() and BackupDisk()
		//----------------------------------------------------------------------------
		case M_SAVE_REQUESTED:
		{
			// Set the path and filename, and merge it with the rest of the command-line.
			os::String cSavePath;
			pcMessage->FindString("file/path",&cSavePath);
			os::String cExec = cMakeISO1 + cSavePath + os::String(" ") + cMakeISO2;

			// Show the Splash Screen.
			Splash* pcWindow = new Splash(LoadImageFromResource("logo.png"),"Writing to ISO file",false,0.0);
			pcWindow->Go();

			// Write the ISO.
			int nError = 0;
			nError = system( cExec.c_str() );

			// Get the CheckSum and write MimeType + CheckSum attributes to the new ISO.
			if( nError == 0 )
			{
				// Get the CheckSum...
				FILE *fp;
				char zCmd[256];
				char zSum[31];
				sprintf( zCmd, "md5sum \"%s\"", cSavePath.c_str() );
				fp = popen( zCmd, "r" );
				fscanf( fp, "%s", zSum );
				fclose( fp );

				// Write the mimetype and CheckSum to the ISO-file.
				int nFile = open( cSavePath.c_str(), O_RDWR | O_NOTRAVERSE );
				write_attr( nFile, "os::MimeType", O_TRUNC, ATTR_TYPE_STRING, "application/x-iso-image", 0, strlen( "application/x-iso-image" ) );
				write_attr( nFile, "CheckSum::MD5", O_TRUNC, ATTR_TYPE_STRING, zSum, 0, strlen( zSum ) );
			  	close( nFile );
			}

			//  Remove the Splash Screen.
			pcWindow->Quit();

			// Success or Failure?
			if( nError != 0 )		// Failure.
			{
				os::Alert* pcAlert = new os::Alert( "SimpleBurn - Error", "Could not create the ISO...", m_pcIcon->LockBitmap(),0, "_Ok",NULL);
				pcAlert->Go( new os::Invoker( 0 ) );
			}
			else					// Success.
			{
				os::Alert* pcAlert = new os::Alert( "SimpleBurn", "The ISO has been created...", m_pcIcon->LockBitmap(),0, "_Ok",NULL);
				pcAlert->Go( new os::Invoker( 0 ) );
			}

			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC:
		// NOTE:
		// SEE ALSO:
		//----------------------------------------------------------------------------
		case M_LOAD_REQUESTED:
		{
			//----------------------------------------------------------------------------
			// NAME: Flemming H. Sørensen
			// DESC: This will burn an ISO, to a disk.
			// NOTE:
			// SEE ALSO: CopyDisk()
			//----------------------------------------------------------------------------
			if( cDiskAuthor == "copy" )
			{
				// Set the path and filename, merge it with the rest of the command-line, and tell us how many copies to burn.
				os::String cLoadPath;
				pcMessage->FindString("file/path",&cLoadPath);
				os::String cExec = cMakeISO1 + cLoadPath;
				int iNOC = atoi( cMakeISO2.c_str() );

				// Burn (all of) the disks...
				for( int counter = 1; counter <= iNOC; counter = counter + 1)
				{
					// Ask for a new disk, if we need one,
					if( iNOC > 1 )
					{
						char cCounter[99];
						char cNOC[99];
						sprintf( cCounter, "%i", counter );
						sprintf( cNOC, "%i", iNOC );
						os::String cMultiplyBurn = os::String("Ready to burn disk") + os::String( " " ) + cCounter + os::String( " " ) + os::String("of") + os::String( " " ) + cNOC + os::String( "\n" ) + os::String("Make sure there's an empty disk in the drive.");
						os::Alert* pcMultiplyBurn = new os::Alert( "SimpleBurn - Insert new CD", cMultiplyBurn , m_pcIcon->LockBitmap(), 0, "_Ok", NULL );
						pcMultiplyBurn->Go();
					}

					// Show the Splash Screen.
					char cSplashNote[128];
					sprintf( cSplashNote, "Writing ISO file to disk (copy %i of %i)", counter, iNOC );
					Splash* pcWindow = new Splash(LoadImageFromResource("logo.png"),cSplashNote,false,0.0);
					pcWindow->Go();

					// Burn the disk.
					int nError = 0;
					nError = system( cExec.c_str() );

					// Remove the Splash Screen.
					pcWindow->Quit();

					// Success or Failure?
					if( nError != 0 )		// Failure.
					{
						os::Alert* pcAlert = new os::Alert( "SimpleBurn - Error", "Could not write the ISO to the disk...", m_pcIcon->LockBitmap(),0, "_Ok",NULL);
						pcAlert->Go( new os::Invoker( 0 ) );
					}
					else					// Success.
					{
						os::Alert* pcAlert = new os::Alert( "SimpleBurn", "The ISO has been burned to the disk...", m_pcIcon->LockBitmap(),0, "_Ok",NULL);
						pcAlert->Go( new os::Invoker( 0 ) );
					}
				}

				break;
			}


			//----------------------------------------------------------------------------
			// NAME: Flemming H. Sørensen
			// DESC: Set the path to the dir we want to burn to our disk.
			// NOTE:
			// SEE ALSO: DataDisk()
			//----------------------------------------------------------------------------
			if( cDiskAuthor == "data" )
			{
				os::String cDataDir;
				pcMessage->FindString( "file/path", &cDataDir );
				m_pcDataAddPath->Set( cDataDir.c_str() );
				break;
			}


			//----------------------------------------------------------------------------
			// NAME: Flemming H. Sørensen
			// DESC: Check Audio-files, and add them to the ListView.
			// NOTE:
			// SEE ALSO: AudioDisk()
			//----------------------------------------------------------------------------
			if( cDiskAuthor == "audio" )
			{
				int32 nH, nM, nS;
				char cTrackTime[8];

				// Make sure the Mediamanager is loaded.
				if( MediaManager::GetInstance() == NULL )
				{
					pcManager = new MediaManager();
					pcManager->IsValid();
				}

				// Set the file(s) we're working with.
				int n = 0;
				os::String cFileName;
				while( pcMessage->FindString( "file/path", &cFileName, n ) == 0 )
				{
					cAudioSelect = cFileName;
					os::MediaInput* pcInput = pcManager->GetBestInput( cFileName );
					if ( pcInput != NULL && pcInput->Open( cFileName ) == 0 )
					{
						// It should be audio. If not, there's no reason to waste time on it...
						pcInput->SelectTrack( 0 );
						if ( pcInput->GetStreamFormat( 0 ).nType == os::MEDIA_TYPE_AUDIO )
						{
							bool bOK = true;

							// Find out what kind of disk we're trying to make
							int nType = m_pcAudioTypeSelection->GetSelection();
							os::String cType = os::String( m_pcAudioTypeSelection->GetItem(nType) );

							// ...it could be an Audio CD. In that case we will have to check if the Audio files are Ok for this type.
							if( cType == "Audio CD" )
							{
								std::cout << "Audio - Codec: " << pcInput->GetStreamFormat( 0 ).zName.c_str() << std::endl;
								if( strstr( pcInput->GetStreamFormat( 0 ).zName.c_str(), "pcm" ) == NULL )
									bOK = false;
								std::cout << "Audio - SampleRate: " << pcInput->GetStreamFormat( 0 ).nSampleRate << std::endl;
								if( pcInput->GetStreamFormat( 0 ).nSampleRate != 44100 )
									bOK = false;
								std::cout << "Audio - Channels: " << pcInput->GetStreamFormat( 0 ).nChannels << std::endl;
								if( pcInput->GetStreamFormat( 0 ).nChannels != 2 )
									bOK = false;
/*								std::cout << "Audio - CBR/VBR: " << pcInput->GetStreamFormat( 0 ).bVBR << std::endl;
								if( pcInput->GetStreamFormat( 0 ).bVBR != 0 )
									bOK = false;
*/
								// If there was something wrong with the audio-file, we'll let the user know
								if( bOK == false )
								{
									os::Alert* pcAlert = new os::Alert( "SimpleBurn - Audio CD", "This file is not valid for Audio CD's.\nIt should be:\n16bit PCM\nSample-rate: 44100 Hz\nChannel: Stereo\nContant Bitrate", m_pcIcon->LockBitmap(),0, "_Ok",NULL);
									pcAlert->Go( new os::Invoker( 0 ) );
								}
							}

							// If everything is Ok, we add it to the ListView.
							if( bOK == true )
							{
								// Convert time.
								secs_to_hms( pcInput->GetLength(), &nH, &nM, &nS );
								sprintf( cTrackTime, "%.2i:%.2i:%.2i", nH, nM, nS );

								// Add to ListView.
								ListViewStringRow *row = new ListViewStringRow();
								row->AppendString( cTrackTime );
								row->AppendString( cFileName );
								m_pcAudioList->InsertRow( row, true );

								// Now that we've added a file, we should make sure the user doesn't change the disk-type.
								m_pcAudioTypeSelection->SetEnable( false );
							}
						}
					}
					n++;
				}

				// Update the total time.
				UpdateTime();

				break;
			}


			//----------------------------------------------------------------------------
			// NAME: Flemming H. Sørensen
			// DESC: Check Video-files, and add them to the ListView.
			// NOTE:
			// SEE ALSO: VideoDisk()
			//----------------------------------------------------------------------------
			if( cDiskAuthor == "video" )
			{
				int32 nH, nM, nS;
				char cTrackTime[8];

				// Make sure the Mediamanager is loaded.
				if( MediaManager::GetInstance() == NULL )
				{
					pcManager = new MediaManager();
					pcManager->IsValid();
				}

				// Set the file(s) we're working with.
				int n = 0;
				os::String cFileName;
				while( pcMessage->FindString( "file/path", &cFileName, n ) == 0 )
				{
					cVideoSelect = cFileName;
					os::MediaInput* pcInput = pcManager->GetBestInput( cFileName );
					if ( pcInput != NULL && pcInput->Open( cFileName ) == 0 )
					{
						char zFramerate[10];
						bool bOK = true;

						// Find out what kind of disk we're trying to make
						int nType = m_pcVideoTypeSelection->GetSelection();
						os::String cType = os::String( m_pcVideoTypeSelection->GetItem(nType) );

						if( cType == "Video CD - PAL" )
						{
							for (uint32 j=0; j<pcInput->GetStreamCount(); j++)
							{
								pcInput->SelectTrack( j );

								if ( pcInput->GetStreamFormat( j ).nType == os::MEDIA_TYPE_AUDIO )
								{
									std::cout << "Audio - Codec: " << pcInput->GetStreamFormat( j ).zName.c_str() << std::endl;
									if( strstr( pcInput->GetStreamFormat( j ).zName.c_str(), "mp2" ) == NULL )
										bOK = false;
									std::cout << "Audio - SampleRate: " << pcInput->GetStreamFormat( j ).nSampleRate << std::endl;
									if( pcInput->GetStreamFormat( j ).nSampleRate != 44100 )
										bOK = false;
									std::cout << "Audio - BitRate: " << pcInput->GetStreamFormat( j ).nBitRate << std::endl;
									if( pcInput->GetStreamFormat( j ).nBitRate != 224000 )
										bOK = false;
									std::cout << "Audio - Channels: " << pcInput->GetStreamFormat( j ).nChannels << std::endl;
									if( pcInput->GetStreamFormat( j ).nChannels != 2 )
										bOK = false;
/*									std::cout << "Audio - CBR/VBR: " << pcInput->GetStreamFormat( j ).bVBR << std::endl;
									if( pcInput->GetStreamFormat( j ).bVBR == true )
										bOK = false;
*/								}

								if ( pcInput->GetStreamFormat( j ).nType == os::MEDIA_TYPE_VIDEO )
								{
									std::cout << "Video - Codec: " << pcInput->GetStreamFormat( j ).zName.c_str() << std::endl;
									if( strstr( pcInput->GetStreamFormat( j ).zName.c_str(), "mpeg1video" ) == NULL )
										bOK = false;
									std::cout << "Video - Hres: " << pcInput->GetStreamFormat( j ).nWidth << std::endl;
									if( pcInput->GetStreamFormat( j ).nWidth != 352 )
										bOK = false;
									std::cout << "Video - Vres: " << pcInput->GetStreamFormat( j ).nHeight << std::endl;
									if( pcInput->GetStreamFormat( j ).nHeight != 288 )
										bOK = false;
									sprintf( zFramerate, "%.3f", pcInput->GetStreamFormat( j ).vFrameRate );
									std::cout << "Video - FrameRate: " << zFramerate << std::endl;
									if( os::String(zFramerate) != os::String("25.000") )
										bOK = false;
									std::cout << "Video - BitRate: " << pcInput->GetStreamFormat( j ).nBitRate << std::endl;
									if( pcInput->GetStreamFormat( j ).nBitRate != 1150000 )
										bOK = false;
/*									std::cout << "Video - CBR/VBR: " << pcInput->GetStreamFormat( j ).bVBR << std::endl;
									if( pcInput->GetStreamFormat( j ).bVBR == true )
										bOK = false;
*/								}
							}
						}


						if( cType == "Video CD - NTSC" )
						{
							for (uint32 j=0; j<pcInput->GetStreamCount(); j++)
							{
								pcInput->SelectTrack( j );

								if ( pcInput->GetStreamFormat( j ).nType == os::MEDIA_TYPE_AUDIO )
								{
									std::cout << "Audio - Codec: " << pcInput->GetStreamFormat( j ).zName.c_str() << std::endl;
									if( strstr( pcInput->GetStreamFormat( j ).zName.c_str(), "mp2" ) == NULL )
										bOK = false;
									std::cout << "Audio - SampleRate: " << pcInput->GetStreamFormat( j ).nSampleRate << std::endl;
									if( pcInput->GetStreamFormat( j ).nSampleRate != 44100 )
										bOK = false;
									std::cout << "Audio - BitRate: " << pcInput->GetStreamFormat( j ).nBitRate << std::endl;
									if( pcInput->GetStreamFormat( j ).nBitRate != 224000 )
										bOK = false;
									std::cout << "Audio - Channels: " << pcInput->GetStreamFormat( j ).nChannels << std::endl;
									if( pcInput->GetStreamFormat( j ).nChannels != 2 )
										bOK = false;
/*									std::cout << "Audio - CBR/VBR: " << pcInput->GetStreamFormat( j ).bVBR << std::endl;
									if( pcInput->GetStreamFormat( j ).bVBR == true )
										bOK = false;
*/								}

								if ( pcInput->GetStreamFormat( j ).nType == os::MEDIA_TYPE_VIDEO )
								{
									std::cout << "Video - Codec: " << pcInput->GetStreamFormat( j ).zName.c_str() << std::endl;
									if( strstr( pcInput->GetStreamFormat( j ).zName.c_str(), "mpeg1video" ) == NULL )
										bOK = false;
									std::cout << "Video - Hres: " << pcInput->GetStreamFormat( j ).nWidth << std::endl;
									if( pcInput->GetStreamFormat( j ).nWidth != 352 )
										bOK = false;
									std::cout << "Video - Vres: " << pcInput->GetStreamFormat( j ).nHeight << std::endl;
									if( pcInput->GetStreamFormat( j ).nHeight != 240 )
										bOK = false;
									sprintf( zFramerate, "%.3f", pcInput->GetStreamFormat( j ).vFrameRate );
									std::cout << "Video - FrameRate: " << zFramerate << std::endl;
									if( os::String(zFramerate) != os::String("29.970") && os::String(zFramerate) != os::String("23.976") )
										bOK = false;
									std::cout << "Video - BitRate: " << pcInput->GetStreamFormat( j ).nBitRate << std::endl;
									if( pcInput->GetStreamFormat( j ).nBitRate != 1150000 )
										bOK = false;
/*									std::cout << "Video - CBR/VBR: " << pcInput->GetStreamFormat( j ).bVBR << std::endl;
									if( pcInput->GetStreamFormat( j ).bVBR == true )
										bOK = false;
*/								}
							}
						}


						if( cType == "S-Video CD - PAL" )
						{
							for (uint32 j=0; j<pcInput->GetStreamCount(); j++)
							{
								pcInput->SelectTrack( j );

								if ( pcInput->GetStreamFormat( j ).nType == os::MEDIA_TYPE_AUDIO )
								{
									std::cout << "Audio - Codec: " << pcInput->GetStreamFormat( j ).zName.c_str() << std::endl;
									if( strstr( pcInput->GetStreamFormat( j ).zName.c_str(), "mp2" ) == NULL )
										bOK = false;
									std::cout << "Audio - SampleRate: " << pcInput->GetStreamFormat( j ).nSampleRate << std::endl;
									if( pcInput->GetStreamFormat( j ).nSampleRate != 44100 )
										bOK = false;
									std::cout << "Audio - BitRate: " << pcInput->GetStreamFormat( j ).nBitRate << std::endl;
									if( pcInput->GetStreamFormat( j ).nBitRate < 32000 || pcInput->GetStreamFormat( j ).nBitRate > 384000 )
										bOK = false;
									std::cout << "Audio - Channels: " << pcInput->GetStreamFormat( j ).nChannels << std::endl;
									if( pcInput->GetStreamFormat( j ).nChannels != 2 )
										bOK = false;
/*									std::cout << "Audio - CBR/VBR: " << pcInput->GetStreamFormat( j ).bVBR << std::endl;
									if( pcInput->GetStreamFormat( j ).bVBR == true )
										bOK = false;
*/								}

								if ( pcInput->GetStreamFormat( j ).nType == os::MEDIA_TYPE_VIDEO )
								{
									std::cout << "Video - Codec: " << pcInput->GetStreamFormat( j ).zName.c_str() << std::endl;
									if( strstr( pcInput->GetStreamFormat( j ).zName.c_str(), "mpeg2video" ) == NULL )
										bOK = false;
									std::cout << "Video - Hres: " << pcInput->GetStreamFormat( j ).nWidth << std::endl;
									if( pcInput->GetStreamFormat( j ).nWidth != 480 )
										bOK = false;
									std::cout << "Video - Vres: " << pcInput->GetStreamFormat( j ).nHeight << std::endl;
									if( pcInput->GetStreamFormat( j ).nHeight != 576 )
										bOK = false;
									sprintf( zFramerate, "%.3f", pcInput->GetStreamFormat( j ).vFrameRate );
									std::cout << "Video - FrameRate: " << zFramerate << std::endl;
									if( os::String(zFramerate) != os::String("25.000") )
										bOK = false;
									std::cout << "Video - BitRate: " << pcInput->GetStreamFormat( j ).nBitRate << std::endl;
									if( pcInput->GetStreamFormat( j ).nBitRate < 2500000 || pcInput->GetStreamFormat( j ).nBitRate > 2600000 )
										bOK = false;
								}
							}
						}


						if( cType == "S-Video CD - NTSC" )
						{
							for (uint32 j=0; j<pcInput->GetStreamCount(); j++)
							{
								pcInput->SelectTrack( j );

								if ( pcInput->GetStreamFormat( j ).nType == os::MEDIA_TYPE_AUDIO )
								{
									std::cout << "Audio - Codec: " << pcInput->GetStreamFormat( j ).zName.c_str() << std::endl;
									if( strstr( pcInput->GetStreamFormat( j ).zName.c_str(), "mp2" ) == NULL )
										bOK = false;
									std::cout << "Audio - SampleRate: " << pcInput->GetStreamFormat( j ).nSampleRate << std::endl;
									if( pcInput->GetStreamFormat( j ).nSampleRate != 44100 )
										bOK = false;
									std::cout << "Audio - BitRate: " << pcInput->GetStreamFormat( j ).nBitRate << std::endl;
									if( pcInput->GetStreamFormat( j ).nBitRate < 32000 || pcInput->GetStreamFormat( j ).nBitRate > 384000 )
										bOK = false;
									std::cout << "Audio - Channels: " << pcInput->GetStreamFormat( j ).nChannels << std::endl;
									if( pcInput->GetStreamFormat( j ).nChannels != 2 )
										bOK = false;
/*									std::cout << "Audio - CBR/VBR: " << pcInput->GetStreamFormat( j ).bVBR << std::endl;
									if( pcInput->GetStreamFormat( j ).bVBR == true )
										bOK = false;
*/								}

								if ( pcInput->GetStreamFormat( j ).nType == os::MEDIA_TYPE_VIDEO )
								{
									std::cout << "Video - Codec: " << pcInput->GetStreamFormat( j ).zName.c_str() << std::endl;
									if( strstr( pcInput->GetStreamFormat( j ).zName.c_str(), "mpeg2video" ) == NULL )
										bOK = false;
									std::cout << "Video - Hres: " << pcInput->GetStreamFormat( j ).nWidth << std::endl;
									if( pcInput->GetStreamFormat( j ).nWidth != 480 )
										bOK = false;
									std::cout << "Video - Vres: " << pcInput->GetStreamFormat( j ).nHeight << std::endl;
									if( pcInput->GetStreamFormat( j ).nHeight != 480 )
										bOK = false;
									sprintf( zFramerate, "%.3f", pcInput->GetStreamFormat( j ).vFrameRate );
									std::cout << "Video - FrameRate: " << zFramerate << std::endl;
									if( os::String(zFramerate) != os::String("29.970") && os::String(zFramerate) != os::String("23.976") )
										bOK = false;
									std::cout << "Video - BitRate: " << pcInput->GetStreamFormat( j ).nBitRate << std::endl;
									if( pcInput->GetStreamFormat( j ).nBitRate < 2500000 || pcInput->GetStreamFormat( j ).nBitRate > 2600000 )
										bOK = false;
								}
							}
						}


						if( cType == "CVD - PAL" )
						{
							for (uint32 j=0; j<pcInput->GetStreamCount(); j++)
							{
								pcInput->SelectTrack( j );

								if ( pcInput->GetStreamFormat( j ).nType == os::MEDIA_TYPE_AUDIO )
								{
									std::cout << "Audio - Codec: " << pcInput->GetStreamFormat( j ).zName.c_str() << std::endl;
									if( strstr( pcInput->GetStreamFormat( j ).zName.c_str(), "mp2" ) == NULL )
										bOK = false;
									std::cout << "Audio - SampleRate: " << pcInput->GetStreamFormat( j ).nSampleRate << std::endl;
									if( pcInput->GetStreamFormat( j ).nSampleRate != 44100 )
										bOK = false;
									std::cout << "Audio - BitRate: " << pcInput->GetStreamFormat( j ).nBitRate << std::endl;
									if( pcInput->GetStreamFormat( j ).nBitRate < 32000 || pcInput->GetStreamFormat( j ).nBitRate > 384000 )
										bOK = false;
									std::cout << "Audio - Channels: " << pcInput->GetStreamFormat( j ).nChannels << std::endl;
									if( pcInput->GetStreamFormat( j ).nChannels != 2 )
										bOK = false;
/*									std::cout << "Audio - CBR/VBR: " << pcInput->GetStreamFormat( j ).bVBR << std::endl;
									if( pcInput->GetStreamFormat( j ).bVBR == true )
										bOK = false;
*/								}

								if ( pcInput->GetStreamFormat( j ).nType == os::MEDIA_TYPE_VIDEO )
								{
									std::cout << "Video - Codec: " << pcInput->GetStreamFormat( j ).zName.c_str() << std::endl;
									if( strstr( pcInput->GetStreamFormat( j ).zName.c_str(), "mpeg2video" ) == NULL )
										bOK = false;
									std::cout << "Video - Hres: " << pcInput->GetStreamFormat( j ).nWidth << std::endl;
									if( pcInput->GetStreamFormat( j ).nWidth != 352 )
										bOK = false;
									std::cout << "Video - Vres: " << pcInput->GetStreamFormat( j ).nHeight << std::endl;
									if( pcInput->GetStreamFormat( j ).nHeight != 576 )
										bOK = false;
									sprintf( zFramerate, "%.3f", pcInput->GetStreamFormat( j ).vFrameRate );
									std::cout << "Video - FrameRate: " << zFramerate << std::endl;
									if( os::String(zFramerate) != os::String("25.000") )
										bOK = false;
									std::cout << "Video - BitRate: " << pcInput->GetStreamFormat( j ).nBitRate << std::endl;
									if( pcInput->GetStreamFormat( j ).nBitRate < 2500000 || pcInput->GetStreamFormat( j ).nBitRate > 2600000 )
										bOK = false;
								}
							}
						}


						if( cType == "CVD - NTSC" )
						{
							for (uint32 j=0; j<pcInput->GetStreamCount(); j++)
							{
								pcInput->SelectTrack( j );

								if ( pcInput->GetStreamFormat( j ).nType == os::MEDIA_TYPE_AUDIO )
								{
									std::cout << "Audio - Codec: " << pcInput->GetStreamFormat( j ).zName.c_str() << std::endl;
									if( strstr( pcInput->GetStreamFormat( j ).zName.c_str(), "mp2" ) == NULL )
										bOK = false;
									std::cout << "Audio - SampleRate: " << pcInput->GetStreamFormat( j ).nSampleRate << std::endl;
									if( pcInput->GetStreamFormat( j ).nSampleRate != 44100 )
										bOK = false;
									std::cout << "Audio - BitRate: " << pcInput->GetStreamFormat( j ).nBitRate << std::endl;
									if( pcInput->GetStreamFormat( j ).nBitRate < 32000 || pcInput->GetStreamFormat( j ).nBitRate > 384000 )
										bOK = false;
									std::cout << "Audio - Channels: " << pcInput->GetStreamFormat( j ).nChannels << std::endl;
									if( pcInput->GetStreamFormat( j ).nChannels != 2 )
										bOK = false;
/*									std::cout << "Audio - CBR/VBR: " << pcInput->GetStreamFormat( j ).bVBR << std::endl;
									if( pcInput->GetStreamFormat( j ).bVBR == true )
										bOK = false;
*/								}

								if ( pcInput->GetStreamFormat( j ).nType == os::MEDIA_TYPE_VIDEO )
								{
									std::cout << "Video - Codec: " << pcInput->GetStreamFormat( j ).zName.c_str() << std::endl;
									if( strstr( pcInput->GetStreamFormat( j ).zName.c_str(), "mpeg2video" ) == NULL )
										bOK = false;
									std::cout << "Video - Hres: " << pcInput->GetStreamFormat( j ).nHeight << std::endl;
									if( pcInput->GetStreamFormat( j ).nWidth != 352 )
										bOK = false;
									std::cout << "Video - Vres: " << pcInput->GetStreamFormat( j ).nWidth << std::endl;
									if( pcInput->GetStreamFormat( j ).nHeight != 480 )
										bOK = false;
									sprintf( zFramerate, "%.3f", pcInput->GetStreamFormat( j ).vFrameRate );
									std::cout << "Video - FrameRate: " << zFramerate << std::endl;
									if( os::String(zFramerate) != os::String("29.970") && os::String(zFramerate) != os::String("23.976") )
										bOK = false;
									std::cout << "Video - BitRate: " << pcInput->GetStreamFormat( j ).nBitRate << std::endl;
									if( pcInput->GetStreamFormat( j ).nBitRate < 2500000 || pcInput->GetStreamFormat( j ).nBitRate > 2600000 )
										bOK = false;
								}
							}
						}


						// If everything is Ok, we add it to the ListView.
						if( bOK == true )
						{
							// Convert time.
							secs_to_hms( pcInput->GetLength(), &nH, &nM, &nS );
							sprintf( cTrackTime, "%.2i:%.2i:%.2i", nH, nM, nS );

							// Add to ListView.
							ListViewStringRow *row = new ListViewStringRow();
							row->AppendString( cTrackTime );
							row->AppendString( cFileName );
							m_pcVideoList->InsertRow( row, true );

							// Now that we've added a file, we should make sure the user doesn't change the disk-type.
							m_pcVideoTypeSelection->SetEnable( false );
						}
					}
					n++;
				}

				// Update the total time.
				UpdateTime();

				break;

			}
			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC: Send people to a website.
		// NOTE:
		// SEE ALSO:
		//----------------------------------------------------------------------------
		case M_MENU_WEB:
		case M_MENU_HELP:
		{
			if( fork() == 0 )
			{
				set_thread_priority( -1, 0 );
				execlp( "/Applications/Webster/Webster","/Applications/Webster/Webster","http://www.syllable.org/",NULL );
				exit(0);
			}
			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC: Show an About Box.
		// NOTE:
		// SEE ALSO:
		//----------------------------------------------------------------------------
		case M_MENU_ABOUT:
		{
			os::String cText = String("Version:") + os::String(" 0.4.1\n\n") + String("Author:") + os::String(" Flemming H. Sørensen (BurningShadow)\n\n") + String("Desc:") + String(" A CD burning program.") + os::String("\n\n") + String("Licens: ") + String("GNU GPL v2.\n");
			os::Alert* pcAlert = new Alert( "About SimpleBurn", cText, m_pcIcon->LockBitmap(),0, "_Ok",NULL);
			pcAlert->Go( new os::Invoker( 0 ) );
			pcAlert->MakeFocus();
			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC: Save Settings.
		// NOTE:
		// SEE ALSO: SaveSettings()
		//----------------------------------------------------------------------------
		case M_MENU_SETTINGS_SAVENOW:
		{
			SaveSettings();
			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC: Set the media Type to "CD".
		// NOTE:
		// SEE ALSO: SetMediaType()
		//----------------------------------------------------------------------------
		case M_MENU_MEDIA_CD:
		{
			m_pcMenuMediatypeCD->SetChecked(true);
			m_pcMenuMediatypeDVD->SetChecked(false);
			m_pcMenuMediatypeHDDVD->SetChecked(false);
			m_pcMenuMediatypeBLURAY->SetChecked(false);
			SetMediaType();
			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC: Set the media Type to "DVD".
		// NOTE:
		// SEE ALSO: SetMediaType()
		//----------------------------------------------------------------------------
		case M_MENU_MEDIA_DVD:
		{
			m_pcMenuMediatypeCD->SetChecked(false);
			m_pcMenuMediatypeDVD->SetChecked(true);
			m_pcMenuMediatypeHDDVD->SetChecked(false);
			m_pcMenuMediatypeBLURAY->SetChecked(false);
			SetMediaType();
			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC: Set the media Type to "HD-DVD".
		// NOTE:
		// SEE ALSO: SetMediaType()
		//----------------------------------------------------------------------------
		case M_MENU_MEDIA_HDDVD:
		{
			m_pcMenuMediatypeCD->SetChecked(false);
			m_pcMenuMediatypeDVD->SetChecked(false);
			m_pcMenuMediatypeHDDVD->SetChecked(true);
			m_pcMenuMediatypeBLURAY->SetChecked(false);
			SetMediaType();
			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC: Set the media Type to "Blu-ray".
		// NOTE:
		// SEE ALSO: SetMediaType()
		//----------------------------------------------------------------------------
		case M_MENU_MEDIA_BLURAY:
		{
			m_pcMenuMediatypeCD->SetChecked(false);
			m_pcMenuMediatypeDVD->SetChecked(false);
			m_pcMenuMediatypeHDDVD->SetChecked(false);
			m_pcMenuMediatypeBLURAY->SetChecked(true);
			SetMediaType();
			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC: Fast Erase of a disk.
		// NOTE:
		// SEE ALSO: erasewindow.cpp
		//----------------------------------------------------------------------------
		case M_MENU_ERASE_FAST:
		{
			pcEraseWin = new EraseWindow( "fast" );
			pcEraseWin->CenterInScreen();
			pcEraseWin->Show();
			pcEraseWin->MakeFocus();
			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC: Full Erase of a disk.
		// NOTE:
		// SEE ALSO: erasewindow.cpp
		//----------------------------------------------------------------------------
		case M_MENU_ERASE_FULL:
		{
			pcEraseWin = new EraseWindow( "full" );
			pcEraseWin->CenterInScreen();
			pcEraseWin->Show();
			pcEraseWin->MakeFocus();
			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC: Show the Burn/Copy/Erase Disk view
		// NOTE:
		// SEE ALSO: burnLayout.cpp
		//----------------------------------------------------------------------------
		case M_SIDEMENU_BURN:
		{
			pcMainView->SetRoot( m_pcBurn );
			cDiskAuthor = os::String( "copy" );
			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC: Show the Data Disk view
		// NOTE:
		// SEE ALSO: dataLayout.cpp
		//----------------------------------------------------------------------------
		case M_SIDEMENU_DATA:
		{
			pcMainView->SetRoot( m_pcData );
			cDiskAuthor = os::String( "data" );
			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC: Show the Audio Disk view
		// NOTE:
		// SEE ALSO: audioLayout.cpp
		//----------------------------------------------------------------------------
		case M_SIDEMENU_AUDIO:
		{
			pcMainView->SetRoot( m_pcAudio );
			cDiskAuthor = os::String( "audio" );
			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC: Show the Video Disk view
		// NOTE:
		// SEE ALSO: videoLayout.cpp
		//----------------------------------------------------------------------------
		case M_SIDEMENU_VIDEO:
		{
			pcMainView->SetRoot( m_pcVideo );
			cDiskAuthor = os::String( "video" );
			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC: Show the Backup Disk view
		// NOTE:
		// SEE ALSO: backupLayout.cpp
		//----------------------------------------------------------------------------
		case M_SIDEMENU_BACKUP:
		{
			// Warn the user, that ISO9660+RockRidge sucks.
			if( bBackupWarning == true )
			{
				os::Alert* pcAlert = new Alert( "SimpleBurn - Backup", "The filesystem on the CD does not\nsupport Syllable's metadata.\n", m_pcIcon->LockBitmap(),0, "_Ok",NULL);
				pcAlert->Go( new os::Invoker( 0 ) );
				pcAlert->MakeFocus();
				bBackupWarning = false;
			}

			pcMainView->SetRoot( m_pcBackup );
			cDiskAuthor = os::String( "backup" );
			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC:
		// NOTE:
		// SEE ALSO: CopyDisk()
		//----------------------------------------------------------------------------
		case M_BURN_BURN:
		{
			CopyDisk();
			break;
		}

		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC:
		// NOTE:
		// SEE ALSO: EraseDisk()
		//----------------------------------------------------------------------------
		case M_BURN_ERASE:
		{
			EraseDisk();
			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC: Open a FileRequester, so the user can select a dir, to burn on the disk.
		// NOTE:
		// SEE ALSO: DataDisk()
		//----------------------------------------------------------------------------
		case M_DATA_ADD:
		{
			m_pcFileRequesterAdd = new os::FileRequester( os::FileRequester::LOAD_REQ, new os::Messenger( this ), "", os::FileRequester::NODE_DIR, true );
			m_pcFileRequesterAdd->Show();
			m_pcFileRequesterAdd->MakeFocus();
			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC:
		// NOTE:
		// SEE ALSO: DataDisk()
		//----------------------------------------------------------------------------
		case M_DATA_BURN:
		{
			DataDisk();
			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC: Open a FileRequester, so the user can select some audio-files.
		// NOTE:
		// SEE ALSO: AudioDisk()
		//----------------------------------------------------------------------------
		case M_AUDIO_ADD:
		{
			if( m_pcAudioTypeSelection->GetSelection() != 0 )		// Open the FileRequester, unless the user hasn't selected a disk type.
			{
				m_pcFileRequesterAdd = new os::FileRequester( os::FileRequester::LOAD_REQ, new os::Messenger( this ), cAudioSelect, os::FileRequester::NODE_FILE, true );
				m_pcFileRequesterAdd->Show();
				m_pcFileRequesterAdd->MakeFocus();
			} else {												// ...in that case, we tell the user to select a disk type.
				os::Alert* pcAlert = new os::Alert( "SimpleBurn - Error", "You need to select what kind of disk you want to make.", m_pcIcon->LockBitmap(),0, "_Ok",NULL);
				pcAlert->Go( new os::Invoker( 0 ) );
			}
			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC: Remove an Audio-file from the list.
		// NOTE:
		// SEE ALSO: AudioDisk()
		//----------------------------------------------------------------------------
		case M_AUDIO_REMOVE:
		{
			uint16 nSelected = m_pcAudioList->GetLastSelected();
			if( nSelected < m_pcAudioList->GetRowCount() && m_pcAudioList->GetRowCount() > 0 )
			{
				m_pcAudioList->RemoveRow( nSelected );
				UpdateTime();
			}

			if( m_pcAudioList->GetRowCount() == 0 )			// If there's nothing left in the list, we will let the user choose the disk-type again.
			{
				m_pcAudioTypeSelection->SetEnable( true );
			}

			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC: Move an Audio-file up.
		// NOTE:
		// SEE ALSO: AudioDisk()
		//----------------------------------------------------------------------------
		case M_AUDIO_UP:
		{
			int16 nSelected = m_pcAudioList->GetLastSelected();
			if( nSelected > 0 )
			{
				// Get the Line...
				ListViewStringRow* pcRow = static_cast<os::ListViewStringRow*>( m_pcAudioList->GetRow( nSelected ) );
				os::String cFileTime = pcRow->GetString(0);
				os::String cFileName = pcRow->GetString(1);
				m_pcAudioList->RemoveRow( nSelected );

				// ...and put it somewhere else
				ListViewStringRow *row = new ListViewStringRow();
				row->AppendString( cFileTime );
				row->AppendString( cFileName );
				m_pcAudioList->InsertRow( nSelected-1, row );
				m_pcAudioList->Select( nSelected-1 );
			}
			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC: Move an Audio-file down.
		// NOTE:
		// SEE ALSO: AudioDisk()
		//----------------------------------------------------------------------------
		case M_AUDIO_DOWN:
		{
			uint16 nSelected = m_pcAudioList->GetLastSelected();
			if( nSelected < m_pcAudioList->GetRowCount()-1 && m_pcAudioList->GetRowCount() > 1 )
			{
				// Get the Line...
				ListViewStringRow* pcRow = static_cast<os::ListViewStringRow*>( m_pcAudioList->GetRow( nSelected ) );
				os::String cFileTime = pcRow->GetString(0);
				os::String cFileName = pcRow->GetString(1);
				m_pcAudioList->RemoveRow( nSelected );

				// ...and put it somewhere else
				ListViewStringRow *row = new ListViewStringRow();
				row->AppendString( cFileTime );
				row->AppendString( cFileName );
				m_pcAudioList->InsertRow( nSelected+1, row );
				m_pcAudioList->Select( nSelected+1 );
			}
			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC:
		// NOTE:
		// SEE ALSO: AudioDisk()
		//----------------------------------------------------------------------------
		case M_AUDIO_BURN:
		{
			AudioDisk();
			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC: Open a FileRequester, so the user can select some video-files.
		// NOTE:
		// SEE ALSO: VideoDisk()
		//----------------------------------------------------------------------------
		case M_VIDEO_ADD:
		{
			if( m_pcVideoTypeSelection->GetSelection() != 0 )		// Open the FileRequester, unless the user hasn't selected a disk type.
			{
				m_pcFileRequesterAdd = new os::FileRequester( os::FileRequester::LOAD_REQ, new os::Messenger( this ), cVideoSelect, os::FileRequester::NODE_FILE, true );
				m_pcFileRequesterAdd->Show();
				m_pcFileRequesterAdd->MakeFocus();
			} else {												// ...in that case, we tell the user to select a disk type.
				os::Alert* pcAlert = new os::Alert( "SimpleBurn - Error", "You need to select what kind of disk you want to make.", m_pcIcon->LockBitmap(),0, "_Ok",NULL);
				pcAlert->Go( new os::Invoker( 0 ) );
			}
			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC: Remove a Video-file from the list.
		// NOTE:
		// SEE ALSO: VideoDisk()
		//----------------------------------------------------------------------------
		case M_VIDEO_REMOVE:
		{
			uint16 nSelected = m_pcVideoList->GetLastSelected();
			if( nSelected < m_pcVideoList->GetRowCount() && m_pcVideoList->GetRowCount() > 0 )
			{
				m_pcVideoList->RemoveRow( nSelected );
				UpdateTime();
			}

			if( m_pcVideoList->GetRowCount() == 0 )			// If there's nothing left in the list, we will let the user choose the disk-type again.
			{
				m_pcVideoTypeSelection->SetEnable( true );
			}

			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC: Move an Video-file up.
		// NOTE:
		// SEE ALSO: VideoDisk()
		//----------------------------------------------------------------------------
		case M_VIDEO_UP:
		{
			int16 nSelected = m_pcVideoList->GetLastSelected();
			if( nSelected > 0 )
			{
				// Get the Line...
				ListViewStringRow* pcRow = static_cast<os::ListViewStringRow*>( m_pcVideoList->GetRow( nSelected ) );
				os::String cFileTime = pcRow->GetString(0);
				os::String cFileName = pcRow->GetString(1);
				m_pcVideoList->RemoveRow( nSelected );

				// ...and put it somewhere else
				ListViewStringRow *row = new ListViewStringRow();
				row->AppendString( cFileTime );
				row->AppendString( cFileName );
				m_pcVideoList->InsertRow( nSelected-1, row );
				m_pcVideoList->Select( nSelected-1 );
			}
			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC: Move an Video-file down.
		// NOTE:
		// SEE ALSO: VideoDisk()
		//----------------------------------------------------------------------------
		case M_VIDEO_DOWN:
		{
			uint16 nSelected = m_pcVideoList->GetLastSelected();
			if( nSelected < m_pcVideoList->GetRowCount()-1 && m_pcVideoList->GetRowCount() > 1 )
			{
				// Get the Line...
				ListViewStringRow* pcRow = static_cast<os::ListViewStringRow*>( m_pcVideoList->GetRow( nSelected ) );
				os::String cFileTime = pcRow->GetString(0);
				os::String cFileName = pcRow->GetString(1);
				m_pcVideoList->RemoveRow( nSelected );

				// ...and put it somewhere else
				ListViewStringRow *row = new ListViewStringRow();
				row->AppendString( cFileTime );
				row->AppendString( cFileName );
				m_pcVideoList->InsertRow( nSelected+1, row );
				m_pcVideoList->Select( nSelected+1 );
			}
			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC:
		// NOTE:
		// SEE ALSO: VideoDisk()
		//----------------------------------------------------------------------------
		case M_VIDEO_BURN:
		{
			VideoDisk();
			break;
		}


		//----------------------------------------------------------------------------
		// NAME: Flemming H. Sørensen
		// DESC:
		// NOTE:
		// SEE ALSO: BurnDisk()
		//----------------------------------------------------------------------------
		case M_BACKUP_BURN:
		{
			BackupDisk();
			break;
		}


		//----------------------------------------------------------------------------
		// NAME:
		// DESC: Quit.
		// NOTE:
		// SEE ALSO:
		//----------------------------------------------------------------------------
		case M_MENU_QUIT:
		case M_APP_QUIT:
		{
			PostMessage( os::M_QUIT );
		}

		break;
	}
}


//----------------------------------------------------------------------------
// NAME: Flemming H. Sørensen
// DESC: Erase a disk.
// NOTE: A disk can also be erased, from the "Erase"-menu. See erasewindow.cpp for more info on that.
// SEE ALSO: 
//----------------------------------------------------------------------------
void MainWindow::EraseDisk()
{
	os::String cError;
	int iError = 0;


	// Get the path to the burning device.
	int nDrive = m_pcBurnEraseDeviceSelection->GetSelection();
	os::String cDrive = os::String( m_pcBurnEraseDeviceSelection->GetItem(nDrive) );
	if(  nDrive == 0 || cDrive == "" )
	{
		cError =  os::String( "No device selected for Erasing..." );
		iError = iError + 1;
	}


	// Fast or Full erase.
	os::String cEraseMode = os::String( "fast" );
	if( m_pcBurnEraseModeFast->GetValue() == "true" )
		cEraseMode = os::String( "fast" );
	if( m_pcBurnEraseModeFull->GetValue() == "true" )
		cEraseMode = os::String( "all" );


	// Erase the disk...
	if( iError > 0 )			// ...if the user had selected a devie. The user hadn't, so we tell him/her to do so.
	{
		os::Alert* pcBurnNoWriteDevice = new os::Alert( "SimpleBurn - Error", cError , m_pcIcon->LockBitmap(), 0, "_Ok", NULL );
		pcBurnNoWriteDevice->Go( new os::Invoker( 0 ) );
	}
	else						// ...As I said; Erase the disk.
	{
		// Set the execute command.
		os::String cExec = os::String( "cdrecord -force blank=" ) + cEraseMode + os::String ( " gracetime=2 dev=" ) + cDrive;

		// Show the Splash Screen.
		Splash* pcWindow = new Splash(LoadImageFromResource("logo.png"),"Erasing disk...",false,0.0);
		pcWindow->Go();

		// Erase the disk.
		int nError = 0;
		nError = system( cExec.c_str() );

		// Remove the Splash Screen.
		pcWindow->Quit();

		// Success or Failure?
		if( nError != 0 )				// Failure.
		{
			os::Alert* pcAlert = new os::Alert( "SimpleBurn - Error", "Could not erase the disk...", m_pcIcon->LockBitmap(),0, "_Ok",NULL);
			pcAlert->Go( new os::Invoker( 0 ) );
		}
		else							// Success.
		{
			os::Alert* pcAlert = new os::Alert( "SimpleBurn", "The disk has been erased...", m_pcIcon->LockBitmap(),0, "_Ok",NULL);
			pcAlert->Go( new os::Invoker( 0 ) );
		}
	}
}


//----------------------------------------------------------------------------
// NAME: Flemming H. Sørensen
// DESC: We can copy an ISO to a disk - We can copy a disk to an ISO - We can copy a disk to a disk.
// NOTE:
// SEE ALSO: M_SAVE_REQUESTED and M_LOAD_REQUESTED (cDiskAuthor=copy)
//----------------------------------------------------------------------------
void MainWindow::CopyDisk()
{
	os::String cError = os::String( "" );
	int iError = 0;


	// Get the path to the read device.
	int nReadDrive = m_pcBurnReadDeviceSelection->GetSelection();
	os::String cReadDrive = m_pcBurnReadDeviceSelection->GetItem( nReadDrive );
	if( nReadDrive == 0 || cReadDrive == "" )
	{
		cError =  cError + os::String("No read-device selected...") + os::String( "\n" );
		iError = iError + 1;
	}


	// Get the path to the writing device.
	int nWriteDrive = m_pcBurnWriteDeviceSelection->GetSelection();
	os::String cWriteDrive = m_pcBurnWriteDeviceSelection->GetItem( nWriteDrive );
	if( nWriteDrive == 0 || cWriteDrive == "" )
	{
		cError =  cError + os::String("No device selected for writing...") + os::String( "\n" );
		iError = iError + 1;
	}


	// Make sure we're not trying to write from am image-drive to an image-drive.
	if( nReadDrive == 1 && nWriteDrive == 1 && iError == 0 )				// We can't copy from one ISO to another.
	{
		cError =  cError + os::String("You can't copy from one file to another, with this program...") + os::String( "\n" );
		iError = iError + 1;
	}
	else if( cReadDrive == cWriteDrive && iError == 0 )						// We can't copy to the same drive as we're reading from.
	{
		cError =  cError + os::String("You can't write to the same drive as you're reading from.\nYou should copy the disk to an ISO, and then burn the ISO.") + os::String( "\n" );
		iError = iError + 1;
	}


	// Get the burn speed.
	char cSpeed[99];
	os::String cMaxSpeed = m_pcBurnSpeedCheckBox->GetValue();
	if( cMaxSpeed == "true" )			// Max. Speed.
	{
		if( nReadDrive > 1 )
			sprintf( cSpeed, "12" );
		else
			sprintf( cSpeed, "50" );
	}
	else								// Selected Speed.
	{
		int iSpeed = (int)( m_pcBurnSpeedSlider->GetValue().AsFloat() );
		sprintf( cSpeed, "%i", iSpeed );
		if( iSpeed == 0 )				// If Speed = 0, we set it to Max. instead.
		{
			sprintf( cSpeed, "50" );
		}
	}


	// Get the number of copies to burn.
	int iNOC = m_pcBurnCopiesSpinner->GetValue();
	if( iNOC < 1 || iNOC > 25)
	{
		cError =  cError + os::String("You have to burn between 1 and 25 disks...") + os::String( "\n" );
		iError = iError + 1;
	}


	// Burn it, if everything is Ok.
	if( nReadDrive > 1 && nWriteDrive == 1 )			// We copy a disk to an ISO file.
	{
		cMakeISO1 = os::String( "cp " ) + cReadDrive + os::String( " " );
		cMakeISO2 = os::String( "" );
		m_pcFileRequesterISO = new os::FileRequester( os::FileRequester::SAVE_REQ, new os::Messenger( this ), "", os::FileRequester::NODE_FILE, false );
		m_pcFileRequesterISO->Show();
		m_pcFileRequesterISO->MakeFocus();
	}
	else if( iError > 0 )								// Something wasn't the way it should be, so we can't write to a disk.
	{
		os::Alert* pcBurnNoWriteDevice = new os::Alert( "SimpleBurn - Error", cError , m_pcIcon->LockBitmap(), 0, "_Ok", NULL );
		pcBurnNoWriteDevice->Go( new os::Invoker( 0 ) );
	}
	else												// Burn it to a disk.
	{
		if( nReadDrive == 1 && nWriteDrive > 1 )		// We copy an ISO to a disk.
		{
			char cNOC[2];
			sprintf( cNOC, "%i", iNOC );
			cMakeISO1 = os::String( "cdrecord gracetime=2 dev=" ) + cWriteDrive + os::String( " speed=" ) + cSpeed + os::String( " " );
			cMakeISO2 = cNOC;
			m_pcFileRequesterISO = new os::FileRequester( os::FileRequester::LOAD_REQ, new os::Messenger( this ), "", os::FileRequester::NODE_FILE, false );
			m_pcFileRequesterISO->Show();
			m_pcFileRequesterISO->MakeFocus();
		}
		else											// We copy from disk to disk.
		{
			// Set the execute command.
			os::String cExec = os::String( "cdrecord gracetime=2 dev=" ) + cWriteDrive + os::String( " speed=" ) + cSpeed + os::String( " " ) + cReadDrive;

			// Burn (all of) the disks...
			for( int counter = 1; counter <= iNOC; counter = counter + 1)
			{
				// Ask for a new disk, if we need one.
				if( iNOC > 1 )
				{
					char cCounter[99];
					char cNOC[99];
					sprintf( cCounter, "%i", counter );
					sprintf( cNOC, "%i", iNOC );
					os::String cMultiplyBurn = os::String("Ready to burn disk") + os::String( " " ) + cCounter + os::String( " " ) + os::String("of") + os::String( " " ) + cNOC + os::String( "\n" ) + os::String("Make sure there's an empty disk in the drive.");
					os::Alert* pcMultiplyBurn = new os::Alert( "SimpleBurn - Insert new CD", cMultiplyBurn , m_pcIcon->LockBitmap(), 0, "_Ok", NULL );
					pcMultiplyBurn->Go();
				}

				// Show the Splash Screen.
				char cSplashNote[128];
				sprintf( cSplashNote, "Copying disk (copy %i of %i)", counter, iNOC );
				Splash* pcWindow = new Splash(LoadImageFromResource("logo.png"),cSplashNote,false,0.0);
				pcWindow->Go();

				// Burn the disk.
				int nError = 0;
				nError = system( cExec.c_str() );

				// Remove the Splash Screen.
				pcWindow->Quit();

				// Success or Failure?
				if( nError != 0 )			// Failure.
				{
					os::Alert* pcAlert = new os::Alert( "SimpleBurn - Error", "Could not burn the new disk...", m_pcIcon->LockBitmap(),0, "_Ok",NULL);
					pcAlert->Go( new os::Invoker( 0 ) );
				}
				else						// Success.
				{
					os::Alert* pcAlert = new os::Alert( "SimpleBurn", "The new disk has been burned...", m_pcIcon->LockBitmap(),0, "_Ok",NULL);
					pcAlert->Go( new os::Invoker( 0 ) );
				}
			}
		}
	}
}


//----------------------------------------------------------------------------
// NAME: Flemming H. Sørensen
// DESC: Burn a dir. to a disk.
// NOTE: This way of making data disks sucks, and should be replaced, so people don't have to move everything
//		 into a directory, and select that. It should be more like what we see in most other burning programs.
// SEE ALSO:
//----------------------------------------------------------------------------
void MainWindow::DataDisk()
{
	os::String cError = os::String( "" );
	int iError = 0;


	// Get the path to the writing device.
	int nDrive = m_pcDataWriteDeviceSelection->GetSelection();
	os::String cDrive = m_pcDataWriteDeviceSelection->GetItem( nDrive );
	if( nDrive == 0 || cDrive == "" )
	{
		cError =  cError + os::String("No device selected for writing...") + os::String( "\n" );
		iError = iError + 1;
	}


	// Get the burn speed.
	char cSpeed[99];
	os::String cMaxSpeed = m_pcDataSpeedCheckBox->GetValue();
	if( cMaxSpeed == "true" )					// Max. Speed.
	{
		sprintf( cSpeed, "50" );
	}
	else										// Selected Speed.
	{
		int iSpeed = (int)( m_pcDataSpeedSlider->GetValue().AsFloat() );
		sprintf( cSpeed, "%i", iSpeed );
		if( iSpeed == 0 )						// If Speed = 0, we set it to Max. instead.
		{
			sprintf( cSpeed, "50" );
		}
	}


	// Get the number of copies to burn.
	int iNOC = m_pcDataCopiesSpinner->GetValue();
	if( iNOC < 1 || iNOC > 25)
	{
		cError =  cError + os::String("You have to burn between 1 and 25 disks...") + os::String( "\n" );
		iError = iError + 1;
	}


	// Get the Disk-name.
	os::String cDiskNameTemp = m_pcDataDiskNameText->GetValue();
	os::String cDiskName = os::String( "\"" ) + cDiskNameTemp + os::String( "\"" );
	if( cDiskName == "" )
	{
		cDiskName = os::String("Data");
	}


	// Get the Directory.
	os::String cDataDir = m_pcDataAddPath->GetValue();
	if( cDataDir == "" )
	{
		cError =  cError + os::String("You need to choose a directory, to burn...") + os::String( "\n" );
		iError = iError + 1;
	}


	// Burn it, if everything is Ok.
	if( nDrive == 1 )							// We "burn" it to an ISO file.
	{
		cMakeISO1 = os::String( "mkisofs -r -J -joliet-long -udf -hide-rr-moved -V ") + cDiskName + os::String( " -o " );
		cMakeISO2 = cDataDir;

		m_pcFileRequesterISO = new os::FileRequester( os::FileRequester::SAVE_REQ, new os::Messenger( this ), cDiskName, os::FileRequester::NODE_FILE, false );
		m_pcFileRequesterISO->Show();
		m_pcFileRequesterISO->MakeFocus();
	}
	else if( iError > 0 )						// Something wasn't the way it should be, so we can't write to a disk.
	{
		os::Alert* pcBurnNoWriteDevice = new os::Alert( "SimpleBurn - Error", cError , m_pcIcon->LockBitmap(), 0, "_Ok", NULL );
		pcBurnNoWriteDevice->Go( new os::Invoker( 0 ) );
	}
	else										// Burn it to a disk.
	{
		// Set the execute command.
		os::String cExec = os::String( "mkisofs -r -J -joliet-long -udf -hide-rr-moved -V ") + cDiskName + os::String( " " ) + cDataDir + os::String( " | cdrecord gracetime=2 dev=" ) + cDrive + os::String( " speed=" ) + cSpeed + os::String( " -" );

		// Burn (all of) the disks...
		for( int counter = 1; counter <= iNOC; counter = counter + 1)
		{
			// Ask for a new disk, if we need one.
			if( iNOC > 1 )
			{
				char cCounter[99];
				char cNOC[99];
				sprintf( cCounter, "%i", counter );
				sprintf( cNOC, "%i", iNOC );
				os::String cMultiplyBurn = os::String("Ready to burn disk") + os::String( " " ) + cCounter + os::String( " " ) + os::String("of") + os::String( " " ) + cNOC + os::String( "\n" ) + os::String("Make sure there's an empty disk in the drive.");
				os::Alert* pcMultiplyBurn = new os::Alert( "SimpleBurn - Insert new CD", cMultiplyBurn , m_pcIcon->LockBitmap(), 0, "_Ok", NULL );
				pcMultiplyBurn->Go();
			}

			// Show the Splash Screen.
			char cSplashNote[128];
			sprintf( cSplashNote, "Writing data-disk (copy %i of %i)", counter, iNOC );
			Splash* pcWindow = new Splash(LoadImageFromResource("logo.png"),cSplashNote,false,0.0);
			pcWindow->Go();

			// Burn the disk.
			int nError = 0;
			nError = system( cExec.c_str() );

			// Remove the Splash Screen.
			pcWindow->Quit();

			// Success or Failure?
			if( nError != 0 )			// Failure.
			{
				os::Alert* pcAlert = new os::Alert( "SimpleBurn - Error", "Could not burn the disk...", m_pcIcon->LockBitmap(),0, "_Ok",NULL);
				pcAlert->Go( new os::Invoker( 0 ) );
			}
			else						// Success.
			{
				os::Alert* pcAlert = new os::Alert( "SimpleBurn", "The disk has been burned...", m_pcIcon->LockBitmap(),0, "_Ok",NULL);
				pcAlert->Go( new os::Invoker( 0 ) );
			}
		}
	}
}


//----------------------------------------------------------------------------
// NAME: Flemming H. Sørensen
// DESC: Make an Audio disk.
// NOTE: There's no way to burn this to an ISO, so it will have to be burned to a disk.
// SEE ALSO:
//----------------------------------------------------------------------------
void MainWindow::AudioDisk()
{
	os::String cError = os::String( "" );
	int iError = 0;


	// Get the path to the writing device.
	int nDrive = m_pcAudioWriteDeviceSelection->GetSelection();
	os::String cDrive = m_pcAudioWriteDeviceSelection->GetItem( nDrive );
	if( nDrive == 0 || cDrive == "" )
	{
		cError =  cError + os::String("No device selected for writing...") + os::String( "\n" );
		iError = iError + 1;
	}


	// Get the burn speed.
	char cSpeed[99];
	os::String cMaxSpeed = m_pcAudioSpeedCheckBox->GetValue();
	if( cMaxSpeed == "true" )				// Max. Speed.
	{
		sprintf( cSpeed, "50" );
	}
	else									// Selected Speed.
	{
		int iSpeed = (int)( m_pcAudioSpeedSlider->GetValue().AsFloat() );
		sprintf( cSpeed, "%i", iSpeed );
		if( iSpeed == 0 )					// If Speed = 0, we set it to Max. instead.
		{
			sprintf( cSpeed, "50" );
		}
	}


	// Get the number of copies to burn.
	int iNOC = m_pcAudioCopiesSpinner->GetValue();
	if( iNOC < 1 || iNOC > 25)
	{
		cError =  cError + os::String("You have to burn between 1 and 25 disks...") + os::String( "\n" );
		iError = iError + 1;
	}


	// Make a list of tracks.
	os::String cFileList = os::String("");
	int iCount = m_pcAudioList->GetRowCount();
	if( iCount > 0 )
	{
		for( int8 counter = 0; counter < iCount; counter = counter + 1)
		{
			ListViewStringRow* pcRow = static_cast<os::ListViewStringRow*>( m_pcAudioList->GetRow( counter ) );
			os::String cAddFile = pcRow->GetString(1);
			cFileList = cFileList + os::String( " " ) + cAddFile;
		}
	}
	else
	{
		cError =  cError + os::String("You need to choose atleast one audio file, to burn...") + os::String( "\n" );
		iError = iError + 1;
	}


	// Burn it, if everything is Ok.
	if( iError > 0 )							// Something wasn't the way it should be, so we can't write to a disk.
	{
		os::Alert* pcBurnNoWriteDevice = new os::Alert( "SimpleBurn - Error", cError , m_pcIcon->LockBitmap(), 0, "_Ok", NULL );
		pcBurnNoWriteDevice->Go( new os::Invoker( 0 ) );
	}
	else										// Burn it to a disk.
	{
		// Set the execute command.
		os::String cExec;
		if( m_pcMenuMediatypeCD->IsChecked() == true ) // Audio CD
			cExec = os::String( "cdrecord gracetime=2 -pad dev=" ) + cDrive + os::String( " speed=" ) + cSpeed + os::String( " -audio " ) + cFileList;
		if( m_pcMenuMediatypeDVD->IsChecked() == true ) // Audio DVD
			cExec = os::String( "" );

		// Burn (all of) the disks...
		for( int counter = 1; counter <= iNOC; counter = counter + 1)
		{
			// Ask for a new disk, if we need one.
			if( iNOC > 1 )
			{
				char cCounter[99];
				char cNOC[99];
				sprintf( cCounter, "%i", counter );
				sprintf( cNOC, "%i", iNOC );
				os::String cMultiplyBurn = os::String("Ready to burn disk") + os::String( " " ) + cCounter + os::String( " " ) + os::String("of") + os::String( " " ) + cNOC + os::String( "\n" ) + os::String("Make sure there's an empty disk in the drive.");
				os::Alert* pcMultiplyBurn = new os::Alert( "SimpleBurn - Insert new CD", cMultiplyBurn , m_pcIcon->LockBitmap(), 0, "_Ok", NULL );
				pcMultiplyBurn->Go();
			}

			// Show the Splash Screen.
			char cSplashNote[128];
			sprintf( cSplashNote, "Writing audio-disk (copy %i of %i)", counter, iNOC );
			Splash* pcWindow = new Splash(LoadImageFromResource("logo.png"),cSplashNote,false,0.0);
			pcWindow->Go();

			// Burn the disk.
			int nError = 0;
			nError = system( cExec.c_str() );

			// Remove the Splash Screen.
			pcWindow->Quit();

			// Success or Failure?
			if( nError != 0 )					// Failure.
			{
				os::Alert* pcAlert = new os::Alert( "SimpleBurn - Error", "Could not burn the disk...", m_pcIcon->LockBitmap(),0, "_Ok",NULL);
				pcAlert->Go( new os::Invoker( 0 ) );
			}
			else								// Success.
			{
				os::Alert* pcAlert = new os::Alert( "SimpleBurn", "The disk has been burned...", m_pcIcon->LockBitmap(),0, "_Ok",NULL);
				pcAlert->Go( new os::Invoker( 0 ) );
			}
		}
	}
}


//----------------------------------------------------------------------------
// NAME: Flemming H. Sørensen
// DESC: Make a Video disk.
// NOTE: There's no way to burn this to an ISO, so it will have to be burned to a disk.
//		 It still needs a bit of work, to make the disk.
// SEE ALSO:
//----------------------------------------------------------------------------
void MainWindow::VideoDisk()
{
	os::String cError = os::String( "" );
	int iError = 0;


	// Get the path to the writing device.
	int nDrive = m_pcVideoWriteDeviceSelection->GetSelection();
	os::String cDrive = m_pcVideoWriteDeviceSelection->GetItem( nDrive );
	if( nDrive == 0 || cDrive == "" )
	{
		cError =  cError + os::String("No device selected for writing...") + os::String( "\n" );
		iError = iError + 1;
	}


	// Get the burn speed.
	char cSpeed[99];
	os::String cMaxSpeed = m_pcVideoSpeedCheckBox->GetValue();
	if( cMaxSpeed == "true" )				// Max. Speed.
	{
		sprintf( cSpeed, "50" );
	}
	else									// Selected Speed.
	{
		int iSpeed = (int)( m_pcVideoSpeedSlider->GetValue().AsFloat() );
		sprintf( cSpeed, "%i", iSpeed );
		if( iSpeed == 0 )					// If Speed = 0, we set it to Max. instead.
		{
			sprintf( cSpeed, "50" );
		}
	}


	// Get the number of copies to burn.
	int iNOC = m_pcVideoCopiesSpinner->GetValue();
	if( iNOC < 1 || iNOC > 25)
	{
		cError =  cError + os::String("You have to burn between 1 and 25 disks...") + os::String( "\n" );
		iError = iError + 1;
	}


	// Get the Disk-name.
	os::String cDiskNameTemp = m_pcVideoDiskNameText->GetValue();
	os::String cDiskName = os::String( "\"" ) + cDiskNameTemp + os::String( "\"" );
	if( cDiskName == "" )
	{
		cDiskName = os::String("Video");
	}


	// Make a list of tracks.
	os::String cFileList = os::String("");
	int iCount = m_pcVideoList->GetRowCount();
	if( iCount > 0 )
	{
		for( int8 counter = 0; counter < iCount; counter = counter + 1)
		{
			ListViewStringRow* pcRow = static_cast<os::ListViewStringRow*>( m_pcVideoList->GetRow( counter ) );
			os::String cAddFile = pcRow->GetString(1);
			cFileList = cFileList + os::String( " " ) + cAddFile;
		}
	}
	else
	{
		cError =  cError + os::String("You need to choose atleast one video file, to burn...") + os::String( "\n" );
		iError = iError + 1;
	}


	// Burn it, if everything is Ok.
	if( iError > 0 )						// Something wasn't the way it should be, so we can't write to a disk.
	{
		os::Alert* pcBurnNoWriteDevice = new os::Alert( "SimpleBurn - Error", cError , m_pcIcon->LockBitmap(), 0, "_Ok", NULL );
		pcBurnNoWriteDevice->Go( new os::Invoker( 0 ) );
	}
	else									// Burn it to a disk.
	{
		// Set the execute command.
		os::String cExec;
		if( m_pcMenuMediatypeCD->IsChecked() == true ) // (Super-) Video CD
			cExec = os::String( "cdrecord gracetime=2 -pad dev=" ) + cDrive + os::String( " speed=" ) + cSpeed + os::String( " -V " ) + cDiskName + os::String( " -cdi " ) + cFileList;
		if( m_pcMenuMediatypeCD->IsChecked() == true ) // Video DVD
			cExec = os::String( "" );

		// Burn (all of) the disks...
		for( int counter = 1; counter <= iNOC; counter = counter + 1)
		{
			// Ask for a new disk, if we need one.
			if( iNOC > 1 )
			{
				char cCounter[99];
				char cNOC[99];
				sprintf( cCounter, "%i", counter );
				sprintf( cNOC, "%i", iNOC );
				os::String cMultiplyBurn = os::String("Ready to burn disk") + os::String( " " ) + cCounter + os::String( " " ) + os::String("of") + os::String( " " ) + cNOC + os::String( "\n" ) + os::String("Make sure there's an empty disk in the drive.");
				os::Alert* pcMultiplyBurn = new os::Alert( "SimpleBurn - Insert new CD", cMultiplyBurn , m_pcIcon->LockBitmap(), 0, "_Ok", NULL );
				pcMultiplyBurn->Go();
			}

			// Show the Splash Screen.
			char cSplashNote[128];
			sprintf( cSplashNote, "Writing video-disk (copy %i of %i)", counter, iNOC );
			Splash* pcWindow = new Splash(LoadImageFromResource("logo.png"),cSplashNote,false,0.0);
			pcWindow->Go();

			// Burn the disk.
			int nError = 0;
			nError = system( cExec.c_str() );

			// Remove the Splash Screen.
			pcWindow->Quit();

			// Success or Failure?
			if( nError != 0 )					// Failure.
			{
				os::Alert* pcAlert = new os::Alert( "SimpleBurn - Error", "Could not burn the disk...", m_pcIcon->LockBitmap(),0, "_Ok",NULL);
				pcAlert->Go( new os::Invoker( 0 ) );
			}
			else								// Success.
			{
				os::Alert* pcAlert = new os::Alert( "SimpleBurn", "The disk has been burned...", m_pcIcon->LockBitmap(),0, "_Ok",NULL);
				pcAlert->Go( new os::Invoker( 0 ) );
			}
		}
	}
}


//----------------------------------------------------------------------------
// NAME: Flemming H. Sørensen
// DESC: Burn a backup...
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------
void MainWindow::BackupDisk()
{
	os::String cError = os::String( "" );
	int iError = 0;


	// Get the path to the writing device.
	int nDrive = m_pcBackupWriteDeviceSelection->GetSelection();
	os::String cDrive = m_pcBackupWriteDeviceSelection->GetItem( nDrive );
	if( nDrive == 0 || cDrive == "" )
	{
		cError =  cError + os::String("No device selected for writing...") + os::String( "\n" );
		iError = iError + 1;
	}


	// Get the burn speed.
	char cSpeed[99];
	os::String cMaxSpeed = m_pcBackupSpeedCheckBox->GetValue();
	if( cMaxSpeed == "true" )		// Max. Speed.
	{
		sprintf( cSpeed, "50" );
	}
	else							// Selected Speed.
	{
		int iSpeed = (int)( m_pcBackupSpeedSlider->GetValue().AsFloat() );
		sprintf( cSpeed, "%i", iSpeed );
		if( iSpeed == 0 )			// If Speed = 0, we set it to Max. instead.
		{
			sprintf( cSpeed, "50" );
		}
	}


	// Get the number of copies to burn.
	int iNOC = m_pcBackupCopiesSpinner->GetValue();
	if( iNOC < 1 || iNOC > 25)
	{
		cError =  cError + os::String("You have to burn between 1 and 25 disks...") + os::String( "\n" );
		iError = iError + 1;
	}


	// Get the Disk-name.
	os::String cDiskNameTemp = m_pcBackupDiskNameText->GetValue();
	os::String cDiskName = os::String( "\"" ) + cDiskNameTemp + os::String( "\"" );
	if( cDiskName == "" )
	{
		cDiskName = os::String("Backup");
	}


	// Get the Directories we want to burn.
	os::String cHome = getenv( "HOME" );
	os::String cDataDir;
	if( m_pcBackupSelectMail->GetValue() == "true" )
		cDataDir = cDataDir + os::String( "/Mail/=" ) + cHome + os::String( "/Mail " );
	if( m_pcBackupSelectBookmarks->GetValue() == "true" )
		cDataDir = cDataDir + os::String( "/Bookmarks/=" ) + cHome + os::String( "/Bookmarks " );
	if( m_pcBackupSelectContacts->GetValue() == "true" )
		cDataDir = cDataDir + os::String( "/Contacts/=" ) + cHome + os::String( "/Contacts " );
	if( m_pcBackupSelectDocuments->GetValue() == "true" )
		cDataDir = cDataDir + os::String( "/Documents/=" ) + cHome + os::String( "/Documents " );
	if( m_pcBackupSelectPictures->GetValue() == "true" )
		cDataDir = cDataDir + os::String( "/Pictures/=" ) + cHome + os::String( "/Pictures " );
	if( m_pcBackupSelectMusic->GetValue() == "true" )
		cDataDir = cDataDir + os::String( "/Music/=" ) + cHome + os::String( "/Music " );
	if( m_pcBackupSelectMovies->GetValue() == "true" )
		cDataDir = cDataDir + os::String( "/Movies/=" ) + cHome + os::String( "/Movies " );
	if( m_pcBackupSelectSettings->GetValue() == "true" )
		cDataDir = cDataDir + os::String( "/Settings/=" ) + cHome + os::String( "/Settings " );
	if( m_pcBackupSelectEverything->GetValue() == "true" )
		cDataDir = cHome;
	if( cDataDir == "" )		// Show an error, if the user want to make a backup of nothing...
	{
		cError =  cError + os::String("You can't make a backup of nothing...") + os::String( "\n" );
		iError = iError + 1;
	}


	// Burn it, if everything is Ok.
	if( nDrive == 1 )				// We "burn" it to an ISO file.
	{
		cMakeISO1 = os::String( "mkisofs -r -J -joliet-long -udf -hide-rr-moved -graft-points -V ") + cDiskName + os::String( " -o " );
		cMakeISO2 = cDataDir;

		m_pcFileRequesterISO = new os::FileRequester( os::FileRequester::SAVE_REQ, new os::Messenger( this ), cDiskName, os::FileRequester::NODE_FILE, false );
		m_pcFileRequesterISO->Show();
		m_pcFileRequesterISO->MakeFocus();
	}
	else if( iError > 0 )			// Something wasn't the way it should be, so we can't write to a disk.
	{
		os::Alert* pcBurnNoWriteDevice = new os::Alert( "SimpleBurn - Error", cError , m_pcIcon->LockBitmap(), 0, "_Ok", NULL );
		pcBurnNoWriteDevice->Go( new os::Invoker( 0 ) );
	}
	else							// Burn it to a disk.
	{
		// Set the execute command.
		os::String cExec = os::String( "mkisofs -r -J -joliet-long -udf -hide-rr-moved -graft-points -V ") + cDiskName + os::String( " " ) + cDataDir + os::String( " | cdrecord gracetime=2 dev=" ) + cDrive + os::String( " speed=" ) + cSpeed + os::String( " -" );

		// Burn (all of) the disks...
		for( int counter = 1; counter <= iNOC; counter = counter + 1)
		{
			// Ask for a new disk, if we need one.
			if( iNOC > 1 )
			{
				char cCounter[99];
				char cNOC[99];
				sprintf( cCounter, "%i", counter );
				sprintf( cNOC, "%i", iNOC );
				os::String cMultiplyBurn = os::String("Ready to burn disk") + os::String( " " ) + cCounter + os::String( " " ) + os::String("of") + os::String( " " ) + cNOC + os::String( "\n" ) + os::String("Make sure there's an empty disk in the drive.");
				os::Alert* pcMultiplyBurn = new os::Alert( "SimpleBurn - Insert new CD", cMultiplyBurn , m_pcIcon->LockBitmap(), 0, "_Ok", NULL );
				pcMultiplyBurn->Go();
			}

			// Show the Splash Screen.
			char cSplashNote[128];
			sprintf( cSplashNote, "Writing backup-disk (copy %i of %i)", counter, iNOC );
			Splash* pcWindow = new Splash(LoadImageFromResource("logo.png"),cSplashNote,false,0.0);
			pcWindow->Go();

			// Burn the disk.
			int nError = 0;
			nError = system( cExec.c_str() );

			// Remove the Splash Screen.
			pcWindow->Quit();

			// Success or Failure?
			if( nError != 0 )			// Failure.
			{
				os::Alert* pcAlert = new os::Alert( "SimpleBurn - Error", "Could not burn the disk...", m_pcIcon->LockBitmap(),0, "_Ok",NULL);
				pcAlert->Go( new os::Invoker( 0 ) );
			}
			else						// Success.
			{
				os::Alert* pcAlert = new os::Alert( "SimpleBurn", "The disk has been burned...", m_pcIcon->LockBitmap(),0, "_Ok",NULL);
				pcAlert->Go( new os::Invoker( 0 ) );
			}
		}
	}
}


//----------------------------------------------------------------------------
// NAME: Flemming H. Sørensen
// DESC: Load the Settings
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------
void MainWindow::LoadSettings()
{
	os::String zPath = getenv( "HOME" );
	zPath += "/Settings/SimpleBurn/Settings";

	try
	{
		os::Settings* pcSettings = new os::Settings(new File(zPath));
		pcSettings->Load();

		// Get Menu-settings.
		m_pcMenuMediatypeCD->SetChecked( pcSettings->GetBool( "menu_media_cd", true ) );
		m_pcMenuMediatypeDVD->SetChecked( pcSettings->GetBool( "menu_media_dvd", false ) );
		m_pcMenuMediatypeHDDVD->SetChecked( pcSettings->GetBool( "menu_media_hddvd", false ) );
		m_pcMenuMediatypeBLURAY->SetChecked( pcSettings->GetBool( "menu_media_bluray", false ) );
		m_pcMenuSaveOnExit->SetChecked( pcSettings->GetBool( "menu_save_saveonexit", true ) );

		// Get Erase-settings.
		for( int8 i = 0 ; i < m_pcBurnEraseDeviceSelection->GetItemCount() ; i++ )
			if( pcSettings->GetString( "erase_writedevice", "Choose Device..." ) == m_pcBurnEraseDeviceSelection->GetItem( i ) )
				m_pcBurnEraseDeviceSelection->SetSelection( i );
		m_pcBurnEraseModeFast->SetValue( pcSettings->GetBool( "erase_fast", true ) );
		m_pcBurnEraseModeFull->SetValue( pcSettings->GetBool( "erase_full", false ) );

		// Get Copy-settings.
		for( int8 i = 0 ; i < m_pcBurnReadDeviceSelection->GetItemCount() ; i++ )
			if( pcSettings->GetString( "copy_readdevice", "Choose Device..." ) == m_pcBurnReadDeviceSelection->GetItem( i ) )
				m_pcBurnReadDeviceSelection->SetSelection( i );
		for( int8 i = 0 ; i < m_pcBurnWriteDeviceSelection->GetItemCount() ; i++ )
			if( pcSettings->GetString( "burn_writedevice", "Choose Device..." ) == m_pcBurnWriteDeviceSelection->GetItem( i ) )
				m_pcBurnWriteDeviceSelection->SetSelection( i );
		m_pcBurnSpeedCheckBox->SetValue( pcSettings->GetBool( "copy_maxspeed", true ) );
		m_pcBurnSpeedSlider->SetValue( pcSettings->GetFloat( "copy_speed", 0.000000 ) );

		// Get Data-settings.
		for( int8 i = 0 ; i < m_pcDataWriteDeviceSelection->GetItemCount() ; i++ )
			if( pcSettings->GetString( "data_writedevice", "Choose Device..." ) == m_pcDataWriteDeviceSelection->GetItem( i ) )
				m_pcDataWriteDeviceSelection->SetSelection( i );
		m_pcDataSpeedCheckBox->SetValue( pcSettings->GetBool( "data_maxspeed", true ) );
		m_pcDataSpeedSlider->SetValue( pcSettings->GetFloat( "data_speed", 0.000000 ) );

		// Get Audio-settings.
		for( int8 i = 0 ; i < m_pcAudioWriteDeviceSelection->GetItemCount() ; i++ )
			if( pcSettings->GetString( "audio_writedevice", "Choose Device..." ) == m_pcAudioWriteDeviceSelection->GetItem( i ) )
				m_pcAudioWriteDeviceSelection->SetSelection( i );
		m_pcAudioSpeedCheckBox->SetValue( pcSettings->GetBool( "audio_maxspeed", true ) );
		m_pcAudioSpeedSlider->SetValue( pcSettings->GetFloat( "audio_speed", 0.000000 ) );

		// Get Video-settings.
		for( int8 i = 0 ; i < m_pcVideoWriteDeviceSelection->GetItemCount() ; i++ )
			if( pcSettings->GetString( "video_writedevice", "Choose Device..." ) == m_pcVideoWriteDeviceSelection->GetItem( i ) )
				m_pcVideoWriteDeviceSelection->SetSelection( i );
		m_pcVideoSpeedCheckBox->SetValue( pcSettings->GetBool( "video_maxspeed", true ) );
		m_pcVideoSpeedSlider->SetValue( pcSettings->GetFloat( "video_speed", 0.000000 ) );

		// Get Backup-settings.
		for( int8 i = 0 ; i < m_pcBackupWriteDeviceSelection->GetItemCount() ; i++ )
			if( pcSettings->GetString( "backup_writedevice", "Choose Device..." ) == m_pcBackupWriteDeviceSelection->GetItem( i ) )
				m_pcBackupWriteDeviceSelection->SetSelection( i );
		m_pcBackupSpeedCheckBox->SetValue( pcSettings->GetBool( "backup_maxspeed", true ) );
		m_pcBackupSpeedSlider->SetValue( pcSettings->GetFloat( "backup_speed", 0.000000 ) );

		delete( pcSettings );
	}
	catch(...)
	{
	}
}


//----------------------------------------------------------------------------
// NAME: Flemming H. Sørensen
// DESC: Save the Settings
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------
void MainWindow::SaveSettings()
{
	os::String zPath = getenv( "HOME" );
	zPath += "/Settings/SimpleBurn";

	try
	{
		mkdir( zPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO );
		zPath += "/Settings";
		os::File cFile( zPath, O_RDWR | O_CREAT );	
		os::Settings* pcSettings = new os::Settings(new File(zPath));

		// Set Menu-settings.
		pcSettings->SetBool( "menu_media_cd", m_pcMenuMediatypeCD->IsChecked() );
		pcSettings->SetBool( "menu_media_dvd", m_pcMenuMediatypeDVD->IsChecked() );
		pcSettings->SetBool( "menu_media_hddvd", m_pcMenuMediatypeHDDVD->IsChecked() );
		pcSettings->SetBool( "menu_media_bluray", m_pcMenuMediatypeBLURAY->IsChecked() );
		pcSettings->SetBool( "menu_save_saveonexit", m_pcMenuSaveOnExit->IsChecked() );

		// Set Erase-settings.
		pcSettings->SetString( "erase_writedevice", m_pcBurnEraseDeviceSelection->GetItem( m_pcBurnEraseDeviceSelection->GetSelection() ) );
		pcSettings->SetBool( "erase_fast", m_pcBurnEraseModeFast->GetValue() );
		pcSettings->SetBool( "erase_full", m_pcBurnEraseModeFull->GetValue() );

		// Set Copy-settings.
		pcSettings->SetString( "copy_readdevice", m_pcBurnReadDeviceSelection->GetItem( m_pcBurnReadDeviceSelection->GetSelection() ) );
		pcSettings->SetString( "copy_writedevice", m_pcBurnWriteDeviceSelection->GetItem( m_pcBurnWriteDeviceSelection->GetSelection() ) );
		pcSettings->SetBool( "copy_maxspeed", m_pcBurnSpeedCheckBox->GetValue() );
		pcSettings->SetFloat( "copy_speed", m_pcBurnSpeedSlider->GetValue() );

		// Set Data-settings.
		pcSettings->SetString( "data_writedevice", m_pcDataWriteDeviceSelection->GetItem( m_pcDataWriteDeviceSelection->GetSelection() ) );
		pcSettings->SetBool( "data_maxspeed", m_pcDataSpeedCheckBox->GetValue() );
		pcSettings->SetFloat( "data_speed", m_pcDataSpeedSlider->GetValue() );

		// Set Audio-settings.
		pcSettings->SetString( "audio_writedevice", m_pcAudioWriteDeviceSelection->GetItem( m_pcAudioWriteDeviceSelection->GetSelection() ) );
		pcSettings->SetBool( "audio_maxspeed", m_pcAudioSpeedCheckBox->GetValue() );
		pcSettings->SetFloat( "audio_speed", m_pcAudioSpeedSlider->GetValue() );

		// Set Video-settings.
		pcSettings->SetString( "video_writedevice", m_pcVideoWriteDeviceSelection->GetItem( m_pcVideoWriteDeviceSelection->GetSelection() ) );
		pcSettings->SetBool( "video_maxspeed", m_pcVideoSpeedCheckBox->GetValue() );
		pcSettings->SetFloat( "video_speed", m_pcVideoSpeedSlider->GetValue() );

		// Set Backup-settings.
		pcSettings->SetString( "backup_writedevice", m_pcBackupWriteDeviceSelection->GetItem( m_pcBackupWriteDeviceSelection->GetSelection() ) );
		pcSettings->SetBool( "backup_maxspeed", m_pcBackupSpeedCheckBox->GetValue() );
		pcSettings->SetFloat( "backup_speed", m_pcBackupSpeedSlider->GetValue() );

		pcSettings->Save();
		delete( pcSettings );
	}
	catch(...)
	{
	}
}


//----------------------------------------------------------------------------
// NAME: Flemming H. Sørensen
// DESC: Scan for devices
// NOTE: I'm not sure this is the best way to do it, but it works...
// SEE ALSO:
//----------------------------------------------------------------------------
void MainWindow::DeviceScan()
{
	struct stat f__stat;
	char cDevice[24];

	/* Scan for ATA drives */
	for( int counter = 97 ; counter <= 122 ; counter = counter + 1 )
	{
		os::String cDev = os::String( "/dev/disk/ata/cd" ) + counter + os::String( "/" );
		os::String cDevRaw = cDev + os::String( "raw" );
		sprintf( cDevice, "%s", cDevRaw.c_str() );
		bool FileExist = (stat(cDevice,&f__stat) == 0);
		if( FileExist == true )
		{
			m_pcBurnReadDeviceSelection->AppendItem( cDevRaw );
			m_pcBurnWriteDeviceSelection->AppendItem( cDevRaw );
			m_pcBurnEraseDeviceSelection->AppendItem( cDevRaw );
			m_pcDataWriteDeviceSelection->AppendItem( cDevRaw );
			m_pcAudioWriteDeviceSelection->AppendItem( cDevRaw );
			m_pcVideoWriteDeviceSelection->AppendItem( cDevRaw );
			m_pcBackupWriteDeviceSelection->AppendItem( cDevRaw );
		}
	}

	/* Scan for USB/SCSI drives */
	for( int counter = 97 ; counter <= 122 ; counter = counter + 1 )
	{
		os::String cDev = os::String( "/dev/disk/scsi/cd" ) + counter + os::String( "/" );
		os::String cDevRaw = cDev + os::String( "raw" );
		sprintf( cDevice, "%s", cDevRaw.c_str() );
		bool FileExist = (stat(cDevice,&f__stat) == 0);
		if( FileExist == true )
		{
			m_pcBurnReadDeviceSelection->AppendItem( cDevRaw );
			m_pcBurnWriteDeviceSelection->AppendItem( cDevRaw );
			m_pcBurnEraseDeviceSelection->AppendItem( cDevRaw );
			m_pcDataWriteDeviceSelection->AppendItem( cDevRaw );
			m_pcAudioWriteDeviceSelection->AppendItem( cDevRaw );
			m_pcVideoWriteDeviceSelection->AppendItem( cDevRaw );
			m_pcBackupWriteDeviceSelection->AppendItem( cDevRaw );
		}
	}
}


//----------------------------------------------------------------------------
// NAME: Flemming H. Sørensen
// DESC: Set the Media-type
// NOTE: HD-DVD and Blu-ray is still missing
// SEE ALSO:
//----------------------------------------------------------------------------
void MainWindow::SetMediaType()
{
	// CD
	if( m_pcMenuMediatypeCD->IsChecked() == true )
	{
		m_pcAudioList->Clear();
		m_pcAudioTypeSelection->Clear();
		m_pcAudioTypeSelection->AppendItem( "Select disk-type" );
		m_pcAudioTypeSelection->AppendItem( "Audio CD" );
		m_pcAudioTypeSelection->SetSelection( (int) 0 );
		m_pcAudioTypeSelection->SetEnable( true );

		m_pcVideoList->Clear();
		m_pcVideoTypeSelection->Clear();
		m_pcVideoTypeSelection->AppendItem( "Select disk-type" );
		m_pcVideoTypeSelection->AppendItem( "Video CD - PAL" );
		m_pcVideoTypeSelection->AppendItem( "Video CD - NTSC" );
		m_pcVideoTypeSelection->AppendItem( "S-Video CD - PAL" );
		m_pcVideoTypeSelection->AppendItem( "S-Video CD - NTSC" );
		m_pcVideoTypeSelection->AppendItem( "CVD - PAL" );
		m_pcVideoTypeSelection->AppendItem( "CVD - NTSC" );
		m_pcVideoTypeSelection->SetSelection( (int) 0 );
		m_pcVideoTypeSelection->SetEnable( true );

		UpdateTime();
	}

	// DVD
	if( m_pcMenuMediatypeDVD->IsChecked() == true )
	{
		m_pcAudioList->Clear();
		m_pcAudioTypeSelection->Clear();
		/* m_pcAudioTypeSelection->AppendItem( "Select disk-type" ); */
		/* m_pcAudioTypeSelection->AppendItem( "Audio DVD" ); */
		m_pcAudioTypeSelection->AppendItem( "NOT SUPPORTED" );
		m_pcAudioTypeSelection->SetSelection( (int) 0 );
		m_pcAudioTypeSelection->SetEnable( true );

		m_pcVideoList->Clear();
		m_pcVideoTypeSelection->Clear();
		m_pcVideoTypeSelection->AppendItem( "Select disk-type" );
		m_pcVideoTypeSelection->AppendItem( "Video DVD - PAL" );
		m_pcVideoTypeSelection->AppendItem( "Video DVD - NTSC" );
		m_pcVideoTypeSelection->SetSelection( (int) 0 );
		m_pcVideoTypeSelection->SetEnable( true );

		UpdateTime();
	}

	// HD-DVD		NOT DONE!
	if( m_pcMenuMediatypeHDDVD->IsChecked() == true )
	{
		m_pcAudioList->Clear();
		m_pcAudioTypeSelection->Clear();
		/* m_pcAudioTypeSelection->AppendItem( "Select disk-type" ); */
		m_pcAudioTypeSelection->AppendItem( "NOT SUPPORTED" );
		m_pcAudioTypeSelection->SetSelection( (int) 0 );
		m_pcAudioTypeSelection->SetEnable( true );

		m_pcVideoList->Clear();
		m_pcVideoTypeSelection->Clear();
		/* m_pcVideoTypeSelection->AppendItem( "Select disk-type" ); */
		m_pcVideoTypeSelection->AppendItem( "NOT SUPPORTED" );
		m_pcVideoTypeSelection->SetSelection( (int) 0 );
		m_pcVideoTypeSelection->SetEnable( true );

		UpdateTime();
	}

	// Blu-ray		NOT DONE!
	if( m_pcMenuMediatypeBLURAY->IsChecked() == true )
	{
		m_pcAudioList->Clear();
		m_pcAudioTypeSelection->Clear();
		/* m_pcAudioTypeSelection->AppendItem( "Select disk-type" ); */
		m_pcAudioTypeSelection->AppendItem( "NOT SUPPORTED" );
		m_pcAudioTypeSelection->SetSelection( (int) 0 );
		m_pcAudioTypeSelection->SetEnable( true );

		m_pcVideoList->Clear();
		m_pcVideoTypeSelection->Clear();
		/* m_pcVideoTypeSelection->AppendItem( "Select disk-type" ); */
		m_pcVideoTypeSelection->AppendItem( "NOT SUPPORTED" );
		m_pcVideoTypeSelection->SetSelection( (int) 0 );
		m_pcVideoTypeSelection->SetEnable( true );

		UpdateTime();
	}
}


//----------------------------------------------------------------------------
// NAME: Flemming H. Sørensen
// DESC: Update the total Audio/Video time
// NOTE:
// SEE ALSO: Add/Remove Audio/Video
//----------------------------------------------------------------------------
void MainWindow::UpdateTime()
{
	int64 nLength = 0;
	int32 nH, nM, nS;
	int8 nCount = 0;
	char zTime[8];

	// Get the number of Rows.
	if( cDiskAuthor == "audio" )
		nCount = m_pcAudioList->GetRowCount();
	if( cDiskAuthor == "video" )
		nCount = m_pcVideoList->GetRowCount();

	// If there's one or more rows, we will read the time from them...
	if( nCount > 0 )
	{
		for( int8 counter = 0; counter < nCount; counter = counter + 1)
		{
			// Read the row we want to get the time from...
			ListViewStringRow* pcRow;
			if( cDiskAuthor == "audio" )
				pcRow = static_cast<os::ListViewStringRow*>( m_pcAudioList->GetRow( counter ) );
			if( cDiskAuthor == "video" )
				pcRow = static_cast<os::ListViewStringRow*>( m_pcVideoList->GetRow( counter ) );

			// ...and convert it to seconds.
			sprintf( zTime, "%s", pcRow->GetString(0).c_str() );
			nLength = nLength + atoi( strtok( zTime, ":" ) ) * 3600;
			nLength = nLength + atoi( strtok( NULL, ":" ) ) * 60;
			nLength = nLength + atoi( strtok( NULL, ":" ) );
		}
	}

	// Convert the time to Hours, Minutes, and Seconds.
	secs_to_hms( nLength, &nH, &nM, &nS );
	sprintf( zTime, "%.2i:%.2i:%.2i", nH, nM, nS );
	os::String cTime = zTime;

	// Write te converted value, to the TextView.
	if( cDiskAuthor == "audio" )
		m_pcAudioTime->SetValue( cTime );
	if( cDiskAuthor == "video" )
		m_pcVideoTime->SetValue( cTime );
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------
BitmapImage* MainWindow::LoadImageFromResource( String zResource )
{
	BitmapImage *pcImage = new BitmapImage();
	Resources cRes( get_image_id() );
	ResStream *pcStream = cRes.GetResourceStream( zResource );
	pcImage->Load( pcStream );
	delete ( pcStream );
	return pcImage;
}


//----------------------------------------------------------------------------
// NAME:
// DESC: Quit
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------
bool MainWindow::OkToQuit()
{
	if( m_pcMenuSaveOnExit->IsChecked() == true )
		SaveSettings();
	os::Application::GetInstance()->PostMessage( os::M_QUIT );
	return( true );
}
