/*
 *  A CUPS print-queue manager for Syllable
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
#include <gui/rect.h>
#include <gui/window.h>
#include <gui/menu.h>
#include <gui/checkmenu.h>
#include <gui/splitter.h>
#include <gui/treeview.h>
#include <gui/listview.h>
#include <gui/checkmenu.h>
#include <gui/requesters.h>
#include <gui/toolbar.h>
#include "resources/PrintQueue.h"

#define QUEUE_VERSION "1.0"

using namespace os;

enum messages
{
	M_APP_QUIT,
	M_APP_ABOUT,
	M_VIEW_REFRESH,
	M_VIEW_COMPLETED,
	M_JOB_CANCEL,
	M_PRINTER_PAUSE,
	M_PRINTER_RESUME,
	M_ALERT_DONE,
	M_SELECT_PRINTER,
	M_SELECT_JOB,
	M_UPDATE
};

/* These don't seem to be defined in the CUPS headers, oddly */
enum printer_state
{
	STATE_IDLE = 3,
	STATE_STOPPED = 5
};

class QueueWindow : public Window
{
	public:
		QueueWindow( const Rect &cFrame );
		~QueueWindow();
		void HandleMessage( Message *pcMessage );
		bool OkToQuit( void );
		void TimerTick( int nID );

	private:
		void LoadPrinters( void );
		void DisplayJobs( const char *zPrinter );
		void EnableDisableJobMenus( ipp_jstate_t nState );
		status_t EnablePrinter( const char *zPrinter, bool bEnable );
		void EnableDisablePrinterMenus( int nState );

		Menu *m_pcMenuBar;
		float m_vMenuHeight;
		CheckMenu *m_pcItemViewCompleted;

		MenuItem *m_pcItemCancel;

		MenuItem *m_pcItemPause;
		MenuItem *m_pcItemResume;

		Menu *m_pcJobContextMenu;
		MenuItem *m_pcCtxItemCancel;

		Menu *m_pcPrinterContextMenu;
		MenuItem *m_pcCtxItemPause;
		MenuItem *m_pcCtxItemResume;

		ToolBar *m_pcToolBar;

		TreeView *m_pcPrinters;
		ListView *m_pcJobs;
		Splitter *m_pcSplitter;

		int m_nJobSelect;						/* -1 == Show all, 0 == Show active */
		TreeViewStringNode *m_pcCurrentPrinter;	/* Currently selected Printer */
};

QueueWindow::QueueWindow( const Rect &cFrame ) : Window( cFrame, "print_queue", MSG_MAINWND_TITLE )
{
	Rect cBounds = GetBounds();

	Rect cMenuFrame = cBounds;
	cMenuFrame.bottom = 18;

	m_pcMenuBar = new Menu( cMenuFrame, "Queues", ITEMS_IN_ROW );

	Menu *pcApplicationMenu = new Menu( Rect(), MSG_MAINWND_MENU_APPLICATION, ITEMS_IN_COLUMN );
	pcApplicationMenu->AddItem( MSG_MAINWND_MENU_APPLICATION_QUIT, new Message( M_APP_QUIT ) );
	pcApplicationMenu->AddItem( new MenuSeparator() );
	pcApplicationMenu->AddItem( MSG_MAINWND_MENU_APPLICATION_ABOUT, new Message( M_APP_ABOUT ) );
	m_pcMenuBar->AddItem( pcApplicationMenu );

	Menu *pcViewMenu = new Menu( Rect(), MSG_MAINWND_MENU_VIEW, ITEMS_IN_COLUMN );
	pcViewMenu->AddItem( MSG_MAINWND_MENU_VIEW_REFRESH, new Message( M_VIEW_REFRESH ) );
	pcViewMenu->AddItem( new MenuSeparator() );

	/* By default, only show active jobs */
	m_nJobSelect = 0;
	m_pcItemViewCompleted = new CheckMenu( MSG_MAINWND_MENU_VIEW_COMPLETED, new Message( M_VIEW_COMPLETED ) );
	pcViewMenu->AddItem( m_pcItemViewCompleted );

	m_pcMenuBar->AddItem( pcViewMenu );

	Menu *pcJobMenu = new Menu( Rect(), MSG_MAINWND_MENU_JOB, ITEMS_IN_COLUMN );
	m_pcItemCancel = new MenuItem( MSG_MAINWND_MENU_JOB_CANCEL, new Message( M_JOB_CANCEL ) );
	pcJobMenu->AddItem( m_pcItemCancel );

	m_pcMenuBar->AddItem( pcJobMenu );

	Menu *pcPrinterMenu = new Menu( Rect(), MSG_MAINWND_MENU_PRINTER, ITEMS_IN_COLUMN );
	m_pcItemPause = new MenuItem( MSG_MAINWND_MENU_PRINTER_PAUSE, new Message( M_PRINTER_PAUSE ) );
	pcPrinterMenu->AddItem( m_pcItemPause );
	m_pcItemResume = new MenuItem( MSG_MAINWND_MENU_PRINTER_RESUME, new Message( M_PRINTER_RESUME ) );
	pcPrinterMenu->AddItem( m_pcItemResume );

	m_pcMenuBar->AddItem( pcPrinterMenu );

	m_vMenuHeight = m_pcMenuBar->GetPreferredSize( true ).y - 1.0f;
	cMenuFrame.bottom = m_vMenuHeight;
	m_pcMenuBar->SetFrame( cMenuFrame );
	m_pcMenuBar->SetTargetForItems( this );
	AddChild( m_pcMenuBar );

	cBounds.top = m_vMenuHeight + 1;

	/* Toolbar */
	Rect cToolBarFrame = cBounds;
	cToolBarFrame.top = m_vMenuHeight + 1;
	cToolBarFrame.bottom = cToolBarFrame.top + 48;

	m_pcToolBar = new ToolBar( cToolBarFrame, "Printers", CF_FOLLOW_LEFT | CF_FOLLOW_TOP | CF_FOLLOW_RIGHT );

	BitmapImage *pcToolBarIcon;
	Resources cRes( get_image_id() );

	pcToolBarIcon = new BitmapImage();
	pcToolBarIcon->Load( cRes.GetResourceStream( "cancel.png" ) );
	m_pcToolBar->AddButton( "cancel", MSG_MAINWND_TOOLBAR_CALCEL, pcToolBarIcon, new Message( M_JOB_CANCEL ) );

	m_pcToolBar->AddSeparator( "" );

	pcToolBarIcon = new BitmapImage();
	pcToolBarIcon->Load( cRes.GetResourceStream( "pause.png" ) );
	m_pcToolBar->AddButton( "pause", MSG_MAINWND_TOOLBAR_PAUSE, pcToolBarIcon, new Message( M_PRINTER_PAUSE ) );

	pcToolBarIcon = new BitmapImage();
	pcToolBarIcon->Load( cRes.GetResourceStream( "resume.png" ) );
	m_pcToolBar->AddButton( "resume", MSG_MAINWND_TOOLBAR_RESUME, pcToolBarIcon, new Message( M_PRINTER_RESUME ) );

	AddChild( m_pcToolBar );

	/* Printers & Jobs */
	cBounds.top = cToolBarFrame.bottom + 1;

	m_pcPrinters = new TreeView( cBounds, "Printers", ListView::F_RENDER_BORDER | ListView::F_NO_HEADER | ListView::F_NO_AUTO_SORT, CF_FOLLOW_ALL );
	m_pcPrinters->InsertColumn( "Printers", 1 );
	m_pcPrinters->SetSelChangeMsg( new Message( M_SELECT_PRINTER ) );

	m_pcJobs = new ListView( cBounds, "Documents", ListView::F_RENDER_BORDER | ListView::F_NO_AUTO_SORT, CF_FOLLOW_ALL );
	m_pcJobs->InsertColumn( MSG_MAINWND_DOCLIST_ID.c_str(), 25 );
	m_pcJobs->InsertColumn( MSG_MAINWND_DOCLIST_NAME.c_str(), 145 );
	m_pcJobs->InsertColumn( MSG_MAINWND_DOCLIST_STATUS.c_str(), 90 );
	m_pcJobs->InsertColumn( MSG_MAINWND_DOCLIST_OWNER.c_str(), 100 );
	m_pcJobs->InsertColumn( MSG_MAINWND_DOCLIST_SIZE.c_str(), 50 );
	m_pcJobs->InsertColumn( MSG_MAINWND_DOCLIST_SUBMITTED.c_str(), 100 );
	m_pcJobs->SetSelChangeMsg( new Message( M_SELECT_JOB ) );

	m_pcSplitter = new Splitter( cBounds, "Queues", m_pcPrinters, m_pcJobs, HORIZONTAL, CF_FOLLOW_ALL );
	m_pcSplitter->SetSplitRatio( 0.20f );
	AddChild( m_pcSplitter );

	/* A context menu for the Jobs ListView */
	m_pcJobContextMenu = new Menu( Rect(), "Jobs", ITEMS_IN_COLUMN );

	m_pcCtxItemCancel = new MenuItem( MSG_MAINWND_CONTEXTMENU_CANCEL, new Message( M_JOB_CANCEL ) );
	m_pcJobContextMenu->AddItem( m_pcCtxItemCancel );
	m_pcJobContextMenu->SetTargetForItems( this );

	m_pcJobs->SetContextMenu( m_pcJobContextMenu );

	/* A context menu for the Printers TreeView.  Note that the Menu and it's two Items are created but *NOT*
	   attached here!  One of the two Items will be attached to the Menu in EnableDisablePrinterMenus() */
	m_pcPrinterContextMenu = new Menu( Rect(), "Printer", ITEMS_IN_COLUMN );

	m_pcCtxItemPause = new MenuItem( MSG_MAINWND_CONTEXTMENU_PAUSE, new Message( M_PRINTER_PAUSE ) );
	m_pcCtxItemResume = new MenuItem( MSG_MAINWND_CONTEXTMENU_RESUME, new Message( M_PRINTER_RESUME ) );

	m_pcPrinters->SetContextMenu( m_pcPrinterContextMenu );

	/* Set the application icon */
	BitmapImage	*pcAppIcon = new BitmapImage();
	pcAppIcon->Load( cRes.GetResourceStream( "icon24x24.png" ) );
	SetIcon( pcAppIcon->LockBitmap() );
	delete( pcAppIcon );

	/* Disable the job-specific menus */
	EnableDisableJobMenus( IPP_JOB_COMPLETED );

	/* Populate the printers list */
	LoadPrinters();

	/* Automatically refresh the Jobs list every 5 seconds */
	AddTimer( this, 0, 5 * 1000000, false );

	/* Set some sensible GUI limits */
	SetSizeLimits( Point( cFrame.Width(), 100 ), Point( 4096, 4096 ) );
	m_pcSplitter->SetSplitLimits( 100 );
}

QueueWindow::~QueueWindow()
{
}

void QueueWindow::HandleMessage( Message *pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case M_APP_ABOUT:
		{
			String cAbout, cCUPS;

			cAbout  = MSG_ABOUTWND_TEXT1 + String( " " ) + String( QUEUE_VERSION );
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

		case M_APP_QUIT:
		{
			OkToQuit();
			break;
		}

		case M_VIEW_REFRESH:
		{
			LoadPrinters();	/* This will cause an M_SELECT to be sent */
			break;
		}

		case M_VIEW_COMPLETED:
		{
			m_nJobSelect = m_pcItemViewCompleted->IsChecked() ? -1 : 0;
			/* Fall through */
		}

		case M_UPDATE:
		case M_SELECT_PRINTER:
		{
			int nSelected = m_pcPrinters->GetLastSelected();
			if( nSelected >= 0 )
			{
				m_pcCurrentPrinter = static_cast<TreeViewStringNode*>( m_pcPrinters->GetRow( nSelected ) );

				/* Display all jobs for the selected printer */
				DisplayJobs( m_pcCurrentPrinter->GetString( 0 ).c_str() );

				int nState = m_pcCurrentPrinter->GetCookie().AsInt32();
				EnableDisablePrinterMenus( nState );
			}
			break;
		}

		case M_PRINTER_PAUSE:
		{
			if( EnablePrinter( m_pcCurrentPrinter->GetString( 0 ).c_str(), false ) != EOK )
			{
				/* Failed */
				String cFailure = cupsLastErrorString();

				Alert *pcAlert = new Alert( MSG_ALERTWND_PAUSE_TITLE, cFailure, Alert::ALERT_WARNING, 0x00, MSG_ALERTWND_PAUSE_CLOSE.c_str(), NULL );
				pcAlert->CenterInWindow( this );
				pcAlert->Go( new Invoker( new Message( M_ALERT_DONE ) ) );
			}
			LoadPrinters();
			break;
		}

		case M_PRINTER_RESUME:
		{
			if( EnablePrinter( m_pcCurrentPrinter->GetString( 0 ).c_str(), true ) != EOK )
			{
				/* Failed */
				String cFailure = cupsLastErrorString();

				Alert *pcAlert = new Alert( MSG_ALERTWND_RESUME_TITLE, cFailure, Alert::ALERT_WARNING, 0x00, MSG_ALERTWND_RESUME_CLOSE.c_str(), NULL );
				pcAlert->CenterInWindow( this );
				pcAlert->Go( new Invoker( new Message( M_ALERT_DONE ) ) );
			}
			LoadPrinters();
			break;
		}

		case M_SELECT_JOB:
		{
			int nSelected = m_pcJobs->GetLastSelected();
			if( nSelected >= 0 )
			{
				ListViewRow *pcRow = m_pcJobs->GetRow( nSelected );
				ipp_jstate_t nState = static_cast<ipp_jstate_t>( pcRow->GetCookie().AsInt32() );

				EnableDisableJobMenus( nState );
			}
			break;
		}

		case M_JOB_CANCEL:
		{
			int nSelected = m_pcJobs->GetLastSelected();
			if( nSelected >= 0 )
			{
				String cPrinter = m_pcCurrentPrinter->GetString( 0 );

				ListViewStringRow *pcRow = static_cast<ListViewStringRow *>( m_pcJobs->GetRow( nSelected ) );
				int nJob = atoi( pcRow->GetString( 0 ).c_str() );

				int nError = cupsCancelJob( cPrinter.c_str(), nJob );
				if( nError == 0 )
				{
					/* Failed */
					String cFailure = cupsLastErrorString();

					Alert *pcAlert = new Alert( MSG_ALERTWND_CANCEL_TITLE, cFailure, Alert::ALERT_WARNING, 0x00, MSG_ALERTWND_CANCEL_CLOSE.c_str(), NULL );
					pcAlert->CenterInWindow( this );
					pcAlert->Go( new Invoker( new Message( M_ALERT_DONE ) ) );
				}

				/* Refresh */
				DisplayJobs( cPrinter.c_str() );
			}
			break;
		}

		default:
			Window::HandleMessage( pcMessage );
	}
}

bool QueueWindow::OkToQuit( void )
{
	Application::GetInstance()->PostMessage( M_QUIT );
	return true;
}

void QueueWindow::TimerTick( int nID )
{
	if( nID == 0 )
		PostMessage( M_UPDATE );
}

/* Get the list of printers from CUPS and populate the Printers TreeView */
void QueueWindow::LoadPrinters( void )
{
	int n, m, nSelected, nCount;
	cups_dest_t *psDests;
	BitmapImage *pcIcon;

	Resources cRes( get_image_id() );

	/* Ensure we re-select the current Printer */
	nSelected = m_pcPrinters->GetLastSelected();
	if( nSelected < 0 )
		nSelected = 0;
	m_pcPrinters->Clear();

	nCount = cupsGetDests( &psDests );
	for( n = 0; n < nCount; n++ )
	{
		TreeViewStringNode *pcNode = new TreeViewStringNode();
		pcNode->AppendString( psDests[n].name );

		/* Is the print-queue enabled? */
		int nState = STATE_IDLE;
		for( m = 0; m < psDests[n].num_options; m++ )
		{
			cups_option_t sOption = psDests[n].options[m];
			if( strncmp( sOption.name, "printer-state", 13 ) == 0 )
			{
				nState = atoi( sOption.value );
				break;
			}
		}
		pcNode->SetCookie( nState );

		pcIcon = new BitmapImage();
		pcIcon->Load( cRes.GetResourceStream( "printer.png" ) );
		if( nState != STATE_IDLE )
			pcIcon->GrayFilter();
		pcNode->SetIcon( pcIcon );

		m_pcPrinters->InsertNode( pcNode );
	}
	cupsFreeDests( nCount, psDests );

	if( nCount > 0 )
		m_pcPrinters->Select( nSelected, true );
}

/* Get the list of jobs from CUPS for the selected printer and populate the Jobs ListView */
void QueueWindow::DisplayJobs( const char *zPrinter )
{
	int n, nCount;
	cups_job_t *psJobs;

	m_pcJobs->Clear();

	nCount = cupsGetJobs( &psJobs, zPrinter, 0, m_nJobSelect );
	for( n = 0; n < nCount; n++ )
	{
		ListViewStringRow *pcRow = new ListViewStringRow();

		/* ID */
		String cID;
		cID.Format( "%d", psJobs[n].id );
		pcRow->AppendString( cID );

		/* Name */
		pcRow->AppendString( psJobs[n].title );

		/* Status */
		String cStatus;
		switch( psJobs[n].state )
		{
			case IPP_JOB_PENDING:
			{
				cStatus = "Pending";
				break;
			}

			case IPP_JOB_HELD:
			{
				cStatus = "Held";
				break;
			}

			case IPP_JOB_PROCESSING:
			{
				cStatus = "Processing";
				break;
			}

			case IPP_JOB_STOPPED:
			{
				cStatus = "Stopped";
				break;
			}

			case IPP_JOB_CANCELLED:
			{
				cStatus = "Cancelled";
				break;
			}

			case IPP_JOB_ABORTED:
			{
				cStatus = "Aborted";
				break;
			}

			case IPP_JOB_COMPLETED:
			{
				cStatus = "Completed";
				break;
			}

			default:
			{
				cStatus = "Unknown";
				break;
			}
		}
		pcRow->AppendString( cStatus );

		/* Owner */
		pcRow->AppendString( psJobs[n].user  );

		/* Size */
		String cSize;
		int nSize = psJobs[n].size;	/* Size in KB */
		if( nSize <= 1048576 )
			cSize.Format( MSG_MAINWND_JOBSIZE_KB.c_str(), nSize );
		else if( nSize <= 1073741824 )
			cSize.Format( MSG_MAINWND_JOBSIZE_MB.c_str(), nSize / 1024 );
		else
			cSize.Format( MSG_MAINWND_JOBSIZE_GB.c_str(), nSize / 1048576 );
		pcRow->AppendString( cSize );

		/* Submitted */
		char zTime[64] = {0};
		struct tm *psTime = gmtime( &psJobs[n].creation_time );

		strftime( zTime, 64, "%T %e %b %Y", psTime );

		pcRow->AppendString( zTime );

		/* Store the CUPS job status internally */
		pcRow->SetCookie( psJobs[n].state );

		m_pcJobs->InsertRow( pcRow );
	}
	cupsFreeJobs( nCount, psJobs );
}

void QueueWindow::EnableDisableJobMenus( ipp_jstate_t nState )
{
	bool bEnableCancel;

	switch( nState )
	{
		case IPP_JOB_PENDING:
		case IPP_JOB_HELD:
		case IPP_JOB_PROCESSING:
		case IPP_JOB_STOPPED:
		{
			/* Can cancel */
			bEnableCancel = true;
			break;
		}

		case IPP_JOB_CANCELLED:
		case IPP_JOB_ABORTED:
		case IPP_JOB_COMPLETED:
		default:
		{
			/* Can not cancel */
			bEnableCancel = false;
			break;
		}
	}

	m_pcItemCancel->SetEnable( bEnableCancel );
	m_pcCtxItemCancel->SetEnable( bEnableCancel );
}

status_t QueueWindow::EnablePrinter( const char *zPrinter, bool bEnable )
{
	http_t *phHttp;
	ipp_op_t nOp;
	ipp_t *pRequest, *pResponse;
	char zURI[1024];
	status_t nError = EOK;

	if( bEnable )
		nOp = IPP_RESUME_PRINTER;
	else
		nOp = IPP_PAUSE_PRINTER;

	phHttp = httpConnectEncrypt(cupsServer(), ippPort(), cupsEncryption());
	if( phHttp != NULL )
	{
		/*
		 * Build an IPP request, which requires the following
		 * attributes:
		 *
		 *    attrgibutes-charset
		 *    attributes-natural-language
		 *    printer-uri
		 *    printer-state-message [optional]
		 */

		pRequest = ippNewRequest( nOp );

		httpAssembleURIf( HTTP_URI_CODING_ALL, zURI, sizeof( zURI ), "ipp", NULL, "localhost", 0, "/printers/%s", zPrinter );
		ippAddString( pRequest, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, zURI );
		ippAddString( pRequest, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsUser() );

		/* Do the request and get back a response... */
		if( ( pResponse = cupsDoRequest( phHttp, pRequest, "/admin/" ) ) != NULL )
		{
	    	if( pResponse->request.status.status_code > IPP_OK_CONFLICT )
				nError = errno;

			ippDelete( pResponse );
		}
		else
			nError = errno;
	}
	else
		nError = errno;

	return nError;
}

void QueueWindow::EnableDisablePrinterMenus( int nState )
{
	bool bEnablePause, bEnableResume;

	m_pcPrinterContextMenu->RemoveItem( 0 );

	switch( nState )
	{
		case STATE_IDLE:
		{
			m_pcPrinterContextMenu->AddItem( m_pcCtxItemPause );

			bEnablePause = true;
			bEnableResume = false;
			break;
		}

		case STATE_STOPPED:
		{
			m_pcPrinterContextMenu->AddItem( m_pcCtxItemResume );

			bEnablePause = false;
			bEnableResume = true;
			break;
		}
	}
	m_pcPrinterContextMenu->SetTargetForItems( this );

	m_pcItemPause->SetEnable( bEnablePause );
	m_pcItemResume->SetEnable( bEnableResume );
}

class QueueApplication : public Application
{
	public:
		QueueApplication();

	private:
		QueueWindow *m_pcWindow;
};

QueueApplication::QueueApplication() : Application( "application/x-VND.syllable-PrintQueue" )
{
	SetCatalog("PrintQueue.catalog");

	m_pcWindow = new QueueWindow( Rect( 100,100,800,350 ) );
	m_pcWindow->Show();
	m_pcWindow->MakeFocus();
}

int main( void )
{
	QueueApplication *pcApplication = new QueueApplication();
	pcApplication->Run();

	return EXIT_SUCCESS;
}

