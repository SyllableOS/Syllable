/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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

#ifndef	__F_GUI_GUIDEFS_H__
#define	__F_GUI_GUIDEFS_H__

/** \file gui/guidefines.h
 * \par Description:
 *	Verious global defines used throughout the GUI toolkit.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/



#ifdef __cplusplus
namespace os
{
#endif
    
//
static const float COORD_MAX = 16000000.0f;

static const char SYS_FONT_FAMILY[] = "system_default_font";
static const char SYS_FONT_FIXED[]  = "sys_fixed";
static const char SYS_FONT_PLAIN[]  = "sys_plain";
static const char SYS_FONT_BOLD[]   = "sys_bold";

static const int TAB_STOP = 28;

enum alignment
{
    ALIGN_LEFT,
    ALIGN_RIGHT,
    ALIGN_TOP,
    ALIGN_BOTTOM,
    ALIGN_CENTER
};

enum orientation
{
    HORIZONTAL,
    VERTICAL
};

enum color_space
{
    CS_NO_COLOR_SPACE,
    CS_RGB32,
    CS_RGBA32,
    CS_RGB24,
    CS_RGB16,
    CS_RGB15,
    CS_RGBA15,
    CS_CMAP8,
    CS_GRAY8,
    CS_GRAY1,
    CS_YUV422,
    CS_YUV411,
    CS_YUV420,
    CS_YUV444,
    CS_YUV9,
    CS_YUV12,
    CS_YUY2
};

enum mouse_ptr_mode
{
    MPTR_MONO  = 0x01,
    MPTR_CMAP8 = 0x02,
    MPTR_RGB32 = 0x04
};

enum {
    TRANSPARENT_CMAP8 = 0xff,
    TRANSPARENT_RGB32 = 0xffffffff
};

enum drawing_mode
{
    DM_COPY,
    DM_OVER,
    DM_INVERT,
    DM_ERASE,
    DM_BLEND,
    DM_ADD,
    DM_SUBTRACT,
    DM_MIN,
    DM_MAX,
    DM_SELECT
};

enum
{
    MOUSE_BUT_LEFT  = 0x01,
    MOUSE_BUT_MID	  = 0x02,
    MOUSE_BUT_RIGHT = 0x04,
};


enum drawtext_flags
{
	DTF_DEFAULT			= 0x00000000,
	DTF_CENTER			= 0x00000001,
	DTF_ALIGN_CENTER	= DTF_CENTER,
	DTF_ALIGN_RIGHT		= 0x00000002,
	DTF_ALIGN_LEFT		= 0x00000000,
	DTF_ALIGN_TOP		= 0x00000004,
	DTF_ALIGN_BOTTOM	= 0x00000008,
	DTF_ALIGN_MIDDLE	= 0x00000000,
	DTF_IGNORE_FMT		= 0x00000100,
	DTF_UNDERLINES		= 0x00000200,
	DTF_WRAP_SOFT		= 0x00001000
};

enum selection_flags
{
	SEL_DEFAULT			= 0x00000000,
	SEL_NONE			= SEL_DEFAULT,
	SEL_CHAR			= 0x00000001,
	SEL_WORD			= 0x00000002,
	SEL_RECT			= 0x00000004
};

/**\anchor os_gui_qualifiers
 * \par Description:
 *	Bit-masks for the verious qualifier keys. This defines can be used
 *	to check whether a given qualifier (or group of qualifiers) are
 *	pressed or not. The current qualifier mask is passed into various
 *	hook members dealing with keyboard input in the View class and it
 *	can also be retrieved asyncronously with the View::GetQualifiers()
 *	member.
 *****************************************************************************/

enum
{
    QUAL_LSHIFT = 0x01,				//!< Left <SHIFT> key
    QUAL_RSHIFT = 0x02,				//!< Right <SHIFT> key
    QUAL_SHIFT  = QUAL_LSHIFT | QUAL_RSHIFT,	//!< Any <SHIFT> key

    QUAL_LCTRL  = 0x04,				//!< Left <CONTROL> key
    QUAL_RCTRL  = 0x08,				//!< Right <CONTROL> key
    QUAL_CTRL   = QUAL_LCTRL | QUAL_RCTRL,	//!< Any <CONTROL> key

    QUAL_LALT   = 0x10,				//!< Left <ALT> key
    QUAL_RALT   = 0x20,				//!< Right <ALT> key
    QUAL_ALT    = QUAL_LALT | QUAL_RALT,	//!< Any <ALT> key

    QUAL_REPEAT = 0x40,				/*!< Set if the key-down event was caused
						 *   by key repeating.
						 */

    QUAL_DEADKEY = 0x100			/** Set if the event was caused by a deadkey */
};

/** \anchor os_gui_keyvalues
 * Predefined ASCII values for the various special keys. 
 *****************************************************************************/

enum
{
    VK_BACKSPACE    = 0x08,
    VK_ENTER	    = 0x0a,
    VK_RETURN	    = 0x0a,
    VK_SPACE	    = 0x20,
    VK_TAB	    = 0x09,
    VK_ESCAPE	    = 0x1b,
    VK_LEFT_ARROW   = 0x1c,
    VK_RIGHT_ARROW  = 0x1d,
    VK_UP_ARROW	    = 0x1e,
    VK_DOWN_ARROW   = 0x1f,
    VK_INSERT	    = 0x05,
    VK_DELETE	    = 0x7f,
    VK_HOME	    = 0x01,
    VK_END	    = 0x04,
    VK_PAGE_UP	    = 0x0b,
    VK_PAGE_DOWN    = 0x0c,
    VK_FUNCTION_KEY = 0x10
};


enum
{
    MBI_LEFT        = 1,
    MBI_RIGHT       = 2,
    MBI_MIDDLE      = 3,
    MBI_FIRST_EXTRA = 4
};


enum
{
    MBM_LEFT	= 0x01,
    MBM_RIGHT	= 0x02,
    MBM_MIDDLE  = 0x04
};

/** \anchor os_gui_sysmessages
 * This is the various messages that is sent by the system. Most of these
 * are handled internally by the various GUI classes but every now and then
 * you might have to deal with them on your own. Input event messages like
 * M_MOUSE_DOWN/M_MOUSE_UP/M_MOUSE_MOVED/M_KEY_DOWN, etc etc are all translated
 * into calls of the various os::View virtual hook members so you don't have
 * to know anything about the raw messages to deal with them.
 *
 *
 * \par M_KEY_DOWN/M_KEY_UP:
 *	This messages are sendt to the active window to report keyboard
 *	activity. They are translated into calls of View::KeyDown()
 *	View::KeyUp() on the active view by the Window class so you
 *	normally don't have to deal with it on your own. Not all
 *	members of the message are extracted and passed to the
 *	virtual hooks though so you might occationally have to
 *	mess with the raw message yourself.
 * \par
 *	<table><tr><td>Name</td><td>Type</td><td>Description</td></tr>
 *	<tr><td>_raw_key</td><td>T_INT32</td><td>The raw untranslated key-code</td></tr>
 *	<tr><td>_qualifiers</td><td>T_INT32</td><td>The current \ref os_gui_qualifiers "qualifier" mask</td></tr>
 *	<tr><td>_string</td><td>T_STRING</td><td>String containing a single UTF-8 encoded character.</td></tr>
 *	<tr><td>_raw_string</td><td>T_STRING</td><td>String containing a single UTF-8 encoded character.</td></tr>
 *	</table>
 *	<p>
 * \par M_MOUSE_DOWN/M_MOUSE_UP
 *	This messages are send when the user click one of the mouse buttons.
 *	They are translated into calls of View::MouseDown() and View::MouseUp()
 *	by the Window class and are rarly touched from user code.
 * \par
 *	<table><tr><td>Name</td><td>Type</td><td>Description</td></tr>
 *	<tr><td>_button</td><td>T_INT32</td><td>Button number. Buttons are numbered from 1 where 1 is
 *						the left button, 2 is the right button, and 3 is the
 *						middle button. The mouse driver can also suppor
 *						additional buttons and this will then be assigned
 *						numbers from 4 and up.</td></tr>
 *	<tr><td>_drag_message</td><td>T_MESSAGE</td><td>This member is only present in M_MOUSE_UP
 *							messages and only if the M_MOUSE_UP was the
 *							end of a drag and drop operating. The content
 *							is the data being dragged as defined by the
 *							View::BeginDrag() member.</td></tr>
 *	</table>
 * 
 *****************************************************************************/

enum {
	/* Attention! */
	/* Be careful not to change the sequence of message codes here, */
	/* as doing so would break binary compatibility! */
	
    M_LAST_USER_MSG   = 9999999,	
    M_ABOUT_REQUESTED = 10000000,
    M_WINDOW_ACTIVATED,
    M_APP_ACTIVATED,
    M_ARGV_RECEIVED,
    M_QUIT_REQUESTED,
    M_CLOSE_REQUESTED,

    M_KEY_DOWN,
    M_KEY_UP,
    M_MINIMIZE,
    M_MOUSE_DOWN,
    M_MOUSE_UP,
    M_MOUSE_MOVED,
    M_WHEEL_MOVED,
    M_READY_TO_RUN,
    M_PATHS_RECEIVED,
    M_SCREEN_CHANGED,
    M_VALUE_CHANGED,
    M_VIEW_MOVED,
    M_VIEW_RESIZED,

    M_WINDOW_MOVED,
    M_WINDOW_RESIZED,
    M_DESKTOP_ACTIVATED,
    M_SCREENMODE_CHANGED,
    M_ZOOM,
    M_PAINT,
    M_COLOR_CONFIG_CHANGED,
    M_FONT_CHANGED,
    M_MENU_EVENT,
    M_WINDOWS_CHANGED,
    _M_UNUSED3,
    M_QUIT,			/* Quit if you want	*/
    M_TERMINATE,			/* Quit NOW	*/
    _M_UNUSED4,
    _M_UNUSED5,
    _M_UNUSED6,
    _M_UNUSED7,
    _M_UNUSED8,
    _M_UNUSED9,
    M_MOVE_WINDOW,
    M_RESIZE_WINDOW,
    M_WINDOW_FRAME_CHANGED,

	/* Add new appserver message codes here. */

    M_SET_PROPERTY = 20000000,
    M_GET_PROPERTY,
    M_CREATE_PROPERTY,
    M_DELETE_PROPERTY,
    M_GET_SUPPORTED_SUITES,
    M_CUT,
    M_COPY,
    M_PASTE,
    M_LOAD_REQUESTED,
    M_SAVE_REQUESTED,
    M_FILE_REQUESTER_CANCELED,

    M_MESSAGE_NOT_UNDERSTOOD,
    M_NO_REPLY,
    M_REPLY,
    M_SIMPLE_DATA,
    M_MIME_DATA,
    M_ARCHIVED_OBJECT,
    M_UPDATE_STATUS_BAR,
    M_RESET_STATUS_BAR,
    M_NODE_MONITOR,

   	M_FONT_REQUESTED,
	M_FONT_REQUESTER_CANCELED,

	M_BOX_REQUESTED,
	M_BOX_REQUESTER_CANCELED,

	M_COLOR_REQUESTER_CANCELED,
	M_COLOR_REQUESTED,
	M_COLOR_REQUESTER_CHANGED,
	
	M_CLIPBOARD_CHANGED,
    
    /* Add new libsyllable message codes here. */
    
    M_FIRST_EVENT = 1000000000,
};

struct screen_mode
{
#ifdef __cplusplus
	screen_mode() { }
	screen_mode( int nWidth, int nHeight, int nBytesPerLine, color_space eColorSpc, float vRefreshRate ) {
		m_nWidth = nWidth;
		m_nHeight = nHeight;
		m_nBytesPerLine = nBytesPerLine;
		m_eColorSpace = eColorSpc;
		m_vRefreshRate = vRefreshRate;
	}
#endif

    int		     m_nWidth;
    int		     m_nHeight;
    int		     m_nBytesPerLine;
    enum color_space m_eColorSpace;
    float	     m_vRefreshRate;
    float	     m_vHPos;
    float	     m_vVPos;
    float	     m_vHSize;
    float	     m_vVSize;
};

#ifdef __cplusplus
} // End of namespace
#endif

#endif	// __F_GUI_GUIDEFS_H__






