/*  The Syllable Application Server (AppServer)
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 - 2004 The Syllable Team
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */

#ifndef	DEVICES_DISPLAY_PACKETS_H
#define	DEVICES_DISPLAY_PACKETS_H

#include <string.h>
#include <atheos/types.h>

#include <gui/gfxtypes.h>
#include <gui/font.h>
#include <gui/region.h>
#include <gui/guidefines.h>

//#include  <atheos/mouse.h>

class SrvWindow;

namespace os
{
#if 0
}	// Fool Emacs auto-indent
#endif 

enum
{
    DR_CREATE_APP = 1,
    DR_CREATE_CLIPBOARD,
    DR_GET_CLIPBOARD_DATA,
    DR_SET_CLIPBOARD_DATA,
    DR_SET_WINDOW_DECORATOR,
    DR_SET_COLOR_CONFIG,
    DR_CREATE_APP_NEW,
    DR_GET_KEYBOARD_CFG,
    DR_SET_KEYBOARD_CFG,
    DR_SET_APPSERVER_CONFIG,
    DR_GET_APPSERVER_CONFIG,
    DR_GET_DEFAULT_FONT_NAMES,
    DR_GET_DEFAULT_FONT,
    DR_SET_DEFAULT_FONT,
    DR_ADD_DEFAULT_FONT,
    DR_RESCAN_FONTS,
    DR_SET_DESKTOP,
    _DR_unused_1,
    _DR_unused_2,
    DR_CLOSE_WINDOWS,
    _DR_unused_3,    
    DR_SET_DESKTOP_MAX_WINFRAME,
    DR_GET_DESKTOP_MAX_WINFRAME,
    DR_MINIMIZE_ALL,
    DR_GET_MOUSE_CFG,
    DR_SET_MOUSE_CFG,
	DR_REGISTER_KEY_EVNT,
	DR_UNREGISTER_KEY_EVNT,
	DR_REGISTERED_KEY_EVNTS,
	
    AR_OPEN_WINDOW = 10000,
    AR_CLOSE_WINDOW,
    AR_DELETE_WINDOW,
    AR_OPEN_BITMAP_WINDOW,
  
    AR_CREATE_BITMAP,
    AR_DELETE_BITMAP,
    AR_GET_QUALIFIERS,
    AR_CREATE_FONT,
    AR_DELETE_FONT,
    AR_SET_FONT_FAMILY_AND_STYLE,
    AR_GET_FONT_FAMILY_AND_STYLE,
    AR_SET_FONT_PROPERTIES,
    AR_GET_STRING_WIDTHS,
    AR_GET_STRING_LENGTHS,

    AR_CREATE_SPRITE,
    AR_DELETE_SPRITE,
    AR_MOVE_SPRITE,

    AR_RESCAN_FONTS,
    AR_GET_FONT_FAMILY_COUNT,
    AR_GET_FONT_FAMILY,
    AR_GET_FONT_STYLE_COUNT,
    AR_GET_FONT_STYLE,
    AR_GET_SCREENMODE_COUNT,
    AR_GET_SCREENMODE_INFO,
    AR_LOCK_DESKTOP,
    AR_UNLOCK_DESKTOP,
    AR_SET_SCREEN_MODE,
    AR_GET_IDLE_TIME,
    AR_GET_FONT_SIZES,

    AR_GET_TEXT_EXTENTS,
    AR_GET_FONT_CHARACTERS,
    
    AR_CLONE_BITMAP,
    
    WR_GET_VIEW_FRAME = 20000,
    WR_SET_SIZE_LIMITS,
    WR_TOGGLE_VIEW_DEPTH,
    WR_RENDER,
    WR_BEGIN_DRAG,
    WR_CREATE_VIEW,
    WR_DELETE_VIEW,
    WR_MAKE_FOCUS,
    WR_SET_ALIGNMENT,
    WR_WND_MOVE_REPLY,
    WR_SET_TITLE,
    _WR_unused_1,
    WR_GET_PEN_POSITION,
    WR_SET_FLAGS,
    WR_GET_MOUSE,
    WR_SET_MOUSE_POS,
    WR_PUSH_CURSOR,
    WR_POP_CURSOR,
    WR_CREATE_VIDEO_OVERLAY,
    WR_RECREATE_VIDEO_OVERLAY,
    WR_UPDATE_VIDEO_OVERLAY,
    WR_DELETE_VIDEO_OVERLAY,
    WR_SET_ICON,
    WR_ACTIVATE,
    WR_MINIMIZE,
    WR_PAINT_FINISHED,
    WR_LOCK_FB,
    
    EV_REGISTER = 30000,
    EV_UNREGISTER,
    EV_GET_INFO,
    EV_GET_LAST_EVENT_MESSAGE,
    EV_ADD_MONITOR,
    EV_REMOVE_MONITOR,
    EV_CALL,
    EV_GET_CHILDREN
};


enum
{
    DRC_PING,
    DRC_SET_FLAGS,
    _DRC_unused3,
    DRC_LINE32,
    _DRC_unused4,
    DRC_FILL_RECT32,
    DRC_COPY_RECT,
    _DRC_unused5,
    DRC_SET_COLOR32,
    DRC_SET_PEN_POS,
    DRC_SET_FONT,
    DRC_DRAW_STRING,
    DRC_DRAW_BITMAP,
    DRC_SET_FRAME,
    DRC_SCROLL_VIEW,
    DRC_BEGIN_UPDATE,
    DRC_END_UPDATE,
    DRC_SET_DRAWING_MODE,
    DRC_SHOW_VIEW,
    DRC_INVALIDATE_VIEW,
    DRC_INVALIDATE_RECT,
    DRC_SET_DRAW_REGION,
    DRC_SET_SHAPE_REGION,
    DRC_DRAW_TEXT,
    DRC_DRAW_SELECTED_TEXT,
    DRC_GET_SELECTION
};

typedef	struct
{
    uint32 nCmd;
    uint32 nSize;
    uint32 hViewToken;
} GRndHeader_s;

struct GRndSetFlags_s : GRndHeader_s
{
    uint32 nFlags;
};

struct GRndSetFrame_s : GRndHeader_s
{
    Rect cFrame;
};

struct GRndShowView_s : GRndHeader_s
{
    bool bVisible;
};

typedef	struct
{
    GRndHeader_s	sHdr;
    Point		sToPos;
} GRndLine32_s;

struct GRndFillRect32_s
{
    GRndHeader_s	sHdr;
    Rect		sRect;
    Color32_s		sColor;
};

struct GRndCopyRect_s
{
    GRndHeader_s	sHdr;
    Rect		cSrcRect;
    Rect		cDstRect;
};

typedef	struct
{
    GRndHeader_s	sHdr;
    int			hBitmapToken;
    Rect		cSrcRect;
    Rect		cDstRect;
} GRndDrawBitmap_s;

enum
{
    PEN_HIGH,
    PEN_LOW,
    PEN_ERASE
};

struct GRndSetDrawingMode_s : GRndHeader_s
{
    int nDrawingMode;
};

typedef	struct
{
    GRndHeader_s	sHdr;
    int			nWhichPen;
    Color32_s		sColor;
} GRndSetColor32_s;

typedef	struct
{
    GRndHeader_s	sHdr;
    bool		bRelative;
    Point		sPosition;
} GRndSetPenPos_s;

typedef	struct
{
    GRndHeader_s	sHdr;
    int			hFontID;
} GRndSetFont_s;

typedef	struct
{
    GRndHeader_s	sHdr;
    int		nLength;
    char		zString[1];	/* String of nLength characters, or null terminated if nLength == -1	*/
} GRndDrawString_s;

typedef	struct
{
    GRndHeader_s	sHdr;				/* Render header */
	Rect			cPos;
	uint32			nFlags;				/* Flags */
    int				nLength;			/* Length of the string, or -1 if null terminated */
    char			zString[1];			/* String of nLength characters, or null terminated if nLength == -1	*/
} GRndDrawText_s;

typedef	struct
{
    GRndHeader_s	sHdr;				/* Render header */
	Rect			cPos;
	uint32			nFlags;				/* Flags */
    int				nLength;			/* Length of the string, or -1 if null terminated */
	IPoint			cSel1;				/* First point of the selection */
	IPoint			cSel2;				/* Second point of the selection */
	uint32			nMode;				/* Selection mode */
    char			zString[1];			/* String of nLength characters, or null terminated if nLength == -1	*/
} GRndDrawSelectedText_s;

typedef struct
{
	GRndHeader_s	sHdr;				/* Render header */
	char    m_zName[64];				/* Clipboard to copy to */
} GRndGetSelection_s;

struct GRndScrollView_s : GRndHeader_s
{
    Point cDelta;
};

struct GRndInvalidateRect_s : GRndHeader_s
{
    bool m_bRecurse;
    Rect m_cRect;
};

struct GRndInvalidateView_s : GRndHeader_s
{
    bool m_bRecurse;
};

struct GRndRegion_s : GRndHeader_s
{
    int m_nClipCount;
};

/***	Messages sendt to main thread of the display server	***/

#define CLIPBOARD_FRAGMENT_SIZE (1024 * 32)

struct DR_GetClipboardData_s
{
    DR_GetClipboardData_s( const char* pzName, port_id hReply ) {
	strcpy( m_zName, pzName ); m_hReply = hReply;
    }
    char    m_zName[64];
    port_id m_hReply;
};

struct DR_GetClipboardDataReply_s
{
    int		m_nTotalSize;
    int		m_nFragmentSize;
    uint8	m_anBuffer[CLIPBOARD_FRAGMENT_SIZE];
};


struct DR_SetClipboardData_s
{
    char    m_zName[64];
    int     m_nTotalSize;
    int	    m_nFragmentSize;
    port_id m_hReply;		// Just used as an source ID so the server wont interleave multi-package commits
    uint8	  m_anBuffer[CLIPBOARD_FRAGMENT_SIZE];
};

struct DR_SetWindowDecorator_s
{
    char	m_zDecoratorPath[1024];
};

struct AR_DeleteWindow_s
{
    SrvWindow* pcWindow;
};
struct AR_LockDesktop_s
{
    AR_LockDesktop_s( port_id hReply, int nDesktop ) { m_hReply = hReply; m_nDesktop = nDesktop; }
  
    port_id m_hReply;
    int	  m_nDesktop;
};

struct AR_LockDesktopReply_s
{
    int	      m_nCookie;
    int	      m_nDesktop;
    IPoint      m_cResolution;
    color_space m_eColorSpace;
    area_id     m_hFrameBufferArea;
};

struct AR_UnlockDesktop_s
{
    AR_UnlockDesktop_s( int nCookie ) { m_nCookie = nCookie; }
    int m_nCookie;
};

typedef	struct
{
    int	nLength;
    char	zString[1];
} StringHeader_s;

typedef struct
{
    port_id	 hReply;
    int		 hFontToken;
    int		 nStringCount;
    StringHeader_s sFirstHeader;
      /*** nStringCount - 1, string headers follows	***/
} AR_GetStringWidths_s;


typedef struct
{
    int	nError;
    int	anLengths[1];
      /*** nStringCount - 1, lengths follow	***/
} AR_GetStringWidthsReply_s;

typedef struct
{
    port_id	 hReply;
    int		 hFontToken;
    int		 nStringCount;
    int		 nWidth;	/* Pixel width	*/
    int		 bIncludeLast;	// Should be a bool
    StringHeader_s sFirstHeader;
      /*** nStringCount - 1, string headers follows	***/
} AR_GetStringLengths_s;


typedef struct
{
    int	nError;
    int	anLengths[1];
      /*** nStringCount - 1, lengths follow	***/
} AR_GetStringLengthsReply_s;


typedef struct
{
    port_id	 hReply;			/* Reply port handle */
    int		 hFontToken;		/* Font handle */
    int		 nStringCount;		/* Number of strings to measure */
	uint32   nFlags;			/* Flags */
	int		 nTargetWidth;		/* Max. width; only valid when DTF_WRAP_SOFT is passed */
    StringHeader_s sFirstHeader;
      /*** nStringCount - 1, string headers follows	***/
} AR_GetTextExtents_s;

typedef struct
{
    int		nError;
    IPoint	acExtent[1];
      /*** nStringCount - 1 follow	***/
} AR_GetTextExtentsReply_s;


/***	Bitmap messages	***/

struct AR_CreateBitmap_s
{
    port_id	hReply;
    uint32	nFlags;
    int		nWidth;
    int		nHeight;
    color_space	eColorSpc;
};

struct AR_CreateBitmapReply_s
{
    AR_CreateBitmapReply_s() {}
    AR_CreateBitmapReply_s( int hHandle, area_id hArea ) { m_hHandle = hHandle; m_hArea = hArea; }
    int	    m_hHandle;
    area_id m_hArea;
};

struct AR_CloneBitmap_s
{
	port_id	m_hReply;
	int		m_hHandle;
};

struct AR_CloneBitmapReply_s
{
	int	    m_hHandle;
	area_id m_hArea;
	uint32	m_nFlags;
	int		m_nWidth;
	int		m_nHeight;
	color_space	m_eColorSpc;
};


struct AR_DeleteBitmap_s
{
    AR_DeleteBitmap_s( int nHandle ) { m_nHandle = nHandle; }
    int m_nHandle;
};

struct AR_CreateSprite_s
{
    AR_CreateSprite_s( port_id hReply, const Rect& cFrame, int nBitmap ) {
	m_hReply = hReply; m_cFrame = cFrame; m_nBitmap = nBitmap;
    }
    port_id m_hReply;
    int	    m_nBitmap;
    Rect   m_cFrame;
};

struct AR_CreateSpriteReply_s
{
    AR_CreateSpriteReply_s() {}
    AR_CreateSpriteReply_s( uint32 nHandle, int nError ) { m_nHandle = nHandle; m_nError = nError; }
  
    uint32 m_nHandle;
    int    m_nError;
};

struct AR_DeleteSprite_s
{
    AR_DeleteSprite_s( uint32 nHandle ) { m_nSprite = nHandle; }
    uint32 m_nSprite;
};

struct AR_MoveSprite_s
{
    AR_MoveSprite_s( uint32 nHandle, const Point& cNewPos ) {
	m_nSprite = nHandle; m_cNewPos = cNewPos;
    }
    uint32 m_nSprite;
    Point m_cNewPos;
};

struct AR_GetQualifiers_s
{
    AR_GetQualifiers_s( port_id hReply ) { m_hReply = hReply; }
    port_id m_hReply;
};

struct AR_GetQualifiersReply_s
{
    AR_GetQualifiersReply_s() {}
    AR_GetQualifiersReply_s( uint32 nQualifiers ) { m_nQualifiers = nQualifiers; }
    uint32	m_nQualifiers;
};

struct AR_GetScreenModeCount_s
{
    port_id	m_hReply;
};

struct AR_GetScreenModeCountReply_s
{
    int	m_nCount;
};

struct AR_GetScreenModeInfo_s
{
    port_id	m_hReply;
    int		m_nIndex;
};

struct AR_GetScreenModeInfoReply_s
{
    int	      m_nError;
    int	      m_nWidth;
    int 	      m_nHeight;
    int	      m_nBytesPerLine;
    color_space m_eColorSpace;
};

/***	Messages sendt to window threads in the display server	***/

struct WR_Request_s
{
    int m_hTopView;
};

enum { RENDER_BUFFER_SIZE = 8180 };

struct WR_Render_s : WR_Request_s
{
    port_id	hReply;
    int		nCount;	/* Number of render instructions	*/
    uint8	aBuffer[RENDER_BUFFER_SIZE];
};

struct WR_GetPenPosition_s : WR_Request_s
{
    port_id m_hReply;
    int	  m_hViewHandle;
};

struct WR_GetPenPositionReply_s
{
    Point m_cPos;
};

struct WR_LockFb_s
{
	port_id m_hReply;
};

struct WR_LockFbReply_s
{
	bool m_bSuccess;
	sem_id m_hSem;
	Rect m_cFrame;
	color_space m_eColorSpc;
	int m_nBytesPerLine;
};



}
#endif	//	DEVICES_DISPLAY_PACKETS_H



