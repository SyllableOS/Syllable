#include <gui/dropdownmenu.h>
#include <stdio.h>
#include <stdlib.h>
#include <gui/frameview.h>
#include <gui/window.h>
#include <gui/view.h>
#include <gui/font.h>
#include <gui/requesters.h>
#include <util/message.h>
#include <unistd.h>

const char* pzSelectPath;
char zClose[1024];

using namespace os;

class NewDrop : public DropdownMenu
{
public:
	NewDrop();
};


class NewFrameView : public FrameView
{
public:
	NewFrameView(Rect & r);
	NewDrop* m_pcDrop;
	TextView* m_pcDirectoryTextView;
	Button* m_pcFileButton;
	TextView* m_pcFileTextView;	
	Button* m_pcDirectoryButton;

private:
	StringView* m_pcDropView;
	StringView* m_pcFileStringView;
	StringView* m_pcDirectoryStringView;
};

class NewView : public View
{
public:
	NewView(const Rect & r);
	NewFrameView* m_pcFrameView;
	Button* m_pcCreate;
	Button* m_pcCancel;
};

class NewWindow : public Window
{
public:
	NewWindow();
	NewView* m_pcView;
	FileRequester* m_pcOpenSelect;
	FileRequester* m_pcSaveSelect;
	bool Read();
	void HandleMessage(Message* pcMessage);
        void Create(int nSel);
};

	

NewDrop::NewDrop() : DropdownMenu( Rect( 0, 0, 100, 15 ), "" )
{
    InsertItem( 0, "Bzip" );
    InsertItem( 1, "Gzip" );
    InsertItem( 2, "Tar" );
    SetSelection( 1, true );

}



NewFrameView::NewFrameView( Rect & r ) : FrameView( r, "new_view", "Create File" )
{
    m_pcDrop = new NewDrop();
    m_pcDrop->SetFrame( Rect( 0, 0, 100, 15 ) + Point( 110, 70 ) );
    m_pcDrop->SetReadOnly();
    AddChild( m_pcDrop );

    m_pcDropView = new StringView(Rect(0,0,0,0),"drop_str","Compress With:");
    m_pcDropView->SetFrame(Rect(0,0,80,15) + Point ( 30,71));
    AddChild(m_pcDropView);

    m_pcFileButton = new Button( Rect( 0, 0, 0, 0 ), "file_but", "Select", new Message(ID_NEW_SELECT_1) );
    m_pcFileTextView = new TextView( Rect( 0, 0, 0, 0 ), "file_text", "" );
    m_pcFileStringView = new StringView( Rect( 0, 0, 0, 0 ), "file_str", "Name:" );

   m_pcFileStringView->SetFrame( Rect( 0, 0, 60, 15 ) + Point( 10, 10 ) );
    m_pcFileTextView->SetFrame( Rect( 0, 0, 150, 20 ) + Point( 75, 10 ) );
    m_pcFileButton->SetFrame( Rect( 0, 0, 60, 20 ) + Point( 230, 10 ) );

    AddChild( m_pcFileButton );
    AddChild( m_pcFileTextView );
    AddChild( m_pcFileStringView );

    m_pcDirectoryTextView = new TextView( Rect( 0, 0, 0, 0 ), "file_text", "" );
    m_pcDirectoryStringView = m_pcFileStringView = new StringView( Rect( 0, 0, 0, 0 ), "file_str", "Directory:" );
    m_pcDirectoryButton = new Button( Rect( 0, 0, 0, 0 ), "file_but", "Select", new Message(ID_NEW_SELECT_2) );

    m_pcDirectoryStringView->SetFrame( Rect( 0, 0, 55, 15 ) + Point( 10, 40 ) );
    m_pcDirectoryTextView->SetFrame( Rect( 0, 0, 150, 20 ) + Point( 75, 40 ) );
    m_pcDirectoryButton->SetFrame( Rect( 0, 0, 60, 20 ) + Point( 230, 40 ) );



    AddChild( m_pcDirectoryButton );
    AddChild( m_pcDirectoryTextView );
    AddChild( m_pcDirectoryStringView );

    m_pcFileTextView->SetTabOrder(0);
    m_pcFileButton->SetTabOrder(1);
    m_pcDirectoryButton->SetTabOrder(3);
    m_pcDirectoryTextView->SetTabOrder(2);


}



NewView::NewView( const Rect & r ) : View( r, "" )
{
    Rect r( 0, 0, 310, 100 );
    r.Resize( 5, 5, 0, 0 );
    m_pcFrameView = new NewFrameView( r );
    AddChild( m_pcFrameView );
    m_pcCancel = new Button( Rect( 0, 0, 50, 25 ), "", "Cancel", new Message( ID_QUIT ) );
    m_pcCreate = new Button( Rect( 0, 0, 50, 25 ), "", "Create", new Message(ID_NEW_CREATE) );

    m_pcCancel->SetFrame( Rect( 0, 0, 60, 20 ) + Point( GetBounds().Width()/2-105, 105 ) );
    m_pcCreate->SetFrame( Rect( 0, 0, 60, 20 ) + Point( GetBounds().Width()/2+55, 105 ) );

    AddChild( m_pcCancel );
    AddChild( m_pcCreate );

    m_pcCancel->SetTabOrder(4);
    m_pcCreate->SetTabOrder(5);

}



NewWindow::NewWindow() : Window( Rect( 0, 0, 315, 130 ), "new_file", "Create New File", WND_NOT_RESIZABLE | WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT )
{

   // pcParentWindow = pcParent;
    Desktop dDesktop;
    Rect dBounds = GetBounds();
    m_pcView = new NewView( dBounds );
    AddChild( m_pcView );
    MoveTo( dDesktop.GetResolution().x / 2 - dBounds.Width() / 2, dDesktop.GetResolution().y / 2 - dBounds.Height() / 2 );
    SetFocusChild(m_pcView->m_pcFrameView->m_pcFileTextView);
    SetDefaultButton(m_pcView->m_pcCreate);
    m_pcOpenSelect = new FileRequester( FileRequester::LOAD_REQ, new Messenger( this ), "/", NODE_DIR, false, NULL, NULL, true, true, "Open", "Cancel" );
    m_pcSaveSelect = new FileRequester( FileRequester::SAVE_REQ, new Messenger( this ), "/", NODE_FILE, false, NULL, NULL, true, true, "Save", "Cancel" );
    //Read();
}

bool NewWindow::Read()
{
    char* zConfigFile;
    
    sprintf( zConfigFile, "%s/config/Archiver/Archiver.cfg", getenv( "HOME" ) );
    ifstream ConfigFile;
    char junk[ 1024 ];

    ConfigFile.open( zConfigFile );
    if ( !ConfigFile.eof() )
    {
        for(int i=0; i<=6; i++){
            ConfigFile.getline( junk, 1024 );
        }
        ConfigFile.getline( zClose, 1024 );
    }
    ConfigFile.close();

    return false;
}

void NewWindow::HandleMessage( Message* pcMessage )
{

    switch ( pcMessage->GetCode() )
    {

    case ID_QUIT:
        NewWindow::Close();
        break;

    case ID_NEW_SELECT_1:
        {
            m_pcSaveSelect->Show();
            m_pcSaveSelect->MakeFocus();
        }
        break;

    case ID_NEW_SELECT_2:
        {


            m_pcOpenSelect->Show();
            m_pcOpenSelect->MakeFocus();
        }
        break;

    case M_LOAD_REQUESTED:
        {
            if ( pcMessage->FindString( "file/path", &pzSelectPath ) == 0 )
            {
                m_pcView->m_pcFrameView->m_pcDirectoryTextView->Set(pzSelectPath);
            }
        }
        break;

    case M_SAVE_REQUESTED:
        {
            if ( pcMessage->FindString( "file/path", &pzSelectPath ) == 0 )
            {
                m_pcView->m_pcFrameView->m_pcFileTextView->Set(pzSelectPath);
            }
        }
        break;

    case ID_NEW_CREATE:
        {
            int nSel = m_pcView->m_pcFrameView->m_pcDrop->GetSelection();
            Create(nSel);
        }
        break;
    }
}


void NewWindow::Create(int Sel)
{
    String cDirect = m_pcView->m_pcFrameView->m_pcDirectoryTextView->GetBuffer()[0];
    String cArchive = m_pcView->m_pcFrameView->m_pcFileTextView->GetBuffer()[0];
    char openArc[1024] = "";
    Message* newMsg;
    newMsg = new Message(ID_GOING_TO_VIEW);
    newMsg->AddString("newPath",cArchive.c_str());

    if(Sel == 0){
        strcpy(openArc,"");
        strcat(openArc, "tar -cjf ");
        strcat(openArc,cArchive.c_str());
        strcat(openArc,"  ");
        strcat(openArc,"*");
        chdir(cDirect.c_str());
        system(openArc);
        sleep(1);
       // pcParentWindow->PostMessage(newMsg,pcParentWindow);
        //if(strstr(close,"Yes")){
          //  PostMessage(M_QUIT);
        //}
    }



    if(Sel == 1){

        strcpy(openArc,"");
        strcat(openArc, "tar -czf ");
        strcat(openArc,cArchive.c_str());
        strcat(openArc,"  ");
        strcat(openArc,"*");
        chdir(cDirect.c_str());
        system(openArc);
        sleep(1);
        //pcParentWindow->PostMessage(newMsg,pcParentWindow);
        //if(strstr(close,"Yes")){
          //  PostMessage(M_QUIT);
        //}

    }


    if(Sel == 2){
        strcpy(openArc,"");
        strcat(openArc, "tar -cf ");
        strcat(openArc,cArchive.c_str());
        strcat(openArc,"  ");
        strcat(openArc,"*");
        chdir(cDirect.c_str());
        system(openArc);
        sleep(1);
        //pcParentWindow->PostMessage(newMsg,pcParentWindow);
        //if(strstr(close,"Yes")){
          //  PostMessage(M_QUIT);
        //}
    }
}



/*NewWindow::~NewWindow()
{
    Invoker *pcParentInvoker = new Invoker(new Message(ID_NEW_CLOSE),pcParentWindow);
    pcParentInvoker->Invoke();

}*/

