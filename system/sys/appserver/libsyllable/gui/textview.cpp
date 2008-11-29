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

#include <stdio.h>
#include <assert.h>
#include <ctype.h>

#include <gui/textview.h>
#include <gui/scrollbar.h>
#include <gui/bitmap.h>
#include <util/clipboard.h>
#include <util/application.h>
#include <util/message.h>


#define LEFT_BORDER   2
#define RIGHT_BORDER  2
#define TOP_BORDER    2
#define BOTTOM_BORDER 2

#define POINTER_WIDTH  7
#define POINTER_HEIGHT 14
static uint8 g_anMouseImg[] = {
	0x02, 0x02, 0x02, 0x00, 0x02, 0x02, 0x02,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x02, 0x02, 0x02, 0x00, 0x02, 0x02, 0x02
};


#define BACKBUF_HEIGHT 128


namespace os
{

	class TextEdit:public View
	{
	      public:
		TextEdit( TextView * pcParent, const Rect & cFrame, const String &cTitle, uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP, uint32 nFlags = WID_WILL_DRAW | WID_CLEAR_BACKGROUND );
		 ~TextEdit();

		void SetEnable( bool bEnabled = true );
		bool IsEnabled() const;

		void SetMultiLine( bool bMultiLine )
		{
			m_bMultiLine = bMultiLine;
		}
		bool GetMultiLine() const
		{
			return ( m_bMultiLine );
		}

		void SetPasswordMode( bool bPassword );
		bool GetPasswordMode() const;

		void SetNumeric( bool bNumeric )
		{
			m_bNumeric = bNumeric;
		}
		bool GetNumeric() const
		{
			return ( m_bNumeric );
		}

		void SetReadOnly( bool bFlag );
		bool GetReadOnly() const;

		uint32 GetEventMask() const;
		void SetEventMask( uint32 nMask );

		int GetMaxUndoSize() const
		{
			return ( m_nMaxUndoSize );
		}
		void SetMaxUndoSize( int nSize );

		void SetMinPreferredSize( int nWidthChars, int nHeightChars );
		IPoint GetMinPreferredSize() const;
		void SetMaxPreferredSize( int nWidthChars, int nHeightChars );
		IPoint GetMaxPreferredSize() const;

		void GetRegion( IPoint cStart, IPoint cEnd, String *pcBuffer, bool bAddToClipboard = true ) const;
		void GetRegion( String *pcBuffer, bool bAddToClipboard = true ) const;
		void MakeCsrVisible();
		void InsertString( IPoint * pcPos, const char *pzBuffer, bool bMakeUndo = true );
		void Delete( IPoint cStart, IPoint cEnd, bool bMakeUndo = true );
		void Delete();
		void Clear();

		void Select( const IPoint & cStart, const IPoint & cEnd );
		void SelectAll();
		void ClearSelection();
		bool GetSelection( IPoint * pcStart, IPoint * pcEnd ) const;
		void SetCursor( const IPoint & cPos, bool bSelect );
		IPoint GetCursor() const;

		void SetMaxLength( size_t nMaxLength );
		size_t GetMaxLength() const;
		size_t GetCurrentLength() const;


		const TextView::buffer_type &GetBuffer() const
		{
			return ( m_cBuffer );
		}

		virtual Point GetPreferredSize( bool bLargest )const;

		virtual void Activated( bool bIsActive );
		virtual void FontChanged( Font * pcNewFont );
		virtual void FrameSized( const Point & cDelta );
		virtual void Paint( const Rect & cUpdateRect );
		void MouseMove( const Point & cNewPos, int nCode, uint32 nButtons, Message * pcData );
		void MouseDown( const Point & cPosition, uint32 nButtons );
		void MouseUp( const Point & cPosition, uint32 nButtons, Message * pcData );
		bool HandleKeyDown( const char *pzString, const char *pzRawString, uint32 nQualifiers );
		void UpdateBackBuffer();
		void CommitEvents();

//    private:
		enum
		{ UNDO_INSERT, UNDO_DELETE };

		struct UndoNode
		{
			UndoNode( int nMode )
			{
				m_nMode = nMode;
			}
			int m_nMode;
			IPoint m_cPos;
			IPoint m_cEndPos;

			String m_cText;
		};

		void DrawCursor( View * pcView = NULL, float vHOffset = 0.0f, float vVOffset = 0.0f );
		float GetPixelPosX( const String &cString, int x );
		float GetPixelPosY( int y );
		int GetCharPosX( const String &cString, float vPos );
		void MaybeDrawString( View * pcView, float vHOffset, const char *pzString, int nLength );
		void RenderLine( View * pcView, int y, float vHOffset, float vVOffset, bool bClear = true );
		void InsertChar( char nChar );
		void MoveHoriz( int nDelta, bool bExpBlock );
		void MoveVert( int nDelta, bool bExpBlock );
		void MoveWord( bool bDirection, bool bExpBlock );
		void RecalcPrefWidth();
		void AddUndoNode( UndoNode * psNode );
		void Undo();
		void Redo();
		void InvalidateLines( int nFirst, int nLast );

		uint32 m_nEventMask;
		uint32 m_nPendingEvents;
		float m_vCsrGfxPos;
		font_height m_sFontHeight;
		float m_vGlyphHeight;
		IPoint m_cCsrPos;
		IPoint m_cPrevCursorPos;
		IPoint m_cRegionStart;
		IPoint m_cRegionEnd;
		bool m_bRegionActive;
		bool m_bMultiLine;
		bool m_bPassword;
		bool m_bReadOnly;
		bool m_bNumeric;
		bool m_bEnabled;
		bool m_bEnforceBackBuffer;
		bool m_bMouseDownSeen;
		bool m_bIBeamActive;
		TextView::buffer_type m_cBuffer;
		std::vector < float >m_cLineSizes;
		TextView *m_pcParent;
		Point m_cPreferredSize;
		std::list <UndoNode * >m_cUndoStack;
		std::list <UndoNode * >::iterator m_iUndoIterator;
		int m_nUndoMemSize;
		int m_nMaxUndoSize;
		IPoint m_cMinPreferredSize;
		IPoint m_cMaxPreferredSize;
		Color32_s m_sCurFgColor;
		Color32_s m_sCurBgColor;
		int m_nMaxLength;
		int m_nCurrentLength;

		Bitmap *m_pcBackBuffer;
		View *m_pcBgView;

	};

}				// end of namespace



using namespace os;


/** os::TextView constructor
 * \par Description:
 *	The contructor will initialize the text view and set
 *	all properties to default values.
 *	See the documentaition of each Setxxx() members to see
 *	what the verious default values might be.
 *
 * \param cFrame
 *	Passed on to the os::View constructor.
 * \param pzTitle
 *	Passed on to the os::View constructor (only used to identify
 *	the view. Newer rendered anywhere).
 * \param pzText
 *	The initial content of the TextView or NULL if the TextView
 *	should be empty.
 * \param nResizeMask
 *	Passed on to the os::View constructor.
 * \param nFlags
 *	Passed on to the os::View constructor.
 * \sa os::View::View()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

TextView::TextView( const Rect & cFrame, const String &cTitle, const char *pzText, uint32 nResizeMask, uint32 nFlags ):Control( cFrame, cTitle, "", NULL, nResizeMask, nFlags )
{
	Rect cChildFrame = GetBounds();

	cChildFrame.Resize( 2, 2, -2, -2 );

	m_pcEditor = new TextEdit( this, cChildFrame, "edit_box", CF_FOLLOW_NONE );

	AddChild( m_pcEditor );

	if( pzText != NULL )
	{
		Set( pzText );
	}
}


TextView::~TextView()
{
}

void TextView::LabelChanged( const String &cNewLabel )
{
}

void TextView::EnableStatusChanged( bool bIsEnabled )
{
	m_pcEditor->SetEnable( bIsEnabled );
}

void TextView::Activated( bool bIsActive )
{
	m_pcEditor->Activated( bIsActive );
}

bool TextView::Invoked( Message * pcMessage )
{
	if( m_pcEditor->m_cPrevCursorPos != m_pcEditor->m_cCsrPos )
	{
		m_pcEditor->m_cPrevCursorPos = m_pcEditor->m_cCsrPos;
		m_pcEditor->m_nPendingEvents |= EI_CURSOR_MOVED;
	}
	if( ( m_pcEditor->m_nPendingEvents & m_pcEditor->m_nEventMask ) != 0 )
	{
		pcMessage->AddInt32( "events", m_pcEditor->m_nPendingEvents & m_pcEditor->m_nEventMask );

		// "final" is just for backward compatibility. Will be removed some day.
		bool bFinal = false;
		if( m_pcEditor->m_nPendingEvents & ( EI_ENTER_PRESSED | EI_FOCUS_LOST ) )
			bFinal = true;

		pcMessage->AddBool( "final", bFinal );
		return ( true );
	}
	else
	{
		return ( false );
	}
}

void TextView::SetValue( Variant cValue, bool bInvoke )
{
	if( PreValueChange( &cValue ) )
	{
		Set( cValue.AsString().c_str(  ) );
		if( bInvoke )
		{
			Invoke();
		}
		PostValueChange( cValue );
	}
}

Variant TextView::GetValue() const
{
	String cBuffer;

	m_pcEditor->GetRegion( IPoint( 0, 0 ), IPoint( -1, -1 ), &cBuffer, false );
	Variant cVariant( cBuffer );

	return ( cVariant );
}

void TextView::MouseMove( const Point & cNewPos, int nCode, uint32 nButtons, Message * pcData )
{
	//if( m_pcEditor->HandleMouseMove( m_pcEditor->ConvertFromParent( cNewPos ), nCode, nButtons, pcData ) == false )
	//{
		View::MouseMove( cNewPos, nCode, nButtons, pcData );
	//}
}

void TextView::MouseDown( const Point & cPosition, uint32 nButtons )
{
	//if( m_pcEditor->HandleMouseDown( m_pcEditor->ConvertFromParent( cPosition ), nButtons ) == false )
	//{
		View::MouseDown( cPosition, nButtons );
	//}
}

void TextView::MouseUp( const Point & cPosition, uint32 nButtons, Message * pcData )
{
	//if( m_pcEditor->HandleMouseUp( m_pcEditor->ConvertFromParent( cPosition ), nButtons, pcData ) == false )
	//{
		View::MouseUp( cPosition, nButtons, pcData );
	//}
}

void TextView::WheelMoved( const Point & cDelta )
{
	if( m_pcEditor->GetVScrollBar() != NULL )
	{
		m_pcEditor->GetVScrollBar()->WheelMoved( cDelta );
	}
	else
	{
		View::WheelMoved( cDelta );
	}
}

/** Get a pointer to the editor view.
 * \par Description:
 *	The os::TextView class is only responsible for rendering the borders
 *	and controlling the scroll-bars. Everything else is handled by a
 *	child view crated by the constructor. The os::TextView class provide
 *	an interface to all the functionality of the internal editor but
 *	in some rare cases it might be useful to have a pointer to this
 *	child view.
 *
 * \par Note:
 *	In erlier version (before V0.3.3) the only way to change the font
 *	used by the text editor was to retrieve the editor and then set
 *	it's font manually. This is not neccessarry (and not possible) any
 *	longer. Setting the font on the os::TextView class itself will
 *	automatically be relected by the internal editor.
 *
 * \return
 *	A const pointer to the internal editor.
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


const View *TextView::GetEditor() const
{
	return ( m_pcEditor );
}

void TextView::SetTabOrder( int nOrder )
{
	View::SetTabOrder( nOrder );
//    m_pcEditor->SetTabOrder( nOrder );
}

void TextView::FrameSized( const Point & cDelta )
{
	Rect cBounds = GetBounds();

	cBounds.Resize( 2, 2, -2, -2 );

	if( m_pcEditor->GetVScrollBar() != NULL )
	{
		Rect cSBFrame = m_pcEditor->GetVScrollBar()->GetFrame(  );

		cBounds.right -= ( cSBFrame.Width() + 1.0f ) - 2;
	}
	if( m_pcEditor->GetHScrollBar() != NULL )
	{
		Rect cSBFrame = m_pcEditor->GetHScrollBar()->GetFrame(  );

		cBounds.bottom -= ( cSBFrame.Height() + 1.0f ) - 2;
	}

	m_pcEditor->SetFrame( cBounds );
}

void TextView::FontChanged( Font * pcNewFont )
{
	m_pcEditor->SetFont( pcNewFont );
}

void TextView::Paint( const Rect & cUpdateRect )
{
	SetEraseColor( get_default_color( COL_NORMAL ) );
	DrawFrame( GetBounds(), FRAME_RECESSED );
}

void TextView::KeyDown( const char *pzString, const char *pzRawString, uint32 nQualifiers )
{
	if( m_pcEditor->HandleKeyDown( pzString, pzRawString, nQualifiers ) == false )
	{
		View::KeyDown( pzString, pzRawString, nQualifiers );
	}
}

/** Enable/disable the TextView.
 * \par Description:
 *	When disabled the TextView will not accept any user input and the
 *	text will be dark-gray on a light-gray background to indicate that
 *	that it is disabled.
 * \param bEnabled
 *	If true the view will be enabled if false it will be disabled.
 * \sa IsEnabled()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

/*
void TextView::SetEnable( bool bEnabled )
{
    m_pcEditor->SetEnable( bEnabled );
}
*/

/** Get the enabled state of the view
 * \return
 *	True if the view is enabled, false if the view is disabled.
 * \par Error codes:
 * \sa SetEnable()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

/*
bool TextView::IsEnabled() const
{
    return( m_pcEditor->IsEnabled() );
}
*/

/** Enable/disable multiline mode.
 * \par Description:
 *	The TextView have two quite different modes. Multiline and single line.
 *
 *	In singleline mode it will only allow a single line of text
 *	and it will send the EI_ENTER_PRESSED event (if not masked out) when
 *	the user press <ENTER> or <RETURN>. It will also always center the
 *	single line vertically inside the views boundary and it will never
 *	display scroll bars.
 *
 *	In multiline mode it will allow multiple lines of text and
 *	<ENTER> and <RETURN> will insert a new line instead of trigging
 *	the EI_ENTER_PRESSED event. The view will automatically add
 *	and remove vertical and horizontal scroll-bars as needed.
 *	
 * \param bMultiLine
 *	True to set the view in multiline mode or false to set it
 *	in singleline mode.
 *
 * \sa GetMultiLine(), SetEventMask(), SetPasswordMode()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void TextView::SetMultiLine( bool bMultiLine )
{
	if( m_pcEditor->GetMultiLine() == bMultiLine )
	{
		return;
	}
	m_pcEditor->SetMultiLine( bMultiLine );
	AdjustScrollbars();
}

/** Get the current editor mode.
 * \return
 *	True if the view is in multiline mode, false if it is in singleline
 *	mode.
 *
 * \sa SetMultiLine()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


bool TextView::GetMultiLine() const
{
	return ( m_pcEditor->GetMultiLine() );
}

/** Disable/enable password mode.
 * \par Description:
 *	When setting the view in password mode it will render all
 *	characters as asterics (*) to hide it's content and it will
 *	not allow the user to copy the content to the clipboard.
 *
 * \par Note:
 *	Only single-line TextView's should be set in password mode.
 * \param bPassword
 *	True to enable password mode, false to disable it.
 * \sa GetPasswordMode(), SetMultiLine()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void TextView::SetPasswordMode( bool bPassword )
{
	m_pcEditor->SetPasswordMode( bPassword );
}

/** Get the current echo mode.
 * \return
 *	True if the view is in password mode, false if not.
 * \sa SetPasswordMode()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

bool TextView::GetPasswordMode() const
{
	return ( m_pcEditor->GetPasswordMode() );
}

/** Set the view in "numeric" mode.
 * \par Description:
 *	When numeric mode is enabled the TextView will only accept
 *	valid numbers to be typed.
 *
 *	Digits are always accepted. Punctuation (.) is accepted once.
 *	"E" is accepted once. Plus and minus (+/-) is accepted as the
 *	first character of the line or as the first character after
 *	an "E".
 *
 *	If you have other restrictions on what should be accepted
 *	as input (for example hexadecimal numbers) you can overload
 *	the FilterKeyStroke() hook and filter the user input yourself.
 *	
 * \param bNumeric
 *	True to enable numeric mode, false to disable it.
 * \sa GetNumeric(), FilterKeyStroke()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void TextView::SetNumeric( bool bNumeric )
{
	m_pcEditor->SetNumeric( bNumeric );
}

/** Check if the view is in numeric mode.
 * \return
 *	True if numeric mode is enabled, false if it is disabled.
 * \sa SetNumeric()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

bool TextView::GetNumeric() const
{
	return ( m_pcEditor->GetNumeric() );
}

void TextView::SetReadOnly( bool bFlag )
{
	m_pcEditor->SetReadOnly( bFlag );
}

bool TextView::GetReadOnly() const
{
	return ( m_pcEditor->GetReadOnly() );
}

uint32 TextView::GetEventMask() const
{
	return ( m_pcEditor->GetEventMask() );
}
void TextView::SetEventMask( uint32 nMask )
{
	m_pcEditor->SetEventMask( nMask );
}

int TextView::GetMaxUndoSize() const
{
	return ( m_pcEditor->GetMaxUndoSize() );
}

void TextView::SetMaxUndoSize( int nSize )
{
	m_pcEditor->SetMaxUndoSize( nSize );
}

void TextView::SetMinPreferredSize( int nWidthChars, int nHeightChars )
{
	m_pcEditor->SetMinPreferredSize( nWidthChars, nHeightChars );
}

IPoint TextView::GetMinPreferredSize() const
{
	return ( m_pcEditor->GetMinPreferredSize() );
}

void TextView::SetMaxPreferredSize( int nWidthChars, int nHeightChars )
{
	m_pcEditor->SetMaxPreferredSize( nWidthChars, nHeightChars );
}

IPoint TextView::GetMaxPreferredSize() const
{
	return ( m_pcEditor->GetMaxPreferredSize() );
}

void TextView::GetRegion( String *pcBuffer ) const
{
	m_pcEditor->GetRegion( pcBuffer );
}

void TextView::MakeCsrVisible()
{
	m_pcEditor->MakeCsrVisible();
}

void TextView::Clear( bool bSendNotify )
{
	m_pcEditor->Clear();
	if( bSendNotify )
	{
		m_pcEditor->CommitEvents();
	}
}

void TextView::Set( const char *pzBuffer, bool bSendNotify )
{
	m_pcEditor->Clear();
	m_pcEditor->InsertString( NULL, pzBuffer, false );
	m_pcEditor->SetCursor( IPoint( 0, 0 ), false );
	if( bSendNotify )
	{
		m_pcEditor->CommitEvents();
	}
}

void TextView::Insert( const char *pzBuffer, bool bSendNotify )
{
	m_pcEditor->InsertString( NULL, pzBuffer );
	if( bSendNotify )
	{
		m_pcEditor->CommitEvents();
	}
}

void TextView::Insert( const IPoint & cPos, const char *pzBuffer, bool bSendNotify )
{
	m_pcEditor->SetCursor( cPos, false );
	m_pcEditor->InsertString( NULL, pzBuffer );
	if( bSendNotify )
	{
		m_pcEditor->CommitEvents();
	}
}


void TextView::Select( const IPoint & cStart, const IPoint & cEnd, bool bSendNotify )
{
	m_pcEditor->Select( cStart, cEnd );
	if( bSendNotify )
	{
		m_pcEditor->CommitEvents();
	}
}

void TextView::SelectAll( bool bSendNotify )
{
	m_pcEditor->SelectAll();
	if( bSendNotify )
	{
		m_pcEditor->CommitEvents();
	}
}

void TextView::ClearSelection( bool bSendNotify )
{
	m_pcEditor->ClearSelection();
	if( bSendNotify )
	{
		m_pcEditor->CommitEvents();
	}
}

bool TextView::GetSelection( IPoint * pcStart, IPoint * pcEnd ) const
{
	return ( m_pcEditor->GetSelection( pcStart, pcEnd ) );
}

void TextView::SetCursor( int x, int y, bool bSelect, bool bSendNotify )
{
	m_pcEditor->SetCursor( IPoint( x, y ), bSelect );
	if( bSendNotify )
	{
		m_pcEditor->CommitEvents();
	}
}

void TextView::SetCursor( const IPoint & cPos, bool bSelect, bool bSendNotify )
{
	m_pcEditor->SetCursor( cPos, bSelect );
	if( bSendNotify )
	{
		m_pcEditor->CommitEvents();
	}
}

void TextView::GetCursor( int *x, int *y ) const
{
	IPoint cPos = m_pcEditor->GetCursor();

	if( x != NULL )
	{
		*x = cPos.x;
	}
	if( y != NULL )
	{
		*y = cPos.y;
	}
}

IPoint TextView::GetCursor() const
{
	return ( m_pcEditor->GetCursor() );
}


void TextView::SetMaxLength( size_t nMaxLength )
{
	m_pcEditor->SetMaxLength( nMaxLength );
}

size_t TextView::GetMaxLength() const
{
	return ( m_pcEditor->GetMaxLength() );
}

size_t TextView::GetCurrentLength() const
{
	return ( m_pcEditor->GetCurrentLength() );
}




void TextView::Cut( bool bSendNotify )
{
	String cBuffer;

	m_pcEditor->GetRegion( &cBuffer );
	m_pcEditor->Delete();
	if( bSendNotify )
	{
		m_pcEditor->CommitEvents();
	}
}

void TextView::Copy()
{
	String cBuffer;

	m_pcEditor->GetRegion( &cBuffer );
}

void TextView::Paste( bool bSendNotify )
{
	const char *pzBuffer;
	int nError;
	Clipboard cClipboard;

	cClipboard.Lock();
	Message *pcData = cClipboard.GetData();

	nError = pcData->FindString( "text/plain", &pzBuffer );
	if( nError == 0 )
	{
		m_pcEditor->InsertString( NULL, pzBuffer );
		if( bSendNotify )
		{
			m_pcEditor->CommitEvents();
		}
	}
	cClipboard.Unlock();
}

void TextView::Delete( bool bSendNotify )
{
	m_pcEditor->Delete();
	if( bSendNotify )
	{
		m_pcEditor->CommitEvents();
	}
}

void TextView::Delete( const IPoint & cStart, const IPoint & cEnd, bool bSendNotify )
{
	m_pcEditor->Delete( cStart, cEnd );
	if( bSendNotify )
	{
		m_pcEditor->CommitEvents();
	}
}

const TextView::buffer_type &TextView::GetBuffer() const
{
	return ( m_pcEditor->GetBuffer() );
}

bool TextView::FilterKeyStroke( const String *pcString )
{
	return ( true );
}

Point TextView::GetPreferredSize( bool bLargest ) const
{
	return ( m_pcEditor->GetPreferredSize( bLargest ) );
}

void TextView::AdjustScrollbars( bool bDoScroll, float vDeltaX, float vDeltaY )
{
	bool bChanged = false;
	Rect cFrame = m_pcEditor->GetFrame();

	cFrame.Floor();
	Point cScrollOffset = m_pcEditor->GetScrollOffset() + Point( vDeltaX, vDeltaY );

	if( m_pcEditor->GetMultiLine() == false )
	{
		if( m_pcEditor->GetVScrollBar() != NULL )
		{
			ScrollBar *pcScrollBar = m_pcEditor->GetVScrollBar();
			Rect cSBFrame = pcScrollBar->GetFrame();

			RemoveChild( pcScrollBar );
			delete pcScrollBar;

			cFrame.right += ( cSBFrame.Width() + 1.0f ) - 2;
			bChanged = true;
		}
		if( m_pcEditor->GetHScrollBar() != NULL )
		{
			ScrollBar *pcScrollBar = m_pcEditor->GetHScrollBar();
			Rect cSBFrame = pcScrollBar->GetFrame();

			RemoveChild( pcScrollBar );
			delete pcScrollBar;

			cFrame.bottom += cSBFrame.Height() + 1.0f - 2.0f;
			bChanged = true;
		}
		if( bChanged )
		{
			m_pcEditor->ScrollTo( cScrollOffset.x, 0 );
			m_pcEditor->SetFrame( cFrame );
			MakeCsrVisible();
		}
		return;
	}
	Point cSize = m_pcEditor->m_cPreferredSize;

	if( ( cSize.y > cFrame.Height() + 1.0f ) && m_pcEditor->GetVScrollBar(  ) == NULL )
	{
		Rect cSBFrame = cFrame + Point( 2, 0 );

		cSBFrame.left = cSBFrame.right - 15;
		cSBFrame.Resize( 0, -2, 0, 2 );
		ScrollBar *pcScrollBar = new ScrollBar( cSBFrame, "v_scrollbar", NULL, 0, 0, VERTICAL, CF_FOLLOW_RIGHT | CF_FOLLOW_TOP | CF_FOLLOW_BOTTOM );

		AddChild( pcScrollBar );
		pcScrollBar->SetScrollTarget( m_pcEditor );
		pcScrollBar->SetSteps( m_pcEditor->m_vGlyphHeight, cFrame.Height() + 1.0f - m_pcEditor->m_vGlyphHeight );
		cFrame.right -= ( cSBFrame.Width() + 1.0f ) - 2;
		if( m_pcEditor->GetHScrollBar() != NULL )
		{
			m_pcEditor->GetHScrollBar()->ResizeBy( -( cSBFrame.Width(  ) + 1.0f ), 0 );
		}
		bChanged = true;
	}
	else if( ( cSize.y <= cFrame.Height() + 1.0f ) && m_pcEditor->GetVScrollBar(  ) != NULL )
	{
		ScrollBar *pcScrollBar = m_pcEditor->GetVScrollBar();
		Rect cSBFrame = pcScrollBar->GetFrame();

		RemoveChild( pcScrollBar );
		delete pcScrollBar;

		if( m_pcEditor->GetHScrollBar() != NULL )
		{
			m_pcEditor->GetHScrollBar()->ResizeBy( cSBFrame.Width(  ) + 1.0f, 0 );
		}
		m_pcEditor->ScrollTo( cScrollOffset.x, 0 );
		cScrollOffset.y = 0;
		bChanged = true;
		cFrame.right += cSBFrame.Width() + 1.0f - 2.0f;
	}

	if( ( cSize.x > cFrame.Width() + 1.0f ) && m_pcEditor->GetHScrollBar(  ) == NULL )
	{
		Rect cSBFrame = cFrame + Point( 0, 2 );

		cSBFrame.top = cSBFrame.bottom - 15;
		cSBFrame.Resize( -2, 0, 2, 0 );
		ScrollBar *pcScrollBar = new ScrollBar( cSBFrame, "h_scrollbar", NULL, 0, 0, HORIZONTAL, CF_FOLLOW_BOTTOM | CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT );

		pcScrollBar->SetSteps( m_pcEditor->m_vGlyphHeight, cFrame.Width() + 1.0f - m_pcEditor->m_vGlyphHeight );
		AddChild( pcScrollBar );
		pcScrollBar->SetScrollTarget( m_pcEditor );
		if( m_pcEditor->GetVScrollBar() != NULL )
		{
			m_pcEditor->GetVScrollBar()->ResizeBy( 0, -( cSBFrame.Height(  ) + 1.0f ) );
		}
		cFrame.bottom -= cSBFrame.Height() + 1.0f - 2.0f;
		bChanged = true;
	}
	else if( ( cSize.x <= cFrame.Width() + 1.0f ) && m_pcEditor->GetHScrollBar(  ) != NULL )
	{
		ScrollBar *pcScrollBar = m_pcEditor->GetHScrollBar();
		Rect cSBFrame = pcScrollBar->GetFrame();

		RemoveChild( pcScrollBar );
		delete pcScrollBar;

		if( m_pcEditor->GetVScrollBar() != NULL )
		{
			m_pcEditor->GetVScrollBar()->ResizeBy( 0, cSBFrame.Height(  ) + 1.0f );
		}
		m_pcEditor->ScrollTo( 0, cScrollOffset.y );
		cScrollOffset.x = 0;
		cFrame.bottom += cSBFrame.Height() + 1.0f - 2.0f;
		bChanged = true;
	}

	ScrollBar *pcScrollBar = m_pcEditor->GetVScrollBar();

	if( pcScrollBar != NULL )
	{
		float nMax = cSize.y - ( cFrame.Height() + 1.0f );

		pcScrollBar->SetMinMax( 0, nMax );
		float nOffset = -cScrollOffset.y;

		if( bDoScroll && nOffset > nMax )
		{
			nOffset = nMax;
		}
		pcScrollBar->SetProportion( ( cFrame.Height() + 1.0f ) / float ( cSize.y ) );

		pcScrollBar->SetValue( nOffset );
	}
	pcScrollBar = m_pcEditor->GetHScrollBar();
	if( pcScrollBar != NULL )
	{
		float nMax = cSize.x - ( cFrame.Width() + 1.0f );

		pcScrollBar->SetMinMax( 0, nMax );
		float nOffset = -cScrollOffset.x;

		if( bDoScroll && nOffset > nMax )
		{
			nOffset = nMax;
		}
		pcScrollBar->SetProportion( ( cFrame.Width() + 1.0f ) / float ( cSize.x ) );

		pcScrollBar->SetValue( nOffset );
	}
	if( bChanged )
	{
		m_pcEditor->SetFrame( cFrame );
	}
}

void TextView::Undo( void )
{
	m_pcEditor->Undo();
}

void TextView::Redo( void )
{
	m_pcEditor->Redo();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

TextEdit::TextEdit( TextView * pcParent, const Rect & cFrame, const String &cTitle, uint32 nResizeMask, uint32 nFlags ):View( cFrame, cTitle, nResizeMask, nFlags ), m_cCsrPos( 0, 0 ), m_cPrevCursorPos( 0, 0 ), m_cPreferredSize( LEFT_BORDER + RIGHT_BORDER, TOP_BORDER + BOTTOM_BORDER ), m_cMinPreferredSize( 0, 0 ), m_cMaxPreferredSize( 0, 0 ), m_sCurFgColor( 0, 0, 0 ), m_sCurBgColor( 255, 255, 255 )
{
	m_pcParent = pcParent;
	m_cBuffer.push_back( "" );
	m_cLineSizes.push_back( 0 );

	GetFontHeight( &m_sFontHeight );
	m_vGlyphHeight = m_sFontHeight.ascender + m_sFontHeight.descender + m_sFontHeight.line_gap;
	m_vCsrGfxPos = -1.0f;
	m_bRegionActive = false;
	m_bMultiLine = false;
	m_bPassword = false;
	m_bNumeric = false;
	m_bReadOnly = false;
	m_bEnforceBackBuffer = false;
	m_bMouseDownSeen = false;
	m_nMaxUndoSize = 10000;
	m_nUndoMemSize = 0;
	m_bEnabled = true;
	m_nEventMask = TextView::EI_CONTENT_CHANGED | TextView::EI_FOCUS_LOST | TextView::EI_ENTER_PRESSED | TextView::EI_WAS_EDITED;
	m_nPendingEvents = 0;
	m_nMaxLength = -1;
	m_nCurrentLength = 0;
	m_bIBeamActive = false;


	m_pcBackBuffer = NULL;
	m_pcBgView = new View( Rect(), "text_view_bg" );
	UpdateBackBuffer();
}


TextEdit::~TextEdit()
{
	if( m_bIBeamActive )
	{
		Application::GetInstance()->PopCursor(  );
	}

	while( m_cUndoStack.empty() == false )
	{
		delete m_cUndoStack.front();

		m_cUndoStack.erase( m_cUndoStack.begin() );
	}

	if( m_pcBackBuffer != NULL )
	{
		m_pcBackBuffer->RemoveChild( m_pcBgView );
		delete m_pcBackBuffer;

		m_pcBackBuffer = NULL;
	}
	delete( m_pcBgView );
}

void TextEdit::UpdateBackBuffer()
{
	Rect cBounds = GetBounds();
	int w = int ( ceil( cBounds.Width() ) ) + 1;
	int h;

	if( m_bMultiLine )
	{
		h = int ( ceil( m_vGlyphHeight ) ) * 3;
	}
	else
	{
		h = int ( ceil( cBounds.Height() ) ) + 1;
	}

	// If it gets this larg it is probably a bug and we disable
	// double buffereing to avoid running out of memory.
	if( w * h > 4000000 )
	{
		w = -1;
	}

	if( w <= 0 || h <= 0 )
	{
		if( m_pcBackBuffer != NULL )
		{
			m_pcBackBuffer->RemoveChild( m_pcBgView );
			delete m_pcBackBuffer;

			m_pcBackBuffer = NULL;
		}
	}
	else if( m_pcBackBuffer == NULL )
	{
		try
		{
			m_pcBackBuffer = new Bitmap( w, h, CS_RGB16, Bitmap::ACCEPT_VIEWS );
			m_pcBgView->SetFrame( m_pcBackBuffer->GetBounds() );
			m_pcBackBuffer->AddChild( m_pcBgView );
		}
		catch( ... )
		{
		}
	}
	else
	{
		Rect cOldBounds = m_pcBackBuffer->GetBounds();
		int ow = int ( cOldBounds.Width() + 1 );
		int oh = int ( cOldBounds.Height() + 1 );

		if( ow < w || ow > w * 2 || oh < h || oh > h * 2 )
		{
			m_pcBackBuffer->RemoveChild( m_pcBgView );
			delete m_pcBackBuffer;

			try
			{
				m_pcBackBuffer = new Bitmap( w, h, CS_RGB16, Bitmap::ACCEPT_VIEWS );
				m_pcBgView->SetFrame( m_pcBackBuffer->GetBounds() );
				m_pcBackBuffer->AddChild( m_pcBgView );
			}
			catch( ... )
			{
				m_pcBackBuffer = NULL;
			}
		}
	}
}

void TextEdit::SetEnable( bool bEnabled )
{
	if( bEnabled != m_bEnabled )
	{
		m_bEnabled = bEnabled;
		if( m_bEnabled )
		{
			m_sCurFgColor = Color32_s( 0, 0, 0 );
			m_sCurBgColor = Color32_s( 255, 255, 255 );
		}
		else
		{
			m_sCurFgColor = Color32_s( 100, 100, 100 );
			m_sCurBgColor = get_default_color( COL_NORMAL );
		}

		Invalidate( GetBounds() );
		//InvalidateLines( m_cCsrPos.y, m_cCsrPos.y );
		Flush();
	}
}

bool TextEdit::IsEnabled() const
{
	return ( m_bEnabled );
}

void TextEdit::SetMinPreferredSize( int nWidthChars, int nHeightChars )
{
	m_cMinPreferredSize = IPoint( nWidthChars, nHeightChars );
}

IPoint TextEdit::GetMinPreferredSize() const
{
	return ( m_cMinPreferredSize );
}

void TextEdit::SetMaxPreferredSize( int nWidthChars, int nHeightChars )
{
	m_cMaxPreferredSize = IPoint( nWidthChars, nHeightChars );
}

IPoint TextEdit::GetMaxPreferredSize() const
{
	return ( m_cMaxPreferredSize );
}

float TextEdit::GetPixelPosX( const String &cString, int nChar )
{
	if( nChar > int ( cString.size() ) )
	{
		nChar = cString.size();
	}
	float x = LEFT_BORDER;

	int nStart = 0;

	for( int i = 0; i <= nChar; ++i )
	{
		if( i == nChar || cString[i] == '\t' )
		{
			int nLen = i - nStart;

			if( m_bPassword )
			{
				x += GetStringWidth( "*", 1 ) * nLen;
			}
			else
			{
				x += GetStringWidth( cString.c_str() + nStart, nLen );
			}
			nStart = i + 1;
			if( i != nChar )
			{
				int nSkip = TAB_STOP - int ( x ) % TAB_STOP;

				if( nSkip < 2 )
				{
					nSkip = TAB_STOP;
				}
				x += nSkip;
			}
		}
	}
	return ( x );
}

float TextEdit::GetPixelPosY( int nLine )
{
	return ( m_vGlyphHeight * float ( nLine ) + TOP_BORDER );
}

int TextEdit::GetCharPosX( const String &cString, float vPos )
{
	int nStart = 0;
	float vWidth = 0.0f;
	int nLength = cString.size();

	for( int i = nStart; i <= nLength; ++i )
	{
		if( i == nLength || cString[i] == '\t' )
		{
			int nLen = i - nStart;

			float vCurWidth = GetStringWidth( cString.c_str() + nStart, nLen );

			if( vWidth + vCurWidth > vPos || i == nLength )
			{
				return ( nStart + GetStringLength( cString.c_str() + nStart, nLen, vPos - vWidth, false ) );
			}
			vWidth += vCurWidth;
			nStart = i + 1;
			if( cString[i] == '\t' )
			{
				float x = vWidth;
				int nSkip = TAB_STOP - int ( x ) % TAB_STOP;

				if( nSkip < 2 )
				{
					nSkip = TAB_STOP;
				}
				vWidth += nSkip;
			}
			if( vWidth > vPos || i == nLength )
			{
				return ( i );
			}

		}
	}
	return ( 0 );
}

Point TextEdit::GetPreferredSize( bool bLargest ) const
{
	if( bLargest )
	{
		if( m_cMaxPreferredSize != IPoint( 0, 0 ) )
		{
			return ( Point( ceil( m_cMaxPreferredSize.x * GetStringWidth( "M" ) + LEFT_BORDER + RIGHT_BORDER ), ceil( m_cMaxPreferredSize.y * m_vGlyphHeight + TOP_BORDER + BOTTOM_BORDER ) ) );
		}
		else
		{
			if( m_bMultiLine )
			{
				return ( Point( 10000.0f, 10000.0f ) );
			}
			else
			{
				return ( Point( 10000.0f, m_cBuffer.size() * m_vGlyphHeight + TOP_BORDER + BOTTOM_BORDER ) );
			}
		}
	}
	else
	{
		if( m_cMinPreferredSize != IPoint( 0, 0 ) )
		{
			return ( Point( ceil( m_cMinPreferredSize.x * GetStringWidth( "M" ) + LEFT_BORDER + RIGHT_BORDER ), ceil( m_cMinPreferredSize.y * m_vGlyphHeight + TOP_BORDER + BOTTOM_BORDER ) ) );
		}
		else
		{
			return Point( LEFT_BORDER + RIGHT_BORDER, m_vGlyphHeight + TOP_BORDER + BOTTOM_BORDER );
//                      return ( m_cPreferredSize );
		}
//      return( Point( GetStringWidth( m_cBuffer[0].c_str() ) + LEFT_BORDER + RIGHT_BORDER,
//                      m_cBuffer.size() * m_vGlyphHeight + TOP_BORDER + BOTTOM_BORDER ) );
	}
}

void TextEdit::FontChanged( Font * pcNewFont )
{
	m_pcBgView->SetFont( pcNewFont );
	m_bEnforceBackBuffer = true;
	GetFontHeight( &m_sFontHeight );

	m_vGlyphHeight = ceil( m_sFontHeight.ascender + m_sFontHeight.descender + m_sFontHeight.line_gap );

	UpdateBackBuffer();

	m_cPreferredSize.y = float ( m_cBuffer.size() ) * m_vGlyphHeight + TOP_BORDER + BOTTOM_BORDER;

	m_cPreferredSize.x = LEFT_BORDER + RIGHT_BORDER;
	for( uint i = 0; i < m_cBuffer.size(); ++i )
	{
		float vWidth = GetPixelPosX( m_cBuffer[i], m_cBuffer[i].size() ) + RIGHT_BORDER;

		m_cLineSizes[i] = vWidth;
		if( vWidth > m_cPreferredSize.x )
		{
			m_cPreferredSize.x = vWidth;
		}
	}

	m_pcParent->AdjustScrollbars();
	Invalidate();
	Flush();
}

void TextEdit::Activated( bool bIsActive )
{
	if( bIsActive == false )
	{
		m_nPendingEvents |= TextView::EI_FOCUS_LOST;
	}
	InvalidateLines( m_cCsrPos.y, m_cCsrPos.y );
	Flush();
	CommitEvents();
}

void TextEdit::FrameSized( const Point & cDelta )
{
	UpdateBackBuffer();
	MakeCsrVisible();
}

void TextEdit::SetPasswordMode( bool bPassword )
{
	m_bPassword = bPassword;
}

bool TextEdit::GetPasswordMode() const
{
	return ( m_bPassword );
}

void TextEdit::SetReadOnly( bool bFlag )
{
	m_bReadOnly = bFlag;
	UpdateBackBuffer();
}

bool TextEdit::GetReadOnly() const
{
	return ( m_bReadOnly );
}

uint32 TextEdit::GetEventMask() const
{
	return ( m_nEventMask );
}

void TextEdit::SetEventMask( uint32 nMask )
{
	m_nEventMask = nMask;
}

void TextEdit::Select( const IPoint & cStart, const IPoint & cEnd )
{
	int x1 = cStart.x;
	int y1 = cStart.y;
	int x2 = cEnd.x;
	int y2 = cEnd.y;

	if( m_bRegionActive && cStart == m_cRegionStart && cEnd == m_cRegionEnd )
	{
		return;
	}

	if( y2 < y1 )
	{
		int t = y2;

		y2 = y1;
		y1 = t;
		t = x2;
		x2 = x1;
		x1 = 1;
	}

	if( m_bRegionActive )
	{
		int ox1 = m_cRegionStart.x;
		int oy1 = m_cRegionStart.y;
		int ox2 = m_cRegionEnd.x;
		int oy2 = m_cRegionEnd.y;

		if( oy2 < oy1 )
		{
			int t = oy2;

			oy2 = oy1;
			oy1 = t;
			t = ox2;
			ox2 = ox1;
			ox1 = t;
		}

		m_cRegionStart = cStart;
		m_cRegionEnd = cEnd;
		if( y2 < oy1 || y1 > oy2 )
		{
			InvalidateLines( y1, y2 );
			InvalidateLines( oy1, oy2 );
		}
		else
		{
			if( y1 != oy1 || x1 != ox1 )
			{
				InvalidateLines( y1, oy1 );
			}
			if( y2 != oy2 || x2 != ox2 )
			{
				InvalidateLines( y2, oy2 );
			}
//          InvalidateLines( (y1 < oy1) ? y1 : oy1, (y2 > oy2) ? y2 : oy2 );
		}
	}
	else
	{
		m_cRegionStart = cStart;
		m_cRegionEnd = cEnd;
		InvalidateLines( y1, y2 );
		m_bRegionActive = true;
	}
	m_nPendingEvents |= TextView::EI_SELECTION_CHANGED;
}

void TextEdit::SelectAll()
{
	Select( IPoint( 0, 0 ), IPoint( m_cBuffer[m_cBuffer.size() - 1].size(  ), m_cBuffer.size(  ) - 1 ) );
}

void TextEdit::ClearSelection()
{
	if( m_bRegionActive )
	{
		m_bRegionActive = false;
		InvalidateLines( m_cRegionStart.y, m_cRegionEnd.y );
		Flush();
		m_nPendingEvents |= TextView::EI_SELECTION_CHANGED;
	}
}

bool TextEdit::GetSelection( IPoint * pcStart, IPoint * pcEnd ) const
{
	if( m_bRegionActive )
	{
		if( pcStart != NULL )
		{
			*pcStart = m_cRegionStart;
		}
		if( pcEnd != NULL )
		{
			*pcEnd = m_cRegionEnd;
		}
	}
	return ( m_bRegionActive );
}

void TextEdit::SetCursor( const IPoint & cPos, bool bSelect )
{
	int x = cPos.x;
	int y = cPos.y;

	if( m_bMultiLine == false )
	{
		y = 0;
	}

	if( y == -1 )
	{
		y = m_cBuffer.size() - 1;
	}
	else if( y < 0 )
	{
		y = 0;
	}
	else if( y >= int ( m_cBuffer.size() ) )
	{
		y = m_cBuffer.size() - 1;
	}
	if( x == -1 )
	{
		x = m_cBuffer[y].size();
	}
	else if( x > int ( m_cBuffer[y].size() ) )
	{
		x = m_cBuffer[y].size();
	}
	if( x < 0 )
	{
		x = 0;
	}

	if( x == m_cCsrPos.x && y == m_cCsrPos.y )
	{
		if( bSelect == false )
		{
			ClearSelection();
		}
		return;
	}

	IPoint cOldPos = m_cCsrPos;

	InvalidateLines( m_cCsrPos.y, m_cCsrPos.y );
	m_cCsrPos.y = y;
	m_cCsrPos.x = x;
	InvalidateLines( m_cCsrPos.y, m_cCsrPos.y );
	MakeCsrVisible();

	if( bSelect )
	{
		Select( ( m_bRegionActive ) ? m_cRegionStart : cOldPos, IPoint( x, y ) );
	}
	else
	{
		ClearSelection();
	}
	Flush();
	m_nPendingEvents |= TextView::EI_CURSOR_MOVED;
}

IPoint TextEdit::GetCursor() const
{
	return ( m_cCsrPos );
}

void TextEdit::SetMaxLength( size_t nMaxLength )
{
	m_nMaxLength = nMaxLength;
}

size_t TextEdit::GetMaxLength() const
{
	return ( m_nMaxLength );
}

size_t TextEdit::GetCurrentLength() const
{
	return ( m_nCurrentLength );
}

void TextEdit::InsertChar( char nChar )
{
	char zStr[2] = { nChar, '\0' };
	InsertString( NULL, zStr );
}

void TextEdit::RecalcPrefWidth()
{
	m_cPreferredSize.x = 0;
	for( uint i = 0; i < m_cBuffer.size(); ++i )
	{
		float nWidth = m_cLineSizes[i];

		if( nWidth > m_cPreferredSize.x )
		{
			m_cPreferredSize.x = nWidth;
		}
	}
	m_cPreferredSize.x += LEFT_BORDER + RIGHT_BORDER;
}

void TextEdit::InsertString( IPoint * pcPos, const char *pzBuffer, bool bMakeUndo )
{
	if( m_nMaxLength != -1 && m_nCurrentLength >= m_nMaxLength )
	{
		return;
	}
	if( pcPos == NULL )
	{
		pcPos = &m_cCsrPos;
	}
	int nStart = 0;
	int nLine = 0;
	IPoint cStartPos = *pcPos;
	bool bLinesInserted = false;

	int nLength;

	for( nLength = 0; pzBuffer[nLength] != '\0'; nLength += utf8_char_length( pzBuffer[nLength] ) )
	{
		if( m_nMaxLength != -1 && m_nCurrentLength >= m_nMaxLength )
		{
			break;
		}
		m_nCurrentLength++;
	}

	assert( m_cBuffer.size() > 0 );
	assert( m_cBuffer.size() == m_cLineSizes.size(  ) );

	String cRest;

	for( int i = 0; i <= nLength; ++i )
	{
		if( pzBuffer[i] == '\n' || i == nLength )
		{
			String cLine( pzBuffer + nStart, i - nStart );

			if( nLine == 0 )
			{
				if( uint ( pcPos->x ) > m_cBuffer[pcPos->y].size() )
				{
					dbprintf( "Error: Attemt to insert at %dx%d at %d long line\n", pcPos->x, pcPos->y, m_cBuffer[pcPos->y].size() );
				}
				assert( uint ( pcPos->x ) <= m_cBuffer[pcPos->y].size() );

				cRest = m_cBuffer[pcPos->y].c_str() + pcPos->x;
				float vOldLen = m_cLineSizes[pcPos->y];	// GetStringWidth( m_cBuffer[pcPos->y].c_str() ) + LEFT_BORDER + RIGHT_BORDER;

				m_cBuffer[pcPos->y].resize( pcPos->x );
				m_cBuffer[pcPos->y] += cLine;
				float vNewLen = GetPixelPosX( m_cBuffer[pcPos->y], m_cBuffer[pcPos->y].size() ) + RIGHT_BORDER;

				// GetStringWidth( m_cBuffer[pcPos->y].c_str() ) + LEFT_BORDER + RIGHT_BORDER;
				m_cLineSizes[pcPos->y] = vNewLen;
				if( vNewLen > vOldLen )
				{
					if( vNewLen > m_cPreferredSize.x )
					{
						m_cPreferredSize.x = vNewLen;
					}
				}
				else
				{
					if( vOldLen >= m_cPreferredSize.x )
					{
						RecalcPrefWidth();
					}
				}
				pcPos->x = cLine.size();
			}
			else
			{
				m_cBuffer.insert( m_cBuffer.begin() + pcPos->y + nLine, cLine );
				pcPos->x = cLine.size();
				float vWidth = GetPixelPosX( cLine, cLine.size() ) + RIGHT_BORDER;

				// GetStringWidth( cLine.c_str() );
				m_cLineSizes.insert( m_cLineSizes.begin() + pcPos->y + nLine, vWidth );
				if( vWidth > m_cPreferredSize.x )
				{
					m_cPreferredSize.x = vWidth;
				}
				m_cPreferredSize.y += m_vGlyphHeight;
				bLinesInserted = true;
			}
			nStart = i + 1;
			nLine++;
		}
	}
	pcPos->x = m_cBuffer[pcPos->y + nLine - 1].size();
	if( cRest.size() > 0 )
	{
		m_cBuffer[pcPos->y + nLine - 1] += cRest;
		float vWidth = GetPixelPosX( m_cBuffer[pcPos->y + nLine - 1], m_cBuffer[pcPos->y + nLine - 1].size() ) + RIGHT_BORDER;

		// GetStringWidth( m_cBuffer[pcPos->y + nLine - 1].c_str() );
		m_cLineSizes[pcPos->y + nLine - 1] = vWidth;
		if( vWidth > m_cPreferredSize.x )
		{
			m_cPreferredSize.x = vWidth;
		}

	}
	pcPos->y += nLine - 1;

	if( bMakeUndo )
	{
		UndoNode *psNode = new UndoNode( UNDO_INSERT );

		psNode->m_cPos = cStartPos;
		psNode->m_cEndPos = *pcPos;
		psNode->m_cText = pzBuffer;
		AddUndoNode( psNode );
	}

	MakeCsrVisible();
	Rect cInvRect = GetBounds();

	if( m_bMultiLine )
	{
		cInvRect.top = TOP_BORDER + cStartPos.y * m_vGlyphHeight;
		if( bLinesInserted == false )
		{
			cInvRect.bottom = cInvRect.top + m_vGlyphHeight;
			if( cStartPos.x > 0 )
				cStartPos.x--;
			cInvRect.left = GetPixelPosX( m_cBuffer[cStartPos.y], cStartPos.x );
		}
	}
	Invalidate( cInvRect );
	Flush();
	m_vCsrGfxPos = -1.0f;
	m_nPendingEvents |= TextView::EI_CONTENT_CHANGED;
	if( m_nMaxLength != -1 && m_nCurrentLength >= m_nMaxLength )
	{
		m_nPendingEvents |= TextView::EI_MAX_SIZE_REACHED;
	}
}

void TextEdit::GetRegion( IPoint cStart, IPoint cEnd, String *pcBuffer, bool bAddToClipboard ) const
{
	IPoint cRegStart;
	IPoint cRegEnd;

	if( cStart < cEnd )
	{
		cRegStart = cStart;
		cRegEnd = cEnd;
	}
	else
	{
		cRegStart = cEnd;
		cRegEnd = cStart;
	}

	if( cRegStart.y == cRegEnd.y )
	{
		*pcBuffer = String ( m_cBuffer[cRegStart.y].begin() + cRegStart.x, m_cBuffer[cRegStart.y].begin(  ) + cRegEnd.x );
	}
	else if( cStart == IPoint( 0, 0 ) && cEnd == IPoint( -1, -1 ) )
	{
		TextView::buffer_type::const_iterator iBfr = m_cBuffer.begin();

		*pcBuffer = String ( "" );
		bool first = true;

		while( iBfr != m_cBuffer.end() )
		{
			if( !first )
			{
				*pcBuffer += "\n";
			}
			*pcBuffer += *iBfr;
			first = false;
			iBfr++;
		}
	}
	else
	{
		*pcBuffer = String ( m_cBuffer[cRegStart.y].begin() + cRegStart.x, m_cBuffer[cRegStart.y].end(  ) );

		for( int i = cRegStart.y + 1; i < cRegEnd.y; ++i )
		{
			*pcBuffer += "\n";
			*pcBuffer += m_cBuffer[i];
		}
		*pcBuffer += "\n";
		*pcBuffer += String ( m_cBuffer[cRegEnd.y].begin(), m_cBuffer[cRegEnd.y].begin(  ) + cRegEnd.x );
	}
	if( m_bPassword == false && bAddToClipboard )
	{
		Clipboard cClipboard;

		cClipboard.Lock();
		cClipboard.Clear();
		Message *pcData = cClipboard.GetData();

		pcData->AddString( "text/plain", *pcBuffer );
		cClipboard.Commit();
		cClipboard.Unlock();
	}
}

void TextEdit::GetRegion( String *pcBuffer, bool bAddToClipboard ) const
{
	GetRegion( m_cRegionStart, m_cRegionEnd, pcBuffer, bAddToClipboard );
}

void TextEdit::Clear()
{
	m_bRegionActive = false;
	SetCursor( IPoint( 0, 0 ), false );
	ScrollTo( 0, 0 );
	m_cBuffer.clear();
	m_cBuffer.push_back( String () );

	m_cLineSizes.clear();
	m_cLineSizes.push_back( 0 );

	while( m_cUndoStack.empty() == false )
	{
		delete m_cUndoStack.front();

		m_cUndoStack.erase( m_cUndoStack.begin() );
	}
	m_nUndoMemSize = 0;

	m_cPreferredSize.x = LEFT_BORDER + RIGHT_BORDER;
	m_cPreferredSize.y = TOP_BORDER + LEFT_BORDER + m_vGlyphHeight;
	Invalidate();
	m_pcParent->AdjustScrollbars();

	if( m_nMaxLength != -1 && m_nCurrentLength >= m_nMaxLength )
	{
		m_nPendingEvents |= TextView::EI_MAX_SIZE_LEFT;
	}
	m_nCurrentLength = 0;

	m_nPendingEvents |= TextView::EI_CONTENT_CHANGED;
}

void TextEdit::SetMaxUndoSize( int nSize )
{
	m_nMaxUndoSize = nSize;

	while( ( m_nMaxUndoSize == 0 && m_cUndoStack.size() > 0 ) || m_cUndoStack.size(  ) > 5 && m_nUndoMemSize > m_nMaxUndoSize )
	{
		UndoNode *psNode = m_cUndoStack.back();

		m_nUndoMemSize -= sizeof( UndoNode ) + psNode->m_cText.size();
		delete psNode;

		m_cUndoStack.erase( --m_cUndoStack.end() );
	}

}

void TextEdit::AddUndoNode( UndoNode * psNode )
{
	if( m_nMaxUndoSize == 0 )
	{
		return;
	}

	while( m_cUndoStack.size() != 0 && ( m_iUndoIterator ) != m_cUndoStack.begin(  ) )
	{
		UndoNode *psTmpNode = m_cUndoStack.front();

		m_nUndoMemSize -= sizeof( UndoNode ) + psTmpNode->m_cText.size();
		delete psTmpNode;

		m_cUndoStack.erase( m_cUndoStack.begin() );
	}

	m_nUndoMemSize += sizeof( UndoNode ) + psNode->m_cText.size();
	m_cUndoStack.push_front( psNode );

	if( m_cUndoStack.size() > 5 && m_nUndoMemSize > m_nMaxUndoSize )
	{
		psNode = m_cUndoStack.back();
		m_nUndoMemSize -= sizeof( UndoNode ) + psNode->m_cText.size();
		delete psNode;

		m_cUndoStack.erase( --m_cUndoStack.end() );
	}

	m_iUndoIterator = m_cUndoStack.begin();
}

void TextEdit::Undo()
{
	if( m_cUndoStack.size() == 0 || m_iUndoIterator == m_cUndoStack.end(  ) )
	{
//      dbprintf("UNDO: END\n");
		return;
	}
	m_bRegionActive = false;

//    UndoNode* psNode = m_cUndoStack.front();
//    m_cUndoStack.erase( m_cUndoStack.begin() );
//    m_nUndoMemSize -= sizeof( UndoNode ) + psNode->m_cText.size();
	UndoNode *psNode = *( m_iUndoIterator++ );

	switch ( psNode->m_nMode )
	{
	case UNDO_DELETE:
		m_cCsrPos = psNode->m_cPos;
		InsertString( NULL, psNode->m_cText.c_str(), false );
//          dbprintf("UNDO: UNDO_DELETE %s\n", psNode->m_cText.c_str() );
		break;
	case UNDO_INSERT:
		Delete( psNode->m_cPos, psNode->m_cEndPos, false );
//          dbprintf("UNDO: UNDO_INSERT %s\n", psNode->m_cText.c_str() );
		break;
	}
}

void TextEdit::Redo()
{
	if( m_cUndoStack.size() == 0 || m_iUndoIterator == m_cUndoStack.begin(  ) )
	{
//      dbprintf("REDO: END\n");
		return;
	}
	m_bRegionActive = false;

	UndoNode *psNode = *( --m_iUndoIterator );

	switch ( psNode->m_nMode )
	{
	case UNDO_INSERT:
		m_cCsrPos = psNode->m_cPos;
		InsertString( NULL, psNode->m_cText.c_str(), false );
//          dbprintf("REDO: UNDO_INSERT %s\n", psNode->m_cText.c_str() );
		break;
	case UNDO_DELETE:
		Delete( psNode->m_cPos, psNode->m_cEndPos, false );
//          dbprintf("REDO: UNDO_DELETE %s %d %d\n", psNode->m_cText.c_str(), psNode->m_cPos.x, psNode->m_cEndPos.x );
		break;
	}
}

void TextEdit::Delete( IPoint cStart, IPoint cEnd, bool bMakeUndo )
{
	IPoint cRegStart;
	IPoint cRegEnd;

	int nOldLen = m_nCurrentLength;

	if( cStart < cEnd )
	{
		cRegStart = cStart;
		cRegEnd = cEnd;
	}
	else
	{
		cRegStart = cEnd;
		cRegEnd = cStart;
	}

	if( cRegStart.y == -1 )
	{
		cRegStart.y = m_cBuffer.size() - 1;
	}
	else if( cRegStart.y < 0 )
	{
		cRegStart.y = 0;
	}
	else if( cRegStart.y >= int ( m_cBuffer.size() ) )
	{
		cRegStart.y = m_cBuffer.size() - 1;
	}
	if( cRegStart.x == -1 )
	{
		cRegStart.x = m_cBuffer[cRegStart.y].size();
	}
	else if( cRegStart.x > int ( m_cBuffer[cRegStart.y].size() ) )
	{
		cRegStart.x = m_cBuffer[cRegStart.y].size();
	}
	if( cRegStart.x < 0 )
	{
		cRegStart.x = 0;
	}

	if( cRegEnd.y == -1 )
	{
		cRegEnd.y = m_cBuffer.size() - 1;
	}
	else if( cRegEnd.y < 0 )
	{
		cRegEnd.y = 0;
	}
	else if( cRegEnd.y >= int ( m_cBuffer.size() ) )
	{
		cRegEnd.y = m_cBuffer.size() - 1;
	}
	if( cRegEnd.x == -1 )
	{
		cRegEnd.x = m_cBuffer[cRegEnd.y].size();
	}
	else if( cRegEnd.x > int ( m_cBuffer[cRegEnd.y].size() ) )
	{
		cRegEnd.x = m_cBuffer[cRegEnd.y].size();
	}
	if( cRegEnd.x < 0 )
	{
		cRegEnd.x = 0;
	}
	if( cRegStart == cRegEnd )
	{
		return;
	}
	if( bMakeUndo )
	{
		UndoNode *psUndoNode = new UndoNode( UNDO_DELETE );

		GetRegion( cRegStart, cRegEnd, &psUndoNode->m_cText, false );
		psUndoNode->m_cPos = cRegStart;
		psUndoNode->m_cEndPos = cRegEnd;
		AddUndoNode( psUndoNode );
	}

	if( cRegStart != m_cCsrPos )
	{
		m_cCsrPos = cRegStart;
	}

	if( cRegStart.y == cRegEnd.y )
	{
		int nLen = cRegEnd.x - cRegStart.x;

		if( cRegStart.x + nLen > int ( m_cBuffer[cRegStart.y].size() ) )
		{
			nLen = m_cBuffer[cRegStart.y].size() - cRegStart.x;
		}
		if( nLen > 0 )
		{
			m_nCurrentLength -= String ( m_cBuffer[cRegStart.y].begin() + cRegStart.x, m_cBuffer[cRegStart.y].begin(  ) + cRegStart.x + nLen ).CountChars(  );

			m_cBuffer[cRegStart.y].erase( cRegStart.x, nLen );
			m_cLineSizes[cRegStart.y] = GetPixelPosX( m_cBuffer[cRegStart.y], m_cBuffer[cRegStart.y].size() ) + RIGHT_BORDER;
		}
	}
	else
	{
		int nOldLineLen = m_cBuffer[cRegStart.y].CountChars();

		m_cBuffer[cRegStart.y].resize( cRegStart.x );
		m_cBuffer[cRegStart.y] += m_cBuffer[cRegEnd.y].c_str() + cRegEnd.x;

		m_nCurrentLength += m_cBuffer[cRegStart.y].CountChars() - nOldLineLen;

		m_cLineSizes[cRegStart.y] = GetPixelPosX( m_cBuffer[cRegStart.y], m_cBuffer[cRegStart.y].size() ) + RIGHT_BORDER;
		for( int i = cRegStart.y + 1; i <= cRegEnd.y; ++i )
		{
			m_nCurrentLength -= m_cBuffer[i].CountChars() + 1;
		}
		m_cBuffer.erase( m_cBuffer.begin() + cRegStart.y + 1, m_cBuffer.begin(  ) + cRegEnd.y + 1 );
		m_cLineSizes.erase( m_cLineSizes.begin() + cRegStart.y + 1, m_cLineSizes.begin(  ) + cRegEnd.y + 1 );
		Invalidate();
	}
	InvalidateLines( m_cCsrPos.y, m_cCsrPos.y );

	RecalcPrefWidth();
	m_cPreferredSize.y = m_cBuffer.size() * m_vGlyphHeight + TOP_BORDER + BOTTOM_BORDER;
	MakeCsrVisible();
	Flush();

	if( m_nMaxLength != -1 && nOldLen >= m_nMaxLength && m_nCurrentLength < m_nMaxLength )
	{
		m_nPendingEvents |= TextView::EI_MAX_SIZE_LEFT;
	}
	m_nPendingEvents |= TextView::EI_CONTENT_CHANGED;
}

void TextEdit::Delete()
{
	if( m_bRegionActive )
	{
		m_bRegionActive = false;
		Delete( m_cRegionStart, m_cRegionEnd );
	}
	else
	{
		if( m_cCsrPos.y == int ( m_cBuffer.size() - 1 ) && m_cCsrPos.x >= int ( m_cBuffer[m_cCsrPos.y].size(  ) ) )
		{
			return;
		}
		IPoint cEnd = m_cCsrPos;

		if( cEnd.x == int ( m_cBuffer[cEnd.y].size() ) )
		{
			cEnd.x = 0;
			cEnd.y++;
		}
		else
		{
			cEnd.x += utf8_char_length( m_cBuffer[m_cCsrPos.y][m_cCsrPos.x] );
		}
		Delete( m_cCsrPos, cEnd );
	}
}

void TextEdit::MakeCsrVisible()
{
	Rect cBounds = GetBounds();

	float x = GetPixelPosX( m_cBuffer[m_cCsrPos.y], m_cCsrPos.x );
	float y = m_cCsrPos.y * m_vGlyphHeight + TOP_BORDER;

	float nDeltaX = 0;
	float nDeltaY = 0;

	if( x < cBounds.left + LEFT_BORDER )
	{
		nDeltaX = ( cBounds.left + LEFT_BORDER ) - x;
	}
	else if( x + m_vGlyphHeight - 1 > cBounds.right )
	{
		nDeltaX = ( cBounds.right - RIGHT_BORDER ) - x - m_vGlyphHeight;
	}
	if( m_bMultiLine )
	{
		if( y < cBounds.top + TOP_BORDER )
		{
			nDeltaY = ( cBounds.top + TOP_BORDER ) - y;
		}
		else if( y + m_vGlyphHeight - 1 > cBounds.bottom )
		{
			nDeltaY = ( cBounds.bottom - BOTTOM_BORDER ) - y - m_vGlyphHeight;
		}
	}

/*    Point cOffset = GetScrollOffset();
    Rect cFrame = GetFrame();
    if ( -(cOffset.x + nDeltaX) < 0 ) {
	nDeltaX = -cOffset.x;
    } else if ( -(cOffset.x + nDeltaX) > */
//    if ( nDeltaX != 0 || nDeltaY != 0 ) {
	m_pcParent->AdjustScrollbars( false, nDeltaX, nDeltaY );
//      ScrollBy( nDeltaX, nDeltaY );
//    }
//    m_pcParent->AdjustScrollbars();
}

void TextEdit::DrawCursor( View * pcView, float vHOffset, float vVOffset )
{
	if( pcView == NULL )
	{
		pcView = this;
	}
	if( m_bReadOnly == false && ( HasFocus() || m_pcParent->HasFocus(  ) ) && m_bEnabled )
	{
		if( m_bRegionActive == false || ( m_cCsrPos.y == m_cRegionEnd.y && m_cRegionEnd.x == 0 ) )
		{
			Rect cBounds = GetBounds();

/*	    float nTop = m_vGlyphHeight * m_cCsrPos.y + vVOffset;
	    if ( m_bMultiLine )  nTop += TOP_BORDER;
	    
	    float nBottom = nTop + m_vGlyphHeight - 1;
	    float nCsrPos = GetPixelPosX( m_cBuffer[m_cCsrPos.y], m_cCsrPos.x ) + vHOffset;

	    if ( m_bMultiLine == false ) {
		float nOffset = (cBounds.Height()+1.0f) / 2.0f - m_vGlyphHeight / 2.0f;
		nTop    += nOffset;
		nBottom += nOffset;
	    }*/
			float vTop = m_vGlyphHeight * m_cCsrPos.y + TOP_BORDER + vVOffset;
			float vBottom = vTop + m_vGlyphHeight - 1;

			if( m_bMultiLine == false )
			{

				vTop = ceil( ( cBounds.Height() + 1.0f ) * 0.5f - ( m_sFontHeight.ascender + m_sFontHeight.descender ) * 0.5f /*- m_sFontHeight.ascender*/  );
				vBottom = vTop + m_vGlyphHeight - 1.0f;
			}
			float vCsrPos = GetPixelPosX( m_cBuffer[m_cCsrPos.y], m_cCsrPos.x ) + vHOffset;

			pcView->SetFgColor( m_sCurFgColor );
			pcView->SetDrawingMode( DM_INVERT );
			pcView->DrawLine( Point( vCsrPos, vTop ), Point( vCsrPos, vBottom ) );
			vCsrPos += 1.0f;
			pcView->DrawLine( Point( vCsrPos, vTop ), Point( vCsrPos, vBottom ) );
			pcView->SetDrawingMode( DM_COPY );
		}
	}
}

void TextEdit::MaybeDrawString( View * pcView, float vHOffset, const char *pzString, int nLength )
{
	if( m_bPassword )
	{
		while( nLength > 0 )
		{
			int nCurLen = std::min( nLength, 32 );

			pcView->DrawString( "********************************", nCurLen );
			nLength -= nCurLen;
		}
	}
	else
	{
		int nStart = 0;

		if( pzString[0] == '\t' )
		{
			pcView->Sync();
			float x = pcView->GetPenPosition().x - vHOffset;
			int nSkip = TAB_STOP - int ( x ) % TAB_STOP;

			if( nSkip < 2 )
			{
				nSkip = TAB_STOP;
			}
			pcView->MovePenBy( nSkip, 0 );
			nStart = 1;
		}

		for( int i = nStart; i <= nLength; ++i )
		{
			if( i == nLength || pzString[i] == '\t' )
			{
				int nLen = i - nStart;

				pcView->DrawString( pzString + nStart, nLen );
				nStart = i + 1;
				if( i != nLength && pzString[i] == '\t' )
				{
					pcView->Sync();
					float x = pcView->GetPenPosition().x - vHOffset;
					int nSkip = TAB_STOP - int ( x ) % TAB_STOP;

					if( nSkip < 2 )
					{
						nSkip = TAB_STOP;
					}
					pcView->MovePenBy( nSkip, 0 );
				}
			}
		}
	}
}

void TextEdit::RenderLine( View * pcView, int y, float vHOffset, float vVOffset, bool bClear )
{
	Rect cBounds = GetBounds();

	float vTop = m_vGlyphHeight * y + TOP_BORDER + vVOffset;
	float vBottom = vTop + m_vGlyphHeight - 1;

	if( m_bMultiLine == false )
	{

		vTop = ceil( ( cBounds.Height() + 1.0f ) * 0.5f - ( m_sFontHeight.ascender + m_sFontHeight.descender ) * 0.5f /*- m_sFontHeight.ascender*/  );
		vBottom = vTop + m_vGlyphHeight - 1.0f;

		pcView->SetFgColor( m_sCurBgColor );
		pcView->FillRect( Rect( cBounds.left + vHOffset, cBounds.top, cBounds.right + vHOffset, cBounds.top + vTop - 1.0f ) );
		pcView->FillRect( Rect( cBounds.left + vHOffset, cBounds.top + vBottom + 1, cBounds.right + vHOffset, cBounds.bottom ) );
	}

	bool bRegionActive = false;
	IPoint cRegStart;
	IPoint cRegEnd;
	int l1 = 0, l2 = 0, l3 = 0;
	int nStart = 0;
	int nEnd = 0;

	if( m_bRegionActive )
	{
		if( m_cRegionStart < m_cRegionEnd )
		{
			cRegStart = m_cRegionStart;
			cRegEnd = m_cRegionEnd;
		}
		else
		{
			cRegStart = m_cRegionEnd;
			cRegEnd = m_cRegionStart;
		}
	}
	if( m_bRegionActive && y >= cRegStart.y && y <= cRegEnd.y )
	{
		bRegionActive = true;
		if( y == cRegStart.y )
		{
			nStart = cRegStart.x;
		}
		else
		{
			nStart = 0;
		}
		if( y == cRegEnd.y )
		{
			nEnd = cRegEnd.x;
		}
		else
		{
			nEnd = m_cBuffer[y].size();
		}
		l1 = nStart;
		l2 = nEnd - nStart;
		if( nStart + l2 > int ( m_cBuffer[y].size() ) )
		{
			l2 = m_cBuffer[y].size() - nStart;
		}
		if( int ( m_cBuffer[y].size() ) > nEnd )
		{
			l3 = m_cBuffer[y].size() - nEnd;
		}
		else
		{
			l3 = 0;
		}
	}

	if( bRegionActive )
	{
		float x = LEFT_BORDER;

		if( l1 > 0 )
		{
			pcView->SetFgColor( m_sCurBgColor );
			float nTmp = GetPixelPosX( m_cBuffer[y], l1 );

			if( bClear )
			{
				pcView->FillRect( Rect( x + vHOffset, vTop, nTmp + vHOffset, vBottom ) );
			}
			x = nTmp;
		}
		if( l2 > 0 )
		{
			pcView->SetFgColor( m_sCurFgColor );
			float nTmp = GetPixelPosX( m_cBuffer[y], l1 + l2 );

			pcView->FillRect( Rect( x + vHOffset, vTop, nTmp + vHOffset, vBottom ) );
			x = nTmp;
		}
		pcView->SetFgColor( m_sCurBgColor );
		if( bClear )
		{
			pcView->FillRect( Rect( x + vHOffset, vTop, cBounds.right + vHOffset, vBottom ) );
		}
	}
	else
	{
		if( bClear )
		{
			pcView->SetFgColor( m_sCurBgColor );
			pcView->FillRect( Rect( cBounds.left + vHOffset, vTop, cBounds.right + vHOffset, vBottom ) );
		}
	}
	if( y >= 0 && y < int ( m_cBuffer.size() ) && m_cBuffer[y].empty(  ) == false )
	{
		pcView->MovePenTo( LEFT_BORDER + vHOffset, floor( vTop + m_sFontHeight.ascender + m_sFontHeight.line_gap - 1.0f ) );

		if( bRegionActive )
		{
			if( l1 > 0 )
			{
				pcView->SetBgColor( m_sCurBgColor );
				pcView->SetFgColor( m_sCurFgColor );
				MaybeDrawString( pcView, vHOffset, m_cBuffer[y].c_str(), l1 );
			}
			if( l2 > 0 )
			{
				pcView->SetBgColor( m_sCurFgColor );
				pcView->SetFgColor( m_sCurBgColor );
				MaybeDrawString( pcView, vHOffset, m_cBuffer[y].c_str() + nStart, l2 );
			}
			if( l3 > 0 )
			{
				pcView->SetBgColor( m_sCurBgColor );
				pcView->SetFgColor( m_sCurFgColor );
				MaybeDrawString( pcView, vHOffset, m_cBuffer[y].c_str() + nEnd, l3 );
			}
		}
		else
		{
			pcView->SetBgColor( m_sCurBgColor );
			pcView->SetFgColor( m_sCurFgColor );
			MaybeDrawString( pcView, vHOffset, m_cBuffer[y].c_str(), m_cBuffer[y].size(  ) );
		}
	}
	if( y == m_cCsrPos.y )
	{
		DrawCursor( pcView, vHOffset, vVOffset );
	}
}

void TextEdit::Paint( const Rect & cUpdateRect )
{
	if( m_bMultiLine )
	{
		int nTop = int ( ( cUpdateRect.top - TOP_BORDER ) / m_vGlyphHeight );
		int nBottom = int ( ( ( cUpdateRect.bottom - TOP_BORDER ) + m_vGlyphHeight - 1 ) / m_vGlyphHeight );

		SetFgColor( m_sCurBgColor );
		FillRect( Rect( cUpdateRect.left, 0, cUpdateRect.right, TOP_BORDER - 1 ) );
		FillRect( Rect( 0, cUpdateRect.top, LEFT_BORDER - 1, cUpdateRect.bottom ) );


		Rect cRect = cUpdateRect;

		cRect.top = ( nTop * m_vGlyphHeight ) + TOP_BORDER;
		cRect.bottom = cRect.top - 1.0f;

		if( ( m_bEnforceBackBuffer == false && nBottom - nTop > 3 ) || m_pcBackBuffer == NULL )
		{
			for( int i = nTop; i <= nBottom; ++i )
			{
				RenderLine( this, i, 0.0f, 0.0f );
			}
		}
		else
		{
			for( int i = nTop; i <= nBottom; ++i )
			{
				if( cRect.bottom - cRect.top + m_vGlyphHeight > m_pcBackBuffer->GetBounds().bottom )
				{
					m_pcBgView->Sync();
					DrawBitmap( m_pcBackBuffer, cRect.Bounds(), cRect );
					Sync();
					cRect.top = i * m_vGlyphHeight + TOP_BORDER;
					cRect.bottom = cRect.top - 1.0f;
				}
				RenderLine( m_pcBgView, i, -cRect.left, -cRect.top );
				cRect.bottom += m_vGlyphHeight;
			}
			if( cRect.bottom > cRect.top )
			{
				m_pcBgView->Sync();
				DrawBitmap( m_pcBackBuffer, cRect.Bounds(), cRect );
				Sync();
			}
		}
	}
	else
	{
		if( m_pcBackBuffer == NULL )
		{
			RenderLine( this, 0, 0.0f, 0.0f );
		}
		else
		{
			Rect cRect = cUpdateRect;
			float vHOffset;
 
 			cRect.top = 0.0f;

			vHOffset = GetStringWidth( m_cBuffer[0].substr( 0, m_cCsrPos.x).c_str() );
			vHOffset -= ( m_pcBgView->Width() + RIGHT_BORDER );

			if( vHOffset < 0.0f )
				vHOffset = 0.0f;
			else
			{
				m_pcBgView->SetFgColor( m_sCurBgColor );
				m_pcBgView->FillRect( m_pcBgView->GetBounds() );
			}

			RenderLine( m_pcBgView, 0, -vHOffset, 0.0f );
			m_pcBgView->Sync();
			DrawBitmap( m_pcBackBuffer, cRect, cRect );
			Sync();
		}
	}
	m_bEnforceBackBuffer = false;
}

void TextEdit::MoveHoriz( int nDelta, bool bExpBlock )
{
	IPoint cCsrPos = m_cCsrPos;

	cCsrPos.x += nDelta;
	if( cCsrPos.x < 0 )
	{
		if( m_bMultiLine && cCsrPos.y > 0 )
		{
			cCsrPos.y--;
			cCsrPos.x = m_cBuffer[cCsrPos.y].size();
		}
		else
		{
			cCsrPos.x = 0;
		}
	}
	else if( cCsrPos.x > int ( m_cBuffer[cCsrPos.y].size() ) )
	{
		if( m_bMultiLine && cCsrPos.y < int ( m_cBuffer.size() ) - 1 )
		{
			cCsrPos.y++;
			cCsrPos.x = 0;
		}
		else
		{
			cCsrPos.x = m_cBuffer[cCsrPos.y].size();
		}
	}
	SetCursor( cCsrPos, bExpBlock );
}

void TextEdit::MoveVert( int nDelta, bool bExpBlock )
{
	if( m_bMultiLine == false )
	{
		return;
	}
	if( nDelta < 0 )
	{
		if( m_cCsrPos.y + nDelta < 0 )
		{
			nDelta = -m_cCsrPos.y;
		}
	}
	else
	{
		if( m_cCsrPos.y + nDelta >= int ( m_cBuffer.size() ) )
		{
			nDelta = m_cBuffer.size() - m_cCsrPos.y - 1;
		}
	}
	if( nDelta != 0 )
	{
		if( m_vCsrGfxPos < 0 )
		{
			m_vCsrGfxPos = GetPixelPosX( m_cBuffer[m_cCsrPos.y], m_cCsrPos.x ) + 1.0f;
		}
		int x = GetCharPosX( m_cBuffer[m_cCsrPos.y + nDelta], m_vCsrGfxPos );

		SetCursor( IPoint( x, m_cCsrPos.y + nDelta ), bExpBlock );
	}
}

/* TextEdit::MoveWord: move the cursor to the previous/next word.
 *  bDirection: true=move to next word (to the right), false=move to prev word (to the left)
 *  bExpBlock: if true, extend the selection block
 * 
 *  Anthony Morphett, awmorp@gmail.com
 */
void TextEdit::MoveWord( bool bDirection, bool bExpBlock )
{
	IPoint cNewPos = m_cCsrPos;
	int nLineLength = m_cBuffer[cNewPos.y].size();
	int nTotalLines = m_cBuffer.size();
	
	if( bDirection )  /* Moving right */
	{
		/* first move to the end of the word (if the cursor is already sitting inside a word) */
		while( isalnum( m_cBuffer[cNewPos.y][cNewPos.x] ) )  /* while the character to the right of cursor is part of a word */
		{
			if( cNewPos.x < nLineLength )
			{  /* move to next char */
				cNewPos.x++;
			}
			else
			{  /* wrap over to next line */
				if( cNewPos.y == nTotalLines-1 ) { goto done; }
				else { cNewPos.x = 0; cNewPos.y++; nLineLength = m_cBuffer[cNewPos.y].size(); }
			}
		}
		
		/* if cursor currently sitting on non-word character, move to start of next word character */
		while( !isalnum( m_cBuffer[cNewPos.y][cNewPos.x] ) )  /* while the character to the right of cursor is not part of a word */
		{
			if( cNewPos.x < nLineLength )
			{  /* move to next char */
				cNewPos.x++;
			}
			else
			{  /* at end of line; wrap over to next line */
				if( cNewPos.y == nTotalLines-1 ) { goto done; }  /* at end of data; can't go further */
				else { cNewPos.x = 0; cNewPos.y++; nLineLength = m_cBuffer[cNewPos.y].size(); }
			}
		}
	}
	else  /* Moving left */
	{
		/* if cursor is currently sitting in some whitespace, first move to the end of prev word */
		while( !( cNewPos.x > 0 ? isalnum( m_cBuffer[cNewPos.y][cNewPos.x-1] ) : ( cNewPos.y > 0 ? isalnum( m_cBuffer[cNewPos.y-1][(m_cBuffer[cNewPos.y-1].size() > 0 ? m_cBuffer[cNewPos.y-1].size()-1:0)] ) : false ) ) )
		{  /* this ugly-looking expression is asking if the character immediately left of cursor is part of a word (ie alphanumeric); taking into account linewrapping and possibly being at pos (0,0) */
			if( cNewPos.x > 0 )
			{  /* move to prev char on line */
				cNewPos.x--;
			}
			else
			{  /* at start of line; wrap to previous line */
				if( cNewPos.y == 0 ) { goto done; }  /* at beginning of textarea; can't go any further */
				else { cNewPos.y--; nLineLength = m_cBuffer[cNewPos.y].size(); cNewPos.x = nLineLength; }
			}
		}

		/* now move to the start of the word */
		while( ( cNewPos.x > 0 ? isalnum( m_cBuffer[cNewPos.y][cNewPos.x-1] ) : ( cNewPos.y > 0 ? isalnum( m_cBuffer[cNewPos.y-1][(m_cBuffer[cNewPos.y-1].size() > 0 ? m_cBuffer[cNewPos.y-1].size()-1:0)] ) : false ) ) )
		{
			if( cNewPos.x > 0 )
			{  /* move to prev char on line */
				cNewPos.x--;
			}
			else
			{  /* at start of line; wrap to previous line */
				if( cNewPos.y == 0 ) { goto done; }  /* at beginning of textarea; can't go any further */
				else { cNewPos.y--; nLineLength = m_cBuffer[cNewPos.y].size(); cNewPos.x = nLineLength; }
			}
		}
	}
	
	done:
	if( cNewPos != m_cCsrPos ) { SetCursor( cNewPos, bExpBlock ); }
}

void TextEdit::MouseMove( const Point & cPosition, int nCode, uint32 nButtons, Message * pcData )
{
	Rect	cBounds( GetBounds() );
	
	if( nCode == MOUSE_ENTERED )
	{
		if( !m_bIBeamActive ) {
			Application::GetInstance()->PushCursor( MPTR_MONO, g_anMouseImg, POINTER_WIDTH, POINTER_HEIGHT, IPoint( POINTER_WIDTH / 2, POINTER_HEIGHT / 2 ) );
			m_bIBeamActive = true;
		}
	}
	else if( nCode == MOUSE_EXITED )
	{
		if( m_bIBeamActive ) {
			m_bIBeamActive = false;
			Application::GetInstance()->PopCursor(  );
		}
	}

	if( m_bEnabled == false || m_bMouseDownSeen == false )
	{
		return os::View::MouseMove( cPosition, nCode, nButtons,pcData );
	}
	if( ( nButtons & 0x01 ) == 0 )
	{
		return os::View::MouseMove( cPosition, nCode, nButtons,pcData );
	}

	if( nButtons & 0x0001 )
	{
		IPoint cCharPos;
		cCharPos.y = int ( ( cPosition.y - TOP_BORDER ) / m_vGlyphHeight );

		if( cCharPos.y < 0 )
		{
			cCharPos.y = 0;
		}
		else if( cCharPos.y >= int ( m_cBuffer.size() ) )
		{
			cCharPos.y = m_cBuffer.size() - 1;
		}
		cCharPos.x = GetCharPosX( m_cBuffer[cCharPos.y], cPosition.x + 1 );	// GetStringLength( m_cBuffer[cCharPos.y], cPosition.x + 1, false );

		SetCursor( cCharPos, true );
	}
	CommitEvents();
}

void TextEdit::MouseDown( const Point & cPosition, uint32 nButton )
{
	if( nButton != 1 || m_bEnabled == false )
	{
		return os::View::MouseDown( cPosition, nButton );
	}
	m_pcParent->MakeFocus( true );

	IPoint cCsrPos;

	cCsrPos.y = int ( ( cPosition.y - TOP_BORDER ) / m_vGlyphHeight );

	if( cCsrPos.y < 0 )
	{
		cCsrPos.y = 0;
	}
	else if( cCsrPos.y >= int ( m_cBuffer.size() ) )
	{
		cCsrPos.y = m_cBuffer.size() - 1;
	}

	cCsrPos.x = GetCharPosX( m_cBuffer[cCsrPos.y], cPosition.x + 1 );	// GetStringLength( m_cBuffer[m_cCsrPos.y], cPosition.x + 1, false );


	SetCursor( cCsrPos, false );

	Flush();
	CommitEvents();
	m_bMouseDownSeen = true;
}

void TextEdit::MouseUp( const Point & cPosition, uint32 nButtons, Message * pcData )
{
	if( m_bRegionActive && m_cRegionStart == m_cRegionEnd )
	{
		m_bRegionActive = false;
		m_nPendingEvents |= TextView::EI_SELECTION_CHANGED;
	}
	CommitEvents();
	m_bMouseDownSeen = false;
	if( pcData != NULL )
		return os::View::MouseUp( cPosition, nButtons, pcData );
}


bool TextEdit::HandleKeyDown( const char *pzString, const char *pzRawString, uint32 nQualifiers )
{
	bool bShift = nQualifiers & QUAL_SHIFT;
	bool bAlt = nQualifiers & QUAL_ALT;
	bool bCtrl = nQualifiers & QUAL_CTRL;
	bool bDead = nQualifiers & QUAL_DEADKEY;
	
	if( m_bEnabled == false )
	{
		return false;
	}

	if( bCtrl && !bAlt && !bShift )
	{			// If CTRL is held down
		if( pzString[0] == VK_INSERT )
		{
			String cBuffer;

			GetRegion( &cBuffer );
			return ( true );
		}
		else if( strlen( pzRawString ) == 1 )
		{
			switch ( tolower( pzRawString[0] ) )
			{
			case 'x':
				if( !m_bReadOnly )
				{
					String cBuffer;

					GetRegion( &cBuffer );
					Delete();
					CommitEvents();
					return true;
				}
				return false;
			case 'v':
				if( !m_bReadOnly )
				{
					const char *pzBuffer;
					int nError;
					Clipboard cClipboard;

					cClipboard.Lock();
					Message *pcData = cClipboard.GetData();

					nError = pcData->FindString( "text/plain", &pzBuffer );
					if( nError == 0 )
					{
						if( m_bRegionActive )
						{
							Delete();
						}
						InsertString( NULL, pzBuffer );
					}
					cClipboard.Unlock();
					CommitEvents();
					return true;
				}
				return false;
			case 'c':
				{
					String cBuffer;

					GetRegion( &cBuffer );
				}
				return true;
			case 'y':
				Redo();
				return true;
			case 'z':
				Undo();
				return true;
			}
		}
	}

	if( !bCtrl && !bAlt && bShift )
	{			// If SHIFT is held down
		if( pzString[0] == VK_INSERT )
		{
			if( !m_bReadOnly )
			{
				const char *pzBuffer;
				int nError;
				Clipboard cClipboard;

				cClipboard.Lock();
				Message *pcData = cClipboard.GetData();

				nError = pcData->FindString( "text/plain", &pzBuffer );
				if( nError == 0 )
				{
					if( m_bRegionActive )
					{
						Delete();
					}
					InsertString( NULL, pzBuffer );
				}
				cClipboard.Unlock();
				CommitEvents();
				return true;
			}
		}
	}

	if( m_bReadOnly )
		return false;

	switch ( pzString[0] )
	{
	case 0:
		break;
	case VK_ESCAPE:
		m_nPendingEvents |= TextView::EI_ESC_PRESSED;
		MakeFocus( false );
		break;
	case VK_ENTER:
		if( m_bMultiLine )
		{
			String cStr( pzString );

			if( m_pcParent->FilterKeyStroke( &cStr ) )
			{
				if( m_bRegionActive )
				{
					Delete();
				}
				InsertString( NULL, cStr.c_str() );
				m_vCsrGfxPos = -1.0f;
			}
		}
		else
		{
			m_nPendingEvents |= TextView::EI_ENTER_PRESSED;
			MakeFocus( false );
		}
		break;
	case VK_LEFT_ARROW:
		if( bCtrl )
		{
			MoveWord( false, bShift );
			m_vCsrGfxPos = -1.0f; /* what is this for? */
		}
		else
		{
			int nDelta = -1;

			while( m_cCsrPos.x + nDelta > 0 )
			{
//              if ( (m_cBuffer[m_cCsrPos.y][m_cCsrPos.x+nDelta] & 0x80) == 0 || (m_cBuffer[m_cCsrPos.y][m_cCsrPos.x+nDelta] & 0xc0) == 0xc0 ) {
				if( is_first_utf8_byte( m_cBuffer[m_cCsrPos.y][m_cCsrPos.x + nDelta] ) )
				{
					break;
				}
				nDelta--;
			}
			MoveHoriz( nDelta, bShift );
			m_vCsrGfxPos = -1.0f;
		}
		break;
	case VK_RIGHT_ARROW:
		if( bCtrl )
		{ /* move cursor to next word */
			MoveWord( true, bShift );
			m_vCsrGfxPos = -1.0f;  /* what is this for? */
		}
		else
		{ /* move cursor right one character */
			MoveHoriz( utf8_char_length( m_cBuffer[m_cCsrPos.y][m_cCsrPos.x] ), bShift );
			m_vCsrGfxPos = -1.0f;
		}
		break;
	case VK_UP_ARROW:
		MoveVert( -1, bShift );
		break;
	case VK_DOWN_ARROW:
		MoveVert( 1, bShift );
		break;
	case VK_PAGE_UP:
		MoveVert( ( int )-( ( GetBounds().Height(  ) + 1.0f ) / m_vGlyphHeight - 1 ), bShift );
		break;
	case VK_PAGE_DOWN:
		MoveVert( ( int )( ( GetBounds().Height(  ) + 1.0f ) / m_vGlyphHeight - 1 ), bShift );
		break;
	case VK_HOME:
		if( bCtrl ) {
			SetCursor( IPoint( 0, 0 ), bShift );
			m_vCsrGfxPos = -1.0f;
		} else {
		MoveHoriz( -m_cCsrPos.x, bShift );
		m_vCsrGfxPos = -1.0f;
		}
		break;
	case VK_END:
		if( bCtrl ) {
			SetCursor( IPoint( -1, -1 ), bShift );
			m_vCsrGfxPos = -1.0f;
		} else {
		MoveHoriz( m_cBuffer[m_cCsrPos.y].size() - m_cCsrPos.x, bShift );
		m_vCsrGfxPos = -1.0f;
		}
		break;
	case VK_BACKSPACE:
		{
			if( nQualifiers & QUAL_LALT )
			{
				Undo();
				break;
			}
			if( m_bRegionActive )  /* if a region is selected, delete it (same as pressing delete) */
			{
				Delete();
				m_vCsrGfxPos = -1.0f;
				break;
			}
			if( m_cCsrPos.x == 0 && m_cCsrPos.y == 0 )
			{
				break;
			}
			if( m_cCsrPos.x == 0 )
			{
				m_cCsrPos.x = m_cBuffer[m_cCsrPos.y - 1].size();
				m_cCsrPos.y--;
			}
			else
			{
				int nDelta = -1;

				while( m_cCsrPos.x + nDelta > 0 )
				{
					if( ( m_cBuffer[m_cCsrPos.y][m_cCsrPos.x + nDelta] & 0x80 ) == 0 || ( m_cBuffer[m_cCsrPos.y][m_cCsrPos.x + nDelta] & 0xc0 ) == 0xc0 )
					{
						break;
					}
					nDelta--;
				}
				m_cCsrPos.x += nDelta;
			}
			Delete();
			m_vCsrGfxPos = -1.0f;
			break;
		}
	case VK_DELETE:
		if( nQualifiers & QUAL_SHIFT )
		{
			String cBuffer;

			GetRegion( &cBuffer );
		}
		Delete();
		m_vCsrGfxPos = -1.0f;
		break;
	default:
		String cStr( pzString );

		if( m_pcParent->FilterKeyStroke( &cStr ) )
		{
			if( ( m_bMultiLine || m_bNumeric == false ) || isdigit( cStr[0] ) || ( cStr[0] == '.' && int ( m_cBuffer[0].str().find( '.' ) ) == -1 )||( toupper( cStr[0] ) == 'E' && int ( m_cBuffer[0].str(  ).find( 'e' ) ) == -1 && int ( m_cBuffer[0].str(  ).find( 'E' ) ) == -1 )||( ( m_cCsrPos.x == 0 || toupper( m_cBuffer[0][m_cCsrPos.x - 1] ) == 'E' ) && ( cStr[0] == '+' || cStr[0] == '-' ) ) )
			{
				if( m_bRegionActive )
				{
					Delete();
				}
				InsertString( NULL, cStr.c_str() );
				m_vCsrGfxPos = -1.0f;
			}
		}
		break;
	}

	if( bDead )
	{
		/* Note: this is a bit of hack. It marks the entered character, if it's a deadkey, */
		/* so that it will be overwritten by the next char. */
		int nDelta = -1;

		while( m_cCsrPos.x + nDelta > 0 )
		{
			if( is_first_utf8_byte( m_cBuffer[m_cCsrPos.y][m_cCsrPos.x + nDelta] ) )
			{
				break;
			}
			nDelta--;
		}
		MoveHoriz( nDelta, false );
		MoveHoriz( utf8_char_length( m_cBuffer[m_cCsrPos.y][m_cCsrPos.x] ), true );
		m_vCsrGfxPos = -1.0f;
	}

	CommitEvents();
	return ( true );
}

/*
static String get_flag_str( uint32 nFlags )
{
    String cStr;
    if ( nFlags & TextView::EI_CONTENT_CHANGED ) {
	cStr += "EI_CONTENT_CHANGED, ";
    }
    if ( nFlags & TextView::EI_ENTER_PRESSED ) {
	cStr += "EI_ENTER_PRESSED, ";
    }
    if ( nFlags & TextView::EI_FOCUS_LOST ) {
	cStr += "EI_FOCUS_LOST, ";
    }
    if ( nFlags & TextView::EI_CURSOR_MOVED ) {
	cStr += "EI_CURSOR_MOVED, ";
    }
    if ( nFlags & TextView::EI_SELECTION_CHANGED ) {
	cStr += "EI_SELECTION_CHANGED, ";
    }
    if ( nFlags & TextView::EI_MAX_SIZE_REACHED ) {
	cStr += "EI_MAX_SIZE_REACHED, ";
    }
    if ( nFlags & TextView::EI_MAX_SIZE_LEFT ) {
	cStr += "EI_MAX_SIZE_LEFT, ";
    }
    if ( cStr.size() > 2 ) {
	cStr.resize( cStr.size() - 2 );
    }
    return( cStr );
}
*/
void TextEdit::CommitEvents()
{
	if( m_cPrevCursorPos != m_cCsrPos )
	{
		m_cPrevCursorPos = m_cCsrPos;
		m_nPendingEvents |= TextView::EI_CURSOR_MOVED;
	}

	if( ( m_nPendingEvents & m_nEventMask ) != 0 /*&& m_pcParent->GetMessage() != NULL */  )
	{
		m_pcParent->Invoke();
		//printf( "Commit: %s\n", get_flag_str( m_nPendingEvents ).c_str() );
	}

	// If EI_CONTENT_CHANGED was set, ensure EI_WAS_EDITED also becomes set
	uint32 nPreviousEvents = m_nPendingEvents;
	m_nPendingEvents = 0;
	if( nPreviousEvents & TextView::EI_CONTENT_CHANGED )
		nPreviousEvents |= TextView::EI_WAS_EDITED;

	// EI_WAS_EDITED will persist until either EI_ENTER_PRESSED or EI_FOCUS_LOST are also generated
	if( ( nPreviousEvents & TextView::EI_WAS_EDITED ) && !( nPreviousEvents & ( TextView::EI_ENTER_PRESSED | TextView::EI_FOCUS_LOST ) ) )
			m_nPendingEvents |= TextView::EI_WAS_EDITED;
}


void TextEdit::InvalidateLines( int nFirst, int nLast )
{
	if( m_bMultiLine )
	{
		if( nLast < nFirst )
		{
			int t = nLast;

			nLast = nFirst;
			nFirst = t;
		}
		Rect cRect = GetBounds();
		cRect.top = float ( nFirst ) * m_vGlyphHeight + TOP_BORDER;
		cRect.bottom = float ( nLast + 1 ) * m_vGlyphHeight - 1.0f + TOP_BORDER;

		Invalidate( cRect );
	}
	else if( nFirst == 0 || nLast == 0 )
	{
		Invalidate();
	}
}

/** \internal */
void TextView::__TV_reserved1__()
{
}

/** \internal */
void TextView::__TV_reserved2__()
{
}

/** \internal */
void TextView::__TV_reserved3__()
{
}

/** \internal */
void TextView::__TV_reserved4__()
{
}

/** \internal */
void TextView::__TV_reserved5__()
{
}

