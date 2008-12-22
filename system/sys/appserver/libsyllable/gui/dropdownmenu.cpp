/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 - 2004 Syllable Team
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

#include <gui/dropdownmenu.h>
#include <gui/textview.h>
#include <gui/bitmap.h>
#include <gui/window.h>
#include <util/message.h>
#include <util/shortcutkey.h>

using namespace os;

static uint8 g_anArrow[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
	0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
	0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff
};


#define ARROW_WIDTH  10
#define ARROW_HEIGHT 5

#define MAX_DD_HEIGHT 12
#define EV_SB_INVOKED 31337

static Bitmap *g_pcArrows = NULL;

class DropdownMenu::DropdownView : public View
{
public:
	DropdownView( DropdownMenu* pcParent );

	virtual void	Paint( const Rect& cUpdateRect );
	virtual void	MouseDown( const Point& cPosition, uint32 nButtons );
	virtual void	MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData );
	virtual void	MouseMove( const Point& cPosition, int nCode, uint32 nButtons, Message* pcData );
	virtual void	KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );
	virtual void	AllAttached();
	virtual void	Activated( bool bIsActive );
	virtual void	WheelMoved( const Point& cDelta );
	virtual void	FontChanged( Font* pcNewFont );

    virtual void	HandleMessage( Message* pcMessage );

private:
	void _Layout();
	
	friend class DropdownMenu;
	DropdownMenu* m_pcParent;
	Point	      m_cContentSize;
	font_height   m_sFontHeight;
	float	      m_vGlyphHeight;
	int	      m_nOldSelection;
	int	      m_nCurSelection;
    int		m_nScrollPos;
    ScrollBar *m_pcScrollBar;
    Point m_cOldPosition;
    
    bool m_bIsAttached;
};

class DropdownMenu::DropdownTextView : public os::TextView
{
public:
	DropdownTextView(DropdownMenu * pcParent, const Rect &cFrame, const char *pzTitle, const char *pzBuffer, uint32 nResizeMask=CF_FOLLOW_LEFT|CF_FOLLOW_TOP, uint32 nFlags=WID_WILL_DRAW|WID_FULL_UPDATE_ON_RESIZE);
	~DropdownTextView() { };
	void KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );
private:
	DropdownMenu *m_pcParent;
};


DropdownMenu::DropdownTextView::DropdownTextView( DropdownMenu * pcParent, const Rect & cFrame,
	const char *pzTitle, const char *pzBuffer, uint32 nResizeMask, uint32 nFlags )
:os::TextView( cFrame, pzTitle, pzBuffer, nResizeMask, nFlags )
{
	m_pcParent = pcParent;
}

void DropdownMenu::DropdownTextView::KeyDown( const char *pzString, const char *pzRawString, uint32 nQualifiers )
{
	switch ( pzString[0] )
	{
	case VK_DOWN_ARROW:
		m_pcParent->OpenMenu();
		break;

	default:
		TextView::KeyDown( pzString, pzRawString, nQualifiers );
		break;
	}

}


/** DropdownMenu constructor
 * \param cFrame - The size and position of the edit box and it's associated button.
 * \param pzName - The identifier passed down to the Handler class (Never rendered)
 * \param nResizeMask - Flags deskribing which edge follows edges of the parent
 *			when the parent is resized (Se View::View())
 * \param nFlags - Various flags passed to the View::View() constructor. 
 * \sa os::View, os::Invoker
 * \author	Kurt Skauen (kurt@atheos.cx)
    *//////////////////////////////////////////////////////////////////////////////

DropdownMenu::DropdownMenu( const Rect & cFrame, const String& cName, uint32 nResizeMask, uint32 nFlags ):View( cFrame, cName, nResizeMask, nFlags )
{
	m_nSelection = -1;
	m_bMenuOpen = false;
	m_bSendIntermediateMsg = false;

	m_pcEditMsg = NULL;
	m_pcSelectionMsg = NULL;
	m_pcMenuWindow = NULL;
	m_bIsEnabled = true;

	m_pcEditBox = new DropdownTextView( this, Rect( 0, 0, 1, 1 ), "text_view", NULL );
	AddChild( m_pcEditBox );

	FrameSized( Point( 0, 0 ) );

	m_pcEditBox->SetMessage( new Message( ID_STRING_CHANGED ) );

	if( g_pcArrows == NULL )
	{
		g_pcArrows = new Bitmap( ARROW_WIDTH, ARROW_HEIGHT * 3, CS_CMAP8, Bitmap::SHARE_FRAMEBUFFER );

		uint8 *pRaster = g_pcArrows->LockRaster();

		for( int y = 0; y < ARROW_HEIGHT; ++y )
		{
			for( int x = 0; x < ARROW_WIDTH; ++x )
			{
				*pRaster++ = g_anArrow[y * ARROW_WIDTH + x];
			}
			pRaster += g_pcArrows->GetBytesPerRow() - ARROW_WIDTH;
		}
		for( int y = 0; y < ARROW_HEIGHT; ++y )
		{
			for( int x = 0; x < ARROW_WIDTH; ++x )
			{
				*pRaster++ = ( g_anArrow[y * ARROW_WIDTH + x] == 0xff ) ? 0xff : 63;
			}
			pRaster += g_pcArrows->GetBytesPerRow() - ARROW_WIDTH;
		}
		for( int y = 0; y < ARROW_HEIGHT; ++y )
		{
			for( int x = 0; x < ARROW_WIDTH; ++x )
			{
				*pRaster++ = ( g_anArrow[y * ARROW_WIDTH + x] == 0xff ) ? 0xff : 14;
			}
			pRaster += g_pcArrows->GetBytesPerRow() - ARROW_WIDTH;
		}
	}
}

DropdownMenu::~DropdownMenu()
{
	if( m_bMenuOpen && m_pcMenuWindow ) {
		m_pcMenuWindow->PostMessage( M_QUIT );
	}
	delete m_pcEditMsg;
	delete m_pcSelectionMsg;
}


void DropdownMenu::SetEnable( bool bEnable )
{
	if( bEnable == m_bIsEnabled )
	{
		return;
	}
	m_bIsEnabled = bEnable;
	m_pcEditBox->SetEnable( bEnable );
	Invalidate();
	Flush();
}

bool DropdownMenu::IsEnabled() const
{
	return ( m_bIsEnabled );
}

/** Change the "read-only" status.
 * \par Description:
 *	When the DropdownMenu is set in read-only mode the user will not be able
 *	to edit the contents of the edit box. It will also make the menu open
 *	when the user click inside the edit box.
 * \param bFlag - Is true the menu will be read-only, if false it will be read/write.
 * \sa GetReadOnly()
 * \author	Kurt Skauen (kurt@atheos.cx)
    *//////////////////////////////////////////////////////////////////////////////


void DropdownMenu::SetReadOnly( bool bFlag )
{
	m_pcEditBox->SetReadOnly();
}

/** Returns the read-only status.
 * \return true if read only, false if read/write.
 * \sa SetReadOnly()
 * \author	Kurt Skauen (kurt@atheos.cx)
    *//////////////////////////////////////////////////////////////////////////////

bool DropdownMenu::GetReadOnly() const
{
	return ( m_pcEditBox->GetReadOnly() );
}

/** Add a item to the end of the drop down list.
 * \param cString - The string to be appended
 * \sa InsertItem(), DeleteItem(), GetItemCount(), GetItem()
 * \author	Kurt Skauen (kurt@atheos.cx)
    *//////////////////////////////////////////////////////////////////////////////

void DropdownMenu::AppendItem( const String& cString )
{
	m_cStringList.push_back( cString );
}

/** Insert and item at a given position.
 * \Description:
 *	The new item is inserted before the nPosition'th item.
 * \param nPosition - Zero based index of the item to insert the string in front of
 * \param pzString  - The string to be insert
 * \sa AppendItem(), DeleteItem(), GetItemCount(), GetItem()
 * \author	Kurt Skauen (kurt@atheos.cx)
    *//////////////////////////////////////////////////////////////////////////////

void DropdownMenu::InsertItem( int nPosition, const String& cString )
{
	m_cStringList.insert( m_cStringList.begin() + nPosition, cString );
}

/** Delete a item
 * \par Description:
 *	Delete the item at the given position
 * \param nPosition - The zero based index of the item to delete.
 * \return true if an item was deleted, false if the index was out of range.
 * \sa AppendItem(), InsertItem(), GetItemCount(), GetItem()
 * \author	Kurt Skauen (kurt@atheos.cx)
    *//////////////////////////////////////////////////////////////////////////////


bool DropdownMenu::DeleteItem( int nPosition )
{
	if( nPosition >= 0 && nPosition < int ( m_cStringList.size() ) )
	{
		m_cStringList.erase( m_cStringList.begin() + nPosition );
		return ( true );
	}
	else
	{
		return ( false );
	}
}

/** Get the item count
 * \return The number of items in the menu
 * \sa GetItem(), AppendItem(), InsertItem(), DeleteItem()
 * \author	Kurt Skauen (kurt@atheos.cx)
    *//////////////////////////////////////////////////////////////////////////////

int DropdownMenu::GetItemCount() const
{
	return ( m_cStringList.size() );
}

/** Get one of the item strings.
 * \par Description:
 *	Returns one of the items encapsulated in a stl string.
 * \param nItem - Zero based index of the item to return.
 * \return const reference to an stl string containing the item string
 * \sa GetItemCount(), AppendItem(), InsertItem(), DeleteItem()
 * \author	Kurt Skauen (kurt@atheos.cx)
    *//////////////////////////////////////////////////////////////////////////////

const String & DropdownMenu::GetItem( int nItem ) const
{
	assert( nItem >= 0 );
	assert( nItem < int ( m_cStringList.size() ) );

	return ( m_cStringList[nItem] );
}

/** Delete all items
 * \par Description:
 *	Delete all the items in the menu
 * \param nPosition - The zero based index of the item to delete.
 * \return true if an item was deleted, false if the index was out of range.
 * \sa AppendItem(), InsertItem(), GetItemCount(), GetItem()
 * \author	Kurt Skauen (kurt@atheos.cx)
    *//////////////////////////////////////////////////////////////////////////////

void DropdownMenu::Clear()
{
	m_cStringList.clear();
}

/** Get the current selection
 * \return The index of the selected item, or -1 if no item is selected.
 * \sa SetSelection(), SetSelectionMessage()
 * \author	Kurt Skauen (kurt@atheos.cx)
    *//////////////////////////////////////////////////////////////////////////////

int DropdownMenu::GetSelection() const
{
	return ( m_nSelection );
}

/** Set current selection
 * \par Description:
 *	The given item will be highlighted when the menu is opened
 *	and the item will be copyed into the edit box.
 *	If the bNotify parameter is true and the selection differs from the currend
 *	the "SelectionMessage" (see SetSelectionMessage()) will be sendt to the
 *	target set by SetTarget().
 * \param nItem - The new selection
 * \param bNotify - If true a notification will be sendt if the new selection
 *		    differ from the current.
 * \sa GetSelection(), SetSelectionMessage(), GetSelectionMessage(), Invoker::SetTarget()
 * \author	Kurt Skauen (kurt@atheos.cx)
    *//////////////////////////////////////////////////////////////////////////////

void DropdownMenu::SetSelection( int nItem, bool bNotify )
{
	if( nItem >= 0 && nItem < int ( m_cStringList.size() ) )
	{
		m_pcEditBox->Set( m_cStringList[nItem].c_str() );

		if( nItem != m_nSelection )
		{
			m_nSelection = nItem;
			if( bNotify && m_pcSelectionMsg != NULL )
			{
				Message cMsg( *m_pcSelectionMsg );

				cMsg.AddBool( "final", true );
				cMsg.AddInt32( "selection", m_nSelection );
				Invoke( &cMsg );
			}
		}
	}
}

const String & DropdownMenu::GetCurrentString() const
{
	return ( m_pcEditBox->GetBuffer()[0] );
}

void DropdownMenu::SetCurrentString( const String & cString )
{
	m_pcEditBox->Set( cString.c_str(), false );
}

/** Set the message that will be sendt when the selection changes.
 * \par Description:
 *	When the selection is changed eighter by the user or by calling
 *	SetSelection() with bNotify = true, the DropdownMenu() will clone off
 *	the message set with SetSelectionMessage() and send it to it's target
 *	by calling Invoker::Invoke().
 *
 *	Before sending the message the two fields "final" and "selection" are added.
 *	"final" is a boolean that is true when the user select (click's on) an
 *	item from the menu, or if the event was triggered by SetSelection().
 *	"final" is false when the event was triggered by the user moving the
 *	mouse over the menu. "selection" is an int32 that contain the new selection.
 *
 *	In addition to those fields comes the fields added by Invoker::Invoke().
 *
 *	If pcMsg is NULL no event will be triggered by changing the selection.
 * \par Note:
 *	The DropdownMenu overtake the ownership of the message. You should not
 *	delete or reference the message after SetSelectionMessage() returns.
 * \param pcMsg - The message to be sendt when selection changes.
 * \sa GetSelectionMessage(), SetEditMessage(), Invoker::SetTarget()
 * \author	Kurt Skauen (kurt@atheos.cx)
    *//////////////////////////////////////////////////////////////////////////////

void DropdownMenu::SetSelectionMessage( Message * pcMsg )
{
	if( pcMsg != m_pcSelectionMsg )
	{
		delete m_pcSelectionMsg;

		m_pcSelectionMsg = pcMsg;
	}
}

/** Get a pointer to the current "SelectionChanged" message.
 * \par Description:
 *	Return the pointer last set by SetSelectionMessage() or NULL if no message
 *	is assigned. If you change the message the changes will be reflected in
 *	the following "SelectionChanged" events.
 * \par Note:
 *	The message is still owned by the DropdownMenu. You should not delete it.
 * \return Pointer to the internel "SelectionChanged" message.
 * \sa SetSelectionMessage(), Invoker::SetTarget()
 * \author	Kurt Skauen (kurt@atheos.cx)
    *//////////////////////////////////////////////////////////////////////////////

Message *DropdownMenu::GetSelectionMessage() const
{
	return ( m_pcSelectionMsg );
}

void DropdownMenu::SetSendIntermediateMsg( bool bFlag )
{
	m_bSendIntermediateMsg = bFlag;
}

bool DropdownMenu::GetSendIntermediateMsg() const
{
	return ( m_bSendIntermediateMsg );
}

/** Set the message sendt when the user changes the content of the edit box.
 * \par Description:
 *	The message is sendt everytime the user changes the content of the edit
 *	box. Before the message is sendt a boolean member called "final" is added.
 *	This member is false when the user is typing into the edit box, and true
 *	for the last message sendt when the user hits <ENTER>.
 *
 *	The content of the edit box is not automatically copyed into the item-list.
 *	If you f.eks. want to add a new entry to the item list each time the user
 *	change the string and hits <ENTER>, you must listen to this message and
 *	read out the string with GetCurrentString() and add it to the list with
 *	AppendItem() or InsertItem() each time you receive a message with the
 *	"final" member equal "true".
 * \par Note:
 *	The DropdownMenu overtake the ownership of the message. You should not
 *	delete or reference the message after SetSelectionMessage() returns.
 * \param pcMsg - The message to be sendt when the edit box changes.
 * \sa GetEditMessage(), SetSelectionMessage(), Invoker::SetTarget(), Invoker::Invoke()
 * \author	Kurt Skauen (kurt@atheos.cx)
    *//////////////////////////////////////////////////////////////////////////////


void DropdownMenu::SetEditMessage( Message * pcMsg )
{
	if( pcMsg != m_pcEditMsg )
	{
		delete m_pcEditMsg;

		m_pcEditMsg = pcMsg;
	}
}

/** Get a pointer to the current "SelectionChanged" message.
 * \par Description:
 *	Return the pointer last set by SetEditMessage() or NULL if no message
 *	is assigned. If you change the message the changes will be reflected in
 *	the following events sendt when the edit box is changed.
 * \par Note:
 *	The message is still owned by the DropdownMenu. You should not delete is
 * \return Pointer to the internel "string-changed" message.
 * \sa SetSelectionMessage(), Invoker::SetTarget()
 * \author	Kurt Skauen (kurt@atheos.cx)
    *//////////////////////////////////////////////////////////////////////////////

Message *DropdownMenu::GetEditMessage() const
{
	return ( m_pcEditMsg );
}

/** Handle events from sub components.
 * \par Description:
 *	DropdownMenu overloads Handler::HandleMessage() to be able to comunicate
 *	with it's sub components.
 * \sa Handler::HandleMessage()
 * \author	Kurt Skauen (kurt@atheos.cx)
    *//////////////////////////////////////////////////////////////////////////////

void DropdownMenu::HandleMessage( Message * pcMessage )
{
	switch ( pcMessage->GetCode() )
	{
	case ID_STRING_CHANGED:
		{
			if( m_pcEditMsg != NULL )
			{
				bool bFinal = false;
				int32 nEvents = 0;

				pcMessage->FindInt32( "events", &nEvents );
				if( ( nEvents & TextView::EI_WAS_EDITED ) && ( nEvents & ( TextView::EI_ENTER_PRESSED | TextView::EI_FOCUS_LOST ) ) )
					bFinal = true;

				if( bFinal || m_bSendIntermediateMsg )
				{
					Message cMsg( *m_pcEditMsg );

					cMsg.AddBool( "final", bFinal );
					Invoke( &cMsg );
				}
			}
			break;
		}
	case ID_MENU_CLOSED:
		if( m_pcMenuWindow != NULL )
		{
			m_pcMenuWindow->PostMessage( M_QUIT );
//              m_pcMenuWindow->Quit();
			m_pcMenuWindow = NULL;
			m_bMenuOpen = false;
			Invalidate( GetBounds() );
			Flush();
		}
		break;
	case ID_SELECTION_CHANGED:
		{
			int32 nSelection;
			bool bFinal = false;

			pcMessage->FindInt32( "new_selection", &nSelection );
			pcMessage->FindBool( "final", &bFinal );

			if( bFinal || nSelection != m_nSelection )
			{
				m_nSelection = nSelection;

				if( bFinal && m_nSelection >= 0 && m_nSelection < int ( m_cStringList.size() ) )
				{
					m_pcEditBox->Set( m_cStringList[m_nSelection].c_str() );
				}

				if( m_nSelection >= 0 && m_nSelection < int ( m_cStringList.size() ) )
				{

					if( m_pcSelectionMsg != NULL )
					{
						if( m_bSendIntermediateMsg || bFinal )
						{
							Message cMsg( *m_pcSelectionMsg );

							cMsg.AddBool( "final", bFinal );
							cMsg.AddInt32( "selection", m_nSelection );
							Invoke( &cMsg );
						}
					}
				}
			}
			break;
		}
	default:
		View::HandleMessage( pcMessage );
		break;
	}
}

void DropdownMenu::MouseDown( const Point & cPosition, uint32 nButton )
{
	if( m_pcEditBox->IsEnabled() == false )
	{
		View::MouseDown( cPosition, nButton );
		return;
	}
	if( m_pcMenuWindow == NULL && m_cStringList.size() > 0 )
	{
		m_bMenuOpen = true;
		Invalidate( GetBounds() );
		Flush();
		OpenMenu();
	}
}

void DropdownMenu::WheelMoved( const Point& cDelta )
{
	if( !m_bMenuOpen ) {
		m_bMenuOpen = true;
		MakeFocus();
		Invalidate( GetBounds() );
		Flush();
		OpenMenu();
	} else {
		Message* pcMsg = new Message( M_WHEEL_MOVED );
		pcMsg->AddPointer( "_widget", m_pcMenuWindow->FindView( "drop_down_view" ) );
		pcMsg->AddPoint( "delta_move", cDelta );
		m_pcMenuWindow->PostMessage( pcMsg );
	}
}

void DropdownMenu::KeyDown( const char *pzString, const char *pzRawString, uint32 nQualifiers )
{
	if( IsEnabled() == false )
	{
		View::KeyDown( pzString, pzRawString, nQualifiers );
		return;
	}
	if( ( pzString[1] == '\0' && ( pzString[0] == VK_ENTER || pzString[0] == ' ' ) ) ||
		( GetShortcut() == ShortcutKey( pzRawString, nQualifiers ) ) )
	{
		m_bMenuOpen = true;
		MakeFocus();
		Invalidate( GetBounds() );
		Flush();
		OpenMenu();
	}
	else
	{
		View::KeyDown( pzString, pzRawString, nQualifiers );
	}
}

void DropdownMenu::SetTabOrder( int nOrder )
{
	m_pcEditBox->SetTabOrder( nOrder );
	View::SetTabOrder();
}

int DropdownMenu::GetTabOrder() const
{
	return( m_pcEditBox->GetTabOrder() );
}

void DropdownMenu::Activated( bool bIsActive )
{
	Invalidate();
	Flush();
}

void DropdownMenu::Paint( const Rect & cUpdateRect )
{
	Rect cBounds = GetBounds();

	SetEraseColor( get_default_color( COL_NORMAL ) );
	
	Rect cArrowRect = m_cArrowRect;

	if( IsEnabled() && HasFocus() )
	{
		SetFgColor( get_default_color( COL_FOCUS ) );
		DrawLine( Point( cArrowRect.left, cArrowRect.top ), Point( cArrowRect.right, cArrowRect.top ) );
		DrawLine( Point( cArrowRect.right, cArrowRect.bottom ) );
		DrawLine( Point( cArrowRect.left, cArrowRect.bottom ) );
		DrawLine( Point( cArrowRect.left, cArrowRect.top ) );
		cArrowRect.Resize( 1, 1, -1, -1 );
	}

	DrawFrame( cArrowRect, ( m_bMenuOpen ) ? FRAME_RECESSED : FRAME_RAISED );

	float nCenterX = m_cArrowRect.left + ( m_cArrowRect.Width() + 1.0f ) / 2;
	float nCenterY = m_cArrowRect.top + ( m_cArrowRect.Height() + 1.0f ) / 2;

	Point cBmOffset( 0.0f, 0.0f );

	if( m_pcEditBox->IsEnabled() == false )
	{
		nCenterX += 1.0f;
		cBmOffset.y += ARROW_HEIGHT;
	}

	SetFgColor( 0, 0, 0 );
	Rect cArrow( 0, 0, ARROW_WIDTH - 1, ARROW_HEIGHT - 1 );

	SetDrawingMode( DM_OVER );
	DrawBitmap( g_pcArrows, cArrow + cBmOffset, cArrow + Point( nCenterX - 4, nCenterY - 2 ) );
	if( m_pcEditBox->IsEnabled() == false )
	{
		cBmOffset.y += ARROW_HEIGHT;
		DrawBitmap( g_pcArrows, cArrow + cBmOffset, cArrow + Point( nCenterX - 5, nCenterY - 3 ) );
	}
	SetDrawingMode( DM_COPY );
}

void DropdownMenu::FrameSized( const Point & cDelta )
{
	Rect cEditFrame = GetBounds();
	
	float vArrowHeight = cEditFrame.Height() + 1.0f;

	m_cArrowRect = Rect( cEditFrame.right - vArrowHeight * 0.9f, 0, cEditFrame.right, cEditFrame.bottom );

	cEditFrame.right = m_cArrowRect.left - 1;
	m_pcEditBox->SetFrame( cEditFrame );
}


void DropdownMenu::SetMinPreferredSize( int nWidthChars )
{
	if( nWidthChars == 0 )
	{
		m_pcEditBox->SetMinPreferredSize( 0, 0 );
	}
	else
	{
		m_pcEditBox->SetMinPreferredSize( nWidthChars, 1 );
	}
}

int DropdownMenu::GetMinPreferredSize() const
{
	return ( m_pcEditBox->GetMinPreferredSize().x );
}

void DropdownMenu::SetMaxPreferredSize( int nWidthChars )
{
	if( nWidthChars == 0 )
	{
		m_pcEditBox->SetMaxPreferredSize( 0, 0 );
	}
	else
	{
		m_pcEditBox->SetMaxPreferredSize( nWidthChars, 1 );
	}
}

int DropdownMenu::GetMaxPreferredSize() const
{
	return ( m_pcEditBox->GetMaxPreferredSize().x );
}

Point DropdownMenu::GetPreferredSize( bool bLargest ) const
{
	font_height sFontHeight;

	GetFontHeight( &sFontHeight );

	Point cSize( 30.0f, sFontHeight.ascender + sFontHeight.descender + 7.0f );

	if( ( bLargest ) ? ( m_pcEditBox->GetMaxPreferredSize().x == 0 ) : ( m_pcEditBox->GetMinPreferredSize(  ).x == 0 ) )
	{
		for( uint i = 0; i < m_cStringList.size(); ++i )
		{
			float vWidth = GetStringWidth( m_cStringList[i] );

			if( vWidth > cSize.x )
			{
				cSize.x = vWidth;
			}
		}
		cSize.x += 16 + 9.0f + cSize.y * 0.9f;
	}
	else
	{
		cSize.x = 16 + 9.0f + cSize.y * 0.9f + m_pcEditBox->GetPreferredSize( bLargest ).x;
	}
	cSize.x += cSize.y;
	return ( cSize + Point( 2, 2 ) );
}

void DropdownMenu::AllAttached()
{
	m_pcEditBox->SetTarget( this );
	m_pcEditBox->SetFont( GetFont() );
}

void DropdownMenu::FontChanged( Font* pcNewFont )
{
	m_pcEditBox->SetFont( pcNewFont );
	if( m_bMenuOpen && m_pcMenuWindow ) {
		/* TODO: pointer to the DropdownView should be stored, to avoid the overhead of FindView() */
		View* pcView = m_pcMenuWindow->FindView( "drop_down_view" );
		if( pcView ) {
			pcView->SetFont( pcNewFont );
		}	/* else: bug!? */
	}
}

void DropdownMenu::OpenMenu()
{
	m_pcMenuWindow = new Window( Rect( 0, 0, 0, 0 ), "menu", "menu", WND_NO_BORDER | WND_SYSTEM );

	Locker *pcMutex = GetLooper()->GetMutex(  );

	pcMutex->Lock();
	m_pcMenuWindow->SetMutex( pcMutex );

	DropdownView *pcMenuView = new DropdownView( this );

	m_pcMenuWindow->AddChild( pcMenuView );
	if( m_cStringList.size() > MAX_DD_HEIGHT )
	{
		pcMenuView->m_pcScrollBar = new ScrollBar( Rect(), "ScrollBar", new Message( EV_SB_INVOKED ), 0, m_cStringList.size(  ) - 1 );
		pcMenuView->m_pcScrollBar->SetTarget( pcMenuView );
		pcMenuView->m_pcScrollBar->SetValue( m_nSelection );

		float vSBWidth = pcMenuView->m_pcScrollBar->GetPreferredSize( true ).x;
		Rect cFrame = pcMenuView->GetFrame();

		pcMenuView->m_pcScrollBar->SetFrame( Rect( cFrame.Width() - vSBWidth, 0, cFrame.Width(  ), cFrame.Height(  ) ) );
		cFrame.right -= vSBWidth;
		pcMenuView->SetFrame( cFrame );

		m_pcMenuWindow->AddChild( pcMenuView->m_pcScrollBar );
	}
	m_pcMenuWindow->SetDefaultWheelView( pcMenuView );
	pcMenuView->MakeFocus();
	m_pcMenuWindow->Show();
	m_pcMenuWindow->MakeFocus();
}



DropdownMenu::DropdownView::DropdownView( DropdownMenu * pcParent ):View( Rect( 0, 0, 0, 0 ), "drop_down_view", CF_FOLLOW_ALL )
{
	m_pcParent = pcParent;
	m_nCurSelection = pcParent->m_nSelection;
	m_nScrollPos = 0;
	m_nOldSelection = m_nCurSelection;

	m_pcScrollBar = NULL;
	
	m_bIsAttached = false;
	
	SetFont( pcParent->GetFont() );

	if( m_nCurSelection > MAX_DD_HEIGHT )
	{
		m_nScrollPos = ( m_pcParent->m_cStringList.size() - MAX_DD_HEIGHT ) < ( uint )m_nCurSelection ? m_pcParent->m_cStringList.size(  ) - MAX_DD_HEIGHT : m_nCurSelection - 1;
		m_nCurSelection -= m_nScrollPos;
	}
}

void DropdownMenu::DropdownView::Paint( const Rect & cUpdateRect )
{
	SetEraseColor( 255, 255, 255 );
	DrawFrame( GetBounds(), FRAME_RECESSED );

	SetFgColor( 0, 0, 0 );
	SetBgColor( 255, 255, 255 );

	uint nSize = m_pcParent->m_cStringList.size() > MAX_DD_HEIGHT ? MAX_DD_HEIGHT : m_pcParent->m_cStringList.size(  );
	float y = 4 + m_sFontHeight.ascender + m_sFontHeight.line_gap / 2;

	for( uint i = 0; i < nSize; i++ )
	{
		MovePenTo( 4, y );
		if( i == uint ( m_nCurSelection ) )
		{
			Rect cItemFrame = GetBounds();

			cItemFrame.top = y - m_sFontHeight.ascender - m_sFontHeight.line_gap * 0.5f;
			cItemFrame.bottom = cItemFrame.top + m_vGlyphHeight - 1;
			cItemFrame.left += 2;
			cItemFrame.right -= 2;
			FillRect( cItemFrame, os::Color32_s( 186, 199, 227 ) );

			SetBgColor( os::Color32_s( 186, 199, 227 ) );
		}
		DrawString( m_pcParent->m_cStringList[i + m_nScrollPos].c_str() );
		SetFgColor( 0, 0, 0 );
		SetBgColor( 255, 255, 255 );

		y += m_vGlyphHeight;
	}
}

void DropdownMenu::DropdownView::Activated( bool bIsActive )
{
	return;
	if( bIsActive == false )
	{
		Message cMsg( DropdownMenu::ID_SELECTION_CHANGED );

		cMsg.AddInt32( "new_selection", m_nOldSelection );
		m_pcParent->GetWindow()->PostMessage( &cMsg, m_pcParent );
		m_pcParent->GetWindow()->PostMessage( DropdownMenu::ID_MENU_CLOSED, m_pcParent );;
	}
}

void DropdownMenu::DropdownView::MouseDown( const Point & cPosition, uint32 nButtons )
{
	if( GetBounds().DoIntersect( cPosition ) )
	{
		if( !( m_nCurSelection == ( MAX_DD_HEIGHT - 1 ) && ( uint )m_nScrollPos < ( m_pcParent->m_cStringList.size() - MAX_DD_HEIGHT ) ) && !( m_nCurSelection == 0 && m_nScrollPos > 0 ) )
		{
			Message cMsg( DropdownMenu::ID_SELECTION_CHANGED );

			cMsg.AddInt32( "new_selection", m_nCurSelection + m_nScrollPos );
			cMsg.AddBool( "final", true );
			m_pcParent->GetWindow()->PostMessage( &cMsg, m_pcParent );
		}
	}
	else
	{
		Message cMsg( DropdownMenu::ID_SELECTION_CHANGED );

		cMsg.AddInt32( "new_selection", m_nOldSelection );
		m_pcParent->GetWindow()->PostMessage( &cMsg, m_pcParent );
		m_pcParent->GetWindow()->PostMessage( DropdownMenu::ID_MENU_CLOSED, m_pcParent );;
	}
}

void DropdownMenu::DropdownView::MouseUp( const Point & cPosition, uint32 nButtons, Message * pcData )
{
	if( GetBounds().DoIntersect( cPosition ) )
	{
		if( m_nCurSelection == ( MAX_DD_HEIGHT - 1 ) && ( uint )m_nScrollPos < ( m_pcParent->m_cStringList.size() - MAX_DD_HEIGHT ) )
		{
			m_nScrollPos++;
		}
		else if( m_nCurSelection == 0 && m_nScrollPos > 0 )
		{
			m_nScrollPos--;
		}
		else
		{
			Message cMsg( DropdownMenu::ID_SELECTION_CHANGED );

			cMsg.AddInt32( "new_selection", m_nCurSelection + m_nScrollPos );
			cMsg.AddBool( "final", true );
			m_pcParent->GetWindow()->PostMessage( &cMsg, m_pcParent );
			m_pcParent->GetWindow()->PostMessage( DropdownMenu::ID_MENU_CLOSED, m_pcParent );
		}
		if( m_pcScrollBar != NULL )
		{
			m_pcScrollBar->SetValue( m_nScrollPos + m_nCurSelection );
		}
		Invalidate();
		Flush();
	}
}

void DropdownMenu::DropdownView::MouseMove( const Point & cPosition, int nCode, uint32 nButtons, Message * pcData )
{
	if( GetBounds().DoIntersect( cPosition ) == false )
	{
		return;
	}

	if( cPosition == m_cOldPosition )
	{
		return;
	}
	m_cOldPosition.x = cPosition.x;
	m_cOldPosition.y = cPosition.y;

	int nNewSel = int ( ( cPosition.y - 2 ) / m_vGlyphHeight );

	if( nNewSel >= int ( m_pcParent->m_cStringList.size() ) )
	{
		nNewSel = m_pcParent->m_cStringList.size() - 1;
	}

	if( nNewSel != m_nCurSelection )
	{
		int nPrevSel = m_nCurSelection;

		m_nCurSelection = nNewSel;
		Rect cItemFrame = GetBounds();

		cItemFrame.top = nPrevSel * m_vGlyphHeight + 4;
		cItemFrame.bottom = cItemFrame.top + m_vGlyphHeight - 1;
		Invalidate( cItemFrame );

		cItemFrame.top = m_nCurSelection * m_vGlyphHeight + 4;
		cItemFrame.bottom = cItemFrame.top + m_vGlyphHeight - 1;

		Message cMsg( DropdownMenu::ID_SELECTION_CHANGED );

		cMsg.AddInt32( "new_selection", nNewSel );
		m_pcParent->GetWindow()->PostMessage( &cMsg, m_pcParent );

		if( m_pcScrollBar != NULL )
		{
			m_pcScrollBar->SetValue( m_nScrollPos + m_nCurSelection );
		}

		Invalidate( cItemFrame );
		Flush();
	}
}

void DropdownMenu::DropdownView::WheelMoved( const Point& cDelta )
{
	char bfr[2];
	bfr[1] = 0;
	if( cDelta.y > 0 ) {
		bfr[0] = VK_DOWN_ARROW;
		KeyDown( bfr, bfr, 0 );
	} else if( cDelta.y < 0 ) {
		bfr[0] = VK_UP_ARROW;
		KeyDown( bfr, bfr, 0 );
	}
}

void DropdownMenu::DropdownView::KeyDown( const char *pzString, const char *pzRawString, uint32 nQualifiers )
{
	switch ( pzString[0] )
	{
	case VK_DOWN_ARROW:
		if( ( uint )( m_nCurSelection + m_nScrollPos ) < m_pcParent->m_cStringList.size() - 1 )
		{
			m_nCurSelection++;
			if( m_nCurSelection == ( MAX_DD_HEIGHT - 1 ) && m_pcParent->m_cStringList.size() > MAX_DD_HEIGHT )
			{
				if( ( uint )( m_nScrollPos + MAX_DD_HEIGHT ) < m_pcParent->m_cStringList.size() )
				{
					m_nScrollPos++;
					m_nCurSelection--;
				}
			}
		} else if( m_nCurSelection == -1 && m_pcParent->m_cStringList.size() > 0 )  /* Nothing selected; select the first item */
		{
			m_nCurSelection = 0;
			m_nScrollPos = 0;
		}
		break;
	case VK_UP_ARROW:
		if( ( m_nCurSelection + m_nScrollPos ) > 0 )
		{
			m_nCurSelection--;
			if( m_nScrollPos > 0 && m_nCurSelection < 1 )
			{
				m_nCurSelection = 1;
				m_nScrollPos--;
			}
		} else if( m_nCurSelection == -1 && m_pcParent->m_cStringList.size() > 0 ) /* Nothing selected; select the first item */
		{
			m_nCurSelection = 0;
			m_nScrollPos = 0;
		}
		break;

	case VK_ENTER:
		{
			Message cMsg( DropdownMenu::ID_SELECTION_CHANGED );

			cMsg.AddInt32( "new_selection", m_nCurSelection + m_nScrollPos );
			cMsg.AddBool( "final", true );
			m_pcParent->GetWindow()->PostMessage( &cMsg, m_pcParent );
			m_pcParent->GetWindow()->PostMessage( DropdownMenu::ID_MENU_CLOSED, m_pcParent );
			break;
		}

	case VK_ESCAPE:
		{
			Message cMsg( DropdownMenu::ID_SELECTION_CHANGED );

			cMsg.AddInt32( "new_selection", m_nOldSelection );
			m_pcParent->GetWindow()->PostMessage( &cMsg, m_pcParent );
			m_pcParent->GetWindow()->PostMessage( DropdownMenu::ID_MENU_CLOSED, m_pcParent );
			break;
		}

	case VK_HOME:
		m_nCurSelection = 0;
		m_nScrollPos = 0;
		break;

	case VK_END:
		if( m_pcParent->m_cStringList.size() > MAX_DD_HEIGHT )
		{
			m_nScrollPos = m_pcParent->m_cStringList.size() - MAX_DD_HEIGHT;
			m_nCurSelection = MAX_DD_HEIGHT - 1;
		}
		else
		{
			m_nScrollPos = 0;
			m_nCurSelection = m_pcParent->m_cStringList.size() - 1;
		}
		break;

	case VK_PAGE_UP:
		if( m_pcParent->m_cStringList.size() > MAX_DD_HEIGHT && m_nScrollPos > 0 )
		{
			m_nScrollPos -= MAX_DD_HEIGHT;
			if( m_nScrollPos < 0 )
			{
				m_nScrollPos = 0;
				m_nCurSelection = 0;
			}
		}
		break;

	case VK_PAGE_DOWN:
		if( m_pcParent->m_cStringList.size() > MAX_DD_HEIGHT && ( uint )m_nScrollPos < ( m_pcParent->m_cStringList.size(  ) - MAX_DD_HEIGHT ) )
		{
			m_nScrollPos += MAX_DD_HEIGHT;
			if( ( uint )m_nScrollPos > ( m_pcParent->m_cStringList.size() - MAX_DD_HEIGHT ) )
			{
				m_nScrollPos = m_pcParent->m_cStringList.size() - MAX_DD_HEIGHT;
				m_nCurSelection = MAX_DD_HEIGHT - 1;
			}
		}
		break;

	default:
		{
			int nItem;
			char nKey = pzString[0];

			if( nKey > 64 && nKey < 91 )
			{
				nKey += 32;
			}
			for( uint i = 1; i < m_pcParent->m_cStringList.size(); i++ )
			{
				nItem = i + m_nScrollPos + m_nCurSelection;
				if( ( uint )nItem >= m_pcParent->m_cStringList.size() )
				{
					nItem -= m_pcParent->m_cStringList.size();
				}

				char nChar = m_pcParent->m_cStringList[nItem][0];

				if( nChar > 64 && nChar < 91 )
				{
					nChar += 32;
				}

				if( nKey == nChar )
				{
					if( m_pcParent->m_cStringList.size() > MAX_DD_HEIGHT )
					{
						m_nScrollPos = nItem;
						m_nCurSelection = 0;
					}
					else
					{
						m_nCurSelection = nItem;
					}
					if( ( uint )m_nScrollPos > ( m_pcParent->m_cStringList.size() - MAX_DD_HEIGHT ) )
					{
						m_nScrollPos = m_pcParent->m_cStringList.size() - MAX_DD_HEIGHT;
						m_nCurSelection = nItem - m_nScrollPos;
					}
					break;
				}
			}

			View::KeyDown( pzString, pzRawString, nQualifiers );
		}
	}

	if( m_pcScrollBar != NULL )
	{
		m_pcScrollBar->SetValue( m_nScrollPos + m_nCurSelection );
	}

	Invalidate();
	Flush();
}

void DropdownMenu::DropdownView::AllAttached()
{
	m_bIsAttached = true;
	_Layout();
}

void DropdownMenu::DropdownView::_Layout()
{
	m_cContentSize.x = 0;	/* Recompute these when the font changes */
	m_cContentSize.y = 0;
	
	Window *pcWindow = GetWindow();

	GetFont()->GetHeight( &m_sFontHeight );
	m_vGlyphHeight = m_sFontHeight.descender + m_sFontHeight.ascender + m_sFontHeight.line_gap;

	for( uint i = 0; i < m_pcParent->m_cStringList.size(); ++i )
	{
		float vLength = GetStringWidth( m_pcParent->m_cStringList[i].c_str() );

		if( vLength > m_cContentSize.x )
		{
			m_cContentSize.x = vLength;
		}
	}
	m_cContentSize.x += 8;

	Rect cMenuRect = m_pcParent->GetBounds();

	if( m_cContentSize.x <= cMenuRect.Width() + 1.0f )
	{
		m_cContentSize.x = cMenuRect.Width() + 1.0f;
	}
	cMenuRect.right = cMenuRect.left + m_cContentSize.x - 1;
	m_cContentSize.y = ( m_pcParent->m_cStringList.size() > MAX_DD_HEIGHT ? MAX_DD_HEIGHT * m_vGlyphHeight : m_pcParent->m_cStringList.size(  ) * m_vGlyphHeight ) + 8;

	cMenuRect.top = cMenuRect.bottom;
	cMenuRect.bottom = cMenuRect.top + m_cContentSize.y - 1;

	pcWindow->SetFrame( m_pcParent->ConvertToScreen( cMenuRect ) );
}

void DropdownMenu::DropdownView::FontChanged( Font* pcNewFont )
{
	/* _Layout() is only valid after the view is attached. */
	if( m_bIsAttached ) {	/* TODO: Use View::IsAttached() if/when it is implemented */
		/* Call _Layout() to make the view recompute the sizes, etc. */
		_Layout();

		Invalidate();
		Flush();
	}
}


void DropdownMenu::DropdownView::HandleMessage( Message * pcMessage )
{
	switch ( pcMessage->GetCode() )
	{
	case EV_SB_INVOKED:
		{
			int32 nNewVal = m_pcScrollBar->GetValue().AsInt32(  );
			int32 nOldVal = m_nScrollPos + m_nCurSelection;
			int32 nDelta = nNewVal - nOldVal;

			if( nDelta != 0 )
			{	// nDelta will be 0 if the scrollbar value was set by the dropdownmenu
				if( abs( nDelta ) == 1 )
				{
					m_nScrollPos += nDelta;
				}
				else
				{
					m_nScrollPos = nNewVal - ( MAX_DD_HEIGHT / 2 );
					m_nCurSelection = nNewVal - m_nScrollPos;
				}
				Invalidate();
				Flush();
			}

			if( m_nScrollPos < 0 )
			{
				m_nScrollPos = 0;
				if( m_nCurSelection > 0 )
				{
					m_nCurSelection--;
				}
			}
			else if( ( uint )m_nScrollPos > ( m_pcParent->m_cStringList.size() - MAX_DD_HEIGHT ) )
			{
				m_nScrollPos = m_pcParent->m_cStringList.size() - MAX_DD_HEIGHT;
				m_nCurSelection = nNewVal - m_nScrollPos;
			}
			break;
		}
	}
}

