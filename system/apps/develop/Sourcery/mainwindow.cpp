//  Sourcery -:-  (C)opyright 2003-2004 Rick Caudill
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


#include <gui/textview.h>
#include <gui/font.h>
#include <gui/layoutview.h>
#include <util/regexp.h>
#include <util/event.h>
#include <storage/nodemonitor.h> 
#include <storage/registrar.h>


#include "mainwindow.h"
#include "messages.h"
#include "file.h"
#include "splash.h"
#include "fonthandle.h"
#include "toolhandle.h"
#include "execute.h"

#include <unistd.h>

using namespace os;
using namespace std;

/*************************************************************
* Description: Constructor called when file has been passed
* Author: Rick Caudill
* Date: Tue Mar 16 11:40:01 2004
*************************************************************/
MainWindow::MainWindow(AppSettings* pcSettings,const os::String& cString) : Window( os::Rect( 0, 0, 600, 600 ), "main_wnd",String((String)APP_NAME + (String)" v" + (String)APP_VERSION + (String)": "+ cString), 0/*os::WND_SINGLEBUFFER*/)
{
	pcMainSettings = pcSettings;

	SetupMenus();	
	SetupToolBar();
	SetupStatusBar();

	CenterInScreen();

	Init();

	/* Create registrar calls */
	
	pcGetFileEvent = os::Event::Register( "app/Sourcery/GetFilePath", "Returns the path of the opened file", this, M_REGISTRAR_GET_PATH );
	pcMakeFocusEvent = os::Event::Register( "app/Sourcery/MakeFocus", "Focus the window", this, M_REGISTRAR_MAKE_FOCUS );
	
	if (!strcmp(cString.c_str(),"") == 0)
	{
		Load(cString,true);
	}
	
	pcEditor->SetMessage(new Message(M_TEXT_INVOKED));
	os::Font* pcFont = new Font(GetFontProperties(pcMainSettings->GetFontHandle()));
	pcEditor->FontChanged( pcFont );
	pcFont->Release();
	

	AddChild(pcEditor);
	SetFocusChild(pcEditor);
	pcEditor->MakeFocus();
	os::BitmapImage* pcImage = LoadImageFromResource("icon24x24.png");
	SetIcon(pcImage->LockBitmap());
	delete( pcImage );
	
	if (pcMainSettings->GetSaveFileEveryMin())
	{
		AddTimer( this, 1, 60000000, false );
	}
	
	SetSizeLimits(Point(m_pcMenu->GetPreferredSize(false).x + 15,600), Point(100000.0f,100000.0f));	
	

}

MainWindow::~MainWindow()
{
	/* Delete registrar calls */
	delete( pcGetFileEvent );
	delete( pcMakeFocusEvent );

	if (pcMonitorFile != NULL)
	{
		pcMonitorFile->Unset();
		pcMonitorFile = NULL;
	}
	delete( m_pcContextMenu );
}

/*************************************************************
* Description: Initalize variables
* Author: Rick Caudill
* Date: Fri Jun  4 11:01:55 2004
*************************************************************/
void MainWindow::Init()
{
	pcOpenRequester = NULL;
	pcSaveRequester = NULL;
	
	bChanged = false;
	bNamed = false;
	bFound = false;
	
	cConvertor = "";
	cFile = "";
	sFile = NULL;
	pcFileProp = NULL;
	pcAbout = NULL;
	pcUserInserts = NULL;
	m_pcBrowserWindow = NULL;
	m_pcSettingsDialog = NULL;
	pcToolsWindow = NULL;
	pcMonitorFile = NULL;
	nMonitor = 0;
	
	pcEditor = new Editor(Rect(0,m_pcToolBar->GetFrame().bottom+1,GetBounds().Width(),GetBounds().Height() - m_pcStatusBar->GetFrame().Height()-1),this);
	pcEditor->SetEventMask(os::TextView::EI_CONTENT_CHANGED | os::TextView::EI_CURSOR_MOVED);
	pcEditor->SetShowLineNumbers(pcMainSettings->GetShowLineNumbers());
	pcEditor->SetHighlightColor(pcMainSettings->GetHighColor());
	pcEditor->SetBackground(pcMainSettings->GetBackColor());
	pcEditor->SetTextColor(pcMainSettings->GetTextColor());
	pcEditor->SetLineNumberBgColor(pcMainSettings->GetNumberColor());	
	pcEditor->SetContextMenu(m_pcContextMenu);	
	
}


/*************************************************************
* Description: Sets the toolbar
* Author: Rick Caudill
* Date: Tue Mar 16 11:40:01 2004
*************************************************************/
void MainWindow::SetupToolBar()
{
	m_pcToolBar = new ToolBar(Rect(),"", os::CF_FOLLOW_LEFT | os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_TOP, os::WID_WILL_DRAW );
	
	os::ImageButton* pcBreaker = new os::ImageButton(os::Rect(), "side_t_breaker", "",NULL,NULL, os::ImageButton::IB_TEXT_BOTTOM,false,false,false);
	pcBreaker->SetImage(LoadImageFromResource("breaker.png"));
	m_pcToolBar->AddChild(pcBreaker, ToolBar::TB_FIXED_MINIMUM);

	os::ImageButton* pcFileNew = new os::ImageButton(os::Rect(), "editor_new", "",new Message(M_FILE_NEW),NULL, os::ImageButton::IB_TEXT_BOTTOM,true,false,true);
	pcFileNew->SetImage(LoadImageFromResource("filenew.png")); 	
	m_pcToolBar->AddChild(pcFileNew, ToolBar::TB_FIXED_MINIMUM);
	
	os::ImageButton* pcFileOpen = new os::ImageButton(os::Rect(), "editor_open", "",new Message(M_FILE_OPEN),NULL, os::ImageButton::IB_TEXT_BOTTOM,true,false,true);
	pcFileOpen->SetImage(LoadImageFromResource("fileopen.png"));
	m_pcToolBar->AddChild(pcFileOpen, ToolBar::TB_FIXED_MINIMUM);

	os::ImageButton* pcFileSave = new os::ImageButton(os::Rect(), "editor_save", "",new Message(M_FILE_SAVE),NULL, os::ImageButton::IB_TEXT_BOTTOM,true,false,true);
	pcFileSave->SetImage(LoadImageFromResource("filesave.png"));
	m_pcToolBar->AddChild(pcFileSave, ToolBar::TB_FIXED_MINIMUM);

	os::ImageButton* pcFileSaveAs = new os::ImageButton(os::Rect(), "editor_saveas", "",new Message(M_FILE_SAVE_AS),NULL, os::ImageButton::IB_TEXT_BOTTOM,true,false,true);
	pcFileSaveAs->SetImage(LoadImageFromResource("filesaveas.png"));
	m_pcToolBar->AddChild(pcFileSaveAs, ToolBar::TB_FIXED_MINIMUM);

	pcBreaker = new os::ImageButton(os::Rect(), "side_t_breaker", "",NULL,NULL, os::ImageButton::IB_TEXT_BOTTOM,false,false,true);
	pcBreaker->SetImage(LoadImageFromResource("breaker.png"));
	m_pcToolBar->AddChild(pcBreaker, ToolBar::TB_FIXED_MINIMUM);

	os::ImageButton* pcEditCut = new os::ImageButton(os::Rect(), "editor_cut", "",new Message(M_EDIT_CUT),NULL, os::ImageButton::IB_TEXT_BOTTOM,true,false,true);
	pcEditCut->SetImage(LoadImageFromResource("editcut.png"));
	m_pcToolBar->AddChild(pcEditCut, ToolBar::TB_FIXED_MINIMUM);	

	os::ImageButton* pcEditCopy = new os::ImageButton(os::Rect(), "editor_copy", "",new Message(M_EDIT_COPY),NULL,os::ImageButton::IB_TEXT_BOTTOM,true,false,true);
	pcEditCopy->SetImage(LoadImageFromResource("editcopy.png"));
	m_pcToolBar->AddChild(pcEditCopy, ToolBar::TB_FIXED_MINIMUM);

	os::ImageButton* pcEditPaste = new os::ImageButton(os::Rect(), "editor_paste", "",new Message(M_EDIT_PASTE),NULL, os::ImageButton::IB_TEXT_BOTTOM,true,false,true);
	pcEditPaste->SetImage(LoadImageFromResource("editpaste.png"));
	m_pcToolBar->AddChild(pcEditPaste, ToolBar::TB_FIXED_MINIMUM);

	pcBreaker = new os::ImageButton(os::Rect(), "side_t_breaker", "",NULL,NULL, os::ImageButton::IB_TEXT_BOTTOM,false,false,false);
	pcBreaker->SetImage(LoadImageFromResource("breaker.png"));
	m_pcToolBar->AddChild(pcBreaker, ToolBar::TB_FIXED_MINIMUM);

	PopupMenu* pcFindPop = new PopupMenu(Rect(),"pop_find","Find",m_pcPopMenu,LoadImageFromResource("filefind.png"));
	pcFindPop->SetFrame(Rect());
	m_pcToolBar->AddChild(pcFindPop, ToolBar::TB_FIXED_MINIMUM);
	
	m_pcToolBar->SetFrame(Rect(0,m_pcMenu->GetFrame().bottom+1,GetBounds().Width(),m_pcMenu->GetFrame().bottom+38));
	AddChild(m_pcToolBar);
}

/*************************************************************
* Description: Sets the statusbar
* Author: Rick Caudill
* Date: Tue Mar 16 11:40:01 2004
*************************************************************/
void MainWindow::SetupStatusBar()
{
	m_pcStatusBar = new StatusBar(Rect(0,GetBounds().Height()-25,GetBounds().Width(),GetBounds().Height()),"",3);
	m_pcStatusBar->ConfigurePanel(0,StatusBar::CONSTANT, 100);
	m_pcStatusBar->ConfigurePanel(1,StatusBar::CONSTANT, 100);
	m_pcStatusBar->ConfigurePanel(2,StatusBar::FILL, 0);
	AddChild(m_pcStatusBar);
}

/*************************************************************
* Description: Sets all the menus
* Author: Rick Caudill
* Date: Tue Mar 16 11:40:01 2004
*************************************************************/
void MainWindow::SetupMenus()
{
	Rect cMenuFrame = GetBounds();

	m_pcMenu = new Menu(Rect(0,0,0,0), "main_menu", ITEMS_IN_ROW, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP );
	
	m_pcAppMenu = new Menu(Rect(0,0,0,0), "_Application", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP );
	m_pcFileMenu = new Menu(Rect(0,0,0,0), "_File", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP );
	m_pcEditMenu = new Menu(Rect(0,0,0,0), "_Edit", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP );
	m_pcFormatMenu = new Menu(Rect(0,0,0,0), "Fo_rmat", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP );		
	m_pcViewMenu = new Menu(Rect(0,0,0,0), "_View", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP );	
	m_pcFindMenu = GetFindMenu();
	m_pcHelpMenu = new Menu(Rect(0,0,0,0), "_Help", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP );
	//m_pcInsertMenu = new Menu(Rect(0,0,0,0),"Insert", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP );
	m_pcInsertCustomMenu = new Menu(Rect(),"Custom", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP);
	m_pcLanguageMenu = new Menu(Rect(),"_Language",ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP);
	m_pcDateMenu = new Menu(Rect(),"Date", ITEMS_IN_COLUMN,CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP);
	m_pcToolMenu = new Menu(Rect(),"Tool_s",ITEMS_IN_COLUMN,CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP);
	m_pcTemplateMenu = new Menu(Rect(), "New From Template",ITEMS_IN_COLUMN,CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP);

	pcRecentMenu = new Menu(Rect(),"ReOpen",ITEMS_IN_COLUMN,CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP);
	pcRecentMenu->AddItem("Clear Recent List",new Message(M_FILE_RECENT_CLEAR));
	pcRecentMenu->AddItem(new MenuSeparator());
	ReloadRecentMenu();

	//app menu
	m_pcAppMenu->AddItem(new MenuItem("About...",new Message(M_APP_ABOUT),""));
	m_pcAppMenu->AddItem(new MenuSeparator());
	m_pcAppMenu->AddItem(new MenuItem("Settings...",new Message(M_APP_SETTINGS),""));
	m_pcAppMenu->AddItem(new MenuSeparator());
	m_pcAppMenu->AddItem(new MenuItem("Quit",new Message(M_APP_QUIT),"CTRL+Q"));

	m_pcTemplateMenu->AddItem("Template Settings...",new Message(M_FILE_NEW_FROM_TEMPLATE_SETTINGS));
	m_pcTemplateMenu->AddItem(new MenuSeparator());


	//file menu
	m_pcFileMenu->AddItem(new MenuItem("_New",new Message(M_FILE_NEW),"CTRL+N"));
	m_pcFileMenu->AddItem(m_pcTemplateMenu);
	m_pcFileMenu->AddItem(new MenuSeparator());
	m_pcFileMenu->AddItem(new MenuItem("_Open...",new Message(M_FILE_OPEN),"CTRL+O"));
	m_pcFileMenu->AddItem(new MenuItem("Open Corresponding File...",new Message(M_FILE_OPEN_CORRES_FILE)));
	m_pcFileMenu->AddItem(new MenuItem("Open File's Directory...",new Message(M_FILE_OPEN_DIRECTORY)));
	m_pcFileMenu->AddItem(new MenuSeparator());
	m_pcFileMenu->AddItem(new MenuItem(pcRecentMenu,NULL));
	m_pcFileMenu->AddItem(new MenuSeparator());		
	m_pcFileMenu->AddItem(new MenuItem("_Save",new Message(M_FILE_SAVE),"CTRL+S"));
	m_pcFileMenu->AddItem(new MenuItem("Save As...",new Message(M_FILE_SAVE_AS)));
	m_pcFileMenu->AddItem(new MenuSeparator());
	m_pcFileMenu->AddItem(new MenuItem("Close",new Message(M_FILE_CLOSE),"CTRL+W"));
	m_pcFileMenu->AddItem(new MenuSeparator());
	m_pcFileMenu->AddItem(new MenuItem("Print...", new Message(M_FILE_PRINT)));
	m_pcFileMenu->AddItem(new MenuItem("Print Settings...", new Message(M_FILE_PRINT_SETTINGS)));
	m_pcFileMenu->AddItem(new MenuSeparator());
	m_pcFileMenu->AddItem(new MenuItem("Export to HTML...",new Message(M_FILE_EXPORT_TO_HTML),"CTRL+E"));
	m_pcFileMenu->AddItem(new MenuSeparator());		
	m_pcFileMenu->AddItem(new MenuItem("File Properties...",new Message(M_EDIT_FILE_PROPERTIES),"CTRL+P"));			
	m_pcFileMenu->GetItemAt(4)->SetEnable(false);
	m_pcFileMenu->GetItemAt(14)->SetEnable(false);
	m_pcFileMenu->GetItemAt(15)->SetEnable(false);

	//custom insert menu
	m_pcInsertCustomMenu->AddItem("Custom...",new Message(M_INSERT_CUSTOM));
	m_pcInsertCustomMenu->AddItem(new MenuSeparator());
	ReloadCustomItems(true);
	
	m_pcDateMenu->AddItem("Date and Time",new Message(M_INSERT_DATE_TIME));
	m_pcDateMenu->AddItem("Date",new Message(M_INSERT_DATE));
	m_pcDateMenu->AddItem("Time",new Message(M_INSERT_TIME));

	//edit menu
	m_pcEditMenu->AddItem(new MenuItem("Cut", new Message(M_EDIT_CUT)));
	m_pcEditMenu->AddItem(new MenuItem("Copy", new Message(M_EDIT_COPY)));
	m_pcEditMenu->AddItem(new MenuItem("Paste",new Message(M_EDIT_PASTE)));
	m_pcEditMenu->AddItem(new MenuSeparator());
	m_pcEditMenu->AddItem(new MenuItem("Select All",new Message(M_EDIT_SELECT_ALL)));
	m_pcEditMenu->AddItem(new MenuSeparator());
	m_pcEditMenu->AddItem(new MenuItem("Undo", new Message(M_EDIT_UNDO)));	 
	m_pcEditMenu->AddItem(new MenuItem("Redo", new Message(M_EDIT_REDO)));		

	m_pcFormatMenu->AddItem(new MenuItem("To Upper",new Message(M_TEXT_TO_UPPER),"CTRL+T"));
	m_pcFormatMenu->AddItem(new MenuItem("To Lower",new Message(M_TEXT_TO_LOWER),"CTRL+L"));
	m_pcFormatMenu->AddItem(new MenuSeparator());
	m_pcFormatMenu->AddItem(GetInsertMenu());

	Menu* m_pcToolBarMenu = new Menu(Rect(0,0,0,0), "ToolBars", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP );
	m_pcToolBarMenu->AddItem(m_pcMainToolBarCheck = new CheckMenu("Main ToolBar",NULL,true));
	m_pcToolBarMenu->AddItem(m_pcFindToolBarCheck = new CheckMenu("Find ToolBar",NULL,true));
	m_pcMainToolBarCheck->SetEnable(false);

	m_pcViewMenu->AddItem(m_pcToolBarMenu);
	m_pcViewMenu->AddItem(new MenuSeparator());
	m_pcViewMenu->AddItem(m_pcShowLineNumberCheck = new CheckMenu("Show Line Numbers",new Message(M_SHOW_LINE_NUMBERS),pcMainSettings->GetShowLineNumbers()));
	m_pcViewMenu->AddItem(new MenuSeparator());
	m_pcViewMenu->AddItem(new MenuItem("Zoom In",new Message(M_ZOOM_IN),"CTRL++"));
	m_pcViewMenu->AddItem(new MenuItem("Zoom Out",new Message(M_ZOOM_OUT),"CTRL+-"));

	AddLanguageInformation();
	
	m_pcToolMenu->AddItem("Tool Settings...",new Message(M_TOOLS_SETTINGS));
	m_pcToolMenu->AddItem(new MenuSeparator());
	ReloadCustomTools(true);

	m_pcMenu->AddItem(m_pcAppMenu); // app menu
	m_pcMenu->AddItem(m_pcFileMenu); // file menu
	m_pcMenu->AddItem(m_pcEditMenu); // edit menu
	m_pcMenu->AddItem(m_pcFormatMenu); //format menu
	m_pcMenu->AddItem(m_pcViewMenu); // view menu
	m_pcMenu->AddItem(m_pcFindMenu); // find menu
	m_pcMenu->AddItem(m_pcLanguageMenu); //formats menu
	m_pcMenu->AddItem(m_pcToolMenu); //tool menu
	m_pcMenu->AddItem(m_pcHelpMenu); // help menu
	
	//context menu
	m_pcContextMenu = new Menu(Rect(),"context_menu",ITEMS_IN_COLUMN,CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP);

	m_pcContextMenu->AddItem(new MenuItem("Cut", new Message(M_EDIT_CUT)));

	m_pcContextMenu->AddItem(new MenuItem("Copy", new Message(M_EDIT_COPY)));
	m_pcContextMenu->AddItem(new MenuItem("Paste",new Message(M_EDIT_PASTE)));
	m_pcContextMenu->AddItem(new MenuSeparator());
	m_pcContextMenu->AddItem(new MenuItem("Select All",new Message(M_EDIT_SELECT_ALL)));
	m_pcContextMenu->AddItem(new MenuSeparator());
	m_pcContextMenu->AddItem(new MenuItem("Undo", new Message(M_EDIT_UNDO)));	 
	m_pcContextMenu->AddItem(new MenuItem("Redo", new Message(M_EDIT_REDO)));
	m_pcContextMenu->AddItem(new MenuSeparator());
	m_pcContextMenu->AddItem(new MenuItem("File Properties...",new Message(M_EDIT_FILE_PROPERTIES)));	
	m_pcContextMenu->SetTargetForItems(this);

	m_pcPopMenu = GetFindMenu();
	m_pcPopMenu->SetTargetForItems(this);

	//size the frame correctly
	cMenuFrame.left = 0;
	cMenuFrame.top = 0;
	cMenuFrame.right = GetBounds().Width();
	cMenuFrame.bottom = m_pcMenu->GetPreferredSize(true).y;
	m_pcMenu->SetFrame( cMenuFrame );
	
	//set target for menuitems and then add the menu to the screen
	m_pcMenu->SetTargetForItems( this );
	
	AddChild( m_pcMenu );
}

/*************************************************************
* Description: Adds format information to format menu 
* Author: Rick Caudill
* Date: Fri Jun  4 10:59:08 2004
*************************************************************/
void MainWindow::AddLanguageInformation()
{
	for (uint i=0; i<GetFormats().size(); i++)
	{
		m_pcLanguageMenu->AddItem(GetFormats()[i].cName.c_str(),new Message(M_FORMAT_CHANGE));
	}
}	

Menu* MainWindow::GetInsertMenu()
{
	//ERROR: this is what is causing problems with inserts menu not closing... look at this
	Menu* pcReturnMenu = new Menu(Rect(),"Insert",ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_TOP | CF_FOLLOW_RIGHT);
	pcReturnMenu->AddItem(m_pcInsertCustomMenu);
	pcReturnMenu->AddItem(new MenuSeparator());
	pcReturnMenu->AddItem(m_pcDateMenu);
	pcReturnMenu->AddItem("Program Header",new Message(M_INSERT_PROGRAM_HEADER));
	pcReturnMenu->AddItem("Comment",new Message(M_INSERT_COMMENT));
	return pcReturnMenu;
}

Menu* MainWindow::GetFindMenu()
{
	Menu* pcMenu = new Menu(Rect(0,0,0,0), "F_ind", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP );
	pcMenu->AddItem(new MenuItem("Find...",new Message(M_FIND_FIND),"CTRL+F"));
	pcMenu->AddItem(new MenuItem("Find Next...",new Message(M_FIND_AGAIN), "F3"));
	pcMenu->AddItem(new MenuSeparator());
	pcMenu->AddItem(new MenuItem("Replace...",new Message(M_FIND_REPLACE),"CTRL+R"));
	pcMenu->AddItem(new MenuSeparator());
	pcMenu->AddItem(new MenuItem("Goto Line...",new Message(M_FIND_GOTO),"CTRL+G"));
	pcMenu->AddItem(new MenuSeparator());
	pcMenu->AddItem(new MenuItem("Lookup Documentation...",new Message(M_FIND_DOCUMENTATION), "F5"));
	return pcMenu;
}

/*************************************************************
* Description: Loads/Reloads Custom Items based on bool passed to it
* Author: Rick Caudill
* Date: Mon May 31 18:21:35 2004
*************************************************************/
void MainWindow::ReloadCustomItems(bool bFirstTime)
{
	if (!bFirstTime)
	{

		for (int i=m_pcInsertCustomMenu->GetItemCount(); i>=2; i--)
		{
			m_pcInsertCustomMenu->RemoveItem(i);
		}
	}	
		
	try
	{
		os::String zPath = getenv( "HOME" );
		zPath += "/Settings/Sourcery/Inserts";
		os::Settings* pcSettings = new os::Settings( new os::File(zPath ,O_RDONLY));
		pcSettings->Load();
		
		for (int i=0; i<pcSettings->CountNames(); i++)
		{
			String cName = pcSettings->GetName(i);
			String cInsertText;
			pcSettings->FindString(cName.c_str(),&cInsertText);
			m_pcInsertCustomMenu->AddItem(cName,LoadCustomItemMessage(cInsertText));
		}
		delete pcSettings;
		m_pcMenu->SetTargetForItems(this);
	}
	catch(...)
	{
	}
}

/*************************************************************
* Description: Loads/Reloads Custom Items based on bool passed to it
* Author: Rick Caudill
* Date: Mon May 31 18:21:35 2004
*************************************************************/
void MainWindow::ReloadCustomTools(bool bFirstTime)
{
	if (!bFirstTime)
	{

		for (int i=m_pcToolMenu->GetItemCount(); i>=2; i--)
		{
			m_pcToolMenu->RemoveItem(i);
		}
	}	
		
	try
	{
		os::String zPath = getenv( "HOME" );
		zPath += "/Settings/Sourcery/Tools";
		os::Settings* pcSettings = new os::Settings( new os::File(zPath ,O_RDONLY));
		pcSettings->Load();
		
		for (int i=0; i<pcSettings->CountNames(); i++)
		{
			ToolHandle* pcHandle;
			size_t nToolHandleSize;
			pcSettings->FindData(pcSettings->GetName(i).c_str(),T_RAW,(const void**)&pcHandle, &nToolHandleSize);
			m_pcToolMenu->AddItem(pcHandle->zName,LoadCustomToolsMessage(pcHandle));
		}
		delete pcSettings;
		m_pcMenu->SetTargetForItems(this);
	}
	catch(...)
	{
	}
}

/*************************************************************
* Description: Gets the items in the recent list
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 10:16:38 
*************************************************************/
vector<String> MainWindow::GetRecentList()
{	
    vector<String> vList;
    String cRecentName = String(getenv("HOME")) + String("/Settings/Sourcery/Lists");
    char zTemp[1024];        
    FILE* fin = fopen(cRecentName.c_str(), "r");

    if (fin)
    {
        while (fscanf(fin, "%s", zTemp) != EOF)
        {
            if (zTemp != NULL)
                vList.push_back(zTemp);
        }
        fclose(fin);
    }
    
    while( vList.size() > 10 )
    {
    	vList.erase( vList.begin() );
    }
    
    return (vList);
}

/*************************************************************
* Description: Reloads all items of the list
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 10:16:16 
*************************************************************/
void MainWindow::ReloadRecentMenu()
{
	vector<String> vList = GetRecentList();

	while (pcRecentMenu->GetItemCount() != 2)
	{
		pcRecentMenu->RemoveItem(2);
	}
	
	for (uint nSize = 0; nSize < vList.size(); nSize++)
	{
		pcRecentMenu->AddItem(new MenuItem(vList[nSize],new Message(M_FILE_RECENT)),2);
	}
	pcRecentMenu->SetTargetForItems(this);
}

/*************************************************************
* Description: Writes to the recent file list
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 10:15:47 
*************************************************************/
void MainWindow::SetRecentList(const String& cPath)
{
    vector<String> tList = GetRecentList();
    if (cPath.c_str() != "")
    {
    	String cRecentName = String(getenv("HOME")) + String("/Settings/Sourcery/Lists");
		FILE* fin = fopen(cRecentName.c_str(), "w");   	
        if (!fin);
        
        else
        {
            for (uint n=0; n<tList.size(); n++)
            {
                fprintf(fin, "%s\n",tList[n].c_str());
            }
            fprintf(fin, "%s\n",cPath.c_str());
            fclose(fin);
        }
    }
}

/*************************************************************
* Description: Loads a Custom Tool Message
* Author: Rick Caudill
* Date: Mon May 31 18:24:16 2004
*************************************************************/
Message* MainWindow::LoadCustomToolsMessage(ToolHandle* pcHandle)
{
	Message* pcMessage = new Message(M_TOOLS);
	pcMessage->AddString("command",pcHandle->zCommand);
	pcMessage->AddString("arguments",pcHandle->zArguments);
	return pcMessage;
}

/*************************************************************
* Description: Loads a Custom Message
* Author: Rick Caudill
* Date: Mon May 31 18:24:16 2004
*************************************************************/
Message* MainWindow::LoadCustomItemMessage(const String& cInsertedText)
{
	Message* pcMessage = new Message(M_INSERT_CUSTOM_ITEM);
	pcMessage->AddString("custom",cInsertedText);
	return pcMessage;
}


/*************************************************************
* Description: Depending on the message code this method will call a specific command
* Author: Rick Caudill
* Date: Tue Mar 16 11:40:01 2004
*************************************************************/
void MainWindow::HandleMessage( os::Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case M_APP_QUIT: //app is exiting
		{
			OkToQuit();
			break;
		}
		
		case M_APP_ABOUT: //aboutbox is being opened
		{
			if (pcAbout == NULL)
			{
				pcAbout = new AboutBoxWindow(this);
				pcAbout->CenterInWindow(this);
				pcAbout->Show();
				pcAbout->MakeFocus();
			}
			else
				pcAbout->MakeFocus();
			break;
		}
		
		case M_APP_ABOUT_REQUESTED:  //aboutbox is being closed
		{
			pcAbout->Close();
			pcAbout = NULL;
			break;
		}

		case M_APP_SETTINGS: //settings are being opened
		{
			if (m_pcSettingsDialog == NULL)
			{
				m_pcSettingsDialog = new SettingsDialog(this,pcMainSettings);
				m_pcSettingsDialog->CenterInWindow(this);
				m_pcSettingsDialog->Show();
				m_pcSettingsDialog->MakeFocus();
			}
			
			else
				m_pcSettingsDialog->MakeFocus();			
			break;
		}
		
		case M_SETTINGS_CANCEL:
		{
			m_pcSettingsDialog->Close();
			m_pcSettingsDialog = NULL;
			break;
		}
		
		case M_SETTINGS_SAVE:
		{
			pcMainSettings->SaveSettings();
			os::Font* pcFont = new Font(GetFontProperties(pcMainSettings->GetFontHandle()));
			pcEditor->FontChanged( pcFont );
			pcFont->Release();
			pcEditor->SetShowLineNumbers(pcMainSettings->GetShowLineNumbers());
			pcEditor->SetHighlightColor(pcMainSettings->GetHighColor());
			pcEditor->SetBackground(pcMainSettings->GetBackColor());
			pcEditor->SetTextColor(pcMainSettings->GetTextColor());
			pcEditor->SetLineNumberBgColor(pcMainSettings->GetNumberColor());				
			m_pcShowLineNumberCheck->SetChecked(pcMainSettings->GetShowLineNumbers());
			m_pcSettingsDialog->Close();
			m_pcSettingsDialog = NULL;
			break;
		}
		
		case M_FILE_NEW: //new
		{
			if (!bChanged)
			{
				CloseFile();
			}
			else
			{
				os::Message* pcMsg = new os::Message(M_CLOSE_FILE_SAVE);
				HandleMessage( pcMsg );
				delete( pcMsg );
				CloseFile();
			}
			break;
		}
		
		case M_FILE_NEW_FROM_TEMPLATE_SETTINGS:  //template settings
		{
			break;
		}
		
		case M_FILE_NEW_FROM_TEMPLATE:  //new from template
		{
			if (!bChanged)
			{
			}
			else
			{
			}			
			break;
		}
		
		//file openings
		case M_FILE_OPEN:  //open was called from the menu or toolbar button
		{
			if (pcOpenRequester == NULL)
			{
				pcOpenRequester = new FileRequester(FileRequester::LOAD_REQ,new Messenger(this),String(String(getenv("HOME")) + String("/")),FileRequester::NODE_FILE);
				pcOpenRequester->CenterInWindow(this);
				pcOpenRequester->MakeFocus();
				pcOpenRequester->Show();
			}
			else
			{
				pcOpenRequester->Lock();
				if( !pcOpenRequester->IsVisible() )
				{
					pcOpenRequester->CenterInWindow(this);
					pcOpenRequester->Show();
				}
				pcOpenRequester->MakeFocus();
				pcOpenRequester->Unlock();
			}
			break;
		}
		
		case M_FILE_OPEN_CORRES_FILE: //open corresponding file(only works with cpp/h files)
		{
			if (sFile != NULL)
			{
				OpenCorrespondingFile();
			}
			
			else
				ShowError(ERR_NO_FILE_SELECTED);
			break;
		}
		
		case M_FILE_OPEN_DIRECTORY:  //open files directory
		{
			if (sFile != NULL)
			{
				String cDir = GetDirectoryofFileOpened(sFile->GetFileName());
				Directory* pcDir = new Directory(cDir);
				if (pcDir->IsDir())
				{
					UpdateStatus("Opening diectory...",0);
					Execute* pcExecute = new Execute("/system/bin/FileBrowser",cDir);
					
					if (pcExecute->IsValid()) //just making sure that the FileBrowser is there
						pcExecute->Run();
				}
				else
				{
					ShowError(ERR_NOT_DIRECTORY);
				}
				delete( pcDir );
			}
			else
				ShowError(ERR_NO_FILE_SELECTED);
			break;
		}
		
		case M_FILE_SAVE: //save was called from the menu or toolbar button
		{
			if (cFile == "")  //there is no file
				PostMessage(M_FILE_SAVE_AS, this);
			else //there is a file now lets save it
				Save(cFile);
			break;
		}
		
		case M_FILE_SAVE_AS:  //save as was called
		{
			if (pcSaveRequester == NULL)
			{
				pcSaveRequester = new FileRequester(FileRequester::SAVE_REQ,new Messenger(this),String(String(getenv("HOME")) + String("/")),FileRequester::NODE_FILE);
				pcSaveRequester->CenterInWindow(this);
				pcSaveRequester->MakeFocus();
				pcSaveRequester->Show();
			}
			else
			{
				pcSaveRequester->Lock();
				if( !pcSaveRequester->IsVisible() )
				{
					pcSaveRequester->CenterInWindow(this);
					pcSaveRequester->Show();
				}
				pcSaveRequester->MakeFocus();
				pcSaveRequester->Unlock();
			}
			break;
		}
		
		case M_LOAD_REQUESTED: //load requester was closed
		{
			if (pcMessage->FindString("file/path",&cFile)==0)
				Load(cFile,false);
			break;
		}
		
		case M_SAVE_REQUESTED: //save requester was closed
		{
			if (pcMessage->FindString("file/path",&cFile)==0)
				Save(cFile);
			break;
		}
		
		
		case M_FILE_CLOSE: //closes the current file
		{
			CloseFile();
			break;
		}
		
		case M_FILE_RECENT: //reopens a file
		{
			void* pMenuItem;
			pcMessage->FindPointer("source",&pMenuItem);
			MenuItem* pcItem = static_cast<MenuItem*> (pMenuItem);
			Load(pcItem->GetLabel(),false);		
			break;
		}
		
		case M_FILE_RECENT_CLEAR:
		{
			while (pcRecentMenu->GetItemCount() != 2)
			{
				delete( pcRecentMenu->RemoveItem(2) );
			}
			String cRecentFile = String(getenv("HOME")) + String("/Settings/Sourcery/Lists");
			remove(cRecentFile.c_str());
			break;
		}
		
		case M_FILE_EXPORT_TO_HTML:  //export was called
		{
			LaunchConvertor();
			break;
		}
		
		case M_EDIT_FILE_PROPERTIES:  //file properties dialog was opened
		{
			if (sFile) //there must be a sourcery file
			{
				pcPassedMessage = new Message(1000);
				pcPassedMessage->AddString("rel",sFile->GetFilesRelativePath().c_str());
				pcPassedMessage->AddString("abs",sFile->GetFileName().c_str());
				pcPassedMessage->AddString("lines",sFile->GetLines().c_str());
				pcPassedMessage->AddString("time",sFile->GetDateTime()->GetDate().c_str());
				pcPassedMessage->AddInt32("size",sFile->GetSize());
				pcFileProp = new FileProp(this,pcPassedMessage);
				pcFileProp->CenterInWindow(this);
				pcFileProp->Show();
				pcFileProp->MakeFocus();
			}
			else //cannot go any further
				ShowError(ERR_NO_FILE_SELECTED);
			break;
		}
		
		case M_EDIT_FILE_PROPERTIES_REQUESTED: //file properties dialog was closed
		{
			pcFileProp->Quit();
			pcPassedMessage = NULL;
			break;
		}
		
		case M_EDIT_CUT: //cut text from the editor(will cut the from the current positon of the pen)
		{
			pcEditor->Cut();
			break;
		}
		
		case M_EDIT_COPY: //copy text into the editor(will copy in the current position of the pen)
		{
			pcEditor->Copy();
			break;
		}
		
		case M_EDIT_PASTE: //paste text into the editor(will paste in the current position of the pen)
		{
			pcEditor->Paste();
			break;
		}
		
		case M_EDIT_SELECT_ALL: //select all text in the editor
		{
			pcEditor->SelectAll();
			break;
		}
		
		case M_EDIT_UNDO: //undo an action in the editor 
		{
			pcEditor->Undo();
			break;
		}
		
		case M_EDIT_REDO:  //redo an action in the editor 
		{
			pcEditor->Redo();
			break;
		}

		/*insert messages*/
		case M_INSERT_DATE:  //insert the date
		{
			String cDate=GetDate();
			IPoint cPoint = pcEditor->GetCursor();
			pcEditor->Insert(cPoint,cDate.c_str());
			break;
		}
		
		case M_INSERT_TIME:  //insert the time
		{
			String cTime=GetTime();
			IPoint cPoint = pcEditor->GetCursor();
			pcEditor->Insert(cPoint,cTime.c_str());
			break;
		}			

		case M_INSERT_DATE_TIME:  //insert the date and time
		{
			String cDateTime=GetDateAndTime();
			IPoint cPoint = pcEditor->GetCursor();
			pcEditor->Insert(cPoint,cDateTime.c_str());
			break;
		}	
		
		case M_INSERT_COMMENT:  //insert a comment
		{
			if (sFile)
			{
				String cComment = GetComment(sFile->GetFilesExtension());
				IPoint cPoint = pcEditor->GetCursor();
				pcEditor->Insert(cPoint,cComment.c_str());
			}
			else
			{
				String cComment = GetComment(".cpp");
				IPoint cPoint = pcEditor->GetCursor();
				pcEditor->Insert(cPoint,cComment.c_str());				
			}
			break;
		}
		
		case M_INSERT_CUSTOM: //called when opening insert dialog
		{
			if (pcUserInserts == NULL)
			{
				pcUserInserts = new UserInserts(this);
				pcUserInserts->CenterInWindow(this);
				pcUserInserts->Show();
				pcUserInserts->MakeFocus();
			}
			else
				pcUserInserts->MakeFocus();
			break;
		}
		
		case M_USER_INSERT_QUIT:  //called when insert dialog closes
		{
			pcUserInserts->Close();
			pcUserInserts = NULL;
			ReloadCustomItems(false);	
			break;
		}
		
		case M_INSERT_PROGRAM_HEADER: //called when inserting program header
		{
			String cHeader=GetHeader();
			IPoint cPoint = pcEditor->GetCursor();
			pcEditor->Insert(cPoint,cHeader.c_str());
			break;
		}
		
		case M_INSERT_CUSTOM_ITEM: //called when inserting custom item
		{
			String cCustomItem;
			if (pcMessage->FindString("custom",&cCustomItem) == 0)
			{
				String cItem = ParseForMacros(cCustomItem,sFile);
				IPoint cPoint = pcEditor->GetCursor();
				pcEditor->Insert(cPoint,cItem.c_str());
				break;
			}
		}

		case M_ZOOM_IN: //called when you want to zoom in
		{
			ZoomIn();
			break;
		}
		
		case M_ZOOM_OUT:  //called when you want to zoom out
		{
			ZoomOut();
			break;
		}
		
		case M_CLOSE_FILE_SAVE: //close file???
		{
			Alert* pcCloseAlert = new Alert("Do you wish to save?","You have made changes to this file.\nDo you wish to save these changes?\n",Alert::ALERT_QUESTION,0,"Yes","No",NULL);
			pcCloseAlert->CenterInWindow(this);			
			pcCloseAlert->Go(new Invoker(new Message(M_CLOSE_FILE_SAVE_REQUESTED),this));
			pcCloseAlert->MakeFocus();
			break;
		}	
		
		case M_CLOSE_FILE_SAVE_REQUESTED: //close file alert was clicked
		{
			int32 nWhich;
			pcMessage->FindInt32("which",&nWhich);
			if (nWhich == 0)
			{
				os::Message cMsg( M_FILE_SAVE );
				HandleMessage( &cMsg );
			}
			bChanged = false;
			OkToQuit();
			break;
		}
		
		case M_FIND_GOTO:  //this case is called when you want to open goto dialog
		{
			pcGoToDialog = new GoToDialog(this,pcMainSettings->GetAdvancedGoToDialog());
			pcGoToDialog->CenterInWindow(this);
			pcGoToDialog->MakeFocus();
			pcGoToDialog->Show();
			break;
		}
		
		case M_GOTO_GOTO:  //this case is called when you click "Go to" on the go to dialog
		{
			int32 nCol, nLine;
			pcMessage->FindInt32("goto_line",&nLine);
			pcMessage->FindInt32("goto_col",&nCol);
			pcGoToDialog->Close();
			GoTo(nCol,nLine);
			pcEditor->MakeFocus();
			break;
		}

		case M_FIND_FIND:
		{
			pcFindDialog = new FindDialog(this,cSearchText,true,false,true);
			pcFindDialog->CenterInWindow(this);
			pcFindDialog->MakeFocus();
			pcFindDialog->Show();
			break;
		}

		case M_FIND_AGAIN:
		{
			if (!cSearchText.empty()) //just a test to make sure
			{
				Find(cSearchText,bSearchDown,bSearchCase,bSearchExtended);
			}
			break;
		}
		
		case M_REPLACE_DO:
		{
			String cReplaceText;
			pcMessage->FindString("text",&cSearchText);
			pcMessage->FindBool("down", &bSearchDown);
			pcMessage->FindBool("case", &bSearchCase);
			pcMessage->FindBool("extended", &bSearchExtended);
			
			if (Find(cSearchText, bSearchDown, bSearchCase, bSearchExtended) == true && !pcMessage->FindString("replace_text",&cReplaceText))
			{
				pcEditor->Delete();
				pcEditor->Insert(cReplaceText.c_str(),false);	
			}
			break;
		}
		
		case M_REPLACE_ALL_DO:
		{
			String cReplaceText;
			pcMessage->FindString("text",&cSearchText);
			pcMessage->FindBool("down", &bSearchDown);
			pcMessage->FindBool("case", &bSearchCase);
			pcMessage->FindBool("extended", &bSearchExtended);
			
			if (!pcMessage->FindString("replace_text",&cReplaceText))
			{
				while (Find(cSearchText, bSearchDown, bSearchCase, bSearchExtended) == true)
				{
					pcEditor->Delete();
					pcEditor->Insert(cReplaceText.c_str(),false);	
				}				
			}
			break;
		}
		
		case M_SEND_FIND: //find dialog found sends this
		{
			pcMessage->FindString("text",&cSearchText);
			pcMessage->FindBool("down", &bSearchDown);
			pcMessage->FindBool("case", &bSearchCase);
			pcMessage->FindBool("extended", &bSearchExtended);
			Find(cSearchText, bSearchDown, bSearchCase, bSearchExtended);
			break;
		}
		
		case M_FIND_REPLACE: //replace dialog is called
		{
			pcReplaceDialog = new ReplaceDialog(this,cSearchText,true,true,true);
			pcReplaceDialog->CenterInWindow(this);
			pcReplaceDialog->Show();
			pcReplaceDialog->MakeFocus();
			break;
		}
		
		case M_FIND_DOCUMENTATION: //the api doc window is opened
		{
			RegistrarManager* pcManager = RegistrarManager::Get();
			if( pcManager )
			{
				pcManager->Launch( NULL, "/boot/Documentation/gui/index.html" );
				pcManager->Put();
			}
			#if 0
			if (m_pcBrowserWindow == NULL)
			{
				m_pcBrowserWindow = new APIBrowserWindow(this,Rect(0,0,600,450));
				m_pcBrowserWindow->CenterInWindow(this);
				m_pcBrowserWindow->Show();
				m_pcBrowserWindow->MakeFocus();
			}
			else
			{
				m_pcBrowserWindow->MakeFocus();
			}
			#endif
			break;
		}
		
		case M_FIND_DOC_CANCEL: //the api doc was cancelled
		{
			m_pcBrowserWindow->Close();
			m_pcBrowserWindow = NULL;
			break;
		}

		case M_FORMAT_CHANGE: //the user changed the format, shame on them :)
		{
			void* pMenuItem;
			pcMessage->FindPointer("source",&pMenuItem);
			MenuItem* pcItem = static_cast<MenuItem*> (pMenuItem);
			LoadNewFormat(pcItem->GetLabel());			
			break;
		}
		
		case M_SHOW_LINE_NUMBERS: //when called shows/unshows line numbering
		{
			pcEditor->SetShowLineNumbers(m_pcShowLineNumberCheck->IsChecked());
			pcEditor->SetCursor(pcEditor->GetCursor(),false,true);  //generic way
			break;
		}
		
		case M_TOOLS_SETTINGS: //the tools dialog is opened
		{
			if (pcToolsWindow == NULL)
			{
				pcToolsWindow = new ToolsWindow(this);
				pcToolsWindow->CenterInWindow(this);
				pcToolsWindow->Show();
				pcToolsWindow->MakeFocus();
			}
			
			else
			{
				pcToolsWindow->MakeFocus(true);
			}
			break;
		}
		
		case M_TOOLS_QUIT:  //the tools dialog was closed... lets clean up
		{
			bool bSavedTools;
			pcMessage->FindBool("saved",&bSavedTools);
			
			if (bSavedTools) //the save button in the tools dialog was clicked, lets reload the tools
				ReloadCustomTools(false);
			
			pcToolsWindow->Close();
			pcToolsWindow = NULL;
			break;
		}
		
		case M_TOOLS: //a tool was clicked
		{
			String cCommand;
			String cArguments;
			pcMessage->FindString("command",&cCommand);
			pcMessage->FindString("arguments",&cArguments);
			Execute* pcExecute = new Execute(cCommand,cArguments);
			pcExecute->Run();
			break;
		}
		
		case M_NODE_MONITOR: //the file has been modified outside of Sourcery, uh oh :)
		{
			nMonitor++;
			
			if(pcMainSettings->GetMonitorFile() && nMonitor == 1)
			{
				String cFileChanged = String("'") + sFile->GetFileName() + String("' has changed.\n\nWhat would you like to do?");
				Alert* pcAlert = new Alert("Sourcery v0.1: File Changed...",cFileChanged.c_str(),LoadImageFromResource("icon48x48.png")->LockBitmap(), 0, "Resave", "Close","Reload","Cancel",NULL); 
				MakeFocus(true);
				pcAlert->Go(new Invoker(new Message(M_NODE_MONITOR_REQUESTED),this));
				pcAlert->MakeFocus();
			}
			break;
		}
		
		case M_NODE_MONITOR_REQUESTED:  //node monitor alert was clicked
		{
			nMonitor = 0;
			int32 nWhich;
			pcMessage->FindInt32("which",&nWhich);
			
			switch (nWhich)
			{
				case 0:  //lets save the current file
					Save(sFile->GetFileName());
					break;
					
				case 1: //lets close the current file
					CloseFile();
					break;
					
				case 2: //lets reload the current file
					Load(sFile->GetFileName(),false);
					break;
				
				case 3: //lets cancel out
					break;
			}
			break;
		}
		
		case M_TEXT_TO_UPPER:
		{
			String cUpper="";
			pcEditor->GetRegion(&cUpper);
			if (cUpper.c_str() != "" && cUpper.size()!=0)
			{
				String cToUpper = cUpper.Upper();
				pcEditor->Delete();
				pcEditor->Insert(cToUpper.c_str(),false);
			}
			break;
		}
		
		case M_TEXT_TO_LOWER:
		{
			String cLower="";
			pcEditor->GetRegion(&cLower);
			if (cLower.c_str() != "" && cLower.size()!=0 )
			{
				String cToLower = cLower.Lower();
				pcEditor->Delete();
				pcEditor->Insert(cToLower.c_str(),false);
			}			
			break;
		}
			
		//when this is called, it means that something happend in the codeview
		case M_TEXT_INVOKED:
		{
			int32 nEvent;
			if (pcMessage->FindInt32("events",&nEvent)==0)
			{
				if (!bChanged && nEvent & cv::CodeView::EI_CONTENT_CHANGED)
				{
					bChanged = true;
					UpdateTitle();
				}
				if (nEvent & cv::CodeView::EI_CURSOR_MOVED)
				{
					UpdateStatus(cFile,0);
				}
				break;
			}
		}
		case M_REGISTRAR_GET_PATH:
		{
			int32 nCode;
			pcMessage->FindInt32( "reply_code", &nCode );
			int nStringLength = 0;
			os::String zNormalized( PATH_MAX, 0 );
			int nFd = open( cFile.c_str(), O_RDONLY );
			os::Message cReply( nCode );
			if( ( nStringLength = get_directory_path( nFd, (char*)zNormalized.c_str(), PATH_MAX ) ) < 0 )
			{
				pcMessage->SendReply( &cReply );
				close( nFd );
				break;
			}
			zNormalized.Resize( nStringLength );
			close( nFd );
			cReply.AddString( "file/path", zNormalized );
			pcMessage->SendReply( &cReply );
			break;
		}	
		case M_REGISTRAR_MAKE_FOCUS:
		{
			MakeFocus();
			break;
		}
	}
}

/*************************************************************
* Description: Called when application quits
* Author: Rick Caudill
* Date: Tue Mar 16 11:40:01 2004
*************************************************************/
void MainWindow::TimerTick(int nID)
{
	if (nID == 1) //if the ID matches Timer set
	{
		if (sFile != NULL && bChanged)
		{
			PostMessage(M_FILE_SAVE,this);
		}
	}
}

/*************************************************************
* Description: Called when application quits
* Author: Rick Caudill
* Date: Tue Mar 16 11:40:01 2004
*************************************************************/
bool MainWindow::OkToQuit()
{
	if (bChanged) //if the file has changed
	{
		PostMessage( M_CLOSE_FILE_SAVE, this );
		return false;
	}
	
	if (sFile != NULL) //if there is a SourceryFile
		delete sFile;	
		
	//set size and position, then save settings, then delete	
	pcMainSettings->SetSizeAndPosition(GetFrame());
	pcMainSettings->SaveSettings();
	
	os::Application::GetInstance()->PostMessage( os::M_TERMINATE );
	return( false );
}

/*************************************************************
* Description: Using this method to catch input
*			   This method will be superceded when menu shortcuts 
*			   are enabled.
* Author: Rick Caudill
* Date: Tue Mar 16 11:40:01 2004
*************************************************************/
void MainWindow::DispatchMessage(os::Message* pcMessage, os::Handler* pcHandler)
{
	String cRawString;
	int32 nQual, nRawKey;
	if (pcMessage->GetCode()==os::M_KEY_DOWN)  //a key must have been pressed
	{
		if (pcMessage->FindString("_raw_string",&cRawString)!=0 || pcMessage->FindInt32("_qualifiers", &nQual)!=0 || pcMessage->FindInt32("_raw_key", &nRawKey)!=0)
			Window::DispatchMessage(pcMessage,pcHandler);
		else
		{
	
			if (nQual &os::QUAL_CTRL)  //ctrl key was caught
			{
				if (nRawKey == 53) //CTRL - END
				{
					GoTo(0,pcEditor->GetLineCount()+1);
				}
				
				else if (nRawKey == 32) //CTRL - HOME
				{
					GoTo(0,1);
				}
					
				else if (nRawKey != 79 && nRawKey != 60 && nRawKey != 78 && nRawKey != 77 && nRawKey != 76 && nRawKey != 94)
				{	
					m_pcMenu->MakeFocus(); //simple hack in order to get menus to work 
				}
				else
				{
					pcEditor->MakeFocus();
				}
			}
			else if (nQual &os::QUAL_ALT)
			{

				if (nRawKey == 60) //ALT+A
				{
					Point cPoint = m_pcMenu->GetItemAt(0)->GetContentLocation();
					m_pcMenu->MouseDown(cPoint,1);
				}
					
				if (nRawKey == 63)//ALT+F
				{
					Point cPoint = m_pcMenu->GetItemAt(1)->GetContentLocation();
					m_pcMenu->MouseDown(cPoint,1);
				}

				if (nRawKey == 41)//ALT+E
				{
					Point cPoint = m_pcMenu->GetItemAt(2)->GetContentLocation();
					m_pcMenu->MouseDown(cPoint,1);
				}
				
				if (nRawKey == 42) //ALT+R
				{
					Point cPoint = m_pcMenu->GetItemAt(3)->GetContentLocation();
					m_pcMenu->MouseDown(cPoint,1);
				}					
				
				if (nRawKey == 79)//ALT+V
				{
					Point cPoint = m_pcMenu->GetItemAt(4)->GetContentLocation();
					m_pcMenu->MouseDown(cPoint,1);
				}

				if (nRawKey == 46)//ALT+I
				{
					Point cPoint = m_pcMenu->GetItemAt(5)->GetContentLocation();
					m_pcMenu->MouseDown(cPoint,1);
				}

				if (nRawKey == 68) //ALT+L
				{
					Point cPoint = m_pcMenu->GetItemAt(6)->GetContentLocation();
					m_pcMenu->MouseDown(cPoint,1);
				}

				if (nRawKey == 61) //ALT+S
				{
					Point cPoint = m_pcMenu->GetItemAt(7)->GetContentLocation();
					m_pcMenu->MouseDown(cPoint,1);
				}

				if (nRawKey == 65) //ALT+H
				{
					Point cPoint = m_pcMenu->GetItemAt(8)->GetContentLocation();
					m_pcMenu->MouseDown(cPoint,1);
				}		
													
				else;
			}
			else if (nRawKey == 4) // F3
			{
				PostMessage(M_FIND_AGAIN,this);
			}
			
			else if (nRawKey == 6) //F5
			{
				PostMessage(M_FIND_DOCUMENTATION,this);
			}
		}
	}	
	Window::DispatchMessage(pcMessage,pcHandler);
}

/*************************************************************
* Description: Zooms in
* Author: Rick Caudill
* Date: Tue Mar 16 11:40:01 2004
*************************************************************/
void MainWindow::ZoomIn()
{
	pcEditor->MakeFocus();
	float vSize = pcEditor->GetEditorFont()->GetSize();  //not sure if this is working correctly

	if (vSize < 28.0f) //file can be zoomed in	
	{
		Font* pcFont = pcEditor->GetEditorFont();
		pcFont->SetSize(pcFont->GetSize()+1.0f);
		pcEditor->FontChanged(pcFont);
		pcEditor->Flush();
		m_pcViewMenu->FindItem("Zoom In")->SetEnable(true);
		m_pcViewMenu->FindItem("Zoom Out")->SetEnable(true);
	}
	else //cannot zoom in any further
	{
		UpdateStatus("Error: You cannot zoom in anymore",0);
		m_pcViewMenu->FindItem("Zoom In")->SetEnable(false);
	}	
}

/*************************************************************
* Description: Zooms out of the editor
* Author: Rick Caudill
* Date: Tue Mar 16 11:40:01 2004
*************************************************************/
void MainWindow::ZoomOut()
{
	pcEditor->MakeFocus();			
	float vSize = pcEditor->GetEditorFont()->GetSize();		
	if (vSize > 5.0f) //file can be zoomed out
	{
		Font* pcFont = pcEditor->GetEditorFont();
		pcFont->SetSize((pcFont->GetSize()-1.0f));
		pcEditor->FontChanged(pcFont);
		pcEditor->Flush();
		m_pcViewMenu->FindItem("Zoom Out")->SetEnable(true);
		m_pcViewMenu->FindItem("Zoom In")->SetEnable(true);
	}
	
	else  //cannot zoom out any further
	{	
		UpdateStatus("Error: You cannot zoom out anymore",0);
		m_pcViewMenu->FindItem("Zoom Out")->SetEnable(false);
	}
}

/*************************************************************
* Description: Sets the cursor to a specific position in the CodeView
* Author: Rick Caudill
* Date: Tue Mar 16 11:40:01 2004
*************************************************************/
void MainWindow::GoTo(uint nWidth, uint nHeight)
{
	pcEditor->SetCursor(IPoint(nWidth,nHeight-1),false,true);
}

/*************************************************************
* Description: Loads the file
* Author: Rick Caudill
* Date: Tue Mar 16 11:40:01 2004
*************************************************************/
void MainWindow::Load(const os::String& cString, bool bOpenedByMain=false)
{
	if (sFile != NULL) //lets delete the sourceryfile if there is one
		delete sFile;
		
	sFile = new SourceryFile(cString);
	
	if (sFile->IsValid())  //make sure that the file is valid
	{
		if (pcMainSettings->GetBackup())  //if backup is on then save backup
			SetBackupFile(cString,pcMainSettings->GetBackupExtension());
			
		if (pcMainSettings->GetMonitorFile()) //if monitoring is on then monitor the file
		{
			if (pcMonitorFile == NULL)
			{
				pcMonitorFile = new NodeMonitor(cString,NWATCH_ALL,this);
			}
			
			else
			{
				pcMonitorFile->SetTo(cString,NWATCH_ALL,this);
			}
		}
		
		//now lets clear the editor of all things	
		pcEditor->SetEventMask(0);
		pcEditor->Set("");
		pcEditor->Clear("");
		
		//determine whether or not the user wants to convert to lf and then load the editor
		String cLines = pcMainSettings->GetConvertToLf() ? ConvertCrLfToLf(sFile->GetLines()) : sFile->GetLines(); 
		pcEditor->Set(cLines.c_str());
		
		if (pcMainSettings->GetCursorPosition())  //if cursor position is on then set the position
			GoTo(sFile->GetCursorWidth(),sFile->GetCursorHeight()+1);
		
		//now make sure the editor knows there is a file and that it is new and unchanged
		bNamed = true;
		bChanged = false;
		cFile = sFile->GetFileName();
		
		//set the title	
		SetTitle((String)APP_NAME + (String)" v" + (String)APP_VERSION + (String)": " + cFile);	
		
		//load the appropriate format
		LoadFormat();
		
		//if (pcMainSettings->GetFoldCode())
		//	pcEditor->FoldAll();
		
		//set the event mask and focus the editor
		pcEditor->SetEventMask(os::TextView::EI_CONTENT_CHANGED | os::TextView::EI_CURSOR_MOVED);
		pcEditor->MakeFocus();
		
		//set the item into the recent list and refresh
		SetRecentList(sFile->GetFileName());
		ReloadRecentMenu();
		
		//set "Open Corresponding File" MenuItem to enabled or disabled
		if (sFile->GetFilesExtension() == ".cpp" || sFile->GetFilesExtension() == ".h")
			m_pcFileMenu->GetItemAt(4)->SetEnable(true);
		else
			m_pcFileMenu->GetItemAt(4)->SetEnable(false);	
	}
	// Send event
	int nStringLength = 0;
	os::String zNormalized( PATH_MAX, 0 );
	os::Message cReply;
	int nFd = open( cFile.c_str(), O_RDONLY );
	if( ( nStringLength = get_directory_path( nFd, (char*)zNormalized.c_str(), PATH_MAX ) ) < 0 )
	{
		close( nFd );
		return;
	}
	zNormalized.Resize( nStringLength );
	close( nFd );
	cReply.AddString( "file/path", zNormalized );
	pcGetFileEvent->PostEvent( &cReply );
}

/*************************************************************
* Description: Loads a new format based on the menuitem clicked
* Author: Rick Caudill
* Date: Thu Aug  5 15:05:46 2004
*************************************************************/
void MainWindow::LoadNewFormat(const String& cItemName)
{
	tList cList = GetFormats();
	for (uint nIncrement=0; nIncrement<cList.size(); nIncrement++)
	{
		if (cItemName == cList[nIncrement].cName)
		{
			pcEditor->SetFormat(cList[nIncrement].pcFormat);
			pcEditor->SetTabSize(cList[nIncrement].nTabSize);
			pcEditor->SetUseTab(cList[nIncrement].bUseTab);
			pcEditor->SetTextColor(cList[nIncrement].sFgColor);
			pcEditor->SetBackground(cList[nIncrement].sBgColor);
			UpdateStatus(String("Using format: ") + cList[nIncrement].cName,0);			
			break;
		}
	}
}

/*************************************************************
* Description: Saves the file(uses FSNode for attributes/saving)
* Author: Rick Caudill
* Date: Tue Mar 16 11:40:01 2004
*************************************************************/
void MainWindow::Save(const String& cFile)
{
	try
	{
		if (pcMonitorFile) pcMonitorFile->Unset();
		File* pcFile = new File(cFile.c_str(),O_CREAT | O_WRONLY | O_TRUNC);
		String cEditorString;
		uint nLines=pcEditor->GetLineCount();

		for (uint i=0;i<=nLines;i++)
		{
			cEditorString += pcEditor->GetLine(i); 
			cEditorString += "\n";
		}
		pcFile->Write(cEditorString.c_str(),cEditorString.size());
		pcFile->Flush();

		IPoint cPoint = pcEditor->GetCursor();
		pcFile->WriteAttr( "Sourcery::CursorWidth", O_TRUNC, ATTR_TYPE_INT32, &cPoint.x,0,sizeof(int32));
	   	pcFile->WriteAttr( "Sourcery::CursorHeight", O_TRUNC, ATTR_TYPE_INT32, &cPoint.y,0,sizeof(int32));
		delete pcFile;
	
		bChanged = false;
		UpdateTitle();
		LoadFormat();
		pcEditor->MakeFocus();
		if (sFile)  //reinitalize if there is a file
			sFile->ReInitialize();
		else //else create a new file handle
			sFile = new SourceryFile(cFile);
		if (pcMonitorFile)
			pcMonitorFile->SetTo(cFile,NWATCH_ALL,this);
		else if (pcMainSettings->GetMonitorFile())
			pcMonitorFile = new NodeMonitor(cFile,NWATCH_ALL,this);
		return;
	}

	catch(...)  //the file could not be saved???
	{
		UpdateStatus("Error: Could not save file",0);
		return;
	}
}

/*************************************************************
* Description: Closes the file
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 10:12:50 
*************************************************************/
void MainWindow::CloseFile()
{
	if (sFile)
	{
		
		delete sFile;
		sFile = NULL;
	}
	
	if (pcMonitorFile)
		delete pcMonitorFile;
	
	if (cFile.c_str() != "")
		cFile = "";
	
	UpdateStatus("",0);		
	pcEditor->SetEventMask(0);
	pcEditor->Clear("");
	pcEditor->Set("");
	SetTitle("Sourcery v0.1:");
	pcEditor->SetEventMask(os::TextView::EI_CONTENT_CHANGED | os::TextView::EI_CURSOR_MOVED);
}

/*************************************************************
* Description: Updates the title.  If the file has been changed, it adds an '*' to the title
* Author: Rick Caudill
* Date: Tue Mar 16 11:40:01 2004
*************************************************************/
void MainWindow::UpdateTitle()
{
	String cTitle = (String)APP_NAME + (String)" v" + (String)APP_VERSION;
	if (cFile == "") //no file, so lets add "Untitled" to it
		cTitle += ": Untitled";
	else //there is a file
		cTitle += String(": ") + cFile;

	if(bChanged) //file is changed, so lets add an indicator that it has been changed.
        cTitle+="*";
	SetTitle(cTitle);
}

/*************************************************************
* Description: Updates the statusbar
* Author: Rick Caudill
* Date: Tue Mar 16 11:40:01 2004
*************************************************************/
void MainWindow::UpdateStatus(const String& cString, bigtime_t nTime)
{
	char zTmp[100];
	os::IPoint cPoint=pcEditor->GetCursor();
	
	sprintf(zTmp,"Line: %i",cPoint.y+1);
	m_pcStatusBar->SetText(zTmp,0);
	
	sprintf(zTmp,"Col: %i",cPoint.x);
	m_pcStatusBar->SetText(zTmp,1);
	
	m_pcStatusBar->SetText(cString,2,nTime);
}

/*************************************************************
* Description: Loads the format based on the files extension.
			   This was taken from CodeEdit and will be replaced
			   once mimetypes are available.
* Author: Rick Caudill
* Date: Tue Mar 16 11:40:01 2004
*************************************************************/
void MainWindow::LoadFormat()
{
	const tList& cList = GetFormats();  //gets the formats
	std::string cTitle = GetTitle(); //gets the current title???
	uint nIncrement;  //increment variable

	if ( cList.size() == 0) //if the list is empty then we cannot load a format
		return;
	
	if (sFile)  //there is a sourcery file so lets try and load the format
	{
		for (nIncrement=1; nIncrement<cList.size();++nIncrement) //go through the list 
		{
			int nFindOne = 0; 
			int nFindTwo = cList[nIncrement].cFilter.find(';',0);

			while (nFindOne < nFindTwo)  //found a semicolon
			{
				std::string cFilter = cList[nIncrement].cFilter.substr(nFindOne,nFindTwo-nFindOne);
				if (cTitle.size() > cFilter.size())  //if the title is bigger than the filter size, then go on
				{
					std::string cStripped = sFile->GetFilesExtension();  //get the files extension
					if (cFilter == cStripped) //if the files extension is same as the filter
					{
						cConvertor = cFilter;
						pcEditor->SetFormat(cList[nIncrement].pcFormat);
						pcEditor->SetTabSize(cList[nIncrement].nTabSize);
						pcEditor->SetUseTab(cList[nIncrement].bUseTab);
						pcEditor->SetTextColor(cList[nIncrement].sFgColor);
						pcEditor->SetBackground(cList[nIncrement].sBgColor);
						UpdateStatus(String("Using format: ") + cList[nIncrement].cName,0);
						return;	
					} 
				}	 
				nFindOne=nFindTwo+1;
				nFindTwo = cList[nIncrement].cFilter.find(';',nFindOne);	
			}
		}
	}
   //no extension match, so lets load the defaults
   pcEditor->SetFormat(cList[0].pcFormat);
   pcEditor->SetTabSize(cList[0].nTabSize);
   pcEditor->SetUseTab(cList[0].bUseTab);
   pcEditor->SetTextColor(cList[0].sFgColor);
   pcEditor->SetBackground(cList[0].sBgColor);
   UpdateStatus("Using format: Default",0);
}

/*************************************************************
* Description: Launches the convertor if it is installed and the file is a file that can be converted.
* Author: Rick Caudill
* Date: Tue Mar 16 11:40:01 2004
*************************************************************/
void MainWindow::LaunchConvertor()
{
	String cPath = "^/bin/source-highlight";
	
	try
	{
		os::File* pFile = new File(cPath);
		
		if (cFile == "" || cConvertor == "") //there must not be an file selected
		{
			ShowError(ERR_NO_FILE_SELECTED);
			return;
		}
		//there is a file selected now lets see if it matches what can be converted
		else if (pFile && (cConvertor == ".cpp" || cConvertor== ".h" || cConvertor == ".c" || cConvertor == ".C" ||  cConvertor == ".pm" || cConvertor == ".perl" || cConvertor == ".java" ))
		{
			pcConvertDialog = new ConvertDialog(cConvertor,cFile);
			pcConvertDialog->CenterInWindow(this);
			pcConvertDialog->Show();
			pcConvertDialog->MakeFocus();
		}
		
		else  //the file type was not a type that can be converted, so flag an error
		{
			ShowError(ERR_CONVERTOR_WRONG_FILE_TYPE);
		}	
		delete pFile;  //don't forget to delete the file
	}

	catch(...) //source-highlight must not be in ^/bin
	{
		ShowError(ERR_NO_HIGHLIGHTER);
	}
}

/*************************************************************
* Description: Opens the files corresponding file(only works with .h and .cpp)
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 10:13:13 
*************************************************************/
void MainWindow::OpenCorrespondingFile()
{
	String cNewFile = sFile->GetFilesExtension() == ".cpp" ? RemoveExtension(sFile->GetFileName()) + ".h" : RemoveExtension(sFile->GetFileName()) + ".cpp";
	File* pFile = new File();
	
	if (pFile->SetTo(cNewFile) == 0)
	{
		String cApplication = String(GetApplicationPath());
		Execute* pcExecute = new Execute(cApplication,cNewFile);				
		if (pcExecute->IsValid())
			pcExecute->Run();
		delete pcExecute;
	}
	
	else
		ShowError(ERR_CORR_FILE);
	delete pFile;
}

/*************************************************************
* Description: Posts an error message on the status bar
* Author: Rick Caudill
* Date: Tue Mar 16 11:40:01 2004
*************************************************************/
void MainWindow::ShowError(int nCode)
{
	switch (nCode)
	{
		case ERR_NO_HIGHLIGHTER:
			UpdateStatus("Error: The source highlighter application is not installed",0);
			break;
		
		case ERR_NO_FILE_SELECTED:
			UpdateStatus("Error: There is not any file selected",0);
			break;

		case ERR_UNFORSEEN_ERROR:
			UpdateStatus("Error: There is an unforseen error",0);
			break;
		
		case ERR_CONVERTOR_WRONG_FILE_TYPE:
			UpdateStatus("Error: This file type cannot be converted",0);
			break;
			
		case ERR_NOT_DIRECTORY:
			UpdateStatus("Error: This is not a directory",0);
			break;
			
		case ERR_ONLY_WORKS_WITH_CPP:
			UpdateStatus("Error: This function only works with c++ files/headers",0);	
			break;
			
		case ERR_NO_FILE_BROWSER:
			UpdateStatus("Error: The file browser is not installed",0);
			break;
		
		case ERR_CORR_FILE:
			UpdateStatus("Error: The corresponding file is not valid",0);
			break;
				
		default:
			ShowError(ERR_UNFORSEEN_ERROR);
			break;
	}
}

/*************************************************************
* Description: Finds a specfic string based on all parameters 
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 10:14:10 
*************************************************************/
bool MainWindow::Find(const String &pcString, bool bDown, bool bCaseSensitive, bool bExtended)
{
    if (pcString.empty())
    {
        UpdateStatus("Search string empty!",0);
        return false;
    }

    os::RegExp sRegularExpression;
    status_t nStatus=sRegularExpression.Compile(pcString, !bCaseSensitive, bExtended);
    
	switch(nStatus)
    {
    case os::RegExp::ERR_BADREPEAT:
        UpdateStatus("Error in regular expression: Bad repeat!",0);
        return false;
    case os::RegExp::ERR_BADBACKREF:
        UpdateStatus("Error in regular expression: Bad back reference!",0);
        return false;
    case os::RegExp::ERR_BADBRACE:
        UpdateStatus("Error in regular expression: Bad brace!",0);
        return false;
    case os::RegExp::ERR_BADBRACK:
        UpdateStatus("Error in regular expression: Bad bracket!",0);
        return false;
    case os::RegExp::ERR_BADPARENTHESIS:
        UpdateStatus("Error in regular expression: Bad parenthesis!",0);
        return false;
    case os::RegExp::ERR_BADRANGE:
        UpdateStatus("Error in regular expression: Bad range!",0);
        return false;
    case os::RegExp::ERR_BADSUBREG:
        UpdateStatus("Error in regular expression: Bad number in \\digit construct!",0);
        return false;
    case os::RegExp::ERR_BADCHARCLASS:
        UpdateStatus("Error in regular expression: Bad character class name!",0);
        return false;
    case os::RegExp::ERR_BADESCAPE:
        UpdateStatus("Error in regular expression: Bad escape!",0);
        return false;
    case os::RegExp::ERR_BADPATTERN:
        UpdateStatus("Error in regular expression: Syntax error!",0);
        return false;
    case os::RegExp::ERR_TOLARGE:
        UpdateStatus("Error in regular expression: Expression to large!",0);
        return false;
    case os::RegExp::ERR_NOMEM:
        UpdateStatus("Error in regular expression: Out of memory!",0);
        return false;
    case os::RegExp::ERR_GENERIC:
        UpdateStatus("Error in regular expression: Unknown error!",0);
        return false;
    }

    os::IPoint start=pcEditor->GetCursor();

    if(bDown)
    {
        //test current line
        if(sRegularExpression.Search(pcEditor->GetLine(start.y), start.x))
        {//found!
            pcEditor->SetCursor(start.x+sRegularExpression.GetEnd(), start.y, false, false);
            pcEditor->Select(os::IPoint(start.x+sRegularExpression.GetStart(), start.y), os::IPoint(start.x+sRegularExpression.GetEnd(), start.y));
            Flush();
            return true;
        }

        //test to end of file
        for(uint a=start.y+1;a<pcEditor->GetLineCount();++a)
        {
            if(sRegularExpression.Search(pcEditor->GetLine(a)))
            {//found!
                pcEditor->SetCursor(sRegularExpression.GetEnd(), a, false, false);
                pcEditor->Select(os::IPoint(sRegularExpression.GetStart(), a), os::IPoint(sRegularExpression.GetEnd(), a));
                Flush();
                return true;
            }
        }

        //test from start of file
        for(int a=0;a<=start.y;++a)
        {
            if(sRegularExpression.Search(pcEditor->GetLine(a)))
            {//found!
                pcEditor->SetCursor(sRegularExpression.GetEnd(), a, false, false);
                pcEditor->Select(os::IPoint(sRegularExpression.GetStart(), a), os::IPoint(sRegularExpression.GetEnd(), a));
                Flush();
                return true;
            }
        }

        //not found
        UpdateStatus("Search string not found!",0);
		return false;
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
            pcEditor->Select(os::IPoint(begin, start.y), os::IPoint(end, start.y));
            Flush();
            return true;
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
                pcEditor->Select(os::IPoint(begin, a), os::IPoint(end, a));
                Flush();
                return true;
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
                pcEditor->Select(os::IPoint(begin, a), os::IPoint(end, a));
                Flush();
                return true;
            }
        }
	//not found
	UpdateStatus("Search string not found!",0);
	return false;
	}
}

