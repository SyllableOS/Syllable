/*
 *  Syllable Printer configuration preferences
 *
 *  (C)opyright 2006 - Kristian Van Der Vliet
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <storage/file.h>
#include <gui/exceptions.h>
#include <gui/requesters.h>

#include <printers.h>
#include <model_window.h>

#include "resources/Printers.h"

using namespace os;

ModelWindow::ModelWindow( const Rect &cFrame, Handler *pcParent ) : Window( cFrame, "ppds", MSG_SELECTWND_TITLE )
{
	m_pcParent = pcParent;

	Rect cBounds = GetBounds();

	Rect cTreeFrame = cBounds;
	cTreeFrame.top += 10;
	cTreeFrame.left += 10;
	cTreeFrame.right -= 10;
	cTreeFrame.bottom -= 35;

	m_pcTreeView = new TreeView( cTreeFrame, "ppd_window_treeview", ( ListView::F_NO_HEADER | ListView::F_RENDER_BORDER ), CF_FOLLOW_ALL );
	m_pcTreeView->InsertColumn( "Models", 1 );
	AddChild( m_pcTreeView );

	/* The Apply/Save/Cancel buttons live in a layoutview of their own */
	Rect cButtonFrame = cBounds;
	cButtonFrame.top = cTreeFrame.bottom + 10;

	m_pcLayoutView = new LayoutView( cButtonFrame, "ppd_window_layout", NULL, CF_FOLLOW_BOTTOM | CF_FOLLOW_RIGHT );

	VLayoutNode *pcNode = new VLayoutNode( "ppd_window_root" );
	pcNode->SetBorders( Rect( 5, 4, 5, 4 ) );
	pcNode->AddChild( new VLayoutSpacer( "ppd_window_v_spacer", 1.0f ) );

	HLayoutNode *pcButtons = new HLayoutNode( "ppd_window_buttons" );
	pcButtons->AddChild( new HLayoutSpacer( "ppd_window_h_spacer" ) );
	pcButtons->AddChild( new Button( Rect(), "ppd_window_cancel", MSG_SELECTWND_BUTTON_CANCEL, new Message( M_MODEL_WINDOW_CANCEL ) ), 0.0f );
	pcButtons->AddChild( new HLayoutSpacer( "ppd_window_h_spacer", 0.5f, 0.5f, pcButtons, 0.1f ) );
	pcButtons->AddChild( new Button( Rect(), "ppd_window_save", MSG_SELECTWND_BUTTON_SELECT, new Message( M_MODEL_WINDOW_SELECT ) ), 0.0f );

	pcButtons->SameWidth( "ppd_window_cancel", "ppd_window_save", NULL );

	pcNode->AddChild( pcButtons );

	m_pcLayoutView->SetRoot( pcNode );
	AddChild( m_pcLayoutView );

	/* Load the list of printers */
	LoadModels();

	SetSizeLimits( Point( 200, 200 ), Point( 4096, 4096 ) );
}

ModelWindow::~ModelWindow()
{
}

void ModelWindow::HandleMessage( Message *pcMessage )
{
	Messenger cMessenger( m_pcParent );

	switch( pcMessage->GetCode() )
	{
		case M_MODEL_WINDOW_SELECT:
		{
			String cModel, cPPD;

			int nSelected = m_pcTreeView->GetLastSelected();
			if( nSelected >= 0 )
			{
				ListViewRow *pcRow = m_pcTreeView->GetRow( nSelected );

				cModel = static_cast<TreeViewStringNode *>( pcRow )->GetString( 0 );
				cPPD = pcRow->GetCookie().AsString();
			}

			Message *pcCloseMessage = new Message( M_MODEL_WINDOW_CLOSED );
			pcCloseMessage->AddString( "model", cModel );
			pcCloseMessage->AddString( "ppd", cPPD );

			cMessenger.SendMessage( pcCloseMessage );

			Close();

			break;
		}

		case M_MODEL_WINDOW_CANCEL:
		{
			cMessenger.SendMessage( M_MODEL_WINDOW_CLOSED );
			Close();
			break;
		}

		default:
			Window::HandleMessage( pcMessage );
	}
}

bool ModelWindow::OkToQuit( void )
{
	return true;
}

/* Parse the printers.list file and populate the TreeView */
status_t ModelWindow::LoadModels( void )
{
	status_t nError = EOK;
	FILE *hPrintersList;

	hPrintersList = fopen( MODELS_LIST, "r" );
	if( hPrintersList != NULL )
	{
		char zManf[1024], zModel[1024], zPPD[1024];
		char zCurrentManf[1024] = {0};

		/* The printers.list file is generated for Syllable by our own script.
		   The format is very simple, with three columns seperated by tabs.
		   The first coloumn if the manufacturer name.  The second column is
		   the model name.  The third column is the PPD to use. */

		while( fscanf( hPrintersList, "%s\t%[^\t]\t%s\n", zManf, zModel, zPPD ) != EOF )
		{
			//fprintf( stderr, "Manf: %s\nModel: %s\nPPD: %s\n", zManf, zModel, zPPD );
			if( strncmp( zCurrentManf, zManf, 1024 ) != 0 )
			{
				/* New manufacturer */
				TreeViewStringNode *pcManfNode = new TreeViewStringNode();
				pcManfNode->AppendString( zManf );
				pcManfNode->SetIndent( 1 );

				m_pcTreeView->InsertNode( pcManfNode, false );
				strncpy( zCurrentManf, zManf, 1024 );
			}

			TreeViewStringNode *pcModelNode = new TreeViewStringNode();
			pcModelNode->AppendString( zModel );
			pcModelNode->SetIndent( 2 );
			pcModelNode->SetCookie( String( zPPD ) );

			m_pcTreeView->InsertNode( pcModelNode, true );

			zManf[0] = zModel[0] = zPPD[0] = '\0';
		}
		fclose( hPrintersList );
	}
	else
		nError = errno;

#if 1
	/* XXXKV: There isn't any way to add everything to a TreeView with the nodes already
	   collapsed, so we have to add everything then go back and collapse every node...*/
	TreeViewStringNode *pcNode;
	int n=0;
	while( pcNode = static_cast<TreeViewStringNode *>( m_pcTreeView->GetRow( n++ ) ) )
		m_pcTreeView->Collapse( pcNode );

	/* XXXKV: HACK!  The TreeView doesn't draw itself intially.  Calling
	   Invalidate()/Flush() doesn't do it either.  This spooks the View into
	   thinking it's been resized and *forces* a redraw, but it's not Standard Practice */
	m_pcTreeView->FrameSized( Point( 0, 0 ) );
#else
	/* XXXKV: Fixme! */
	m_pcTreeView->Invalidate();
	m_pcTreeView->Flush();
#endif

	return nError;
}

