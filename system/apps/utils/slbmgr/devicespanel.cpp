
/*
 *  SyllableManager - System Manager for Syllable
 *  Copyright (C) 2001 John Hall
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
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <termios.h>
#include <assert.h>
#include <sys/ioctl.h>

#include <gui/button.h>
#include <gui/treeview.h>
#include <gui/stringview.h>
#include <gui/desktop.h>

#include <atheos/device.h>
#include <atheos/types.h>
#include <atheos/kernel.h>

#include <signal.h>

#include <util/application.h>
#include <util/message.h>

#include <macros.h>

#include "devicespanel.h"

#include "resources/SysInfo.h"

using namespace os;

enum
{
	ID_REFRESH
};

//----------------------------------------------------------------------------

DevicesPanel::DevicesPanel( const Rect & cFrame ):LayoutView( cFrame, "", NULL, CF_FOLLOW_NONE )
{
	VLayoutNode *pcRoot = new VLayoutNode( "root" );


	m_pcRefreshBut = new Button( Rect( 0, 0, 0, 0 ), "refresh_but", MSG_TAB_DEVICES_BUTTON_REFRESH, new Message( ID_REFRESH ), CF_FOLLOW_NONE );
	m_pcDevicesList = new TreeView( Rect( 0, 0, 0, 0 ), "devices_list", ListView::F_RENDER_BORDER | ListView::F_NO_AUTO_SORT );


	
	m_pcDevicesList->InsertColumn( MSG_TAB_DEVICES_DEVICE.c_str(), (int)GetBounds().Width() );
	

	pcRoot->AddChild( m_pcDevicesList, 1.0f );
	pcRoot->AddChild( m_pcRefreshBut, 0.0f );

	m_pcDevicesList->SetTabOrder();
	m_pcRefreshBut->SetTabOrder();

	pcRoot->SetBorders( Rect( 10.0f, 10.0f, 10.0f, 10.0f ), "devices_list", NULL );
	pcRoot->SetBorders( Rect( 10.0f, 5.0f, 10.0f, 5.0f ), "refresh_but", NULL );
	SetRoot( pcRoot );

}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DevicesPanel::AllAttached()
{
	m_pcDevicesList->SetTarget( this );
	m_pcRefreshBut->SetTarget( this );
	RefreshDevicesList();
}


//----------------------------------------------------------------------------

char g_zGroups[][255] = {
	"Unknown/Unsupported devices",
	"System devices",
	"Controllers",
	"Video devices",
	"Audio devices",
	"Network devices",
	"Ports",
	"Input devices",
	"Drives",
	"Processors"
};

uint g_nGroupCount = 10;

void DevicesPanel::RefreshDevicesList()
{
	/* Open */
	int nFd = open( "/dev/devices", O_RDWR );

	if ( nFd < 0 )
		return;
		
	std::vector<DeviceInfo_s> asDevices;
	DeviceInfo_s sDevice;
	/* Read entries */
	while( ioctl( nFd, 0, &sDevice ) == 0 )
	{
		asDevices.push_back( sDevice );
	}
	close( nFd );

	m_pcDevicesList->Clear();
	
	
	/* Add computer node */
	char zBuf[255];
	gethostname( zBuf, 255 );
	os::TreeViewStringNode* pcRootNode = new os::TreeViewStringNode();
	pcRootNode->AppendString( os::String( MSG_TAB_DEVICES_TREE_COMPUTER.c_str() ) +  os::String( " - " ) + os::String( zBuf ) );
	pcRootNode->SetIndent( 1 );
	m_pcDevicesList->InsertNode( pcRootNode );
	pcRootNode->SetExpanded( true );
	
	
	/* Build list */
	for( uint i = 0; i < g_nGroupCount; i++ )
	{
		bool bGroupCreated = false;
		for( uint j = 0; j < asDevices.size(); j++ )
		{
			if( asDevices[j].di_eType == (int)i )
			{
				if( !bGroupCreated )
				{
					os::TreeViewStringNode* pcRow = new os::TreeViewStringNode();
					pcRow->AppendString( g_zGroups[i] );
					pcRow->SetIndent( 2 );
					
					m_pcDevicesList->InsertNode( pcRow );
					bGroupCreated = true;
				}
				
				os::TreeViewStringNode* pcRow = new os::TreeViewStringNode();
				pcRow->AppendString( String( asDevices[j].di_zName ) );
				pcRow->SetIndent( 3 );
				m_pcDevicesList->InsertNode( pcRow );
			}
		}
	}
	
	asDevices.clear();
}

void DevicesPanel::HandleMessage( Message * pcMessage )
{
	switch ( pcMessage->GetCode() )
	{
	case ID_REFRESH:
		{
			RefreshDevicesList();
			break;
		}
	default:
		View::HandleMessage( pcMessage );
		break;
	}
}


