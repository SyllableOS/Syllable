/*	libcodeview.so - A programmers editor widget for Atheos
	Copyright (c) Andreas Engh-Halstvedt

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

#ifndef _INNEREDIT_H_
#define _INNEREDIT_H_

#include <vector>

#include <gui/view.h>
#include <gui/control.h>
#include <gui/bitmap.h>
#include <gui/menu.h>
#include <util/looper.h>
#include <util/message.h>

#include "format.h"

using namespace std;

namespace cv
{

class InnerEdit;

/**
 * \internal
 */
class Line
{
public:
	os::String		text;		/** Text on this line */
	os::String		style;		/** Style numbers for each character on this line */
	CodeViewContext cookie;		/** Used by the formatters */
	float		w;		/** ? */

	Line(): cookie(0), w(0){}
};

typedef vector<Line> buffer_type;

/**
 * \internal
 */
class UndoNode
{
public:
	enum {
		ADDED,
		REMOVED,
		SET_LINE
	};
	
	os::String		text;		/** The text that changed */
	uint		mode,		/** ADDED, REMOVED or SET_LINE */
			x,		/** Column that was affected */
			y;		/** Row that was affected */

	UndoNode	*next,		/** Link to next UndoNode */
			*previous;	/** Link to previous UndoNode */

	UndoNode() {
		next = 0;
		previous = 0;
	}
};

/**
 * \internal
 */
class Fold
{
public:
	uint32		nStart;		/** First line in the fold section */
	uint32		nEnd;		/** Last hidden line */

	Fold( uint32 s, uint32 e ) {
		nStart = s;
		nEnd = e;
	}
};

/**
 * \internal
 */
class InnerEdit : public os::View
{
	friend class CodeView;
private:
	
	buffer_type	buffer;
	mutable list<Fold>	cFoldedSections;
	/* Line index = visible line number */
	/* Buffer index = index in buffer */
	uint		_TranslateBufferIndex( uint32 nLineIndex ) const;
	uint 		_TranslateLineNumber( uint32 nBufferIndex ) const;
	void		_FoldSection( uint32 nStart, uint32 nEnd );
	void		_UnfoldSection( uint32 nStart );
	uint		_LineIsFolded( uint32 nBufferIndex );
	void		_AdjustFoldedSections( uint nStart, int nLen );

	os::Control*	control;
	uint32		eventMask;
	uint32		eventBuffer;

	bool		mousePressed;
	bool		IcursorActive;

	Format*		format;

	os::Bitmap*	backBM;
	os::View*	backView;
	void		UpdateBackBuffer();

	float		maxWidth;
	float		lineHeight;
	float		lineBase;
	float		spaceWidth;

	float		cursorX; // horisontal pixel pos, NOT char index!
	uint		cursorY; // line number cursor is in
	float		old_cursorX;
	uint		old_cursorY;
	uint		tabSize;
	bool		useTab;
	uint		m_nMargin;	// Margin in pixels (for line numbers)
	
	float		GetW(uint x, uint y) const;
	uint		GetChar(float x, uint line) const;

	bool		selectionValid;
	os::IPoint	selStart,
			selEnd;

	void		UpdateScrollbars();
	void		UpdateWidth(uint);
	void		UpdateAllWidths();
	void		InvalidateLines(int, int);

	void		SplitLine(vector<os::String>&, const os::String&, const char splitter='\n');

	void		PreMove(bool);
	void		PostMove(bool);

	void		Reformat(uint first, uint last);
	
	void		IndentSelection(bool unindent);

	bool		enabled;
	bool		readOnly;

	void		AddUndoNode(uint mode, const os::String &str, uint x, uint y);
	uint		maxUndo;
	uint		undoCount;
	uint		redoCount;
	UndoNode	*undoHead,
			*undoTail,
			*undoCurrent;

	os::Menu* m_pcContextMenu;
	os::Color32_s sHighlight;
	os::Color32_s m_sLineNumberFg;
	os::Color32_s m_sLineNumberBg;
	os::Color32_s m_sLineBackColor;

public:

	InnerEdit(os::Control* c);
	~InnerEdit();

	void SetText(const os::String &);

	void GetText(os::String*, uint startx, uint starty, uint endx, uint endy) const;
	void GetText(os::String *str, const os::IPoint &p0, const os::IPoint &p1) const{
		GetText(str, p0.x, p0.y, p1.x, p1.y);
	}
	void InsertText(const os::String &, uint x, uint y, bool addUndo=true);
	void InsertText(const os::String &str, const os::IPoint &p){
		InsertText(str, p.x, p.y);
	}
	void RemoveText(uint startx, uint starty, uint startx, uint starty, bool addUndo=true);
	void RemoveText(const os::IPoint &p0, const os::IPoint &p1){
		RemoveText(p0.x, p0.y, p1.x, p1.y);
	}
	const os::String& GetLine(uint y)const { return buffer[y].text; }
	void SetLine(const os::String &, uint y, bool addUndo=true);
	uint GetLineCount() const{ return buffer.size()-1; }

	void SetFormat(Format *);
	Format* GetFormat() const{ return format; }

	void SetTabSize(uint);
	uint GetTabSize() const{ return tabSize; }
	void SetUseTab(bool b){ useTab=b; }
	bool GetUseTab(){ return useTab; }

	void SetCursor(uint x, uint y, bool select=false);
	void SetCursor(const os::IPoint &p, bool select=false){
		SetCursor(p.x, p.y, select);
	}
	os::IPoint GetCursor() const;
	void ShowCursor();

	void MoveLeft(bool select=false);
	void MoveRight(bool select=false);
	void MoveUp(bool select=false);
	void MoveDown(bool select=false);
	void MoveTop(bool select=false);
	void MoveBottom(bool select=false);
	void MovePageUp(bool select=false);
	void MovePageDown(bool select=false);
	void MoveLineStart(bool select=false);
	void MoveLineEnd(bool select=false);
	void MoveWordLeft(bool select=false);
	void MoveWordRight(bool select=false);

	void ScrollLeft();
	void ScrollRight();
	void ScrollUp();
	void ScrollDown();
	void ScrollPageUp();
	void ScrollPageDown();

	void SetSelectionStart(uint x, uint y);
	void SetSelectionStart(const os::IPoint &p){
		SetSelectionStart(p.x, p.y);
	}
	void SetSelectionEnd(uint x, uint y);
	void SetSelectionEnd(const os::IPoint &p){
		SetSelectionEnd(p.x, p.y);
	}
	void SetSelection(const os::IPoint &p0, const os::IPoint &p1){
		SetSelectionStart(p0.x, p0.y);
		SetSelectionEnd(p1.x, p1.y);
	}
	void SelectAll(){
		SetSelectionStart(0,0);
		SetSelectionEnd(buffer.back().text.size(), buffer.size());
	}
	void ClearSelection();

	void Copy() const;
	void Cut();
	void Paste();
	void Del();

	void SetEnable(bool b);
	bool GetEnable(){ return enabled; }
	void SetReadOnly(bool);
	bool GetReadOnly() { return readOnly; }

	void SetShowLineNumbers( bool bShowLineNumbers );
	bool GetShowLineNumbers( ) { return ( m_nMargin != 0 ); }
	
	size_t GetLength();

	void CommitEvents();
	void SetEventMask( uint32 nMask ) { eventMask = nMask; }
	uint32 GetEventMask() { return eventMask; }

	void SetMaxUndoSize(int i);
	int GetMaxUndoSize();
	bool Undo();
	bool Redo();
	bool UndoAvailable();
	bool RedoAvailable();
	void ClearUndo();

	virtual void Paint(const os::Rect&);
	virtual void FrameSized(const os::Point &);
	virtual void Activated(bool);
	virtual void KeyDown(const char *, const char *, uint32);
	virtual void FontChanged(os::Font *);
	virtual void MouseDown(const os::Point &, uint32);
	virtual void MouseUp(const os::Point &, uint32, os::Message*);
	virtual void MouseMove(const os::Point &, int, uint32, os::Message*);
	virtual void WheelMoved(const os::Point&);

	void FoldSection( uint nFirst, uint nLast );

	void SetContextMenu( os::Menu* );
	os::Menu* GetContextMenu()
	{
		return m_pcContextMenu;
	}

	void SetHighlightColor(os::Color32_s sHigh)
	{
		sHighlight = sHigh;
	}

	os::Color32_s GetHighlightColor()
	{
		return sHighlight;
	}

	void SetLineNumberBgColor(os::Color32_s sColor)
	{
		m_sLineNumberBg = sColor;
	}

	os::Color32_s GetLineNumberBgColor()
	{
		return m_sLineNumberBg;
	}

	void SetLineNumberFgColor(os::Color32_s sColor)
	{
		m_sLineNumberFg = sColor;
	}

	os::Color32_s GetLineNumberFgColor()
	{
		return m_sLineNumberFg;
	}
	
	void SetLineBackColor(os::Color32_s sColor)
	{
		m_sLineBackColor = sColor;
	}
	
	os::Color32_s GetLineBackColor()
	{
		return m_sLineBackColor;
	}
};

} /* namespace cv */

#endif

