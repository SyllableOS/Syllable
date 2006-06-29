// sIDE		(C) 2001-2005 Arno Klenke     
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

#include <atheos/threads.h>
#include <atheos/filesystem.h>
#include <gui/window.h>
#include <gui/menu.h>
#include <gui/listview.h>
#include <gui/textview.h>
#include <gui/dropdownmenu.h>
#include <gui/button.h>
#include <gui/requesters.h>
#include <gui/treeview.h>
#include <gui/filerequester.h>
#include <gui/imagebutton.h>
#include <gui/layoutview.h>
#include <gui/toolbar.h>
#include <gui/radiobutton.h>
#include <gui/separator.h>
#include <util/application.h>
#include <util/message.h>
#include <storage/path.h>


#include "messages.h"
#include "project.h"
#include "GroupWindow.h"
#include "AddWindow.h"
#include "AddLibrary.h"
#include "ProjectPrefs.h"
#include "ProjectWindow.h"
#include "NewProject.h"


#include <iostream>
#include <unistd.h>


BitmapImage* LoadImage(const std::string& zResource)
{
	os::File cSelf( open_image_file( get_image_id() ) );
	os::Resources cCol( &cSelf );		
    ResStream* pcStream = cCol.GetResourceStream(zResource);
    BitmapImage* vImage = new BitmapImage(pcStream);
    delete( pcStream );
    return(vImage);
}


void SetButtonImageFromResource( os::ImageButton* pcButton, os::String zResource )
{
	os::File cSelf( open_image_file( get_image_id() ) );
	os::Resources cCol( &cSelf );		
	os::ResStream *pcStream = cCol.GetResourceStream( zResource );
	pcButton->SetImage( pcStream );
	delete( pcStream );
}

/* Project tree */
ProjectTree::ProjectTree( const os::Rect& cFrame, const os::String& cTitle, uint32 nModeFlags, uint32 nResizeMask, uint32 nFlags )
			: os::TreeView( cFrame, cTitle, nModeFlags, nResizeMask, nFlags )
{
}

void ProjectTree::MouseUp( const os::Point & cPos, uint32 nButtons, os::Message * pcData )
{
	os::String zFile;

	if ( pcData == NULL || pcData->FindString( "file/path", &zFile.str() ) != 0 )
	{
		return ( os::TreeView::MouseUp( cPos, nButtons, pcData ) );
	}
	/* Tell CFApp class */
	os::Message cMsg( *pcData );
	cMsg.SetCode( os::M_LOAD_REQUESTED );
	GetWindow()->PostMessage( &cMsg, GetWindow() );
}

ProjectWindow::ProjectWindow( const os::Rect& cFrame, const std::string &cTitle )
				:os::Window( cFrame, "side_project_window", cTitle )
{
	m_cProject.New( os::Path( "NewProject.side" ) );
	m_cProject.Load( os::Path( "NewProject.side" ) );

	/* Create main view */

	m_pcView = new os::View( GetBounds(), "side_back", os::CF_FOLLOW_ALL, os::WID_WILL_DRAW );
	AddChild( m_pcView );
	
	
	/* Create menus */
	
	m_pcMenuBar = new os::Menu( os::Rect(), "side_menu_bar", os::ITEMS_IN_ROW, os::CF_FOLLOW_LEFT |
							os::CF_FOLLOW_RIGHT, os::WID_FULL_UPDATE_ON_RESIZE );  
	
	m_pcMenuApp = new os::Menu( os::Rect( 0, 0, 1, 1 ), MSG_PWND_MENU_APPLICATION, os::ITEMS_IN_COLUMN );
	m_pcMenuApp->AddItem( MSG_PWND_MENU_APPLICATION_ABOUT, new os::Message( M_HELP_ABOUT ) );
	m_pcMenuApp->AddItem( new os::MenuSeparator() );
	m_pcMenuApp->AddItem( MSG_PWND_MENU_APPLICATION_QUIT, new os::Message( M_PROJECT_QUIT ) );
	
	m_pcMenuProject = new os::Menu( os::Rect( 0, 0, 1, 1 ), MSG_PWND_MENU_PROJECT, os::ITEMS_IN_COLUMN );
	m_pcMenuProject->AddItem( MSG_PWND_MENU_PROJECT_NEW, new os::Message( M_PROJECT_NEW ) );
	m_pcMenuProject->AddItem( MSG_PWND_MENU_PROJECT_OPEN, new os::Message( M_PROJECT_OPEN ) );
	m_pcMenuProject->AddItem( MSG_PWND_MENU_PROJECT_SAVE, new os::Message( M_PROJECT_SAVE ) );
	m_pcMenuProject->AddItem( MSG_PWND_MENU_PROJECT_SAVEAS, new os::Message( M_PROJECT_SAVE_AS ) );
	m_pcMenuProject->AddItem( new os::MenuSeparator() );
	m_pcMenuProject->AddItem( MSG_PWND_MENU_PROJECT_PREF, new os::Message( M_PROJECT_PREFS ) );
	m_pcMenuProject->AddItem( MSG_PWND_MENU_PROJECT_GROUPS, new os::Message( M_PROJECT_GROUPS ) );
	m_pcMenuProject->AddItem( MSG_PWND_MENU_PROJECT_NEW_FILE, new os::Message( M_PROJECT_FILE_NEW ) );
	m_pcMenuProject->AddItem( MSG_PWND_MENU_PROJECT_ADD_FILE, new os::Message( M_PROJECT_FILE_ADD ) );
	m_pcMenuProject->AddItem( MSG_PWND_MENU_PROJECT_ADD_LIB, new os::Message( M_PROJECT_FILE_ADD_LIBRARY ) );
	m_pcMenuProject->AddItem( MSG_PWND_MENU_PROJECT_REMOVE_FILE, new os::Message( M_PROJECT_FILE_REMOVE ) );
	
	
	m_pcMenuBuild = new os::Menu( os::Rect( 0, 0, 1, 1 ), MSG_PWND_MENU_BUILD, os::ITEMS_IN_COLUMN );
	m_pcMenuBuild->AddItem( MSG_PWND_MENU_BUILD_CLEAN, new os::Message( M_BUILD_CLEAN ) );
	m_pcMenuBuild->AddItem( MSG_PWND_MENU_BUILD_MAKE, new os::Message( M_BUILD_COMPILE ) );
	m_pcMenuBuild->AddItem( MSG_PWND_MENU_BUILD_RUN, new os::Message( M_BUILD_RUN ) );
	m_pcMenuBuild->AddItem( MSG_PWND_MENU_BUILD_EXPORT_MAKEFILE, new os::Message( M_BUILD_MAKEFILE ) );
	
	
	m_pcMenuBar->AddItem( m_pcMenuApp );
	m_pcMenuBar->AddItem( m_pcMenuProject );
	m_pcMenuBar->AddItem( m_pcMenuBuild );
	
	m_pcMenuBar->SetTargetForItems(this);
	m_pcMenuBar->SetFrame( os::Rect( 0, 0, cFrame.Width(), m_pcMenuBar->GetPreferredSize( false ).y - 1 ) );
	AddChild( m_pcMenuBar );
	
	/* Create toolbar */
	os::ToolBar* pcToolBar = new os::ToolBar( os::Rect( 0, m_pcMenuBar->GetPreferredSize( false ).y, GetBounds().Width(), m_pcMenuBar->GetPreferredSize( false ).y + 40 ), "side_toolbar" );
	
	
	os::ImageButton* pcBreaker = new os::ImageButton(os::Rect(2,2,8,30), "side_t_breaker", "",NULL,NULL, os::ImageButton::IB_TEXT_BOTTOM,false,false,false);
    SetButtonImageFromResource( pcBreaker, "breaker.png" );
    pcToolBar->AddChild( pcBreaker, ToolBar::TB_FIXED_MINIMUM );
    
	os::ImageButton* pcProjOpen = new os::ImageButton( os::Rect(), "side_t_project_open", "",new os::Message( M_PROJECT_OPEN )
    								,NULL, os::ImageButton::IB_TEXT_BOTTOM, true, false, true );
    SetButtonImageFromResource( pcProjOpen, "project_open.png" );
	pcToolBar->AddChild( pcProjOpen, ToolBar::TB_FIXED_MINIMUM );
	
	os::ImageButton* pcProjSave = new os::ImageButton( os::Rect(), "side_t_project_save", "",new os::Message( M_PROJECT_SAVE )
    								,NULL, os::ImageButton::IB_TEXT_BOTTOM, true, false, true );
    SetButtonImageFromResource( pcProjSave, "project_save.png" );
	pcToolBar->AddChild( pcProjSave, ToolBar::TB_FIXED_MINIMUM );


    pcBreaker = new os::ImageButton(os::Rect(2,2,8,30), "side_t_breaker", "",NULL,NULL, os::ImageButton::IB_TEXT_BOTTOM,false,false,false);
    SetButtonImageFromResource( pcBreaker, "breaker.png" );
    pcToolBar->AddChild( pcBreaker, ToolBar::TB_FIXED_MINIMUM );
    
    os::ImageButton* pcClean = new os::ImageButton( os::Rect(), "side_t_clean", "",new os::Message( M_BUILD_CLEAN )
    								,NULL, os::ImageButton::IB_TEXT_BOTTOM, true, false, true );
    SetButtonImageFromResource( pcClean, "clean.png" );
	pcToolBar->AddChild( pcClean, ToolBar::TB_FIXED_MINIMUM );
	
	os::ImageButton* pcCompile = new os::ImageButton( os::Rect(), "side_t_compile", "",new os::Message( M_BUILD_COMPILE )
    								,NULL, os::ImageButton::IB_TEXT_BOTTOM, true, false, true );
    SetButtonImageFromResource( pcCompile, "compile.png" );
	pcToolBar->AddChild( pcCompile, ToolBar::TB_FIXED_MINIMUM );
	
	os::ImageButton* pcRun = new os::ImageButton( os::Rect(), "side_t_run", "",new os::Message( M_BUILD_RUN )
    								,NULL, os::ImageButton::IB_TEXT_BOTTOM, true, false, true );
    SetButtonImageFromResource( pcRun, "run.png" );
	pcToolBar->AddChild( pcRun, ToolBar::TB_FIXED_MINIMUM );
	
	AddChild( pcToolBar );
	
	/* Create file list */
	
	m_pcList = new ProjectTree( os::Rect( 5, pcToolBar->GetFrame().bottom + 1, cFrame.Width() - 5, cFrame.Height() - 5 ), "side_files", 
							os::ListView::F_RENDER_BORDER, os::CF_FOLLOW_ALL, os::WID_FULL_UPDATE_ON_RESIZE );
	m_pcList->InsertColumn( MSG_PWND_LIST_FILE.c_str(), (int)m_pcList->GetFrame().Width() * 3 / 4 );
	m_pcList->InsertColumn( MSG_PWND_LIST_TYPE.c_str(), (int)m_pcList->GetFrame().Width() * 1 / 4 );
	m_pcList->SetAutoSort( false );
	m_pcList->SetInvokeMsg( new os::Message( M_LIST_INVOKED ) );
	m_pcList->SetTarget( this );
	m_pcList->SetDrawTrunk( true );
	m_pcList->SetDrawExpanderBox( false );
	m_pcList->SetExpandedImage( LoadImage( "expander.png" ) );
    m_pcList->SetCollapsedImage( LoadImage( "collapse.png" ) );
	m_pcView->AddChild( m_pcList );
	
	
	m_pcGrp = NULL;
	m_pcAdd = NULL;
	m_pcAddLibrary = NULL;
	m_pcPrefs = NULL;
	
	/* Create load requester */
	m_pcLoadRequester = new os::FileRequester( os::FileRequester::LOAD_REQ, new os::Messenger( this ), "", os::FileRequester::NODE_FILE, false );
	m_pcLoadRequester->Start();
	
	/* Create save requester */
	m_pcSaveRequester = new os::FileRequester( os::FileRequester::SAVE_REQ, new os::Messenger( this ), "", os::FileRequester::NODE_FILE, false );
	m_pcSaveRequester->Start();
	
	/* Create add requester */
	m_pcAddRequester = new os::FileRequester( os::FileRequester::LOAD_REQ, new os::Messenger( this ), "", os::FileRequester::NODE_FILE, true );
	m_pcAddRequester->Start();

	SetTitle( os::String("sIDE : ") + os::String( m_cProject.GetFilePath().GetLeaf() ) );

	/* Set Icon */
	os::Resources cCol( get_image_id() );
	os::ResStream *pcStream = cCol.GetResourceStream( "icon24x24.png" );
	os::BitmapImage *pcIcon = new os::BitmapImage( pcStream );
	SetIcon( pcIcon->LockBitmap() );
	delete( pcIcon );

	RefreshList();
}

ProjectWindow::~ProjectWindow()
{
	m_pcLoadRequester->Quit();
	m_pcSaveRequester->Quit();
}

bool ProjectWindow::OkToQuit()
{
	/* Close the project */
	m_cProject.Close();
	while( m_cProject.IsWorking() ) { }
	if( m_pcGrp )
		m_pcGrp->Close();
	if( m_pcAdd )
		m_pcAdd->Close();
	if( m_pcAddLibrary )
		m_pcAddLibrary->Close();
	os::Application::GetInstance()->PostMessage( os::M_QUIT );
	return( false );
}

os::Image* GetImageFromResource( os::String zResource )
{
		os::File cSelf( open_image_file( get_image_id() ) );
		os::Resources cCol( &cSelf );		
		os::ResStream *pcStream = cCol.GetResourceStream( zResource );
		os::BitmapImage* pcImage = new os::BitmapImage();
		pcImage->Load( pcStream );
		delete( pcStream );
		return( pcImage );
}

/* refresh the file list */
void ProjectWindow::RefreshList()
{
	
	m_pcList->Clear();
	os::TreeViewStringNode *pcNewRow;
	for( uint i = 0; i < m_cProject.GetGroupCount(); i++ )
	{
		pcNewRow = new os::TreeViewStringNode();
		pcNewRow->AppendString( m_cProject.GetGroupName( i ) );
		pcNewRow->AppendString( "" );
		pcNewRow->SetIndent( 1 );
		m_pcList->InsertNode( pcNewRow );
		for( uint j = 0; j < m_cProject.GetFileCount( i ); j++ )
		{
			pcNewRow = new os::TreeViewStringNode();
			pcNewRow->SetIndent( 2 );
			pcNewRow->AppendString( m_cProject.GetFileName( i, j )  );
			switch( m_cProject.GetFileType( i, j ) )
			{
				case TYPE_HEADER:
					pcNewRow->AppendString( MSG_TYPE_HEADER );
					pcNewRow->SetIcon( GetImageFromResource( "type_h.png" ) );
				break;
				case TYPE_C:
					pcNewRow->AppendString( MSG_TYPE_C );
					pcNewRow->SetIcon( GetImageFromResource( "type_c.png" ) );
				break;
				case TYPE_CPP:
					pcNewRow->AppendString( MSG_TYPE_CPP );
					pcNewRow->SetIcon( GetImageFromResource( "type_cpp.png" ) );
				break;
				case TYPE_LIB:
					pcNewRow->AppendString( MSG_TYPE_LIB );
					pcNewRow->SetIcon( GetImageFromResource( "type_lib.png" ) );
				break;
				case TYPE_OTHER:
					pcNewRow->AppendString( MSG_TYPE_OTHER );
					pcNewRow->SetIcon( GetImageFromResource( "type_other.png" ) );
				break;
				case TYPE_RESOURCE:
					pcNewRow->AppendString( MSG_TYPE_RES );
					pcNewRow->SetIcon( GetImageFromResource( "type_resource.png" ) );
				break;
				case TYPE_CATALOG:
					pcNewRow->AppendString( MSG_TYPE_CATALOG );
					pcNewRow->SetIcon( GetImageFromResource( "type_resource.png" ) );
				break;
				case TYPE_INTERFACE:
					pcNewRow->AppendString( MSG_TYPE_IF );
					pcNewRow->SetIcon( GetImageFromResource( "type_if.png" ) );
				break;
				case TYPE_NONE:
				default:
					pcNewRow->AppendString( MSG_TYPE_UNKNOWN );
					pcNewRow->SetIcon( GetImageFromResource( "type_other.png" ) );
			}
			m_pcList->InsertNode( pcNewRow );
		}
	}
	if( m_pcGrp )
		m_pcGrp->PostMessage( new os::Message( M_LIST_REFRESH ), m_pcGrp );
	if( m_pcAdd )
		m_pcAdd->PostMessage( new os::Message( M_LIST_REFRESH ), m_pcAdd );
	if( m_pcAddLibrary )
		m_pcAddLibrary->PostMessage( new os::Message( M_LIST_REFRESH ), m_pcAddLibrary );
	m_pcList->Invalidate();
}

/* show an about window */
int ShowAbout(void *data)
{
	os::Alert* pcAbout = new os::Alert(MSG_PWND_ABOUT_TITLE, os::String( "sIDE 0.4.5\n" ) + MSG_PWND_ABOUT, os::Alert::ALERT_INFO,
											0x00, MSG_BUTTON_OK.c_str(), NULL);
	pcAbout->Go();
	return( 0 );
}

/* get the selected file */
void ProjectWindow::GetSelected( int16 *pnGroup, int16 *pnFile )
{
	*pnGroup = -1;
	*pnFile = -1;
	int nCounter = -1;
	for( uint i = 0; i < m_cProject.GetGroupCount(); i++ )
	{
		nCounter++;
		if( nCounter == m_pcList->GetFirstSelected() )
		{
			*pnGroup = i;
			*pnFile = -1;
			return; 
		}
		for( uint j = 0; j < m_cProject.GetFileCount( i ); j++ )
		{
			nCounter++;
			if( nCounter == m_pcList->GetFirstSelected() ) {
				*pnGroup = i;
				*pnFile = j;
				return; 
			}
		}
	}
}

void ProjectWindow::HandleMessage( os::Message* pcMessage )
{
	thread_id about;
	int16 nGroup;
	int16 nFile;
	switch( pcMessage->GetCode() )
	{
		case M_PROJECT_NEW:
		{
			/*m_cProject.New( "NewProject.side" );
			RefreshList();
			SetTitle( os::String("sIDE v0.1 : ") + os::String( m_cProject.GetFilePath().GetLeaf() ) );
		*/
		NewProjectDialog* pcDialog = new NewProjectDialog(this);
		pcDialog->CenterInWindow(this);
		pcDialog->Show();
		pcDialog->MakeFocus();
		}
		break;
		case M_PROJECT_OPEN:
			m_pcLoadRequester->Lock();
			m_pcLoadRequester->CenterInWindow(this);
			m_pcLoadRequester->Show();
			m_pcLoadRequester->MakeFocus( true );
			m_pcLoadRequester->Unlock();
		break;
		case M_PROJECT_SAVE:
			m_cProject.Save( m_cProject.GetFilePath() );
		break;
		case M_PROJECT_SAVE_AS:
			m_pcSaveRequester->Lock();
			m_pcSaveRequester->CenterInWindow(this);
			m_pcSaveRequester->Show();
			m_pcSaveRequester->MakeFocus( true );
			m_pcSaveRequester->Unlock();
		break;
		case M_PROJECT_PREFS:
			if( m_pcPrefs == NULL ) {
				m_pcPrefs = new ProjectPrefs( os::Rect( 20, 20, 600, 150 )
											, &m_cProject, this );
				m_pcPrefs->CenterInWindow(this);
				m_pcPrefs->Show();
			}
			m_pcPrefs->MakeFocus( true );
		break;
		case M_PROJECT_GROUPS:
			if( m_pcGrp == NULL ) {
				m_pcGrp = new GroupWindow( os::Rect( 20, 20, 320, 220 )
											, &m_cProject, this );
				m_pcGrp->CenterInWindow(this);
				m_pcGrp->Show();
			}
			m_pcGrp->MakeFocus( true );
		break;
		case M_PROJECT_FILE_NEW:
			if( m_cProject.GetGroupCount() < 1 )
			{
				os::Alert* pcAbout = new os::Alert( MSG_PWND_TITLE, MSG_PWND_NO_GROUPS, os::Alert::ALERT_INFO, 0x00, MSG_BUTTON_OK.c_str(), NULL);
				pcAbout->CenterInWindow(this);
				pcAbout->Go( new os::Invoker() );
				break;
			}
			if( m_pcAdd == NULL ) {
				std::vector<os::String> azNull;
				m_pcAdd = new AddWindow( os::Rect( 20, 20, 270, 100 )
											, &m_cProject, azNull, this );
				m_pcAdd->CenterInWindow(this);
				m_pcAdd->Show();
			}
			m_pcAdd->MakeFocus( true );
		break;
		case M_PROJECT_FILE_ADD_LIBRARY:
			if( m_cProject.GetGroupCount() < 1 )
			{
				os::Alert* pcAbout = new os::Alert( MSG_PWND_TITLE, MSG_PWND_NO_GROUPS, os::Alert::ALERT_INFO, 0x00, MSG_BUTTON_OK.c_str(), NULL);
				pcAbout->CenterInWindow(this);
				pcAbout->Go( new os::Invoker() );
				break;
			}
			if( m_pcAddLibrary == NULL ) {
				m_pcAddLibrary = new AddLibraryWindow( os::Rect( 20, 20, 350, 150 )
											, &m_cProject, this );
				m_pcAddLibrary->CenterInWindow(this);
				m_pcAddLibrary->Show();
			}
			m_pcAddLibrary->MakeFocus( true );
		break;
		case M_PROJECT_FILE_ADD:
			m_pcAddRequester->Lock();
			m_pcAddRequester->CenterInWindow(this);
			m_pcAddRequester->Show();
			m_pcAddRequester->MakeFocus( true );
			m_pcAddRequester->Unlock();
		break;
		case M_PROJECT_FILE_REMOVE:
			GetSelected( &nGroup, &nFile );
			if( nFile == -1 )
				break;
			std::cout<<"Remove File:"<<m_cProject.GetFileName( nGroup, nFile ).c_str()<<std::endl;
			m_cProject.RemoveFile( nGroup, nFile );
			RefreshList();
		break;
		case M_PROJECT_QUIT:
			m_cProject.Close();
			while( m_cProject.IsWorking() ) { }
			os::Application::GetInstance()->PostMessage( os::M_QUIT );
		break;
		case M_BUILD_CLEAN:
			m_cProject.Clean();
		break;
		case M_BUILD_COMPILE:
			m_cProject.Compile();
		break;
		case M_BUILD_RUN:
			m_cProject.Run();
		break;
		case M_BUILD_MAKEFILE:
			m_cProject.ExportMakefile();
		break;
		case M_HELP_ABOUT:
			about = spawn_thread( "side_about", (void*)ShowAbout, 0, 0, NULL );
			resume_thread( about );
		break;
		case M_LIST_INVOKED:
			GetSelected( &nGroup, &nFile );
			if( nFile == -1 )
				break;
			std::cout<<"Open File:"<<m_cProject.GetFileName( nGroup, nFile ).c_str()<<std::endl;
			m_cProject.OpenFile( nGroup, nFile, this );
		break;
		case M_LIST_REFRESH:
			RefreshList();
		break;
		case M_GROUP_CLOSED:
			MakeFocus( true );
			m_pcGrp = NULL;
		break;
		case M_PROJECT_PREFS_CLOSED:
			MakeFocus( true );
			m_pcPrefs = NULL;
		break;
		case M_ADD_CLOSED:
			MakeFocus( true );
			m_pcAdd = NULL;
		break;
		case M_ADD_LIBRARY_CLOSED:
			MakeFocus( true );
			m_pcAddLibrary = NULL;
		break;
		case os::M_LOAD_REQUESTED:
		{
			os::FileRequester* pcRequester = NULL;
			os::String zPath;
			if( !( pcMessage->FindPointer( "source", (void**)&pcRequester ) == 0 
				&& pcRequester == m_pcLoadRequester ) )
			{
				/* Add file */
				int i = 0;
				std::vector<os::String> azPaths;
				while( pcMessage->FindString( "file/path", &zPath, i ) == 0 )
				{
					/* We need the normalized paths of the project and of the file */
					int nStringLength = 0;
					os::String zNormalized( PATH_MAX, 0 );
					os::String zProject( PATH_MAX, 0 );
					int nFd = open( m_cProject.GetFilePath().GetDir().GetPath().c_str(), O_RDONLY );
					if( ( nStringLength = get_directory_path( nFd, (char*)zProject.c_str(), PATH_MAX ) ) < 0 )
					{
						os::Alert* pcAbout = new os::Alert( MSG_PWND_TITLE, MSG_PWND_COULD_NOT_OPEN_PROJ, os::Alert::ALERT_WARNING, 0x00, MSG_BUTTON_OK.c_str(), NULL);
						pcAbout->CenterInWindow(this);
						pcAbout->Go( new os::Invoker() );
						close( nFd );
						break;
					}
					zProject.Resize( nStringLength );
					close( nFd );
					nFd = open( zPath.c_str(), O_RDONLY );
					if( ( nStringLength = get_directory_path( nFd, (char*)zNormalized.c_str(), PATH_MAX ) ) < 0 )
					{
						os::Alert* pcAbout = new os::Alert( MSG_PWND_TITLE, MSG_PWND_COULD_NOT_OPEN_FILE, os::Alert::ALERT_WARNING, 0x00, MSG_BUTTON_OK.c_str(), NULL);
						pcAbout->CenterInWindow(this);
						pcAbout->Go( new os::Invoker() );
						close( nFd );
						break;
					}
					zNormalized.Resize( nStringLength );
					close( nFd );
					/* Now check that the new file is in a subdirectory of the project */
					if( zNormalized.substr( 0, zProject.Length() ) != zProject )
					{
						os::Alert* pcAbout = new os::Alert( MSG_PWND_TITLE, MSG_PWND_NOT_IN_PROJ_DIR, os::Alert::ALERT_WARNING, 0x00, MSG_BUTTON_OK.c_str(), NULL);
						pcAbout->CenterInWindow(this);
						pcAbout->Go( new os::Invoker() );
						break;
					}
					
					zPath = zNormalized.substr( zProject.Length() + 1 );
					printf( "%s\n", zPath.c_str() );
					azPaths.push_back( zPath );

					i++;
				}
				if( !azPaths.empty() )
				{
					if( m_cProject.GetGroupCount() < 1 )
					{
						os::Alert* pcAbout = new os::Alert( MSG_PWND_TITLE, MSG_PWND_NO_GROUPS, os::Alert::ALERT_INFO, 0x00, MSG_BUTTON_OK.c_str(), NULL);
						pcAbout->CenterInWindow(this);
						pcAbout->Go( new os::Invoker() );
						break;
					}
					if( m_pcAdd == NULL ) {
						m_pcAdd = new AddWindow( os::Rect( 20, 20, 270, 100 )
											,&m_cProject, azPaths, this );
						m_pcAdd->CenterInWindow(this);
						m_pcAdd->Show();
					}
					m_pcAdd->MakeFocus( true );
				}
				break;
			}
			
			if( pcMessage->FindString( "file/path", &zPath ) == 0 )
			{
				/* Load one project */
				m_cProject.Close();
				if( m_cProject.Load( os::Path( zPath.c_str() ) ) == false ) {
					os::Alert* pcAbout = new os::Alert( MSG_PWND_TITLE, MSG_PWND_COULD_NOT_OPEN_PROJ, os::Alert::ALERT_WARNING, 0x00, MSG_BUTTON_OK.c_str(), NULL);
					pcAbout->Go( new os::Invoker() );
					m_cProject.Load( os::Path( "NewProject.side" ) );
				}
				RefreshList();
				SetTitle( MSG_PWND_TITLE + ": " + os::String( m_cProject.GetFilePath().GetLeaf() ) );
			}
			
		}	
		break;
		case os::M_SAVE_REQUESTED:
		{
			os::String zPath;
			if( pcMessage->FindString( "file/path", &zPath.str() ) == 0 )
			{
				/* Save project */
	
				if( m_cProject.Save( os::Path( zPath.c_str() ) ) == false ) {
					os::Alert* pcAbout = new os::Alert( MSG_PWND_TITLE, MSG_PWND_COULD_NOT_SAVE_PROJ, os::Alert::ALERT_WARNING, 0x00, MSG_BUTTON_OK.c_str(), NULL);
					pcAbout->CenterInWindow(this);
					pcAbout->Go( new os::Invoker() );
					m_cProject.Load( os::Path( "NewProject.side" ) );
				}
				RefreshList();
				SetTitle( MSG_PWND_TITLE + ": " + os::String( m_cProject.GetFilePath().GetLeaf() ) );
			}
		}
		break;
		
		case M_PASSED_MESSAGE:
		{
			const char* zFilePath;
			if (pcMessage->FindString("file/side", &zFilePath)==0)
			{
					m_cProject.Close();
					Path cFilePath(zFilePath);
					m_cProject.Load(cFilePath);
					
			}
			RefreshList();
			SetTitle( MSG_PWND_TITLE + os::String( m_cProject.GetFilePath().GetLeaf() ) );
			break;
		}
		
		default:
			os::Window::HandleMessage( pcMessage );
	}
}



















































