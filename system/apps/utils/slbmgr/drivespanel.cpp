
/*
 *  AtheMgr - System Manager for AtheOS
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
#include <dirent.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/utsname.h>

#include <gui/button.h>
#include <gui/stringview.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <gui/view.h>
#include <gui/listview.h>

#include <atheos/kernel.h>
#include <atheos/time.h>
#include <atheos/types.h>
#include <atheos/filesystem.h>
#include <atheos/udelay.h>
#include <atheos/irq.h>
#include <atheos/areas.h>
#include <atheos/smp.h>

#include <util/application.h>
#include <util/message.h>
#include <util/appserverconfig.h>

#include "drivespanel.h"
#include "resources/SysInfo.h"

using namespace os;

enum
{
	ID_END
};

extern void human( char *pzBuffer, off_t nValue );

//---------------------------------------------------------------------------
DrivesPanel::DrivesPanel( const Rect & cFrame ):LayoutView( cFrame, "", NULL, CF_FOLLOW_NONE )
{
	VLayoutNode *pcRoot = new VLayoutNode( "root" );

	//HLayoutNode* pcTime  = new HLayoutNode( "time" );


	pcRoot->ExtendMaxSize( Point( 0.0f, MAX_SIZE ) );

	m_pcHDView = new ListView( Rect( 0, 0, 0, 0 ), "hd_info", ListView::F_RENDER_BORDER | ListView::F_NO_AUTO_SORT );

    /****************************/

	pcRoot->AddChild( m_pcHDView, 1.0f );

	SetUpHDView();

	pcRoot->SetBorders( Rect( 5.0f, 5.0f, 5.0f, 5.0f ), "hd_info", NULL );
	SetRoot( pcRoot );

	//m_bDetail = false;
}

DriveRow *DrivesPanel::AddRow( int nRow, off_t nSize, const char *pzCol, ... )
{
	va_list sArgList;
	DriveRow *pcRow = new DriveRow();

	pcRow->nSize = nSize;

	va_start( sArgList, pzCol );

	for( const char* txt = pzCol ; txt != NULL ; txt = va_arg( sArgList, const char* ) )
	{
		pcRow->AppendString( txt );
	}

	va_end( sArgList );

	m_pcHDView->InsertRow( pcRow );

	return ( pcRow );
}

void DrivesPanel::SetUpHDView()
{
	m_pcHDView->InsertColumn( MSG_TAB_DRIVES_VOLUME.c_str(), 78 );
	m_pcHDView->InsertColumn( MSG_TAB_DRIVES_TYPE.c_str(), 50 );
	m_pcHDView->InsertColumn( MSG_TAB_DRIVES_SIZE.c_str(), 62 );
	m_pcHDView->InsertColumn( MSG_TAB_DRIVES_USED.c_str(), 75 );
	m_pcHDView->InsertColumn( MSG_TAB_DRIVES_AVAIL.c_str(), 58 );
	m_pcHDView->InsertColumn( MSG_TAB_DRIVES_PERFREE.c_str(), 80 );
}

void DrivesPanel::UpdateHDInfo( bool bUpdate )
{
	fs_info fsInfo;

	int nMountCount;

	char szTmp[1024];
	char zSize[64];
	char zUsed[64];
	char zAvail[64];

	char szRow1[128], szRow2[128], szRow3[128], szRow4[128], szRow5[128], szRow6[128];

	nMountCount = get_mount_point_count();

	for( int i = 0; i < nMountCount; ++i )
	{
		if( get_mount_point( i, szTmp, PATH_MAX ) < 0 )
		{
			continue;
		}

		int nFD = open( szTmp, O_RDONLY );

		if( nFD < 0 )
		{
			continue;
		}

		if( get_fs_info( nFD, &fsInfo ) >= 0 )
		{
			if( ( off_tHDSize[i] != fsInfo.fi_free_blocks ) || !bUpdate ) {
				off_tHDSize[i] = fsInfo.fi_free_blocks;

				human( zSize, fsInfo.fi_total_blocks * fsInfo.fi_block_size );
				human( zUsed, ( fsInfo.fi_total_blocks - fsInfo.fi_free_blocks ) * fsInfo.fi_block_size );
				human( zAvail, fsInfo.fi_free_blocks * fsInfo.fi_block_size );

				sprintf( szRow1, "%s", fsInfo.fi_volume_name );
				sprintf( szRow2, "%s", fsInfo.fi_driver_name );
				sprintf( szRow3, "%s", zSize );
				sprintf( szRow4, "%s", zUsed );
				sprintf( szRow5, "%s", zAvail );
				sprintf( szRow6, "%.1f%%", ( ( double )fsInfo.fi_free_blocks / ( ( double )fsInfo.fi_total_blocks ) ) * 100.0 );

				if( !bUpdate )
				{
					AddRow( 0, off_tHDSize[i], szRow1, szRow2, szRow3, szRow4, szRow5, szRow6, NULL );
/*					ListViewStringRow *pcRow = new ListViewStringRow();
		
					pcRow->AppendString( szRow1 );
					pcRow->AppendString( szRow2 );
					pcRow->AppendString( szRow3 );
					pcRow->AppendString( szRow4 );
					pcRow->AppendString( szRow5 );
					pcRow->AppendString( szRow6 );

					m_pcHDView->InsertRow( pcRow );*/
				} else {
					ListViewStringRow* pcRow = dynamic_cast<ListViewStringRow*>( m_pcHDView->GetRow( i ) );

					if( pcRow ) {
						pcRow->SetString( 0, szRow1 );
						pcRow->SetString( 1, szRow2 );
						pcRow->SetString( 2, szRow3 );
						pcRow->SetString( 3, szRow4 );
						pcRow->SetString( 4, szRow5 );
						pcRow->SetString( 5, szRow6 );
							
						m_pcHDView->InvalidateRow( i, ListView::INV_VISUAL );
					}
				}
			}

		}
		close( nFD );
	}
}

//----------------------------------------------------------------------------
void DrivesPanel::AllAttached()
{
	SetupPanel();
}

//----------------------------------------------------------------------------
void DrivesPanel::FrameSized( const Point & cDelta )
{
	LayoutView::FrameSized( cDelta );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DrivesPanel::SetupPanel()
{
	UpdateHDInfo( false );
}

void DrivesPanel::UpdatePanel()
{
	if( !IsVisible() )
		return;

	UpdateHDInfo( true );
}

void DrivesPanel::HandleMessage( Message * pcMessage )
{
	switch ( pcMessage->GetCode() )
	{
		//case ID_END:   break;
	default:
		View::HandleMessage( pcMessage );
		break;
	}
}


