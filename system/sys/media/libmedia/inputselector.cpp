/*  Media Input Selector class
 *  Copyright (C) 2003 Arno Klenke
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

#include <media/inputselector.h>
#include <gui/requesters.h>

using namespace os;

enum {
	MEDIA_INPUT_SELECTOR_CHANGE,
	MEDIA_INPUT_SELECTOR_FILES,
	MEDIA_INPUT_SELECTOR_OPEN
};

class MediaInputSelector::Private
{
public:
	bool m_bAutoDetect;
	MediaManager* m_pcManager;
	Message* m_pcOpenMessage;
	Message* m_pcCancelMessage;
	Messenger* m_pcTarget;
	DropdownMenu* m_pcInputList;
	StringView* m_pcInputLabel;
	TextView* m_pcFileInput;
	StringView* m_pcFileLabel;
	Button* m_pcFileButton;
	Button* m_pcOpenButton;
	FileRequester* m_pcFileDialog;
};


/** Open new input selector.
 * \par Description:
 * Opens a new input selector.
 * \param cPosition - Position of the window.
 * \param zTitle - Title of the created window.
 * \param pcTarget - Target to where the event messages should be send.
 * \param pcOpenMessage - This message is sent if a file / device has been selected.
 *						  "file/path" includes the filename and "input" the identifier
 *						  of the input.
 * \param pcCancelMessage - This message is sent if the selector has been closed without
 *							selecting a file.
 * \param bAutoDetect - If true then the system will try to autodetect the required input for the
 *						file / device ( although it is possible to force a special file / device ).
 *						Otherwise the default input will be selected in the list of inputs.
 * \author	Arno Klenke
 *****************************************************************************/
MediaInputSelector::MediaInputSelector( const Point cPosition, const String zTitle, Messenger* pcTarget, Message* pcOpenMessage, 
						Message* pcCancelMessage, bool bAutoDetect )
						:Window( os::Rect( cPosition.x, cPosition.y, cPosition.x + 200, cPosition.y + 85 ), "media_input_selector", 
							zTitle, WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT | WND_NOT_V_RESIZABLE )
					
{
	m = new Private;
	
	/* Save parameters */
	m->m_pcTarget = pcTarget;
	m->m_pcOpenMessage = pcOpenMessage;
	m->m_pcCancelMessage = pcCancelMessage;
	m->m_bAutoDetect = bAutoDetect;
	
	/* Create dropdown menu with all input plugins */
	m->m_pcInputLabel = new StringView( Rect( 0, 0, 1, 1 ), "input_label", "Input" );
	m->m_pcInputLabel->SetFrame( Rect( 5, 10, m->m_pcInputLabel->GetPreferredSize( false ).x + 5, 
									m->m_pcInputLabel->GetPreferredSize( false ).y + 10 ) );
	m->m_pcInputList = new DropdownMenu( Rect( 0, 0, 1, 1 ), "input_list", CF_FOLLOW_LEFT | CF_FOLLOW_TOP |
																			CF_FOLLOW_RIGHT );
	m->m_pcInputList->SetFrame( Rect( m->m_pcInputLabel->GetPreferredSize( false ).x + 10, 5, 190,
							 m->m_pcInputList->GetPreferredSize( false ).y + 5 ) );
	m->m_pcInputList->SetSelectionMessage( new os::Message( MEDIA_INPUT_SELECTOR_CHANGE ) );
	m->m_pcInputList->SetTarget( this );
	/* Create file input field */
	m->m_pcFileLabel = new StringView( Rect( 0, 0, 1, 1 ), "file_label", "File" );
	m->m_pcFileLabel->SetFrame( Rect( 5, 40, m->m_pcFileLabel->GetPreferredSize( false ).x + 5, 
									m->m_pcFileLabel->GetPreferredSize( false ).y + 40 ) );
	m->m_pcFileInput = new TextView( Rect( 0, 0, 1, 1 ), "file_input", "", CF_FOLLOW_LEFT | CF_FOLLOW_TOP |
																			CF_FOLLOW_RIGHT );
	m->m_pcFileInput->SetFrame( Rect( m->m_pcInputLabel->GetPreferredSize( false ).x + 10, 35, 170,
							 m->m_pcFileInput->GetPreferredSize( false ).y + 35 ) );
	m->m_pcFileInput->SetMultiLine( false );
	m->m_pcFileButton = new Button( Rect( 172, 35, 190, m->m_pcFileInput->GetPreferredSize( false ).y + 35 ),
							"file_button", "..", new Message( MEDIA_INPUT_SELECTOR_FILES ), CF_FOLLOW_RIGHT | CF_FOLLOW_TOP );
	/* Create open button */
	m->m_pcOpenButton = new Button( Rect( 0, 0, 1, 1 ),
							"open_button", "Open", new Message( MEDIA_INPUT_SELECTOR_OPEN ), CF_FOLLOW_RIGHT | CF_FOLLOW_BOTTOM );
	m->m_pcOpenButton->SetFrame( Rect( 150, 60, 190, m->m_pcOpenButton->GetPreferredSize( false ).y + 60 ) );
	
	/* Add autodetect item */
	if( bAutoDetect ) {
		m->m_pcInputList->AppendItem( "Autodetect" );
		m->m_pcInputList->SetSelection( 0 );
	}
	/* Add all inputs */
	m->m_pcManager = MediaManager::GetInstance();
	if( m->m_pcManager == NULL )
	{
		dbprintf( "Input selector constructed without media manager!\n" );
		return;
	}
	MediaInput* pcInput;
	MediaInput* pcDefaultInput = m->m_pcManager->GetDefaultInput();
	int i = 0;
	while( ( pcInput = m->m_pcManager->GetInput( i ) ) != NULL )
	{
		m->m_pcInputList->AppendItem( pcInput->GetIdentifier().c_str() );
		/* Select default input */
		if( !bAutoDetect && pcDefaultInput && 
				pcDefaultInput->GetIdentifier() == pcInput->GetIdentifier() )
		{
			m->m_pcInputList->SetSelection( i );
		}
		pcInput->Release();
		i++;
	}
	if( pcDefaultInput )
		pcDefaultInput->Release();
		
	/* Look if we have to disable the file input field */
	if( !bAutoDetect ) {
		pcInput = m->m_pcManager->GetInput( m->m_pcInputList->GetSelection() );
		if( pcInput && !pcInput->FileNameRequired() ) {
			m->m_pcFileInput->SetEnable( false );
			m->m_pcFileButton->SetEnable( false );
		}
		if( pcInput )
			pcInput->Release();
	}
	
	/* Create file selector */
	m->m_pcFileDialog = new FileRequester( FileRequester::LOAD_REQ, new os::Messenger( this ), "", FileRequester::NODE_FILE, false );
	m->m_pcFileDialog->Lock();
	m->m_pcFileDialog->Start();
	m->m_pcFileDialog->Unlock();
	
	AddChild( m->m_pcFileLabel );
	AddChild( m->m_pcFileInput );
	AddChild( m->m_pcFileButton );
	AddChild( m->m_pcOpenButton );
	AddChild( m->m_pcInputLabel );
	AddChild( m->m_pcInputList );
	
	SetFocusChild( m->m_pcInputList );
	
}


MediaInputSelector::~MediaInputSelector()
{
	/* Delete everything */
	
	RemoveChild( m->m_pcFileInput );
	RemoveChild( m->m_pcFileLabel );
	RemoveChild( m->m_pcInputLabel );
	RemoveChild( m->m_pcInputList );
	RemoveChild( m->m_pcFileButton );
	RemoveChild( m->m_pcOpenButton );
	delete( m->m_pcFileInput );
	delete( m->m_pcFileLabel );
	delete( m->m_pcInputLabel );
	delete( m->m_pcInputList );
	delete( m->m_pcFileButton );
	delete( m->m_pcOpenButton );
	m->m_pcFileDialog->Close();
	if( m->m_pcTarget );
		delete( m->m_pcTarget );
	if( m->m_pcOpenMessage );
		delete( m->m_pcOpenMessage );
	if( m->m_pcCancelMessage );
		delete( m->m_pcCancelMessage );
		
	delete( m );
}

bool MediaInputSelector::OkToQuit()
{
	if( m->m_pcTarget && m->m_pcCancelMessage ) {
		m->m_pcTarget->SendMessage( m->m_pcCancelMessage );
	}
	return( true );
}

void MediaInputSelector::HandleMessage( Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case MEDIA_INPUT_SELECTOR_OPEN:
		{
			/* If autodetect is enabled try to find out the right input */
			bool bSuccess = false;
			MediaInput* pcInput;
			if( m->m_bAutoDetect && m->m_pcInputList->GetSelection() == 0 ) {
				pcInput = m->m_pcManager->GetBestInput( m->m_pcFileInput->GetBuffer()[0] );
				if( pcInput != NULL ) {
					bSuccess = true;
				}
			} else {
				/* Use selected item */
				int nIndex = m->m_pcInputList->GetSelection();
				if( m->m_bAutoDetect )
					nIndex--;
				pcInput = m->m_pcManager->GetInput( nIndex );
				if( pcInput && pcInput->Open( m->m_pcFileInput->GetBuffer()[0] ) == 0 ) {
					pcInput->Close();
					bSuccess = true;
				}
			}
			if( !bSuccess ) {
				/* Show alert window */
				Alert* pcAlert = new Alert( m->m_pcFileInput->GetBuffer()[0], "This file / device is not supported by the system.", 
									Alert::ALERT_WARNING, WND_NOT_RESIZABLE, "Ok", NULL );
				pcAlert->Go();
			} else {
				/* Send success message */
				if( m->m_pcTarget && m->m_pcOpenMessage ) {
					m->m_pcOpenMessage->AddString( "file/path", m->m_pcFileInput->GetBuffer()[0] );
					m->m_pcOpenMessage->AddString( "input", pcInput->GetIdentifier() );
					m->m_pcTarget->SendMessage( m->m_pcOpenMessage );
				}
				pcInput->Release();
				delete( m->m_pcCancelMessage );
				m->m_pcCancelMessage = NULL;
				PostMessage( M_QUIT );
			}
		}
		break;
		case MEDIA_INPUT_SELECTOR_CHANGE:
			/* Enable or disable file input field */
			if( m->m_bAutoDetect && m->m_pcInputList->GetSelection() == 0 ) {
				m->m_pcFileInput->SetEnable( true );
				m->m_pcFileButton->SetEnable( true );
			} else {
				int nIndex = m->m_pcInputList->GetSelection();
				if( m->m_bAutoDetect )
					nIndex--;
				MediaInput* pcInput = m->m_pcManager->GetInput( nIndex );
				if( pcInput ) {
					m->m_pcFileInput->SetEnable( pcInput->FileNameRequired() );
					m->m_pcFileButton->SetEnable( pcInput->FileNameRequired() );
					pcInput->Release();
				}
			}
		break;
		case MEDIA_INPUT_SELECTOR_FILES:
			m->m_pcFileDialog->Show();
			m->m_pcFileDialog->MakeFocus( true );
		break;
		case M_LOAD_REQUESTED:
		{
			String zFile;
			if( pcMessage->FindString( "file/path", &zFile.str() ) == 0 ) {
				m->m_pcFileInput->Clear();
				m->m_pcFileInput->Insert( zFile.c_str() );
			}
		}
		break;
		default:
			Window::HandleMessage( pcMessage );
		break;
	}
}

void MediaInputSelector::_reserved1() {}
void MediaInputSelector::_reserved2() {}
void MediaInputSelector::_reserved3() {}
void MediaInputSelector::_reserved4() {}
void MediaInputSelector::_reserved5() {}
void MediaInputSelector::_reserved6() {}
void MediaInputSelector::_reserved7() {}
void MediaInputSelector::_reserved8() {}
void MediaInputSelector::_reserved9() {}
void MediaInputSelector::_reserved10() {}

























