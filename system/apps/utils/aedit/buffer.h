// AEdit -:-  (C)opyright 2004 Jonas Jarvoll
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

#ifndef __BUFFER_H_
#define __BUFFER_H_

#include <string.h>
#include <list.h>
#include "editview.h"
#include "appwindow.h"

using namespace os;

void SetCurrentTab(int iSel);
bool AnyBufferNeedsToBeSaved(void);
bool MenuItemSaveAllShallBeActive(void);
bool SaveAllBuffers(void);
int CheckIfAlreadyOpened(std::string name);

class Buffer
{
	public:
		Buffer(AEditWindow* pcTar, char* pFileName, char* pFileBuffer);
		~Buffer();
		EditView* GetEditView(void) {return pcEditView;};
	
		std::string &GetNameOfBuffer(void);
		std::string &GetRealName(void);
		std::string &GetTabNameOfBuffer(void);
		bool NeedToSave(void) {return bContentChanged;};
		bool SaveStatus(void);
		void MakeCurrent(void);
		void Cut(void) {pcEditView->Cut();};
		void Copy(void) {pcEditView->Copy();};
		void Paste(void) {pcEditView->Paste();};
		void SelectAll(void) {pcEditView->SelectAll(); pcEditView->Invalidate(); pcEditView->Flush(); };
		void Undo(void) {pcEditView->Undo();};
		void Redo(void) {pcEditView->Redo();};
		void Invoked(int32 nEvent);
		void ContentChanged(bool iContent);
		bool Save(void);
		bool SaveAs(const char* pFileName);
		void GotoLine(int32 iLineNo);
		void MakeFocus(void);
		void SetFontFamily(const char* cFam, const char* cStyle, const uint32 flags);
		void SetFontSize(float size);
		bool FindFirst(std::string zFindText, bool bCaseSensitive);
		std::string &FindNext(void);
		bool Replace(std::string zFText, std::string zRText);
		void ForceRedraw(void);
		void SetFont( Font* font );

		friend void SetCurrentTab(int iSel);
		friend bool AnyBufferNeedsToBeSaved(void);
		friend bool MenuItemSaveAllShallBeActive(void);
		friend bool SaveAllBuffers(void);
		friend int CheckIfAlreadyOpened(std::string name);

	private:

		EditView* pcEditView;

		// Path to where file is stored
		std::string zFilename;
		
		// My new buffer number
		int iEmptyBufferNumber;

		// Set to true if the content in the buffer has changed since last save
		bool bContentChanged;

		// Font information
		std::string zFamily;				// Font family currently selected
		std::string zStyle;					// Font style currently selected
		float vSize;						// Font size currently selectes
		uint32 iFlags; 						// Font flags

		// Parameters for Find and Replace function
		std::string zFindText;				// All used for find & replace
		std::string zReplaceText;
		int32 nTextPosition,nCurrentLine, nTotalLines;
		bool bHasFind;
		bool bCaseSensitive;

	protected:
		// Counter to keep track of the number of new buffers
		static int iEmptyBufferCounter;
		
		// Keep track of all opened buffer
		static std::list<Buffer *> pcBufferList;

		// For easy reference
		static AEditWindow* pcTarget;
};

#endif

