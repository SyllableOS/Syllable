/*	libcodeview.so - A programmers editor widget for Atheos
	Copyright (c) 2001 Andreas Engh-Halstvedt
	Copyright (c) 2003 Henrik Isaksson

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Library General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.
	
	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Library General Public License for more details.
	
	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free
	Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
	MA 02111-1307, USA
*/

#ifndef F_CODEVIEW_CODEVIEW_H
#define F_CODEVIEW_CODEVIEW_H

#include <gui/control.h>
#include <gui/menu.h>

namespace cv
{

class InnerEdit;
class Format;
class os::ScrollBar;

/**A programmers editor widget for Atheos.
 * The widget is written to be so similar to the native os::TextView
 * widget in Atheos that switching to it should require few changes.
 * The differences are in three groups:
 * <ul><li>Not yet implemented methods. Not all methods are implemented
 * in this version of CodeView. They will probably come later.
 * <li>Impractical methods. Some methods in os::TextView does not map
 * easily onto CodeView because of different internal strucures. This
 * mainly applies to the os::TextView::GetBuffer() method. 
 * <li>Additional methods. The reason for writing this widget was to
 * implement additional functionality over what is available in 
 * os::TextView. Many of these new features are represented through
 * new methods.
 * </ul>
 * \sa GetLine() SetLine() GetLineCount() \n
 * SetFormat() \n
 * Undo() Redo() UndoAvailable() RedoAvailable() \n
 * SetTabSize() GetTabSize()
 * \author Andreas Engh-Halstvedt (aeh@operamail.com)
 */
class CodeView : public os::Control{
public:
    enum {
	EI_CONTENT_CHANGED   = 0x0001,
	EI_ENTER_PRESSED     = 0x0002,
	EI_ESC_PRESSED       = 0x0004,
	EI_FOCUS_LOST        = 0x0008,
	EI_CURSOR_MOVED      = 0x0010,
	EI_SELECTION_CHANGED = 0x0020
    };

public:
    CodeView( const os::Rect&, const os::String& name, const os::String& buffer, 
	uint32 resizeMask=os::CF_FOLLOW_LEFT||os::CF_FOLLOW_TOP, 
	uint32 flags=os::WID_WILL_DRAW || os::WID_FULL_UPDATE_ON_RESIZE );

    void Paint( const os::Rect& );

    //******************************
    //  TextView emulation methods
    //******************************
    void EnableStatusChanged(bool enable);
    void FontChanged(os::Font *f);
		
    virtual void MakeFocus(bool =true);
    virtual bool HasFocus();
		
    bool Invoked(os::Message*);

    void SetReadOnly(bool flag=true);
    bool GetReadOnly() const;

    int GetMaxUndoSize() const;
    void SetMaxUndoSize(int size);

    uint32 GetEventMask() const;
    void SetEventMask(uint32 mask);

    void GetRegion(os::String *buffer);

    void MakeCsrVisible();
    void Clear(bool sendNotify=true);
    void Set(const char *text, bool sendNotify=true);
    void Insert(const char *text, bool sendNotify=true);
    void Insert(const os::IPoint &pos, const char *text, bool sendNotify=true);
			
    void Select(const os::IPoint &start, const os::IPoint &end, bool sendNotify=true);
    void SelectAll(bool sendNotify=true);
    void ClearSelection(bool sendNotify=true);

    void SetCursor(int x, int y, bool select=false, bool sendNotify=true);
    void SetCursor(const os::IPoint &pos, bool select=false, bool sendNotify=true);
    os::IPoint GetCursor() const;

    size_t GetCurrentLength() const;
		
    void Cut(bool sendNotify=true);
    void Copy();
    void Paste(bool sendNotify=true);
    void Delete(bool sendNotify=true);
    void Delete(const os::IPoint &start, const os::IPoint &end, bool sendNotify=true);
		
    void SetTabOrder(int order);
		
    //******************************
    //  CodeView specific methods
    //******************************
    void SetFormat(Format* f);
    Format* GetFormat();
		
    void SetTextColor(os::Color32_s);
    os::Color32_s GetTextColor();
    void SetBackground(os::Color32_s);
    os::Color32_s GetBackground();

	void SetShowLineNumbers( bool bShowLN );
	bool GetShowLineNumbers();

    uint GetLineCount() const;
    const os::String& GetLine(uint i) const;
    void SetLine(const os::String &text, uint i);

    bool Undo(bool sendNotify=true);
    bool Redo(bool sendNotify=true);
    bool UndoAvailable() const;
    bool RedoAvailable() const;

    void SetTabSize(uint size);
    uint GetTabSize();
    void SetUseTab(bool);
    bool GetUseTab();

    void ClearEvents();

	void SetHighlightColor(os::Color32_s);
	os::Color32_s GetHighlightColor();

	void SetLineNumberFgColor(os::Color32_s);
	os::Color32_s GetLineNumberFgColor();

	void SetLineNumberBgColor(os::Color32_s);
	os::Color32_s GetLineNumberBgColor();

	void SetLineBackColor(os::Color32_s);
	os::Color32_s GetLineBackColor();
	
	os::Font* GetEditorFont() const;
	void SetContextMenu( os::Menu * pcMenu );
	
	void FoldAll();

	virtual void MouseUp(const os::Point&, uint32, os::Message*);

private:
    InnerEdit* edit;
    os::ScrollBar *vScroll, *hScroll;
};

} /* namespace cv */

#endif /* F_CODEVIEW_CODEVIEW_H */



