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

#include <cups/cups.h>

#include <util/application.h>
#include <util/message.h>
#include <util/resources.h>

#include <printers.h>
#include <print_view.h>
#include <config.h>

#include "resources/Printers.h"

#define PRINTERS_VERSION "1.0"

using namespace os;

PrintersWindow::PrintersWindow( const Rect &cFrame ) : Window( cFrame, "printers", MSG_MAINWND_TITLE )
{
	Rect cBounds = GetBounds();
	Resources cRes( get_image_id() );

	/* Menu */
	Rect cMenuFrame = cBounds;
	cMenuFrame.bottom = 18;

	m_pcMenuBar = new Menu( cMenuFrame, "Queues", ITEMS_IN_ROW );

	Menu *pcApplicationMenu = new Menu( Rect(), MSG_MAINWND_MENU_APPLICATION, ITEMS_IN_COLUMN );
	pcApplicationMenu->AddItem( MSG_MAINWND_MENU_APPLICATION_SAVE, new Message( M_WINDOW_SAVE ) );
	pcApplicationMenu->AddItem( MSG_MAINWND_MENU_APPLICATION_CANCEL, new Message( M_WINDOW_CANCEL ) );
	pcApplicationMenu->AddItem( new MenuSeparator() );
	pcApplicationMenu->AddItem( MSG_MAINWND_MENU_APPLICATION_ABOUT, new Message( M_APP_ABOUT ) );
	m_pcMenuBar->AddItem( pcApplicationMenu );

	Menu *pcPrintersMenu = new Menu( Rect(), MSG_MAINWND_MENU_PRINTER, ITEMS_IN_COLUMN );
	pcPrintersMenu->AddItem( MSG_MAINWND_MENU_PRINTER_ADD, new Message( M_ADD_PRINTER ) );
	pcPrintersMenu->AddItem( MSG_MAINWND_MENU_PRINTER_REMOVE, new Message( M_REMOVE_PRINTER ) );
	pcPrintersMenu->AddItem( new MenuSeparator() );
	pcPrintersMenu->AddItem( MSG_MAINWND_MENU_PRINTER_DEFAULT, new Message( M_DEFAULT_PRINTER ) );
	m_pcMenuBar->AddItem( pcPrintersMenu );

	m_vMenuHeight = m_pcMenuBar->GetPreferredSize( true ).y - 1.0f;
	cMenuFrame.bottom = m_vMenuHeight;
	m_pcMenuBar->SetFrame( cMenuFrame );
	m_pcMenuBar->SetTargetForItems( this );
	AddChild( m_pcMenuBar );

	/* Toolbar */
	Rect cToolBarFrame = cBounds;
	cToolBarFrame.top = m_vMenuHeight + 1;
	cToolBarFrame.bottom = cToolBarFrame.top + 48;

	m_pcToolBar = new ToolBar( cToolBarFrame, "Printers", CF_FOLLOW_LEFT | CF_FOLLOW_TOP | CF_FOLLOW_RIGHT );

	BitmapImage *pcToolBarIcon;

	pcToolBarIcon = new BitmapImage();
	pcToolBarIcon->Load( cRes.GetResourceStream( "add.png" ) );
	m_pcToolBar->AddButton( "add", MSG_MAINWND_TOOLBAR_ADD, pcToolBarIcon, new Message( M_ADD_PRINTER ) );

	pcToolBarIcon = new BitmapImage();
	pcToolBarIcon->Load( cRes.GetResourceStream( "remove.png" ) );
	m_pcToolBar->AddButton( "remove", MSG_MAINWND_TOOLBAR_REMOVE, pcToolBarIcon, new Message( M_REMOVE_PRINTER ) );

	m_pcToolBar->AddSeparator( "" );

	pcToolBarIcon = new BitmapImage();
	pcToolBarIcon->Load( cRes.GetResourceStream( "default.png" ) );
	m_pcToolBar->AddButton( "default", MSG_MAINWND_TOOLBAR_DEFAULT, pcToolBarIcon, new Message( M_DEFAULT_PRINTER ) );

	AddChild( m_pcToolBar );

	/* Printers & configuration View */
	Rect cUpperBounds;
	cUpperBounds.left = cBounds.left + 10;
	cUpperBounds.top = cToolBarFrame.bottom + 10;
	cUpperBounds.right = cBounds.right - 10;
	cUpperBounds.bottom = cBounds.bottom - 45 ;

	m_pcTreeView = new TreeView( Rect(), "print_queues", ( ListView::F_RENDER_BORDER | ListView::F_NO_HEADER | ListView::F_NO_AUTO_SORT ), CF_FOLLOW_TOP | CF_FOLLOW_BOTTOM );
	m_pcTreeView->InsertColumn( "Printers", 1 );
	m_pcTreeView->SetSelChangeMsg( new Message( M_SELECT_PRINTER ) );

	/* The selected item is displayed inside this View */
	m_pcView = new View( Rect(), "printers_window_view", CF_FOLLOW_ALL );

	m_pcSplitter = new Splitter( cUpperBounds, "Printers", m_pcTreeView, m_pcView, HORIZONTAL, CF_FOLLOW_ALL );
	m_pcSplitter->SetSplitRatio( 0.30f );
	AddChild( m_pcSplitter );

	/* The Apply/Save/Cancel buttons live in a layoutview of their own */
	Rect cButtonFrame = cBounds;
	cButtonFrame.top = cButtonFrame.bottom - 25;

	m_pcLayoutView = new LayoutView( cButtonFrame, "printers_window", NULL, CF_FOLLOW_BOTTOM | CF_FOLLOW_RIGHT );

	VLayoutNode *pcNode = new VLayoutNode( "printers_window_root" );
	pcNode->SetBorders( Rect( 5, 4, 5, 4 ) );
	pcNode->AddChild( new VLayoutSpacer( "printers_window_v_spacer", 1.0f ) );

	HLayoutNode *pcButtons = new HLayoutNode( "printers_window_buttons" );
	pcButtons->AddChild( new HLayoutSpacer( "printers_window_h_spacer" ) );
	pcButtons->AddChild( new Button( Rect(), "printers_window_cancel", MSG_MAINWND_BUTTONS_CANCEL, new Message( M_WINDOW_CANCEL ) ), 0.0f );
	pcButtons->AddChild( new HLayoutSpacer( "printers_window_h_spacer", 0.5f, 0.5f, pcButtons, 0.1f ) );
	pcButtons->AddChild( new Button( Rect(), "printers_window_save", MSG_MAINWND_BUTTONS_SAVE, new Message( M_WINDOW_SAVE ) ), 0.0f );

	pcButtons->SameWidth( "printers_window_cancel", "printers_window_save", NULL );

	pcNode->AddChild( pcButtons );

	m_pcLayoutView->SetRoot( pcNode );
	AddChild( m_pcLayoutView );

	/* Set the application icon */
	BitmapImage	*pcAppIcon = new BitmapImage();
	pcAppIcon->Load( cRes.GetResourceStream( "icon24x24.png" ) );
	SetIcon( pcAppIcon->LockBitmap() );
	delete( pcAppIcon );

	/* Load config */
	ReadConfig();

	/* Populate the printers list */
	m_pcCurrentPrintView = NULL;
	LoadPrinters();

	/* Set some sensible GUI limits */
	SetSizeLimits( Point( 450, 400 ), Point( 4096, 4096 ) );
	m_pcSplitter->SetSplitLimits( 100, 300 );
}

PrintersWindow::~PrintersWindow()
{
}

void PrintersWindow::HandleMessage( Message *pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case M_APP_ABOUT:
		{
			String cAbout, cCUPS;

			cAbout  = MSG_ABOUTWND_TEXT1 + String( " " ) + String( PRINTERS_VERSION );
			cAbout += String( "\n\n" );

			cCUPS.Format( MSG_ABOUTWND_TEXT2.c_str(), CUPS_VERSION_MAJOR, CUPS_VERSION_MINOR, CUPS_VERSION_PATCH );
			cAbout += cCUPS;

			Resources cRes( get_image_id() );
			BitmapImage *pcAboutIcon = new BitmapImage();
			pcAboutIcon->Load( cRes.GetResourceStream( "icon48x48.png" ) );

			Alert *pcAlert = new Alert( MSG_ABOUTWND_TITLE, cAbout, pcAboutIcon->LockBitmap(), 0x00, MSG_ABOUTWND_CLOSE.c_str(), NULL );
			pcAlert->CenterInWindow( this );
			pcAlert->Go( new Invoker( new Message( M_ALERT_DONE ) ) );

			break;
		}

		case M_ALERT_DONE:
			break;

		case M_SELECT_PRINTER:
		{
			if( m_pcCurrentPrintView != NULL )
				m_pcView->RemoveChild( m_pcCurrentPrintView );

			m_pcCurrentPrintView = m_vpcPrintViews[ m_pcTreeView->GetLastSelected() ];
			m_pcCurrentPrintView->SetFrame( m_pcView->GetBounds() );
			m_pcView->AddChild( m_pcCurrentPrintView );

			break;
		}

		case M_ADD_PRINTER:
		{
			CUPS_Printer *pcPrinter = new CUPS_Printer();

			m_vpcPrinters.push_back( pcPrinter );
			AppendPrinter( pcPrinter );

			m_pcTreeView->Select( m_pcTreeView->GetRowCount() - 1, true );

			break;
		}

		case M_DEFAULT_PRINTER:
		{
			std::vector< PrintView *>::iterator i;
			for( i = m_vpcPrintViews.begin(); i != m_vpcPrintViews.end(); i++ )
				(*i)->SetDefault( false );

			if( m_pcCurrentPrintView )
				m_pcCurrentPrintView->SetDefault( true );

			ReloadPrinters();

			break;
		}

		case M_REMOVE_PRINTER:
		{
			if( m_pcTreeView->GetRowCount() > 0 )
			{
				Message *pcAlertMessage = new Message( M_REMOVE_PRINTER_DIALOG );
				pcAlertMessage->AddInt32( "index", m_pcTreeView->GetLastSelected() );

				Alert *pcAlert = new Alert( MSG_ALRTWND_REMOVE_TITLE, MSG_ALRTWND_REMOVE_TEXT, Alert::ALERT_QUESTION, 0x00, MSG_ALRTWND_REMOVE_YES.c_str(), MSG_ALRTWND_REMOVE_NO.c_str(), NULL );
				pcAlert->CenterInWindow( this );
				pcAlert->Go( new Invoker( pcAlertMessage, this ) );
			}
			break;
		}

		case M_REMOVE_PRINTER_DIALOG:
		{
			/* Check if OK or Cancel was selected */
			int32 nWhich;
			if( pcMessage->FindInt32( "which", &nWhich ) == EOK )
				if( nWhich == 0 )
				{
					int32 nIndex;
					if( pcMessage->FindInt32( "index", &nIndex ) == EOK )
						RemovePrinter( nIndex );
				}
			break;
		}

		case M_WINDOW_SAVE:
		{
			std::vector< PrintView *>::iterator i;
			uint n;
			for( n = 0, i = m_vpcPrintViews.begin(); i != m_vpcPrintViews.end(); n++, i++ )
				m_vpcPrinters[n] = (*i)->Save();

			status_t nError = WriteConfig();
			if( nError != EOK )
			{
				Alert *pcAlert = new Alert( MSG_ALRTWND_SAVE_TITLE, MSG_ALRTWND_SAVE_TEXT, Alert::ALERT_WARNING, 0x00, MSG_ALRTWND_SAVE_CLOSE.c_str(), NULL );
				pcAlert->CenterInWindow( this );
				pcAlert->Go( new Invoker( new Message( M_ALERT_DONE ) ) );

				ReloadPrinters();
			}
			else
				OkToQuit();

			break;
		}

		case M_WINDOW_CANCEL:
		{
			OkToQuit();
			break;
		}

		default:
			Window::HandleMessage( pcMessage );
	}
}

bool PrintersWindow::OkToQuit( void )
{
	Application::GetInstance()->PostMessage( M_QUIT );
	return true;
}

/* Get the list of printers and populate the Printers TreeView */
void PrintersWindow::LoadPrinters( void )
{
	m_pcTreeView->Hide();
	m_pcTreeView->Clear();

	std::vector <CUPS_Printer*>::iterator i;
	for( i = m_vpcPrinters.begin(); i != m_vpcPrinters.end(); i++ )
		AppendPrinter( (*i) );

	m_pcTreeView->Show();
	m_pcTreeView->Select( 0, true );
}

void PrintersWindow::AppendPrinter( CUPS_Printer *pcPrinter )
{
	/* Create a new node */
	TreeViewStringNode *pcNode = new TreeViewStringNode();

	String cName = pcPrinter->cName;
	if( cName == "" )
		cName = MSG_MAINWND_DEFAULT_PRINTER_NAME;
	pcNode->AppendString( cName );

	BitmapImage *pcIcon;
	Resources cRes( get_image_id() );

	if( pcPrinter->bDefault )
	{
		pcIcon = new BitmapImage();
		pcIcon->Load( cRes.GetResourceStream( "default_printer.png" ) );
	}
	else
	{
		pcIcon = new BitmapImage();
		pcIcon->Load( cRes.GetResourceStream( "printer.png" ) );
	}
	pcNode->SetIcon( pcIcon );

	m_pcTreeView->InsertNode( pcNode );

	/* Create a PrintView */
	PrintView *pcPrintView = new PrintView( m_pcView->GetBounds(), pcPrinter, this );
	m_vpcPrintViews.push_back( pcPrintView );
}

void PrintersWindow::RemovePrinter( uint nIndex )
{
	std::vector <CUPS_Printer*>::iterator i;
	std::vector< PrintView *>::iterator j;
	uint n;
	for( n = 0, i = m_vpcPrinters.begin(), j = m_vpcPrintViews.begin(); i != m_vpcPrinters.end(), n != nIndex; n++, i++, j++ )
		;

	/* XXXKV: Blergh.  Why does calling RemoveChild() for this View not work? */
	if( (*j) == m_pcCurrentPrintView )
		m_pcCurrentPrintView->Hide();

	m_vpcPrinters.erase(i);
	m_vpcPrintViews.erase(j);

	ReloadPrinters();
}

void PrintersWindow::ReloadPrinters( void )
{
	BitmapImage *pcIcon;
	Resources cRes( get_image_id() );
	uint nSelected;

	nSelected = m_pcTreeView->GetLastSelected();

	m_pcTreeView->Hide();
	m_pcTreeView->Clear();

	std::vector <CUPS_Printer*>::iterator i;
	for( i = m_vpcPrinters.begin(); i != m_vpcPrinters.end(); i++ )
	{
		pcIcon = new BitmapImage();
		pcIcon->Load( cRes.GetResourceStream( "printer.png" ) );

		/* Create a new node */
		TreeViewStringNode *pcNode = new TreeViewStringNode();
		pcNode->AppendString( (*i)->cName );

		if( (*i)->bDefault )
		{
			pcIcon = new BitmapImage();
			pcIcon->Load( cRes.GetResourceStream( "default_printer.png" ) );
		}
		else
		{
			pcIcon = new BitmapImage();
			pcIcon->Load( cRes.GetResourceStream( "printer.png" ) );
		}
		pcNode->SetIcon( pcIcon );

		m_pcTreeView->InsertNode( pcNode );
	}
	m_pcTreeView->Show();

	nSelected = std::min( m_pcTreeView->GetRowCount() - 1, nSelected );
	m_pcTreeView->Select( nSelected, true );
}

class PrintersApplication : public Application
{
	public:
		PrintersApplication();

	private:
		PrintersWindow *m_pcWindow;
};

PrintersApplication::PrintersApplication() : Application( "application/x-VND.syllable-Printers" )
{
	SetCatalog("Printers.catalog");

	m_pcWindow = new PrintersWindow( Rect( 100,100,700,600 ) );
	m_pcWindow->Show();
	m_pcWindow->MakeFocus();
}

int main( void )
{
	PrintersApplication *pcApplication = new PrintersApplication();
	pcApplication->Run();

	return EXIT_SUCCESS;
}

