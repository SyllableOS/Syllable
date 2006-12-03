//  AEdit -:-  (C)opyright 2005-2006 Jonas Jarvoll
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

#include <util/message.h>
#include <gui/stringview.h>

#include "aboutdialog.h"
#include "main.h"
#include "buffer.h"
#include "messages.h"
#include "resources/aedit.h"
#include "version.h"

using namespace os;

AboutDialog::AboutDialog() : Window( Rect(), "about_dialog", MSG_ABOUT_TITLE, WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT | WND_NOT_RESIZABLE)
{
	// Create layout
	pcLayoutView = new LayoutView( Rect(), "" );
	AddChild( pcLayoutView );

	// Create basepanel	
	pcRoot = new VLayoutNode( "" );
	pcRoot->SetVAlignment( os::ALIGN_CENTER );
	pcRoot->SetHAlignment( os::ALIGN_CENTER );
	pcLayoutView->SetRoot( pcRoot );

	// Create the AEdit image
	pcAEditImage = new BitmapImage();
	Resources cRes( get_image_id() );
	ResStream* pcStream = cRes.GetResourceStream( "about" );

    if( pcStream != NULL)
        pcAEditImage->Load( pcStream );

	pcAEditImageView = new ImageView( Rect(), "aedit_image", pcAEditImage );
	pcRoot->AddChild( pcAEditImageView );

	pcRoot->AddChild( new VLayoutSpacer( "", 8, 8 ) );

	// Create version label
	String version;
	version = MSG_ABOUT_VERSION + AEDIT_VERSION;
	pcVersionLabel = new StringView( Rect(), "version_label", version, ALIGN_RIGHT );
	pcRoot->AddChild( pcVersionLabel );
	Font* f = pcVersionLabel->GetFont();
	f->SetSize( 8 );
	pcVersionLabel->SetFont( f );

	pcRoot->AddChild( new VLayoutSpacer( "", 6, 6 ) );

	// Create syllable label
	pcTextEditorLabel = new StringView( Rect(), "texteditor_label", MSG_ABOUT_LINE_1, ALIGN_CENTER );
	pcRoot->AddChild( pcTextEditorLabel );

	pcRoot->AddChild( new VLayoutSpacer( "", 4, 4 ) );

	pcTextEditorLabel = new StringView( Rect(), "texteditor_label", MSG_ABOUT_LINE_2, ALIGN_CENTER );
	pcRoot->AddChild( pcTextEditorLabel );

	pcRoot->AddChild( new VLayoutSpacer( "", 4, 4 ) );

	pcTextEditorLabel = new StringView( Rect(), "texteditor_label", MSG_ABOUT_LINE_3, ALIGN_CENTER );
	pcRoot->AddChild( pcTextEditorLabel );

	pcRoot->AddChild( new VLayoutSpacer( "", 8, 8 ) );

	// Create close button
	pcCloseButton = new Button( Rect(), "close_button", MSG_ABOUT_CLOSE, new Message( M_BUT_ABOUT_CLOSE ) );
	pcCloseButton->SetTarget( this );
	pcRoot->AddChild( pcCloseButton );
	SetDefaultButton( pcCloseButton );
	
	// Set size of window
	Point size = pcLayoutView->GetPreferredSize( false );
	ResizeTo( size.x + 20, size.y + 20 );
	CenterInScreen();
}

AboutDialog :: ~AboutDialog()
{
	delete pcLayoutView;
}

void AboutDialog::HandleMessage(Message* pcMessage)
{
	if( pcMessage->GetCode() == M_BUT_ABOUT_CLOSE)
		_Close();
}

bool AboutDialog::OkToQuit()
{
	_Close();
	return false;
}

void AboutDialog::Raise()
{
	if( IsVisible() )
		Show( false );

	Show( true );

	MakeFocus();
}

void AboutDialog :: FrameSized( const Point& cDelta )
{
	Window::FrameSized( cDelta );

	Rect cFrame = GetBounds();
	cFrame.Resize( 2, 2, -2, -2 );
	pcLayoutView->SetFrame( cFrame );	
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// P R I V A T E
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void AboutDialog :: _Close()
{
	// Close the About window
	Hide();

	Buffer* current = AEditApp::GetAEditWindow()->pcCurrentBuffer;

	if( current != NULL )
		current->MakeFocus();
}
