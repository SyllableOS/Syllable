//  AEdit -:-  (C)opyright 2000-2002 Kristian Van Der Vliet
//             (C)opyright 2004-2006 Jonas Jarvoll
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

#include "finddialog.h"
#include "buffer.h"
#include "main.h"
#include "icons.h"
#include "messages.h"
#include "resources/aedit.h"

using namespace os;

FindDialog :: FindDialog() : Dialog()
{
	// Create layout
	pcLayoutView = new LayoutView( Rect(), "" );
	AddChild( pcLayoutView );

	// Create basepanel	
	pcRoot = new VLayoutNode( "" );
	pcRoot->SetVAlignment( os::ALIGN_LEFT );
	pcRoot->SetHAlignment( os::ALIGN_LEFT );
	pcLayoutView->SetRoot( pcRoot );

	// Create panels
	pcTopPanel = new HLayoutNode( "" );
	pcTopPanel->SetVAlignment( os::ALIGN_LEFT );
	pcTopPanel->SetHAlignment( os::ALIGN_LEFT );
	pcRoot->AddChild( pcTopPanel );

	pcBottomPanel = new HLayoutNode( "" );
	pcBottomPanel->SetVAlignment( os::ALIGN_LEFT );
	pcBottomPanel->SetHAlignment( os::ALIGN_LEFT );
	pcRoot->AddChild( pcBottomPanel );

	// Create close button
	pcCloseButton = new ImageButton( Rect(), "close","", new Message( M_BUT_FIND_CLOSE ), GetStockIcon( STOCK_CLOSE, STOCK_SIZE_MENUITEM ) );
	pcTopPanel->AddChild( pcCloseButton );

	// Create space
	pcTopPanel->AddChild( new HLayoutSpacer( "", 8, 8 ) );

	// Create string
	pcFindString = new StringView( Rect(), "find_string", MSG_FIND_FIND_LABEL );
	pcTopPanel->AddChild( pcFindString );

	// Create space
	pcTopPanel->AddChild( new HLayoutSpacer( "", 4, 4 ) );

	// Create textview
	pcFindTextView = new TextView( Rect(), "", "" );
	pcFindTextView->SetMessage( new Message( M_BUT_FIND_TEXTVIEW ) );
	pcFindTextView->SetEventMask( TextView::EI_ENTER_PRESSED );
	pcFindTextView->SetMinPreferredSize( 20, 1 );
	pcFindTextView->SetMaxPreferredSize( 200, 1 );
	pcTopPanel->AddChild( pcFindTextView );

	// Create space
	pcTopPanel->AddChild( new HLayoutSpacer( "", 4, 4 ) );

	// Create the find/find next button
	pcFirstButton = new Button( Rect(), "first_button", MSG_FIND_FIND, new Message( M_BUT_FIND_GO ) );
	pcTopPanel->AddChild( pcFirstButton );

	// Create space
	pcTopPanel->AddChild( new HLayoutSpacer( "", 4, 4 ) );

	pcNextButton = new Button( Rect(), "next_button", MSG_FIND_FIND_NEXT, new Message( M_BUT_FIND_NEXT ) );
	pcTopPanel->AddChild( pcNextButton );

	// Create checkbox for case sensitivity
	pcCaseCheckbox = new CheckBox( Rect(),"case_checkbox", MSG_FIND_CASE_SENSITIVE, new Message( M_VOID ) );
	pcBottomPanel->AddChild( pcCaseCheckbox );

	// Set the focus
	MakeFocus();
}

FindDialog :: ~FindDialog()
{
}

void FindDialog :: FrameSized( const Point& cDelta )
{
	View::FrameSized( cDelta );
	_Layout();
}

void FindDialog :: AllAttached()
{
	View::AllAttached();
	pcFirstButton->SetTarget( this );
	pcNextButton->SetTarget( this );
	pcCloseButton->SetTarget( this );
	pcFindTextView->SetTarget( this );
	pcCaseCheckbox->SetTarget( this );
}

void FindDialog :: HandleMessage( Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case M_BUT_FIND_GO:
		{
			_FindFirst();
			break;
		}
		case M_BUT_FIND_NEXT:
		{
			_FindNext();
			break;
		}
		case M_BUT_FIND_CLOSE:
		{
			AEditApp::GetAEditWindow()->SetDialog( NULL );
			break;
		}
		case M_BUT_FIND_TEXTVIEW:
		{
			int32 nEvent;
			
			if( pcMessage->FindInt32( "events", &nEvent ) == EOK )
			{
				if( nEvent & TextView::EI_ENTER_PRESSED )
					_FindFirst();			
			}
			break;
		}
		default:
			View::HandleMessage( pcMessage );
			break;
	}
}

// Disable/enable buttons in dialog depending on the parameter
void FindDialog :: SetEnable( bool bEnable )
{
	pcFirstButton->SetEnable( bEnable );
	pcNextButton->SetEnable( bEnable );
}

void FindDialog :: Init()
{
	// Set cursor on the text entry
	pcFindTextView->SelectAll();
	pcFindTextView->MakeFocus();
}

Point FindDialog :: GetPreferredSize( bool bSize ) const
{
	Point p = pcLayoutView->GetPreferredSize( bSize );
	p.x += 4;
	p.y += 4;
	return p;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// P R I V A T E
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void FindDialog :: _FindFirst()
{
	Buffer* current = AEditApp::GetAEditWindow()->pcCurrentBuffer;

	if( current != NULL )
	{
		String text = pcFindTextView->GetBuffer()[0];

		if( current->FindFirst( text, pcCaseCheckbox->GetValue() ? true : false ) )
		{
			AEditApp::GetAEditWindow()->SetStatus( MSG_STATUSBAR_SEARCH_FOUND.c_str(), text.c_str() );
			current->MakeFocus();
		}
		else
			AEditApp::GetAEditWindow()->SetStatus( MSG_STATUSBAR_SEARCH_NOT_FOUND_1.c_str(), text.c_str() );
	}
}

void FindDialog :: _FindNext()
{
	Buffer* current = AEditApp::GetAEditWindow()->pcCurrentBuffer;

	if( current != NULL )
	{
		String text;
		text = current->FindNext();

		if( text != "" )
		{
			AEditApp::GetAEditWindow()->SetStatus( MSG_STATUSBAR_SEARCH_FOUND.c_str(), text.c_str() );
			current->MakeFocus();
		}
		else
		{
			AEditApp::GetAEditWindow()->SetStatus( MSG_STATUSBAR_SEARCH_NOT_FOUND_2 );
		}
	}
}

void FindDialog :: _Layout()
{
	Rect cFrame = GetBounds();
	cFrame.Resize( 2, 2, -2, -2 );
	cFrame.right = cFrame.left + GetPreferredSize( false ).x;

	pcLayoutView->SetFrame( cFrame );	
}
