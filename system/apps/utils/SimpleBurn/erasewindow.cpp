#include "erasewindow.h"
using namespace os;

//----------------------------------------------------------------------------
// NAME: Flemming H. Sørensen
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------
EraseWindow::EraseWindow( const os::String& cArgv ) : os::Window( os::Rect( 0, 0, 300, 100 ), "simpleburn_erase", "Erase Disk", WND_NOT_RESIZABLE | WND_NO_DEPTH_BUT | WND_NO_ZOOM_BUT )
{
	// Set the GUI.
	os::LayoutView* pcView = new os::LayoutView( GetBounds(), "layout_view" );
	m_pcRoot = new os::VLayoutNode( "Root" );
	m_pcVRoot = new os::VLayoutNode( "VRoot", 1.000000 );
	m_pcVRoot->SetBorders( os::Rect( 5.000000, 0.000000, 5.000000, 5.000000 ) );
	m_pcRoot->AddChild( m_pcVRoot );
	m_pcDeviceNode = new os::HLayoutNode( "DeviceNode", 1.000000 );
	m_pcDeviceNode->SetBorders( os::Rect( 0.000000, 0.000000, 0.000000, 0.000000 ) );
	m_pcVRoot->AddChild( m_pcDeviceNode );
	m_pcDeviceString = new os::StringView( os::Rect(), "DeviceString", "Device:" );
	m_pcDeviceNode->AddChild( m_pcDeviceString, 1.000000 );
	m_pcDeviceDropdown = new os::DropdownMenu( os::Rect(), "DeviceDropdown" );
	m_pcDeviceDropdown->SetMinPreferredSize( 10 );
	m_pcDeviceDropdown->SetReadOnly( true );
	m_pcDeviceDropdown->AppendItem( "Choose Device..." );
	m_pcDeviceDropdown->SetSelection( (int) 0 );
	m_pcDeviceNode->AddChild( m_pcDeviceDropdown, 1.000000 );
	m_pcButtonNode = new os::HLayoutNode( "ButtonNode", 0.000001 );
	m_pcButtonNode->SetBorders( os::Rect( 0.000000, 0.000000, 0.000000, 0.000000 ) );
	m_pcVRoot->AddChild( m_pcButtonNode );
	m_pcButtonSpacer = new os::HLayoutSpacer( "ButtonSpacer", 0.000000, 100000.000000, NULL, 1.000000 );
	m_pcButtonNode->AddChild( m_pcButtonSpacer );
	os::File cEraseButtonFile( open_image_file( get_image_id() ) );
	os::Resources cEraseButtonResources( &cEraseButtonFile );
	os::ResStream* pcEraseButtonStream = cEraseButtonResources.GetResourceStream( "burn.png" );
	m_pcEraseButton = new os::ImageButton( os::Rect(), "EraseButton", "Erase", new os::Message( M_ERASE_ERASE ), new os::BitmapImage( pcEraseButtonStream ), os::ImageButton::IB_TEXT_RIGHT, true, true, false );
	delete( pcEraseButtonStream );
	m_pcButtonNode->AddChild( m_pcEraseButton, 0.000001 );
	pcView->SetRoot( m_pcRoot );
	AddChild( pcView );
	
	// Set an application icon for Dock.
	os::BitmapImage *pcImage = new os::BitmapImage();
	os::Resources cRes( get_image_id() );
	pcImage->Load( cRes.GetResourceStream( "icon24x24.png" ) );
	SetIcon(pcImage->LockBitmap());
	m_pcIcon = pcImage;   // Set an alertbox icon, too.

	// Set Fast or Full erase-mode
	if( cArgv == "full" )
		cEraseMode = os::String( "all" );
	else
		cEraseMode = os::String( "fast" );

	// Scan for devices.
	DeviceScan();
}


//----------------------------------------------------------------------------
// NAME: Flemming H. Sørensen
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------
void EraseWindow::HandleMessage( os::Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case M_ERASE_ERASE:
		{
			EraseDisk();
			break;
		}
		break;
	}
}


//----------------------------------------------------------------------------
// NAME: Flemming H. Sørensen
// DESC: Erase the disk
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------
void EraseWindow::EraseDisk()
{
	os::String cError;
	int iError = 0;

	// Get the path to the burning device.
	int nDrive = m_pcDeviceDropdown->GetSelection();
	os::String cDrive = m_pcDeviceDropdown->GetItem( nDrive );
	if( nDrive == 0 || cDrive == "" )
	{
		cError =  os::String( "No device selected for Erasing..." );
		iError = iError + 1;
	}

	// Erase the disk...
	if( iError > 0 )		// ...if the user had selected a devie. The user hadn't, so we tell him/her to do so.
	{
		os::Alert* pcBurnNoWriteDevice = new os::Alert( "SimpleBurn - Error", cError , m_pcIcon->LockBitmap(), 0, "_Ok", NULL );
		pcBurnNoWriteDevice->Go( new os::Invoker( 0 ) );
	}
	else					// ...As I said; Erase the disk.
	{
		// Set the execute command.
		os::String cExec = os::String( "cdrecord gracetime=2 -force blank=" ) + cEraseMode + os::String( " dev=" ) + cDrive;

		//  Erase the disk.
		int nError = 0;
		nError = system( cExec.c_str() );

		// Success or Failure?
		if( nError != 0 )			// Failure.
		{
			os::Alert* pcAlert = new os::Alert( "SimpleBurn - Error", "Could not erase the disk...", m_pcIcon->LockBitmap(),0, "_Ok",NULL);
			pcAlert->Go( new os::Invoker( 0 ) );
		}
		else						// Success.
		{
			os::Alert* pcAlert = new os::Alert( "SimpleBurn", "The disk has been erased...", m_pcIcon->LockBitmap(),0, "_Ok",NULL);
			pcAlert->Go( new os::Invoker( 0 ) );
		}
	}
}


//----------------------------------------------------------------------------
// NAME: Flemming H. Sørensen
// DESC: Scan for devices
// NOTE: I'm not sure this is the best way to do it, but it works...
// SEE ALSO:
//----------------------------------------------------------------------------
void EraseWindow::DeviceScan()
{
	// Set device-dir.
	os::String cDevPath = os::String( "/dev/disk" );

	// Open up device/disk directory and check it contains something.
	DIR *pDir = opendir( cDevPath.c_str() );
	if( pDir == NULL )
	{
		return;
	}

	// Loop through directory and add each device to list.
	int i = 0;
	dirent *psEntry;
	while( ( psEntry = readdir( pDir ) ) != NULL )
	{
		// If it's a special directory (i.e. dot(.) and dotdot(..) then ignore it.
		if( strcmp( psEntry->d_name, "." ) == 0 || strcmp( psEntry->d_name, ".." ) == 0 )
		{
			continue;
		}
		else
		{
			// Set new device-dir.
			os::String cDevDiskPath = cDevPath + os::String( "/" ) + psEntry->d_name;

			// Open up device/disk directory and check if it contains something.
			DIR *pTempDir = opendir( cDevDiskPath.c_str() );
			if( pTempDir == NULL )
			{
				return;
			}

			// Loop through directory and add each device to list.
			int i = 0;
			dirent *psEntry;
			while( ( psEntry = readdir( pTempDir ) ) != NULL )
			{
				// If special it's a directory (i.e. dot(.) and dotdot(..) then ignore it.
				if( strcmp( psEntry->d_name, "." ) == 0 || strcmp( psEntry->d_name, ".." ) == 0 )
				{
					continue;
				}
				else
				{
					// Set new device-dir.
					os::String cDevDiskFinalPath = cDevDiskPath + os::String( "/" ) + psEntry->d_name;

					// Open up device/disk directory and check if it contains something.
					DIR *pTempFinalDir = opendir( cDevDiskFinalPath.c_str() );
					if( pTempFinalDir == NULL )
					{
						return;
					}

					// We only want CD-devices.
					/* if( strcmp( os::String( psEntry->d_name ).c_str(), os::String( "cd" ).c_str() ) == 0 ) */	// For some reason, that doesn't work :-(
					char cDevDir[2];
					snprintf( cDevDir, 3, "%s", psEntry->d_name );
					os::String cDevTemp = cDevDir;
					if( cDevTemp == "cd" )
					{
						// Finally we add it to the DropdownMenu.
						cDevDiskFinalPath = cDevDiskFinalPath + os::String( "/raw" );
						struct stat f__stat;
						if( ( stat( cDevDiskFinalPath.c_str(), &f__stat ) == 0 ) == true )
						{
								m_pcDeviceDropdown->AppendItem( cDevDiskFinalPath );
						}
					}
/*					closedir( pTempFinalDir ); */		// For some reason I have to remove this one, to make it work. Not sure why.
				}
				++i;
			}
			closedir( pTempDir );
		}
		++i;
	}
	closedir( pDir );
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------
BitmapImage* EraseWindow::LoadImageFromResource( String zResource )
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
bool EraseWindow::OkToQuit()
{
	PostMessage(new Message(M_ERASE_CANCEL),this);
	return( true );
}

