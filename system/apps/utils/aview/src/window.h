//  AView 0.2 -:-  (C)opyright 2000-2001 Kristian Van Der Vliet
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Library General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Library General Public License for more details.
//
//  You should have received a copy of the GNU Library General Public
//  License along with this library; if not, write to the Free
//  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
//  MA 02111-1307, USA
//

#include <gui/window.h>
#include <gui/scrollbar.h>
#include <util/message.h>
#include <gui/menu.h>
#include <gui/filerequester.h>
#include "loadbitmap.h"
#include <gui/view.h>
#include <gui/desktop.h>
#include <gui/requesters.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <vector.h>

#include <stdio.h>
#include <strings.h>

enum Menus{ID_FILE_LOAD,ID_VOID,ID_EXIT, ID_ABOUT};
const unsigned int MENU_OFFSET=20;

using namespace os;

class BitmapView : public View
{
public:
    BitmapView(Bitmap* pcBitmap);
    void Paint( const Rect& cUpdateRect );
    void KeyDown( const char* pzString , const char* pzRawString, uint32 nQualifiers);
    void MouseDown( const Point &nPos, uint32 nButtons );

private:
    Bitmap* m_pcBitmap;
    View* main_bitmap_view;
};

class AppWindow : public Window
{
public:
    AppWindow(const Rect& cFrame);
    void AddItems(Window* m_pcMainWindow);
    void DispatchMessage(Message* pcMsg, Handler* pcHandler);
    virtual void HandleMessage(Message* pcMessage);
    virtual bool AppWindow::OkToQuit( void );
    void Load(const char *cFileName);
    void HandleKeyEvent( int nKeyCode );
    void BuildDirList(const char* pzFPath);

private:
    Rect cWindowBounds;
    Rect cMenuBounds;

    FileRequester* m_pcLoadRequester;
    const char* pzFPath;
    struct stat FPath_buf;
    struct dirent *FDir_entry;
    DIR* FDir_handle;

    std::vector <std::string> file_list;
    uint32 current_image;
    const char* next_image;

    Window* MainWindow;
    Bitmap* main_bitmap;
    View* main_bitmap_view;
    Rect pcBitmapRect;
    void SetupMenus(Window*);
    float winWidth;
    float winHeight;

    int nKeyCode;
    bool bSetTitle;
};

AppWindow::AppWindow(const Rect& cFrame) : Window(cFrame, "main_window", "AView 0.3b")
{
    m_pcLoadRequester=NULL;
}

void AppWindow::AddItems(Window* pc_MainWindow)
{
    float win_width, win_height;
    SetupMenus(pc_MainWindow);
    Rect pcWinBounds=pc_MainWindow->GetBounds();
    win_width=( pcWinBounds.right - pcWinBounds.left );
    win_height=( ( pcWinBounds.bottom - pcWinBounds.top ) - MENU_OFFSET );

    main_bitmap=new Bitmap( win_width, win_height, CS_RGB32, 0x0000);
    main_bitmap_view = new BitmapView ( main_bitmap );

    main_bitmap_view->MoveBy(0,MENU_OFFSET+1);  // Move the view/bitmap down so it does not obscure the menu

    Rect pcBitmapRect=main_bitmap_view->GetBounds();
    float winWidth=pcBitmapRect.left+pcBitmapRect.right;
    float winHeight=pcBitmapRect.top+pcBitmapRect.bottom+MENU_OFFSET;

    pc_MainWindow->ResizeTo(winWidth,winHeight);
    pc_MainWindow->AddChild( main_bitmap_view );

    main_bitmap_view->MakeFocus();
    MainWindow=pc_MainWindow;

    // We need to reset a few variables etc

    current_image=0;

    //MainWindow->AddChild(new ScrollBar(Rect(385,22,400,400),"sc",NULL,CF_FOLLOW_ALL));

}

void AppWindow::SetupMenus(Window* pc_MainWindow)
{
    cMenuBounds = pc_MainWindow->GetBounds();
    cMenuBounds.bottom = MENU_OFFSET;

    Menu* pcMenuBar = new Menu(cMenuBounds, "main_menu", ITEMS_IN_ROW, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT, WID_FULL_UPDATE_ON_H_RESIZE);

    // Create the menus within the bar
    Menu* pcAppMenu = new Menu(Rect(0,0,0,0),"Application",ITEMS_IN_COLUMN);
    pcAppMenu->AddItem("About...", new Message(ID_ABOUT));
    pcAppMenu->AddItem(new MenuSeparator());
    pcAppMenu->AddItem("Exit", new Message (ID_EXIT));

    Menu* pcFileMenu = new Menu(Rect(0,0,1,1),"File", ITEMS_IN_COLUMN);
    pcFileMenu->AddItem("Open...", new Message( ID_FILE_LOAD));


    pcMenuBar->AddItem(pcAppMenu);
    pcMenuBar->AddItem(pcFileMenu);

    pcMenuBar->SetTargetForItems(this);

    // Add the menubar to the window

    pc_MainWindow->AddChild(pcMenuBar);
}

void AppWindow::DispatchMessage( Message* pcMsg, Handler* pcHandler)
{
    Window::DispatchMessage(pcMsg, pcHandler);
}

void AppWindow::HandleMessage(Message* pcMessage)
{

    switch(pcMessage->GetCode()) //Get the message code from the message
    {
    case ID_FILE_LOAD:
        {
            if (m_pcLoadRequester==NULL)
                m_pcLoadRequester = new FileRequester(FileRequester::LOAD_REQ, new Messenger(this), "/");
            m_pcLoadRequester->Show();
            m_pcLoadRequester->MakeFocus();
        }

    case M_LOAD_REQUESTED:
        {
            if(pcMessage->FindString( "file/path", &pzFPath) == 0)
            {

                if( !stat ( pzFPath, &FPath_buf ) )
                {
                    if( FPath_buf.st_mode & S_IFDIR )
                    {
                        BuildDirList(pzFPath);
                        break;
                    }
                    else
                    {
                        Load( pzFPath );
                        break;
                    }
                }
            }

            break;
        }

    case M_KEY_DOWN:
        // This is the message passed back from BitmapView::Keydown.  We *know*
        // that the message has the data we need, so we'll get it & handle it from here.
        {
            pcMessage->FindInt( "_raw_key", &nKeyCode);
            HandleKeyEvent(nKeyCode);
            break;
        }

    case ID_EXIT:
        OkToQuit();
        break;
    case ID_ABOUT:
        {
            Alert* pcAbout = new Alert("About AView","AView 0.3  Beta\n\nImage Viewer for Syllable    \nCopyright 2002 - 2003 Syllable Desktop Team\n\nAView is released under the GNU General\nPublic License. Please go to www.gnu.org\nmore information.\n",  Alert::ALERT_INFO, 0x00, "OK", NULL);
            pcAbout->CenterInWindow(this);
            pcAbout->Go(new Invoker());
            break;
        }

    default:
        Window::HandleMessage( pcMessage);
        break;
    }
}

bool AppWindow::OkToQuit( void )
{
    Application::GetInstance()->PostMessage( M_QUIT);
    return( false );
}


void AppWindow::Load(const char *cFileName)
// Load requested file
{

    if(strcmp(cFileName,""))  // This is a safeguard aginst use being passed an empty string.
        // Easier than finding the actual bug at the moment! ;)
    {

        if ( strstr(cFileName, ".jpg") || (strstr(cFileName,".png")) || (strstr(cFileName, ".gif")) )
        {

            bSetTitle=true;
            Desktop d;
            // Get rid of the old bitmap
            MainWindow->RemoveChild(main_bitmap_view);

            // delete the bitmap to free up precious memory!
            delete main_bitmap;

            // load the new jpeg & create the BitmapView for it
            char img_fname[128];           //
            strcpy(img_fname,cFileName);   // This needs fixing! :)

            main_bitmap=LoadBitmapFromFile(img_fname);

            if(main_bitmap==NULL)
            {         // We wern't able to load the given file
                float win_width=300;
                float win_height=275-MENU_OFFSET;

                // We're going to make an empty bitmap to handle not being able to load the
                // given bitmap

                main_bitmap=new Bitmap( win_width, win_height, CS_RGB32, 0x0000);
                bSetTitle=false;  // We don't want this filename in the title now
            }

            main_bitmap_view = new BitmapView ( main_bitmap );

            main_bitmap_view->MoveBy(0,MENU_OFFSET+1);  // Move the view/bitmap down so it does not obscure the menu

            // We need to know the size of the new bitmap so we can match the window to it
            pcBitmapRect=main_bitmap_view->GetBounds();

            winWidth=pcBitmapRect.left+pcBitmapRect.right;
            winHeight=pcBitmapRect.top+pcBitmapRect.bottom+MENU_OFFSET;

            // Now we can resize the window to match the size of the bitmap
            if (winWidth >= d.GetResolution().x || winHeight >= d.GetResolution().y)
            {
                MainWindow->MoveTo(0,-5);
                MainWindow->ResizeTo(winWidth, winHeight);
            }

            else if (winWidth > MainWindow->GetBounds().Width()  || winHeight > MainWindow->GetBounds().Height())
            {
                MainWindow->ResizeTo(winWidth, winHeight);
            }



            // And finally, we can add the new BitmapView to the window
            MainWindow->AddChild( main_bitmap_view );

            // Just to make things a little useful, set the filename in the window bar
            if(bSetTitle==true)
            {
                MainWindow->SetTitle(cFileName);
            }
            else
            {
                MainWindow->SetTitle("AView 0.3b");
            }

        }

        else
        {
            Alert* pcError = new Alert("Not Supported","This filetype is not supported by AtheOS\n", 0x00, "OK",NULL);
            pcError->Go(new Invoker());
        }
    }
}

void AppWindow::HandleKeyEvent( int nKeyCode )
{
    switch(nKeyCode)
    {

    case 30:  // Backspace - Go to previous image
        {
            if(current_image)      // If we arn't already at the last image
            {
                current_image--;        // We decrement the counter that points to the image currently loaded

                next_image=file_list[current_image].c_str();  // We need the c style string of the first image
                //printf("Loading %s\n",next_image);
                Load( next_image );                           // so that we can load it!

                main_bitmap_view->MakeFocus();
            }

            break;
        }

    case 38:   // Tab
    case 94:   // Space - Go to next image
        {
            if(current_image!=file_list.size())  // If we arn't at the end of the list
            {
                current_image++;

                next_image=file_list[current_image].c_str();  // We need the c style string of the first image
                //printf("Loading %s\n",next_image);
                Load( next_image );                           // so that we can load it!

                main_bitmap_view->MakeFocus();
            }

            break;
        }

    default:   // This isn't a recognised key code that we need to deal with
        break;

    }

}

void AppWindow::BuildDirList(const char* pzFPath)
{
    FDir_handle=opendir( pzFPath);

    char file_path[256];

    while((FDir_entry = readdir(FDir_handle)))
    {
        if(strcmp(FDir_entry->d_name,".") && strcmp(FDir_entry->d_name,".."))
        {

            strcpy( file_path, (char*) pzFPath);
            strcat( file_path, "/");
            strcat( file_path, FDir_entry->d_name);
            strcat( file_path, "\0");

            AppWindow::file_list.push_back( file_path);  // We store every file in a vector

        }

    }

    current_image=0;
    const char* first_image;

    first_image=file_list[current_image].c_str();  // We need the c style string of the first image
    Load( first_image );                           // so that we can load it!


}

// This is the BitmapView method.  Mostly a container class, with a few overriden os::View
// methods to trap events we want to know about.


BitmapView::BitmapView(Bitmap* pcBitmap) : View( pcBitmap->GetBounds() , "bitmap_view", CF_FOLLOW_NONE )
{
    m_pcBitmap = pcBitmap;
}

void BitmapView::Paint( const Rect& cUpdateRect )
{
    Rect cRect = m_pcBitmap->GetBounds();
    DrawBitmap( m_pcBitmap, cRect, cRect );
}

void BitmapView::KeyDown( const char* pzString , const char* pzRawString, uint32 nQualifiers)
{
    // Key down events are traped by this over-riden method.  All we do is take the message created, and pass it
    // stright back to our AppWindow::HandleMessage method.  Why?  Because AppWindow is where most of the code lives,
    // and it is simply cleaner and more logical to handle this event from HandleMessage then down here.

    Message* pcMessage = GetWindow()->GetCurrentMessage();
    GetWindow()->HandleMessage(pcMessage);
}

void BitmapView::MouseDown( const Point &nPos, uint32 nButtons )
{
    GetWindow()->FindView(nPos)->MakeFocus();
}













