// AEdit -:-  (C)opyright 2004 - 2006 Jonas Jarvoll
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

#include <list>
#include <fstream.h>
#include <string>

using namespace std;

#include "buffer.h"
#include "messages.h"
#include "editview.h"
#include "version.h"
#include "appwindow.h"
#include "resources/aedit.h"

// Declare some static variables
int Buffer::iEmptyBufferCounter=0;
std::list<Buffer *> Buffer::pcBufferList;
AEditWindow* Buffer::pcTarget=NULL;

Buffer::Buffer(AEditWindow* pcTar, char* pFileName, char* pFileBuffer)
{
	// One more new buffer if no specified filename
	if(pFileName==NULL) 
		iEmptyBufferCounter++;

	// Set target
	pcTarget=pcTar;

	// Create EditView for this buffer
	pcEditView=new EditView(Rect(0,0,0,0), pcTar);
	pcEditView->SetTarget(pcTarget);
	pcEditView->SetMultiLine(true);

	// Set buffer if we have a buffer
	if(pFileBuffer!=NULL)
	{
		pcEditView->Clear(true);		
		pcEditView->Set(pFileBuffer, true);
	}

	pcEditView->SetMessage(new Message(M_EDITOR_INVOKED));
	pcEditView->SetEventMask(TextView::EI_CONTENT_CHANGED | TextView::EI_CURSOR_MOVED);

    // Get Default defined font
	font_properties fp;
	Font::GetDefaultFont(DEFAULT_FONT_FIXED, &fp);
	Font *f=new Font(fp);
	pcEditView->SetFont(f);

	// Set up variables
	vSize=fp.m_vSize;
	zFamily=fp.m_cFamily;
	zStyle=fp.m_cStyle;
	iFlags=fp.m_nFlags;

	f->Release();

	// Make sure this EditView has the focus
	pcEditView->MakeFocus();

	// Set filename
	if(pFileName==NULL)
		zFilename="";
	else
		zFilename=pFileName;

	// Reset the change flag
	bContentChanged=false;

	// No find has been done
	bHasFind=false;

	// Since this is a new buffer we need to remember the EmptyBufferNumber
	// This number is used to create a nice name of the buffer
	iEmptyBufferNumber=iEmptyBufferCounter;

	// Add this buffer to list of buffers
	pcBufferList.push_back(this);

	// Make this the current one
	MakeCurrent();
}


Buffer::~Buffer()
{
	std::list<Buffer *>::iterator iter;
	int icounter;

	// Is this buffer the current one?
	if(pcTarget->pcCurrentBuffer==this)
		pcTarget->pcCurrentBuffer=NULL;

	// Remove buffer from list
	for(iter=pcBufferList.begin(), icounter=0; iter!=pcBufferList.end(); iter++, icounter++)
	{
		if(*iter==this)
		{
			// remove tab
			pcTarget->pcMyTabView->DeleteTab(icounter);
			// remove buffer from list
			pcBufferList.erase(iter);
			break;
		}
	}
	
	// Set current buffer
	if(pcTarget->pcMyTabView->GetTabCount()>0)
		SetCurrentTab(pcTarget->pcMyTabView->GetSelection());
	else  // No more open document
	{
		// Set the window title etc.
		pcTarget->SetTitle(AEDIT_RELEASE_STRING);
	}
	
	delete pcEditView;
}

// Get real name of buffer (without a asterix)
std::string &Buffer::GetRealName(void)
{
	static std::string zTitle2;
	static char zTitle[32];

	// Check if a filename already exists
	if(zFilename!="")
		return zFilename;

	// Otherwise we create one (this must be done in a nicer way how??)
	sprintf(zTitle, MSG_BUFFER_UNTITLED.c_str(), iEmptyBufferNumber);
	zTitle2=zTitle;

	return zTitle2;
}


// Get the name of the buffer
// Normal cases it will return the path to the file
// if no file exits it will return "Noname"
std::string &Buffer::GetNameOfBuffer(void)
{
	static std::string s;

	if(bContentChanged)
	{
		s="*";
		s+=GetRealName();
		return s;
	}
	else
		return GetRealName();
}

// Gets the filename of the buffer
// Will delete the path from the name
std::string &Buffer::GetTabNameOfBuffer(void)
{
	std::string s;
	static std::string ret;
	unsigned int i;

	s=GetRealName();
	i=s.find_last_of("/");

	// clear string
	ret.assign("");

	// Add a star if the buffer has bene modifed
	if(bContentChanged)
		ret.assign("*");

	// Append the name of the buffer
	if(i==string::npos)
		ret.append(s);
	else
		ret.append(s, i+1, s.size());

	return ret;
}


// Make this buffer the current one
void Buffer::MakeCurrent(void)
{
	pcTarget->pcCurrentBuffer=this;

	// Change appwindow title
	std::string zWindowTitle=AEDIT_RELEASE_STRING;
	zWindowTitle+=" : ";
	zWindowTitle+=GetNameOfBuffer();
	pcTarget->SetTitle(zWindowTitle);

	// Since we have changed the current buffer the find is no longer valid
	bHasFind=false;
}

// Content or cursor changed
void Buffer::Invoked(int32 nEvent)
{
	// The content in the TextView has been changed
	if(!bContentChanged && (nEvent & TextView::EI_CONTENT_CHANGED))
		ContentChanged(true);

	// Update cursor location
	if(nEvent & TextView::EI_CURSOR_MOVED)
	{
		int x, y;

		// Get the cursor position in the TextView
		pcEditView->GetCursor(&x, &y);

		// We start counting from 1 and not from 0
		x++;
		y++;

		// And then make the StatusBar update
		pcTarget->SetStatus( MSG_BUFFER_CURSOR, x, y );
	}

}

// Returns true if buffer has ben modified and there is a proper filename
// for the buffer
bool Buffer::SaveStatus(void)
{
	return (bContentChanged && zFilename!="");
}


// Content has changed
void Buffer::ContentChanged(bool iContent)
{
	std::list<Buffer *>::iterator iter;
	int icounter;

	bContentChanged=iContent;
	
	// Change name of tab by go through list to find matching buffer
	for(iter=pcTarget->pcCurrentBuffer->pcBufferList.begin(), icounter=0; iter!=pcTarget->pcCurrentBuffer->pcBufferList.end(); iter++, icounter++)
	{
		if(*iter==this)
			pcTarget->pcMyTabView->SetTabTitle(icounter, GetTabNameOfBuffer());
	}
	
	// Change name of window if it is the current one
	if(pcTarget->pcCurrentBuffer==this)
		MakeCurrent();

	// Also update the "Save" menu
	pcTarget->UpdateMenuItemStatus();
}

// Tries to save the buffer. Returns FALSE is anything wrong
bool Buffer::Save(void)
{
	return SaveAs( zFilename.c_str() );
}

bool Buffer::SaveAs(const char* pFileName)
{
	bool bSuccess;

	try
	{
		File cFile( pFileName, O_TRUNC | O_CREAT | O_WRONLY );

		uint32 nLineCount = pcEditView->GetLineCount();
		for( uint32 n = 0; n < nLineCount; n++ )
		{
			string cLine;
			if( n > 0 ) {
				cLine = "\n" + pcEditView->GetLine( n );
			} else {
				cLine = pcEditView->GetLine( n );
			}
			cFile.Write( cLine.c_str(), cLine.length() );
		}

		cFile.Flush();

		// Change the name of the buffer
		zFilename = pFileName;

		ContentChanged(false);
		bSuccess = true;
	}
	catch( exception &e )
	{
		cerr << "Error saving \"" << pFileName << "\": %s" << e.what() << endl;
		bSuccess = false;
	}

	return bSuccess;
}

// Move the cursor to a specified line number
void Buffer::GotoLine(int32 iLineNo)
{
	pcEditView->SetCursor(0,(iLineNo-1),false,false);
}

// Make sure TextView has the focus
void Buffer::MakeFocus(void)
{
	pcEditView->MakeFocus();
}

void Buffer :: SetFont( Font* font )
{
	pcEditView->SetFont( font );
}

// Changes the font in the TextView
void Buffer::SetFontFamily(const char* cFam, const char* cStyle, const uint32 iF)
{
	Font *font = new Font();

	font->SetFamilyAndStyle(cFam, cStyle);
	font->SetSize(vSize);

	pcEditView->SetFont(font);

	zFamily=cFam;
	zStyle=cStyle;
	iFlags=iF;
	
	font->Release();
}

void Buffer::SetFontSize(float size)
{
	Font *font = new Font();

	font->SetFamilyAndStyle(zFamily.c_str(), zStyle.c_str());
	font->SetSize(size);
	font->SetFlags(iFlags);

	pcEditView->SetFont(font);

	vSize=size;
	
	font->Release();
}

bool Buffer::FindFirst(std::string zText, bool bCaseSens)
{
	// Setup buffer variables
	bHasFind=false;
	zFindText=zText;
	bCaseSensitive=bCaseSens;

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

	return bHasFind;
}

std::string &Buffer::FindNext(void)
{
	static std::string junk="";

	if(bHasFind)
	{
		nTextPosition+=zFindText.size();	// We want to start the search from just past the last instance.
					 						// There is a lot of fudging in this block of code :)

		for(;nCurrentLine<nTotalLines;nCurrentLine++)
		{
			nTextPosition=pcEditView->Find(pcEditView->GetLine(nCurrentLine), zFindText, bCaseSensitive, nTextPosition);

			// We have found something here!
			if(nTextPosition!=-1)
			{
				pcEditView->SetCursor(nTextPosition,nCurrentLine, false, false);
				pcEditView->SetCursor((nTextPosition+zFindText.size()),nCurrentLine, true, false);

				return zFindText;
			}
			else
			{
				nTextPosition=0;		// This is a big fudge (See the call to EditText::Find() above!)
			}
		}	// End of for() loop
	}	// End of if()

	return junk;
}

bool Buffer::Replace(std::string zFText,std::string zRText)
{
	zFindText=zFText;
	zReplaceText=zRText;

	if(bHasFind)
	{
		pcEditView->Delete();
		pcEditView->Insert(zReplaceText.c_str(),false);

		return true;
	}	// End of outer if()

	return false;
}

// Forces a redraw of the EditView
void Buffer::ForceRedraw(void)
{
	pcEditView->Invalidate(true);
	pcEditView->Sync();
}

///////////////////////////////////////////////////////////////////
// Friends: 
///////////////////////////////////////////////////////////////////
void SetCurrentTab(int iSel)
{
	std::list<Buffer *>::iterator iter;
	int icounter;

	// If we do not have any more buffers

	// Go throught list to find matching buffer
	for(iter=Buffer::pcTarget->pcCurrentBuffer->pcBufferList.begin(), icounter=0; iter!=Buffer::pcTarget->pcCurrentBuffer->pcBufferList.end(); iter++, icounter++)
	{
		if(icounter==iSel)
		{
			(*iter)->MakeCurrent();
			return;	
		}
	}
}

// Returns true if any buffer has modified context and therefor needs to be saved
bool AnyBufferNeedsToBeSaved(void)
{
	std::list<Buffer *>::iterator iter;

	// Go throught list to find matching buffer
	for(iter=Buffer::pcTarget->pcCurrentBuffer->pcBufferList.begin(); iter!=Buffer::pcTarget->pcCurrentBuffer->pcBufferList.end(); iter++)
	{
		if((*iter)->NeedToSave())
			return true;
	}

	return false;
}

// Returns true if Save All menu item should be active
// true - if there is modified buffers and those has a real filename (ie. not "untitled");
bool MenuItemSaveAllShallBeActive(void)
{
	std::list<Buffer *>::iterator iter;

	// Go throught list to find matching buffer
	for(iter=Buffer::pcTarget->pcCurrentBuffer->pcBufferList.begin(); iter!=Buffer::pcTarget->pcCurrentBuffer->pcBufferList.end(); iter++)
	{
		if((*iter)->SaveStatus())
			return true;
	}

	return false;
}

// Save all buffers which has been modified and have a real filename
// returns true if successful
bool SaveAllBuffers(void)
{
	bool ret=true;
	std::list<Buffer *>::iterator iter;

	// Go throught list to find matching buffer
	for(iter=Buffer::pcTarget->pcCurrentBuffer->pcBufferList.begin(); iter!=Buffer::pcTarget->pcCurrentBuffer->pcBufferList.end(); iter++)
	{
		if(! ((*iter)->Save()))
			ret=false;
	}

	return ret;
}

// Returns the buffer with the name
int CheckIfAlreadyOpened(std::string name)
{
	int r=0;

	std::list<Buffer *>::iterator iter;

	// Go throught list to find matching buffer
	for(iter=Buffer::pcTarget->pcCurrentBuffer->pcBufferList.begin(); iter!=Buffer::pcTarget->pcCurrentBuffer->pcBufferList.end(); iter++, r++)
	{
		if((*iter)->GetRealName()==name)
			return r;
	}

	// Buffer was not found
	return -1;
}
