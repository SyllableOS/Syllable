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
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <getopt.h>

#include <atheos/types.h>
#include <atheos/time.h>
#include <atheos/kernel.h>

#include <util/application.h>
#include <util/message.h>
#include <util/application.h>
#include <util/message.h>

#include <gui/window.h>
#include <gui/view.h>
#include <gui/stringview.h>
#include <gui/menu.h>

#include "performancepanel.h"
#include "cpuview.h"
#include "memview.h"
#include "w8378x_driver.h"
#include "resources/SysInfo.h"

using namespace os;

enum
{
  ID_QUIT
};

void GetLoad( int nCPUCount, float* pavLoads )
{
    static bigtime_t anLastIdle[MAX_CPU_COUNT];
    static bigtime_t anLastSys[MAX_CPU_COUNT];

    for ( int i = 0 ; i < nCPUCount ; ++i ) {
	bigtime_t	nCurIdleTime;
	bigtime_t	nCurSysTime;

	bigtime_t	nIdleTime;
	bigtime_t	nSysTime;
	
	static float	avLoad[8];

	nCurIdleTime	= get_idle_time( i % 2 );
	nCurSysTime 	= get_real_time();
	
	nIdleTime	= nCurIdleTime - anLastIdle[i];
	nSysTime	= nCurSysTime  - anLastSys[i];

	anLastIdle[i] = nCurIdleTime;
	anLastSys[i]  = nCurSysTime;

	if ( nIdleTime > nSysTime ) {
	    nIdleTime = nSysTime;
	}
  
	if ( nSysTime == 0 ) {
	    nSysTime = 1;
	}

	avLoad[i] += ( (1.0 - double(nIdleTime) / double(nSysTime)) - avLoad[i]) * 0.5f;
	pavLoads[i] = avLoad[i];
    }
}

//----------------------------------------------------------------------------
PerformancePanel::PerformancePanel( const Rect& cFrame ) : LayoutView( cFrame, "", NULL, CF_FOLLOW_NONE )
{

    /* Layouts For Panel */
    VLayoutNode* pcRoot = new VLayoutNode( "root" );
    
    HLayoutNode* pcMem    = new HLayoutNode( "mem", 1.0f, pcRoot );
    HLayoutNode* pcCpu    = new HLayoutNode( "cpu", 2.0f, pcRoot );
    /*********************/

    char pzTmp[256];

    /* CPU Information */
    system_info sInfo;

    get_system_info( &sInfo );
    m_nCPUCount = sInfo.nCPUCount;
    m_nW8378xDevice = open( "/dev/misc/w8378x", O_RDWR );
    /******************/
   
    /* All of the Mem. Fields */ 
    m_pcMemUsage     = new MemMeter( Rect( 0, 0, 0, 0 ), "mem_usage", WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );
    m_pcMemTitle     = new StringView( Rect( 0, 0, 0, 0 ), "mem_title", MSG_TAB_PERFORMANCE_MEMORYUSAGE, ALIGN_CENTER, WID_WILL_DRAW );
    m_pcCpuTitle     = new StringView( Rect( 0, 0, 0, 0 ), "cpu_title", MSG_TAB_PERFORMANCE_PROCESSORUSAGE, ALIGN_CENTER, WID_WILL_DRAW );
    m_pcMemTitleStr1 = new StringView( Rect( 0, 0, 0, 0 ), "mem_title_str1", MSG_TAB_PERFORMANCE_MEMTOTAL, ALIGN_LEFT, WID_WILL_DRAW );
    m_pcMemTitleStr2 = new StringView( Rect( 0, 0, 0, 0 ), "mem_title_str2", MSG_TAB_PERFORMANCE_MEMFREE, ALIGN_LEFT, WID_WILL_DRAW );
    m_pcMemTitleStr3 = new StringView( Rect( 0, 0, 0, 0 ), "mem_title_str3", MSG_TAB_PERFORMANCE_MEMCACHE, ALIGN_LEFT, WID_WILL_DRAW );

    m_pcMemUsageStr1 = new StringView( Rect( 0, 0, 0, 0 ), "mem_usage_str1", "MaxPages", ALIGN_CENTER, WID_WILL_DRAW );
    m_pcMemUsageStr2 = new StringView( Rect( 0, 0, 0, 0 ), "mem_usage_str2", "BlockCache", ALIGN_CENTER, WID_WILL_DRAW );
    m_pcMemUsageStr3 = new StringView( Rect( 0, 0, 0, 0 ), "mem_usage_str3", "DirtyCache", ALIGN_CENTER, WID_WILL_DRAW );

    pcMem->AddChild( m_pcMemUsage, 1.0f );

    VLayoutNode* pcMemStr = new VLayoutNode( "memStr", 2.0f, pcMem );

	
    pcMemStr->SetHAlignment( ALIGN_CENTER );
    pcMemStr->AddChild( m_pcMemTitle, 1.0f );    

    HLayoutNode* pcMemStr1 = new HLayoutNode( "memStr1", 2.0f, pcMemStr );
    HLayoutNode* pcMemStr2 = new HLayoutNode( "memStr1", 3.0f, pcMemStr );
    HLayoutNode* pcMemStr3 = new HLayoutNode( "memStr1", 4.0f, pcMemStr );
   
    pcMemStr1->SetHAlignment( ALIGN_LEFT );
    pcMemStr2->SetHAlignment( ALIGN_LEFT );
    pcMemStr3->SetHAlignment( ALIGN_LEFT );

    pcMemStr1->AddChild( m_pcMemTitleStr1, 1.0f );
    pcMemStr1->AddChild( m_pcMemUsageStr1, 2.0f );    

    pcMemStr2->AddChild( m_pcMemTitleStr2, 1.0f );
    pcMemStr2->AddChild( m_pcMemUsageStr2, 2.0f );    

    pcMemStr3->AddChild( m_pcMemTitleStr3, 1.0f );
    pcMemStr3->AddChild( m_pcMemUsageStr3, 2.0f );    
    /**************************/

    VLayoutNode* pcCpuUsage = new VLayoutNode( "cpu_usage", 1.0f, pcCpu );
    VLayoutNode* pcCpuStr   = new VLayoutNode( "cpuStr", 2.0f, pcCpu );
    pcCpuStr->SetHAlignment( ALIGN_LEFT );

    pcCpuStr->AddChild( m_pcCpuTitle, 1.0f );

    for ( int i = 0 ; i < m_nCPUCount ; ++i ) {
      sprintf(pzTmp, "cpu_str_%d", i);
      m_pcCpuUsageStr[i] = new StringView( Rect( 0, 0, 0, 0 ), pzTmp,
		  		          "50.0f/10.0f%", ALIGN_CENTER, WID_WILL_DRAW );
    
      sprintf(pzTmp, "cpu_%d", i);
      m_pcCpuUsage[i] = new CpuMeter( Rect( 0, 0, 0, 0 ), pzTmp, 0, WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );
      m_pcCpuUsage[i]->SetFgColor( 0, 0, 0 );

      /* Note: I do *NOT* like this implementation, but I didn't know if
         (float)(3 + (i + 2)) would work with the layout implementation.*/

      if( i == 0 ){
        pcCpuStr->AddChild( m_pcCpuUsageStr[i], 2.0f );
        pcCpuUsage->AddChild( m_pcCpuUsage[i], 1.0f );
      }
      else if( i == 1 ){
        pcCpuStr->AddChild( m_pcCpuUsageStr[i], 3.0f );
        pcCpuUsage->AddChild( m_pcCpuUsage[i], 2.0f );
      }
    }
    
    // Rect(l,t,r,b)
    pcMem->SetBorders( Rect(20.0f,20.0f,20.0f,20.0f), "mem_usage", NULL );
   
    if( m_nCPUCount == 1 ){
      pcCpuUsage->SetBorders( Rect(20.0f,20.0f,20.0f,20.0f), "cpu_0", NULL );
      pcCpuStr->SetBorders( Rect(10.0f,10.0f,10.0f,10.0f), "cpu_str_0", "cpu_title", NULL );
    }
    else if( m_nCPUCount == 2 ){
      pcCpuUsage->SetBorders( Rect(20.0f,20.0f,20.0f,20.0f), "cpu_0", "cpu_1", NULL );
      pcCpuStr->SetBorders( Rect(10.0f,10.0f,10.0f,10.0f), "cpu_str_0", "cpu_str_1", "cpu_title", NULL );							    
    }
    
    pcRoot->SameWidth( "cpuStr", "memStr", NULL );
    pcMemStr->SetBorders(  Rect(5.0f,5.0f,5.0f,5.0f), "mem_title", NULL );
    pcMemStr1->SetBorders( Rect(5.0f,5.0f,5.0f,5.0f), "mem_usage_str1", "mem_title_str1", NULL );
    pcMemStr2->SetBorders( Rect(5.0f,5.0f,5.0f,5.0f), "mem_usage_str2", "mem_title_str2", NULL );
    pcMemStr3->SetBorders( Rect(5.0f,5.0f,5.0f,5.0f), "mem_usage_str3", "mem_title_str3", NULL );

    SetRoot( pcRoot );
}

//----------------------------------------------------------------------------
PerformancePanel::~PerformancePanel()
{
  if ( m_nW8378xDevice >= 0 ) {
    close( m_nW8378xDevice );
  }
}

void PerformancePanel::AllAttached()
{ 
  UpdatePerformanceList();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void PerformancePanel::FrameSized( const Point& cDelta )
{
    LayoutView::FrameSized( cDelta );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void PerformancePanel::KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers )
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
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void PerformancePanel::Paint( const Rect& cUpdateRect )
{
  Rect cBounds = GetBounds();

  SetFgColor( get_default_color( COL_NORMAL ) );
  SetBgColor( get_default_color( COL_NORMAL ) );
  FillRect( cBounds );
}

//----------------------------------------------------------------------------

void PerformancePanel::UpdatePerformanceList()
{
    system_info sInfo;

    get_system_info( &sInfo );

    int nTempSensors[] = { W8378x_READ_TEMP1, W8378x_READ_TEMP2 };
    float avLoads[MAX_CPU_COUNT];
    GetLoad( m_nCPUCount, avLoads );
  
    String pzPostfix;
    String pzCachePF;
    String pzTotalPF;
    float vFreeMem = float(sInfo.nFreePages * PAGE_SIZE);
    float vCache   = float(sInfo.nBlockCacheSize);
    float vTotal   = float(sInfo.nMaxPages * PAGE_SIZE);
    
    if ( vFreeMem > 1000000.0f ) {
	vFreeMem /= 1000000.0f;
	pzPostfix = MSG_TAB_PERFORMANCE_MEGABYTE;
    } else if ( vFreeMem > 1000.0f ) {
	vFreeMem /= 1000.0f;
	pzPostfix = MSG_TAB_PERFORMANCE_KILOBYTE;
    } else {
	pzPostfix = MSG_TAB_PERFORMANCE_BYTE;
    }
    if ( vCache > 1000000.0f ) {
	vCache /= 1000000.0f;
	pzCachePF = MSG_TAB_PERFORMANCE_MEGABYTE;
    } else if ( vCache > 1000.0f ) {
	vCache /= 1000.0f;
	pzCachePF = MSG_TAB_PERFORMANCE_KILOBYTE;
    } else {
	pzCachePF = MSG_TAB_PERFORMANCE_BYTE;
    }
    if ( vTotal > 1000000.0f ) {
	vTotal /= 1000000.0f;
	pzTotalPF = MSG_TAB_PERFORMANCE_MEGABYTE;
    } else if ( vTotal > 1000.0f ) {
	vTotal /= 1000.0f;
	pzTotalPF = MSG_TAB_PERFORMANCE_KILOBYTE;
    } else {
	pzTotalPF = MSG_TAB_PERFORMANCE_BYTE;
    }
    
    char zBuffer[256];
    
    /* Setting Memory Strings */
    sprintf( zBuffer, "%.2f%s", vTotal, pzTotalPF.c_str() );
    m_pcMemUsageStr1->SetString( zBuffer );

    sprintf( zBuffer, "%.2f%s", vFreeMem, pzPostfix.c_str() );
    m_pcMemUsageStr2->SetString( zBuffer );

    sprintf( zBuffer, "%.2f%s", vCache, pzCachePF.c_str() );
    m_pcMemUsageStr3->SetString( zBuffer );
    /**************************/

    //sprintf( zBuffer, "%.2f%s / %.2f%s / %.2f%s", vFreeMem, pzPostfix, vCache, pzCachePF, vDirty, pzDirtyPF );

    for ( int i = 0 ; i < m_nCPUCount ; ++i ) {
        if(i == 0){
  	  float vFreePst  = 1.0f - float(sInfo.nFreePages) / float(sInfo.nMaxPages);
	  float vCachePst = float(sInfo.nBlockCacheSize) / float(sInfo.nMaxPages * PAGE_SIZE);
	  float vDirtyPst = float(sInfo.nDirtyCacheSize) / float(sInfo.nMaxPages * PAGE_SIZE);

	  m_pcMemUsage->AddValue( vFreePst - vCachePst, vCachePst - vDirtyPst, vDirtyPst );
          //m_pcMemUsageStr->SetString( zBuffer );
        }

	m_pcCpuUsage[i]->AddValue( avLoads[i] );

	if ( m_nW8378xDevice >= 0 ) {
	  int nTemp = 0;
	  ioctl( m_nW8378xDevice, nTempSensors[i%2], &nTemp );
	  sprintf( zBuffer, "%.1fc/%.1f%%", float(nTemp)/256.0f, avLoads[i] * 100.0 );
	} 
        else
	  sprintf( zBuffer, "%.1f%%", avLoads[i] * 100.0 );

	  m_pcCpuUsageStr[i]->SetString( zBuffer );
      
    }
}

void PerformancePanel::HandleMessage( Message* pcMessage )
{
    switch( pcMessage->GetCode() )
    {
	case ID_QUIT:
	{
	    break;
	}
	default:
	    View::HandleMessage( pcMessage );
	    break;
    }
}



