/*
 *  Syllable Printer configuration preferences
 *
 *  (C)opyright 2006 - Kristian Van Der Vliet
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>

#include <printers.h>
#include <print_view.h>

#include "resources/Printers.h"

using namespace os;

#include <iostream>

PrintConfigView::PrintConfigView( const Rect &cFrame, CUPS_Printer *pcPrinter, Window *pcParent ) : View( cFrame, "print_config_view", CF_FOLLOW_ALL )
{
	m_pcPrinter = pcPrinter;
	m_pcModelWindow = NULL;
	m_pcParent = pcParent;
	m_hPPD = NULL;

	if( m_pcPrinter->cName != "" )
	{
		/* Load the PPD for this printer */
		const char *zPPDPath = cupsGetPPD( m_pcPrinter->cName.c_str() );
		if( zPPDPath != NULL )
		{
			FILE *hPPDFile = fopen( zPPDPath, "rb" );
			if( hPPDFile != NULL )
			{
				m_hPPD = ppdOpen( hPPDFile );
				fclose( hPPDFile );
			}
			unlink( zPPDPath );
		}
	}

	if( NULL == m_hPPD && m_pcPrinter->cName == "" )
	{
		m_pcPrinter->cName = MSG_MAINWND_DEFAULT_PRINTER_NAME;
		if( m_pcPrinter->cInfo == "" )
			m_pcPrinter->cInfo = m_pcPrinter->cName;
	}

	m_pcModelFrame = new FrameView( Rect(), "print_config_model_frame", MSG_MAINWND_PRINTERTAB_MODEL, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT );

	/* Full printer model name */
	String cModel, cProtocol;
	if( m_hPPD )
	{
		ppd_attr_t *psAttr;

		psAttr = ppdFindAttr( m_hPPD, "ModelName", NULL );
		if( psAttr != NULL )
			cModel = psAttr->value;
		else
			cModel = pcPrinter->cInfo;
	}
	else
		cModel = MSG_MAINWND_PRINTERTAB_UNKNOWN;

	m_pcModel = new StringView( Rect(), "print_config_model", cModel );
	m_pcModelFrame->AddChild( m_pcModel );

	Font *pcFont;
	font_properties cFontProperties;

	pcFont = new Font();
	pcFont->GetDefaultFont( DEFAULT_FONT_BOLD, &cFontProperties );
	pcFont->SetProperties( cFontProperties );
	m_pcModel->SetFont( pcFont );
	pcFont->Release();

	m_pcChange = new Button( Rect(), "print_config_select", MSG_MAINWND_PRINTERTAB_SELECT, new Message( M_CHANGE ), CF_FOLLOW_RIGHT );
	m_pcModelFrame->AddChild( m_pcChange );

	AddChild( m_pcModelFrame );

	m_pcLabel1 = new StringView( Rect(), "print_config_label1", MSG_MAINWND_PRINTERTAB_THEPRINTER );
	AddChild( m_pcLabel1 );

	m_pcName = new TextView( Rect(), "print_config_name", m_pcPrinter->cName.c_str(), CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT );
	AddChild( m_pcName );

	m_pcLabel2 = new StringView( Rect(), "print_config_label2", MSG_MAINWND_PRINTERTAB_ISA );
	AddChild( m_pcLabel2 );

	m_pcProtocol = new DropdownMenu( Rect(), "print_config_protocol" );
	m_pcProtocol->AppendItem( MSG_PROTOCOL_USB );
	m_pcProtocol->AppendItem( MSG_PROTOCOL_PARALLEL );
	m_pcProtocol->AppendItem( MSG_PROTOCOL_SMB );
	m_pcProtocol->AppendItem( MSG_PROTOCOL_LPD );
	m_pcProtocol->AppendItem( MSG_PROTOCOL_IPP );
	m_pcProtocol->AppendItem( MSG_PROTOCOL_HPJETDIRECT );
	m_pcProtocol->AppendItem( MSG_PROTOCOL_UNKNOWN );

	m_pcProtocol->SetSelectionMessage( new Message( M_PROTOCOL_CHANGED ) );

	AddChild( m_pcProtocol );

	/* Type of connection */
	GetProtocol();
	m_pcProtocol->SetSelection( (int)m_nProtocol );

	m_pcLabel3 = new StringView( Rect(), "print_config_label3", MSG_MAINWND_PRINTERTAB_CONNECTEDTO );
	AddChild( m_pcLabel3 );

	/* Location */
	String cDevicePath;
	int nDefault;

	m_pcUSBDevices = new DropdownMenu( Rect(), "print_config_usb_devices", CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT );
	m_pcUSBDevices->AppendItem( "/dev/printer/usb/0" );
	nDefault = 0;
	for( int n=1; n < 4; n++ )
	{
		cDevicePath.Format( "/dev/printer/usb/%d", n );
		if( access( cDevicePath.c_str(), R_OK|W_OK ) == 0 )
		{
			m_pcUSBDevices->AppendItem( cDevicePath );
			if( m_pcPrinter->cDevice == cDevicePath )
				nDefault = n;
		}
	}
	m_pcUSBDevices->SetSelection( nDefault );

	m_pcParallelDevices = new DropdownMenu( Rect(), "print_config_parallel_devices", CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT );
	nDefault = 0;
	for( int n=0; n < 4; n++ )
	{
		cDevicePath.Format( "/dev/printer/lp/%d", n );
		if( access( cDevicePath.c_str(), R_OK|W_OK ) == 0 )
		{
			m_pcParallelDevices->AppendItem( cDevicePath );
			if( m_pcPrinter->cDevice == cDevicePath )
				nDefault = n;
		}
	}
	m_pcParallelDevices->SetSelection( nDefault );

	m_pcRemoteDevice = new TextView( Rect(), "print_config_remote_device", NULL, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT );
	if( m_nProtocol != PROT_USB && m_nProtocol != PROT_PARALLEL )
		m_pcRemoteDevice->Set( m_pcPrinter->cDevice.c_str() );

	m_pcDevice = NULL;
	DisplayDevice();
}

PrintConfigView::~PrintConfigView()
{
	if( m_hPPD )
		ppdClose( m_hPPD );
}

void PrintConfigView::AttachedToWindow( void )
{
	Layout();
	View::AttachedToWindow();
}

void PrintConfigView::AllAttached( void )
{
	View::AllAttached();
	m_pcName->SetTarget( this );
	m_pcChange->SetTarget( this );
	m_pcProtocol->SetTarget( this );
	m_pcUSBDevices->SetTarget( this );
	m_pcParallelDevices->SetTarget( this );
	m_pcRemoteDevice->SetTarget( this );
}

void PrintConfigView::HandleMessage( Message *pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case M_CHANGE:
		{
			if( NULL == m_pcModelWindow )
			{
				m_pcModelWindow = new ModelWindow( Rect( 0, 0, 500, 450 ), this );
				m_pcModelWindow->CenterInWindow( m_pcParent );
				m_pcModelWindow->Show();
			}
			m_pcModelWindow->MakeFocus();

			break;
		}

		case M_MODEL_WINDOW_CLOSED:
		{
			m_pcModelWindow = NULL;

			String cModel, cPPD;
			if( pcMessage->FindString( "model", &cModel ) == EOK )
				if( pcMessage->FindString( "ppd", &cPPD ) == EOK )
				{
					m_pcModel->SetString( cModel );
					m_pcPrinter->cPPD = cPPD;

					String cName = m_pcName->GetBuffer()[0];
					if( cName == MSG_MAINWND_DEFAULT_PRINTER_NAME || cName == "" )
						GenerateName( cModel );

					Layout();
				}

			break;
		}

		case M_PROTOCOL_CHANGED:
		{
			m_nProtocol = (protocol_e)m_pcProtocol->GetSelection();
			DisplayDevice();
			break;
		}

		default:
			View::HandleMessage( pcMessage );
	}
}

void PrintConfigView::SetDefault( bool bDefault )
{
	m_pcPrinter->bDefault = bDefault;
	/* XXXKV: Update display */
}

CUPS_Printer * PrintConfigView::Save( void )
{
	String cName = m_pcName->GetBuffer()[0];
	if( cName == "" )
		cName = MSG_MAINWND_DEFAULT_PRINTER_NAME;
	m_pcPrinter->cName = cName;
	m_pcPrinter->cInfo = cName;

	switch( m_nProtocol )
	{
		case PROT_USB:
		{
			m_pcPrinter->cProtocol = "usb";
			m_pcPrinter->cDevice = m_pcUSBDevices->GetCurrentString();
			break;
		}

		case PROT_PARALLEL:
		{
			m_pcPrinter->cProtocol = "parallel";
			m_pcPrinter->cDevice = m_pcParallelDevices->GetCurrentString();
			break;
		}

		case PROT_SMB:
		{
			m_pcPrinter->cProtocol = "smb";
			m_pcPrinter->cDevice = m_pcRemoteDevice->GetBuffer()[0];
			break;
		}

		case PROT_LPD:
		{
			m_pcPrinter->cProtocol = "lpd";
			m_pcPrinter->cDevice = m_pcRemoteDevice->GetBuffer()[0];
			break;
		}

		case PROT_IPP:
		{
			m_pcPrinter->cProtocol = "ipp";
			m_pcPrinter->cDevice = m_pcRemoteDevice->GetBuffer()[0];
			break;
		}

		case PROT_SOCKET:
		{
			m_pcPrinter->cProtocol = "socket";
			m_pcPrinter->cDevice = m_pcRemoteDevice->GetBuffer()[0];
			break;
		}

		case PROT_UNKNOWN:
		{
			m_pcPrinter->cProtocol = "";
			m_pcPrinter->cDevice = m_pcRemoteDevice->GetBuffer()[0];
			break;
		}
	}

	if( m_nProtocol != PROT_SMB )
	{
		m_pcPrinter->cUser = "";
		m_pcPrinter->cPass = "";
	}

	/* Rebuild the DeviceURI from it's components */
	m_pcPrinter->BuildURI();

	return m_pcPrinter;
}

void PrintConfigView::Layout( void )
{
	Rect cItemBounds, cFrameBounds;

	Rect cBounds = GetBounds();
	cBounds.left += 10;
	cBounds.top += 20;
	cBounds.right -= 10;
	cBounds.bottom -= 10;

	/* Position the "Model" FrameView and everything inside of it */
	cFrameBounds = cBounds;

	cFrameBounds.bottom = cFrameBounds.top + m_pcModelFrame->GetPreferredSize( false ).y + 30;
	m_pcModelFrame->SetFrame( cFrameBounds );

	cFrameBounds = m_pcModelFrame->GetBounds();
	cFrameBounds.left += 10;
	cFrameBounds.top += 20;
	cFrameBounds.right -= 10;
	cFrameBounds.bottom -= 10;

	cItemBounds = cFrameBounds;

	cItemBounds.right = m_pcModel->GetPreferredSize( false ).x + 10;
	cItemBounds.bottom = cItemBounds.top + m_pcChange->GetPreferredSize( false ).y;
	m_pcModel->SetFrame( cItemBounds );

	cItemBounds.right = cFrameBounds.right;
	cItemBounds.left = cFrameBounds.right - m_pcChange->GetPreferredSize( false ).x;
	m_pcChange->SetFrame( cItemBounds );

	/* Position the rest of the Controls & View */
	cItemBounds = cBounds;
	cItemBounds.top = m_pcModelFrame->GetFrame().bottom + 10;

	cItemBounds.bottom = cItemBounds.top + m_pcProtocol->GetPreferredSize( false ).y;
	cItemBounds.right = m_pcLabel1->GetPreferredSize( false ).x + 10;
	m_pcLabel1->SetFrame( cItemBounds );

	cItemBounds.left = cItemBounds.right + 10;
	cItemBounds.right = cBounds.right - 10;
	m_pcName->SetFrame( cItemBounds );

	cItemBounds.left = cBounds.left;
	cItemBounds.right = cItemBounds.left + m_pcLabel2->GetPreferredSize( false ).x + 10;
	cItemBounds.top = cItemBounds.bottom + 10;
	cItemBounds.bottom = cItemBounds.top + m_pcProtocol->GetPreferredSize( false ).y;

	m_pcLabel2->SetFrame( cItemBounds );

	cItemBounds.left = cItemBounds.right;
	cItemBounds.right = cItemBounds.left + m_pcProtocol->GetPreferredSize( false ).x;
	m_pcProtocol->SetFrame( cItemBounds );

	cItemBounds.left = cItemBounds.right + 10;
	cItemBounds.right = cBounds.right - 10;
	m_pcLabel3->SetFrame( cItemBounds );

	float vHeight;
	vHeight = m_pcUSBDevices->GetPreferredSize( false ).y;
	vHeight = std::max( vHeight, m_pcParallelDevices->GetPreferredSize( false ).y );
	vHeight = std::max( vHeight, m_pcRemoteDevice->GetPreferredSize( false ).y );

	cItemBounds.left = cBounds.left;
	cItemBounds.top = cItemBounds.bottom + 10;
	cItemBounds.bottom = cItemBounds.top + vHeight;
	cItemBounds.right = cBounds.right;

	m_pcUSBDevices->SetFrame( cItemBounds );
	m_pcParallelDevices->SetFrame( cItemBounds );
	m_pcRemoteDevice->SetFrame( cItemBounds );
}

void PrintConfigView::GetProtocol( void )
{
	if( m_pcPrinter->cProtocol == "usb" )
		m_nProtocol = PROT_USB;
	else if( m_pcPrinter->cProtocol == "parallel" )
		m_nProtocol = PROT_PARALLEL;
	else if( m_pcPrinter->cProtocol == "smb" )
		m_nProtocol = PROT_SMB;
	else if( m_pcPrinter->cProtocol == "lpd" )
		m_nProtocol = PROT_LPD;
	else if( m_pcPrinter->cProtocol == "ipp" )
		m_nProtocol = PROT_IPP;
	else if( m_pcPrinter->cProtocol == "socket" )
		m_nProtocol = PROT_SOCKET;
	else
		m_nProtocol = PROT_UNKNOWN;
}

void PrintConfigView::DisplayDevice( void )
{
	if( NULL != m_pcDevice )
		RemoveChild( m_pcDevice );

	switch( m_nProtocol )
	{
		case PROT_USB:
		{
			m_pcDevice = m_pcUSBDevices;
			break;
		}

		case PROT_PARALLEL:
		{
			m_pcDevice = m_pcParallelDevices;
			break;
		}

		case PROT_SMB:
		case PROT_LPD:
		case PROT_IPP:
		case PROT_SOCKET:
		case PROT_UNKNOWN:
		{
			m_pcDevice = m_pcRemoteDevice;
			break;
		}
	}
	AddChild( m_pcDevice );
}

/* Generate a suitable queue name from the printer model */
void PrintConfigView::GenerateName( String cModel )
{
	const char *zFrom;
	char *zBuffer, *zTo;

	zBuffer = (char*)calloc( 1, cModel.size() + 1 );
	if( zBuffer == NULL )
		return;

	zTo = zBuffer;
	zFrom = cModel.c_str();
	while( *zFrom != '\0' )
	{
		if( *zFrom != ' ' )
			*zTo++ = *zFrom;
		else
			*zTo++ = '_';

		zFrom++;
	}

	m_pcName->Set( zBuffer );
	free( zBuffer );
}

PrintView::PrintView( const Rect &cFrame, CUPS_Printer *pcPrinter, Window *pcParent ) : View( cFrame, "print_view", CF_FOLLOW_ALL )
{
	Rect cBounds = GetBounds();

	m_pcTabView = new TabView( cBounds, "print_view_tab", CF_FOLLOW_ALL );

	m_pcConfigView = new PrintConfigView( Rect(), pcPrinter, pcParent );

	m_pcTabView->AppendTab( pcPrinter->cName, m_pcConfigView );
	AddChild( m_pcTabView );
}

PrintView::~PrintView()
{
	RemoveChild( m_pcTabView );
	delete( m_pcTabView );
}

void PrintView::SetDefault( bool bDefault )
{
	m_pcConfigView->SetDefault( bDefault );
}

CUPS_Printer * PrintView::Save( void )
{
	CUPS_Printer *pcPrinter = m_pcConfigView->Save();

	/* Ensure the Tab carries the correct title */
	m_pcTabView->SetTabTitle( 0, pcPrinter->cName );

	return pcPrinter;
}

