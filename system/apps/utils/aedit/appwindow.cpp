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

#include <fstream.h>

#include <gui/window.h>
#include <gui/requesters.h>

#include <util/invoker.h>
#include <util/application.h>

AEditWindow::AEditWindow(const Rect& cFrame) : Window(cFrame, "main_window", "AEdit V0.6")
{
	Rect cWinSize;
	pcSettings=new AEditSettings();

	if((pcSettings->Load())==EOK)
	{
		cWinSize=pcSettings->GetWindowPos();

		ResizeTo(cWinSize.Width(),cWinSize.Height());
		MoveTo(cWinSize.left,cWinSize.top);
	}

	Rect cBounds=GetBounds();
	cBounds.top+=16;

	// We use a single layoutview
	pcAppWindowLayout=new LayoutView(cBounds, "", NULL, CF_FOLLOW_ALL);
	pcVLayoutRoot=new VLayoutNode("appwindow_root");

	// Create & attach the button bar
	pcButtonBar=new ButtonBar(Rect(0,0,0,0),"button_bar");
	pcVLayoutRoot->AddChild(pcButtonBar);

	// Attach the buttons to the button bar
	pcButtonBar->AddButton(ImageFileNew,sizeof(ImageFileNew),"appwindow_filenew",new Message(M_BUT_FILE_NEW));
	pcButtonBar->AddButton(ImageFileOpen,sizeof(ImageFileOpen),"appwindow_fileopen",new Message(M_BUT_FILE_OPEN));
	pcButtonBar->AddButton(ImageFileSave,sizeof(ImageFileSave),"appwindow_filesave",new Message(M_BUT_FILE_SAVE));
	pcButtonBar->AddButton(ImageEditCut,sizeof(ImageEditCut),"appwindow_editcut",new Message(M_BUT_EDIT_CUT));
	pcButtonBar->AddButton(ImageEditCopy,sizeof(ImageEditCopy),"appwindow_editcopy",new Message(M_BUT_EDIT_COPY));
	pcButtonBar->AddButton(ImageEditPaste,sizeof(ImageEditPaste),"appwindow_editpaste",new Message(M_BUT_EDIT_PASTE));
	pcButtonBar->AddButton(ImageFindFind,sizeof(ImageFindFind),"appwindow_find",new Message(M_BUT_FIND_FIND));
	pcButtonBar->AddButton(ImageUndo,sizeof(ImageUndo),"appwindow_undo",new Message(M_BUT_EDIT_UNDO));
	pcButtonBar->AddButton(ImageRedo,sizeof(ImageRedo),"appwindow_redo",new Message(M_BUT_EDIT_REDO));
	/*
	pcButtonBar->AddButton(ImageStar,sizeof(ImageStar),"appwindow_anchor",new Message(M_BUT_ANCHOR_ANCHOR));
	pcButtonBar->AddButton(ImageUp,sizeof(ImageUp),"appwindow_up",new Message(M_BUT_ANCHOR_UP));
	pcButtonBar->AddButton(ImageDown,sizeof(ImageDown),"appwindow_down",new Message(M_BUT_ANCHOR_DOWN));
	*/

	// Create & attach EditView
	pcEditView=new EditView(Rect(0,0,0,0));
	pcEditView->SetTarget(this);
	pcEditView->SetMessage(new Message(M_EDITOR_INVOKED));
	pcEditView->SetEventMask(CodeView::EI_CONTENT_CHANGED | CodeView::EI_CURSOR_MOVED);
	pcVLayoutRoot->AddChild(pcEditView);

	// Attach the completed LayoutViews to the window
	pcAppWindowLayout->SetRoot(pcVLayoutRoot);
	AddChild(pcAppWindowLayout);

	// Create the menu bar
	cBounds=GetBounds();
	cBounds.bottom=16;

	pcMenuBar=new Menu(cBounds, "main_menu", ITEMS_IN_ROW, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT, WID_FULL_UPDATE_ON_H_RESIZE);

	// Create the menus
	pcAppMenu=new Menu(Rect(0,0,1,1),"Application",ITEMS_IN_COLUMN);
	pcAppMenu->AddItem("Quit",new Message(M_MENU_APP_QUIT));
	pcAppMenu->AddItem(new MenuSeparator());
	pcAppMenu->AddItem("About",new Message(M_MENU_APP_ABOUT));

	pcFileMenu=new Menu(Rect(0,0,1,1),"File",ITEMS_IN_COLUMN);
	pcFileMenu->AddItem("New",new Message(M_MENU_FILE_NEW));
	pcFileMenu->AddItem("Open",new Message(M_MENU_FILE_OPEN));
	pcFileMenu->AddItem("Save",new Message(M_MENU_FILE_SAVE));
	pcFileMenu->AddItem("Save as...",new Message(M_MENU_FILE_SAVE_AS));

	pcEditMenu=new Menu(Rect(0,0,1,1),"Edit",ITEMS_IN_COLUMN);
	pcEditMenu->AddItem("Cut",new Message(M_MENU_EDIT_CUT));
	pcEditMenu->AddItem("Copy",new Message(M_MENU_EDIT_COPY));
	pcEditMenu->AddItem("Paste",new Message(M_MENU_EDIT_PASTE));
	pcEditMenu->AddItem(new MenuSeparator());
	pcEditMenu->AddItem("Select all",new Message(M_MENU_EDIT_SELECT_ALL));
	pcEditMenu->AddItem(new MenuSeparator());
	pcEditMenu->AddItem("Undo",new Message(M_MENU_EDIT_UNDO));
	pcEditMenu->AddItem("Redo",new Message(M_MENU_EDIT_REDO));

	/*
	pcEditMenu->AddItem(new MenuSeparator());
	pcEditMenu->AddItem("Add Anchor", new Message(M_MENU_EDIT_ANCHOR));
	pcEditMenu->AddItem("Up", new Message(M_MENU_EDIT_UP));
	pcEditMenu->AddItem("Down", new Message(M_MENU_EDIT_DOWN));
	*/
		
	pcFindMenu=new Menu(Rect(0,0,1,1),"Find",ITEMS_IN_COLUMN);
	pcFindMenu->AddItem("Find",new Message(M_MENU_FIND_FIND));
	pcFindMenu->AddItem("Replace",new Message(M_MENU_FIND_REPLACE));
	pcFindMenu->AddItem(new MenuSeparator());
	pcFindMenu->AddItem("Goto line...",new Message(M_MENU_FIND_GOTO));

	pcSettingsMenu=new Menu(Rect(0,0,1,1),"Settings",ITEMS_IN_COLUMN);
	pcSettingsMenu->AddItem("Save settings on Close",new Message(M_MENU_SETTINGS_SAVE_ON_CLOSE));
	pcSettingsMenu->AddItem("Save settings now",new Message(M_MENU_SETTINGS_SAVE_NOW));
	pcSettingsMenu->AddItem("Reset settings", new Message(M_MENU_SETTINGS_RESET));

	pcFormatMenu=new Menu(Rect(0,0,1,1),"Format",ITEMS_IN_COLUMN);
	pcFormatMenu->AddItem("None",new Message(M_MENU_FORMAT_NONE));
	pcFormatMenu->AddItem(new MenuSeparator());
	pcFormatMenu->AddItem("C/C++",new Message(M_MENU_FORMAT_C));
	pcFormatMenu->AddItem("Java",new Message(M_MENU_FORMAT_JAVA));

	pcHelpMenu=new Menu(Rect(0,0,1,1),"Help",ITEMS_IN_COLUMN);
	pcHelpMenu->AddItem("Table of Contents",new Message(M_MENU_HELP_TOC));
	pcHelpMenu->AddItem("AEdit Homepage",new Message(M_MENU_HELP_HOMEPAGE));

	// Attach the menus.  Attach the menu bar to the window
	pcMenuBar->AddItem(pcAppMenu);
	pcMenuBar->AddItem(pcFileMenu);
	pcMenuBar->AddItem(pcEditMenu);
	pcMenuBar->AddItem(pcFindMenu);
	pcMenuBar->AddItem(pcFormatMenu);
	pcMenuBar->AddItem(pcSettingsMenu);
	pcMenuBar->AddItem(pcHelpMenu);

	pcMenuBar->SetTargetForItems(this);
	AddChild(pcMenuBar);

	// Reset the Format
	pcFormatC=NULL;
	pcFormatJava=NULL;

	nFormator='n';

	switch(pcSettings->GetFormat())
	{
		case 'c':
		{
			nFormator='c';
			pcFormatC=new Format_C();
			pcEditView->SetFormat(pcFormatC);
			break;
		}
		
		case 'j':
		{
			nFormator='j';
			pcFormatJava=new Format_java();
			pcEditView->SetFormat(pcFormatJava);
			break;
		}
	}

	bSaveOnExit=pcSettings->GetSaveOnExit();

	// Set the window title etc.
	zWindowTitle=AEDIT_RELEASE_STRING;
	zWindowTitle+=" : New File";

	SetTitle(zWindowTitle);
	SetSizeLimits(Point(300,250),Point(4096,4096));

	// Create the file requesters
	pcLoadRequester = new FileRequester(FileRequester::LOAD_REQ, new Messenger(this), getenv("$HOME"));
	pcSaveRequester = new FileRequester(FileRequester::SAVE_REQ, new Messenger(this), getenv("$HOME"));

	// Reset the changed flag
	bContentChanged=false;

	// Set the focus
	pcEditView->MakeFocus();
}

void AEditWindow::DispatchMessage(Message* pcMessage, Handler* pcHandler)
{
	bool bPassOn=true;

	// Check for a posible shortcut key
	if(pcMessage->GetCode()==M_KEY_DOWN)
	{
		std::string zRaw;
		int32 nQual=0;

		if(pcMessage->FindString("_raw_string", &zRaw)==0 && pcMessage->FindInt32("_qualifiers", &nQual)==0)
			bPassOn=MapKey(zRaw.c_str(),nQual);
	}

	// Check for a mouse-click event
	if(pcMessage->GetCode()==M_MOUSE_DOWN)
	{
		int32 nButtons=0;
		Point cPosition;
		
		if(pcMessage->FindInt32("_buttons",&nButtons)==0 && pcMessage->FindPoint("_scr_pos",&cPosition)==0)
			pcEditView->MouseDown(cPosition,nButtons);
	}

	// Pass on the Message to the base class (If the MapKey() method lets us)
	if(bPassOn)
		Window::DispatchMessage(pcMessage, pcHandler);
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
			std::string zAboutTitle="About ";
			zAboutTitle+=AEDIT_RELEASE_STRING;

			std::string zAboutText=AEDIT_RELEASE_STRING;
			zAboutText+="\n\nText Editor for Atheos\nCopyright 2000-2002 Kristian Van Der Vliet\n\nAEdit is released under the GNU General\nPublic License.  Please see the file COPYING,\ndistributed with AEdit, or http://www.gnu.org\nfor more information.";

			Alert* pcAboutAlert=new Alert(zAboutTitle,zAboutText,0x00,"O.K",NULL);
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

			// Reset the current file name
			zCurrentFile="";

			// Reset the change flag
			bContentChanged=false;

			break;
		}

		case M_MENU_FILE_OPEN:
		case M_BUT_FILE_OPEN:
		{
			pcLoadRequester->Show();
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
			pcSaveRequester->Show();
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

		case M_MENU_EDIT_ANCHOR:
		case M_BUT_ANCHOR_ANCHOR:
		{
			break;
		}

		case M_MENU_EDIT_UP:
		case M_BUT_ANCHOR_UP:
		{
			break;
		}

		case M_MENU_EDIT_DOWN:
		case M_BUT_ANCHOR_DOWN:
		{
			break;
		}

		case M_MENU_FIND_FIND:
		case M_BUT_FIND_FIND:
		{
			pcFindDialog=new FindDialog(Rect(200,200,450,310),this);
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

		case M_MENU_FORMAT_NONE:
		{
			nFormator='n';

			pcEditView->SetFormat(NULL);
			pcEditView->Invalidate();
			pcEditView->Flush();

			break;
		}

		case M_MENU_FORMAT_C:
		{
			nFormator='c';

			if(pcFormatC==NULL)
				pcFormatC=new Format_C();

			pcEditView->SetFormat(pcFormatC);

			break;
		}

		case M_MENU_FORMAT_JAVA:
		{
			nFormator='j';

			if(pcFormatJava==NULL)
				pcFormatJava=new Format_java();

			pcEditView->SetFormat(pcFormatJava);

			break;
		}

		case M_MENU_SETTINGS_SAVE_ON_CLOSE:
		{
			bSaveOnExit=!bSaveOnExit;
			break;
		}
		
		case M_MENU_SETTINGS_SAVE_NOW:
		{
			pcSettings->SetWindowPos(GetFrame());
			pcSettings->SetFormat(nFormator);
			pcSettings->SetSaveOnExit(bSaveOnExit);
			pcSettings->Save();

			break;
		}

		case M_MENU_SETTINGS_RESET:
		{
			ResizeTo(600,375);
			MoveTo(100,125);
			nFormator='n';
			bSaveOnExit=false;

			HandleMessage(new Message(M_MENU_FORMAT_NONE));

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
				if(!bContentChanged && (nEvent & CodeView::EI_CONTENT_CHANGED))
				{
					bContentChanged=true;
				}
				if(nEvent & CodeView::EI_CURSOR_MOVED)
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

	if(bSaveOnExit)
	{
		pcSettings->SetWindowPos(GetFrame());
		pcSettings->SetFormat(nFormator);
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
			Alert* pcFileAlert = new Alert("AEdit", "Unable to open file", 0x00, "O.K", NULL);
			pcFileAlert->Go();
		}
	}
	else
	{
			// Pop an error window, file does not exist?
			Alert* pcFileAlert = new Alert("AEdit", "Unable to open file", 0x00, "O.K", NULL);
			pcFileAlert->Go();
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

bool AEditWindow::MapKey(const char* pzString, int32 nQualifiers)
{
	if(nQualifiers & QUAL_CTRL)
	{
		if(!(nQualifiers & QUAL_REPEAT))
		{
			switch(*pzString)
			{
				case 'o':	// Open (Ctrl+O)
				{
					HandleMessage(new Message(M_MENU_FILE_OPEN));
					return(false);
				}

				case 's':	// Save (Ctrl+S)
				{
					HandleMessage(new Message(M_MENU_FILE_SAVE));
					return(false);
				}

				case 'a':		// Save As (Ctrl+A)
				{
					HandleMessage(new Message(M_MENU_FILE_SAVE_AS));
					return(false);
				}

				case 'f':		// Find (Ctrl+F)
				{
					HandleMessage(new Message(M_MENU_FIND_FIND));
					return(false);
				}

				case 'r':	// Replace (Ctrl+R)
				{
					HandleMessage(new Message(M_MENU_FIND_REPLACE));
					return(false);
				}

				case 'g':		// Goto Line (Ctrl+G)
				{
					HandleMessage(new Message(M_MENU_FIND_GOTO));
					return(false);
				}

				case 'q':	// Quit (Ctrl+Q)
				{
					HandleMessage(new Message(M_MENU_APP_QUIT));
					return(false);
				}

			}	// End of switch() block

		}	// End of if() for testing the repeat
	}	

	return(true);
}

