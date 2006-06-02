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

#include "convertdialog.h"
#include "commonfuncs.h"
#include <storage/filereference.h>
#include <unistd.h>

using namespace os;

/**********************************************
* Description: ConvertDialog constructor
* Author: Rick Caudill
* Date: Thu Mar 18 22:57:43 2004
**********************************************/
ConvertDialog::ConvertDialog(const String& cFormat, const String& cFile) : Window(Rect(0,0,220,100),"convert", "Convert to html:",WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT | WND_NOT_RESIZABLE)
{
	cMainFormat = cFormat;
	cFileName = cFile;
	
	Layout();
	SetTabs();

	pcFileReq = new FileRequester(FileRequester::LOAD_REQ,new Messenger(this),getenv("$HOME"),FileRequester::NODE_FILE);
}

/**********************************************
* Description: ConvertDialog destructor
* Author: Rick Caudill
* Date: Thu Mar 18 22:57:43 2004
**********************************************/
ConvertDialog::~ConvertDialog()
{
}

/**********************************************
* Description: Handles messages between the convert dialog
* Author: Rick Caudill
* Date: Thu Mar 18 22:57:43 2004
**********************************************/
void ConvertDialog::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_CONVERT_CANCEL:
			OkToQuit();
			break;

		case M_CONVERT:
			Convert();
			break;
	
		case M_REQ_OPEN:
		{
			Lock();
			pcFileReq->CenterInWindow(this);
			pcFileReq->Show();
			pcFileReq->MakeFocus();
		}
	
		case M_LOAD_REQUESTED:
		{
			String cPath;
			Unlock();
			if (pcMessage->FindString("file/path",&cPath)==0)
			{
				if (cPath[cPath.size()-1] != '/')
					cPath+="/";
				pcTVSaveTo->Set(cPath.c_str());
			}
			break;
		}
	}
}

/**********************************************
* Description: Called when the window quits
* Author: Rick Caudill
* Date: Thu Mar 18 22:57:43 2004
**********************************************/
bool ConvertDialog::OkToQuit()
{
	Close();
	return false;
}

/**********************************************
* Description: Lays out the dialog
* Author: Rick Caudill
* Date: Thu Mar 18 22:57:43 2004
**********************************************/
void ConvertDialog::Layout()
{
	pcLayoutView = new LayoutView(GetBounds(), "L");
	pcMain = new VLayoutNode("VL");
	pcMain->SetHAlignment(ALIGN_CENTER);
	pcMain->SetVAlignment(ALIGN_CENTER);
	pcMain->SetBorders(Rect(5,5,5,5));

	VLayoutNode* pcNoteNode = new VLayoutNode("HLNoteNode");
	pcNoteNode->SetVAlignment(ALIGN_CENTER);
	pcNoteNode->AddChild(new HLayoutSpacer("",25,25));
	pcNoteNode->AddChild(pcFileString = new StringView(Rect(),"SVFileString",(String)"File will save as\n'" + GetRelativePath(cFileName) + (String)".html'."),ALIGN_CENTER);

	HLayoutNode* pcFileNode = new HLayoutNode("HLFileNode");
	pcFileNode->AddChild(pcSaveToString=new StringView(Rect(0,0,0,0),"SVSaveToString","Save To:"));
	pcFileNode->AddChild(new HLayoutSpacer("",5,5));
	pcFileNode->AddChild(pcTVSaveTo=new TextView(Rect(0,0,0,0),"",""),CF_FOLLOW_LEFT | CF_FOLLOW_TOP | CF_FOLLOW_RIGHT);
	pcTVSaveTo->SetMultiLine(false);
	pcFileNode->AddChild(new HLayoutSpacer("",3,3));
	pcFileNode->AddChild(pcIBTSaveTo=new ImageButton(Rect(),"","",new Message(M_REQ_OPEN),LoadImageFromResource("folder.png"),ImageButton::IB_TEXT_BOTTOM,true,false,true),CF_FOLLOW_NONE);
	
	
	HLayoutNode* pcCheckNode = new HLayoutNode("HLCheckNode");
	pcCheckNode->AddChild(new HLayoutSpacer("",15,15));
	pcCheckNode->AddChild(pcCssCheck = new CheckBox(Rect(0,0,0,0),"","_Export Css",NULL));
	pcCssCheck->SetEnable(false);

	HLayoutNode* pcButtonNode = new HLayoutNode("HLButtonNode");
	pcButtonNode->AddChild(pcConvertButton = new Button(Rect(0,0,0,0),"convert_but","Con_vert",new Message(M_CONVERT)));
	pcButtonNode->AddChild(new HLayoutSpacer("",20,20));
	pcButtonNode->AddChild(pcCancelButton = new Button(Rect(0,0,0,0),"cancel_but","_Cancel",new Message(M_CONVERT_CANCEL)));
	pcButtonNode->SameWidth("convert_but","cancel_but",NULL);
	
	pcMain->AddChild(new VLayoutSpacer("",5,5));	
	pcMain->AddChild(pcNoteNode);
	pcMain->AddChild(new VLayoutSpacer("",5,5));
	pcMain->AddChild(pcFileNode);
	pcMain->AddChild(new VLayoutSpacer("",5,5));
	pcMain->AddChild(pcCheckNode);
	pcMain->AddChild(new VLayoutSpacer("",5,5));
	pcMain->AddChild(pcButtonNode);

	pcMain->SameWidth("HLNoteNode","HLButtonNode",NULL);

	pcLayoutView->SetRoot(pcMain);
	AddChild(pcLayoutView);
}

/**********************************************
* Description: Sets the tab position
* Author: Rick Caudill
* Date: Thu Mar 18 22:57:43 2004
**********************************************/
void ConvertDialog::SetTabs()
{
	SetDefaultButton(pcConvertButton);
	pcConvertButton->SetShortcut(ShortcutKey("CTRL+V"));
	pcCancelButton->SetShortcut(ShortcutKey("CTRL+C"));
	pcCssCheck->SetShortcut(ShortcutKey("CTRL+E"));
	pcTVSaveTo->SetTabOrder(NEXT_TAB_ORDER);
	pcIBTSaveTo->SetTabOrder(NEXT_TAB_ORDER);
	pcCssCheck->SetTabOrder(NEXT_TAB_ORDER);
	pcConvertButton->SetTabOrder(NEXT_TAB_ORDER);
	pcCancelButton->SetTabOrder(NEXT_TAB_ORDER);
}

/**********************************************
* Description: Converts the file into a html file
* Author: Rick Caudill
* Date: Thu Mar 18 22:57:43 2004
**********************************************/
void ConvertDialog::Convert()
{
	String cPath = "";
	String cOutPath= pcTVSaveTo->GetBuffer()[0]=="" ? getenv("$HOME") : pcTVSaveTo->GetBuffer()[0] ;
	
	Directory* pcDir = new Directory("^/bin/");
	pcDir->GetPath(&cPath);
	cPath += "/source-highlight";
	delete pcDir;

	if (cOutPath[cOutPath.size()-1] != '/')
		cOutPath+="/";

	cOutPath+=cFileName;
	cOutPath+=".html";
	String cFileType;


	if (cMainFormat == ".cpp" || cMainFormat == ".c" || cMainFormat == ".h" || cMainFormat == ".C")
	{	
		cFileType = "cpp";
	}
	else if (cMainFormat == ".perl" || cMainFormat == ".pm")
	{
		cFileType = "perl";
	}
	
	else if (cMainFormat == ".java")
	{
		cFileType = "java";
	}
	
	else if (cMainFormat == "Changelog" || cMainFormat == "changelog")
	{
		cFileType = "changelog";
	}	
	
	if( fork() == 0 )
	{
		set_thread_priority( -1, 0 );
		execlp(cPath.c_str(),cPath.c_str(),"-s",cFileType.c_str(),"-f", "html","-i",cFileName.c_str(),"-o",cOutPath.c_str(),NULL);
	}
	OkToQuit();
}
