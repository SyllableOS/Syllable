
/*
 *  The AtheOS application server
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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
#include <dirent.h>
#include <assert.h>

#include <atheos/areas.h>
#include <atheos/image.h>
#include <atheos/device.h>

#include <gui/desktop.h>
#include <gui/window.h>

#include <util/message.h>
#include <util/messenger.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "swindow.h"
#include "sapplication.h"
#include "wndborder.h"
#include "layer.h"
#include "server.h"
#include "config.h"
#include "desktop.h"
#include "bitmap.h"
#include "sprite.h"
#include "inputnode.h"
#include "vesadrv.h"

#include <macros.h>

#include <set>
#include <algorithm>

using namespace os;

#define POINTER_WIDTH 11
#define POINTER_HEIGHT 18


uint8 g_anMouseImg[] = {
"\377\377\377\377\377\377\377\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377\377\377\246\246\246\377\377\377"
  "\377\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\377\377\377\377\2\2\2\377\246\246\246\377\377\377\377\377\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377\377\377\14\14\14\377I"
  "II\377\246\246\246\377\377\377\377\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\377\377\377\377\24\24\24\377]]]\377...\377\246\246\246\377"
  "\377\377\377\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377\377\377"
  "\34\34\34\377lll\377\40\40\40\377---\377\246\246\246\377\377\377\377\377"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377\377\377%%%\377\201\201\201\377L"
  "LL\377\40\40\40\377\"\"\"\377\246\246\246\377\377\377\377\377\0\0\0\0\0\0"
  "\0\0\0\0\0\0\377\377\377\377+++\377\215\215\215\377bbb\377%%%\377\24\24\24"
  "\377\24\24\24\377\246\246\246\377\377\377\377\377\0\0\0\0\0\0\0\0\377\377"
  "\377\377%%%\377\207\207\207\377iii\377+++\377\21\21\21\377\7\7\7\377\7\7"
  "\7\377\246\246\246\377\377\377\377\377\0\0\0\0\377\377\377\377\34\34\34\377"
  "www\377___\377\"\"\"\377\15\15\15\377\2\2\2\377\2\2\2\377\2\2\2\377\246\246"
  "\246\377\377\377\377\377\377\377\377\377666\377___\377'''\377999\377555\377"
  "\346\346\346\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\14\14\14\377+++\377\314\314\314\377\215\215\215"
  "\377===\377\246\246\246\377\354\354\354\377\0\0\0\0\0\0\0\0\0\0\0\0\377\377"
  "\377\377\7\7\7\377\315\315\315\377\373\373\373\377\324\324\324\377DDD\377"
  "EEE\377\365\365\365\377\0\0\0\0\0\0\0\0\0\0\0\0\377\377\377\377\312\312\312"
  "\377\362\362\362\377\0\0\0\0\375\375\375\377]]]\377---\377\304\304\304\377"
  "\335\335\335\377\0\0\0\0\0\0\0\0\377\377\377\377\363\363\363\377\0\0\0\0"
  "\0\0\0\0\352\352\352\377\267\267\267\377---\377PPP\377\373\373\373\377\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\357\357\357\377(("
  "(\377\26\26\26\377\334\334\334\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\363\363\363\377\223\223\223\377///\377\323\323\323\377"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\356\356"
  "\356\377\375\375\375\377\0\0\0\0\0\0\0\0\0\0\0\0"
};

SrvBitmap *g_pcScreenBitmap = NULL;
int g_nFrameBufferArea = -1;

static int g_nActiveDesktop = -1;
static int g_nPrevDesktop = 1;

#define FOCUS_STACK_SIZE  32

static struct Desktop
{
	SrvWindow *m_pcFirstWindow;
	SrvWindow *m_pcFirstSystemWindow;
	SrvWindow *m_pcActiveWindow;
	SrvWindow *m_apcFocusStack[FOCUS_STACK_SIZE];
	screen_mode m_sScreenMode;
	std::string m_cBackdropPath;
	os::Rect m_cMaxWindowFrame;
} g_asDesktops[32];


void set_desktop_config( int nDesktop, const screen_mode & sMode, const std::string & cBackDropPath )
{
	assert( nDesktop >= 0 && nDesktop < 32 );

	g_asDesktops[nDesktop].m_sScreenMode = sMode;
	g_asDesktops[nDesktop].m_cMaxWindowFrame = os::Rect( 0, 0, sMode.m_nWidth - 1, sMode.m_nHeight - 1 );
}


int get_desktop_config( int *pnActiveDesktop, screen_mode * psMode, std::string * pcBackdropPath )
{
	if( *pnActiveDesktop == os::Desktop::ACTIVE_DESKTOP )
	{
		*pnActiveDesktop = g_nActiveDesktop;
	}
	if( psMode != NULL )
	{
		*psMode = g_asDesktops[*pnActiveDesktop].m_sScreenMode;
	}
	if( pcBackdropPath != NULL )
	{
		*pcBackdropPath = g_asDesktops[*pnActiveDesktop].m_cBackdropPath;
	}
	return ( 0 );
}

SrvWindow *get_first_window( int nDesktop )
{
	return ( g_asDesktops[nDesktop].m_pcFirstWindow );
}

SrvWindow *get_active_window( bool bIgnoreSystemWindows )
{
	if( bIgnoreSystemWindows == false && g_asDesktops[g_nActiveDesktop].m_pcFirstSystemWindow != NULL )
	{
		return ( g_asDesktops[g_nActiveDesktop].m_pcFirstSystemWindow );
	}
	else
	{
		return ( g_asDesktops[g_nActiveDesktop].m_pcActiveWindow );
	}
}

void set_active_window( SrvWindow * pcWindow, bool bNotifyPrevious )
{
	if( pcWindow != NULL && ( ( pcWindow->GetFlags() & WND_SYSTEM ) || !( pcWindow->GetDesktopMask() & ( 1 << g_nActiveDesktop ) ) ) )
	{
		return;
	}
	if( pcWindow == NULL && g_asDesktops[g_nActiveDesktop].m_pcFirstSystemWindow != NULL )
	{
		if( !bNotifyPrevious )
			g_asDesktops[g_nActiveDesktop].m_pcActiveWindow = NULL;
		return;
	}
	if( ( pcWindow == g_asDesktops[g_nActiveDesktop].m_pcActiveWindow ) && ( pcWindow != NULL )  )
	{
		return;
	}
	SrvWindow *pcPrevious = g_asDesktops[g_nActiveDesktop].m_pcActiveWindow;

	if( pcWindow == NULL )
	{
		g_asDesktops[g_nActiveDesktop].m_pcActiveWindow = g_asDesktops[g_nActiveDesktop].m_apcFocusStack[0];
		memmove( &g_asDesktops[g_nActiveDesktop].m_apcFocusStack[0], &g_asDesktops[g_nActiveDesktop].m_apcFocusStack[1], sizeof( SrvWindow * ) * ( FOCUS_STACK_SIZE - 1 ) );
		g_asDesktops[g_nActiveDesktop].m_apcFocusStack[FOCUS_STACK_SIZE - 1] = NULL;
	}
	else
	{
		memmove( &g_asDesktops[g_nActiveDesktop].m_apcFocusStack[1], &g_asDesktops[g_nActiveDesktop].m_apcFocusStack[0], sizeof( SrvWindow * ) * ( FOCUS_STACK_SIZE - 1 ) );
		g_asDesktops[g_nActiveDesktop].m_apcFocusStack[0] = g_asDesktops[g_nActiveDesktop].m_pcActiveWindow;
		g_asDesktops[g_nActiveDesktop].m_pcActiveWindow = pcWindow;
	}
	if( pcPrevious != g_asDesktops[g_nActiveDesktop].m_pcActiveWindow )
	{
		if( bNotifyPrevious && pcPrevious != NULL )
		{
			pcPrevious->WindowActivated( false );
		}
		if( g_asDesktops[g_nActiveDesktop].m_pcActiveWindow != NULL )
		{
			g_asDesktops[g_nActiveDesktop].m_pcActiveWindow->WindowActivated( true );
		}
	}
	if( pcPrevious != NULL && pcWindow == NULL )
		__assertw( g_asDesktops[g_nActiveDesktop].m_pcActiveWindow != pcPrevious );
}

class ModeCmp
{
      public:
	bool operator () ( const screen_mode & cMode1, const screen_mode & cMode2 )
	{
		if( BitsPerPixel( cMode1.m_eColorSpace ) != BitsPerPixel( cMode2.m_eColorSpace ) )
		{
			return ( BitsPerPixel( cMode1.m_eColorSpace ) < BitsPerPixel( cMode2.m_eColorSpace ) );
		}
		else
		{
			return ( cMode1.m_nWidth < cMode2.m_nWidth );
		}
	}
};

static bool setup_screenmode( int nDesktop, bool bForce )
{
	if( bForce == false && nDesktop == g_nActiveDesktop )
	{
		return ( false );
	}
	if( bForce == false && g_nActiveDesktop != -1 && g_asDesktops[nDesktop].m_sScreenMode.m_nWidth == g_asDesktops[g_nActiveDesktop].m_sScreenMode.m_nWidth && g_asDesktops[nDesktop].m_sScreenMode.m_nHeight == g_asDesktops[g_nActiveDesktop].m_sScreenMode.m_nHeight && g_asDesktops[nDesktop].m_sScreenMode.m_eColorSpace == g_asDesktops[g_nActiveDesktop].m_sScreenMode.m_eColorSpace )
	{
		return ( false );
	}
	bool bModeSet = true;
	
	if( g_nActiveDesktop != -1 )
		g_pcTopView->FreeBackbuffers();

	if( g_pcDispDrv->SetScreenMode( g_asDesktops[nDesktop].m_sScreenMode ) != 0 )
	{
		bModeSet = false;
		std::vector < screen_mode > cModes;
		int nModeCount = g_pcDispDrv->GetScreenModeCount();

		for( int i = 0; i < nModeCount; ++i )
		{
			screen_mode sMode;

			if( g_pcDispDrv->GetScreenModeDesc( i, &sMode ) == false )
			{
				continue;
			}
			cModes.push_back( sMode );
		}
		if( cModes.size() == 0 )
		{
			dbprintf( "Error: Display driver does not list any screen modes. Please check that your video adapter supports VESA 2.0\n" );
			dbprintf( "Terminating application server...\n" );
			exit( 1 );
		}
		std::sort( cModes.begin(), cModes.end(  ), ModeCmp(  ) );
		// Find the mode closest to 640x480
		for( uint i = 0; i < cModes.size(); ++i )
		{
			if( cModes[i].m_nWidth >= 640 || i == cModes.size() - 1 )
			{
				if( cModes[i].m_nWidth > 640 && i > 0 )
				{
					i--;
				}
				dbprintf( "Error: Failed to set screen mode %dx%d %dbpp on desktop %d. Reverting to %dx%d %dbpp\n", g_asDesktops[nDesktop].m_sScreenMode.m_nWidth, g_asDesktops[nDesktop].m_sScreenMode.m_nHeight, BitsPerPixel( g_asDesktops[nDesktop].m_sScreenMode.m_eColorSpace ), nDesktop, cModes[i].m_nWidth, cModes[i].m_nHeight, BitsPerPixel( cModes[i].m_eColorSpace ) );

				g_asDesktops[nDesktop].m_sScreenMode.m_nWidth = cModes[i].m_nWidth;
				g_asDesktops[nDesktop].m_sScreenMode.m_nHeight = cModes[i].m_nHeight;
				g_asDesktops[nDesktop].m_sScreenMode.m_nBytesPerLine = cModes[i].m_nBytesPerLine;
				g_asDesktops[nDesktop].m_sScreenMode.m_eColorSpace = cModes[i].m_eColorSpace;
				g_asDesktops[nDesktop].m_sScreenMode.m_vRefreshRate = 60.0f;
				g_asDesktops[nDesktop].m_cMaxWindowFrame = os::Rect( 0, 0, g_asDesktops[nDesktop].m_sScreenMode.m_nWidth - 1,
												g_asDesktops[nDesktop].m_sScreenMode.m_nHeight - 1 );


				// If this fails we try all the screen-modes before giving up.
				if( g_pcDispDrv->SetScreenMode( g_asDesktops[nDesktop].m_sScreenMode ) == 0 )
				{
					bModeSet = true;
					break;
				}
				for( i = 0; i < cModes.size(); ++i )
				{
					dbprintf( "Error: Failed to set screen mode %dx%d %dbpp on desktop %d. Reverting to %dx%d %dbpp\n", g_asDesktops[nDesktop].m_sScreenMode.m_nWidth, g_asDesktops[nDesktop].m_sScreenMode.m_nHeight, BitsPerPixel( g_asDesktops[nDesktop].m_sScreenMode.m_eColorSpace ), nDesktop, cModes[i].m_nWidth, cModes[i].m_nHeight, BitsPerPixel( cModes[i].m_eColorSpace ) );

					g_asDesktops[nDesktop].m_sScreenMode.m_nWidth = cModes[i].m_nWidth;
					g_asDesktops[nDesktop].m_sScreenMode.m_nHeight = cModes[i].m_nHeight;
					g_asDesktops[nDesktop].m_sScreenMode.m_nBytesPerLine = cModes[i].m_nBytesPerLine;
					g_asDesktops[nDesktop].m_sScreenMode.m_eColorSpace = cModes[i].m_eColorSpace;
					g_asDesktops[nDesktop].m_sScreenMode.m_vRefreshRate = 60.0f;
					g_asDesktops[nDesktop].m_cMaxWindowFrame = os::Rect( 0, 0, g_asDesktops[nDesktop].m_sScreenMode.m_nWidth - 1,
												g_asDesktops[nDesktop].m_sScreenMode.m_nHeight - 1 );

					if( g_pcDispDrv->SetScreenMode( g_asDesktops[nDesktop].m_sScreenMode ) == 0 )
					{
						bModeSet = true;
						break;
					}
				}

				break;
			}
		}
	}
	if( bModeSet == false )
	{
		dbprintf( "Error: All screen modes failed. Game over!\n" );
		dbprintf( "Terminating application server...\n" );
		exit( 1 );
	}
	
	AreaInfo_s sFBAreaInfo;
	get_area_info( g_nFrameBufferArea, &sFBAreaInfo );
	g_pcScreenBitmap->m_nWidth = g_pcDispDrv->GetCurrentScreenMode().m_nWidth;
	g_pcScreenBitmap->m_nHeight = g_pcDispDrv->GetCurrentScreenMode().m_nHeight;
	g_pcScreenBitmap->m_nBytesPerLine = g_pcDispDrv->GetCurrentScreenMode().m_nBytesPerLine;
	g_pcScreenBitmap->m_eColorSpc = g_pcDispDrv->GetCurrentScreenMode().m_eColorSpace;
	g_pcScreenBitmap->m_pRaster = ( ( uint8 * )sFBAreaInfo.pAddress ) + g_pcDispDrv->GetFramebufferOffset();
	g_pcScreenBitmap->m_nVideoMemOffset = g_pcDispDrv->GetFramebufferOffset();
	// update screenmode with correct nBytesPerLine
	g_asDesktops[nDesktop].m_sScreenMode.m_nBytesPerLine = g_pcScreenBitmap->m_nBytesPerLine;
	
	if( g_nActiveDesktop != -1 )
	{
		g_pcTopView->SetFrame( Rect( 0, 0, g_pcScreenBitmap->m_nWidth - 1, g_pcScreenBitmap->m_nHeight - 1 ) );
		if( g_asDesktops[nDesktop].m_sScreenMode.m_eColorSpace != g_asDesktops[g_nActiveDesktop].m_sScreenMode.m_eColorSpace )
		{
			SrvSprite::ColorSpaceChanged();
		}
	
		Point cMousePos = InputNode::GetMousePos();

		cMousePos.x = floor( cMousePos.x * g_asDesktops[nDesktop].m_sScreenMode.m_nWidth / g_asDesktops[g_nActiveDesktop].m_sScreenMode.m_nWidth );
		cMousePos.y = floor( cMousePos.y * g_asDesktops[nDesktop].m_sScreenMode.m_nHeight / g_asDesktops[g_nActiveDesktop].m_sScreenMode.m_nHeight );
		InputNode::SetMousePos( cMousePos );
	}
	return ( true );
}

void set_desktop_screenmode( int nDesktop, const screen_mode & sMode )
{
	assert( nDesktop >= 0 && nDesktop < 32 );
	SrvWindow *pcWindow;
	int nOldWidth = g_asDesktops[nDesktop].m_sScreenMode.m_nWidth, nOldHeight = g_asDesktops[nDesktop].m_sScreenMode.m_nHeight;

	g_asDesktops[nDesktop].m_sScreenMode = sMode;
	
	if( nDesktop == g_nActiveDesktop )
	{
		setup_screenmode( nDesktop, true );
		g_asDesktops[nDesktop].m_cMaxWindowFrame.left = g_asDesktops[nDesktop].m_cMaxWindowFrame.top = 0;
		g_asDesktops[nDesktop].m_cMaxWindowFrame.right = g_asDesktops[nDesktop].m_sScreenMode.m_nWidth - 1;
		g_asDesktops[nDesktop].m_cMaxWindowFrame.bottom = g_asDesktops[nDesktop].m_sScreenMode.m_nHeight - 1;
		
		/* The following code copied from set_desktop() */
		int nMoveTally = 0;
		
		/* Notify windows of res change, make sure all windows are on-screen if changing to smaller resolution */
		for( pcWindow = g_asDesktops[nDesktop].m_pcFirstWindow; pcWindow != NULL; pcWindow = pcWindow->m_asDTState[nDesktop].m_pcNextWindow )
		{
			Layer *pcTopWid = pcWindow->GetTopView();
			Rect cFrame = pcWindow->GetFrame();
			Rect cNewFrame = cFrame;
			
			/* If screen resolution has decreased, make sure no windows are offscreen */
			if( g_asDesktops[nDesktop].m_sScreenMode.m_nWidth < nOldWidth || g_asDesktops[nDesktop].m_sScreenMode.m_nHeight < nOldHeight )
			{
				bool bMoved = false;
				/* move a window only if too little (or none) of it is visible; don't resize any windows */
				if( cFrame.left > g_asDesktops[nDesktop].m_cMaxWindowFrame.right - 32 )
				{
					cNewFrame.left = 5 + 12*(nMoveTally % 6) + g_asDesktops[nDesktop].m_cMaxWindowFrame.left;
					cNewFrame.right = cNewFrame.left + cFrame.Width();
					bMoved = true;
				}
				if( cFrame.top > g_asDesktops[nDesktop].m_cMaxWindowFrame.bottom - 32 )
				{
					cNewFrame.top = 50 + 12*(nMoveTally % 6) + g_asDesktops[nDesktop].m_cMaxWindowFrame.top;
					cNewFrame.bottom = cNewFrame.top + cFrame.Height();
					bMoved = true;
				}

				if( bMoved )
				{
					nMoveTally++;
					if( pcWindow->GetWndBorder() )
					{
						cFrame = pcWindow->GetWndBorder()->AlignFrame( cNewFrame );
					}
					pcWindow->m_asDTState[nDesktop].m_cFrame = cNewFrame;
					
					pcWindow->SetFrame( cNewFrame );
					pcTopWid->Invalidate( true );  /* Window needs to be redrawn */
					Messenger *pcTarget = pcWindow->GetAppTarget();

					if( pcTarget != NULL )
					{
						Message cMsg( M_WINDOW_FRAME_CHANGED );

						cMsg.AddRect( "_new_frame", cNewFrame );
						if( pcTarget->SendMessage( &cMsg ) < 0 )
						{
							dbprintf( "Error: set_desktop_screenmode() failed to send M_WINDOW_FRAME_CHANGED to %s\n", pcWindow->GetTitle() );
						}
					}
				}
			}
			pcWindow->ScreenModeChanged( IPoint( g_pcScreenBitmap->m_nWidth, g_pcScreenBitmap->m_nHeight ), g_pcScreenBitmap->m_eColorSpc );
		}

		g_pcTopView->ScreenModeChanged();
		g_pcTopView->Invalidate( true );
		g_pcTopView->SetDirtyRegFlags();
		g_pcTopView->UpdateRegions();
	}
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void add_window_to_desktop( SrvWindow * pcWindow )
{
	uint32 nMask = pcWindow->GetDesktopMask();

	for( int i = 0; i < 32; ++i )
	{
		if( nMask & ( 1 << i ) )
		{
			pcWindow->m_asDTState[i].m_pcNextWindow = g_asDesktops[i].m_pcFirstWindow;
			g_asDesktops[i].m_pcFirstWindow = pcWindow;

			if( pcWindow->GetFlags() & WND_SYSTEM )
			{
				pcWindow->m_asDTState[i].m_pcNextSystemWindow = g_asDesktops[i].m_pcFirstSystemWindow;
				g_asDesktops[i].m_pcFirstSystemWindow = pcWindow;
			}

			if( i == g_nActiveDesktop )
			{
				g_pcTopView->AddChild( pcWindow->GetTopView(), true );
			}
		}
	}


}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void remove_window_from_desktop( SrvWindow * pcWindow )
{
	uint32 nMask = pcWindow->GetDesktopMask();

	for( int i = 0; i < 32; ++i )
	{
		if( nMask & ( 1 << i ) )
		{
			SrvWindow **ppcTmp;
			bool bFound = false;

			for( ppcTmp = &g_asDesktops[i].m_pcFirstWindow; *ppcTmp != NULL; ppcTmp = &( *ppcTmp )->m_asDTState[i].m_pcNextWindow )
			{
				if( *ppcTmp == pcWindow )
				{
					*ppcTmp = pcWindow->m_asDTState[i].m_pcNextWindow;
					pcWindow->m_asDTState[i].m_pcNextWindow = NULL;
					bFound = true;
					break;
				}
			}
			if( pcWindow->GetFlags() & WND_SYSTEM )
			{
				for( ppcTmp = &g_asDesktops[i].m_pcFirstSystemWindow; *ppcTmp != NULL; ppcTmp = &( *ppcTmp )->m_asDTState[i].m_pcNextSystemWindow )
				{
					if( *ppcTmp == pcWindow )
					{
						*ppcTmp = pcWindow->m_asDTState[i].m_pcNextSystemWindow;
						pcWindow->m_asDTState[i].m_pcNextSystemWindow = NULL;
						break;
					}
				}
			}
			for( int j = 0; j < FOCUS_STACK_SIZE; ++j )
			{
				if( g_asDesktops[i].m_apcFocusStack[j] == pcWindow )
				{
					for( int k = j; k < FOCUS_STACK_SIZE - 1; ++k )
					{
						g_asDesktops[i].m_apcFocusStack[k] = g_asDesktops[i].m_apcFocusStack[k + 1];
					}
					g_asDesktops[i].m_apcFocusStack[FOCUS_STACK_SIZE - 1] = NULL;
					j = j - 1;
				}
			}
			__assertw( bFound );
			if( i == g_nActiveDesktop )
			{
				pcWindow->GetTopView()->RemoveThis(  );
			}
			else
			{
				if( pcWindow == g_asDesktops[i].m_pcActiveWindow )
				{
					g_asDesktops[i].m_pcActiveWindow = NULL;
				}
			}
		}
	}
	if( pcWindow == get_active_window( true ) )
	{
		set_active_window( NULL, false );
	}
	
	if( pcWindow == get_active_window( true ) )
	{
		dbprintf( "Deleted window still active %i %i\n", g_nActiveDesktop, (int)nMask );
		for( int j = 0; j < FOCUS_STACK_SIZE; ++j )
		{
			dbprintf( "%x\n", (uint)g_asDesktops[g_nActiveDesktop].m_apcFocusStack[j] );
		}
	}


}

//----------------------------------------------------------------------------
// NAME: remove_from_focusstack
// DESC: Removes a window from the focus history, called when window is hidden.
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void remove_from_focusstack( SrvWindow * pcWindow )
{
	uint32 nMask = pcWindow->GetDesktopMask();

	for( int i = 0; i < 32; ++i )
	{
		if( nMask & ( 1 << i ) )
		{
			for( int j = 0; j < FOCUS_STACK_SIZE; ++j )
			{
				if( g_asDesktops[i].m_apcFocusStack[j] == pcWindow )
				{
					for( int k = j; k < FOCUS_STACK_SIZE - 1; ++k )
					{
						g_asDesktops[i].m_apcFocusStack[k] = g_asDesktops[i].m_apcFocusStack[k + 1];
					}
					g_asDesktops[i].m_apcFocusStack[FOCUS_STACK_SIZE - 1] = NULL;
					j = j - 1;
				}
			}
		}
	}

	if( pcWindow == get_active_window( true ) )
	{
		set_active_window( NULL, false );
	}

}

//----------------------------------------------------------------------------
// NAME: set_desktop
// DESC: Activates the specified desktop. nDesktop: number of desktop to activate; bNotifyActiveWnd: if false, do not activate any windows on new desktop - this is used when moving a window between desktops
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void set_desktop( int nDesktop, bool bNotifyActiveWnd )
{
	Layer *pcLayer;
	SrvWindow *pcPrev = NULL;

	if( nDesktop == g_nActiveDesktop )
	{
		return;
	}


//    SrvSprite::Hide();
	std::set < Layer * >cViews;

	if( g_nActiveDesktop != -1 )
	{
		g_nPrevDesktop = g_nActiveDesktop;

		while( ( pcLayer = g_pcTopView->GetTopChild() ) )
		{
			SrvWindow *pcWindow = pcLayer->GetWindow();
			__assertw( pcWindow != NULL );

			Rect cFrame = pcWindow->GetFrame();
			
			pcWindow->DesktopActivated( g_nPrevDesktop, false, IPoint( g_pcScreenBitmap->m_nWidth, g_pcScreenBitmap->m_nHeight ), g_pcScreenBitmap->m_eColorSpc );

			/* Save the old desktop's window list */
			if( pcPrev == NULL )
			{
				g_asDesktops[g_nActiveDesktop].m_pcFirstWindow = pcWindow;
			}
			else
			{
				pcPrev->m_asDTState[g_nActiveDesktop].m_pcNextWindow = pcWindow;
			}
			pcWindow->m_asDTState[g_nActiveDesktop].m_pcNextWindow = NULL;
			
			/* Save each window's position  */
			if( pcWindow->GetFlags() & WND_INDEP_DESKTOP_FRAMES )
			{ /* save frame position only for this desktop */
				pcWindow->m_asDTState[g_nActiveDesktop].m_cFrame = cFrame;
			}
			else
			{  /* save frame position to all desktops */
				for( int i = 0; i < 32; i++ )
				{
					pcWindow->m_asDTState[i].m_cFrame = cFrame;
				}
			}

			pcPrev = pcWindow;
			g_pcTopView->RemoveChild( pcLayer );

			cViews.insert( pcLayer );
		}
	}
	/* deactivate active window of old desktop */
	if( bNotifyActiveWnd && g_asDesktops[g_nActiveDesktop].m_pcActiveWindow != NULL ) { g_asDesktops[g_nActiveDesktop].m_pcActiveWindow->WindowActivated( false ); }
	
	bool bScreenModeChanged = setup_screenmode( nDesktop, false );
	/* Has screen resolution decreased? */
	bool bResolutionDecreased = ( (g_asDesktops[nDesktop].m_sScreenMode.m_nWidth < g_asDesktops[g_nActiveDesktop].m_sScreenMode.m_nWidth) || (g_asDesktops[nDesktop].m_sScreenMode.m_nHeight < g_asDesktops[g_nActiveDesktop].m_sScreenMode.m_nHeight) );
	
	g_nActiveDesktop = nDesktop;

	SrvWindow *pcWindow;

	int nMoveTally = 0;
	for( pcWindow = g_asDesktops[nDesktop].m_pcFirstWindow; pcWindow != NULL; pcWindow = pcWindow->m_asDTState[nDesktop].m_pcNextWindow )
	{
		Layer *pcTopWid = pcWindow->GetTopView();
		Rect cFrame = pcWindow->m_asDTState[nDesktop].m_cFrame;
		Rect cNewFrame = cFrame;

		g_pcTopView->AddChild( pcTopWid, false );
		/* Make sure all windows are unfocused */
		if( pcWindow->GetDecorator() ) pcWindow->GetDecorator()->SetFocusState( false );  /* GetDecorator() could be NULL for bitmap windows? */

		cViews.erase( pcTopWid );

		/* If changing to desktop with smaller resolution, make sure no windows are offscreen */
		/* The following code is re-used in set_desktop_screenmode() */
		if( bResolutionDecreased )
		{
			bool bMoved = false;
            /* move window only if it is almost completely offscreen (left/top edge is within 32 pixels of screen edge). never resize */
			if( cFrame.left > g_asDesktops[nDesktop].m_cMaxWindowFrame.right - 32 )
			{
				cNewFrame.left = 5 + 12*(nMoveTally % 6) + g_asDesktops[nDesktop].m_cMaxWindowFrame.left;
				cNewFrame.right = cNewFrame.left + cFrame.Width();
				bMoved = true;
			}
			if( cFrame.top > g_asDesktops[nDesktop].m_cMaxWindowFrame.bottom - 32 )
			{
				cNewFrame.top = 50 + 12*(nMoveTally % 6) + g_asDesktops[nDesktop].m_cMaxWindowFrame.top;
				cNewFrame.bottom = cNewFrame.top + cFrame.Height();
				bMoved = true;
			}

			if( bMoved )
			{
				if( pcWindow->GetWndBorder() )
				{
					cNewFrame = pcWindow->GetWndBorder()->AlignFrame( cNewFrame );
				}
				pcWindow->m_asDTState[nDesktop].m_cFrame = cNewFrame;
				nMoveTally++;
			}
		}
		/* TODO: compensate for window borders when calculating new positions, using pcWindow->GetDecorator()->GetBorderSize() [see wndborder.cpp line 655] */

		if( pcWindow->GetFrame() != pcWindow->m_asDTState[nDesktop].m_cFrame )
		{
			pcWindow->SetFrame( pcWindow->m_asDTState[nDesktop].m_cFrame );
			pcTopWid->Invalidate( true );  /* Window needs to be redrawn */
			Messenger *pcTarget = pcWindow->GetAppTarget();

			if( pcTarget != NULL )
			{
				Message cMsg( M_WINDOW_FRAME_CHANGED );

				cMsg.AddRect( "_new_frame", pcWindow->m_asDTState[nDesktop].m_cFrame );
				if( pcTarget->SendMessage( &cMsg ) < 0 )
				{
					dbprintf( "Error: set_desktop() failed to send M_WINDOW_FRAME_CHANGED to %s\n", pcWindow->GetTitle() );
				}
			}
		}
		pcWindow->DesktopActivated( nDesktop, true, IPoint( g_pcScreenBitmap->m_nWidth, g_pcScreenBitmap->m_nHeight ), g_pcScreenBitmap->m_eColorSpc );
		if( bScreenModeChanged )
		{
			pcWindow->ScreenModeChanged( IPoint( g_pcScreenBitmap->m_nWidth, g_pcScreenBitmap->m_nHeight ), g_pcScreenBitmap->m_eColorSpc );
		}
	}
	/* Activate the new desktop's active window */
	if( bNotifyActiveWnd ) {
		if( g_asDesktops[nDesktop].m_pcActiveWindow != NULL ) {
			g_asDesktops[nDesktop].m_pcActiveWindow->WindowActivated( true );
		} else {  /* no previously active window; activate the desktop */
			for( pcWindow = g_asDesktops[nDesktop].m_pcFirstWindow; pcWindow != NULL; pcWindow = pcWindow->m_asDTState[nDesktop].m_pcNextWindow )
			{
				if( pcWindow != NULL && pcWindow->GetApp() != NULL && pcWindow->GetApp()->GetName() == "application/syllable-Desktop" ) break;
			}
			if( pcWindow != NULL ) pcWindow->MakeFocus( true );
		}
	}
	
	if( bScreenModeChanged )
	{
		g_pcTopView->Invalidate( true );
	}

	std::set < Layer * >::iterator i;
	for( i = cViews.begin(); i != cViews.end(  ); ++i )
	{
		( *i )->DeleteRegions();
	}
	SrvWindow::HandleMouseTransaction();
//    SrvSprite::Unhide();
}

int get_active_desktop()
{
	return ( g_nActiveDesktop );
}

int get_prev_desktop()
{
	return ( g_nPrevDesktop );
}

os::Rect get_desktop_max_window_frame( int nDesktop )
{
	if( nDesktop == os::Desktop::ACTIVE_DESKTOP )
		nDesktop = g_nActiveDesktop;
	__assertw( nDesktop >= 0 && nDesktop < 32 );
	return ( g_asDesktops[nDesktop].m_cMaxWindowFrame );
}

void set_desktop_max_window_frame( int nDesktop, os::Rect cFrame )
{
	if( nDesktop == os::Desktop::ACTIVE_DESKTOP )
		nDesktop = g_nActiveDesktop;
	__assertw( nDesktop >= 0 && nDesktop < 32 );
	g_asDesktops[nDesktop].m_cMaxWindowFrame = cFrame;
}

bool init_desktops()
{
	g_pcScreenBitmap = new SrvBitmap( 0, 0, g_asDesktops[0].m_sScreenMode.m_eColorSpace, NULL, 0 );
	g_pcScreenBitmap->m_bVideoMem = true;
	g_pcScreenBitmap->m_nFlags = Bitmap::NO_ALPHA_CHANNEL;

	dbprintf( "Initialise graphics driver...\n" );

	/* Iterate through the graphics driver nodes in /dev/graphics */
	DIR *pKernelDriverDir = NULL;

	pKernelDriverDir = opendir( "/dev/graphics" );

	if( pKernelDriverDir != NULL )
	{
		struct dirent *psEntry;

		while( ( psEntry = readdir( pKernelDriverDir ) ) != NULL )
		{
			if( strcmp( psEntry->d_name, "." ) == 0 || strcmp( psEntry->d_name, ".." ) == 0 )
			{
				continue;
			}

			/* Build complete path */
			char zDevPath[PATH_MAX] = "/dev/graphics/";

			strcat( zDevPath, psEntry->d_name );

			/* Try to open the node */
			int nFd = open( zDevPath, O_RDWR );

			if( nFd < 0 )
			{
				dbprintf( "Error: Failed to open graphics driver node %s\n", zDevPath );
				continue;
			}

			/* Get appserver driver */
			char zAppserverDriverPath[PATH_MAX] = "/system/drivers/appserver/video/";
			char zDriverPath[PATH_MAX];

			memset( zDriverPath, 0, PATH_MAX );

			if( ioctl( nFd, IOCTL_GET_APPSERVER_DRIVER, &zDriverPath[0] ) != 0 )
			{
				dbprintf( "Error: Failed to get the AppServer driver for %s\n", zDevPath );
			}
			strcat( zAppserverDriverPath, zDriverPath );


			dbprintf( "Using AppServer driver %s\n", zDriverPath );

			/* Open the appserver driver */
			int nLib = -1;

			nLib = load_library( zAppserverDriverPath, 0 );

			if( nLib < 0 )
			{
				dbprintf( "Error: Failed to load graphics driver %s\n", zAppserverDriverPath );
				close( nFd );
				continue;
			}
			gfxdrv_init_func *pInitFunc;

			/* Get graphics driver init symbol */
			int nError = get_symbol_address( nLib, "init_gfx_driver", -1, ( void ** )&pInitFunc );

			if( nError < 0 )
			{
				dbprintf( "Error: Graphics driver %s does not export entry point init_gfx_driver()\n", zDriverPath );
				unload_library( nLib );
				close( nFd );
				continue;
			}
			/* Try to initialize the driver */
			try
			{
				g_pcDispDrv = pInitFunc( nFd );

				if( g_pcDispDrv != NULL )
				{
					g_nFrameBufferArea = g_pcDispDrv->Open();
					if( g_nFrameBufferArea >= 0 )
					{
						dbprintf( "Using graphics driver %s\n", zDriverPath );
						break;
					}
				}
			}
			catch( ... )
			{
			}
			unload_library( nLib );
		}
	}

	if( g_pcDispDrv == NULL )
	{
		dbprintf( "Falling back to VESA 2.0 driver\n" );
		g_pcDispDrv = new VesaDriver;
		g_nFrameBufferArea = g_pcDispDrv->Open();
		if( g_nFrameBufferArea < 0 )
		{
			dbprintf( "Failed to initialise VESA 2.0 driver.\n" );
		}
	}

	AreaInfo_s sFBAreaInfo;
	get_area_info( g_nFrameBufferArea, &sFBAreaInfo );

	g_pcScreenBitmap->m_pcDriver = g_pcDispDrv;
	g_pcScreenBitmap->m_hArea = g_nFrameBufferArea;
	g_pcScreenBitmap->m_pRaster = ( uint8 * )sFBAreaInfo.pAddress;

	setup_screenmode( 0, false );

	g_nActiveDesktop = 0;
	g_pcTopView = new TopLayer( g_pcScreenBitmap );

	g_pcDispDrv->SetCursorBitmap( MPTR_RGB32, IPoint( 0, 0 ), g_anMouseImg, POINTER_WIDTH, POINTER_HEIGHT );
	g_pcDispDrv->MouseOn();
	return ( 0 );
}
