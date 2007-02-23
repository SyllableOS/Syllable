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

#include "gotodialog.h"
#include "buffer.h"
#include "main.h"
#include "icons.h"
#include "messages.h"
#include "resources/aedit.h"

using namespace os;

GotoDialog :: GotoDialog() : Dialog()
{
	// Create layout
	pcLayoutView = new LayoutView( Rect(), "" );
	AddChild( pcLayoutView );

	pcRoot = new HLayoutNode( "" );
	pcRoot->SetVAlignment( os::ALIGN_LEFT );
	pcRoot->SetHAlignment( os::ALIGN_LEFT );
	pcLayoutView->SetRoot( pcRoot );

	// Create close button
	pcCloseButton = new ImageButton( Rect(), "close","", new Message( M_BUT_GOTO_CLOSE ), GetStockIcon( STOCK_CLOSE, STOCK_SIZE_MENUITEM ) );
	pcRoot->AddChild( pcCloseButton );

	// Create space
	pcRoot->AddChild( new HLayoutSpacer( "", 8, 8 ) );

	// Create string
	pcGotoString = new StringView( Rect(), "goto_string", MSG_GOTO_LINE );
	pcRoot->AddChild( pcGotoString );

	// Create space
	pcRoot->AddChild( new HLayoutSpacer( "", 4, 4 ) );

	// Create textview
	pcLineNoTextView = new TextView( Rect(), "", "" );
	pcLineNoTextView->SetMessage( new Message( M_BUT_GOTO_TEXTVIEW ) );
	pcLineNoTextView->SetEventMask( TextView::EI_ENTER_PRESSED );
	pcLineNoTextView->SetNumeric( true );
	pcLineNoTextView->SetMinPreferredSize( 20, 1 );
	pcLineNoTextView->SetMaxPreferredSize( 200, 1 );
	pcRoot->AddChild( pcLineNoTextView );

	// Create space
	pcRoot->AddChild( new HLayoutSpacer( "", 4, 4 ) );

	// Create the goto button
	pcGotoButton = new Button( Rect(), "goto_button", MSG_GOTO_GOTO, new Message( M_BUT_GOTO_GOTO ) );
	pcRoot->AddChild( pcGotoButton );

	// Set the focus
	MakeFocus();
}

GotoDialog :: ~GotoDialog()
{
}

void GotoDialog :: FrameSized( const Point& cDelta )
{
	View::FrameSized( cDelta );
	_Layout();
}

void GotoDialog :: AllAttached()
{
	View::AllAttached();
	pcGotoButton->SetTarget( this );
	pcCloseButton->SetTarget( this );
	pcLineNoTextView->SetTarget( this );
}

void GotoDialog :: HandleMessage( Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case M_BUT_GOTO_GOTO:
		{
			_Goto();
			break;
		}
		case M_BUT_GOTO_CLOSE:
		{
			AEditApp::GetAEditWindow()->SetDialog( NULL );
			break;
		}
		case M_BUT_GOTO_TEXTVIEW:
		{
			int32 nEvent;
			
			if( pcMessage->FindInt32( "events", &nEvent ) == EOK )
			{
				if( nEvent & TextView::EI_ENTER_PRESSED )
					_Goto();			
			}
			break;
		}
		default:
			View::HandleMessage( pcMessage );
			break;
	}
}

// Disable/enable buttons in dialog depending on the parameter
void GotoDialog :: SetEnable( bool bEnable )
{
	pcGotoButton->SetEnable( bEnable );
}

void GotoDialog :: Init()
{
	// Set cursor on the text entry
	pcLineNoTextView->SelectAll();
	pcLineNoTextView->MakeFocus();
}

Point GotoDialog :: GetPreferredSize( bool bSize ) const
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

void GotoDialog :: _Goto()
{
	Buffer* current = AEditApp::GetAEditWindow()->pcCurrentBuffer;

	if( current != NULL )
	{
		int line_no = atoi( pcLineNoTextView->GetBuffer()[0].c_str() );
		current->GotoLine( line_no );
		current->MakeFocus();
	}
}

void GotoDialog :: _Layout()
{
	Rect cFrame = GetBounds();
	cFrame.Resize( 2, 2, -2, -2 );
	cFrame.right = cFrame.left + GetPreferredSize( false ).x;

	pcLayoutView->SetFrame( cFrame );	
}
