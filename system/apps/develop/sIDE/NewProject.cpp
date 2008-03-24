#include <posix/wait.h>
#include <unistd.h>
#include "NewProject.h"
#include "messages.h"
#include "resources/sIDE.h"


NewProjectDialog :: NewProjectDialog(Window* pcParent) : Window(Rect(0,0,500,300), "prj_wnd", MSG_NEWWND_TITLE,WND_NOT_RESIZABLE)
{
	pcParentWindow = pcParent;
	SetItems();
}

NewProjectDialog::~NewProjectDialog()
{
}

void NewProjectDialog::SetItems()
{
	pcFrameView = new FrameView(Rect(0,0,GetBounds().Width(), GetBounds().Height()-45),"","");
	AddChild(pcFrameView);
	
	pcListView = new TreeView(Rect(5,5,200,250), "pcListView");
	
	/* Iterate through the templates directory and add the projects */
	try
	{
		os::Directory cDir( "^/Templates" );
		os::String zFile;
		while( cDir.GetNextEntry( &zFile ) == 1 )
		{
			if( zFile == "." || zFile == ".." )
				continue;
			os::String zDirPath;
			cDir.GetPath( &zDirPath );
			os::Path cPath( zDirPath );
			cPath.Append( zFile );
			try
			{
				os::FSNode cFile( cPath );
				if( cFile.IsFile() )
				{
					AddNode( 0, zFile.substr( 0, zFile.Length() - 4 ), LoadImage("cpp.png") );
				}
			} catch( ... )
			{
			}
		}
	} catch( ... )
	{
	}
	
	pcListView->SetHasColumnHeader( true );
    pcListView->InsertColumn( MSG_NEWWND_PROJECTS.c_str(), 200 );
    pcListView->SetDrawTrunk(true);
//    pcListView->SetExpandedImage(LoadImage("expander.png"));
//    pcListView->SetCollapsedImage(LoadImage("collapse.png"));
    pcListView->SetDrawExpanderBox(false);
    pcListView->SetTarget( this );
    AddChild(pcListView);
    
    
    pcCancel = new Button(Rect(GetBounds().Width()-75, GetBounds().Height()-30,GetBounds().Width()-5, GetBounds().Height()-5),"cancel_but",MSG_BUTTON_CANCEL, new Message(M_BUTTON_CANCEL));
    AddChild(pcCancel);
    
    pcOk = new Button(Rect(GetBounds().Width()-130,GetBounds().Height()-30,GetBounds().Width()-80,GetBounds().Height()-5),"ok_but",  MSG_BUTTON_OK,new Message(M_BUTTON_OK)); 
	AddChild(pcOk);
	
	pcSVPrjName = new StringView(Rect(210,50,270,75),"namesview",MSG_NEWWND_NAME);
	AddChild(pcSVPrjName);
	
	pcTVPrjName = new TextView(Rect(275,52,GetBounds().Width()-29,72),"nametview",MSG_NEWWND_TITLE.c_str());
	AddChild(pcTVPrjName);
	
	pcSVPrjLocation = new StringView(Rect(210,100,270,125),"locatsview",MSG_NEWWMD_DIR);
	AddChild(pcSVPrjLocation);
	
	pcTVPrjLocation = new TextView(Rect(275,102,GetBounds().Width()-29,122),"locatview",getenv("HOME") );
	AddChild(pcTVPrjLocation);
	
	pcFileReqBut = new ImageButton(Rect(GetBounds().Width()-28,96,GetBounds().Width()-2,124),"blah","open",new Message(M_OPEN),LoadImage("up.png"),ImageButton::IB_TEXT_BOTTOM,true, false, true, CF_FOLLOW_NONE);
    AddChild(pcFileReqBut);
    
    pcFileRequester = new FileRequester(FileRequester::LOAD_REQ, new Messenger(this), getenv("HOME"),FileRequester::NODE_DIR);
	pcFileRequester->Start();
	
	SetTabs();

}

void NewProjectDialog::SetTabs()
{
	pcListView->SetTabOrder(0);
	pcTVPrjName->SetTabOrder(1);
	pcTVPrjLocation->SetTabOrder(2);
	pcOk->SetTabOrder(3);
	pcCancel->SetTabOrder(4);
	SetDefaultButton(pcOk);
	SetFocusChild(pcListView);
	pcListView->Select(0,0);
}  

void NewProjectDialog::AddNode( int nIndent, os::String s1, BitmapImage* pcImage) 
{
    TreeViewStringNode *pcNode = new TreeViewStringNode();
    pcNode->AppendString( s1 );
    pcNode->SetIndent( nIndent );
    pcNode->SetIcon(pcImage);
    pcListView->InsertNode( pcNode);
}

BitmapImage* NewProjectDialog::LoadImage(const std::string& zResource)
{
	os::File cFile( open_image_file( get_image_id() ) );
	Resources cCol( &cFile );
    ResStream* pcStream = cCol.GetResourceStream(zResource);
    BitmapImage* vImage = new BitmapImage(pcStream);
    delete( pcStream );
    return(vImage);
}

void NewProjectDialog::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_BUTTON_OK:
		{
			uint32 i = pcListView->GetLastSelected();
			
			if (strcmp(pcTVPrjLocation->GetBuffer()[0].c_str(), "") != 0 && strcmp(pcTVPrjName->GetBuffer()[0].c_str(), "")!=0)
			{
				Message* pcPassedMessage; 
				char path[1024]={'\0'};
				strcat(path,pcTVPrjLocation->GetBuffer()[0].c_str());
				if (path[strlen(path)-1] != '/') 
				strcat(path,"/");
				strcat(path,pcTVPrjName->GetBuffer()[0].c_str());
				
					
				if( i > 0 )
				{
					int nExitStatus = 0;
					os::TreeViewStringNode* pcRow = static_cast<os::TreeViewStringNode*>(pcListView->GetRow( i ));
					os::Directory cDir( "^/Templates" );
					os::String zDirPath;
					cDir.GetPath( &zDirPath );
							
					os::Path cTemplatePath( zDirPath );
					cTemplatePath.Append( pcRow->GetString( 0 ) + ".zip" );
							
					/* Create path */
					mkdir( path, 0x700 );
								
					/* Unzip templates */
					int nError;
					switch( ( nError = fork() ) )
					{
						case -1:
							break;
						case 0:
						{
							set_thread_priority( -1, 0 );
							execlp( "unzip", "unzip", cTemplatePath.GetPath().c_str(), "-d", path, NULL );
							exit( 0 );
							break;
						}
						default:
						{
							int nPid;
					        nPid = nError;
					        nError = waitpid( nPid, &nExitStatus, 0 );
								
					        break;
    					}
      							
					}
								
					if( nExitStatus == 0 )
					{
						/* Move project file */
						os::Path cSource( path );
						cSource.Append( "Project.side" );
								
						strcat( path,"/" );
						strcat( path,pcTVPrjName->GetBuffer()[0].c_str() );
						strcat( path,".side" );
						os::Path cFilePath( path );
								
						rename( cSource.GetPath().c_str(), cFilePath.GetPath().c_str() );
											
						pcPassedMessage = new Message(M_PASSED_MESSAGE);
						pcPassedMessage->AddString("file/side",path);
						pcParentWindow->PostMessage(pcPassedMessage, pcParentWindow);
						delete( pcPassedMessage );
						PostMessage( os::M_QUIT, this );
					} else {
						pcErrorAlert = new Alert(MSG_NEWWND_ERR_TITLE, MSG_NEWWND_ERR_EXTR,Alert::ALERT_WARNING, 0x00,"OK",NULL);
						pcErrorAlert->CenterInWindow(this);
						pcErrorAlert->Go(new Invoker());
					}
				}
			}
			else
			{
				if (strcmp(pcTVPrjLocation->GetBuffer()[0].c_str(), "") == 0 && strcmp(pcTVPrjName->GetBuffer()[0].c_str(), "")==0)
				{
					pcErrorAlert = new Alert(MSG_NEWWND_ERR_TITLE, MSG_NEWWND_NO_LOC_NAME,Alert::ALERT_WARNING, 0x00,MSG_BUTTON_OK.c_str(),NULL);
				}
				else if (strcmp(pcTVPrjLocation->GetBuffer()[0].c_str(), "") ==0)
				{
					pcErrorAlert = new Alert(MSG_NEWWND_ERR_TITLE, MSG_NEWWND_NO_LOC,Alert::ALERT_WARNING, 0x00,MSG_BUTTON_OK.c_str(),NULL);
				}
				else
				{
					pcErrorAlert = new Alert(MSG_NEWWND_ERR_TITLE, MSG_NEWWND_NO_NAME,Alert::ALERT_WARNING, 0x00,MSG_BUTTON_OK.c_str(),NULL);
				}
				pcErrorAlert->CenterInWindow(this);
				pcErrorAlert->Go(new Invoker());
			}
			break;
		}
			
		case M_BUTTON_CANCEL:
		{
			PostMessage( os::M_QUIT, this );
			break;
		}
		case M_OPEN:
		{
			pcFileRequester->Lock();
			if( !pcFileRequester->IsVisible() )
			{
				pcFileRequester->CenterInWindow(this);
				pcFileRequester->Show();
			}
			pcFileRequester->MakeFocus();
			pcFileRequester->Unlock();
			break;
		}
		
		case M_LOAD_REQUESTED:
		{
			const char* pzFPath;
			if(pcMessage->FindString("file/path",&pzFPath) == 0)
			{
				pcFileNode = new FSNode(pzFPath);
				
				if (pcFileNode->IsDir())
				{
					pcTVPrjLocation->Set(pzFPath);
				}
				else
				{
					std::string cFileError = MSG_NEWWND_NO_DIR_1 + (std::string)pzFPath + MSG_NEWWND_NO_DIR_2; 
					pcTVPrjLocation->Set("");
					
					pcErrorAlert = new Alert(MSG_NEWWND_NO_DIR,cFileError,Alert::ALERT_WARNING,0x00,MSG_BUTTON_OK.c_str(),NULL);
					pcErrorAlert->CenterInWindow(this);
					pcErrorAlert->Go(new Invoker());
				}
				delete pcFileNode;
			}	
			break;
		}	
	}
}
