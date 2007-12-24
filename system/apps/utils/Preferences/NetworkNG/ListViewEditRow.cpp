// Syllable Network Preferences - Copyright 2006 Andrew Kennan
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "ListViewEditRow.h"

#include <gui/textview.h>

class EditTextView:public TextView
{
      public:
	EditTextView( const Rect & cFrame, const char *pzBuffer, ListViewEditRow * pcOwner, int nColumn ):TextView( cFrame, "EditTextView", pzBuffer )
	{
		m_pcOwner = pcOwner;
		m_nColumn = nColumn;
	}

	virtual void Activated( bool bIsActive )
	{
		TextView::Activated( bIsActive );
		if( !bIsActive )
			m_pcOwner->LostFocus( true );
	}

	virtual void MakeFocus( bool bFocus = true )
	{
		TextView::MakeFocus( bFocus );
		if( !bFocus )
			m_pcOwner->LostFocus( true );
	}

	virtual void KeyDown( const char *pzString, const char *pzRawString, uint32 nQualifiers )
	{
		switch ( ( uint8 )pzRawString[0] )
		{
		case VK_TAB:
		case VK_ENTER:
			m_pcOwner->LostFocus( true );
			break;

		case VK_ESCAPE:
			m_pcOwner->LostFocus( false );
			break;

		default:
			TextView::KeyDown( pzString, pzRawString, nQualifiers );
			break;
		}
	}

	int GetColumn( void )
	{
		return m_nColumn;
	}

      private:
	int m_nColumn;
	ListViewEditRow *m_pcOwner;
};

ListViewEditRow::ListViewEditRow():ListViewStringRow(  ), m_pcTextView( NULL ), m_bReadOnly( false )
{
}

ListViewEditRow::~ListViewEditRow()
{
}

bool ListViewEditRow::HitTest( View * pcView, const Rect & cFrame, int nColumn, Point cPos )
{
	bool bHit = ListViewStringRow::HitTest( pcView, cFrame, nColumn, cPos );

	if( !IsReadOnly() && bHit && m_pcTextView == NULL && IsSelected(  ) )
	{
		Rect cCtlFrame;

		cCtlFrame.left = 0;
		cCtlFrame.right = cFrame.Width();
		cCtlFrame.top = cFrame.top;
		cCtlFrame.bottom = cFrame.bottom;
		m_pcTextView = new EditTextView( cCtlFrame, GetString( nColumn ).c_str(), this, nColumn );
		pcView->AddChild( m_pcTextView );
		m_pcTextView->MakeFocus( true );
	}

	return bHit;
}

void ListViewEditRow::LostFocus( bool bSave )
{
	if( m_pcTextView != NULL )
	{
		View *pcParent = m_pcTextView->GetParent();

		if( bSave )
			SetString( m_pcTextView->GetColumn(), m_pcTextView->GetValue(  ).AsString(  ) );

		pcParent->RemoveChild( m_pcTextView );
		delete( m_pcTextView );
		m_pcTextView = NULL;
	}
}
