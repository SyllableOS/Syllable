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
	string		text;		/** Text on this line */
	string		style;		/** Style numbers for each character on this line */
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
	
	string		text;		/** The text that changed */
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
	void		updateBackBuffer();

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
	
	float		getW(uint x, uint y) const;
	uint		getChar(float x, uint line) const;

	bool		selectionValid;
	os::IPoint	selStart,
			selEnd;

	void		updateScrollBars();
	void		updateWidth(uint);
	void		updateAllWidths();
	void		invalidateLines(int, int);

	void		splitLine(vector<string>&, const string&, const char splitter='\n');

	void		preMove(bool);
	void		postMove(bool);

	void		reformat(uint first, uint last);
	
	void		indentSelection(bool unindent);

	bool		enabled;
	bool		readOnly;

	void		addUndoNode(uint mode, const string &str, uint x, uint y);
	uint		maxUndo;
	uint		undoCount;
	uint		redoCount;
	UndoNode	*undoHead,
			*undoTail,
			*undoCurrent;
public:

	InnerEdit(os::Control* c);
	~InnerEdit();

	void setText(const string &);

	void getText(string*, uint startx, uint starty, uint endx, uint endy) const;
	void getText(string *str, const os::IPoint &p0, const os::IPoint &p1) const{
		getText(str, p0.x, p0.y, p1.x, p1.y);
	}
	void insertText(const string &, uint x, uint y, bool addUndo=true);
	void insertText(const string &str, const os::IPoint &p){
		insertText(str, p.x, p.y);
	}
	void removeText(uint startx, uint starty, uint startx, uint starty, bool addUndo=true);
	void removeText(const os::IPoint &p0, const os::IPoint &p1){
		removeText(p0.x, p0.y, p1.x, p1.y);
	}
	const string& getLine(uint y)const { return buffer[y].text; }
	void setLine(const string &, uint y, bool addUndo=true);
	uint getLineCount() const{ return buffer.size()-1; }

	void setFormat(Format *);
	Format* getFormat() const{ return format; }

	void setTabSize(uint);
	uint getTabSize() const{ return tabSize; }
	void setUseTab(bool b){ useTab=b; }
	bool getUseTab(){ return useTab; }

	void setCursor(uint x, uint y, bool select=false);
	void setCursor(const os::IPoint &p, bool select=false){
		setCursor(p.x, p.y, select);
	}
	os::IPoint getCursor() const;
	void showCursor();

	void moveLeft(bool select=false);
	void moveRight(bool select=false);
	void moveUp(bool select=false);
	void moveDown(bool select=false);
	void moveTop(bool select=false);
	void moveBottom(bool select=false);
	void movePageUp(bool select=false);
	void movePageDown(bool select=false);
	void moveLineStart(bool select=false);
	void moveLineEnd(bool select=false);
	void moveWordLeft(bool select=false);
	void moveWordRight(bool select=false);

	void scrollLeft();
	void scrollRight();
	void scrollUp();
	void scrollDown();
	void scrollPageUp();
	void scrollPageDown();

	void setSelectionStart(uint x, uint y);
	void setSelectionStart(const os::IPoint &p){
		setSelectionStart(p.x, p.y);
	}
	void setSelectionEnd(uint x, uint y);
	void setSelectionEnd(const os::IPoint &p){
		setSelectionEnd(p.x, p.y);
	}
	void setSelection(const os::IPoint &p0, const os::IPoint &p1){
		setSelectionStart(p0.x, p0.y);
		setSelectionEnd(p1.x, p1.y);
	}
	void selectAll(){
		setSelectionStart(0,0);
		setSelectionEnd(buffer.back().text.size(), buffer.size());
	}
	void clearSelection();

	void copy() const;
	void cut();
	void paste();
	void del();

	void setEnable(bool b);
	bool getEnable(){ return enabled; }
	void setReadOnly(bool);
	bool GetReadOnly() { return readOnly; }
	
	size_t getLength();

	void commitEvents();
	void SetEventMask( uint32 nMask ) { eventMask = nMask; }
	uint32 GetEventMask() { return eventMask; }

	void setMaxUndoSize(int i);
	int getMaxUndoSize();
	bool undo();
	bool redo();
	bool undoAvailable();
	bool redoAvailable();
	void clearUndo();

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
};

} /* namespace cv */

#endif
