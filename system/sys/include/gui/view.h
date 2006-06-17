/*  libsyllable.so - the highlevel API library for Syllable
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

#ifndef	__F_GUI_VIEW_H__
#define	__F_GUI_VIEW_H__

/** \file gui/view.h
 * \par Description:
 *	Declaration of the os::View class
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


#include <atheos/types.h>
#include <util/string.h>
#include <util/handler.h>
#include <util/shortcutkey.h>
#include <gui/guidefines.h>
#include <gui/gfxtypes.h>
#include <gui/region.h>

namespace os
{
#if 0	// Fool Emacs auto-indent
}
#endif

Color32_s GetStdColor( int i );

class Bitmap;
class ScrollBar;
struct font_height;

#if 0
enum
{
    PEN_DETAIL,	      /* primary pen (Draw, PutBixel, ... )			*/
    PEN_BACKGROUND,   /* secondary pen ( EraseRect, text background, ...)	*/
    PEN_SHINE,	      /* bright side of 3D object 				*/
    PEN_SHADOW,	      /* dark side of 3D object					*/

    PEN_BRIGHT,       /* same as PEN_SHINE, but a bit darker			*/
    PEN_DARK,	      /* same as PEN_SHADOW, but a bit brighter			*/
    PEN_WINTITLE,     /* Fill inside window title of a unselected window	*/
    PEN_WINBORDER,    /* Window border fill when unselected			*/

    PEN_SELWINTITLE,  /* Fill inside window title of a selected window		*/
    PEN_SELWINBORDER, /* Window border fill when selected			*/
    PEN_WINDOWTEXT,
    PEN_SELWNDTEXT,

    PEN_WINCLIENT,    /* Used to clear areas inside client area of window	*/
    PEN_GADGETFILL,
    PEN_SELGADGETFILL,
    PEN_GADGETTEXT,

    PEN_SELGADGETTEXT
};
#endif

enum default_color_t
{
    COL_NORMAL,
    COL_SHINE,
    COL_SHADOW,
    COL_SEL_WND_BORDER,
    COL_NORMAL_WND_BORDER,
    COL_MENU_TEXT,
    COL_SEL_MENU_TEXT,
    COL_MENU_BACKGROUND,
    COL_SEL_MENU_BACKGROUND,
    COL_SCROLLBAR_BG,
    COL_SCROLLBAR_KNOB,
    COL_LISTVIEW_TAB,
    COL_LISTVIEW_TAB_TEXT,
    COL_ICON_TEXT,
    COL_ICON_SELECTED,
	COL_ICON_BG,
	COL_FOCUS,
    COL_COUNT
};

Color32_s get_default_color( default_color_t nColor );
void __set_default_color( default_color_t nColor, const Color32_s& sColor );
void set_default_color( default_color_t nColor, const Color32_s& sColor );

enum
{
    FRAME_RECESSED    = 0x000008,
    FRAME_RAISED      = 0x000010,
    FRAME_THIN	      = 0x000020,
    FRAME_WHIDE	      = 0x000040,
    FRAME_ETCHED      = 0x000080,
    FRAME_FLAT	      = 0x000100,
    FRAME_DISABLED    = 0x000200,
    FRAME_TRANSPARENT = 0x010000
};

class	Window;
class	Message;
class	Font;
class	Menu;


enum
{
    MOUSE_INSIDE,
    MOUSE_OUTSIDE,
    MOUSE_ENTERED,
    MOUSE_EXITED
};

/** \brief Tab order allocation
 * \ingroup gui
 * \sa os::View::SetTabOrder
 *****************************************************************************/
enum tab_order
{
	NO_TAB_ORDER = -1,
	NEXT_TAB_ORDER = -2
};


/** \brief Flags controlling a View
 * \ingroup gui
 * \sa os::view_resize_flags, os::View
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

enum view_flags
{
    WID_FULL_UPDATE_ON_H_RESIZE	= 0x0001,  ///< Cause the entire view to be invalidated if made higher
    WID_FULL_UPDATE_ON_V_RESIZE	= 0x0002,  ///< Cause the entire view to be invalidated if made wider
    WID_FULL_UPDATE_ON_RESIZE	= 0x0003,  ///< Cause the entire view to be invalidated if resized
    WID_WILL_DRAW		= 0x0004,  ///< Tell the appserver that you want to render stuff to it
    WID_TRANSPARENT		= 0x0008,  ///< Allow the parent view to render in areas covered by this view
    WID_CLEAR_BACKGROUND	= 0x0010,  ///< Automatically clear new areas when windows are moved/resized
    WID_DRAW_ON_CHILDREN	= 0x0020   ///< Setting this flag allows the view to render atop of all its childs
};

/** \brief Flags controlling how to resize/move a view when the parent is resized.
 * \ingroup gui
 * \sa os::view_flags, os::View
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

enum view_resize_flags
{
    CF_FOLLOW_NONE	= 0x0000, ///< Neither the size nor the position is changed.
    CF_FOLLOW_LEFT	= 0x0001, ///< Left edge follows the parents left edge.
    CF_FOLLOW_RIGHT	= 0x0002, ///< Right edge follows the parents right edge.
    CF_FOLLOW_TOP	= 0x0004, ///< Top edge follows the parents top edge.
    CF_FOLLOW_BOTTOM	= 0x0008, ///< Bottom edge follows the parents bottom edge.
    CF_FOLLOW_ALL	= 0x000F, ///< All edges follows the corresponding edge in the parent
      /**
       * If the CF_FOLLOW_LEFT is set the right edge follows the parents center.
       * if the CF_FOLLOW_RIGHT is set the left edge follows the parents center.
       */
    CF_FOLLOW_H_MIDDLE	= 0x0010,
      /**
       * If the CF_FOLLOW_TOP is set the bottom edge follows the parents center.
       * if the CF_FOLLOW_BOTTOM is set the top edge follows the parents center.
       */
    CF_FOLLOW_V_MIDDLE	= 0x0020,
    CF_FOLLOW_SPECIAL	= 0x0040,
    CF_FOLLOW_MASK	= 0x007f
};

/** Base class for all GUI components.
 * \ingroup gui
 *
 * The View class is the work horse in the GUI. Before you can render any graphics
 * into a window, you have to create one or more views and attatch them to it.
 * Views can be added to a window, or to another view to create a hierarchy. To render
 * someting into a window you normaly inherit this class and overload the Pain() function.
 * Each view has it's own graphical environment consisting of three colors (foreground
 * background and erase), a pen position and the drawing mode. Each view lives in a
 * coordinate system relative to its parent. Views are also responsible for receiving
 * user input. The active view in the active window receives keybord and mouse events.
 * 
 *  \sa os::Window, os::Handler, \ref os::view_flags
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class View : public Handler
{
public:
    View( const Rect& cFrame, const String& cTitle,
	  uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP,
	  uint32 nFlags = WID_WILL_DRAW | WID_CLEAR_BACKGROUND );
	
    virtual ~View();


      // Hook functions
    virtual void	AttachedToWindow();
    virtual void	AllAttached();
    virtual void	DetachedFromWindow();
    virtual void	AllDetached();
    virtual void	Activated( bool bIsActive );
    virtual void	WindowActivated( bool bIsActive );

    virtual void	Paint( const Rect& cUpdateRect );
  
    virtual void	MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData );
    virtual void	MouseDown( const Point& cPosition, uint32 nButtons );
    virtual void	MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData );
    virtual void	KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );
    virtual void	KeyUp( const char* pzString, const char* pzRawString, uint32 nQualifiers );

    virtual void	FrameMoved( const Point& cDelta );
    virtual void	FrameSized( const Point& cDelta );
    virtual void	ViewScrolled( const Point& cDelta );
    virtual void	FontChanged( Font* pcNewFont );
  
    virtual Point	GetPreferredSize( bool bLargest ) const;
    virtual Point	GetContentSize() const;

    virtual void	WheelMoved( const Point& cDelta );
private:
    virtual void	__VW_reserved2__();
    virtual void	__VW_reserved3__();
    virtual void	__VW_reserved4__();
    virtual void	__VW_reserved5__();
    virtual void	__VW_reserved6__();
    virtual void	__VW_reserved7__();
    virtual void	__VW_reserved8__();
    virtual void	__VW_reserved9__();
    virtual void	__VW_reserved10__();
    virtual void	__VW_reserved11__();
    virtual void	__VW_reserved12__();
    virtual void	__VW_reserved13__();
    virtual void	__VW_reserved14__();
    virtual void	__VW_reserved15__();
    virtual void	__VW_reserved16__();
    virtual void	__VW_reserved17__();
    virtual void	__VW_reserved18__();
    virtual void	__VW_reserved19__();
    virtual void	__VW_reserved20__();
public:
      // End of hook functions

    virtual void	AddChild( View* pcView, bool bAssignTabOrder = false );
    void 		RemoveChild( View* pcChild );
    void		RemoveThis();
    View*		GetChildAt( const Point& cPos ) const;
    View*		GetChildAt( int nIndex ) const;
    View*		GetParent() const;
    ScrollBar*		GetVScrollBar() const;
    ScrollBar*		GetHScrollBar() const;
    Window*		GetWindow() const { return( (Window*)GetLooper() ); }
    String		GetTitle() const;

    virtual int		GetTabOrder() const;
    virtual void	SetTabOrder( int nOrder = NEXT_TAB_ORDER );

	virtual const ShortcutKey& GetShortcut() const;
	virtual void SetShortcut( const ShortcutKey& cShortcut );
	virtual void SetShortcutFromLabel( const String& cLabel );

	void		SetContextMenu( Menu* pcMenu );
	Menu*		GetContextMenu() const;
  
    uint32		GetQualifiers() const;
    void		GetMouse( Point* pcPosition, uint32* pnButtons ) const;
    void		SetMousePos( const Point& cPosition );

    void		BeginDrag( Message* pcData, const Point& cOffset, const Bitmap* pcBitmap,
				   Handler* pcReplyTarget = NULL );
    void		BeginDrag( Message* pcData, const Point& cOffset, const Rect& cBounds,
				   Handler* pcReplyTarget = NULL );
  
    void		SetFlags( uint32 nFlags );
    uint32		GetFlags( uint32 nMask = ~0L )	const;

    void		SetResizeMask( uint32 nFlags );
    uint32		GetResizeMask()	const;
    
    void		Show( bool bVisible = true );
    void		Hide() { Show( false ); }
    bool		IsVisible() const;
    virtual void	MakeFocus( bool bFocus = true );
    virtual bool	HasFocus() const;

    Rect		GetFrame() const;
    Rect		GetBounds() const;
    Rect		GetNormalizedBounds() const;
    float		Width() const;
    float		Height() const;
    Point		GetLeftTop() const;

    virtual void	SetFrame( const Rect& cRect, bool bNotifyServer = true );
    virtual void	MoveBy( const Point& cDelta );
    virtual void	MoveBy( float vDeltaX, float vDeltaY );
    virtual void	MoveTo( const Point& cPos );
    virtual void	MoveTo( float x, float y );

    virtual void	ResizeBy( const Point& cDelta );
    virtual void	ResizeBy( float vDeltaW, float vDeltaH );
    virtual void	ResizeTo( const Point& cSize );
    virtual void	ResizeTo( float W, float H );

    void 		SetDrawingRegion( const Region& cReg );
    void		ClearDrawingRegion();
    void 		SetShapeRegion( const Region& cReg );
    void		ClearShapeRegion();
    
    virtual int		ToggleDepth();
    Point		ConvertToParent( const Point& cPoint ) const;
    void		ConvertToParent( Point* cPoint ) const;
    Rect		ConvertToParent( const Rect& cRect ) const;
    void		ConvertToParent( Rect* cRect ) const;
    Point		ConvertFromParent( const Point& cPoint ) const;
    void		ConvertFromParent( Point* cPoint ) const;
    Rect		ConvertFromParent( const Rect& cRect ) const;
    void		ConvertFromParent( Rect* cRect ) const;

    Point		ConvertToWindow( const Point& cPoint ) const;
    void		ConvertToWindow( Point* cPoint ) const;
    Rect		ConvertToWindow( const Rect& cRect ) const;
    void		ConvertToWindow( Rect* cRect ) const;

    Point		ConvertFromWindow( const Point& cPoint ) const;
    void		ConvertFromWindow( Point* cPoint ) const;
    Rect		ConvertFromWindow( const Rect& cRect ) const;
    void		ConvertFromWindow( Rect* cRect ) const;


    Point		ConvertToScreen( const Point& cPoint ) const;
    void		ConvertToScreen( Point* cPoint ) const;
    Rect		ConvertToScreen( const Rect& cRect ) const;
    void		ConvertToScreen( Rect* cRect ) const;

    Point		ConvertFromScreen( const Point& cPoint ) const;
    void		ConvertFromScreen( Point* cPoint ) const;
    Rect		ConvertFromScreen( const Rect& cRect ) const;
    void		ConvertFromScreen( Rect* cRect ) const;

    void		Invalidate( const Rect&	cRect, bool bRecurse = false );
    void		Invalidate( bool bRecurse = false );

    void		Flush();
    void		Sync();

    void		SetDrawingMode( drawing_mode nMode );
    drawing_mode	GetDrawingMode() const;
    void		SetFont( Font* pcFont );
    Font*		GetFont() const;

    void		SetFgColor( int nRed, int nGreen, int nBlue, int nAlpha = 255 );
    void		SetFgColor( Color32_s sColor );
    Color32_s		GetFgColor() const;

    void		SetBgColor( int nRed, int nGreen, int nBlue, int nAlpha = 255 );
    void		SetBgColor( Color32_s sColor );
    Color32_s		GetBgColor() const;

    void		SetEraseColor( int nRed, int nGreen, int nBlue, int nAlpha = 255 );
    void		SetEraseColor( Color32_s sColor );
    Color32_s		GetEraseColor() const;

    void		MovePenTo( const Point& cPos );
    void		MovePenTo( float x, float y )		{ MovePenTo( Point( x, y ) ); }
    void		MovePenBy( const Point& cPos );
    void		MovePenBy( float x, float y )		{ MovePenBy( Point( x, y ) ); }
    Point		GetPenPosition() const;
    void		DrawLine( const Point& cToPoint );
    void		DrawLine( const Point& cFromPnt, const Point& cToPnt );
    virtual void	ScrollBy( const Point& cDelta );
    virtual void	ScrollBy( float vDeltaX, float vDeltaY ) { ScrollBy( Point( vDeltaX, vDeltaY ) );	}
    virtual void	ScrollTo( Point cTopLeft );
    virtual void	ScrollTo( float x, float y )		 { ScrollTo( Point( x, y ) );	}
    Point		GetScrollOffset() const;
    void		ScrollRect( const Rect& cSrcRect, const Rect& cDstRect );
  
    void		FillRect( const Rect& cRect );
    void		FillRect( const Rect& cRect, Color32_s sColor );	// WARNING: Will leave HiColor at sColor
    void		DrawBitmap( const Bitmap* pcBitmap, const Rect& cSrcRect, const Rect& cDstRect );
    void		EraseRect( const Rect& cRect );
    void		DrawFrame( const Rect& cRect, uint32 nFlags );

    void		DrawString( const Point& cPos, const String& cString );
    void		DrawString( const String& cString );
    void		DrawString( const char *pzStr, int Len = -1 );
    void		DrawText( const Rect& cPos, const String& cString, uint32 nFlags = 0 );
    void		DrawSelectedText( const Rect& cPos, const String& cString, const IPoint& cSel1, const IPoint& cSel2, uint32 nMode = SEL_CHAR, uint32 nFlags = 0 );

	void		GetSelection( const String &cClipboard = "__system_clipboard__" );

      // Font functions.
    void            	GetTruncatedStrings( const char** pazStringArray,
					     int    nStringCount,
					     uint32 nMode,
					     float  nWidth,
					     char** pazResultArray ) const;

    float		GetStringWidth( const String& cString ) const;
    float		GetStringWidth( const char* pzString, int nLen = -1 ) const;
    void		GetStringWidths( const char** apzStringArray, const int* anLengthArray,
					 int nStringCount, float* avWidthArray ) const;

    Point		GetTextExtent( const String& cString, uint32 nFlags = 0, int nTargetWidth = -1 ) const;

    int			GetStringLength( const String& cString, float vWidth, bool bIncludeLast = false ) const;
    int			GetStringLength( const char* pzString, int nLen, float vWidth, bool bIncludeLast = false ) const;
    void		GetStringLengths( const char** apzStringArray, const int* anLengthArray, int nStringCount,
					  float vWidth, int* anMaxLengthArray, bool bIncludeLast = false ) const;

    void		GetFontHeight( font_height* psHeight ) const;
  
    void		Ping( int nSize = 0 ) const;
private:
    friend class Window;
    friend class ScrollBar;
    friend class Font;

    View& operator=( const View& );
    View( const View& );

    static int	s_nNextTabOrder;
  
    void	_LinkChild( View* pcChild, bool bTopmost );  // Add a view to the child list
    void	_UnlinkChild( View* pcChild );		   // Remove a view from the child list
    void	_Attached( Window* pcFrame, View* pcParent, int hHandle, int nHideCount );
    void	_Detached( bool bFirst, int nHideCount );
    void 	_IncHideCount( bool bVisible );
    int		_GetHandle() const;

    void	_SetMouseMoveRun( int nValue );
    int		_GetMouseMoveRun() const;

    void	_SetMouseMode( int nMode );
    int		_GetMouseMode() const;

    void	_SetHScrollBar( ScrollBar* pcScrollBar );
    void	_SetVScrollBar( ScrollBar* pcScrollBar );

    void	_BeginUpdate();
    void	_EndUpdate();
    
    void	_ParentSized( const Point& cDelta );
    void	_WindowActivated( bool bIsActive );
    void	_ReleaseFont();
    void	_ConstrictRectangle( Rect* pcRect, const Point& cOffset );

    class Private;
    Private* m;
};

} // end of namespace

#endif	// __F_GUI_VIEW_H__



