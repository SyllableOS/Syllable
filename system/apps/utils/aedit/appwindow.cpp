//  AEdit -:-  (C)opyright 2000-2002 Kristian Van Der Vliet
//			   (C)opyright 2004 Jonas Jarvoll	
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

#include "appwindow.h"
#include "buffer.h"
#include "editview.h"
#include "status_bar.h"
#include "messages.h"
#include "version.h"
#include "mytabview.h"
#include "icons.h"

#include "resources/aedit.h"

#include <fstream.h>

#include <gui/window.h>
#include <gui/requesters.h>
#include <gui/font.h>
#include <gui/checkmenu.h>
#include <gui/textview.h>
#include <gui/image.h>

#include <util/invoker.h>
#include <util/application.h>
#include <util/string.h>
#include <util/resources.h>

using namespace os;

AEditWindow::AEditWindow(const Rect& cFrame) : Window(cFrame, "main_window", AEDIT_RELEASE_STRING)
{	
	// Load settings
	cSettings.Load();
	
	// Set up latest path used
	LatestOpenPath=cSettings.GetString("LoadPath", "");
	LatestSavePath=cSettings.GetString("SavePath", "");

	// Set window position
	Rect cWinSize=cSettings.GetRect("WindowPos", cFrame);
	ResizeTo(cWinSize.Width(),cWinSize.Height());
	MoveTo(cWinSize.left,cWinSize.top);

	// Make all menus
	SetupMenus();

	// Create & attach the button bar
	Rect cBounds=GetBounds();
	cBounds.top =pcMenuBar->GetBounds().bottom +1.0f;
	cBounds.bottom = BUTTON_HEIGHT + 4 + cBounds.top;

	pcButtonBar=new ButtonBar(Rect(0,cBounds.top,GetBounds().Width(),cBounds.bottom),"button_bar");
	AddChild(pcButtonBar);

	// Attach the buttons to the button bar
	pcImageFileNew=pcButtonBar->AddButton(GetStockIcon(STOCK_NEW, STOCK_SIZE_TOOLBAR), "appwindow_filenew",new Message(M_BUT_FILE_NEW));
	pcImageFileOpen=pcButtonBar->AddButton(GetStockIcon(STOCK_OPEN, STOCK_SIZE_TOOLBAR),"appwindow_fileopen",new Message(M_BUT_FILE_OPEN));
	pcImageFileSave=pcButtonBar->AddButton(GetStockIcon(STOCK_SAVE, STOCK_SIZE_TOOLBAR),"appwindow_filesave",new Message(M_BUT_FILE_SAVE));
	pcImageEditCut=pcButtonBar->AddButton(GetStockIcon(STOCK_CUT, STOCK_SIZE_TOOLBAR),"appwindow_editcut",new Message(M_BUT_EDIT_CUT));
	pcImageEditCopy=pcButtonBar->AddButton(GetStockIcon(STOCK_COPY, STOCK_SIZE_TOOLBAR),"appwindow_editcopy",new Message(M_BUT_EDIT_COPY));
	pcImageEditPaste=pcButtonBar->AddButton(GetStockIcon(STOCK_PASTE, STOCK_SIZE_TOOLBAR),"appwindow_editpaste",new Message(M_BUT_EDIT_PASTE));
	pcImageFindFind=pcButtonBar->AddButton(GetStockIcon(STOCK_SEARCH, STOCK_SIZE_TOOLBAR),"appwindow_find",new Message(M_BUT_FIND_FIND));
#ifdef ENABLE_UNDO
	pcImageEditUndo=pcButtonBar->AddButton(GetStockIcon(STOCK_REDO, STOCK_SIZE_TOOLBAR),"appwindow_redo",new Message(M_BUT_EDIT_UNDO));
	pcImageEditRedo=pcButtonBar->AddButton(GetStockIcon(STOCK_UNDO, STOCK_SIZE_TOOLBAR),"appwindow_undo",new Message(M_BUT_EDIT_REDO));
#endif

	// Create & attach TabView
	cBounds=GetBounds();
	cBounds.top=pcButtonBar->GetBounds().bottom + pcMenuBar->GetBounds().bottom + 1.0f;
	cBounds.bottom-=24;
	pcMyTabView=new MyTabView(cBounds, this);
	pcMyTabView->SetTarget(this);
	pcMyTabView->SetMessage(new Message(M_INVOKED_TAB_CHANGED));
	AddChild(pcMyTabView);
	
	// Create the status bar
	cBounds=GetBounds();
	cBounds.top=pcButtonBar->GetBounds().bottom + pcMenuBar->GetBounds().bottom + pcMyTabView->GetBounds().bottom + 1.0f;
	pcStatusBar=new StatusBar(cBounds, "appwindow_status_bar");
	AddChild(pcStatusBar);

	// Set the window title etc.
	SetTitle(AEDIT_RELEASE_STRING);
	SetSizeLimits(Point(300,250),Point(4096,4096));

	// Set pointers to a well defined value
	pcCurrentBuffer=NULL;
	pcFindDialog=NULL;
	pcReplaceDialog=NULL;
	pcGotoDialog=NULL;
	pcAboutDialog=NULL;

	// Set welcome message
	SetStatus(MSG_STATUSBAR_WELCOME);

	// Update menu items
	UpdateMenuItemStatus();

	// Init file requester pointers
	pcLoadRequester = NULL;
	pcSaveRequester = NULL;
	pcSaveAsRequester = NULL;

	// Set an application icon for Dock
	os::BitmapImage *pcImage = new os::BitmapImage();
	os::Resources cRes( get_image_id() );
	pcImage->Load( cRes.GetResourceStream( "icon24x24.png" ) );
	SetIcon(pcImage->LockBitmap());

}

void AEditWindow::SetupMenus()
{
	Rect cMenuFrame = GetBounds();
    Rect cMainFrame = cMenuFrame;
    cMenuFrame.bottom = 18;
	pcMenuBar=new Menu(cMenuFrame, "main_menu", ITEMS_IN_ROW, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP);

	// Create the menus
	pcAppMenu=new Menu(Rect(0,0,1,1),MSG_MENU_APPLICATION,ITEMS_IN_COLUMN);
	pcAppMenu->AddItem(MSG_MENU_APPLICATION_QUIT,new Message(M_MENU_APP_QUIT), "Ctrl+Q", GetStockIcon(STOCK_EXIT, STOCK_SIZE_MENUITEM));
	pcAppMenu->AddItem(new MenuSeparator());
	pcAppMenu->AddItem(MSG_MENU_APPLICATION_ABOUT,new Message(M_MENU_APP_ABOUT));

	pcFileMenu=new Menu(Rect(0,0,1,1),MSG_MENU_FILE,ITEMS_IN_COLUMN);
	pcFileNew=new MenuItem(MSG_MENU_FILE_NEW,new Message(M_MENU_FILE_NEW), "Ctrl+N", GetStockIcon(STOCK_NEW, STOCK_SIZE_MENUITEM));
	pcFileMenu->AddItem(pcFileNew);
	pcFileMenu->AddItem(new MenuSeparator());
	pcFileOpen=new MenuItem(MSG_MENU_FILE_OPEN,new Message(M_MENU_FILE_OPEN), "Ctrl+O", GetStockIcon(STOCK_OPEN, STOCK_SIZE_MENUITEM));
	pcFileMenu->AddItem(pcFileOpen);
	pcFileMenu->AddItem(new MenuSeparator());
	pcFileClose=new MenuItem(MSG_MENU_FILE_CLOSE,new Message(M_MENU_FILE_CLOSE), "Ctrl+W");
	pcFileMenu->AddItem(pcFileClose);
	pcFileSave=new MenuItem(MSG_MENU_FILE_SAVE,new Message(M_MENU_FILE_SAVE), "Ctrl+S", GetStockIcon(STOCK_SAVE, STOCK_SIZE_MENUITEM));
	pcFileMenu->AddItem(pcFileSave);
	pcFileSaveAs=new MenuItem(MSG_MENU_FILE_SAVE_AS,new Message(M_MENU_FILE_SAVE_AS), "", GetStockIcon(STOCK_SAVE_AS, STOCK_SIZE_MENUITEM));
	pcFileMenu->AddItem(pcFileSaveAs);
	pcFileSaveAll=new MenuItem(MSG_MENU_FILE_SAVE_ALL,new Message(M_MENU_FILE_SAVE_ALL), "Ctrl+L");
	pcFileMenu->AddItem(pcFileSaveAll);
	pcFileMenu->AddItem(new MenuSeparator());
	pcFileNextTab=new MenuItem(MSG_MENU_FILE_NEXT_FILE,new Message(M_MENU_FILE_NEXT_TAB), "Ctrl+.", GetStockIcon(STOCK_RIGHT_ARROW, STOCK_SIZE_MENUITEM));
	pcFileMenu->AddItem(pcFileNextTab);
	pcFilePrevTab=new MenuItem(MSG_MENU_FILE_PREVIOUS_FILE,new Message(M_MENU_FILE_PREV_TAB), "Ctrl+,", GetStockIcon(STOCK_LEFT_ARROW, STOCK_SIZE_MENUITEM));
	pcFileMenu->AddItem(pcFilePrevTab);

	pcEditMenu=new Menu(Rect(0,0,1,1),MSG_MENU_EDIT,ITEMS_IN_COLUMN);
	pcEditCut=new MenuItem(MSG_MENU_EDIT_CUT,new Message(M_MENU_EDIT_CUT), "Ctrl+X", GetStockIcon(STOCK_CUT, STOCK_SIZE_MENUITEM));
	pcEditMenu->AddItem(pcEditCut);
	pcEditCopy=new MenuItem(MSG_MENU_EDIT_COPY,new Message(M_MENU_EDIT_COPY), "Ctrl+C", GetStockIcon(STOCK_COPY, STOCK_SIZE_MENUITEM));
	pcEditMenu->AddItem(pcEditCopy);
	pcEditPaste=new MenuItem(MSG_MENU_EDIT_PASTE,new Message(M_MENU_EDIT_PASTE), "Ctrl+V", GetStockIcon(STOCK_PASTE, STOCK_SIZE_MENUITEM));
	pcEditMenu->AddItem(pcEditPaste);
	pcEditMenu->AddItem(new MenuSeparator());
	pcEditSelectAll=new MenuItem(MSG_MENU_EDIT_SELECT_ALL,new Message(M_MENU_EDIT_SELECT_ALL), "Ctrl+A");
	pcEditMenu->AddItem(pcEditSelectAll);
#ifdef ENABLE_UNDO
	pcEditMenu->AddItem(new MenuSeparator());
	pcEditUndo=new MenuItem(MSG_MENU_EDIT_UNDO,new Message(M_MENU_EDIT_UNDO), "Ctrl+Z", GetStockIcon(STOCK_UNDO, STOCK_SIZE_MENUITEM));
	pcEditMenu->AddItem(pcEditUndo);
	pcEditRedo=new MenuItem(MSG_MENU_EDIT_REDO,new Message(M_MENU_EDIT_REDO), "", GetStockIcon(STOCK_REDO, STOCK_SIZE_MENUITEM));
	pcEditMenu->AddItem(pcEditRedo);
#endif

	pcFindMenu=new Menu(Rect(0,0,1,1),MSG_MENU_FIND,ITEMS_IN_COLUMN);
	pcFindMenu->AddItem(MSG_MENU_FIND_FIND,new Message(M_MENU_FIND_FIND), "Ctrl+F", GetStockIcon(STOCK_SEARCH, STOCK_SIZE_MENUITEM));
	pcFindMenu->AddItem(MSG_MENU_FIND_REPLACE,new Message(M_MENU_FIND_REPLACE), "Ctrl+R", GetStockIcon(STOCK_SEARCH_REPLACE, STOCK_SIZE_MENUITEM));
	pcFindMenu->AddItem(new MenuSeparator());
	pcFindMenu->AddItem(MSG_MENU_FIND_GOTO_LINE,new Message(M_MENU_FIND_GOTO), "Ctrl+G", GetStockIcon(STOCK_JUMP_TO, STOCK_SIZE_MENUITEM));

	// Create Font Menu
	pcFontMenu = new Menu(Rect(0,0,1,1), MSG_MENU_FONT, ITEMS_IN_COLUMN);

	// Add Size Submenu
	Menu* pcSizeMenu = new Menu(Rect(0,0,1,1), MSG_MENU_FONT_SIZE, ITEMS_IN_COLUMN);

	float sz,del=1.0;

	Message *pcSizeMsg;
	char zSizeLabel[8];

	for (sz=5.0; sz < 30.0; sz += del)
	{
		pcSizeMsg = new Message(M_MENU_FONT_SIZE);

		pcSizeMsg->AddFloat("size",sz);
		sprintf(zSizeLabel,"%.1f",sz);
		pcSizeMenu->AddItem(zSizeLabel,pcSizeMsg);

		if (sz == 10.0)
			del = 2.0;
		else if (sz == 16.0)
			del = 4.0;
   }

	pcFontMenu->AddItem(pcSizeMenu);
	pcFontMenu->AddItem(new MenuSeparator());

	// Add Family and Style Menu
	int fc = Font::GetFamilyCount();
	int sc,i=0,j=0;
	char zFFamily[FONT_FAMILY_LENGTH],zFStyle[FONT_STYLE_LENGTH];
	uint32 flags;

	Menu *pcFamilyMenu;
	Message *pcFontMsg;

	for (i=0; i<fc; i++)
	{
		if (Font::GetFamilyInfo(i,zFFamily) == 0)
		{
			sc = Font::GetStyleCount(zFFamily);
			pcFamilyMenu = new Menu(Rect(0,0,1,1),zFFamily,ITEMS_IN_COLUMN);

			for (j=0; j<sc; j++)
			{
				if (Font::GetStyleInfo(zFFamily,j,zFStyle,&flags) == 0)
				{
					pcFontMsg = new Message(M_MENU_FONT_FAMILY_AND_STYLE);
					pcFontMsg->AddString("family",zFFamily);
					pcFontMsg->AddString("style",zFStyle);
					pcFontMsg->AddInt32("flags", flags);

					pcFamilyMenu->AddItem(zFStyle,pcFontMsg);
				}
			}

		pcFontMenu->AddItem(pcFamilyMenu);

		}
	}

	// Create the Settings menu
	pcSettingsMenu=new Menu(Rect(0,0,1,1),MSG_MENU_SETTINGS,ITEMS_IN_COLUMN);
	pcSettingsSaveOnClose=new CheckMenu(MSG_MENU_SETTINGS_SAVE_ON_CLOSE,new Message(M_MENU_SETTINGS_SAVE_ON_CLOSE), 
							cSettings.GetBool("SaveOnExit", true));
	pcSettingsMenu->AddItem(pcSettingsSaveOnClose);
	pcSettingsMenu->AddItem(new MenuSeparator());
	pcSettingsMenu->AddItem(MSG_MENU_SETTINGS_SAVE_NOW, new Message(M_MENU_SETTINGS_SAVE_NOW));
	pcSettingsMenu->AddItem(MSG_MENU_SETTINGS_RESET, new Message(M_MENU_SETTINGS_RESET));

	// Attach the menus.  Attach the menu bar to the window
	pcMenuBar->AddItem(pcAppMenu);
	pcMenuBar->AddItem(pcFileMenu);
	pcMenuBar->AddItem(pcEditMenu);
	pcMenuBar->AddItem(pcFindMenu);
	pcMenuBar->AddItem(pcFontMenu);
	pcMenuBar->AddItem(pcSettingsMenu);
	
	cMenuFrame.bottom = pcMenuBar->GetPreferredSize( true ).y;
    cMainFrame.top = cMenuFrame.bottom + 1;
    pcMenuBar->SetFrame( cMenuFrame );
	pcMenuBar->SetTargetForItems(this);
	AddChild(pcMenuBar);
}

void AEditWindow::HandleMessage(Message* pcMessage)
{
	switch(pcMessage->GetCode())	//Get the message code from the message
	{
		case M_MENU_APP_QUIT:
		{
			OkToQuit();
			break;
		}

		case M_MENU_APP_ABOUT:
		{
			// Create dialog if it doesnt exists
			if(pcAboutDialog==NULL)
				pcAboutDialog=new AboutDialog(Rect(200,200,450,380), this);

			UpdateMenuItemStatus();

			pcAboutDialog->CenterInWindow(this);
			pcAboutDialog->Raise();

			break;
		}

		case M_MENU_FILE_NEW:
		case M_BUT_FILE_NEW:
		{		
			CreateNewBuffer();
			
			break;
		}

		case M_MENU_FILE_OPEN:
		case M_BUT_FILE_OPEN:
		{
			// Create fileselector
			pcLoadRequester = new FileRequester(FileRequester::LOAD_REQ, new Messenger(this), LatestOpenPath);
			pcLoadRequester->Lock();
			pcLoadRequester->Start();
			pcLoadRequester->CenterInWindow(this);	
			if(pcLoadRequester->IsVisible())
				pcLoadRequester->Show(false);
			pcLoadRequester->Show(true);
			pcLoadRequester->MakeFocus();
			pcLoadRequester->Unlock();
			break;
		}

		case M_LOAD_REQUESTED:
		{
			const char* pzFPath;

			FileRequester *pcRequester;
			if( pcMessage->FindPointer( "source", (void**)&pcRequester ) == EOK )
				pcRequester->Close();

			if(pcMessage->FindString("file/path",&pzFPath) == 0)
			{
				int n=0;

				// For each file selected in the filerequester, open it
				while( pcMessage->FindString("file/path",&pzFPath, n) == 0)
				{
					Load((char*)pzFPath);
					n++;
				}

				// Remember path
				LatestOpenPath=String(pzFPath);
			}		

			// Update menu items
			UpdateMenuItemStatus();

			break;
		}

		case M_MENU_FILE_CLOSE:
		{
			if(pcFileClose->IsEnabled())
			{
				// check if closing buffer needs to be saved first
				if(pcCurrentBuffer->NeedToSave())
				{
					pcCloseAlert=new Alert(MSG_ALERT_CLOSE_FILE_TITLE, MSG_ALERT_CLOSE_FILE_BODY, Alert::ALERT_WARNING, MSG_ALERT_CLOSE_FILE_YES.c_str(), MSG_ALERT_CLOSE_FILE_NO.c_str(), MSG_ALERT_CLOSE_FILE_CANCEL.c_str(), NULL);
					pcCloseAlert->CenterInWindow(this);
					pcCloseAlert->Go(new Invoker(new Message(M_INVOKED_CLOSE_ALERT), this));
				}
				else // If the buffer doesnt need to be save just delete the buffer
				{
					char buffer[1024];
					sprintf(buffer, MSG_STATUSBAR_CLOSED_FILE.c_str(), pcCurrentBuffer->GetRealName().c_str());
					SetStatus(string(buffer));	
					delete pcCurrentBuffer;
			   	}

				// Update menu items
				UpdateMenuItemStatus();

			}
			break;
		}

		case M_MENU_FILE_SAVE:
		case M_BUT_FILE_SAVE:
		{
			if(pcFileSave->IsEnabled())
			{
				if(pcCurrentBuffer->Save())
				{
					char buffer[1024];
					sprintf(buffer, MSG_STATUSBAR_SAVED_FILE.c_str(), pcCurrentBuffer->GetRealName().c_str());
					pcStatusBar->SetStatus(buffer);
				}
				else
				{
					char buffer[1024];
					sprintf(buffer, MSG_ALERT_SAVE_FILE_BODY.c_str(), pcCurrentBuffer->GetRealName().c_str());

					Alert* pcSaveErrorAlert=new Alert(MSG_ALERT_SAVE_FILE_TITLE, string(buffer), Alert::ALERT_WARNING, MSG_ALERT_SAVE_FILE_OK.c_str(),NULL);
					pcSaveErrorAlert->Go(new Invoker());
					SetStatus(string(buffer));	
				}
		
				// Update menu items
				UpdateMenuItemStatus();
				
			}
			break;
		}

		case M_MENU_FILE_SAVE_AS:
		{
			if(pcFileSaveAs->IsEnabled())
			{
				pcSaveRequester = new FileRequester(FileRequester::SAVE_REQ, new Messenger(this), LatestSavePath);
				pcSaveRequester->Lock();
			   	pcSaveRequester->Start();
				pcSaveRequester->CenterInWindow(this);
				if(pcSaveRequester->IsVisible())
					pcSaveRequester->Show(false);
 				pcSaveRequester->Show(true);
				pcSaveRequester->MakeFocus();
				pcSaveRequester->Unlock();

			}
			break;
		}

		case M_SAVE_REQUESTED:
		{
			const char* pzFPath;

			FileRequester *pcRequester;
			if( pcMessage->FindPointer( "source", (void**)&pcRequester ) == EOK )
				pcRequester->Close();


			if(pcMessage->FindString("file/path",&pzFPath) == 0)
			{
				if(pcCurrentBuffer!=NULL)
				{
					if(pcCurrentBuffer->SaveAs(pzFPath))
					{
						char buffer[1024];
						sprintf(buffer, MSG_STATUSBAR_SAVED_FILE.c_str(), pcCurrentBuffer->GetRealName().c_str());
						SetStatus(string(buffer));		
					}
					else
					{
						char buffer[1024];
						sprintf(buffer, MSG_ALERT_SAVE_FILE_BODY.c_str(), pcCurrentBuffer->GetRealName().c_str());

						Alert* pcSaveErrorAlert=new Alert(MSG_ALERT_SAVE_FILE_TITLE, string(buffer), Alert::ALERT_WARNING, MSG_ALERT_SAVE_FILE_OK.c_str(),NULL);
						pcSaveErrorAlert->Go(new Invoker());
						SetStatus(string(buffer));		
					}
				}

				// Update menu items
				UpdateMenuItemStatus();

				// Remember path
				LatestSavePath=String(pzFPath);
			}

			break;
		}

		case M_MENU_FILE_SAVE_ALL:
		{
			if(pcFileSaveAll->IsEnabled())
			{
				SaveAllBuffers();
			}
		}
		case M_MENU_FILE_NEXT_TAB:
		{
			if(pcFileNextTab->IsEnabled())
			{
				// Get current selected tab
				int iCurr=pcMyTabView->GetSelection();
	
				// Calc next selection. If we have reach the end of the tab list we start over
				if(iCurr<pcMyTabView->GetTabCount()-1)
					iCurr++;
				else
					iCurr=0;

				// Set new buffer
				pcMyTabView->SetSelection(iCurr);
			}	
			break;
		}

		case M_MENU_FILE_PREV_TAB:
		{
			if(pcFilePrevTab->IsEnabled())
			{
				// Get current selected tab
				int iCurr=pcMyTabView->GetSelection();
	
				// Calc next selection. If we have reach the end of the tab list we start over
				if(iCurr>0)
					iCurr--;
				else
					iCurr=pcMyTabView->GetTabCount()-1;

				// Set new buffer
				pcMyTabView->SetSelection(iCurr);
			}	
			break;
		}

		case M_MENU_EDIT_CUT:
		case M_BUT_EDIT_CUT:
		{
			if(pcEditCut->IsEnabled())
				pcCurrentBuffer->Cut();
			break;
		}

		case M_MENU_EDIT_COPY:
		case M_BUT_EDIT_COPY:
		{
			if(pcEditCopy->IsEnabled())
				pcCurrentBuffer->Copy();
			break;
		}

		case M_MENU_EDIT_PASTE:
		case M_BUT_EDIT_PASTE:
		{
			if(pcEditPaste->IsEnabled())
				pcCurrentBuffer->Paste();
			break;
		}

		case M_MENU_EDIT_SELECT_ALL:
		{
			if(pcEditSelectAll->IsEnabled())
				pcCurrentBuffer->SelectAll();
			break;
		}

		case M_MENU_EDIT_UNDO:
		case M_BUT_EDIT_UNDO:
		{
			if(pcEditUndo->IsEnabled())
				pcCurrentBuffer->Undo();
			break;
		}

		case M_MENU_EDIT_REDO:
		case M_BUT_EDIT_REDO:
		{
			if(pcEditRedo->IsEnabled())
				pcCurrentBuffer->Redo();
			break;
		}

		case M_MENU_FIND_FIND:
		case M_BUT_FIND_FIND:
		{
			// Create dialog if it doesnt exists
			if(pcFindDialog==NULL)
				pcFindDialog=new FindDialog(Rect(200,200,450,310),this);

			UpdateMenuItemStatus();

			pcFindDialog->CenterInWindow(this);
			pcFindDialog->Raise();

			break;
		}

		case M_MENU_FIND_REPLACE:
		{
			// Create dialog if it doesnt exists
			if(pcReplaceDialog==NULL)
				pcReplaceDialog=new ReplaceDialog(Rect(200,200,450,310),this);

			UpdateMenuItemStatus();

			pcReplaceDialog->CenterInWindow(this);
			pcReplaceDialog->Raise();

			break;
		}

		case M_MENU_FIND_GOTO:
		{
			// Create dialog if it doesnt exists
			if(pcGotoDialog==NULL)
				pcGotoDialog=new GotoDialog(Rect(200,200,450,250),this);

			UpdateMenuItemStatus();

			pcGotoDialog->CenterInWindow(this);
			pcGotoDialog->Raise();

			break;
		}

		case M_MENU_FONT_SIZE:
		{
			float size;

			if (pcMessage->FindFloat("size",&size) == 0)
			{
				if(pcCurrentBuffer!=NULL)
					pcCurrentBuffer->SetFontSize(size);
			}

			break;
		}

		case M_MENU_FONT_FAMILY_AND_STYLE:
		{
			const char *family,*style;
			int32 flags;

			if (pcMessage->FindString("family",&family) == 0 && pcMessage->FindString("style",&style) == 0 && pcMessage->FindInt32( "flags", &flags) == 0 )
			{
				if(pcCurrentBuffer!=NULL)
					pcCurrentBuffer->SetFontFamily(family, style, flags);			
			}

			break;
		}

		case M_MENU_SETTINGS_SAVE_ON_CLOSE:
		{
			break;
		}
		
		case M_MENU_SETTINGS_SAVE_NOW:
		{
			cSettings.SetBool("SaveOnExit", ((CheckMenu*)pcSettingsSaveOnClose)->IsChecked());
			cSettings.SetRect("WindowPos", GetFrame());

			cSettings.Save();
			break;
		}

		case M_MENU_SETTINGS_RESET:
		{
			Rect cWinSize(100, 125, 100+600, 125+375);
			ResizeTo(cWinSize.Width(),cWinSize.Height());
			MoveTo(cWinSize.left,cWinSize.top);
			cSettings.SetRect("SetWindowPos", cWinSize);
			((CheckMenu*)pcSettingsSaveOnClose)->SetChecked(true);			
			LatestOpenPath="";
			LatestSavePath="";

			break;
		}
		
		case M_EDITOR_INVOKED:
		{
			int32 nEvent;
			
			if(pcMessage->FindInt32("events", &nEvent)==0)
			{
				if(pcCurrentBuffer!=NULL)
					pcCurrentBuffer->Invoked(nEvent);
			}

			break;
		}

		case M_INVOKED_TAB_CHANGED:  // User has changed the tab. Update pointer to current buffer
		{
			// Get new tab
			int iSel=pcMyTabView->GetSelection();
			
			// Make this page the current one
			SetCurrentTab(iSel);

			// Set cursor position
			pcCurrentBuffer->Invoked(TextView::EI_CURSOR_MOVED);

			// Set focus on this buffer
			pcCurrentBuffer->MakeFocus();

			// Update menu items
			UpdateMenuItemStatus();

			break;
		}

		case M_INVOKED_CLOSE_ALERT:   // The alert created in M_MENU_FILE_CLOSE
		{
			int32 nSelection;

			if(pcMessage->FindInt32("which", &nSelection))
				break;
			
			switch(nSelection)	
			{
				case 0:  // User wants to save it
					// Trying to save
					if(!pcCurrentBuffer->Save())
					{
						pcSaveAsRequester = new FileRequester(FileRequester::SAVE_REQ, new Messenger(this), 
			    											  LatestSavePath, 
														      FileRequester::NODE_FILE, false, 
															  new Message(M_INVOKED_SAVEAS_ALERT),
						 										  NULL, false);
						pcSaveAsRequester->Lock();
						pcSaveAsRequester->Start();						
						if(pcSaveAsRequester->IsVisible())
							pcSaveAsRequester->Show(false);
						pcSaveAsRequester->Show(true);
						pcSaveAsRequester->MakeFocus();
						pcSaveAsRequester->Unlock();
					}
					else
					{
						char buffer[1024];
						sprintf(buffer, MSG_STATUSBAR_FILE_CLOSED_AND_SAVED.c_str(), pcCurrentBuffer->GetRealName().c_str());

						// Delete the buffer (in the destructor the new current buffer will be set)
						SetStatus(string(buffer));

						// Update menu items
						UpdateMenuItemStatus();
						delete pcCurrentBuffer;					
					}
					break;
				case 1:  // User doesnt want to save it
					{
						char buffer[1024];
						sprintf(buffer, MSG_STATUSBAR_FILE_CLOSED_AND_NOT_SAVED.c_str(), pcCurrentBuffer->GetRealName().c_str());
	
						// Delete the buffer (in the destructor the new current buffer will be set)
						SetStatus(buffer);
						delete pcCurrentBuffer;
						// Update menu items
						UpdateMenuItemStatus();
					break;
					}
				default: // User cancels the operation
					break;
			}

			break;
		}

		case M_INVOKED_SAVEAS_ALERT:   // Created in M_INVOKED_CLOSE_ALERT by pcSaveAsRequester
		{
			const char* pzFPath;

			if(pcMessage->FindString("file/path",&pzFPath) == 0)
			{
				if(pcCurrentBuffer!=NULL)
				{
					if(pcCurrentBuffer->SaveAs(pzFPath))
					{
						char buffer[1024];
						sprintf(buffer, MSG_STATUSBAR_FILE_CLOSED_AND_SAVED.c_str(), pcCurrentBuffer->GetRealName().c_str());
						SetStatus(string(buffer));
						delete pcCurrentBuffer;
						// Update menu items
						UpdateMenuItemStatus();
					}
					else
					{
						char buffer[1024];
						sprintf(buffer, MSG_ALERT_SAVE_FILE_BODY.c_str(), pcCurrentBuffer->GetRealName().c_str());

						Alert* pcSaveErrorAlert=new Alert(MSG_ALERT_SAVE_FILE_TITLE, string(buffer), Alert::ALERT_WARNING, MSG_ALERT_SAVE_FILE_OK.c_str(),NULL);
						pcSaveErrorAlert->Go(new Invoker());
						SetStatus(string(buffer));		
					}
				}
			}

			pcSaveAsRequester=NULL;
			break;
		}

		case M_INVOKED_QUIT_ALERT: // Check if the user is sure to quit
		{
			int32 nSelection;

			if(pcMessage->FindInt32("which", &nSelection))
				break;
					
			if(nSelection==0)	
			{
				if(((CheckMenu*)pcSettingsSaveOnClose)->IsChecked())
				{

					cSettings.SetBool("SaveOnExit", true);
					cSettings.SetRect("WindowPos", GetFrame());
					cSettings.SetString("LoadPath", LatestOpenPath);
					cSettings.SetString("SavePath", LatestSavePath);
					cSettings.Save();
				}
				Application::GetInstance()->PostMessage(M_QUIT);
			}
			break;
		}


		default:
		{
			Window::HandleMessage(pcMessage);
			break;
		}
	}
}

bool AEditWindow::OkToQuit(void)
{
	if(AnyBufferNeedsToBeSaved()) // If yes display nice alert
	{
		pcQuitAlert=new Alert(MSG_ALERT_QUIT_TITLE, MSG_ALERT_QUIT_BODY , Alert::ALERT_QUESTION, 0x00, MSG_ALERT_QUIT_YES.c_str(), MSG_ALERT_QUIT_NO.c_str(), NULL);
		pcQuitAlert->CenterInWindow(this);
		pcQuitAlert->Go(new Invoker(new Message(M_INVOKED_QUIT_ALERT), this));
	}
	else  // if not send a message that application can be closed 
	{     // (same message as if the user press "yes" on the above alert)
		Message *msg=new Message(M_INVOKED_QUIT_ALERT);
		msg->AddInt32("which", 0);
		this->PostMessage(msg, this);
	}

	return (false);
}

void AEditWindow::UpdateMenuItemStatus(void)
{
	pcFileClose->SetEnable(pcCurrentBuffer!=NULL);
	pcFileSave->SetEnable(pcCurrentBuffer!=NULL);
	pcFileSaveAs->SetEnable(pcCurrentBuffer!=NULL);
	pcFileSaveAll->SetEnable(MenuItemSaveAllShallBeActive());
	pcEditUndo->SetEnable(pcCurrentBuffer!=NULL);
	pcEditRedo->SetEnable(pcCurrentBuffer!=NULL);
	pcEditCut->SetEnable(pcCurrentBuffer!=NULL);
	pcEditCopy->SetEnable(pcCurrentBuffer!=NULL);
	pcEditPaste->SetEnable(pcCurrentBuffer!=NULL);
	pcEditSelectAll->SetEnable(pcCurrentBuffer!=NULL);

	// Check if the menuitem "save" shall be enabled. It should only be
    // enable if the buffer has been modified since last saved
	if(pcCurrentBuffer!=NULL && !pcCurrentBuffer->SaveStatus())
		pcFileSave->SetEnable(false);

	// Set also enable/disable for the find, replace and goto dialogs
	if(pcFindDialog!=NULL)
		pcFindDialog->SetEnable(pcCurrentBuffer!=NULL);	
	if(pcReplaceDialog!=NULL)
		pcReplaceDialog->SetEnable(pcCurrentBuffer!=NULL);	
	if(pcGotoDialog!=NULL)
		pcGotoDialog->SetEnable(pcCurrentBuffer!=NULL);	

	// Check how many tabs we have. You dont need to switch if you have 0 or 1 tab only
	pcFileNextTab->SetEnable(pcMyTabView->GetTabCount()>1);
	pcFilePrevTab->SetEnable(pcMyTabView->GetTabCount()>1);

	// Check button bar
	pcImageFileSave->SetEnable(pcFileSave->IsEnabled());
	pcImageEditCopy->SetEnable(pcEditCopy->IsEnabled());
	pcImageEditCut->SetEnable(pcEditCut->IsEnabled());
	pcImageEditPaste->SetEnable(pcEditPaste->IsEnabled());
#ifdef ENABLE_UNDO
	pcImageEditUndo->SetEnable(pcEditUndo->IsEnabled());
	pcImageEditRedo->SetEnable(pcEditRedo->IsEnabled());
#endif
}

void AEditWindow::CreateNewBuffer(void)
{
	// Create the new buffer
	Buffer *buf=new Buffer(this ,NULL ,NULL);

	// Attach new buffer to tabview
	int iSel=pcMyTabView->AppendTab(buf->GetTabNameOfBuffer(), buf->GetEditView());

	// Make this page the current one
	pcMyTabView->SetSelection(iSel, false);

	// Set status bar
	char buffer[1024];
	sprintf(buffer, MSG_STATUSBAR_NEW_FILE_CREATED.c_str(), pcCurrentBuffer->GetRealName().c_str());
				
	SetStatus(string(buffer));

	// Update menu items
	UpdateMenuItemStatus();

	// Force a complete redraw
	buf->ForceRedraw();		

	// And set focus
	buf->MakeFocus();
}

// Drag and dropping
void AEditWindow::DragAndDrop(Message* pcData)
{
	int i=0;
	const char* pzFPath;
	
	while(pcData->FindString("file/path",&pzFPath, i)==0)
	{
		Load((char*) pzFPath);
		i++;
	}
}



// Private methods
void AEditWindow::Load(char* pzFileName)
{
	uint32 nFileSize=0;
	struct stat sFileStat;

	// Check if this file is already opened
	int r=CheckIfAlreadyOpened(std::string(pzFileName));

	if(r!=-1)
	{
		// Set the tab	
		pcMyTabView->SetSelection(r);
		return;
	}

	// Load it
	if(!stat(pzFileName, &sFileStat))
	{
		if(sFileStat.st_mode & S_IFREG)
		{
			// File is O.K, we can get the size & load it
			nFileSize=sFileStat.st_size;

			char* pnBuffer=(char*)malloc(nFileSize+1);
			pnBuffer=(char*)memset(pnBuffer,0,nFileSize+1);

			uint32 nCount;

			FILE *hFile=fopen(pzFileName,"r");

			if(hFile)
			{
				for(nCount=0;nCount<nFileSize;nCount++)
					pnBuffer[nCount]=fgetc(hFile);
			}

			fclose(hFile);

			pnBuffer[nFileSize-1]=0;  // This stops the string running off of the end and producing garbage.

			// File has been loaded. Lets open a new Buffer for this file

			// Create the new buffer
			Buffer *buf=new Buffer(this, pzFileName, pnBuffer);

			// Attach new buffer to tabview
			int iSel=pcMyTabView->AppendTab(buf->GetTabNameOfBuffer(), buf->GetEditView());

			// Make this page the current one. Dont send a message as it will screw up the status message
			pcMyTabView->SetSelection(iSel, false);
		
			// since the file has been loaded we can set the ContentChanged flag to false
			buf->ContentChanged(false);

			// Set status bar
			char buffer[1024];
			sprintf(buffer, MSG_STATUSBAR_FILE_LOADED_OK.c_str(), pcCurrentBuffer->GetRealName().c_str());
			SetStatus(string(buffer));

			// Update menu items
			UpdateMenuItemStatus();

			// Make focus
			buf->MakeFocus();

			buf->ForceRedraw();

			free(pnBuffer);
		}
		else
		{
			// Set status bar
			char buffer[1024];
			sprintf(buffer, MSG_STATUSBAR_FILE_UNABLE_TO_OPEN.c_str(), pzFileName);
			SetStatus(string(buffer));

			// Pop error window, file is not a "regular" file
			Alert* pcFileAlert = new Alert(MSG_ALERT_FILE_OPEN_TITLE, string(buffer), Alert::ALERT_WARNING, 0x00, MSG_ALERT_FILE_OPEN_OK.c_str(), NULL);
			pcFileAlert->Go();
		}
	}
	else  // File didnt exists. Open it anyway
	{
		// Create the new buffer
		Buffer *buf=new Buffer(this, pzFileName, NULL);

		// Attach new buffer to tabview
		int iSel=pcMyTabView->AppendTab(buf->GetTabNameOfBuffer(), buf->GetEditView());

		// Make this page the current one
		pcMyTabView->SetSelection(iSel, false);
		
		// since the file has been loaded we can set the ContentChanged flag to false
		buf->ContentChanged(false);

		// Set status bar
		char buffer[1024];
		sprintf(buffer, MSG_STATUSBAR_NEW_FILE_CREATED.c_str(), pzFileName);
		SetStatus(string(buffer));

		// Update menu items
		UpdateMenuItemStatus();

		// Make focus
		buf->MakeFocus();	
	}
}

// Set title of the window
void AEditWindow::SetTitle(std::string zTitle)
{
	Window::SetTitle(zTitle);
}

// Set the status bar 
void AEditWindow::SetStatus(std::string zTitle)
{
	pcStatusBar->SetStatus(zTitle);
}

