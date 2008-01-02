/*
	API Browser
	1.0.0
	by Brent P. Newhall <brent@other-space.com>
	with Modifications by Rick Caudill <syllable@syllable-desk.tk>
*/

#include "browser.h"
#include "messages.h"
#include "commonfuncs.h"

#include <util/regexp.h>

using namespace os;

APIBrowserWindow :: APIBrowserWindow(Window* pcParent, const Rect& r ): Window( r, "BrowserWindow", "Application Programming Interface Browser...", 0, CURRENT_DESKTOP )
{
	SetupMenus();
	SetupStatusBar();
	
	pcParentWindow = pcParent;
	
	
	/* Tree */
	treeView = new TreeView(r,"TreeView", 0, CF_FOLLOW_ALL,WID_WILL_DRAW );
	treeView->InsertColumn( "", (int)(COORD_MAX) );
	treeView->SetHasColumnHeader( false );
	PopulateTree( "/system/development/headers", 1 );
	treeView->SetSelChangeMsg( new Message( MSG_FILE_SELECTED ) );
	/* Code View */
	pcEditor = new CodeView( r, "CodeView", "", CF_FOLLOW_ALL );
	pcEditor->SetReadOnly( true );
	pcEditor->SetFormat( new Format_C() );
	pcEditor->SetShowLineNumbers(false);
	
	/* Splitter */
	splitter = new Splitter(Rect(0,menu->GetFrame().bottom+1,GetBounds().Width()+1,GetBounds().Height()-20), "Splitter", treeView,pcEditor,HORIZONTAL,CF_FOLLOW_ALL,WID_WILL_DRAW);
	splitter->SplitTo( 150 );
	AddChild( splitter );
	
	SetIcon(LoadImageFromResource("icon24x24.png")->LockBitmap());
}

void APIBrowserWindow::SetupStatusBar()
{
	statusbar = new StatusBar(Rect(0,GetBounds().Height()-20,GetBounds().Width(),GetBounds().Height()), "browser_status",1);
	statusbar->ConfigurePanel(0, StatusBar::FILL, GetBounds().Width()+1);
	AddChild(statusbar);
}

void APIBrowserWindow::SetupMenus()
{
	menu = new Menu(Rect(0,0,1,1),"menu",ITEMS_IN_ROW);
	
	Menu* fileMenu = new Menu(Rect(),"_File",ITEMS_IN_COLUMN);
	fileMenu->AddItem("E_xit",new Message(M_FILE_CLOSE));
	menu->AddItem(fileMenu);
	
	Menu* findMenu = new Menu(Rect(),"F_ind",ITEMS_IN_COLUMN);
	findMenu->AddItem("Find in _Documentation...",new Message(M_FIND_FIND));
	findMenu->AddItem("Find _Next",new Message(M_FIND_AGAIN));
	menu->AddItem(findMenu);
	
	menu->SetFrame(Rect(0,0,GetBounds().Width()+1,menu->GetPreferredSize(false).y+1));
	menu->SetTargetForItems(this);
	AddChild(menu);
}

void APIBrowserWindow ::PopulateTree( const String path, const int indent )
{
	String filename;
	String newpath;
	FSNode fsNode;
	TreeViewStringNode *node;
	Directory *dir = new Directory( path );
	status_t result;
	result = dir->GetNextEntry( &filename );
	
	while( result == 1 )
	{
		
		newpath = path;  newpath += "/";  newpath += filename;
		fsNode.SetTo( newpath );
		if( ! ( filename == "."  ||  filename == ".."  || filename == "include" ) )
		{
			node = new TreeViewStringNode();			
			node->AppendString(filename);
			node->SetIndent( indent );
			treeView->InsertNode( node );
			filenames.push_back( newpath );	
			if( fsNode.IsDir() )
			{
				PopulateTree( newpath, indent + 1 );
			}
		}
		result = dir->GetNextEntry( &filename );
	}
}

void APIBrowserWindow::UpdateStatus(const String& cText)
{
	statusbar->SetText(cText,0,0);
}

void APIBrowserWindow::HandleMessage( Message *pcMessage )
{
	int index;
	switch( pcMessage->GetCode() )
	{
		case MSG_FILE_SELECTED:
		{
			index = treeView->GetFirstSelected();
			File file( (filenames[index]) );
			char buffer[file.GetSize()];
			file.Read( &buffer,file.GetSize());
			buffer[file.GetSize()] = 0;
			pcEditor->SetCursor(0,0);			
			pcEditor->Clear();
			pcEditor->Set("");
			pcEditor->Set(buffer);
		
		}
		break;
		
		case M_FILE_CLOSE:
			OkToQuit();
			break;
		
		case M_FIND_FIND:
		{
			findDialog = new FindDialog(this,cSearchText,true,true,true);
			findDialog->CenterInWindow(this);
			findDialog->MakeFocus();
			findDialog->Show();	
		}
		break;
		
		case M_FIND_AGAIN:
		{
			if (!cSearchText.empty()) //just a test to make sure
			{
				Find(cSearchText,bSearchDown,bSearchCase,bSearchExtended);
			}
			break;			
		}
		
		case M_SEND_FIND:
		{
			pcMessage->FindString("text",&cSearchText);
			pcMessage->FindBool("down", &bSearchDown);
			pcMessage->FindBool("case", &bSearchCase);
			pcMessage->FindBool("extended", &bSearchExtended);
			Find(cSearchText, bSearchDown, bSearchCase, bSearchExtended);
			break;		
		}
			
		default:
			Window::HandleMessage( pcMessage );
	}
}

bool APIBrowserWindow :: OkToQuit( void )
{
	pcParentWindow->PostMessage(new Message(M_FIND_DOC_CANCEL),pcParentWindow);
	return( true );
}

void APIBrowserWindow::Find(const String &pcString, bool bDown, bool bCaseSensitive, bool bExtended)
{
    if (pcString.empty())
    {
        UpdateStatus("Search string empty!");
        return;
    }

    os::RegExp sRegularExpression;
    status_t nStatus=sRegularExpression.Compile(pcString, !bCaseSensitive, bExtended);
    
	switch(nStatus)
    {
    case os::RegExp::ERR_BADREPEAT:
        UpdateStatus("Error in regular expression: Bad repeat!");
        return;
    case os::RegExp::ERR_BADBACKREF:
        UpdateStatus("Error in regular expression: Bad back reference!");
        return;
    case os::RegExp::ERR_BADBRACE:
        UpdateStatus("Error in regular expression: Bad brace!");
        return;
    case os::RegExp::ERR_BADBRACK:
        UpdateStatus("Error in regular expression: Bad bracket!");
        return;
    case os::RegExp::ERR_BADPARENTHESIS:
        UpdateStatus("Error in regular expression: Bad parenthesis!");
        return;
    case os::RegExp::ERR_BADRANGE:
        UpdateStatus("Error in regular expression: Bad range!");
        return;
    case os::RegExp::ERR_BADSUBREG:
        UpdateStatus("Error in regular expression: Bad number in \\digit construct!");
        return;
    case os::RegExp::ERR_BADCHARCLASS:
        UpdateStatus("Error in regular expression: Bad character class name!");
        return;
    case os::RegExp::ERR_BADESCAPE:
        UpdateStatus("Error in regular expression: Bad escape!");
        return;
    case os::RegExp::ERR_BADPATTERN:
        UpdateStatus("Error in regular expression: Syntax error!");
        return;
    case os::RegExp::ERR_TOLARGE:
        UpdateStatus("Error in regular expression: Expression to large!");
        return;
    case os::RegExp::ERR_NOMEM:
        UpdateStatus("Error in regular expression: Out of memory!");
        return;
    case os::RegExp::ERR_GENERIC:
        UpdateStatus("Error in regular expression: Unknown error!");
        return;
    }

    os::IPoint start=pcEditor->GetCursor();

    if(bDown)
    {
        //test current line
        if(sRegularExpression.Search(pcEditor->GetLine(start.y), start.x))
        {//found!
            pcEditor->SetCursor(start.x+sRegularExpression.GetEnd(), start.y, false, false);
            //pcEditor->Select(os::IPoint(start.x+sRegularExpression.GetStart(), start.y), os::IPoint(start.x+sRegularExpression.GetEnd(), start.y));
            Flush();
            return;
        }

        //test to end of file
        for(uint a=start.y+1;a<pcEditor->GetLineCount();++a)
        {
            if(sRegularExpression.Search(pcEditor->GetLine(a)))
            {//found!
                pcEditor->SetCursor(sRegularExpression.GetEnd(), a, false, false);
                //pcEditor->Select(os::IPoint(sRegularExpression.GetStart(), a), os::IPoint(sRegularExpression.GetEnd(), a));
                Flush();
                return;
            }
        }

        //test from start of file
        for(int a=0;a<=start.y;++a)
        {
            if(sRegularExpression.Search(pcEditor->GetLine(a)))
            {//found!
                pcEditor->SetCursor(sRegularExpression.GetEnd(), a, false, false);
                //pcEditor->Select(os::IPoint(sRegularExpression.GetStart(), a), os::IPoint(sRegularExpression.GetEnd(), a));
                Flush();
                return;
            }
        }

        //not found
        UpdateStatus("Search string not found!");
    }
    else
    {//search up
        //this isn't nice, but emulates searching from the end of each line

        //test current line
        if(sRegularExpression.Search(pcEditor->GetLine(start.y), 0, start.x))
        {//found!
            int begin=sRegularExpression.GetStart();
            int end=sRegularExpression.GetEnd();

            while(sRegularExpression.Search(pcEditor->GetLine(start.y), begin+1, start.x-begin-1))
            {
                end=begin+sRegularExpression.GetEnd()+1;
                begin+=sRegularExpression.GetStart()+1;
            };

            pcEditor->SetCursor(begin, start.y, false, false);
            //pcEditor->Select(os::IPoint(begin, start.y), os::IPoint(end, start.y));
            Flush();
            return;
        }

        //test to start of file
        for(int a=start.y-1;a>=0;--a)
        {
            if(sRegularExpression.Search(pcEditor->GetLine(a)))
            {//found!
                int begin=sRegularExpression.GetStart();
                int end=sRegularExpression.GetEnd();

                while(sRegularExpression.Search(pcEditor->GetLine(a), begin+1))
                {
                    end=begin+sRegularExpression.GetEnd()+1;
                    begin+=sRegularExpression.GetStart()+1;
                };

                pcEditor->SetCursor(begin, a, false, false);
                //pcEditor->Select(os::IPoint(begin, a), os::IPoint(end, a));
                Flush();
                return;
            }
        }

        //test from end of file
        for(int a=pcEditor->GetLineCount()-1;a>=start.y;--a)
        {
            if(sRegularExpression.Search(pcEditor->GetLine(a)))
            {//found!
                int begin=sRegularExpression.GetStart();
                int end=sRegularExpression.GetEnd();

                while(sRegularExpression.Search(pcEditor->GetLine(a), begin+1))
                {
                    end=begin+sRegularExpression.GetEnd()+1;
                    begin+=sRegularExpression.GetStart()+1;
                };

                pcEditor->SetCursor(begin, a, false, false);
                //pcEditor->Select(os::IPoint(begin, a), os::IPoint(end, a));
                Flush();
                return;
            }
        }
	//not found
	UpdateStatus("Search string not found!");
	}
}

