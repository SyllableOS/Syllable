/*
 *  ATail - Graphic replacement for tail
 *  Copyright (C) 2001-2004 Henrik Isaksson
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details. 
 *
 *  You should have received a copy of the GNU Library General Public 
 *  License along with this library; if not, write to the Free 
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */

#include <util/application.h>
#include <gui/requesters.h>
#include <gui/filerequester.h>
#include <gui/image.h>
#include <util/resources.h>

#include <iostream>
#include <fstream>
#include "tailwin.h"
#include "resources/LogMonitor.h"

using namespace std;

/************************************************
* Description: Loads a bitmapimage from resource
* Author: Rick Caudill
* Date: Thu Mar 18 21:32:37 2004
*************************************************/
os::BitmapImage* LoadImageFromResource( const os::String &cName )
{
	os::BitmapImage * pcImage = new os::BitmapImage();
	os::Resources cRes( get_image_id() );
	os::ResStream * pcStream = cRes.GetResourceStream( cName.c_str() );
	if( pcStream == NULL )
	{
		throw( os::errno_exception("") );
	}
	pcImage->Load( pcStream );
	return pcImage;
}

TailView::TailView(const Rect& cWindowBounds, char* pzTitle, char* pzBuffer, uint32 nResizeMask , uint32 nFlags)
	:TextView (cWindowBounds, pzTitle, pzBuffer, nResizeMask , nFlags)
{
	m_ContextMenu=new Menu(Rect(0,0,0,0),"",ITEMS_IN_COLUMN);

	m_ContextMenu->AddItem(new MenuItem(STR_POPUPMENU_COPY,new Message(ID_COPY),STR_POPUPMENU_COPY_SH,LoadImageFromResource("edit-copy.png")));
	m_ContextMenu->AddItem(new MenuItem(STR_POPUPMENU_CLEAR, new Message(ID_CLEAR),STR_POPUPMENU_CLEAR_SH, LoadImageFromResource("document-new.png")));
	m_ContextMenu->AddItem(new MenuSeparator);
	
	m_ContextMenu->AddItem(new MenuItem(STR_POPUPMENU_OPEN, new Message(ID_OPEN),STR_POPUPMENU_OPEN_SH,LoadImageFromResource("document-open.png")));
	m_ContextMenu->AddItem(new MenuItem(STR_POPUPMENU_SAVE, new Message(ID_SAVE),STR_POPUPMENU_SAVE_SH, LoadImageFromResource("document-save.png")));
	m_ContextMenu->AddItem(new MenuSeparator);
	
	m_ContextMenu->AddItem(new MenuItem(STR_POPUPMENU_ABOUT, new Message(ID_ABOUT),STR_POPUPMENU_ABOUT_SH, LoadImageFromResource("about.png")));
	m_ContextMenu->AddItem(new MenuSeparator);
	m_ContextMenu->AddItem(new MenuItem(STR_POPUPMENU_QUIT, new Message(ID_QUIT),STR_POPUPMENU_QUIT_SH, LoadImageFromResource("close.png")));
}

void TailWin::HandleMessage(Message *msg)
{
	switch(msg->GetCode()) {
		case ID_COPY:
			m_TextView->SelectAll();
			m_TextView->Copy();
			m_TextView->ClearSelection();
			break;
		case ID_CLEAR:
			m_TextView->Clear();
			break;
		case ID_SAVE:
			{
				
				FileRequester *fr = new FileRequester(FileRequester::SAVE_REQ,
					new Messenger(this));
				fr->Show();
				fr->MakeFocus();
			}
			break;

			case ID_OPEN:
			{
				FileRequester *fr = new FileRequester(FileRequester::LOAD_REQ,new Messenger(this));
				fr->Show();
				fr->MakeFocus();
			}
			break;
		case M_SAVE_REQUESTED:
			{
				const char *path;
				if(msg->FindString("file/path", &path) == 0) {
					Save(path);
				}
			}
			break;
		case M_LOAD_REQUESTED:
			{
				os::String path;
				if (msg->FindString("file/path",&path) == 0 )
				{
					RemoveTimer(this,10);
					m_TextView->Clear();
					m_FileName = new String(path);
					AddTimer(this, 10, 1000000, false);
				}
				break;
			}
		case ID_ABOUT:
			{
				char* abouttext = new char[ STR_ABOUT.size() + 64 ];
				sprintf( abouttext, STR_ABOUT.c_str(), ATAIL_VERSION );
				Alert *About = new Alert(STR_ABOUT_WINTITLE,
					abouttext, 0x00, STR_ABOUT_OKBUTTON.c_str(), NULL);
				About->Go(new Invoker(new Message(ID_NOP), 0));
			}
			break;
		case ID_QUIT:
			Application::GetInstance()->PostMessage(M_QUIT);
			break;
		case ID_NOP:
			// Do nothing, dummy ID for the about requester
			break;
		default:
			Window::HandleMessage(msg);
	}
}

void TailView::KeyDown(const char* cString, const char* cRawString, uint32 nQualifiers)
{
	TextView::KeyDown(cString, cRawString, nQualifiers);
}


void TailWin::TimerTick(int id)
{
	Tail();
}

TailWin::TailWin(const Rect & r, const String & file, uint32 desktopmask)
	:Window(r, "TailWin", os::String("Monitor: ") + file, 0, desktopmask)
{
	Rect bounds = GetBounds();

	m_TextView = new TailView(bounds, "tv", "", CF_FOLLOW_ALL, /*WID_FULL_UPDATE_ON_RESIZE|*/WID_WILL_DRAW);
	
	m_TextView->SetMultiLine(true);
	m_TextView->SetReadOnly(true);
	m_TextView->MakeFocus(true);

	AddShortcut( ShortcutKey(STR_POPUPMENU_COPY_SH), new Message(ID_COPY) );
	AddShortcut( ShortcutKey(STR_POPUPMENU_CLEAR_SH), new Message(ID_CLEAR) );
	AddShortcut( ShortcutKey(STR_POPUPMENU_SAVE_SH), new Message(ID_SAVE) );
	AddShortcut( ShortcutKey(STR_POPUPMENU_ABOUT_SH), new Message(ID_ABOUT) );
	AddShortcut( ShortcutKey(STR_POPUPMENU_QUIT_SH), new Message(ID_QUIT) );

	/*Font* pcAppFont = new Font;
	pcAppFont->SetFamilyAndStyle("Lucida Sans Typewriter", "Regular");
	pcAppFont->SetSize(8);

	m_TextView->SetFont(pcAppFont);
	pcAppFont->Release();
	*/
	AddChild(m_TextView);

		/* Set the application icon */
	Resources cRes( get_image_id() );
	BitmapImage	*pcAppIcon = new BitmapImage();
	pcAppIcon->Load( cRes.GetResourceStream( "icon48x48.png" ) );
	SetIcon( pcAppIcon->LockBitmap() );
	delete( pcAppIcon );
	
	m_FileName = new String(file);
	m_LastSize = -1;

	Tail();

	AddTimer(this, 10, 1000000, false);
	// Tried using NodeMonitor, but it doesn't trigger until the file
	// is closed/opened or touched somehow. Just keeping it open and
	// writing doesn't trigger the node monitor, and most log files
	// are handled just like that...
}

TailWin::~TailWin()
{
}

bool TailWin::OkToQuit(void)
{
	Application::GetInstance()->PostMessage(M_QUIT);
	return true;
}

void TailWin::Tail(void)
{
	struct stat FileStat;

	if(!stat(m_FileName->c_str(), &FileStat)) {
		if(FileStat.st_mode & S_IFREG) {
			if(m_LastSize == -1) {
				// First time
				m_LastSize = FileStat.st_size;
			} else if(m_LastSize < FileStat.st_size) {
				FILE *fp = fopen(m_FileName->c_str(), "r");

				if(fp) {
					char bfr[256];
					char *p;

					fseek(fp, m_LastSize, SEEK_SET);

					do {
						if((p=fgets(bfr, sizeof(bfr), fp))) {
							m_TextView->Insert(bfr);

							// Force a linebreak if this line is longer than the buffer
							// (looks better, if some buggy program is writing to the log
							// and not using linefeeds...)
							if(strchr(bfr, '\n') == NULL) {
								m_TextView->Insert("\n");
							}
						}
					} while(p);
					fclose(fp);
				}

				m_LastSize = FileStat.st_size;
			}
		} else {
			m_TextView->Insert(STR_ERROR_FILE_SIZE.c_str());
		}
	} else {
		m_TextView->Insert(STR_ERROR_FILE_SIZE.c_str());
	}
}


void TailWin::Save(const char *cFileName)	// From AEdit
{
	static std::string cBuffer;
	TextView::buffer_type vLines;
   
	int iLineNo, iLineTotal=0;

	vLines=m_TextView->GetBuffer();
	iLineTotal=vLines.size();

	ofstream foutfile(cFileName);
   
	if(foutfile){
		for(iLineNo=0;iLineNo<iLineTotal;iLineNo++){
			cBuffer=m_TextView->GetBuffer()[iLineNo];
			foutfile << cBuffer << endl;
		}
	}

	else{
		//Open an error window

		Alert* sFileAlert = new Alert(STR_ERROR_SAVEFILE_TITLE, STR_ERROR_SAVEFILE_TEXT, 0x00, STR_ERROR_SAVEFILE_OKBUTTON.c_str(), NULL);
		sFileAlert->Go();
	}

	foutfile.close();

}






