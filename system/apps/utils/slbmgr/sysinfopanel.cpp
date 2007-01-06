
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

#include "sysinfopanel.h"
#include "resources/SysInfo.h"

using namespace os;

enum
{
	ID_END
};

const char *gApp_Date = __DATE__;

void human( char *pzBuffer, off_t nValue )
{
	if( nValue > ( 1024 * 1024 * 1024 ) )
	{
		sprintf( pzBuffer, MSG_TAB_SYSINFO_GIGABYTE.c_str(), ( ( double )nValue ) / ( 1024.0 * 1024.0 * 1024.0 ) );
	}
	else if( nValue > ( 1024 * 1024 ) )
	{
		sprintf( pzBuffer, MSG_TAB_SYSINFO_MEGABYTE.c_str(), ( ( double )nValue ) / ( 1024.0 * 1024.0 ) );
	}
	else if( nValue > 1024 )
	{
		sprintf( pzBuffer, MSG_TAB_SYSINFO_KILOBYTE.c_str(), ( ( double )nValue ) / 1024.0 );
	}
	else
	{
		sprintf( pzBuffer, MSG_TAB_SYSINFO_BYTE.c_str(), ( ( double )nValue ) );
	}
}

//---------------------------------------------------------------------------
SysInfoPanel::SysInfoPanel( const Rect & cFrame ):LayoutView( cFrame, "", NULL, CF_FOLLOW_NONE )
{
	VLayoutNode *pcRoot = new VLayoutNode( "root" );

	//HLayoutNode* pcTime  = new HLayoutNode( "time" );


	pcRoot->ExtendMaxSize( Point( 0.0f, MAX_SIZE ) );

	m_pcVersionView = new ListView( Rect( 0, 0, 0, 0 ), "version_info", ListView::F_RENDER_BORDER | ListView::F_NO_AUTO_SORT );
	m_pcCPUView = new ListView( Rect( 0, 0, 0, 0 ), "cpu_info", ListView::F_RENDER_BORDER | ListView::F_NO_AUTO_SORT );
	m_pcMemoryView = new ListView( Rect( 0, 0, 0, 0 ), "mem_info", ListView::F_RENDER_BORDER | ListView::F_NO_AUTO_SORT );
	m_pcAdditionView = new ListView( Rect( 0, 0, 0, 0 ), "add_info", ListView::F_RENDER_BORDER | ListView::F_NO_AUTO_SORT );


	m_pcUptimeView = new TextView( Rect( 0, 0, 0, 0 ), "uptime_view", "Test string", CF_FOLLOW_LEFT | CF_FOLLOW_TOP, WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );

	m_pcUptime = new StringView( Rect( 0, 0, 0, 0 ), "tm", "Uptime: DDD:HH:MM:SS", ALIGN_CENTER, WID_WILL_DRAW );

	/* Configuring m_pcTestView */
	m_pcUptimeView->SetMultiLine( false );
	m_pcUptimeView->SetReadOnly( true );
	//m_pcTestView->SetMinPreferredSize ( 225.0f, 250.0f );
	//m_pcTestView->SetMaxPreferredSize ( 800.0f, 900.0f );

    /****************************/

	pcRoot->AddChild( m_pcVersionView, 2.0f );
	pcRoot->AddChild( m_pcCPUView, 1.0f );
	pcRoot->AddChild( m_pcMemoryView, 3.0f );
	pcRoot->AddChild( m_pcAdditionView, 3.0f );

	pcRoot->AddChild( m_pcUptime, 0.0f );

	//pcRoot->AddChild( m_pcUptimeView );

	SetUpVersionView();
	SetUpCPUView();
	SetUpMemoryView();
	SetUpAdditionView();

	pcRoot->SetBorders( Rect( 5.0f, 5.0f, 5.0f, 5.0f ), "version_info", "cpu_info", "mem_info", "add_info", "uptime_view", NULL );
	SetRoot( pcRoot );

	//m_bDetail = false;
}

void SysInfoPanel::SetUpVersionView()
{
	m_pcVersionView->InsertColumn( MSG_TAB_SYSINFO_VER_NAME.c_str(), 100 );
	m_pcVersionView->InsertColumn( MSG_TAB_SYSINFO_VER_VERSION.c_str(), 80 );
	m_pcVersionView->InsertColumn( MSG_TAB_SYSINFO_VER_BUILDDATE.c_str(), 80 );
}

/**
 *  Quick hack.  Need to make it more useable in the future:  John Hall 8/18/2001
 **/
ListViewStringRow *SysInfoPanel::AddRow( char *pzCol1, char *pzCol2, char *pzCol3, int nRows )
{
	ListViewStringRow *pcRow = new ListViewStringRow();

	if( nRows >= 1 )
		pcRow->AppendString( pzCol1 );
	if( nRows >= 2 )
		pcRow->AppendString( pzCol2 );
	if( nRows >= 3 )
		pcRow->AppendString( pzCol3 );

	return ( pcRow );
}

void SysInfoPanel::SetUpCPUView()
{
	m_pcCPUView->InsertColumn( MSG_TAB_SYSINFO_CPU_CPU.c_str(), 40 );
	m_pcCPUView->InsertColumn( MSG_TAB_SYSINFO_CPU_CORESPEED.c_str(), 80 );
	m_pcCPUView->InsertColumn( MSG_TAB_SYSINFO_CPU_BUSSPEED.c_str(), 80 );
	m_pcCPUView->InsertColumn( MSG_TAB_SYSINFO_CPU_FEATURES.c_str(), 100 );
}

void SysInfoPanel::SetUpMemoryView()
{
	m_pcMemoryView->InsertColumn( MSG_TAB_SYSINFO_MEM_MEMTYPE.c_str(), 120 );
	m_pcMemoryView->InsertColumn( MSG_TAB_SYSINFO_MEM_AMOUNT.c_str(), 100 );
}

void SysInfoPanel::SetUpAdditionView()
{
	m_pcAdditionView->InsertColumn( MSG_TAB_SYSINFO_ADD_ADDINFO.c_str(), 150 );
	m_pcAdditionView->InsertColumn( MSG_TAB_SYSINFO_ADD_AMOUNT.c_str(), 100 );
}

void SysInfoPanel::UpdateAdditionalInfo( bool bUpdate )
{
	char szBfr[128];

	static struct SlbMgrAdditionalInfo {
		char	*pzName;
		char	*pzFormat;
		int		*pnArg;
		int		nOld;
	} addinfo[] = {
		{ "Page Faults",		"%d",	&m_sSysInfo.nPageFaults,		0 },
		{ "Used Semaphores",	"%d",	&m_sSysInfo.nUsedSemaphores,	0 },
		{ "Used Ports",			"%d",	&m_sSysInfo.nUsedPorts,			0 },
		{ "Used Threads",		"%d",	&m_sSysInfo.nUsedThreads,		0 },
		{ "Used Processes",		"%d",	&m_sSysInfo.nUsedProcesses,		0 },
		{ "Open File Count",	"%d",	&m_sSysInfo.nOpenFileCount,		0 },
		{ "Allocated INodes",	"%d",	&m_sSysInfo.nAllocatedInodes,	0 },
		{ "Loaded INodes",		"%d",	&m_sSysInfo.nLoadedInodes,		0 },
		{ "Used INodes",		"%d",	&m_sSysInfo.nUsedInodes,		0 },
	};

	for( int x = 0; x < NUM_OF_ADDITIONAL_ROWS; x++ )
	{
		if( !bUpdate || ( (*(addinfo[x].pnArg)) != addinfo[x].nOld ) ) {
			addinfo[x].nOld = *(addinfo[x].pnArg);

			sprintf( szBfr, addinfo[x].pzFormat, addinfo[x].nOld );

			ListViewStringRow *pcRow;

			if( !bUpdate ) {
				pcRow = new ListViewStringRow();
		
				pcRow->AppendString( addinfo[x].pzName );
				pcRow->AppendString( szBfr );

				m_pcAdditionView->InsertRow( pcRow );
			} else {
				ListViewStringRow* pcRow = dynamic_cast<ListViewStringRow*>( m_pcAdditionView->GetRow( x ) );

				if( pcRow ) {				
					pcRow->SetString( 0, addinfo[x].pzName );
					pcRow->SetString( 1, szBfr );
	
					m_pcAdditionView->InvalidateRow( x, ListView::INV_VISUAL );
				}
			}
		}
	}
}

void SysInfoPanel::UpdateMemoryInfo( bool bUpdate )
{
	static struct SlbMgrMemoryInfo {
		char	*pzName;
		int		*pnArg;
		bool	bInPages;
		long	nOld;
	} meminfo[] = {
		{ "Max. Memory",		&m_sSysInfo.nMaxPages,			true,	0	},
		{ "Free Memory",		&m_sSysInfo.nFreePages,			true,	0	},
		{ "Kernel Memory",		&m_sSysInfo.nKernelMemSize,		false,	0	},
		{ "Block Cache",		&m_sSysInfo.nBlockCacheSize,	false,	0	},
		{ "Dirty Cache",		&m_sSysInfo.nDirtyCacheSize,	false,	0	},
	};

	char szBfr[128];

	for( int x = 0; x < NUM_OF_MEMORY_ROWS; x++ )
	{
		if( !bUpdate || ( meminfo[x].nOld != *(meminfo[x].pnArg) ) ) {
			meminfo[x].nOld = *(meminfo[x].pnArg);
			human( szBfr, ( meminfo[x].bInPages ? PAGE_SIZE : 1 ) * meminfo[x].nOld );
			
			ListViewStringRow *pcRow;

			if( !bUpdate ) {
				pcRow = new ListViewStringRow();
		
				pcRow->AppendString( meminfo[x].pzName );
				pcRow->AppendString( szBfr );

				m_pcMemoryView->InsertRow( pcRow );
			} else {
				ListViewStringRow* pcRow = dynamic_cast<ListViewStringRow*>( m_pcMemoryView->GetRow( x ) );

				if( pcRow ) {				
					pcRow->SetString( 0, meminfo[x].pzName );
					pcRow->SetString( 1, szBfr );
	
					m_pcMemoryView->InvalidateRow( x, ListView::INV_VISUAL );
				}
			}					
		}
	}
}

//----------------------------------------------------------------------------
void SysInfoPanel::AllAttached()
{
	SetupPanel();
}

//----------------------------------------------------------------------------
void SysInfoPanel::FrameSized( const Point & cDelta )
{
	LayoutView::FrameSized( cDelta );
}

/*
 *  DEPRECATED:  John Hall 8/19/2001
void SysInfoPanel::SetDetail(bool bVal)
{
  bool bOld = m_bDetail;

  m_bDetail = bVal;
  if( bOld != bVal )
    UpdateText();
}
 * 
 */


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

/*void SysInfoPanel::KeyDown( const char *pzString, const char *pzRawString, uint32 nQualifiers )
{
	switch ( pzString[0] )
	{
	case VK_UP_ARROW:
		break;
	case VK_DOWN_ARROW:
		break;
	case VK_SPACE:
		break;
	default:
		View::KeyDown( pzString, pzRawString, nQualifiers );
		break;
	}
}*/

//----------------------------------------------------------------------------
/*void SysInfoPanel::Paint( const Rect & cUpdateRect )
{
	Rect cBounds = GetBounds();

	SetFgColor( get_default_color( COL_NORMAL ) );
	FillRect( cBounds );
}*/

void SysInfoPanel::UpdateUptime( bool bUpdate )
{
	int nDays, nHour, nMin, nSec;
	char szTmp[1024];

	bigtime_t nUpTime;

	nUpTime = get_system_time();

	nDays = nUpTime / ( 1000000LL * 60LL * 60LL * 24LL );
	nUpTime -= nDays * ( 1000000LL * 60LL * 60LL * 24LL );

	nHour = nUpTime / ( 1000000LL * 60LL * 60LL );
	nUpTime -= nHour * ( 1000000LL * 60LL * 60LL );
	nMin = nUpTime / ( 1000000LL * 60LL );
	nUpTime -= nMin * ( 1000000LL * 60LL );
	nSec = nUpTime / 1000000LL;

 /*** SETTING THE UPTIME ***/
	sprintf( szTmp, MSG_TAB_SYSINFO_UPTIME_UPTIME.c_str(), nDays, nHour, nMin, nSec );

	m_pcUptime->SetString( szTmp );
	//m_pcUptimeView->Clear();
	//m_pcUptimeView->Insert( szTmp );

 /**************************/
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SysInfoPanel::SetupPanel()
{
	system_info sSysInfo;
	utsname uInfo;

	//myCPUInfo   CPUInfo;

	get_system_info( &sSysInfo );
	ListViewStringRow *pcRow = new ListViewStringRow();

	int x, nCPUCount;

	char szRow1[128], szRow2[128], szRow3[128], szRow4[128];

	uname( &uInfo );

	sprintf( szRow1, MSG_TAB_SYSINFO_VER_SYLLABLE.c_str() );
	sprintf( szRow3, "%s", sSysInfo.zKernelBuildDate );

	if( sSysInfo.nKernelVersion & 0xffff000000000000LL )
	{
		sprintf( szRow2, "%d.%d.%d%c", ( int )( ( sSysInfo.nKernelVersion >> 32 ) & 0xffff ), ( int )( ( sSysInfo.nKernelVersion >> 16 ) & 0xffff ), ( int )( sSysInfo.nKernelVersion & 0xffff ), 'a' + ( int )( sSysInfo.nKernelVersion >> 48 ) );
	}
	else
	{
		sprintf( szRow2, "%d.%d.%d", ( int )( ( sSysInfo.nKernelVersion >> 32 ) & 0xffff ), ( int )( ( sSysInfo.nKernelVersion >> 16 ) % 0xffff ), ( int )( sSysInfo.nKernelVersion & 0xffff ) );
	}

    /*********************************************
     *  Done with adding AtheOS's information
     *********************************************/
	pcRow = AddRow( szRow1, szRow2, szRow3, 3 );

	m_pcVersionView->InsertRow( pcRow );

    /*********************************************
     *  Done with adding AtheMgr's Information
     *********************************************/

	sprintf( szRow1, MSG_TAB_SYSINFO_VER_SLBMGR.c_str() );
	sprintf( szRow2, "%s", APP_VERSION );
	sprintf( szRow3, "%s", gApp_Date );

	pcRow = AddRow( szRow1, szRow2, szRow3, 3 );

	m_pcVersionView->InsertRow( pcRow );

    /*********************************************************************** 
     * Adding CPU information since I just found out that that information
     * is currently available through the kernel:  John Hall:  May 4, 2001
     ***********************************************************************/
	nCPUCount = sSysInfo.nCPUCount;


    /*******************************************************************************
     * As it stands now, Kurt only gets CPU info (and stores it in the system_info
     * data if the machine is multi-processor'd.  Until I can figure a way around
     * this limitation or Kurt adds single CPU data to the system_info, I will
     * use this if():  John Hall (5/9/01)
     *******************************************************************************/



	for( x = 0; x < nCPUCount; x++ )
	{
		ListViewStringRow *pcRow = new ListViewStringRow();


		sprintf( szRow1, "%d", x );
		sprintf( szRow2, MSG_TAB_SYSINFO_MHZ.c_str(), ( sSysInfo.asCPUInfo[x].nCoreSpeed + 500000 ) / 1000000 );
		sprintf( szRow3, MSG_TAB_SYSINFO_MHZ.c_str(), ( sSysInfo.asCPUInfo[x].nBusSpeed + 500000 ) / 1000000 );
		strcpy( szRow4, "" );

		if( sSysInfo.nCPUType & CPU_FEATURE_MMX )
			strcat( szRow4, "MMX " );
		if( sSysInfo.nCPUType & CPU_FEATURE_MMX2 )
			strcat( szRow4, "MMX2 " );
		if( sSysInfo.nCPUType & CPU_FEATURE_3DNOW )
			strcat( szRow4, "3DNow " );
		if( sSysInfo.nCPUType & CPU_FEATURE_3DNOWEX )
			strcat( szRow4, "3DNowEx " );
		if( sSysInfo.nCPUType & CPU_FEATURE_SSE )
			strcat( szRow4, "SSE " );
		if( sSysInfo.nCPUType & CPU_FEATURE_SSE2 )
			strcat( szRow4, "SSE2 " );

		pcRow->AppendString( szRow1 );
		pcRow->AppendString( szRow2 );
		pcRow->AppendString( szRow3 );
		pcRow->AppendString( szRow4 );

		//pcRow = AddRow( szRow1, szRow2, szRow3, "", 4 );

		m_pcCPUView->InsertRow( pcRow );
	}

	get_system_info( &m_sSysInfo );

	//cout << "HERE\n";
	UpdateMemoryInfo( false );
	//cout << "HERE2\n";

	UpdateAdditionalInfo( false );
	UpdateUptime( false );

	//m_pcUptimeView->Clear();
	//m_pcUptimeView->Insert( szTmp );
}

void SysInfoPanel::UpdateSysInfoPanel()
{
	if( !IsVisible() )
		return;

	get_system_info( &m_sSysInfo );
	UpdateMemoryInfo( true );
	UpdateAdditionalInfo( true );
	UpdateUptime( true );
}

void SysInfoPanel::HandleMessage( Message * pcMessage )
{
	switch ( pcMessage->GetCode() )
	{
		//case ID_END:   break;
	default:
		View::HandleMessage( pcMessage );
		break;
	}
}



