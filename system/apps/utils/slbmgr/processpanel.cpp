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
#include <malloc.h>
#include <termios.h>
#include <assert.h>

#include <gui/button.h>
#include <gui/listview.h>
#include <gui/stringview.h>
#include <gui/desktop.h>

#include <atheos/types.h>
#include <atheos/threads.h>
#include <atheos/semaphore.h>
#include <atheos/kernel.h>

#include <signal.h>

#include <util/application.h>
#include <util/message.h>

#include <macros.h>

#include "processpanel.h"

#include "resources/SysInfo.h"

using namespace os;

enum
{
  ID_ENDPROC
};

void kill_proc_threads(int nVal )
{
   
    thread_info sInfo, sInfo2;
    thread_id   nId = -1;
   
    int	nError;
    int i = 0;

    for ( nError = get_thread_info( -1, &sInfo ) ; nError >= 0 ; nError = get_next_thread_info( &sInfo ) ){
      if( i == nVal ){
        nId = sInfo.ti_process_id;
	break;
      }
      i++;
    }
   
    if( nId != -1 )
      for( nError = get_thread_info( -1, &sInfo2 ); nError >= 0; nError = get_next_thread_info( &sInfo2 ) ){
        if( nId == sInfo2.ti_process_id )
          kill( sInfo2.ti_thread_id, 9 );
      }
   
}

//----------------------------------------------------------------------------

ProcessPanel::ProcessPanel( const Rect& cFrame ) : LayoutView( cFrame, "", NULL, CF_FOLLOW_NONE )
{
    VLayoutNode* pcRoot = new VLayoutNode( "root" );

    SetThreadCount( ThreadCount() );
   
    m_pcEndProcBut     = new Button( Rect( 0, 0, 0, 0 ), "endproc_but", MSG_TAB_PROCESSES_BUTTON_ENDPROCESS, new Message( ID_ENDPROC ), CF_FOLLOW_NONE );
    m_pcProcessList    = new ListView( Rect(0 ,0, 0, 0 ), "proc_list", ListView::F_RENDER_BORDER | ListView::F_NO_AUTO_SORT );
   
    m_pcProcessList->InsertColumn( MSG_TAB_PROCESSES_LIST_PID.c_str(), 40 );
    m_pcProcessList->InsertColumn( MSG_TAB_PROCESSES_LIST_PROCESSNAME.c_str(), 100 );
    m_pcProcessList->InsertColumn( MSG_TAB_PROCESSES_LIST_TID.c_str(), 40 );
    m_pcProcessList->InsertColumn( MSG_TAB_PROCESSES_LIST_THREADNAME.c_str(), 120 );
    m_pcProcessList->InsertColumn( MSG_TAB_PROCESSES_LIST_STATE.c_str(), 100 );
 
    pcRoot->AddChild( m_pcProcessList, 1.0f );
    pcRoot->AddChild( m_pcEndProcBut, 0.0f );
   
	m_pcProcessList->SetTabOrder();
	m_pcEndProcBut->SetTabOrder();
   
    pcRoot->SetBorders( Rect(10.0f,10.0f,10.0f,10.0f), "proc_list", NULL );
    pcRoot->SetBorders( Rect(10.0f,5.0f,10.0f,5.0f), "endproc_but", NULL );
    SetRoot( pcRoot );

    m_pcProcessList->SetInvokeMsg( new Message( ID_ENDPROC ));
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ProcessPanel::AllAttached()
{
  m_pcProcessList->SetTarget( this );
  m_pcEndProcBut->SetTarget( this );
  UpdateProcessList();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ProcessPanel::FrameSized( const Point& cDelta )
{
    LayoutView::FrameSized( cDelta );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ProcessPanel::KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers )
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

void ProcessPanel::Paint( const Rect& cUpdateRect )
{
  Rect cBounds = GetBounds();

  SetFgColor( get_default_color( COL_NORMAL ) );
  FillRect( cBounds );
}

//----------------------------------------------------------------------------

void ProcessPanel::UpdateProcessList()
{
    m_pcProcessList->Clear();

    char zBuf[128];
    thread_info sInfo;

    int		nError;

    for ( nError = get_thread_info( -1, &sInfo ) ; nError >= 0 ; nError = get_next_thread_info( &sInfo ) )
    {
	char*	pzLockSemName = NULL;
	String	pzState;

	pzLockSemName = "none";
					
	switch( sInfo.ti_state )
	{
	    case TS_STOPPED:
		pzState = MSG_TAB_PROCESSES_STATES_STOPPED;
		break;
	    case TS_RUN:
		pzState = MSG_TAB_PROCESSES_STATES_RUN;
		break;
	    case TS_READY:
		pzState = MSG_TAB_PROCESSES_STATES_READY;
		break;
	    case TS_WAIT:
		pzState = MSG_TAB_PROCESSES_STATES_WAIT;
		break;
	    case TS_SLEEP:
		pzState = MSG_TAB_PROCESSES_STATES_SLEEP;
		break;
	    case TS_ZOMBIE:
		pzState = MSG_TAB_PROCESSES_STATES_ZOMBIE;
		break;
	    default:
		pzState = MSG_TAB_PROCESSES_STATES_INVALID;
		break;
	}

	if ( sInfo.ti_state == TS_WAIT && sInfo.ti_blocking_sema >= 0 ) {
	    static char zBuffer[256];
	    sem_info sSemInfo;
		
	    if ( get_semaphore_info( sInfo.ti_blocking_sema, sInfo.ti_process_id, &sSemInfo ) >= 0 ) {
		sprintf( zBuffer, "%s", sSemInfo.si_name );
		pzState = zBuffer;
	    }
	}

	ListViewStringRow* pcRow = new ListViewStringRow();

	sprintf( zBuf, "%04d", sInfo.ti_process_id);
	pcRow->AppendString( zBuf );

	sprintf( zBuf, "%13.13s", sInfo.ti_process_name);
	pcRow->AppendString( zBuf );

	sprintf( zBuf, "%04d", sInfo.ti_thread_id);
	pcRow->AppendString( zBuf );

	sprintf( zBuf, "%13.13s", sInfo.ti_thread_name);
	pcRow->AppendString( zBuf );

        sprintf( zBuf, "%18s", pzState.c_str());
	pcRow->AppendString( zBuf );
	  
	m_pcProcessList->InsertRow( pcRow, false );
					
    }
}

void ProcessPanel::HandleMessage( Message* pcMessage )
{
    switch( pcMessage->GetCode() ){
      case ID_ENDPROC:{

        int nSelection = m_pcProcessList->GetFirstSelected();
        if ( nSelection < 0 ) {
          //std::cout << "nSelection < 0" << std::endl;
	  break;
	}

        kill_proc_threads( nSelection );
	 
	break;
      }
      default:
        View::HandleMessage( pcMessage );
        break;
    }
}

int ProcessPanel::ThreadCount(){
  thread_info sInfo;
   
  int count = 0;
  int nError;
   
  for( nError = get_thread_info( -1, &sInfo ); nError >= 0; nError = get_next_thread_info( &sInfo ) )
    count++;
   
  return count;
}

void ProcessPanel::SetThreadCount( int nVal ){
  nThreads = nVal;
}

int ProcessPanel::GetThreadCount(){
  return nThreads;
}
