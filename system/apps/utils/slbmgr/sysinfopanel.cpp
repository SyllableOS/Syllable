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

using namespace os;

enum
{
  ID_END
};

const char* gApp_Date = __DATE__;

void human( char* pzBuffer, off_t nValue )
{
    if ( nValue > (1024*1024*1024) ) {
	sprintf( pzBuffer, "%.1f Gb", ((double)nValue) / (1024.0*1024.0*1024.0) );
    } else if ( nValue > (1024*1024) ) {
	sprintf( pzBuffer, "%.1f Mb", ((double)nValue) / (1024.0*1024.0) );
    } else if ( nValue > 1024 ) {
	sprintf( pzBuffer, "%.1f Kb", ((double)nValue) / 1024.0 );
    } else {
	sprintf( pzBuffer, "%.1f b", ((double)nValue) );
    }
}

/*
   Note:  I can't get this function to work 'cause I don't know what values
 *        APIC_TDCR, APIC_TDCR_MAKE, APIC_TDR_DIV_1, APIC_TMICT, APIC_TMCCT
 *        should be.  Also, I don't know how access to the read_pentium_clock()
 *        stuff.  If I can get Kurt to implement CPUInfo for single CPU machines
 *        or get this data from Kurt, I'll finish this funciton:  John Hall (5/9/01)
 
 void get_CPUInfo( myCPUInfo* CPU ){
   
    uint32 nAPICCount;
    uint64 nStartPerf;
    uint64 nEndPerf;
  
    uint32 nReg;
  
    nReg  = apic_read( APIC_TDCR ) & ~APIC_TDCR_MASK;
    nReg |= APIC_TDR_DIV_1;
    apic_write( APIC_TDCR, nReg );

    isa_writeb( 0x43, 0x34 );
    isa_writeb( 0x40, 0xff );
    isa_writeb( 0x40, 0xff );
  

    wait_pit_wrap();
    apic_write( APIC_TMICT, ~0 ); // Start APIC timer
    nStartPerf = read_pentium_clock();
    wait_pit_wrap();
    nAPICCount = apic_read( APIC_TMCCT );
    nEndPerf = read_pentium_clock();

    CPU->nBusSpeed = 
       ((uint32)((uint64)PIT_TICKS_PER_SEC * (0xffffffffLL - nAPICCount) / 0xffff) +
        500000) / 1000000;
    CPU->nCoreSpeed = 
       ((uint32)((uint64)PIT_TICKS_PER_SEC * (nEndPerf - nStartPerf) / 0xffff) +
        500000) / 1000000;
   //cout << " " << endl;
}*/

//---------------------------------------------------------------------------
SysInfoPanel::SysInfoPanel( const Rect& cFrame ) : LayoutView( cFrame, "", NULL, CF_FOLLOW_NONE )
{
    VLayoutNode* pcRoot  = new VLayoutNode( "root" );

    //HLayoutNode* pcTime  = new HLayoutNode( "time" );
   
   
    pcRoot->ExtendMaxSize( Point( 0.0f, MAX_SIZE ) );

    m_pcVersionView  = new ListView( Rect( 0, 0, 0, 0 ), "version_info", ListView::F_RENDER_BORDER | ListView::F_NO_AUTO_SORT );
    m_pcCPUView      = new ListView( Rect( 0, 0, 0, 0 ), "cpu_info", ListView::F_RENDER_BORDER | ListView::F_NO_AUTO_SORT );
    m_pcMemoryView   = new ListView( Rect( 0, 0, 0, 0 ), "mem_info", ListView::F_RENDER_BORDER | ListView::F_NO_AUTO_SORT );
    m_pcHDView       = new ListView( Rect( 0, 0, 0, 0 ), "hd_info",  ListView::F_RENDER_BORDER | ListView::F_NO_AUTO_SORT );
    m_pcAdditionView = new ListView( Rect( 0, 0, 0, 0 ), "add_info", ListView::F_RENDER_BORDER | ListView::F_NO_AUTO_SORT );
   
    
    m_pcUptimeView = new TextView( Rect( 0, 0, 0, 0 ), "uptime_view", "Test string", CF_FOLLOW_LEFT | CF_FOLLOW_TOP, WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );

    m_pcUptime = new StringView( Rect( 0, 0, 0, 0 ), "tm" , "Uptime: DDD:HH:MM:SS", ALIGN_LEFT, WID_WILL_DRAW );

    /* Configuring m_pcTestView */
    m_pcUptimeView->SetMultiLine( false );
    m_pcUptimeView->SetReadOnly( true );
    //m_pcTestView->SetMinPreferredSize ( 225.0f, 250.0f );
    //m_pcTestView->SetMaxPreferredSize ( 800.0f, 900.0f );
    /****************************/
  
    pcRoot->AddChild( m_pcVersionView , 2.0f );
    pcRoot->AddChild( m_pcCPUView     , 1.0f );
    pcRoot->AddChild( m_pcMemoryView  , 3.0f );
    pcRoot->AddChild( m_pcHDView      , 1.0f );
    pcRoot->AddChild( m_pcAdditionView, 3.0f );
  
    pcRoot->AddChild( m_pcUptime );
 
    //pcRoot->AddChild( m_pcUptimeView );
 
    SetUpVersionView();
    SetUpCPUView();
    SetUpMemoryView();
    SetUpHDView();
    SetUpAdditionView();
   
    pcRoot->SetBorders( Rect(5.0f, 5.0f, 5.0f, 5.0f), "version_info","cpu_info","mem_info",
		                                      "hd_info","add_info", "uptime_view", NULL );
    SetRoot( pcRoot );

    //m_bDetail = false;
}

void SysInfoPanel::SetUpVersionView(){
    m_pcVersionView->InsertColumn( "Name", 100 );
    m_pcVersionView->InsertColumn( "Number", 80 );
    m_pcVersionView->InsertColumn( "Build Date", 80 );
}
/**
 *  Quick hack.  Need to make it more useable in the future:  John Hall 8/18/2001
 **/
ListViewStringRow* SysInfoPanel::AddRow( char* pzCol1, char* pzCol2, char* pzCol3, int nRows ){
    ListViewStringRow* pcRow = new ListViewStringRow();
    
    if( nRows >= 1 )
      pcRow->AppendString( pzCol1 );
    if( nRows >= 2 )
      pcRow->AppendString( pzCol2 );
    if( nRows >= 3 )
      pcRow->AppendString( pzCol3 );
   
    return( pcRow );
}
				       
void SysInfoPanel::SetUpCPUView(){
    m_pcCPUView->InsertColumn( "CPU Number", 100 );
    m_pcCPUView->InsertColumn( "Core Speed", 100 );
    m_pcCPUView->InsertColumn( "Bus Speed" , 100 );
}

void SysInfoPanel::SetUpMemoryView(){
    m_pcMemoryView->InsertColumn( "Memory Type", 120 );
    m_pcMemoryView->InsertColumn( "Amount", 100 );
}
void SysInfoPanel::SetUpHDView(){
    m_pcHDView->InsertColumn( "Volume", 78 );
    m_pcHDView->InsertColumn( "Type",   50 );
    m_pcHDView->InsertColumn( "Size",   62 );
    m_pcHDView->InsertColumn( "Used",   75 );
    m_pcHDView->InsertColumn( "Avail",  58 );
    m_pcHDView->InsertColumn( "Percent Free", 80 );
}
void SysInfoPanel::SetUpAdditionView(){
    m_pcAdditionView->InsertColumn( "Additional Info", 150 );
    m_pcAdditionView->InsertColumn( "Amount", 100 );
}

void SysInfoPanel::UpdateHDInfo( bool bUpdate ){
    fs_info     fsInfo;
   
    int		 nMountCount;

    char        szTmp[1024];  char zSize[64];
    char        zUsed[64];   char zAvail[64];
   
    char        szRow1[128],szRow2[128],szRow3[128],szRow4[128],
                szRow5[128],szRow6[128];

    /*********************************************************************** 
     * Adding drive information:  John Hall:  May 4, 2001
     * However, nMountCount may be off.  I can't tell if it returns all
     * IDE drives (CD-ROMs included) found through the BIOS or just what 
     * AtheOS considers "mountable."
     ***********************************************************************/
    int x = get_mount_point_count();
    nMountCount = 0;
    for( int i = 0; i < x; i++ ){
      if( get_mount_point( i, szTmp, PATH_MAX ) < 0 )
	continue;
       
      int nFD = open( szTmp, O_RDONLY );
      if( nFD < 0 )
        continue;
      
      if( get_fs_info( nFD, &fsInfo ) >= 0 )
	nMountCount++;
       
      close( nFD );
    }
   
    for ( int i = 0 ; i < nMountCount ; ++i ) {
      if ( get_mount_point( i, szTmp, PATH_MAX ) < 0 ) {
        continue;
      }
 
      int nFD = open( szTmp, O_RDONLY );
      if ( nFD < 0 ) {
        continue;
      }
  
      if ( get_fs_info( nFD, &fsInfo ) >= 0 ) {
        human( zSize, fsInfo.fi_total_blocks * fsInfo.fi_block_size );
        human( zUsed, (fsInfo.fi_total_blocks - fsInfo.fi_free_blocks) *fsInfo.fi_block_size );
        human( zAvail, fsInfo.fi_free_blocks * fsInfo.fi_block_size );

	ListViewStringRow* pcRow = new ListViewStringRow();
	
	sprintf( szRow1, "%s", fsInfo.fi_volume_name );
	sprintf( szRow2, "%s", fsInfo.fi_driver_name );
	sprintf( szRow3, "%s", zSize  );
	sprintf( szRow4, "%s", zUsed  );
	sprintf( szRow5, "%s", zAvail );
	sprintf( szRow6, "%.1f%%", ((double)fsInfo.fi_free_blocks / ((double)fsInfo.fi_total_blocks)) * 100.0 );

        pcRow->AppendString( szRow1 );
	pcRow->AppendString( szRow2 );
	pcRow->AppendString( szRow3 );
	pcRow->AppendString( szRow4 );
	pcRow->AppendString( szRow5 );
	pcRow->AppendString( szRow6 );

        off_tHDSize[i][NEW_VALUE] = fsInfo.fi_free_blocks;
	
        if( !bUpdate ){	
          m_pcHDView->InsertRow( pcRow );
          off_tHDSize[i][OLD_VALUE] = off_tHDSize[i][NEW_VALUE];
        }
        else{
          if( off_tHDSize[i][OLD_VALUE] != off_tHDSize[i][NEW_VALUE] ){
            delete m_pcHDView->RemoveRow( i ); // Remove & delete this row
            m_pcHDView->InsertRow( i, pcRow ); // Insert the row thus updating it
            m_pcHDView->InvalidateRow( i, ListView::INV_VISUAL );  

            off_tHDSize[i][OLD_VALUE] = off_tHDSize[i][NEW_VALUE];
          }
        }
          
      }
      close( nFD );
    }
}

void SysInfoPanel::UpdateAdditionalInfo( bool bUpdate ) {
     char szRow1[128],szRow2[128],szRow3[128];

     system_info	sSysInfo;
     get_system_info( &sSysInfo );

     for( int x = 0; x < NUM_OF_ADDITIONAL_ROWS; x++ ){
	ListViewStringRow* pcRow = new ListViewStringRow();

        switch( x ){
          case 0:
            sprintf( szRow1, "Pagefaults" );
            sprintf( szRow2, "%d", sSysInfo.nPageFaults );
       
            pcRow = AddRow( szRow1, szRow2, szRow3, 2 );

            nAddInfo[x][NEW_VALUE] = sSysInfo.nPageFaults;

            break;
          case 1:
            sprintf( szRow1, "Used Semaphores" );
            sprintf( szRow2, "%d", sSysInfo.nUsedSemaphores );
       
            pcRow = AddRow( szRow1, szRow2, szRow3, 2 );

            nAddInfo[x][NEW_VALUE] = sSysInfo.nPageFaults;

            break;
          case 2:
            sprintf( szRow1, "Used Ports" );
            sprintf( szRow2, "%d", sSysInfo.nUsedPorts );
       
            pcRow = AddRow( szRow1, szRow2, szRow3, 2 );

            nAddInfo[x][NEW_VALUE] = sSysInfo.nPageFaults;

            break;
          case 3:
            sprintf( szRow1, "Used Threads" );
            sprintf( szRow2, "%d", sSysInfo.nUsedThreads );
       
            pcRow = AddRow( szRow1, szRow2, szRow3, 2 );

            nAddInfo[x][NEW_VALUE] = sSysInfo.nPageFaults;

            break;
          case 4:
            sprintf( szRow1, "Used Processes" );
            sprintf( szRow2, "%d", sSysInfo.nUsedProcesses );
       
            pcRow = AddRow( szRow1, szRow2, szRow3, 2 );

            nAddInfo[x][NEW_VALUE] = sSysInfo.nPageFaults;

            break;
          case 5:
            sprintf( szRow1, "Open File Count" );
            sprintf( szRow2, "%d", sSysInfo.nOpenFileCount );
       
            pcRow = AddRow( szRow1, szRow2, szRow3, 2 );

            nAddInfo[x][NEW_VALUE] = sSysInfo.nPageFaults;

            break;
          case 6:
            sprintf( szRow1, "Allocated Inodes" );
            sprintf( szRow2, "%d", sSysInfo.nAllocatedInodes );
       
            pcRow = AddRow( szRow1, szRow2, szRow3, 2 );

            nAddInfo[x][NEW_VALUE] = sSysInfo.nPageFaults;

            break;
          case 7:
            sprintf( szRow1, "Loaded Inodes" );
            sprintf( szRow2, "%d", sSysInfo.nLoadedInodes );
       
            pcRow = AddRow( szRow1, szRow2, szRow3, 2 );

            nAddInfo[x][NEW_VALUE] = sSysInfo.nPageFaults;

            break;
          case 8:
            sprintf( szRow1, "Used Inodes" );
            sprintf( szRow2, "%d", sSysInfo.nUsedInodes );
       
            pcRow = AddRow( szRow1, szRow2, szRow3, 2 );

            nAddInfo[x][NEW_VALUE] = sSysInfo.nPageFaults;

            break;
        }

        if( !bUpdate ){	
          m_pcAdditionView->InsertRow( pcRow );
          nAddInfo[x][OLD_VALUE] = nAddInfo[x][NEW_VALUE];
        }
        else{
          if( nAddInfo[x][OLD_VALUE] != nAddInfo[x][NEW_VALUE] ){
            delete m_pcAdditionView->RemoveRow( x ); // Remove & delete this row
            m_pcAdditionView->InsertRow( x, pcRow ); // Insert the row thus updating it
            m_pcAdditionView->InvalidateRow( x, ListView::INV_VISUAL );  

            nAddInfo[x][OLD_VALUE] = nAddInfo[x][NEW_VALUE];
          }
        }

        
      } /* end of for( int x = 0; x < 9; x++ ) */
       
}

void SysInfoPanel::UpdateMemoryInfo( bool bUpdate ) {
    system_info sSysInfo;
    get_system_info( &sSysInfo );
   
    char szRow1[128],szRow2[128],szRow3[128];

    char        *pzMaxMem;
    char        *pzFreeMem;
    char        *pzKernel;
    char        *pzBlockCache;
    char        *pzDirtyCache;  

    float       vMaxMem  = float(sSysInfo.nMaxPages      * PAGE_SIZE);
    float       vFreeMem = float(sSysInfo.nFreePages     * PAGE_SIZE);
    float       vKernel  = float(sSysInfo.nKernelMemSize);
    float       vBlock   = float(sSysInfo.nBlockCacheSize);
    float       vDirty   = float(sSysInfo.nDirtyCacheSize);

    if( vDirty > 1000000.0f ){
      vDirty  /= 1000000.0f;
      pzDirtyCache = "Mb";
    }
    else if( vDirty > 1000.0f ){
      vDirty  /= 1000.0f;
      pzDirtyCache = "Kb";
    }
    else
      pzDirtyCache = "b";
   
    if( vBlock > 1000000.0f ){
      vBlock  /= 1000000.0f;
      pzBlockCache = "Mb";
    }
    else if( vBlock > 1000.0f ){
      vBlock  /= 1000.0f;
      pzBlockCache = "Kb";
    }
    else
      pzBlockCache = "b";
   
    if(vMaxMem > 1000000.0f){
      vMaxMem /= 1000000.0f;
      pzMaxMem = "Mb";
    }
    else if(vMaxMem > 1000.0f){
      vMaxMem /= 1000.0f;
      pzMaxMem = "Kb";
    }
    else
      pzMaxMem = "b";
   
    if(vFreeMem > 1000000.0f){
      vFreeMem /= 1000000.f;
      pzFreeMem = "Mb";
    }
    else if(vFreeMem > 1000.0f){
      vFreeMem /= 1000.f;
      pzFreeMem = "Kb";
    }
    else
      pzFreeMem = "b";
      
    if(vKernel > 1000000.0f){
      vKernel /= 1000000.f;
      pzKernel = "Mb";
    }
    else if(vKernel > 1000.0f){
      vKernel /= 1000.0f;
      pzKernel = "Kb";
    }
    else
      pzKernel = "b";

    for( int x = 0; x < NUM_OF_MEMORY_ROWS; x++ ){
	ListViewStringRow* pcRow = new ListViewStringRow();

        switch( x ){
          case 0:
            sprintf( szRow1, "Max. Memory"   );
            sprintf( szRow2, "%.2f %s", vMaxMem, pzMaxMem );
       
            pcRow = AddRow( szRow1, szRow2, szRow3, 2 );

            /****
             *  Getting the memory for this round...
             ****/
            fMemory[x][NEW_VALUE] = vMaxMem;

            break;
          case 1:
            sprintf( szRow1, "Free memory"    );
            sprintf( szRow2, "%.2f %s", vFreeMem, pzFreeMem );
       
            pcRow = AddRow( szRow1, szRow2, szRow3, 2 );

            /****
             *  Getting the memory for this round...
             ****/
            fMemory[x][NEW_VALUE] = vFreeMem;

            break;
          case 4:
            sprintf( szRow1, "Kernel Memory" );
            sprintf( szRow2, "%.2f %s", vKernel, pzKernel );
       
            pcRow = AddRow( szRow1, szRow2, szRow3, 2 );

            /****
             *  Getting the memory for this round...
             ****/
            fMemory[x][NEW_VALUE] = vKernel;

            break;
          case 2:
            sprintf( szRow1, "Block Cache"   );
            sprintf( szRow2, "%.2f %s", vBlock, pzBlockCache );
       
            pcRow = AddRow( szRow1, szRow2, szRow3, 2 );

            /****
             *  Getting the memory for this round...
             ****/
            fMemory[x][NEW_VALUE] = vBlock;

            break;
          case 3:
            sprintf( szRow1, "Dirty Cache"   );
            sprintf( szRow2, "%.2f %s", vDirty, pzDirtyCache );
       
            pcRow = AddRow( szRow1, szRow2, szRow3, 2 );

            /****
             *  Getting the memory for this round...
             ****/
            fMemory[x][NEW_VALUE] = vDirty;

            break;
        }

        if( !bUpdate ){	
          m_pcMemoryView->InsertRow( pcRow );

          /***
           *  Store this value...
           ***/
          fMemory[x][OLD_VALUE] = fMemory[x][NEW_VALUE];
        }
        else{
          if( fMemory[x][NEW_VALUE] != fMemory[x][OLD_VALUE] ){
            delete m_pcMemoryView->RemoveRow( x ); // Remove & delete this row
            m_pcMemoryView->InsertRow( x, pcRow ); // Insert the row thus updating it
            //m_pcMemoryView->InvalidateRow( x, ListView::INV_VISUAL );  

            /***
             *  Store this value...
             ***/
            fMemory[x][OLD_VALUE] = fMemory[x][NEW_VALUE];
          }
        }  
     } /* end of for( int x = 0; x < 9; x++ ) */     
}

//----------------------------------------------------------------------------
void SysInfoPanel::AllAttached()
{
    SetupPanel();
}

//----------------------------------------------------------------------------
void SysInfoPanel::FrameSized( const Point& cDelta )
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

void SysInfoPanel::KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers )
{
    switch( pzString[0] )
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
}

//----------------------------------------------------------------------------
void SysInfoPanel::Paint( const Rect& cUpdateRect )
{
    Rect cBounds = GetBounds();

    SetFgColor( get_default_color( COL_NORMAL ) );
    FillRect( cBounds );
}

void SysInfoPanel::UpdateUptime( bool bUpdate ){
  int  nDays, nHour, nMin, nSec;
  char szTmp[1024];

  bigtime_t nUpTime;
   
  nUpTime = get_system_time();

  nDays    = nUpTime / (1000000LL * 60LL * 60LL * 24LL);
  nUpTime -= nDays * (1000000LL * 60LL * 60LL * 24LL);

  nHour    = nUpTime / (1000000LL * 60LL * 60LL);
  nUpTime -= nHour * (1000000LL * 60LL * 60LL); 
  nMin     = nUpTime / (1000000LL * 60LL);
  nUpTime -= nMin * (1000000LL * 60LL);
  nSec     = nUpTime / 1000000LL;

 /*** SETTING THE UPTIME ***/
  sprintf( szTmp, "Running For: %3d days, %2d hrs, %2d mins, %2d secs", 
                   nDays, nHour, nMin, nSec );

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
    system_info	sSysInfo;
    utsname     uInfo;
    //myCPUInfo   CPUInfo;
   
    get_system_info( &sSysInfo );
    ListViewStringRow* pcRow = new ListViewStringRow();
   
    int		x, nCPUCount;

    char        szTmp[512];
   
    char        szRow1[128],szRow2[128],szRow3[128];

/*
   Quote From Kurt Skauen in an email to the list:
   RE: [Atheos-developer] Kernel Version Info from 05/01/2001 12:37:47 

   "All released kernels [upto 0.3.4] will return 0x10001 so it is not possible to
   convert the number into a reasonable version-string. I have updated
   the kernel now so the next release will have a sane version-number."

   In the next release [after 0.3.4] you can print the version [in the same manner
   as the else( uname != "0.1.1" || uname != "0.3.4")]"

   John Hall, May 4, 2001
*/

    uname( &uInfo );

    sprintf( szRow1, "Syllable" );
    sprintf( szRow3, "%s", sSysInfo.zKernelBuildDate );
   
    /*****************************************************************************
     * No AtheOS version number shown for versions before 0.3.4 since there is no 
     * reliable data within the kernel or uname(). 
     *****************************************************************************/
    if( strcmp( uInfo.version, "0.1.1") == 0 || strcmp( uInfo.version, "1.1.1" ) == 0 )
      sprintf( szRow2, " " );

    /*****************************************************************************
     *  Since the kernel version number is "sane", I'm using only it, since this
     *  format will also support "special" releases (e.g. 0.3.3b).
     * 
     *  Additional note:  I was having one heck of time trying to figure out how
     *                    to differiate from 0.3.4 and 0.3.5 since the uInfo.version
     *                    number is the same, and since the nKernelVersion wasn't
     *                    actually being used until 0.3.6, I had to rely on the
     *                    uInfo.version.  My solution?  0.3.5 was built on
     *                    "Jun 23 2001" where as 0.3.4 was built earlier, obviously.
     *
     *                    John Hall 8/18/2001
     *****************************************************************************/
    else if( strcmp( uInfo.version, "0.3.4" ) == 0 && 
	     strcmp( sSysInfo.zKernelBuildDate, "Jun 23 2001" ) == 0 ){
      if ( sSysInfo.nKernelVersion & 0xffff000000000000LL ) {
      	sprintf( szRow2, "%d.%d.%d%c",
		(int)((sSysInfo.nKernelVersion >> 32) & 0xffff),
		(int)((sSysInfo.nKernelVersion >> 16) & 0xffff),
		(int)(sSysInfo.nKernelVersion & 0xffff), 'a' + (int)(sSysInfo.nKernelVersion >> 48) );
      } 
      else {
	sprintf( szRow2, "%d.%d.%d",
		(int)((sSysInfo.nKernelVersion >> 32) & 0xffff),
		(int)((sSysInfo.nKernelVersion >> 16) & 0xffff),
		(int)(sSysInfo.nKernelVersion & 0xffff) );
      }
    }
    
    /*****************************************************************************
     *  if uname() returns "0.3.4", use it.  It is being used in this case because
     *  the kernel version number is not "sane".
     *****************************************************************************/
    else if( strcmp( uInfo.version, "0.3.4" ) == 0 )
      sprintf( szRow2,  "%s", uInfo.version );
    else{
      if( sSysInfo.nKernelVersion & 0xffff000000000000LL ){
	sprintf( szRow2, "%d.%d.%d%c",
		  (int)((sSysInfo.nKernelVersion >> 32) & 0xffff),
		  (int)((sSysInfo.nKernelVersion >> 16) & 0xffff),
		  (int)(sSysInfo.nKernelVersion & 0xffff), 'a' + (int)(sSysInfo.nKernelVersion >> 48) );
      }
      else{
	sprintf( szRow2, "%d.%d.%d",
		  (int)((sSysInfo.nKernelVersion >> 32) & 0xffff),
		  (int)((sSysInfo.nKernelVersion >> 16) % 0xffff),
		  (int)(sSysInfo.nKernelVersion & 0xffff) );
      }
    }

    /*********************************************
     *  Done with adding AtheOS's information
     *********************************************/
    pcRow = AddRow( szRow1, szRow2, szRow3, 3 );
   
    m_pcVersionView->InsertRow( pcRow );

    /*********************************************
     *  Done with adding AtheMgr's Information
     *********************************************/
    
    sprintf( szRow1, "SlbMgr" );
    sprintf( szRow2, "%s", APP_VERSION );
    sprintf( szRow3, "%s", gApp_Date   );
   
    pcRow = AddRow( szRow1, szRow2, szRow3, 3 );
   
    m_pcVersionView->InsertRow( pcRow );

    /*********************************************************************** 
     * Adding CPU information since I just found out that that information
     * is currently available through the kernel:  John Hall:  May 4, 2001
     ***********************************************************************/
    nCPUCount = sSysInfo.nCPUCount;
    
    if( nCPUCount == 1 ){
      ListViewStringRow* pcRow = new ListViewStringRow();
       
      sprintf( szRow1, "%d", nCPUCount );
      sprintf( szRow2, " " );
      sprintf( szRow3, " " );
       
      pcRow = AddRow( szRow1, szRow2, szRow3, 3 );
       
      m_pcCPUView->InsertRow( pcRow );
    }

    /*******************************************************************************
     * As it stands now, Kurt only gets CPU info (and stores it in the system_info
     * data if the machine is multi-processor'd.  Until I can figure a way around
     * this limitation or Kurt adds single CPU data to the system_info, I will
     * use this if():  John Hall (5/9/01)
     *******************************************************************************/
   
    if( nCPUCount > 1 ){
      
      for( x=0; x < nCPUCount; x++ ){
	ListViewStringRow* pcRow = new ListViewStringRow();
	 
        //get_CPUInfo( &CPUInfo );
	sprintf( szRow1, "%d", x );
	sprintf( szRow2, "%lld MHz", sSysInfo.asCPUInfo[x].nCoreSpeed );
	sprintf( szRow3, "%lld MHz", sSysInfo.asCPUInfo[x].nBusSpeed  );
        
	pcRow = AddRow( szRow1, szRow2, szRow3, 3 );
	 
	m_pcCPUView->InsertRow( pcRow );
      }
    }

    //cout << "HERE\n";
    UpdateMemoryInfo( false );
    //cout << "HERE2\n";
    UpdateHDInfo( false );
    //cout << "HERE3\n";
    UpdateAdditionalInfo( false );
    UpdateUptime( false );      

    //sprintf( szTmp,  "Running For: %d days %d hours % d mins %d secs",
    //                  nDays, nHour, nMin, nSec );

    //m_pcUptimeView->Clear();
    //m_pcUptimeView->Insert( szTmp );
}

void SysInfoPanel::UpdateSysInfoPanel(){
  UpdateMemoryInfo( true );
  UpdateHDInfo( true );
  UpdateAdditionalInfo( true );
  UpdateUptime( true );
}

void SysInfoPanel::HandleMessage( Message* pcMessage )
{
    switch( pcMessage->GetCode() )
    {
	//case ID_END:   break;
	default:
	    View::HandleMessage( pcMessage );
	    break;
    }
}

