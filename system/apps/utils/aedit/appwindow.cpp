//  AEdit -:-  (C)opyright 2000-2002 Kristian Van Der Vliet
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
#include "appwindow_pixmaps.h"
#include "editview.h"
#include "messages.h"
#include "version.h"

#include "resources/aedit.h"

#include <fstream>

#include <gui/window.h>
#include <gui/requesters.h>
#include <gui/font.h>
#include <gui/checkmenu.h>

#include <util/invoker.h>
#include <util/application.h>

AEditWindow::AEditWindow(const Rect& cFrame) : Window(cFrame, "main_window", AEDIT_RELEASE_STRING)
{
	Rect cWinSize;
	pcSettings=new AEditSettings();
	
	if((pcSettings->Load())==EOK)
	{
		cWinSize=pcSettings->GetWindowPos();

		ResizeTo(cWinSize.Width(),cWinSize.Height());
		MoveTo(cWinSize.left,cWinSize.top);
	}

	SetupMenus();
	Rect cBounds=GetBounds();
	cBounds.top =pcMenuBar->GetBounds().bottom +1.0f;
	cBounds.bottom = BUTTON_HEIGHT + 4 + cBounds.top;

	// Create & attach the button bar
	pcButtonBar=new ButtonBar(Rect(0,cBounds.top,GetBounds().Width(),cBounds.bottom),"button_bar");
	AddChild(pcButtonBar);

	// Attach the buttons to the button bar
	pcButtonBar->AddButton(ImageFileNew,"appwindow_filenew",new Message(M_BUT_FILE_NEW));
	pcButtonBar->AddButton(ImageFileOpen,"appwindow_fileopen",new Message(M_BUT_FILE_OPEN));
	pcButtonBar->AddButton(ImageFileSave,"appwindow_filesave",new Message(M_BUT_FILE_SAVE));
	pcButtonBar->AddButton(ImageEditCut,"appwindow_editcut",new Message(M_BUT_EDIT_CUT));
	pcButtonBar->AddButton(ImageEditCopy,"appwindow_editcopy",new Message(M_BUT_EDIT_COPY));
	pcButtonBar->AddButton(ImageEditPaste,"appwindow_editpaste",new Message(M_BUT_EDIT_PASTE));
	pcButtonBar->AddButton(ImageFindFind,"appwindow_find",new Message(M_BUT_FIND_FIND));
#ifdef ENABLE_UNDO
	pcButtonBar->AddButton(ImageUndo,"appwindow_undo",new Message(M_BUT_EDIT_UNDO));
	pcButtonBar->AddButton(ImageRedo,"appwindow_redo",new Message(M_BUT_EDIT_REDO));
#endif

		// Create & attach EditView
	pcEditView=new EditView(Rect(0,cBounds.bottom+1.0f,GetBounds().Width(),GetBounds().Height()));
	pcEditView->SetTarget(this);
	pcEditView->SetMessage(new Message(M_EDITOR_INVOKED));
	pcEditView->SetEventMask(TextView::EI_CONTENT_CHANGED | TextView::EI_CURSOR_MOVED);
	pcEditView->SetMultiLine(true);
	AddChild(pcEditView);
	
	Font *font = new Font();
	font_properties cProperties;

	font->GetDefaultFont( DEFAULT_FONT_FIXED, &cProperties );

	zFamily=cProperties.m_cFamily;
	zStyle=cProperties.m_cStyle;
	vSize=cProperties.m_vSize;
	nFlags=cProperties.m_nFlags;

	font->SetProperties( cProperties );
	pcEditView->SetFont(font);
	font->Release();
	
	// Set the window title etc.
	zWindowTitle=AEDIT_RELEASE_STRING;
	zWindowTitle+=" : New File";

	SetTitle(zWindowTitle);
	SetSizeLimits(Point(300,250),Point(4096,4096));

	// Create the file requesters
	pcLoadRequester = new FileRequester(FileRequester::LOAD_REQ, new Messenger(this));
	pcLoadRequester->Start();
	pcSaveRequester = new FileRequester(FileRequester::SAVE_REQ, new Messenger(this));
	pcSaveRequester->Start();

	// Set an application icon for Dock
	os::BitmapImage *pcImage = new os::BitmapImage();
	os::Resources cRes( get_image_id() );
	pcImage->Load( cRes.GetResourceStream( "icon24x24.png" ) );
	SetIcon(pcImage->LockBitmap());

	// Reset the changed flag
	bContentChanged=false;

	// Set the focus
	pcEditView->MakeFocus();
}

void AEditWindow::SetupMenus()
{
	Rect cMenuFrame = GetBounds();
    Rect cMainFrame = cMenuFrame;
    cMenuFrame.bottom = 18;
	pcMenuBar=new Menu(cMenuFrame, "main_menu", ITEMS_IN_ROW, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP);

	// Create the menus
	pcAppMenu=new Menu(Rect(0,0,1,1),MSG_MENU_APPLICATION,ITEMS_IN_COLUMN);
	pcAppMenu->AddItem(MSG_MENU_APPLICATION_QUIT,new Message(M_MENU_APP_QUIT));
	pcAppMenu->AddItem(new MenuSeparator());
	pcAppMenu->AddItem(MSG_MENU_APPLICATION_ABOUT,new Message(M_MENU_APP_ABOUT));

	pcFileMenu=new Menu(Rect(0,0,1,1),MSG_MENU_FILE,ITEMS_IN_COLUMN);
	pcFileMenu->AddItem(MSG_MENU_FILE_NEW,new Message(M_MENU_FILE_NEW));
	pcFileMenu->AddItem(MSG_MENU_FILE_OPEN,new Message(M_MENU_FILE_OPEN));
	pcFileMenu->AddItem(MSG_MENU_FILE_SAVE,new Message(M_MENU_FILE_SAVE));
	pcFileMenu->AddItem(MSG_MENU_FILE_SAVE_AS,new Message(M_MENU_FILE_SAVE_AS));

	pcEditMenu=new Menu(Rect(0,0,1,1),MSG_MENU_EDIT,ITEMS_IN_COLUMN);
	pcEditMenu->AddItem(MSG_MENU_EDIT_CUT,new Message(M_MENU_EDIT_CUT));
	pcEditMenu->AddItem(MSG_MENU_EDIT_COPY,new Message(M_MENU_EDIT_COPY));
	pcEditMenu->AddItem(MSG_MENU_EDIT_PASTE,new Message(M_MENU_EDIT_PASTE));
	pcEditMenu->AddItem(new MenuSeparator());
	pcEditMenu->AddItem(MSG_MENU_EDIT_SELECT_ALL,new Message(M_MENU_EDIT_SELECT_ALL));
#ifdef ENABLE_UNDO
	pcEditMenu->AddItem(new MenuSeparator());
	pcEditMenu->AddItem(MSG_MENU_EDIT_UNDO,new Message(M_MENU_EDIT_UNDO));
	pcEditMenu->AddItem(MSG_MENU_EDIT_REDO,new Message(M_MENU_EDIT_REDO));
#endif
	

	pcFindMenu=new Menu(Rect(0,0,1,1),MSG_MENU_FIND,ITEMS_IN_COLUMN);
	pcFindMenu->AddItem(MSG_MENU_FIND_FIND,new Message(M_MENU_FIND_FIND));
	pcFindMenu->AddItem(MSG_MENU_FIND_REPLACE,new Message(M_MENU_FIND_REPLACE));
	pcFindMenu->AddItem(new MenuSeparator());
	pcFindMenu->AddItem(MSG_MENU_FIND_GOTO_LINE,new Message(M_MENU_FIND_GOTO));

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

	bSaveOnExit=pcSettings->GetSaveOnExit();



	// Create the Settings menu
	pcSettingsMenu=new Menu(Rect(0,0,1,1),MSG_MENU_SETTINGS,ITEMS_IN_COLUMN);
	pcSettingsMenu->AddItem(new CheckMenu(MSG_MENU_SETTINGS_SAVE_ON_CLOSE,new Message(M_MENU_SETTINGS_SAVE_ON_CLOSE), bSaveOnExit));
	pcSettingsMenu->AddItem(new MenuSeparator());
	pcSettingsMenu->AddItem(MSG_MENU_SETTINGS_SAVE_NOW, new Message(M_MENU_SETTINGS_SAVE_NOW));
	pcSettingsMenu->AddItem(MSG_MENU_SETTINGS_RESET, new Message(M_MENU_SETTINGS_RESET));

	pcHelpMenu=new Menu(Rect(0,0,1,1),MSG_MENU_HELP,ITEMS_IN_COLUMN);
	pcHelpMenu->AddItem(MSG_MENU_HELP_TABLE_OF_CONTENTS,new Message(M_MENU_HELP_TOC));
	pcHelpMenu->AddItem(MSG_MENU_HELP_HOMEPAGE,new Message(M_MENU_HELP_HOMEPAGE));

	// Attach the menus.  Attach the menu bar to the window
	pcMenuBar->AddItem(pcAppMenu);
	pcMenuBar->AddItem(pcFileMenu);
	pcMenuBar->AddItem(pcEditMenu);
	pcMenuBar->AddItem(pcFindMenu);
	pcMenuBar->AddItem(pcFontMenu);
	pcMenuBar->AddItem(pcSettingsMenu);
	pcMenuBar->AddItem(pcHelpMenu);
	
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
			String zAboutTitle = MSG_ABOUT_TITLE + " ";
			zAboutTitle+=AEDIT_RELEASE_STRING;

			String zAboutText=AEDIT_RELEASE_STRING;
			zAboutText+=MSG_ABOUT_TEXT;

			Alert* pcAboutAlert=new Alert(zAboutTitle,zAboutText,Alert::ALERT_INFO,0x00,MSG_ABOUT_CLOSE.c_str(),NULL);
			pcAboutAlert->CenterInWindow(this);
			pcAboutAlert->Go(new Invoker());

			break;
		}

		case M_MENU_FILE_NEW:
		case M_BUT_FILE_NEW:
		{
			pcEditView->Clear(false);
			pcEditView->MakeFocus();

			// Reset the window title bar
			zWindowTitle=AEDIT_RELEASE_STRING;
			zWindowTitle+=" : New File";

			SetTitle(zWindowTitle);

			// Set the focus
			pcEditView->MakeFocus();

			// Reset the current file name
			zCurrentFile="";

			// Reset the change flag
			bContentChanged=false;

			break;
		}

		case M_MENU_FILE_OPEN:
		case M_BUT_FILE_OPEN:
		{
			pcLoadRequester->CenterInWindow(this);
			pcLoadRequester->Lock();
			pcLoadRequester->Show();
			pcLoadRequester->Unlock();
			pcLoadRequester->MakeFocus();

			break;
		}

		case M_LOAD_REQUESTED:
		{
			const char* pzFPath;

			if(pcMessage->FindString("file/path",&pzFPath) == 0)
			{
				zCurrentFile=pzFPath;
				Load((char*)pzFPath);

				zWindowTitle=AEDIT_RELEASE_STRING;
				zWindowTitle+=" : ";
				zWindowTitle+=pzFPath;

				bContentChanged=false;

				SetTitle(zWindowTitle);
				pcEditView->MakeFocus();
			}

			break;
		}

		case M_MENU_FILE_SAVE:
		case M_BUT_FILE_SAVE:
		{
			if(zCurrentFile!="")
			{
				Save(zCurrentFile.c_str());
			}
			else
			{
				pcMessage->SetCode(M_MENU_FILE_SAVE_AS);	// The file hasn't been created yet
				HandleMessage(pcMessage);					// So we'll pop a "Save As" requester instead
			}

			break;
		}

		case M_MENU_FILE_SAVE_AS:
		{
			pcSaveRequester->CenterInWindow(this);
			pcSaveRequester->Lock();
			pcSaveRequester->Show();
			pcSaveRequester->Unlock();
			pcSaveRequester->MakeFocus();

			break;
		}

		case M_SAVE_REQUESTED:
		{
			const char* pzFPath;

			if(pcMessage->FindString("file/path",&pzFPath) == 0)
			{
				zCurrentFile=pzFPath;
				Save(pzFPath);

				zWindowTitle=AEDIT_RELEASE_STRING;
				zWindowTitle+=" : ";
				zWindowTitle+=pzFPath;

				bContentChanged=false;

				SetTitle(zWindowTitle);
				pcEditView->MakeFocus();
			}

			break;
		}

		case M_MENU_EDIT_CUT:
		case M_BUT_EDIT_CUT:
		{
			pcEditView->Cut();
			break;
		}

		case M_MENU_EDIT_COPY:
		case M_BUT_EDIT_COPY:
		{
			pcEditView->Copy();
			break;
		}

		case M_MENU_EDIT_PASTE:
		case M_BUT_EDIT_PASTE:
		{
			pcEditView->Paste();
			break;
		}

		case M_MENU_EDIT_SELECT_ALL:
		{
			pcEditView->SelectAll();
			break;
		}

		case M_MENU_EDIT_UNDO:
		case M_BUT_EDIT_UNDO:
		{
			pcEditView->Undo();
			break;
		}

		case M_MENU_EDIT_REDO:
		case M_BUT_EDIT_REDO:
		{
			pcEditView->Redo();
			break;
		}

		case M_MENU_FIND_FIND:
		case M_BUT_FIND_FIND:
		{
			pcFindDialog=new FindDialog(Rect(200,200,450,310),this);
			pcFindDialog->CenterInWindow(this);
			pcFindDialog->Show();
			pcFindDialog->MakeFocus();

			break;
		}

		case M_BUT_FIND_GO:
		{
			if(!pcMessage->FindString("find_text",&zFindText,0) && !pcMessage->FindBool("case_check_state",&bCaseSensitive,0))
			{
				nTotalLines=pcEditView->GetLineCount();

				for(nCurrentLine=0;nCurrentLine<nTotalLines;nCurrentLine++)
				{
					nTextPosition=pcEditView->Find(pcEditView->GetLine(nCurrentLine), zFindText, bCaseSensitive, 0);

					if(nTextPosition!=-1)	// Find() returns -1 if the text isn't found
					{
						pcEditView->SetCursor(nTextPosition,nCurrentLine, false, false);
						pcEditView->SetCursor((nTextPosition+zFindText.size()),nCurrentLine, true, false);

						bHasFind=true;

						break;
					}
				}	// End of for() loop
			}	// End of if()

			break;
		}

		case M_BUT_FIND_NEXT:
		{
			if(bHasFind)
			{
				nTextPosition+=zFindText.size();	// We want to start the search from just past the last instance.
													// There is a lot of fudging in this block of code :)

				for(;nCurrentLine<nTotalLines;nCurrentLine++)
				{
					nTextPosition=pcEditView->Find(pcEditView->GetLine(nCurrentLine), zFindText, bCaseSensitive, nTextPosition);

					if(nTextPosition!=-1)
					{
						pcEditView->SetCursor(nTextPosition,nCurrentLine, false, false);
						pcEditView->SetCursor((nTextPosition+zFindText.size()),nCurrentLine, true, false);

						break;
					}
					else
					{
						nTextPosition=0;		// This is a big fudge (See the call to EditText::Find() above!)
					}
				}	// End of for() loop
			}	// End of if()

			break;
		}

		case M_BUT_FIND_CLOSE:
		{
			// Reset the variables used to indicate the last positions
			// we searched from / found text at.
			bHasFind=false;
			nTextPosition=0;
			nCurrentLine=0;

			// Close the Window
			pcFindDialog->Close();
			pcEditView->MakeFocus();

			break;
		}

		case M_MENU_FIND_REPLACE:
		{
			pcReplaceDialog=new ReplaceDialog(Rect(200,200,450,310),this);
			pcReplaceDialog->CenterInWindow(this);
			pcReplaceDialog->Show();
			pcReplaceDialog->MakeFocus();

			break;
		}

		case M_BUT_REPLACE_DO:
		{
			if(bHasFind)
			{
				if(!pcMessage->FindString("replace_text",&zReplaceText,0))
				{
					pcEditView->Delete();
					pcEditView->Insert(zReplaceText.c_str(),false);

					HandleMessage(new Message(M_BUT_FIND_NEXT));
				}	// End of if()
			}	// End of outer if()

			break;
		}

		case M_BUT_REPLACE_CLOSE:
		{
			// Reset the variables used to indicate the last positions
			// we searched from / found text at.
			bHasFind=false;
			nTextPosition=0;
			nCurrentLine=0;

			// Close the Window
			pcReplaceDialog->Close();
			pcEditView->MakeFocus();

			break;
		}

		case M_MENU_FIND_GOTO:
		{
			pcGotoDialog=new GotoDialog(Rect(200,200,450,250),this);
			pcGotoDialog->CenterInWindow(this);
			pcGotoDialog->Show();
			pcGotoDialog->MakeFocus();

			break;
		}

		case M_BUT_GOTO_GOTO:
		{
			int32 nGotoLineNo=0;

			if(!pcMessage->FindInt32("goto_lineno",&nGotoLineNo,0))
				pcEditView->SetCursor(0,(nGotoLineNo-1),false,false);

			// Fall through to close the GotoDialog
		}

		case M_BUT_GOTO_CLOSE:
		{
			// Close the Goto window
			pcGotoDialog->Close();
			pcEditView->MakeFocus();

			break;
		}

		case M_MENU_FONT_SIZE:
		{
			float size;

			if (pcMessage->FindFloat("size",&size) == 0)
			{
				Font *font = new Font();
				font->SetFamilyAndStyle(zFamily.c_str(),zStyle.c_str());
				font->SetSize(size);
				font->SetFlags(nFlags);
				pcEditView->SetFont(font);
				font->Release();

				vSize = size;
			}

			break;
		}

		case M_MENU_FONT_FAMILY_AND_STYLE:
		{
			const char *family,*style;
			int32 flags;

			if (pcMessage->FindString("family",&family) == 0 && pcMessage->FindString("style",&style) == 0 && pcMessage->FindInt32( "flags", &flags) == 0 )
			{
				Font *font = new Font();
				font->SetFamilyAndStyle(family,style);
				font->SetSize(vSize);
				flags |= FPF_SMOOTHED;
				font->SetFlags(flags);
				pcEditView->SetFont(font);
				font->Release();

				zFamily.assign(family);
				zStyle.assign(style);
				nFlags = flags;
			}

			break;
		}

		case M_MENU_SETTINGS_SAVE_ON_CLOSE:
		{
			pcMessage->FindBool("status",&bSaveOnExit);
			pcSettings->SetSaveOnExit(bSaveOnExit);
			
			break;
		}
		
		case M_MENU_SETTINGS_SAVE_NOW:
		{
			pcSettings->SetWindowPos(GetFrame());
			pcSettings->SetSaveOnExit(bSaveOnExit);
			pcSettings->Save();

			break;
		}

		case M_MENU_SETTINGS_RESET:
		{
			ResizeTo(600,375);
			MoveTo(100,125);
			bSaveOnExit=false;

			break;
		}
		
		case M_MENU_HELP_TOC:
		{
			break;
		}

		case M_MENU_HELP_HOMEPAGE:
		{
			break;
		}

		case M_EDITOR_INVOKED:
		{
			int32 nEvent;
			
			if(pcMessage->FindInt32("events", &nEvent)==0)
			{
				if(!bContentChanged && (nEvent & TextView::EI_CONTENT_CHANGED))
				{
					bContentChanged=true;
				}
				if(nEvent & TextView::EI_CURSOR_MOVED)
				{
					// Do nothing just yet
				}
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
	if(bContentChanged)
	{
		// Pop an "Are you sure?" dialog
	}

	if(bSaveOnExit==true)
	{
		pcSettings->SetWindowPos(GetFrame());
		pcSettings->SetSaveOnExit(bSaveOnExit);
		pcSettings->Save();
	}

	Application::GetInstance()->PostMessage(M_QUIT);
	
	delete(pcSettings);

	return(true);
}

void AEditWindow::LoadOnStart(char* pzFilename)
{
	zCurrentFile=pzFilename;
	Load(pzFilename);

	zWindowTitle=AEDIT_RELEASE_STRING;
	zWindowTitle+=" : ";
	zWindowTitle+=pzFilename;

	SetTitle(zWindowTitle);
}

// Private methods
void AEditWindow::Load(char* pzFileName)
{
	uint32 nFileSize=0;
	struct stat sFileStat;

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

			pcEditView->Clear(false);
			pcEditView->Set(pnBuffer,false);

			free(pnBuffer);
		}
		else
		{
			// Pop error window, file is not a "regular" file
			Alert* pcFileAlert = new Alert("AEdit", "Unable to open file", Alert::ALERT_WARNING, 0x00, "O.K", NULL);
			pcFileAlert->Go();
		}
	}
}

void AEditWindow::Save(const char* pzFileName)
{
	uint32 nLineCount,nCurrentLine;

	ofstream hFile(pzFileName);

	if(hFile)
	{
		nLineCount=pcEditView->GetLineCount();
   
		for(nCurrentLine=0;nCurrentLine<nLineCount;nCurrentLine++)
			hFile << pcEditView->GetLine(nCurrentLine) << endl;
	}
	else
	{
		//Open an error window
		Alert* pcFileAlert = new Alert("AEdit", "Unable to save file", 0x00, "O.K", NULL);
		pcFileAlert->Go();
	}

	hFile.close();

}

