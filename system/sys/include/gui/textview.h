/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 - 2005 Syllable Team
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

#ifndef __F_GUI_TEXTVIEW_H__
#define __F_GUI_TEXTVIEW_H__

#include <gui/view.h>
#include <gui/font.h>
#include <gui/control.h>
#include <util/string.h>

#include <vector>
#include <string>
#include <list>
namespace os
{
#if 0
} /* Fool emacs autoindent */
#endif

class TextEdit;

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class TextView : public Control
{
public:
    enum {
	EI_CONTENT_CHANGED	= 0x0001,
	EI_ENTER_PRESSED	= 0x0002,
	EI_ESC_PRESSED		= 0x0004,
	EI_FOCUS_LOST		= 0x0008,
	EI_CURSOR_MOVED		= 0x0010,
	EI_SELECTION_CHANGED	= 0x0020,
	EI_MAX_SIZE_REACHED	= 0x0040,
	EI_MAX_SIZE_LEFT	= 0x0080,
	EI_WAS_EDITED		= 0x0100
    };
public:
    typedef std::vector<String> buffer_type;
  
    TextView( const Rect& cFrame, const String& cTitle, const char* pzBuffer,
	      uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP,
	      uint32 nFlags = WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );
    ~TextView();

    virtual void	MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData );
    virtual void	MouseDown( const Point& cPosition, uint32 nButtons );
    virtual void	MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData );
    virtual void	WheelMoved( const Point& cDelta );
    virtual void	KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );

    virtual void LabelChanged( const String& cNewLabel );
    virtual void EnableStatusChanged( bool bIsEnabled );
    virtual bool Invoked( Message* pcMessage );
    virtual void Activated( bool bIsActive );

    virtual void    SetValue( Variant cValue, bool bInvoke = true );
    virtual Variant GetValue() const;
    
    const View* GetEditor() const;
   
    void  SetMultiLine( bool bMultiLine = true );
    bool  GetMultiLine() const;

    void  SetPasswordMode( bool bPassword = true );
    bool  GetPasswordMode() const;

    void  SetNumeric( bool bNumeric );
    bool  GetNumeric() const;
    
    void  SetReadOnly( bool bFlag = true );
    bool  GetReadOnly() const;

    int	  GetMaxUndoSize() const;
    void  SetMaxUndoSize( int nSize );


    uint32 GetEventMask() const;
    void   SetEventMask( uint32 nMask );
    
    void  GetRegion( String* pcBuffer ) const;

    void   SetMinPreferredSize( int nWidthChars, int nHeightChars );
    IPoint GetMinPreferredSize() const;
    void   SetMaxPreferredSize( int nWidthChars, int nHeightChars );
    IPoint GetMaxPreferredSize() const;
    
    void MakeCsrVisible();
    void Clear( bool bSendNotify = true );
    void Set( const char* pzBuffer, bool bSendNotify = true );
    void Insert( const char* pzBuffer, bool bSendNotify = true );
    void Insert( const IPoint& cPos, const char* pzBuffer, bool bSendNotify = true );

    void	Select( const IPoint& cStart, const IPoint& cEnd, bool bSendNotify = true );
    void	SelectAll( bool bSendNotify = true );
    void	ClearSelection( bool bSendNotify = true );
    bool	GetSelection( IPoint* pcStart = NULL, IPoint* pcEnd = NULL ) const;
    void	SetCursor( int x, int y, bool bSelect = false, bool bSendNotify = true );
    void	SetCursor( const IPoint& cPos, bool bSelect = false, bool bSendNotify = true  );
    void	GetCursor( int* x, int* y ) const;
    IPoint	GetCursor() const;
    
    void SetMaxLength( size_t nMaxLength );
    size_t GetMaxLength() const;
    size_t GetCurrentLength() const;
    
    void Cut( bool bSendNotify = true );
    void Copy();
    void Paste( bool bSendNotify = true );
    void Delete( bool bSendNotify = true );
    void Delete( const IPoint& cStart, const IPoint& cEnd, bool bSendNotify = true );
    void Undo();
    void Redo();
  
    const buffer_type& GetBuffer() const;

    virtual void	SetTabOrder( int nOrder );
    virtual Point	GetPreferredSize( bool bLargest ) const;
    virtual bool	FilterKeyStroke( const String* pcString );
  
    virtual void	FontChanged( Font* pcNewFont );
    virtual void	FrameSized( const Point& cDelta );
    virtual void	Paint( const Rect& cUpdateRect );

private:
    virtual void __TV_reserved1__();
    virtual void __TV_reserved2__();
    virtual void __TV_reserved3__();
    virtual void __TV_reserved4__();
    virtual void __TV_reserved5__();

private:
    TextView& operator=( const TextView& );
    TextView( const TextView& );

    void	AdjustScrollbars( bool bDoScroll = true, float vDeltaX = 0.0f, float vDeltaY = 0.0f );
    friend class TextEdit;
    TextEdit* m_pcEditor;
};


} // end of namespace os

#endif // F_GUI_TEXTVIEW_H
