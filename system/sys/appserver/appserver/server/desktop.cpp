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

#include <gui/desktop.h>
#include <gui/window.h>

#include <util/message.h>
#include <util/messenger.h>

#include "swindow.h"
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

#define POINTER_WIDTH 13
#define POINTER_HEIGHT 13
static uint8 g_anMouseImg[]=
{
    0x02,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x02,0x03,0x02,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x02,0x03,0x03,0x02,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x02,0x03,0x03,0x03,0x03,0x02,0x02,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x02,0x03,0x03,0x03,0x03,0x03,0x02,0x02,0x00,0x00,0x00,
    0x00,0x00,0x02,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x02,0x02,0x00,
    0x00,0x00,0x00,0x02,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x02,0x00,
    0x00,0x00,0x00,0x02,0x03,0x03,0x03,0x03,0x03,0x03,0x02,0x00,0x00,
    0x00,0x00,0x00,0x00,0x02,0x03,0x03,0x03,0x03,0x02,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x02,0x03,0x03,0x03,0x02,0x03,0x02,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x02,0x03,0x02,0x00,0x02,0x03,0x02,0x00,
    0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x00,0x00,0x00,0x02,0x03,0x02,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,
};

SrvBitmap*	g_pcScreenBitmap = NULL;
static int	g_nFrameBufferArea = -1;

static int g_nActiveDesktop = -1;
static int g_nPrevDesktop = 1;

static AreaInfo_s g_sFBAreaInfo;

#define FOCUS_STACK_SIZE  32

static struct Desktop
{
    SrvWindow* 	m_pcFirstWindow;
    SrvWindow*  m_pcFirstSystemWindow;
    SrvWindow* 	m_pcActiveWindow;
    SrvWindow*  m_apcFocusStack[FOCUS_STACK_SIZE];
    screen_mode m_sScreenMode;
    std::string	m_cBackdropPath;
} g_asDesktops[32];


void set_desktop_config( int nDesktop, const screen_mode& sMode, const std::string& cBackDropPath )
{
    assert( nDesktop >= 0 && nDesktop < 32 );

    g_asDesktops[nDesktop].m_sScreenMode = sMode;
}


int  get_desktop_config( int* pnActiveDesktop, screen_mode* psMode, std::string* pcBackdropPath )
{
    if ( *pnActiveDesktop == os::Desktop::ACTIVE_DESKTOP ) {
	*pnActiveDesktop = g_nActiveDesktop;
    }
    if ( psMode != NULL ) {
	*psMode = g_asDesktops[*pnActiveDesktop].m_sScreenMode;
    }
    if ( pcBackdropPath != NULL ) {
	*pcBackdropPath = g_asDesktops[*pnActiveDesktop].m_cBackdropPath;
    }
    return( 0 );
}

SrvWindow* get_active_window( bool bIgnoreSystemWindows )
{
    if ( bIgnoreSystemWindows == false && g_asDesktops[g_nActiveDesktop].m_pcFirstSystemWindow != NULL ) {
	return( g_asDesktops[g_nActiveDesktop].m_pcFirstSystemWindow );
    } else {
	return( g_asDesktops[g_nActiveDesktop].m_pcActiveWindow );
    }
}

void set_active_window( SrvWindow* pcWindow, bool bNotifyPrevious )
{
    if ( pcWindow != NULL && (pcWindow->GetFlags() & WND_SYSTEM) ) {
	return;
    }
    if ( pcWindow == NULL && g_asDesktops[g_nActiveDesktop].m_pcFirstSystemWindow != NULL ) {
	return;
    }
    if ( pcWindow == g_asDesktops[g_nActiveDesktop].m_pcActiveWindow ) {
	return;
    }
    SrvWindow* pcPrevious = g_asDesktops[g_nActiveDesktop].m_pcActiveWindow;
  
    if ( pcWindow == NULL ) {
	g_asDesktops[g_nActiveDesktop].m_pcActiveWindow = g_asDesktops[g_nActiveDesktop].m_apcFocusStack[0];
	memmove( &g_asDesktops[g_nActiveDesktop].m_apcFocusStack[0],
		 &g_asDesktops[g_nActiveDesktop].m_apcFocusStack[1],
		 sizeof( SrvWindow* ) * (FOCUS_STACK_SIZE - 1) );
	g_asDesktops[g_nActiveDesktop].m_apcFocusStack[FOCUS_STACK_SIZE-1] = NULL;
    } else {
	memmove( &g_asDesktops[g_nActiveDesktop].m_apcFocusStack[1],
		 &g_asDesktops[g_nActiveDesktop].m_apcFocusStack[0],
		 sizeof( SrvWindow* ) * (FOCUS_STACK_SIZE - 1) );
	g_asDesktops[g_nActiveDesktop].m_apcFocusStack[0] = g_asDesktops[g_nActiveDesktop].m_pcActiveWindow;
	g_asDesktops[g_nActiveDesktop].m_pcActiveWindow = pcWindow;
    }
    if ( pcPrevious != g_asDesktops[g_nActiveDesktop].m_pcActiveWindow ) {
	if ( bNotifyPrevious && pcPrevious != NULL ) {
	    pcPrevious->WindowActivated( false );
	}
	if ( g_asDesktops[g_nActiveDesktop].m_pcActiveWindow != NULL ) {
	    g_asDesktops[g_nActiveDesktop].m_pcActiveWindow->WindowActivated( true );
	}
    }
}

class ModeCmp
{
public:
    bool operator () ( const ScreenMode& cMode1, const ScreenMode& cMode2 ) {
	if ( BitsPerPixel( cMode1.m_eColorSpace ) != BitsPerPixel( cMode2.m_eColorSpace ) ) {
	    return( BitsPerPixel( cMode1.m_eColorSpace ) < BitsPerPixel( cMode2.m_eColorSpace ) );
	} else {
	    return( cMode1.m_nWidth < cMode2.m_nWidth );
	}
    }
};

static bool setup_screenmode( int nDesktop, bool bForce )
{
    if ( bForce == false && nDesktop == g_nActiveDesktop ) {
	return( false );
    }
    if ( bForce == false && g_nActiveDesktop != -1 &&
	 g_asDesktops[nDesktop].m_sScreenMode.m_nWidth    == g_asDesktops[g_nActiveDesktop].m_sScreenMode.m_nWidth &&
	 g_asDesktops[nDesktop].m_sScreenMode.m_nHeight   == g_asDesktops[g_nActiveDesktop].m_sScreenMode.m_nHeight &&
	 g_asDesktops[nDesktop].m_sScreenMode.m_eColorSpace == g_asDesktops[g_nActiveDesktop].m_sScreenMode.m_eColorSpace )
    {
	return( false );
    }
    bool bModeSet = true;
    if ( g_pcDispDrv->SetScreenMode( g_asDesktops[nDesktop].m_sScreenMode.m_nWidth,
				     g_asDesktops[nDesktop].m_sScreenMode.m_nHeight,
				     g_asDesktops[nDesktop].m_sScreenMode.m_eColorSpace,
				     g_asDesktops[nDesktop].m_sScreenMode.m_vHPos,
				     g_asDesktops[nDesktop].m_sScreenMode.m_vVPos,
				     g_asDesktops[nDesktop].m_sScreenMode.m_vHSize,
				     g_asDesktops[nDesktop].m_sScreenMode.m_vVSize,
				     g_asDesktops[nDesktop].m_sScreenMode.m_vRefreshRate ) != 0 )
    {
	bModeSet = false;
	std::vector<ScreenMode> cModes;
	int nModeCount = g_pcDispDrv->GetScreenModeCount();
	for ( int i = 0 ; i < nModeCount ; ++i ) {
	    ScreenMode sMode;
	    if ( g_pcDispDrv->GetScreenModeDesc( i, &sMode ) == false ) {
		continue;
	    }
	    cModes.push_back( sMode );
	}
	if ( cModes.size() == 0 ) {
	    dbprintf( "Error: Display driver don't list any screenmodes. Check that your gfx-adapter supports Vesa2.0\n" );
	    dbprintf( "Terminate application server...\n" );
	    exit( 1 );
	}
	std::sort( cModes.begin(), cModes.end(), ModeCmp() );
	  // Find the mode closest to 640x480
	for ( uint i = 0 ; i < cModes.size() ; ++i ) {
	    if ( cModes[i].m_nWidth >= 640 || i == cModes.size() - 1 ) {
		if ( cModes[i].m_nWidth > 640 && i > 0 ) {
		    i--;
		}
		dbprintf( "Error: Failed to set screen mode %dx%d %dbpp on desktop %d. Revert to %dx%d %dbpp\n",
			  g_asDesktops[nDesktop].m_sScreenMode.m_nWidth,
			  g_asDesktops[nDesktop].m_sScreenMode.m_nHeight,
			  BitsPerPixel( g_asDesktops[nDesktop].m_sScreenMode.m_eColorSpace ), nDesktop,
			  cModes[i].m_nWidth, cModes[i].m_nHeight, BitsPerPixel( cModes[i].m_eColorSpace ) );

		g_asDesktops[nDesktop].m_sScreenMode.m_nWidth  	   = cModes[i].m_nWidth;
		g_asDesktops[nDesktop].m_sScreenMode.m_nHeight	   = cModes[i].m_nHeight;
		g_asDesktops[nDesktop].m_sScreenMode.m_eColorSpace = cModes[i].m_eColorSpace;

		  // If this fails we try all the screen-modes before giving up.
		if ( g_pcDispDrv->SetScreenMode( cModes[i].m_nWidth, cModes[i].m_nHeight, cModes[i].m_eColorSpace, 80, 50, 70, 80, 60.0f ) == 0 ) {
		    bModeSet = true;
		    break;
		}
		for ( i = 0 ; i < cModes.size() ; ++i ) {
		    dbprintf( "Error: Failed to set screen mode %dx%d %dbpp on desktop %d. Revert to %dx%d %dbpp\n",
			      g_asDesktops[nDesktop].m_sScreenMode.m_nWidth,
			      g_asDesktops[nDesktop].m_sScreenMode.m_nHeight,
			      BitsPerPixel( g_asDesktops[nDesktop].m_sScreenMode.m_eColorSpace ), nDesktop,
			      cModes[i].m_nWidth, cModes[i].m_nHeight, BitsPerPixel( cModes[i].m_eColorSpace ) );

		    g_asDesktops[nDesktop].m_sScreenMode.m_nWidth      = cModes[i].m_nWidth;
		    g_asDesktops[nDesktop].m_sScreenMode.m_nHeight     = cModes[i].m_nHeight;
		    g_asDesktops[nDesktop].m_sScreenMode.m_eColorSpace = cModes[i].m_eColorSpace;
		    if ( g_pcDispDrv->SetScreenMode( cModes[i].m_nWidth, cModes[i].m_nHeight, cModes[i].m_eColorSpace,
						     80, 50, 70, 80, 60.0f ) == 0 ) {
			bModeSet = true;
			break;
		    }
		}
		
		break;
	    }
	}
    }
    if ( bModeSet == false ) {
	dbprintf( "Error: All screenmodes failed. Game over!\n" );
	dbprintf( "Terminate application server...\n" );
	exit( 1 );
    }
    g_pcScreenBitmap->m_nWidth	    = g_pcDispDrv->GetHorizontalRes();
    g_pcScreenBitmap->m_nHeight	    = g_pcDispDrv->GetVerticalRes();
    g_pcScreenBitmap->m_nBytesPerLine = g_pcDispDrv->GetBytesPerLine();
    g_pcScreenBitmap->m_eColorSpc     = g_pcDispDrv->GetColorSpace();
    g_pcScreenBitmap->m_pRaster	      = ((uint8*)g_sFBAreaInfo.pAddress) + g_pcDispDrv->GetFramebufferOffset();
	
    if ( g_nActiveDesktop != -1 ) {
	g_pcTopView->SetFrame( Rect( 0, 0, g_pcScreenBitmap->m_nWidth - 1, g_pcScreenBitmap->m_nHeight - 1 ) );
    }

    if ( g_asDesktops[nDesktop].m_sScreenMode.m_eColorSpace == g_asDesktops[g_nActiveDesktop].m_sScreenMode.m_eColorSpace ) {
	SrvSprite::ColorSpaceChanged();
    }
    if ( g_nActiveDesktop != -1 ) {
	Point cMousePos = InputNode::GetMousePos();
	cMousePos.x = floor( cMousePos.x * g_asDesktops[nDesktop].m_sScreenMode.m_nWidth / g_asDesktops[g_nActiveDesktop].m_sScreenMode.m_nWidth );
	cMousePos.y = floor( cMousePos.y * g_asDesktops[nDesktop].m_sScreenMode.m_nHeight / g_asDesktops[g_nActiveDesktop].m_sScreenMode.m_nHeight );
	InputNode::SetMousePos( cMousePos );
    }
    return( true );
}

void set_desktop_screenmode( int nDesktop, const screen_mode& sMode )
{
    assert( nDesktop >= 0 && nDesktop < 32 );

    g_asDesktops[nDesktop].m_sScreenMode = sMode;

    if ( nDesktop == g_nActiveDesktop ) {
	setup_screenmode( nDesktop, true );
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

void add_window_to_desktop( SrvWindow* pcWindow )
{
    uint32 nMask = pcWindow->GetDesktopMask();
  
    for ( int i = 0 ; i < 32 ; ++i ) {
	if ( nMask & (1 << i) ) {
	    pcWindow->m_asDTState[i].m_pcNextWindow = g_asDesktops[i].m_pcFirstWindow;
	    g_asDesktops[i].m_pcFirstWindow = pcWindow;

	    if ( pcWindow->GetFlags() & WND_SYSTEM ) {
		pcWindow->m_asDTState[i].m_pcNextSystemWindow = g_asDesktops[i].m_pcFirstSystemWindow;
		g_asDesktops[i].m_pcFirstSystemWindow = pcWindow;
	    }
	    
	    if ( i == g_nActiveDesktop ) {
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

void remove_window_from_desktop( SrvWindow* pcWindow )
{
    uint32 nMask = pcWindow->GetDesktopMask();
  
    for ( int i = 0 ; i < 32 ; ++i ) {
	if ( nMask & (1 << i) )	{
	    SrvWindow** ppcTmp;
	    bool bFound = false;
      
	    for ( ppcTmp = &g_asDesktops[i].m_pcFirstWindow ; *ppcTmp != NULL ; ppcTmp = &(*ppcTmp)->m_asDTState[i].m_pcNextWindow ) {
		if ( *ppcTmp == pcWindow ) {
		    *ppcTmp = pcWindow->m_asDTState[i].m_pcNextWindow;
		    pcWindow->m_asDTState[i].m_pcNextWindow = NULL;
		    bFound = true;
		    break;
		}
	    }
	    if ( pcWindow->GetFlags() & WND_SYSTEM ) {
		for ( ppcTmp = &g_asDesktops[i].m_pcFirstSystemWindow ; *ppcTmp != NULL ; ppcTmp = &(*ppcTmp)->m_asDTState[i].m_pcNextSystemWindow ) {
		    if ( *ppcTmp == pcWindow ) {
			*ppcTmp = pcWindow->m_asDTState[i].m_pcNextSystemWindow;
			pcWindow->m_asDTState[i].m_pcNextSystemWindow = NULL;
			break;
		    }
		}
	    }
	    for ( int j = 0 ; j < FOCUS_STACK_SIZE ; ++j ) {
		if ( g_asDesktops[i].m_apcFocusStack[j] == pcWindow ) {
		    for ( int k = j ; k < FOCUS_STACK_SIZE - 1 ; ++k ) {
			g_asDesktops[i].m_apcFocusStack[k] = g_asDesktops[i].m_apcFocusStack[k+1];
		    }
		    g_asDesktops[i].m_apcFocusStack[FOCUS_STACK_SIZE-1] = NULL;
		}
	    }
	    __assertw( bFound );
	    if ( i == g_nActiveDesktop ) {
		pcWindow->GetTopView()->RemoveThis();
	    } else {
		if ( pcWindow == g_asDesktops[i].m_pcActiveWindow ) {
		    g_asDesktops[i].m_pcActiveWindow = NULL;
		}
	    }
	}
    }
    if ( pcWindow == get_active_window(true) ) {
	set_active_window( NULL, false );
    }
  
}

//----------------------------------------------------------------------------
// NAME: remove_from_focusstack
// DESC: Removes a window from the focus history, called when window is hidden.
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void remove_from_focusstack( SrvWindow* pcWindow )
{
    uint32 nMask = pcWindow->GetDesktopMask();

    for ( int i = 0 ; i < 32 ; ++i ) {
	if ( nMask & (1 << i) )	{
	    for ( int j = 0 ; j < FOCUS_STACK_SIZE ; ++j ) {
		if ( g_asDesktops[i].m_apcFocusStack[j] == pcWindow ) {
		    for ( int k = j ; k < FOCUS_STACK_SIZE - 1 ; ++k ) {
			g_asDesktops[i].m_apcFocusStack[k] = g_asDesktops[i].m_apcFocusStack[k+1];
		    }
		    g_asDesktops[i].m_apcFocusStack[FOCUS_STACK_SIZE-1] = NULL;
		}
	    }
	}
    }

    if ( pcWindow == get_active_window(true) ) {
	set_active_window( NULL, false );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void set_desktop( int nNum )
{
    Layer*     pcLayer;
    SrvWindow* pcPrev = NULL;

    if ( nNum == g_nActiveDesktop ) {
	return;
    }
  
//    SrvSprite::Hide();
    std::set<Layer*> cViews;
  
    if ( g_nActiveDesktop != -1 ) {
	g_nPrevDesktop = g_nActiveDesktop;

	while( (pcLayer = g_pcTopView->GetTopChild()) )
	{
	    SrvWindow* pcWindow = pcLayer->GetWindow();
	    __assertw( pcWindow != NULL );

	    if ( pcPrev == NULL ) {
		g_asDesktops[g_nActiveDesktop].m_pcFirstWindow = pcWindow;
	    } else {
		pcPrev->m_asDTState[g_nActiveDesktop].m_pcNextWindow = pcWindow;
	    }
	    pcWindow->m_asDTState[g_nActiveDesktop].m_pcNextWindow = NULL;
	    pcWindow->m_asDTState[g_nActiveDesktop].m_cFrame = pcWindow->GetFrame();

	    pcPrev = pcWindow;
	    g_pcTopView->RemoveChild( pcLayer );

	    cViews.insert( pcLayer );
	}
    }
    bool bScreenModeChanged = setup_screenmode( nNum, false );
    g_nActiveDesktop = nNum;

    SrvWindow* pcWindow;
  
    for ( pcWindow = g_asDesktops[nNum].m_pcFirstWindow ;
	  pcWindow != NULL ;
	  pcWindow = pcWindow->m_asDTState[nNum].m_pcNextWindow )
    {
	Layer* pcTopWid = pcWindow->GetTopView();
	g_pcTopView->AddChild( pcTopWid, false );
    
	cViews.erase( pcTopWid );
	if ( pcWindow->GetFrame() != pcWindow->m_asDTState[nNum].m_cFrame ) {
	    pcWindow->SetFrame( pcWindow->m_asDTState[nNum].m_cFrame );
	    Messenger* pcTarget = pcWindow->GetAppTarget();

	    if ( pcTarget != NULL ) {
		Message cMsg( M_WINDOW_FRAME_CHANGED );
		cMsg.AddRect( "_new_frame", pcWindow->m_asDTState[nNum].m_cFrame );
		if ( pcTarget->SendMessage( &cMsg ) < 0 ) {
		    dbprintf( "Error: set_desktop() failed to send M_WINDOW_FRAME_CHANGED to %s\n", pcWindow->GetTitle() );
		}
	    }
	}
	pcWindow->DesktopActivated( nNum, IPoint( g_pcScreenBitmap->m_nWidth, g_pcScreenBitmap->m_nHeight ),
				    g_pcScreenBitmap->m_eColorSpc );
	if ( bScreenModeChanged ) {
	    pcWindow->ScreenModeChanged( IPoint( g_pcScreenBitmap->m_nWidth, g_pcScreenBitmap->m_nHeight ),
					 g_pcScreenBitmap->m_eColorSpc );
	}
    }
    if ( bScreenModeChanged ) {
	g_pcTopView->Invalidate( true );
    }
    std::set<Layer*>::iterator i;
    for ( i = cViews.begin() ; i != cViews.end() ; ++i ) {
	(*i)->DeleteRegions();
    }
    SrvWindow::HandleMouseTransaction();
//    SrvSprite::Unhide();
}

bool toggle_desktops()
{
    if ( g_nPrevDesktop != g_nActiveDesktop ) {
	set_desktop( g_nPrevDesktop );
	return( true );
    } else {
	return( false );
    }
}

int get_active_desktop()
{
    return( g_nActiveDesktop );
}

bool init_desktops()
{
    g_pcScreenBitmap = new SrvBitmap( 0, 0, g_asDesktops[0].m_sScreenMode.m_eColorSpace, NULL, 0 );
    g_pcScreenBitmap->m_bVideoMem	= true;


    dbprintf( "Initialize display driver\n" );

    char zVideoDrvPath[1024] = "/system/drivers/appserver/video/";
    int  nPathLen = strlen( zVideoDrvPath );

    g_pcDispDrv = NULL;
    DIR*	pDir = opendir( zVideoDrvPath );

    if ( pDir != NULL )
    {
	struct dirent* psEntry;

	while( (psEntry = readdir( pDir )) != NULL ) {
	    if ( strcmp( psEntry->d_name, "." ) == 0 || strcmp( psEntry->d_name, ".." ) == 0 ) {
		continue;
	    }
	    dbprintf( "Probe graphices driver %s\n", psEntry->d_name );
	    zVideoDrvPath[nPathLen] = '\0';
	    strcat( zVideoDrvPath, psEntry->d_name );
	    int nLib = load_library( zVideoDrvPath, 0 );
	    if ( nLib < 0 ) {
		dbprintf( "failed to load video driver %s\n", zVideoDrvPath );
		continue;
	    }
	    gfxdrv_init_func* pInitFunc;
      
	    int nError = get_symbol_address( nLib, "init_gfx_driver", -1, (void**) &pInitFunc );
	    if ( nError < 0 ) {
		dbprintf( "Video driver %s does not export entry point init_gfx_driver()\n", psEntry->d_name );
		unload_library( nLib );
		continue;
	    }
      
	    try {
	       g_pcDispDrv = pInitFunc();
	   
	    if ( g_pcDispDrv != NULL ) {
		g_nFrameBufferArea = g_pcDispDrv->Open();
		if ( g_nFrameBufferArea >= 0 ) {
		    dbprintf( "Using video driver %s\n", psEntry->d_name );
		    break;
		}
	    }
	    } catch(...) {
	    }
	    unload_library( nLib );
	}
    } else {
	dbprintf( "Error: failed to open gfx driver directory\n" );
    }

    if ( g_pcDispDrv == NULL ) {
	dbprintf( "Fall back to vesa20 driver\n" );
	g_pcDispDrv = new VesaDriver;
	g_nFrameBufferArea = g_pcDispDrv->Open();
	if ( g_nFrameBufferArea < 0 ) {
	    dbprintf( "Failed to init vesa20 driver.\n" );
	}
    }

    get_area_info( g_nFrameBufferArea, &g_sFBAreaInfo );


    g_pcScreenBitmap->m_pcDriver = g_pcDispDrv;
    g_pcScreenBitmap->m_hArea	 = g_nFrameBufferArea;
    g_pcScreenBitmap->m_pRaster	 = (uint8*)g_sFBAreaInfo.pAddress;
  
    setup_screenmode( 0, false );

    g_nActiveDesktop = 0;
    g_pcTopView = new Layer( g_pcScreenBitmap );

    g_pcDispDrv->SetCursorBitmap( MPTR_MONO, IPoint(0,0), g_anMouseImg, POINTER_WIDTH, POINTER_HEIGHT );
    g_pcDispDrv->MouseOn();
    return( 0 );
}
