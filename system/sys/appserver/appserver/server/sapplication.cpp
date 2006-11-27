
/*
 *  The AtheOS application server
 *  Copyright (C) 1999 - 2001  Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
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
#include <errno.h>

#include <atheos/threads.h>
#include <atheos/kernel.h>
#include <atheos/msgport.h>
#include <appserver/protocol.h>
#include <util/messenger.h>
#include <signal.h>
#include <gui/desktop.h>

#include "sapplication.h"
#include "swindow.h"
#include "server.h"
#include "sfont.h"
#include "bitmap.h"
#include "sprite.h"
#include "keyboard.h"
#include "config.h"
#include "event.h"

#include <macros.h>
#include <algorithm>

using namespace os;

#define POINTER_WIDTH 11
#define POINTER_HEIGHT 18

extern uint8 g_anMouseImg[];

SrvApplication *SrvApplication::s_pcFirstApp = NULL;
Locker SrvApplication::s_cAppListLock( "app_list" );

std::vector < SrvApplication::Cursor * >SrvApplication::s_cCursorStack;
bool SrvApplication::s_bBorderCursor = false;

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

SrvApplication::SrvApplication( const char *pzName, proc_id hOwner, port_id hEventPort ):m_cName( pzName ), m_cLocker( "app_lock" ), m_cFontLock( "app_font_lock" )
{
	m_hOwner = hOwner;
	m_bClosing = false;

	s_cAppListLock.Lock();
	m_pcNext = s_pcFirstApp;
	s_pcFirstApp = this;
	s_cAppListLock.Unlock();

	m_pcAppTarget = new Messenger( hEventPort, -1, -1 );

	m_hReqPort = create_port( "srvapp", DEFAULT_PORT_SIZE );

	thread_id hThread = spawn_thread( "app_thread", (void*)Entry, DISPLAY_PRIORITY, 0, this );

	if( hThread != -1 )
	{
		resume_thread( hThread );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

SrvApplication::~SrvApplication()
{
	SrvApplication **ppcTmp;
	
	g_pcEvents->ProcessKilled( m_hOwner );

	if( m_cFonts.size() > 0 )
	{
		dbprintf( "Application %s forgot to delete %d fonts!\n", m_cName.c_str(), m_cFonts.size(  ) );
	}
	if( m_cBitmaps.size() > 0 )
	{
		dbprintf( "Application %s forgot to delete %d bitmaps!\n", m_cName.c_str(), m_cBitmaps.size(  ) );
	}
	if( m_cSprites.size() > 0 )
	{
		dbprintf( "Application %s forgot to delete %d sprites!\n", m_cName.c_str(), m_cBitmaps.size(  ) );
	}
	while( m_cFonts.empty() == false )
	{
		std::set < FontNode * >::iterator i = m_cFonts.begin();
		FontNode *pcNode = ( *i );

		pcNode->Release();
		m_cFonts.erase( i );
	}
	while( m_cBitmaps.empty() == false )
	{
		std::set < SrvBitmap * >::iterator i = m_cBitmaps.begin();
		SrvBitmap *pcNode = ( *i );
		pcNode->Release();

		m_cBitmaps.erase( i );
	}
	while( m_cSprites.empty() == false )
	{
		std::set < SrvSprite * >::iterator i = m_cSprites.begin();
		SrvSprite *pcSprite = ( *i );
		delete pcSprite;

		m_cSprites.erase( i );
	}

	s_cAppListLock.Lock();
	for( ppcTmp = &s_pcFirstApp; *ppcTmp != NULL; ppcTmp = &( *ppcTmp )->m_pcNext )
	{
		if( *ppcTmp == this )
		{
			*ppcTmp = m_pcNext;
			break;
		}
	}
	s_cAppListLock.Unlock();

	g_cLayerGate.Close();
	bool bResetCursor = ( s_bBorderCursor == false && s_cCursorStack.size() > 0 && s_cCursorStack[s_cCursorStack.size(  ) - 1]->m_pcOwner == this );

	for( uint i = 0; i < s_cCursorStack.size(); )
	{
		Cursor *pcCursor = s_cCursorStack[i];

		if( pcCursor->m_pcOwner == this )
		{
			s_cCursorStack.erase( s_cCursorStack.begin() + i );
			delete[]static_cast < char *>( pcCursor->m_pImage );
			delete pcCursor;
		}
		else
		{
			i++;
		}
	}
	if( bResetCursor )
	{
		if( s_cCursorStack.size() > 0 )
		{
			Cursor *pcCursor = s_cCursorStack[s_cCursorStack.size() - 1];

			g_pcDispDrv->SetCursorBitmap( pcCursor->m_eMode, pcCursor->m_cHotSpot, pcCursor->m_pImage, pcCursor->m_cSize.x, pcCursor->m_cSize.y );
		}
		else
		{
			g_pcDispDrv->SetCursorBitmap( MPTR_RGB32, IPoint( 0, 0 ), g_anMouseImg, POINTER_WIDTH, POINTER_HEIGHT );
		}
	}
	g_cLayerGate.Open();
	delete_port( m_hReqPort );
}

void SrvApplication::SetCursor( mouse_ptr_mode eMode, const IPoint & cHotSpot, const void *pImage, int nWidth, int nHeight )
{
	s_bBorderCursor = true;
	if( pImage != NULL )
	{
		g_pcDispDrv->SetCursorBitmap( eMode, cHotSpot, pImage, nWidth, nHeight );
	}
	else
	{
		g_pcDispDrv->SetCursorBitmap( MPTR_RGB32, IPoint( 0, 0 ), g_anMouseImg, POINTER_WIDTH, POINTER_HEIGHT );
	}
}

void SrvApplication::RestoreCursor()
{
	if( s_bBorderCursor )
	{
		if( s_cCursorStack.size() > 0 )
		{
			Cursor *pcCursor = s_cCursorStack[s_cCursorStack.size() - 1];

			g_pcDispDrv->SetCursorBitmap( pcCursor->m_eMode, pcCursor->m_cHotSpot, pcCursor->m_pImage, pcCursor->m_cSize.x, pcCursor->m_cSize.y );
		}
		else
		{
			g_pcDispDrv->SetCursorBitmap( MPTR_RGB32, IPoint( 0, 0 ), g_anMouseImg, POINTER_WIDTH, POINTER_HEIGHT );
		}
		s_bBorderCursor = false;
	}
}

void SrvApplication::PostUsrMessage( Message * pcMsg )
{
	if( m_pcAppTarget != NULL )
	{
		m_pcAppTarget->SendMessage( pcMsg );
	}
}

void SrvApplication::ReplaceDecorators()
{
	g_cLayerGate.Close();
	s_cAppListLock.Lock();

	for( SrvApplication * pcApp = s_pcFirstApp; pcApp != NULL; pcApp = pcApp->m_pcNext )
	{
		std::set < SrvWindow * >::iterator i;

		for( i = pcApp->m_cWindows.begin(); i != pcApp->m_cWindows.end(  ); ++i )
		{
			( *i )->ReplaceDecorator();
		}
	}

	s_cAppListLock.Unlock();

	g_pcTopView->Invalidate( true );
	g_pcTopView->SetDirtyRegFlags();
	g_pcTopView->UpdateRegions();
	SrvWindow::HandleMouseTransaction();
	g_cLayerGate.Open();
}

void SrvApplication::NotifyWindowFontChanged( bool bToolWindow )
{
	g_cLayerGate.Close();
	s_cAppListLock.Lock();

	for( SrvApplication * pcApp = s_pcFirstApp; pcApp != NULL; pcApp = pcApp->m_pcNext )
	{
		std::set < SrvWindow * >::iterator i;

		for( i = pcApp->m_cWindows.begin(); i != pcApp->m_cWindows.end(  ); ++i )
		{
			( *i )->NotifyWindowFontChanged( bToolWindow );
		}
	}
	s_cAppListLock.Unlock();
	g_pcTopView->Invalidate( true );
	g_pcTopView->SetDirtyRegFlags();
	g_pcTopView->UpdateRegions();
	SrvWindow::HandleMouseTransaction();
	g_cLayerGate.Open();
}

void SrvApplication::NotifyColorCfgChanged()
{
	s_cAppListLock.Lock();
	for( SrvApplication * pcApp = s_pcFirstApp; pcApp != NULL; pcApp = pcApp->m_pcNext )
	{
		__assertw( pcApp->m_pcAppTarget != NULL );
		if( pcApp->m_pcAppTarget != NULL )
		{
			Message cMsg( M_COLOR_CONFIG_CHANGED );

			cMsg.AddFloat( "_shine_tint", 0.9f );
			cMsg.AddFloat( "_shadow_tint", 0.9f );

			for( int i = 0; i < COL_COUNT; ++i )
			{
				cMsg.AddColor32( "_colors", get_default_color( static_cast < default_color_t > ( i ) ) );
			}
			if( pcApp->m_pcAppTarget->SendMessage( &cMsg ) < 0 )
			{
				dbprintf( "Error: SrvApplication::NotifyColorCfgChanged() failed to send message to app: %s\n", pcApp->m_cName.c_str() );
			}
		}
	}
	s_cAppListLock.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

SrvApplication *SrvApplication::FindApp( proc_id hProc )
{
	SrvApplication *pcTmp;

	s_cAppListLock.Lock();
	for( pcTmp = s_pcFirstApp; pcTmp != NULL; pcTmp = pcTmp->m_pcNext )
	{
		if( pcTmp->m_hOwner == hProc )
		{
			break;
		}
	}
	s_cAppListLock.Unlock();
	return ( pcTmp );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

FontNode *SrvApplication::GetFont( uint32 nID )
{
	FontNode *pcInst = NULL;

	if( m_cFontLock.Lock() == 0 )
	{
		std::set < FontNode * >::iterator i = m_cFonts.find( ( FontNode * ) nID );

		if( i != m_cFonts.end() )
		{
			pcInst = *i;
		}
		else
		{
			dbprintf( "Error : SrvApplication::GetFont() called on invalid font ID %x\n", nID );
		}
		m_cFontLock.Unlock();
	}
	return ( pcInst );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SrvApplication::CreateBitmap( port_id hReply, int nWidth, int nHeight, color_space eColorSpc, uint32 nFlags )
{
	void *pRaster = NULL;
	int nSize = nWidth * nHeight * BitsPerPixel( eColorSpc ) / 8;

	nSize = ( nSize + PAGE_SIZE - 1 ) & PAGE_MASK;

	area_id hArea = -1;

	if( nFlags & Bitmap::SHARE_FRAMEBUFFER )
	{
		hArea = create_area( "bitmap_area", &pRaster, nSize, AREA_FULL_ACCESS, AREA_NO_LOCK );
		if( hArea < 0 )
		{
			AR_CreateBitmapReply_s sReply( -ENOMEM, -1 );

			send_msg( hReply, 0, &sReply, sizeof( sReply ) );
			return;
		}
	}
	SrvBitmap *pcBitmap = new SrvBitmap( nWidth, nHeight, eColorSpc, ( uint8 * )pRaster );

	pcBitmap->m_hArea = hArea;
	pcBitmap->m_nFlags = nFlags;
	pcBitmap->m_pcDriver = g_pcDispDrv;

	m_cBitmaps.insert( pcBitmap );
	AR_CreateBitmapReply_s sReply( pcBitmap->GetToken(), hArea );

	send_msg( hReply, 0, &sReply, sizeof( sReply ) );
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SrvApplication::CloneBitmap( port_id hReply, int hHandle )
{
	SrvBitmap *pcBitmap = g_pcBitmaps->GetObj( hHandle );
	
	if( pcBitmap == NULL )
	{
		AR_CreateBitmapReply_s sReply;
		sReply.m_hHandle = -EINVAL;
		send_msg( hReply, 0, &sReply, sizeof( sReply ) );
		return;
	}
	
	__assertw( pcBitmap != NULL );
	pcBitmap->AddRef();

	m_cBitmaps.insert( pcBitmap );
	AR_CloneBitmapReply_s sReply;
	sReply.m_hHandle = pcBitmap->GetToken();
	sReply.m_hArea = pcBitmap->m_hArea;
	sReply.m_nFlags = pcBitmap->m_nFlags;
	sReply.m_nWidth = pcBitmap->m_nWidth;
	sReply.m_nHeight = pcBitmap->m_nHeight;
	sReply.m_eColorSpc = pcBitmap->m_eColorSpc;

	send_msg( hReply, 0, &sReply, sizeof( sReply ) );
	
	__assertw( g_pcBitmaps->GetObj( hHandle ) );
	
	//dbprintf( "Bitmap %i cloned\n", pcBitmap->GetToken() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SrvApplication::DeleteBitmap( int hHandle )
{
	SrvBitmap *pcBitmap = g_pcBitmaps->GetObj( hHandle );

	if( pcBitmap == NULL )
	{
		dbprintf( "Error: Attempt to delete invalid bitmap %d\n", hHandle );
		return;
	}
	m_cBitmaps.erase( pcBitmap );
	pcBitmap->Release();
}

void SrvApplication::DispatchMessage( Message * pcReq )
{
	switch ( pcReq->GetCode() )
	{
	case AR_OPEN_WINDOW:
		{
			const char *pzTitle;
			port_id hEventPort = -1;
			uint32 nFlags = 0;
			uint32 nDesktopMask = 0;
			void *pUserObj;
			Rect cFrame;

			pcReq->FindString( "title", &pzTitle );
			pcReq->FindInt( "event_port", &hEventPort );
			pcReq->FindInt( "flags", &nFlags );
			pcReq->FindInt( "desktop_mask", &nDesktopMask );
			pcReq->FindRect( "frame", &cFrame );
			pcReq->FindPointer( "user_obj", &pUserObj );

			Message cReply;

			g_cLayerGate.Close();
			SrvWindow *pcWindow;

			try
			{
				pcWindow = new SrvWindow( pzTitle, pUserObj, nFlags, nDesktopMask, cFrame, this, hEventPort );
			} catch( ... )
			{
				g_cLayerGate.Open();
				dbprintf( "Error: Failed to create window\n" );
				cReply.AddInt32( "top_view", -1 );
				cReply.AddInt32( "cmd_port", -1 );
				pcReq->SendReply( &cReply );
				break;
			}
			m_cWindows.insert( pcWindow );
			pcWindow->Show( false );
			pcWindow->PostEvent();
			cReply.AddInt32( "top_view", pcWindow->GetClientView()->GetHandle(  ) );
			cReply.AddInt32( "cmd_port", pcWindow->GetMsgPort() );
			g_cLayerGate.Open();
			pcReq->SendReply( &cReply );
			break;
		}
	case AR_OPEN_BITMAP_WINDOW:
		{
			int hBitmapHandle = -1;
			void *pUserObj;

			pcReq->FindInt( "bitmap_handle", &hBitmapHandle );
			pcReq->FindPointer( "user_obj", &pUserObj );

			g_cLayerGate.Close();
			SrvBitmap *pcBitmap = g_pcBitmaps->GetObj( hBitmapHandle );

			if( pcBitmap == NULL )
			{
				dbprintf( "Error: SrvApplication::DispatchMessage() invalid bitmap-handle %d, cant create window\n", hBitmapHandle );
				Message cReply;

				cReply.AddInt32( "cmd_port", -1 );
				cReply.AddInt32( "top_view", -1 );
				pcReq->SendReply( &cReply );

				g_cLayerGate.Open();
				break;
			}

			SrvWindow *pcWindow = new SrvWindow( this, pcBitmap );

			m_cWindows.insert( pcWindow );

			Message cReply;

			cReply.AddInt32( "cmd_port", m_hReqPort );
			cReply.AddInt32( "top_view", pcWindow->GetClientView()->GetHandle(  ) );

			pcReq->SendReply( &cReply );
			g_cLayerGate.Open();
			break;
		}
	case AR_CLOSE_WINDOW:
		{
			int hTopView;

			if( pcReq->FindInt( "top_view", &hTopView ) < 0 )
			{
				dbprintf( "Error: SrvApplication::DispatchMessage() AR_CLOSE_WINDOW has no 'top_view' field\n" );
				break;
			}

			g_cLayerGate.Close();

			Layer *pcTopLayer = FindLayer( hTopView );

			if( pcTopLayer == NULL )
			{
				dbprintf( "Error: SrvApplication::DispatchMessage() attempt to close invalid window %d\n", hTopView );
				g_cLayerGate.Open();
				break;
			}
			SrvWindow *pcWindow = pcTopLayer->GetWindow();

			if( pcWindow == NULL )
			{
				dbprintf( "Error: SrvApplication::DispatchMessage() top-view has no window! Can't close\n" );
				g_cLayerGate.Open();
				break;
			}
			m_cWindows.erase( pcWindow );
			delete pcWindow;
			
			if( pcReq->IsSourceWaiting() )
				pcReq->SendReply( M_REPLY );

			g_cLayerGate.Open();
			break;
		}
	case AR_GET_IDLE_TIME:
		{
			Message cReply;

			cReply.AddInt64( "idle_time", AppServer::GetInstance()->GetIdleTime(  ) );
			pcReq->SendReply( &cReply );
			break;
		}
	case AR_GET_FONT_FAMILY_COUNT:
		{
			Message cReply;

			cReply.AddInt32( "count", FontServer::GetInstance()->GetFamilyCount(  ) );
			pcReq->SendReply( &cReply );
			break;
		}
	case AR_GET_FONT_FAMILY:
		{
			int nIndex = -1;
			char zFamily[FONT_FAMILY_LENGTH + 1];
			uint32 nFlags;

			pcReq->FindInt( "index", &nIndex );

			zFamily[0] = '\0';
			int nError = FontServer::GetInstance()->GetFamily( nIndex, zFamily, &nFlags );

			Message cReply;

			cReply.AddInt32( "error", nError );
			if( nError >= 0 )
			{
				cReply.AddString( "family", zFamily );
				cReply.AddInt32( "flags", nFlags );
			}

			pcReq->SendReply( &cReply );
			break;
		}
	case AR_GET_FONT_STYLE_COUNT:
		{
			const char *pzFamily = "";

			pcReq->FindString( "family", &pzFamily );

			Message cReply;

			cReply.AddInt32( "count", FontServer::GetInstance()->GetStyleCount( pzFamily ) );
			pcReq->SendReply( &cReply );
			break;
		}
	case AR_GET_FONT_STYLE:
		{
			const char *pzFamily = "";
			int nIndex = -1;
			char zStyle[FONT_STYLE_LENGTH + 1];
			uint32 nFlags;

			pcReq->FindString( "family", &pzFamily );
			pcReq->FindInt( "index", &nIndex );

			zStyle[0] = '\0';
			int nError = FontServer::GetInstance()->GetStyle( pzFamily, nIndex, zStyle, &nFlags );

			Message cReply;

			cReply.AddInt32( "error", nError );
			cReply.AddString( "style", zStyle );
			cReply.AddInt32( "flags", nFlags );

			pcReq->SendReply( &cReply );
			break;
		}
	case AR_GET_FONT_SIZES:
		{
			Message cReply;
			const char *pzFamily = "";
			const char *pzStyle = "";

			pcReq->FindString( "family", &pzFamily );
			pcReq->FindString( "style", &pzStyle );

			SFont *pcFont = FontServer::GetInstance()->OpenFont( pzFamily, pzStyle );

			if( pcFont == NULL )
			{
				cReply.AddInt32( "error", -ENOENT );
				pcReq->SendReply( &cReply );
				break;
			}
			const Font::size_list_t &cSizeList = pcFont->GetBitmapSizes();

			for( uint i = 0; i < cSizeList.size(); ++i )
			{
				cReply.AddFloat( "size_table", cSizeList[i] );
			}
			cReply.AddInt32( "error", 0 );
			pcReq->SendReply( &cReply );
			break;
		}
	case AR_GET_FONT_CHARACTERS:
		{
			int hFont = -1;
			uint32 nLastChar = 0;
			uint32 nMaxChars = 1000;

			pcReq->FindInt( "handle", &hFont );
			pcReq->FindInt32( "lastchar", (int32*)&nLastChar );
			pcReq->FindInt32( "maxchars", (int32*)&nMaxChars );

			Message cReply;

			if( m_cFontLock.Lock() != 0 )
			{
				dbprintf( "Error: Failed to lock m_cFontLock while handling AR_GET_FONT_CHARACTERS request\n" );
				cReply.AddInt32( "error", -EINVAL );
				pcReq->SendReply( &cReply );
				break;
			}
			std::set < FontNode * >::iterator i = m_cFonts.find( ( FontNode * ) hFont );

			if( i == m_cFonts.end() )
			{
				dbprintf( "Error : GetFontCharacters() called on invalid font ID %08x\n", hFont );
				cReply.AddInt32( "error", -EINVAL );
				pcReq->SendReply( &cReply );
				m_cFontLock.Unlock();
				break;
			}
			FontNode *pcNode = *i;

			SFontInstance *pcInstance = pcNode->GetInstance();

			if( pcInstance == NULL )
			{
				dbprintf( "Error : GetFontCharacters() could not get instance for font ID %08x\n", hFont );

				cReply.AddInt32( "error", -ENOMEM );
				pcReq->SendReply( &cReply );
				m_cFontLock.Unlock();
				break;
			}

			cReply.AddInt32( "error", 0 );

			uint32 nCharCode;
			uint32 nNumChars=0;

			if( nLastChar ) {
				nCharCode = nLastChar;
				while( pcInstance->GetNextSupportedChar( &nCharCode ) ) {
					nNumChars++;
					cReply.AddInt32( "character_code", nCharCode );
					if( nNumChars >= nMaxChars ) {
						if( pcInstance->GetNextSupportedChar( &nCharCode ) ) {
							cReply.AddBool( "more_chars", true );
						}
						break;
					}
				}
			} else {
				if( pcInstance->GetFirstSupportedChar( &nCharCode ) ) do {
					nNumChars++;
					cReply.AddInt32( "character_code", nCharCode );
					if( nNumChars >= nMaxChars ) {
						if( pcInstance->GetNextSupportedChar( &nCharCode ) ) {
							cReply.AddBool( "more_chars", true );
						}
						break;
					}
				} while( pcInstance->GetNextSupportedChar( &nCharCode ) );
			}
			
			pcReq->SendReply( &cReply );
			m_cFontLock.Unlock();
			break;
		}
	case AR_CREATE_FONT:
		{
			FontNode *pcNode;

			try
			{
				pcNode = new FontNode;
			}
			catch( ... )
			{
				dbprintf( "SrvApplication::DispatchMessage() Failed to create font node\n" );
				Message cReply;

				cReply.AddInt32( "handle", -ENOMEM );
				pcReq->SendReply( &cReply );
				break;
			}

			int hFontToken = int32 ( pcNode );

			if( m_cFontLock.Lock() == 0 )
			{	// FIXME: Report the error
				m_cFonts.insert( pcNode );
				m_cFontLock.Unlock();
			}
			Message cReply;

			cReply.AddInt32( "handle", hFontToken );
			pcReq->SendReply( &cReply );
			break;
		}
	case AR_DELETE_FONT:
		{
			int hFont = -1;

			pcReq->FindInt( "handle", &hFont );

			if( m_cFontLock.Lock() == 0 )
			{
				std::set < FontNode * >::iterator i = m_cFonts.find( ( FontNode * ) hFont );

				if( i == m_cFonts.end() )
				{
					dbprintf( "Error: Attempt to delete invalid font %d\n", hFont );
					m_cFontLock.Unlock();
					break;
				}
				FontNode *pcNode = *i;

				m_cFonts.erase( pcNode );
				pcNode->Release();
				m_cFontLock.Unlock();
			}
			break;
		}
	case AR_SET_FONT_FAMILY_AND_STYLE:
		{
			int hFont = -1;
			const char *pzFamily = "";
			const char *pzStyle = "";

			pcReq->FindInt( "handle", &hFont );
			pcReq->FindString( "family", &pzFamily );
			pcReq->FindString( "style", &pzStyle );

			Message cReply;

			if( m_cFontLock.Lock() != 0 )
			{
				dbprintf( "Error: Failed to lock m_cFontLock while handling AR_SET_FONT_FAMILY_AND_STYLE request\n" );
				cReply.AddInt32( "error", -EINVAL );
				pcReq->SendReply( &cReply );
				break;
			}
			std::set < FontNode * >::iterator i = m_cFonts.find( ( FontNode * ) hFont );

			if( i == m_cFonts.end() )
			{
				dbprintf( "Error : SetFontFamilyAndStyle() called on invalid font ID %08x\n", hFont );
				cReply.AddInt32( "error", -EINVAL );
				pcReq->SendReply( &cReply );
				m_cFontLock.Unlock();
				break;
			}
			FontNode *pcNode = *i;

			int nError = pcNode->SetFamilyAndStyle( pzFamily, pzStyle );

			if( nError < 0 )
			{
				cReply.AddInt32( "error", nError );
				pcReq->SendReply( &cReply );
				m_cFontLock.Unlock();
				break;
			}
			SFontInstance *pcInstance = pcNode->GetInstance();

			if( NULL == pcInstance )
			{
				dbprintf( "Error : SetFamilyAndStyle() FontNode::GetInstance() returned NULL\n" );
				cReply.AddInt32( "error", -ENOMEM );
				pcReq->SendReply( &cReply );
				m_cFontLock.Unlock();
				break;
			}

			cReply.AddInt32( "error", 0 );
			cReply.AddInt32( "ascender", pcInstance->GetAscender() );
			cReply.AddInt32( "descender", pcInstance->GetDescender() );
			cReply.AddInt32( "line_gap", pcInstance->GetLineGap() );

			pcReq->SendReply( &cReply );

			pcNode->NotifyDependent();
			m_cFontLock.Unlock();
			break;
		}
	case AR_SET_FONT_PROPERTIES:
		{
			int hFont = -1;
			float vSize = 8.0f;
			float vShear = 0.0f;
			float vRotation = 0.0f;
			uint32 nFlags = FPF_SMOOTHED;

			pcReq->FindInt( "handle", &hFont );
			pcReq->FindFloat( "size", &vSize );
			pcReq->FindFloat( "rotation", &vRotation );
			pcReq->FindFloat( "shear", &vShear );
			pcReq->FindInt32( "flags", ( int32 * )&nFlags );

			Message cReply;

			if( m_cFontLock.Lock() != 0 )
			{
				dbprintf( "Error: Failed to lock m_cFontLock while handling AR_SET_FONT_PROPERTIES request\n" );
				cReply.AddInt32( "error", -EINVAL );
				pcReq->SendReply( &cReply );
				break;
			}
			std::set < FontNode * >::iterator i = m_cFonts.find( ( FontNode * ) hFont );

			if( i == m_cFonts.end() )
			{
				dbprintf( "Error : SetFontProperties() called on invalid font ID %08x\n", hFont );
				cReply.AddInt32( "error", -EINVAL );
				pcReq->SendReply( &cReply );
				m_cFontLock.Unlock();
				break;
			}
			FontNode *pcNode = *i;

			pcNode->SnapPointSize( &vSize );
			int nError = pcNode->SetProperties( int ( vSize * 64.0f ),
				int ( ( vShear / 360.0f ) * ( 6.283185f * 0x10000f ) ),
				int ( ( vRotation / 360.0f ) * ( 6.283185f * 0x10000f ) ),
				nFlags );

			if( nError < 0 )
			{
				cReply.AddInt32( "error", nError );
				pcReq->SendReply( &cReply );
				m_cFontLock.Unlock();
				break;
			}

			SFontInstance *pcInstance = pcNode->GetInstance();

			if( pcInstance == NULL )
			{
				cReply.AddInt32( "error", -ENOMEM );
				pcReq->SendReply( &cReply );
				m_cFontLock.Unlock();
				break;
			}
			cReply.AddInt32( "error", 0 );
			cReply.AddInt32( "ascender", pcInstance->GetAscender() );
			cReply.AddInt32( "descender", pcInstance->GetDescender() );
			cReply.AddInt32( "line_gap", pcInstance->GetLineGap() );

			pcReq->SendReply( &cReply );
			pcNode->NotifyDependent();
			m_cFontLock.Unlock();
			break;
		}
	case AR_LOCK_DESKTOP:
		{
			int nDesktop;
			Message cReply;

			if( pcReq->FindInt( "desktop", &nDesktop ) != 0 )
			{
				cReply.AddInt32( "error", EINVAL );
			}
			else if( (nDesktop < 32 && nDesktop >= 0) || nDesktop == os::Desktop::ACTIVE_DESKTOP )
			{
				screen_mode sMode;

				std::string cBackdropPath;
				get_desktop_config( &nDesktop, &sMode, &cBackdropPath );

				cReply.AddInt32( "desktop", nDesktop );
				cReply.AddIPoint( "resolution", IPoint( sMode.m_nWidth, sMode.m_nHeight ) );
				cReply.AddInt32( "bytes_per_line", sMode.m_nBytesPerLine );
				cReply.AddInt32( "color_space", sMode.m_eColorSpace );
				cReply.AddFloat( "refresh_rate", sMode.m_vRefreshRate );
				cReply.AddFloat( "h_pos", sMode.m_vHPos );
				cReply.AddFloat( "v_pos", sMode.m_vVPos );
				cReply.AddFloat( "h_size", sMode.m_vHSize );
				cReply.AddFloat( "v_size", sMode.m_vVSize );
				cReply.AddString( "backdrop_path", cBackdropPath.c_str() );
				if( nDesktop == get_active_desktop() || nDesktop == os::Desktop::ACTIVE_DESKTOP )
				{
					cReply.AddInt32( "screen_area", g_pcScreenBitmap->m_hArea );
				}
				cReply.AddInt32( "error", 0 );
			}
			else
			{  /* out-of-range desktop number */
			    dbprintf( "AR_LOCK_DESKTOP: Invalid desktop number %i\n", nDesktop );
				cReply.AddInt32( "error", EINVAL );
			}
			pcReq->SendReply( &cReply );
			break;
		}
	case AR_UNLOCK_DESKTOP:
		break;
	case AR_GET_SCREENMODE_COUNT:
		{
			Message cReply;

			cReply.AddInt32( "count", g_pcDispDrv->GetScreenModeCount() );
			pcReq->SendReply( &cReply );
			break;
		}
	case AR_GET_SCREENMODE_INFO:
		{
			int32 nIndex = 0;

			Message cReply;

			screen_mode sMode;

			if( pcReq->FindInt( "index", &nIndex ) == 0 && g_pcDispDrv->GetScreenModeDesc( nIndex, &sMode ) )
			{

				cReply.AddIPoint( "resolution", IPoint( sMode.m_nWidth, sMode.m_nHeight ) );
				cReply.AddInt32( "bytes_per_line", sMode.m_nBytesPerLine );
				cReply.AddInt32( "color_space", sMode.m_eColorSpace );
				cReply.AddFloat( "refresh_rate", sMode.m_vRefreshRate );
				cReply.AddInt32( "error", EOK );
			}
			else
			{
				dbprintf( "AR_GET_SCREENMODE_INFO: Invalid index %d\n", nIndex );
				cReply.AddInt32( "error", EINVAL );
			}
			pcReq->SendReply( &cReply );
			break;
		}
	case AR_SET_SCREEN_MODE:
		{
			int32 nDesktop;

			Message cReply;

			if( pcReq->FindInt32( "desktop", &nDesktop ) != 0 )
			{
				cReply.AddInt32( "error", EINVAL );
				pcReq->SendReply( &cReply );
				break;
			}

			screen_mode sMode;
			IPoint cScrRes( 640, 480 );
			int32 nColorSpace;

			pcReq->FindIPoint( "resolution", &cScrRes );
			pcReq->FindInt32( "color_space", &nColorSpace );
			pcReq->FindFloat( "refresh_rate", &sMode.m_vRefreshRate );
			pcReq->FindFloat( "h_pos", &sMode.m_vHPos );
			pcReq->FindFloat( "v_pos", &sMode.m_vVPos );
			pcReq->FindFloat( "h_size", &sMode.m_vHSize );
			pcReq->FindFloat( "v_size", &sMode.m_vVSize );

			sMode.m_nWidth = cScrRes.x;
			sMode.m_nHeight = cScrRes.y;
			sMode.m_eColorSpace = static_cast < color_space > ( nColorSpace );

			if( nDesktop >= 0 && nDesktop <= 32 )
			{
				AppserverConfig::GetInstance()->SetDesktopScreenMode( nDesktop, sMode );
				cReply.AddInt32( "error", 0 );
			}
			else
			{
				dbprintf( "Error: SrvApplication::DispatchMessage() invalid desktop %d in AR_SET_SCREEN_MODE request\n", nDesktop );
				cReply.AddInt32( "error", EINVAL );
			}
			pcReq->SendReply( &cReply );
			break;
		}
	case WR_PUSH_CURSOR:
		{
			IPoint cSize;
			IPoint cHotSpot( 0, 0 );
			const void *pImage;
			size_t nImageSize;
			int32 nMode;

			if( pcReq->FindIPoint( "size", &cSize ) != 0 )
			{
				break;
			}
			if( cSize.x < 0 || cSize.x > 64 || cSize.y < 0 || cSize.y > 64 )
			{
				break;
			}
			if( pcReq->FindInt32( "mode", &nMode ) != 0 )
			{
				break;
			}
			if( pcReq->FindData( "image", T_ANY_TYPE, &pImage, &nImageSize ) != 0 )
			{
				break;
			}
			pcReq->FindIPoint( "hot_spot", &cHotSpot );
			if( ( ( nMode == MPTR_MONO || nMode == MPTR_CMAP8 ) && nImageSize != size_t ( cSize.x * cSize.y ) ) ||
				( nMode == MPTR_RGB32 && nImageSize != size_t ( cSize.x * cSize.y * 4 ) ) )
			{
				break;
			}

			Cursor *pcCursor = new Cursor;
			pcCursor->m_pImage = new char[nImageSize];

			pcCursor->m_cSize = cSize;
			pcCursor->m_pcOwner = this;
			pcCursor->m_eMode = static_cast < mouse_ptr_mode > ( nMode );
			pcCursor->m_cHotSpot = cHotSpot;

			memcpy( pcCursor->m_pImage, pImage, nImageSize );
			g_cLayerGate.Close();
			s_cCursorStack.push_back( pcCursor );
			if( s_bBorderCursor == false )
			{
				g_pcDispDrv->SetCursorBitmap( pcCursor->m_eMode, cHotSpot, pcCursor->m_pImage, pcCursor->m_cSize.x, pcCursor->m_cSize.y );
			}
			g_cLayerGate.Open();
			break;
		}
	case WR_POP_CURSOR:
		{
			g_cLayerGate.Close();
			for( int i = s_cCursorStack.size() - 1; i >= 0; --i )
			{
				Cursor *pcCursor = s_cCursorStack[i];

				if( pcCursor->m_pcOwner == this )
				{
					s_cCursorStack.erase( s_cCursorStack.begin() + i );
					delete[]static_cast < char *>( pcCursor->m_pImage );
					delete pcCursor;

					if( s_bBorderCursor == false && i == int ( s_cCursorStack.size() ) )
					{
						if( s_cCursorStack.size() > 0 )
						{
							pcCursor = s_cCursorStack[s_cCursorStack.size() - 1];
							g_pcDispDrv->SetCursorBitmap( pcCursor->m_eMode, pcCursor->m_cHotSpot, pcCursor->m_pImage, pcCursor->m_cSize.x, pcCursor->m_cSize.y );
						}
						else
						{
							g_pcDispDrv->SetCursorBitmap( MPTR_RGB32, IPoint( 0, 0 ), g_anMouseImg, POINTER_WIDTH, POINTER_HEIGHT );
						}
					}
					break;
				}
			}
			g_cLayerGate.Open();
			break;
		}
	case WR_CREATE_VIDEO_OVERLAY:
		{
			IPoint cSize;
			IRect cDst;
			int32 nFormat;
			color_space eFormat;
			Message cReply;
			Color32_s sColorKey;
			area_id hArea;

			if( pcReq->FindIPoint( "size", &cSize ) != 0 )
			{
				cReply.AddInt32( "error", EINVAL );
				pcReq->SendReply( &cReply );
				break;
			}
			if( pcReq->FindIRect( "dst", &cDst ) != 0 )
			{
				cReply.AddInt32( "error", EINVAL );
				pcReq->SendReply( &cReply );
				break;
			}
			if( pcReq->FindInt32( "format", &nFormat ) != 0 )
			{
				cReply.AddInt32( "error", EINVAL );
				pcReq->SendReply( &cReply );
				break;
			}

			if( pcReq->FindColor32( "color_key", &sColorKey ) != 0 )
			{
				cReply.AddInt32( "error", EINVAL );
				pcReq->SendReply( &cReply );
				break;
			}

			eFormat = static_cast < color_space > ( nFormat );

			g_cLayerGate.Close();
			if( g_pcDispDrv->CreateVideoOverlay( cSize, cDst, eFormat, sColorKey, &hArea ) )
			{
				cReply.AddInt32( "error", 0 );
				cReply.AddInt32( "area", hArea );
				pcReq->SendReply( &cReply );
			}
			else
			{
				cReply.AddInt32( "error", EINVAL );
				pcReq->SendReply( &cReply );
			}
			g_cLayerGate.Open();
			break;
		}
	case WR_RECREATE_VIDEO_OVERLAY:
		{
			IPoint cSize;
			IRect cDst;
			int32 nFormat;
			color_space eFormat;
			Message cReply;
			area_id hArea;

			if( pcReq->FindIPoint( "size", &cSize ) != 0 )
			{
				cReply.AddInt32( "error", EINVAL );
				pcReq->SendReply( &cReply );
				break;
			}
			if( pcReq->FindIRect( "dst", &cDst ) != 0 )
			{
				cReply.AddInt32( "error", EINVAL );
				pcReq->SendReply( &cReply );
				break;
			}
			if( pcReq->FindInt32( "format", &nFormat ) != 0 )
			{
				cReply.AddInt32( "error", EINVAL );
				pcReq->SendReply( &cReply );
				break;
			}
			if( pcReq->FindInt32( "area", ( int32 * )&hArea ) != 0 )
			{
				cReply.AddInt32( "error", EINVAL );
				pcReq->SendReply( &cReply );
				break;
			}

			eFormat = static_cast < color_space > ( nFormat );

			g_cLayerGate.Close();
			if( g_pcDispDrv->RecreateVideoOverlay( cSize, cDst, eFormat, &hArea ) )
			{
				cReply.AddInt32( "error", 0 );
				cReply.AddInt32( "area", hArea );
				pcReq->SendReply( &cReply );
			}
			else
			{
				cReply.AddInt32( "error", EINVAL );
				pcReq->SendReply( &cReply );
			}
			g_cLayerGate.Open();
			break;
		}
	case WR_UPDATE_VIDEO_OVERLAY:
		{
			/* Deprecated */
			break;
		}
	case WR_DELETE_VIDEO_OVERLAY:
		{
			area_id hArea;

			if( pcReq->FindInt32( "area", ( int32 * )&hArea ) != 0 )
			{
				break;
			}
			g_cLayerGate.Close();
			g_pcDispDrv->DeleteVideoOverlay( &hArea );
			g_cLayerGate.Open();
			break;
		}

	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool SrvApplication::DispatchMessage( const void *pMsg, int nCode )
{
	switch ( nCode )
	{
	case AR_OPEN_WINDOW:
	case AR_OPEN_BITMAP_WINDOW:
	case AR_CLOSE_WINDOW:
	case AR_GET_IDLE_TIME:
	case AR_GET_FONT_FAMILY_COUNT:
	case AR_GET_FONT_FAMILY:
	case AR_GET_FONT_STYLE_COUNT:
	case AR_GET_FONT_STYLE:
	case AR_GET_FONT_SIZES:
	case AR_CREATE_FONT:
	case AR_DELETE_FONT:
	case AR_SET_FONT_FAMILY_AND_STYLE:
	case AR_SET_FONT_PROPERTIES:
	case AR_GET_FONT_CHARACTERS:
	case AR_LOCK_DESKTOP:
	case AR_UNLOCK_DESKTOP:
	case AR_GET_SCREENMODE_COUNT:
	case AR_GET_SCREENMODE_INFO:
	case AR_SET_SCREEN_MODE:
	case WR_PUSH_CURSOR:
	case WR_POP_CURSOR:
	case WR_CREATE_VIDEO_OVERLAY:
	case WR_RECREATE_VIDEO_OVERLAY:
	case WR_UPDATE_VIDEO_OVERLAY:
	case WR_DELETE_VIDEO_OVERLAY:
		{
			try
			{
				Message cReq( pMsg );

				DispatchMessage( &cReq );
			}
			catch( ... )
			{
				dbprintf( "Catched exception while handling request %d\n", nCode );
			}
			return ( true );
		}
	case AR_DELETE_WINDOW:
		{
			g_cLayerGate.Close();
			SrvWindow *pcWindow = ( ( AR_DeleteWindow_s * ) pMsg )->pcWindow;

			m_cWindows.erase( pcWindow );
			delete pcWindow;

			g_cLayerGate.Open();
			return ( true );
		}
	case AR_GET_TEXT_EXTENTS:
		{
			AR_GetTextExtents_s *psReq = ( AR_GetTextExtents_s * ) pMsg;
			AR_GetTextExtentsReply_s *psReply = ( AR_GetTextExtentsReply_s * ) pMsg;
			int nStrCount = psReq->nStringCount;
			port_id hReplyPort = psReq->hReply;
			uint32 nFlags = psReq->nFlags;
			int nTargetWidth = psReq->nTargetWidth;

			if( m_cFontLock.Lock() == 0 )
			{
				std::set < FontNode * >::iterator i = m_cFonts.find( ( FontNode * ) psReq->hFontToken );

				if( i == m_cFonts.end() )
				{
					psReply->nError = -ENOENT;
					dbprintf( "Error : GetTextExtent() called on invalid font ID %x\n", psReq->hFontToken );
					send_msg( hReplyPort, 0, psReply, sizeof( AR_GetTextExtentsReply_s ) );
					m_cFontLock.Unlock();
					return ( true );
				}

				SFontInstance *pcFont = ( *i )->GetInstance();

				if( NULL == pcFont )
				{
					dbprintf( "Error : GetTextExtent() FontNode::GetInstance() returned NULL\n" );

					psReply->nError = -ENOMEM;
					send_msg( hReplyPort, 0, psReply, sizeof( AR_GetTextExtentsReply_s ) );
					m_cFontLock.Unlock();
					return ( true );
				}

				StringHeader_s *psHdr = &psReq->sFirstHeader;

				for( int i = 0; i < nStrCount; ++i )
				{
					if( psHdr->zString + psHdr->nLength >= reinterpret_cast < const char *>( pMsg ) + 8192 )
					{
						printf( "Error: GetTextExtents() invalid size %d\n", psHdr->nLength );
						psReply->nError = -EINVAL;
						break;
					}

					psReply->acExtent[i] = pcFont->GetTextExtent( psHdr->zString, psHdr->nLength, nFlags, nTargetWidth );

					int nHdrSize = psHdr->nLength + sizeof( int );

					psHdr = ( StringHeader_s * ) ( ( ( int8 * )psHdr ) + nHdrSize );
				}

				psReply->nError = 0;

				send_msg( hReplyPort, 0, psReply, sizeof( AR_GetTextExtentsReply_s ) );
				m_cFontLock.Unlock();
			}

			return ( true );
		}
	case AR_GET_STRING_WIDTHS:
		{
			AR_GetStringWidths_s *psReq = ( AR_GetStringWidths_s * ) pMsg;
			AR_GetStringWidthsReply_s *psReply = ( AR_GetStringWidthsReply_s * ) pMsg;

			int nStrCount = psReq->nStringCount;
			port_id hReplyPort = psReq->hReply;

			if( m_cFontLock.Lock() == 0 )
			{
				std::set < FontNode * >::iterator i = m_cFonts.find( ( FontNode * ) psReq->hFontToken );

				if( i == m_cFonts.end() )
				{
					psReply->nError = -ENOENT;
					dbprintf( "Error : GetStringWidths() called on invalid font ID %x\n", psReq->hFontToken );
					send_msg( hReplyPort, 0, psReply, sizeof( AR_GetStringWidthsReply_s ) );
					m_cFontLock.Unlock();
					return ( true );
				}

				SFontInstance *pcFont = ( *i )->GetInstance();

				if( NULL == pcFont )
				{
					dbprintf( "Error : GetStringWidths() FontNode::GetInstance() returned NULL\n" );

					psReply->nError = -ENOMEM;
					send_msg( hReplyPort, 0, psReply, sizeof( AR_GetStringWidthsReply_s ) );
					m_cFontLock.Unlock();
					return ( true );
				}

				StringHeader_s *psHdr = &psReq->sFirstHeader;

				for( int i = 0; i < nStrCount; ++i )
				{
					if( psHdr->zString + psHdr->nLength >= reinterpret_cast < const char *>( pMsg ) + 8192 )
					{
						printf( "Error: GetStringWidths() invalid size %d\n", psHdr->nLength );
						psReply->nError = -EINVAL;
						break;
					}
					psReply->anLengths[i] = pcFont->GetStringWidth( psHdr->zString, psHdr->nLength );

					int nHdrSize = psHdr->nLength + sizeof( int );

					psHdr = ( StringHeader_s * ) ( ( ( int8 * )psHdr ) + nHdrSize );
				}
				psReply->nError = 0;
				send_msg( hReplyPort, 0, psReply, sizeof( AR_GetStringWidthsReply_s ) + ( nStrCount - 1 ) * sizeof( int ) );
				m_cFontLock.Unlock();
			}
			return ( true );
		}
	case AR_GET_STRING_LENGTHS:
		{
			AR_GetStringLengths_s *psReq = ( AR_GetStringLengths_s * ) pMsg;
			AR_GetStringLengthsReply_s *psReply = ( AR_GetStringLengthsReply_s * ) pMsg;

			port_id hReplyPort = psReq->hReply;
			int nStrCount = psReq->nStringCount;

			if( m_cFontLock.Lock() == 0 )
			{

				std::set < FontNode * >::iterator i = m_cFonts.find( ( FontNode * ) psReq->hFontToken );

				if( i == m_cFonts.end() )
				{
					psReply->nError = -ENOENT;
					dbprintf( "Error : GetStringLengths() called on invalid font ID %x\n", psReq->hFontToken );
					send_msg( hReplyPort, 0, psReply, sizeof( AR_GetStringLengthsReply_s ) );
					m_cFontLock.Unlock();
					return ( true );
				}
				SFontInstance *pcFont = ( *i )->GetInstance();

				if( NULL == pcFont )
				{
					dbprintf( "Error : GetStringLengths() FontNode::GetInstance() returned NULL\n" );

					psReply->nError = -ENOMEM;
					send_msg( hReplyPort, 0, psReply, sizeof( AR_GetStringLengthsReply_s ) );
					m_cFontLock.Unlock();
					return ( true );
				}

				StringHeader_s *psHdr = &psReq->sFirstHeader;

				for( int i = 0; i < nStrCount; ++i )
				{
					if( psHdr->zString + psHdr->nLength >= reinterpret_cast < const char *>( pMsg ) + 8192 )
					{
						psReply->nError = -EINVAL;
						dbprintf( "Error: GetStringLengths() invalid size %d\n", psHdr->nLength );
						break;
					}

					psReply->anLengths[i] = pcFont->GetStringLength( psHdr->zString, psHdr->nLength, psReq->nWidth, psReq->bIncludeLast );

					int nHdrSize = psHdr->nLength + sizeof( int );

					psHdr = ( StringHeader_s * ) ( ( ( int8 * )psHdr ) + nHdrSize );
				}
				psReply->nError = 0;
				send_msg( hReplyPort, 0, psReply, sizeof( AR_GetStringLengthsReply_s ) + ( nStrCount - 1 ) * sizeof( int ) );
				m_cFontLock.Unlock();
			}
			return ( true );
		}
	case AR_CREATE_BITMAP:
		{
			AR_CreateBitmap_s *psReq = ( AR_CreateBitmap_s * ) pMsg;

			g_cLayerGate.Close();
			CreateBitmap( psReq->hReply, psReq->nWidth, psReq->nHeight, psReq->eColorSpc, psReq->nFlags );
			g_cLayerGate.Open();
			return ( true );
		}
	case AR_CLONE_BITMAP:
		{
			AR_CloneBitmap_s *psReq = ( AR_CloneBitmap_s * ) pMsg;

			g_cLayerGate.Close();
			CloneBitmap( psReq->m_hReply, psReq->m_hHandle );
			g_cLayerGate.Open();
			return ( true );
		}
	case AR_DELETE_BITMAP:
		{
			AR_DeleteBitmap_s *psReq = ( AR_DeleteBitmap_s * ) pMsg;

			g_cLayerGate.Close();
			DeleteBitmap( psReq->m_nHandle );
			g_cLayerGate.Open();
			return ( true );
		}
	case AR_GET_QUALIFIERS:
		{
			AR_GetQualifiers_s *psReq = ( AR_GetQualifiers_s * ) pMsg;
			AR_GetQualifiersReply_s sReply( GetQualifiers() );

			send_msg( psReq->m_hReply, 0, &sReply, sizeof( sReply ) );
			return ( true );
		}
	case AR_CREATE_SPRITE:
		{
			AR_CreateSprite_s *psReq = ( AR_CreateSprite_s * ) pMsg;
			SrvBitmap *pcBitmap = NULL;
			int nError;
			uint32 nHandle;



			if( psReq->m_nBitmap != -1 )
			{
				pcBitmap = g_pcBitmaps->GetObj( psReq->m_nBitmap );

				if( pcBitmap == NULL )
				{
					dbprintf( "Error: Attempt to create sprite with invalid bitmap %d\n", psReq->m_nBitmap );
				}
			}
			g_cLayerGate.Close();
			SrvSprite *pcSprite = new SrvSprite( static_cast < IRect > ( psReq->m_cFrame ), IPoint( 0, 0 ), IPoint( 0, 0 ),
				g_pcTopView->GetBitmap(),
				pcBitmap );

			g_cLayerGate.Open();

			m_cSprites.insert( pcSprite );
			nHandle = ( uint32 )pcSprite;
			nError = 0;


			AR_CreateSpriteReply_s sReply( nHandle, nError );

			send_msg( psReq->m_hReply, 0, &sReply, sizeof( sReply ) );
			return ( true );
		}
	case AR_DELETE_SPRITE:
		{
			AR_DeleteSprite_s *psReq = ( AR_DeleteSprite_s * ) pMsg;
			SrvSprite *pcSprite = ( SrvSprite * ) psReq->m_nSprite;


			std::set < SrvSprite * >::iterator i = m_cSprites.find( pcSprite );

			if( i != m_cSprites.end() )
			{
				m_cSprites.erase( i );

				g_cLayerGate.Close();
				delete pcSprite;

				g_cLayerGate.Open();

			}
			else
			{
				dbprintf( "Error: Attempt to delete invalid sprite %d\n", psReq->m_nSprite );
			}
			return ( true );
		}
	case AR_MOVE_SPRITE:
		{
			AR_MoveSprite_s *psReq = ( AR_MoveSprite_s * ) pMsg;
			SrvSprite *pcSprite = ( SrvSprite * ) psReq->m_nSprite;

			if( m_cSprites.find( pcSprite ) != m_cSprites.end() )
			{
				g_cLayerGate.Close();
//      g_pcDispDrv->MouseOff();
				pcSprite->MoveTo( static_cast < IPoint > ( psReq->m_cNewPos ) );
//      g_pcDispDrv->MouseOn();
				g_cLayerGate.Open();
			}
			return ( true );
		}
	case M_QUIT:
		{
			std::set < SrvWindow * >::iterator i;
			g_cLayerGate.Close();
			for( i = m_cWindows.begin(); i != m_cWindows.end(  ); ++i )
			{
				SrvWindow *pcWnd = *i;

				pcWnd->Quit();
			}
			g_cLayerGate.Open();
			return ( false );
		}
	case WR_CREATE_VIEW:
	case WR_DELETE_VIEW:
	case WR_TOGGLE_VIEW_DEPTH:
		{
			Message cReq( pMsg );

			int hTopView = -1;

			cReq.FindInt( "top_view", &hTopView );

			g_cLayerGate.Lock();

			Layer *pcTopLayer = FindLayer( hTopView );

			if( pcTopLayer == NULL )
			{
				dbprintf( "Error: SrvApplication::DispatchMessage() message to invalid window %d\n", hTopView );
				g_cLayerGate.Unlock();
				return ( true );
			}
			SrvWindow *pcWindow = pcTopLayer->GetWindow();

			if( pcWindow == NULL )
			{
				dbprintf( "Error: SrvApplication::DispatchMessage() top-view has no window! Can't forward message\n" );
				g_cLayerGate.Unlock();
				return ( true );
			}
			g_cLayerGate.Unlock();
			pcWindow->DispatchMessage( &cReq );
			return ( true );
		}
		break;
	case WR_GET_VIEW_FRAME:
	case WR_RENDER:
	case WR_GET_PEN_POSITION:
		{
			const WR_Request_s *psReq = static_cast < const WR_Request_s * >( pMsg );

			g_cLayerGate.Lock();

			Layer *pcTopLayer = FindLayer( psReq->m_hTopView );

			if( pcTopLayer == NULL )
			{
				dbprintf( "Error: SrvApplication::DispatchMessage() message to invalid window %d\n", psReq->m_hTopView );
				g_cLayerGate.Unlock();
				return ( true );
			}
			SrvWindow *pcWindow = pcTopLayer->GetWindow();

			if( pcWindow == NULL )
			{
				dbprintf( "Error: SrvApplication::DispatchMessage() top-view has no window! Can't forward message\n" );
				g_cLayerGate.Unlock();
				return ( true );
			}
			g_cLayerGate.Unlock();
			pcWindow->DispatchMessage( pMsg, nCode );
			return ( true );
		}
	default:
		dbprintf( "Error: SrvApplication::DispatchMessage() unknown message %d\n", nCode );
		return ( true );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SrvApplication::Loop()
{
	char* pMsg = new char[8192];
	size_t nSize, nOldSize = 8192;
	bool bDoLoop = true;

	while( bDoLoop || m_cWindows.empty() == false )
	{
		uint32 nCode;

		nSize = std::max( (int)get_msg_size( m_hReqPort ), 8192 );
		if( nSize != nOldSize )
		{
			delete[]pMsg;
			pMsg = new char[nSize];
		}
		nOldSize = nSize;

		if( get_msg( m_hReqPort, &nCode, pMsg, nSize ) >= 0 )
		{
			Lock();
			if( DispatchMessage( pMsg, nCode ) == false )
			{
				bDoLoop = false;
			}
			Unlock();
		}
	}
	delete[]pMsg;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

uint32 SrvApplication::Entry( void *pData )
{
	SrvApplication *pcApp = ( SrvApplication * ) pData;

	signal( SIGINT, SIG_IGN );
	signal( SIGQUIT, SIG_IGN );
	signal( SIGTERM, SIG_IGN );

	pcApp->Loop();
	delete pcApp;

	exit_thread( 0 );
	return ( 0 );
}

SrvApplication *get_active_app()
{
	SrvWindow *pcWnd = get_active_window( true );

	if( pcWnd == NULL )
	{
		return ( NULL );
	}
	else
	{
		return ( pcWnd->GetApp() );
	}
}



