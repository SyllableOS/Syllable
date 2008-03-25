/*	libcodeview.so - A programmers editor widget for Atheos
	Copyright (c) 2001 Andreas Engh-Halstvedt
	Copyright (c) 2003 Henrik Isaksson

	Some code copied from:
		libatheos.so - the highlevel API library for AtheOS
		Copyright (C) 1999 - 2001 Kurt Skauen


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

#include "inneredit.h"
#include "codeview.h"

#include <assert.h>

#include <gui/font.h>
#include <gui/scrollbar.h>
#include <util/clipboard.h>
#include <util/application.h>

using namespace cv;

#define POINTER_WIDTH  7
#define POINTER_HEIGHT 14
static uint8 mouseImg[]=
{
    0x02,0x02,0x02,0x00,0x02,0x02,0x02,
    0x00,0x00,0x00,0x02,0x00,0x00,0x00,
    0x00,0x00,0x00,0x02,0x00,0x00,0x00,
    0x00,0x00,0x00,0x02,0x00,0x00,0x00,
    0x00,0x00,0x00,0x02,0x00,0x00,0x00,
    0x00,0x00,0x00,0x02,0x00,0x00,0x00,
    0x00,0x00,0x00,0x02,0x00,0x00,0x00,
    0x00,0x00,0x00,0x02,0x00,0x00,0x00,
    0x00,0x00,0x00,0x02,0x00,0x00,0x00,
    0x00,0x00,0x00,0x02,0x00,0x00,0x00,
    0x00,0x00,0x00,0x02,0x00,0x00,0x00,
    0x00,0x00,0x00,0x02,0x00,0x00,0x00,
    0x00,0x00,0x00,0x02,0x00,0x00,0x00,
    0x02,0x02,0x02,0x00,0x02,0x02,0x02
};



InnerEdit::InnerEdit(os::Control* c) 
: View(os::Rect(), "", os::CF_FOLLOW_ALL, os::WID_WILL_DRAW | os::WID_FULL_UPDATE_ON_H_RESIZE),
control(c), eventMask(0), eventBuffer(0),
mousePressed(false), IcursorActive(false),
format(0), backBM(0), backView(0),
maxWidth(0), cursorX(0), cursorY(0), old_cursorX(0), old_cursorY(0), tabSize(4), useTab(true),
selectionValid(false),
enabled(true), readOnly(false),
maxUndo(1000), undoCount(0), redoCount(0), undoHead(0), undoTail(0), undoCurrent(0),
m_pcContextMenu( NULL ), sHighlight(0,0,0,0)
{
	SetBgColor(255, 255, 255);
	SetFgColor(0, 0, 0);

	m_sLineNumberBg = os::Color32_s( 200, 200, 255 );
	m_sLineNumberFg = os::Color32_s( 0, 0, 0 );
	m_sLineBackColor = os::Color32_s(220,220,250);
	
	os::font_properties fp;
	os::Font::GetDefaultFont(DEFAULT_FONT_FIXED, &fp);
	os::Font *f=new os::Font(fp);
	SetFont(f);
	f->Release();

	buffer.resize(1);

	UpdateBackBuffer();
	UpdateScrollbars();
	
	m_nMargin = 40;
}

InnerEdit::~InnerEdit()
{
	if(backBM)
		delete backBM;

	if(IcursorActive)
		os::Application::GetInstance()->PopCursor();
}

void InnerEdit::SetContextMenu( os::Menu * pcMenu )
{
	m_pcContextMenu = pcMenu;
}

void InnerEdit::SetShowLineNumbers( bool bShowLineNumbers )
{
	if( bShowLineNumbers ) {
		m_nMargin = (int)GetStringWidth( "000000" );
	} else {
		m_nMargin = 0;
	}
}

inline static os::Color32_s invert(os::Color32_s c)
{
	c.red=255-c.red;
	c.green=255-c.green;
	c.blue=255-c.blue;
	return c;
}

inline static os::Color32_s dim(os::Color32_s c)
{
	c.red/=2;
	c.green/=2;
	c.blue/=2;
	return c;
}

inline static bool operator==(const os::Color32_s &a, const os::Color32_s &b)
{
	return a.red==b.red && a.green==b.green && a.blue==b.blue && a.alpha==b.alpha;
}

void InnerEdit::Paint(const os::Rect &r)
{
	uint nTopLine = (int)(r.top/lineHeight);
	uint nBotLine = _TranslateBufferIndex( 1 + (int)( r.bottom / lineHeight ) );

	uint selTop=selStart.y;
	uint selLeft=selStart.x;
	uint selBot=selEnd.y;
	uint selRight=selEnd.x;

	if( selectionValid ) {
		if( selTop > selBot ) {
			uint tmp=selTop;
			selTop=selBot;
			selBot=tmp;
			tmp=selLeft;
			selLeft=selRight;
			selRight=tmp;
		} else if( selTop == selBot && selLeft > selRight ) {
			uint tmp=selLeft;
			selLeft=selRight;
			selRight=tmp;
		}
	}

//	const uint nDigits = log10( buffer.size() ) + 1;
//	const uint nMargin = 40; //( nDigits + 1 ) * backView->GetStringWidth( "0" );
	const float xOff = GetScrollOffset().x;
	const float w = Width();

	uint32 nVisibleLine = nTopLine;
	uint32 nBufferIndex = _TranslateBufferIndex( nVisibleLine );
	float y = nVisibleLine * lineHeight;

	while( nBufferIndex <= nBotLine && nBufferIndex < buffer.size() ) {
		os::String& pcLineText = buffer[ nBufferIndex ].text;
		uint invStart=0, invEnd=0;
		bool bIsFolded = _LineIsFolded( nBufferIndex );

		if( m_nMargin ) {
			backView->FillRect( os::Rect( xOff, 0, xOff + m_nMargin - 1/*was -4*/, lineHeight ),m_sLineBackColor);
			backView->FillRect(os::Rect(m_nMargin-4,0,m_nMargin-2,lineHeight),os::Color32_s(255,255,255));
			backView->FillRect( os::Rect(m_nMargin-2,0,m_nMargin-1,lineHeight ),m_sLineBackColor);
			backView->MovePenTo( xOff, lineBase );
			backView->SetFgColor( m_sLineNumberFg );
			backView->SetBgColor( m_sLineNumberBg );
			if( bIsFolded ) {
				backView->DrawFrame( os::Rect( xOff, 0, xOff + m_nMargin - 1, lineHeight),os::FRAME_TRANSPARENT );
				backView->DrawString( "+" );
			} else {
				os::String cLineNo;
				cLineNo.Format( "%d", nBufferIndex+1 );
				backView->DrawString( cLineNo );
			}
		}

		if(selectionValid){
			if( nVisibleLine == selTop ) {
				invStart = selLeft;
				invEnd = pcLineText.size();
			}
			if( nVisibleLine == selBot )
				invEnd = selRight;
			if( nVisibleLine > selTop && nVisibleLine < selBot )
				invEnd = pcLineText.size();

			float f0=0, f1=0;
			if(invStart>0){
				f0=GetW( invStart, nBufferIndex );
				if(enabled)
					backView->FillRect(os::Rect( m_nMargin + xOff, 0, m_nMargin + xOff + f0, lineHeight), GetBgColor());
				else
					backView->FillRect(os::Rect( m_nMargin + xOff, 0, m_nMargin + xOff + f0, lineHeight), dim(GetBgColor()));
			}
			if(invEnd>0){
				f1=GetW(invEnd, nBufferIndex );				
				if(enabled)
					backView->FillRect(os::Rect( m_nMargin + xOff + f0, 0, m_nMargin + xOff + f1, lineHeight), invert(GetBgColor()));
				else
					backView->FillRect(os::Rect( m_nMargin + xOff + f0, 0, m_nMargin + xOff + f1, lineHeight), dim(invert(GetBgColor())));
			}
			if(xOff+f1<w)
				if(enabled)
					backView->FillRect(os::Rect( m_nMargin + xOff + f1, 0, w, lineHeight), GetBgColor());
				else
					backView->FillRect(os::Rect( m_nMargin + xOff + f1, 0, w, lineHeight), dim(GetBgColor()));
		} else {
			if(enabled) {
				if ( ( nVisibleLine == cursorY ) &&  ( sHighlight.alpha != 0 ) )
					backView->FillRect( os::Rect( m_nMargin + xOff, 0, w, lineHeight ), GetHighlightColor() );
				else
					backView->FillRect( os::Rect( m_nMargin + xOff, 0, w, lineHeight ), GetBgColor() );
			} else {
				backView->FillRect( os::Rect( m_nMargin + xOff, 0, w, lineHeight ), dim(GetBgColor()));
			}
		}

		os::Color32_s fg=GetFgColor();
		os::Color32_s bg=GetBgColor();

		if ( (nVisibleLine == cursorY) && (sHighlight.alpha != 0))
			backView->SetBgColor( GetHighlightColor() );
		else
			backView->SetBgColor(bg);
		
		backView->SetFgColor(fg);

		backView->MovePenTo( m_nMargin + xOff, lineBase );

		for( uint a = 0; a < pcLineText.size(); ) {
			char chr = pcLineText[a];

			if(chr=='\t') {
				backView->Sync();//need this to get correct pen pos
				float tab=spaceWidth*tabSize;
				float x = backView->GetPenPosition().x - xOff - m_nMargin;
				x = tab*ceil((x+1)/tab);
				
				++a;
				while( a < pcLineText.size() && pcLineText[ a ] == '\t' ) {
					++a;
					x+=tab;
				}
				
				backView->MovePenTo( xOff + x + m_nMargin, lineBase );
			} else {
				os::Color32_s nfg, nbg;

				if(a>=invStart && a<invEnd){
					if( format && pcLineText.size() == buffer[ nBufferIndex ].style.size() )
						nfg = invert( format->GetStyle(buffer[ nBufferIndex ].style[a]).sColor );
					else
						nfg=invert(GetFgColor());
					nbg=invert(GetBgColor());
				}else{
					if(format && pcLineText.size()==buffer[ nBufferIndex ].style.size())
						nfg = format->GetStyle(buffer[ nBufferIndex ].style[a]).sColor;
					else
						nfg=GetFgColor();
					nbg=GetBgColor();
				}
				if(!enabled)
					nbg=dim(nbg);

				if(!(nfg==fg)){
					fg=nfg;
					backView->SetFgColor(fg);
				}
				if(!(nbg==bg)){
					bg=nbg;
					backView->SetBgColor(bg);
				}

				int len=os::utf8_char_length(chr);
				backView->DrawString( pcLineText.c_str() + a, len );
				a+=len;
			}

		}
		
		if( bIsFolded ) {
			backView->SetFgColor( os::Color32_s( 200, 50,  50 ) );
			backView->SetBgColor( bg );
			backView->DrawString( " (...)" );
		}

		if( HasFocus() && nVisibleLine == cursorY ) {
			int c = min( GetChar( cursorX, nBufferIndex ), pcLineText.size() );
			float x = GetW( c, nBufferIndex ) + xOff + m_nMargin;
			backView->SetDrawingMode(os::DM_INVERT);
			backView->DrawLine(os::Point(x, 0), os::Point(x, lineHeight));
			x+=1.0f;
			backView->DrawLine(os::Point(x, 0), os::Point(x, lineHeight));
			backView->SetDrawingMode(os::DM_COPY);
		}


		backView->Sync();
		DrawBitmap( backBM, backBM->GetBounds(), os::Rect( -xOff, y, backBM->GetBounds().Width() - xOff, y + lineHeight - 1 ) );
		Sync();
		nBufferIndex = _TranslateBufferIndex( ++nVisibleLine );
		y += lineHeight;
	}

	if( nBufferIndex < nBotLine ) {
		os::Color32_s bg = GetBgColor();
		bg.red ^= 7;
		bg.green ^= 7;
		bg.blue ^= 7;
		FillRect( os::Rect( r.left, nVisibleLine * lineHeight, r.right, r.bottom ), bg );
	}
}

/** Invalidate lines to cause a refresh */
void InnerEdit::InvalidateLines(int start, int stop)
{
	os::Rect r=GetBounds();
	r.top=start*lineHeight;
	r.bottom=(stop+1)*lineHeight;
	Invalidate(r);	
}

/** Set the entire buffer */
void InnerEdit::SetText(const os::String &s)
{
	buffer.clear();
	vector<os::String> list;

	SplitLine(list, s);
	buffer.resize(list.size());

	for(uint a=0;a<list.size();++a){
		buffer[a].text=list[a];
	}
	UpdateAllWidths();

	Reformat(0, buffer.size()-1);

	cursorY=0;
	cursorX=0;

	UpdateScrollbars();
	Invalidate();
	eventBuffer |= CodeView::EI_CONTENT_CHANGED;
	ClearUndo();
}

/** Insert text at a certain location and optionally create an UndoNode */
void InnerEdit::InsertText(const os::String &str, uint x, uint y, bool addUndo)
{
	uint nBufferIndex = _TranslateBufferIndex( y );
	nBufferIndex = min( nBufferIndex, buffer.size() );
	x = min( x, buffer[nBufferIndex].text.size() );

	vector<os::String> list;

	SplitLine(list, str);

	uint cursorChar = GetChar( cursorX, _TranslateBufferIndex( cursorY ) );

	if(addUndo)
		AddUndoNode(UndoNode::ADDED, str, x, y);

	if(list.size()==1){
		buffer[nBufferIndex].text.str().insert(x, str);
	
		if( _TranslateBufferIndex(cursorY) == nBufferIndex && cursorChar >= x )
			cursorChar += list[0].size();

		UpdateWidth(y);
		//_AdjustFoldedSections( nBufferIndex, 1 );
		InvalidateLines(y, y);
	}else{
		os::String tmp=buffer[ nBufferIndex ].text.str().substr(x);
		buffer[nBufferIndex].text.erase(x, ~0);
		buffer[nBufferIndex].text+=list[0];

		if(nBufferIndex==buffer.size()-1){
			for(uint a=1;a<list.size();++a){
				buffer.push_back(Line());
				buffer.back().text=list[a];
			}
		}else{
			buffer_type::iterator iter;
			for(uint a=1;a<list.size();++a){
				//buffer.insert(&buffer[nBufferIndex+a], Line());
				iter = buffer.begin() + nBufferIndex + a;
				buffer.insert(iter, Line());
				buffer[nBufferIndex+a].text=list[a];
			}
		}

		buffer[nBufferIndex+list.size()-1].text+=tmp;
		UpdateAllWidths();

		if( _TranslateBufferIndex(cursorY) == nBufferIndex && cursorChar >= x ) {
			cursorY+=list.size()-1;
			cursorChar+=list.back().size()-x;
		} else if( _TranslateBufferIndex(cursorY) > nBufferIndex ) {
			cursorY+=list.size()-1;
		}

		_AdjustFoldedSections( nBufferIndex, list.size()-1 );

		InvalidateLines( y, buffer.size()-1 );
	}

	cursorX = GetW( cursorChar, _TranslateBufferIndex( cursorY ) );

	Reformat( nBufferIndex, nBufferIndex+list.size() );

	UpdateScrollbars();
	eventBuffer |= CodeView::EI_CONTENT_CHANGED;
}

void InnerEdit::GetText(os::String* str, uint left, uint top, uint right, uint bot) const
{
	if(!str)
		return;

	top = min( _TranslateBufferIndex( top ), buffer.size() - 1 );
	bot = min( _TranslateBufferIndex( bot ), buffer.size()-1);
	left = min( left, buffer[ top ].text.size() );
	right = min( right, buffer[ bot ].text.size() );

	if( top > bot ) {
		uint tmp=top;
		top=bot;
		bot=tmp;
		tmp=left;
		left=right;
		right=tmp;
	} else if(bot==top && left>right) {
		uint tmp=left;
		left=right;
		right=tmp;
	}


	if( top == bot ) {
		*str = buffer[top].text.const_str().substr(left, right-left);
		return;
	}

	*str=buffer[top].text.const_str().substr(left);
	*str+='\n';
	

	for(uint a=top+1;a<bot;++a){
		*str += buffer[a].text;
		*str += '\n';
	}

	*str += buffer[bot].text.const_str().substr(0, right);
}

void InnerEdit::RemoveText( uint nLeft, uint nTop, uint nRight, uint nBot, bool bAddUndo )
{
	uint nBfrTop   = min( _TranslateBufferIndex( nTop ), buffer.size() - 1 );
	uint nBfrBot   = min( _TranslateBufferIndex( nBot ), buffer.size() - 1 );
	nLeft	       = min( nLeft, buffer[ nBfrTop ].text.size() );
	nRight	       = min( nRight, buffer[ nBfrBot ].text.size() );

	if( nBfrTop > nBfrBot ) {
		uint tmp = nBfrTop;	nBfrTop = nBfrBot;	nBfrBot = tmp;
		tmp = nTop;		nTop = nBot;		nBot = tmp;
		tmp = nLeft;		nLeft = nRight;		nRight = tmp;
	} else if( nBfrTop == nBfrBot && nLeft > nRight ) {
		uint tmp = nLeft;	nLeft = nRight;		nRight = tmp;
	}

	uint nCursorChar = GetChar( cursorX, _TranslateBufferIndex( cursorY ) );

	if( nBfrTop == nBfrBot ) {
		if(bAddUndo)
			AddUndoNode( UndoNode::REMOVED,
				buffer[ nBfrTop ].text.str().substr( nLeft, nRight - nLeft), nLeft, nTop);

		buffer[ nBfrTop ].text.erase( nLeft, nRight - nLeft );
		UpdateWidth( nBfrTop );
		
		if( cursorY == nTop ) {
			if( nCursorChar > nRight ) {
				nCursorChar -= nRight - nLeft;
			}else if( nCursorChar > nLeft){
				nCursorChar = nLeft;
			}
		}

	//	_AdjustFoldedSections( nBfrTop, -1 );
		InvalidateLines( nTop, nTop );
	} else {
		os::String cUndoStr;

		if(bAddUndo)
			cUndoStr = buffer[ nBfrTop ].text.str().substr( nLeft ) + "\n";

		buffer[ nBfrTop ].text.erase( nLeft, ~0 );
		if(cursorY == nTop && nCursorChar > nLeft)
			nCursorChar = nLeft;

		buffer[ nBfrTop ].text += buffer[ nBfrBot ].text.str().substr( nRight );

		for( uint a = nBfrTop + 1; a <= nBfrBot; ++a ) {
			if( bAddUndo )
				if( a < nBfrBot )
					cUndoStr += buffer[ nBfrTop + 1 ].text + "\n";
				else
					cUndoStr += buffer[ nBfrTop + 1 ].text.str().substr( 0, nRight );

			//TODO: erase all lines at once - this is slow
			buffer.erase( buffer.begin() + nBfrTop + 1 );
		}
		UpdateAllWidths();
		
		_AdjustFoldedSections( nBfrTop, nBfrTop - nBfrBot );

		if(bAddUndo)
			AddUndoNode(UndoNode::REMOVED, cUndoStr, nLeft, nTop);
		
		if(cursorY==nTop){
			if(nCursorChar>nLeft){
				nCursorChar=nLeft;
			}
		}else if(cursorY==nBot){
			cursorY=nTop;
			if(nCursorChar>nRight){
				nCursorChar+=nLeft-nRight;
			}else{
				nCursorChar=nLeft;
			}
		}else if(cursorY>nBot){//after deleted area
			cursorY-=nBot-nTop;
		}else if(cursorY>nTop){//in deleted area
			cursorY=nTop;
			nCursorChar=nLeft;
		}

		UpdateScrollbars();
		Invalidate();
	}

	cursorX = GetW( nCursorChar, _TranslateBufferIndex( cursorY ) );

	Reformat( nBfrTop, nBfrTop + 1 );
	eventBuffer |= CodeView::EI_CONTENT_CHANGED;
}

/** Adjust font, if it has changed */
void InnerEdit::FontChanged(os::Font* f)
{
	os::font_height fh;
	f->GetHeight(&fh);

	lineBase=fh.ascender+fh.line_gap;
	lineHeight=fh.ascender+fh.line_gap+fh.descender+1;
	spaceWidth=f->GetStringWidth(" ");

	if(f->GetDirection()!=os::FONT_LEFT_TO_RIGHT){
		throw "Cannot use fonts with other directions than left-to-right!";
	}

	UpdateBackBuffer();
	UpdateScrollbars();
	Invalidate();
	Flush();
}

void InnerEdit::SplitLine(vector<os::String> &list, const os::String& line, const char splitter)
{
	uint start=0;
	int end;

	list.clear();

	while(start<=line.size()){
		end=line.const_str().find(splitter, start);
		if(end==-1)
			end=line.size();

		list.push_back(line.const_str().substr(start, end-start));

		start=end+1;
	}
}

void InnerEdit::Activated(bool b)
{
	InvalidateLines(cursorY, cursorY);
	Flush();
	
	if(!b){
		eventBuffer |= CodeView::EI_FOCUS_LOST;
		CommitEvents();
	}
}


void InnerEdit::KeyDown(const char* str, const char* rawstr, uint32 modifiers)
{
	if(!enabled)
		return;

	bool shift=modifiers&os::QUAL_SHIFT;
	bool alt=modifiers&os::QUAL_ALT;
	bool ctrl=modifiers&os::QUAL_CTRL;
	bool dead=modifiers&os::QUAL_DEADKEY;
	uint charY=_TranslateBufferIndex( cursorY );

	if(strlen(rawstr)==1){
		switch(rawstr[0]){
		case ' ':
			if(ctrl && !alt && !shift) {
				if( _LineIsFolded( charY ) ) {
					_UnfoldSection( charY );
					InvalidateLines( cursorY, buffer.size()-1 );
					UpdateScrollbars();
				} else if( selectionValid && selStart.y != selEnd.y ) {
					uint nStart = min( selStart.y, selEnd.y );
					uint nEnd = max( selStart.y, selEnd.y );
					_FoldSection( _TranslateBufferIndex( nStart ), _TranslateBufferIndex( nEnd ) );
					selectionValid = false;
					cursorY = nStart;
					ShowCursor();
					InvalidateLines( nStart, buffer.size()-1 );
					UpdateScrollbars();
				} else {
					FoldSection( charY, charY + 1 );
					UpdateScrollbars();
				}
			}
			break;
		case 'x':
		case 'X':
			if(ctrl && !alt && !shift){
				if(readOnly)
					break;
				Cut();
				ShowCursor();
				goto done;
			}
			break;
		case 'c':
		case 'C':
			if(ctrl && !alt && !shift){
				Copy();
				ShowCursor();
				goto done;
			}
			break;
		case 'v':
		case 'V':
			if(ctrl && !alt && !shift){
				if(readOnly)
					break;
				Paste();
				ShowCursor();
				goto done;
			}
			break;
		case 'y':
		case 'Y':
			if(ctrl && !alt && !shift){
				if(readOnly)
					break;
				Redo();
				ShowCursor();
				goto done;
			}
			break;
		case 'z':
		case 'Z':
			if(ctrl && !alt && !shift){
				if(readOnly)
					break;
				Undo();
				ShowCursor();
				goto done;
			}
			if(ctrl && !alt && shift){
				if(readOnly)
					break;
				Redo();
				ShowCursor();
				goto done;
			}
			break;
		}
	}

	switch(str[0]){
	case 0:
		break;
	case os::VK_LEFT_ARROW:
		if(alt && !ctrl)
			ScrollLeft();
		else if(!alt && ctrl){
			MoveWordLeft(shift);
			ShowCursor();
		}else if(!alt && !ctrl){
			MoveLeft(shift);
			ShowCursor();
		}
		break;
	case os::VK_RIGHT_ARROW:
		if(alt && !ctrl)
			ScrollRight();
		else if(!alt && ctrl){
			MoveWordRight(shift);
			ShowCursor();
		}else if(!alt && !ctrl){
			MoveRight(shift);
			ShowCursor();
		}
		break;
	case os::VK_UP_ARROW:
		if(alt)
			ScrollUp();
		else{
			MoveUp(shift);
			ShowCursor();
		}
		break;
	case os::VK_DOWN_ARROW:
		if(alt)
			ScrollDown();
		else{
			MoveDown(shift);
			ShowCursor();
		}
		break;
	case os::VK_HOME:
		if(ctrl)
			MoveTop(shift);
		else
			MoveLineStart(shift);
		ShowCursor();
		break;
	case os::VK_END:
		if(ctrl)
			MoveBottom(shift);
		else
			MoveLineEnd(shift);
		ShowCursor();
		break;
	case os::VK_PAGE_UP:
		if(alt)
			ScrollPageUp();
		else{
			MovePageUp(shift);
			ShowCursor();
		}
		break;
	case os::VK_PAGE_DOWN:
		if(alt)
			ScrollPageDown();
		else{
			MovePageDown(shift);
			ShowCursor();
		}
		break;
	case os::VK_TAB:
		if(readOnly)
			break;
			
		if(selectionValid){
			IndentSelection(shift);
		}else{
			cursorX=min(cursorX, GetW(buffer[charY].text.size(), charY));
			if(useTab)
/***/				InsertText("\t", GetChar(cursorX, charY), cursorY);
			else{
			    os::String tmp;
			    tmp.resize(tabSize, ' ');
				InsertText(tmp, GetChar(cursorX, charY), cursorY);
			}
			    
			ShowCursor();
		}
		break;
	case os::VK_BACKSPACE:
		if(readOnly)
			break;
		if(alt && !shift && !ctrl)
			Undo();
		else if(!alt){
			if(selectionValid){
				Del();
			}else if(cursorY!=0 || (GetChar(cursorX, 0)!=0 && buffer[0].text.size()!=0)){
				MoveLeft(false);
				Del();
			}
		}
		ShowCursor();
		break;		
	case os::VK_DELETE:
		if(readOnly)
			break;
		if(shift)
			Cut();
		else
			Del();
		ShowCursor();
		break;
	case os::VK_ENTER:
		{
			if(readOnly)
				break;
			if(selectionValid)
				Del();

			cursorX=min(cursorX, GetW(buffer[charY].text.size(), charY));
			uint chr=GetChar(cursorX, charY);
			if(format){
				InsertText(os::String("\n")+format->GetIndentString( buffer[charY].text.str().substr(0, chr), 
					useTab, tabSize), chr, cursorY);
			}else{
				InsertText("\n", chr, cursorY);
			}

			ShowCursor();
			eventBuffer |= CodeView::EI_ENTER_PRESSED;
		}
		break;		
	case os::VK_INSERT:
		if(shift && !ctrl){
			if(readOnly)
				break;
			Paste();
			ShowCursor();
		}
		if(!shift && ctrl){
			Copy();
			ShowCursor();
		}
		break;			
	case os::VK_ESCAPE:
		eventBuffer |= CodeView::EI_ESC_PRESSED;
		break;
	case os::VK_FUNCTION_KEY:
		break;
	default:
		if(readOnly)
			break;
		if(selectionValid)
			Del();

		cursorX=min(cursorX, GetW(buffer[charY].text.size(), charY));
		InsertText(str, GetChar(cursorX, charY), cursorY);
		if(dead) {
			MoveLeft(false);
			MoveRight(true);
		}
		ShowCursor();
	}

done:
	Flush();
	CommitEvents();
}

void InnerEdit::WheelMoved(const os::Point &p)
{
	os::Point off=GetScrollOffset();

	off.y=off.y-3*p.y*lineHeight;
	float max=buffer.size()*lineHeight-Height();
	if(off.y<-max)
		off.y=-max;
	if(off.y>0.0f)
		off.y=0.0f;
		
	ScrollTo(off);
}

void InnerEdit::MouseDown(const os::Point &p, uint32 but)
{
	if(!HasFocus())
		MakeFocus();

	if( but & 1 ) {
		mousePressed = true;

		ClearSelection();

		uint line=min((uint)(p.y/lineHeight), buffer.size()-1);

		InvalidateLines(cursorY, cursorY);
		cursorY=line;
		cursorX=p.x - m_nMargin;
		InvalidateLines(cursorY, cursorY);

		ShowCursor();
	
		Flush();

		CommitEvents();
	}

	if( but & 2 && m_pcContextMenu )
	{
		m_pcContextMenu->Open( ConvertToScreen( p ) );
	}
}

void InnerEdit::MouseUp(const os::Point &p, uint32 but, os::Message* m)
{
	os::String cString;
	
	if (m!=NULL && m->FindString("file/path", &cString) == 0)
	{
		control->MouseUp(p,but,m);
	}

	if(but&1)
		mousePressed=false;
}

void InnerEdit::MouseMove(const os::Point &p, int code, uint32 but, os::Message *m)
{
	if(code==os::MOUSE_ENTERED){
		os::Application::GetInstance()->PushCursor(os::MPTR_MONO, mouseImg, 
			POINTER_WIDTH, POINTER_HEIGHT, os::IPoint(POINTER_WIDTH/2, POINTER_HEIGHT/2));
		IcursorActive=true;
	}else if(code==os::MOUSE_EXITED){
		IcursorActive=false;
		os::Application::GetInstance()->PopCursor();
    }

	
    if(enabled && mousePressed && but&1){
		int line=min((int)(p.y/lineHeight), (int)(buffer.size()-1));
		if(line<0)
			line=0;
		float x=p.x - m_nMargin;
		if(x<0)
			x=0;
		uint chr=GetChar(x, _TranslateBufferIndex( line ));

		SetCursor(chr, line, true);

		Flush();
		CommitEvents();
    }
}

void InnerEdit::SetCursor(uint x, uint y, bool select)
{
	PreMove(select);
	
	InvalidateLines(cursorY, cursorY);
	cursorY=min(y, buffer.size()-1);
	cursorX=GetW(x, _TranslateBufferIndex( cursorY ) );
	InvalidateLines(cursorY, cursorY);

	ShowCursor();

	PostMove(select);
}

void InnerEdit::PreMove(bool select)
{
	if(selectionValid && !select)
		ClearSelection();
	else if(!selectionValid && select){
		SetSelectionStart(GetChar(cursorX, _TranslateBufferIndex( cursorY )), cursorY);
	}
}

void InnerEdit::PostMove(bool select)
{
	if(select)
		SetSelectionEnd(GetChar(cursorX, _TranslateBufferIndex( cursorY )), cursorY);
}

void InnerEdit::MoveLeft(bool select)
{
	PreMove(select);

	uint y = _TranslateBufferIndex( cursorY );;
	uint x = min(GetChar(cursorX, y), buffer[y].text.size());

	if( !x ) {
		if( cursorY > 0 ) {
			--cursorY;
			y = _TranslateBufferIndex( cursorY );
			cursorX = GetW( buffer[y].text.size(), y );
			InvalidateLines(cursorY, cursorY+1);
		}
	}else{
		--x;
		while(x>0 && !os::is_first_utf8_byte(buffer[y].text[x]))
			--x;
		cursorX=GetW(x, y);
		InvalidateLines(cursorY, cursorY);
	}
	
	PostMove(select);	
}

void InnerEdit::MoveRight(bool select)
{
	PreMove(select);

	uint y = _TranslateBufferIndex( cursorY );
	uint x = min(GetChar(cursorX, y), buffer[y].text.size());

	if(x==buffer[y].text.size()){
		if(y<buffer.size()-1){
			++cursorY;
			y = _TranslateBufferIndex( cursorY );
			cursorX=0;
			InvalidateLines(cursorY-1, cursorY);
		}
	}else{
		int len=os::utf8_char_length(buffer[y].text[x]);
		cursorX=GetW(x+len, y);
		InvalidateLines(cursorY, cursorY);
	}

	PostMove(select);
}

void InnerEdit::MoveUp(bool select)
{
	PreMove(select);

	if(cursorY>0){
		--cursorY;
		InvalidateLines(cursorY, cursorY+1);
	}

	PostMove(select);
}

void InnerEdit::MoveDown(bool select)
{
	PreMove(select);

	if( _TranslateBufferIndex( cursorY ) < buffer.size() - 1 ) {
		++cursorY;
		InvalidateLines(cursorY-1, cursorY);
	}

	PostMove(select);
}

void InnerEdit::MoveTop(bool select)
{
	PreMove(select);

	cursorX=0;
	InvalidateLines(0, cursorY);
	cursorY=0;

	PostMove(select);
}

void InnerEdit::MoveBottom(bool select)
{
	PreMove(select);

	InvalidateLines(cursorY, buffer.size());
	uint y = buffer.size()-1;
	cursorY = _TranslateLineNumber( y );
	cursorX=GetW(buffer[y].text.size(), y);

	PostMove(select);
}

void InnerEdit::MovePageUp(bool select)
{
	PreMove(select);

	uint lines=((int)Height())/((int)lineHeight);

	InvalidateLines(cursorY, cursorY);
	if(lines>cursorY)
		cursorY=0;
	else
		cursorY-=lines;
	InvalidateLines(cursorY, cursorY);

	PostMove(select);
}

void InnerEdit::MovePageDown(bool select)
{
	PreMove(select);

	uint lines=((int)Height())/((int)lineHeight);

	InvalidateLines(cursorY, cursorY);
	cursorY = min( _TranslateLineNumber( buffer.size()-1 ), cursorY + lines );
	InvalidateLines(cursorY, cursorY);

	PostMove(select);
}

void InnerEdit::MoveLineStart(bool select)
{
	PreMove(select);

	cursorX=0;
	InvalidateLines(cursorY, cursorY);

	PostMove(select);
}

void InnerEdit::MoveLineEnd(bool select)
{
	PreMove(select);

	uint y = _TranslateBufferIndex( cursorY );
	cursorX=GetW(buffer[y].text.size(), y);
	InvalidateLines(cursorY, cursorY);

	PostMove(select);
}

void InnerEdit::MoveWordLeft(bool select){
	uint y = _TranslateBufferIndex( cursorY );
	uint x = min(GetChar(cursorX, y), buffer[y].text.size());

	if(x==0 || format==NULL)
		MoveLeft(select);
	else{
		PreMove(select);

		cursorX=GetW(format->GetPreviousWordLimit(buffer[y].text, x), y);
		InvalidateLines(cursorY, cursorY);

		PostMove(select);
	}
}

void InnerEdit::MoveWordRight(bool select)
{
	uint y = _TranslateBufferIndex( cursorY );
	uint x=min(GetChar(cursorX, y), buffer[y].text.size());

	if(x==buffer[y].text.size() || format==NULL)
		MoveRight(select);
	else{
		PreMove(select);

		cursorX=GetW(format->GetNextWordLimit(buffer[y].text, x), y);
		InvalidateLines(cursorY, cursorY);

		PostMove(select);
	}
}

void InnerEdit::SetFormat(Format* f)
{
	format=f;

	if(format){
		Reformat(0, buffer.size()-1);
	}else{
		for(uint a=0;a<buffer.size();++a)
			buffer[a].style.resize(0);
	}
}

void InnerEdit::FrameSized(const os::Point &p)
{
	UpdateBackBuffer();
	UpdateScrollbars();
}

void InnerEdit::UpdateScrollbars()
{
	os::ScrollBar* sb = GetVScrollBar();

	if( sb ) {
		float h = Height();
		uint nBufSize = _TranslateLineNumber( buffer.size() );
		sb->SetSteps( lineHeight, h - lineHeight );
		sb->SetMinMax( 0, max( 0.0f, lineHeight * nBufSize - h ) );
		sb->SetProportion( h / ( lineHeight * nBufSize ) );
	}

	sb = GetHScrollBar();
	if( sb ) {
		float w = Width();
		sb->SetSteps( spaceWidth, w - spaceWidth );
		sb->SetMinMax(0, maxWidth + spaceWidth - w );
		sb->SetProportion( w / ( maxWidth + spaceWidth ) );
	}
}

void InnerEdit::ShowCursor()
{
	const float vOffset = 5;
	os::Point cScroll = GetScrollOffset();
	os::Point cInitialScroll = cScroll;
	float w = Width();
	float h = Height();

	uint nCharY = _TranslateBufferIndex( cursorY );

	if( nCharY > buffer.size() - 1 )
		nCharY = buffer.size() - 1;
	
	float y = cursorY * lineHeight;
	int cx = min( GetChar( cursorX, nCharY ), buffer[ nCharY ].text.size() );
	float x = GetW( cx, nCharY );

	if( y < -cScroll.y ){
		cScroll.y = -( y - vOffset );
	}
	if( y + lineHeight + cScroll.y > h ) {
		cScroll.y = -( y + lineHeight + vOffset - h );
	}

	if( x < -cScroll.x ) {
		cScroll.x = -( x - vOffset );
	}
	if( x + cScroll.x + m_nMargin > w ) {
		cScroll.x = -( x + m_nMargin + vOffset - w );
	}
	if( cScroll.y > 0 ) {
		cScroll.y = 0 ;
	}
	if( cScroll.x > 0 ) {
		cScroll.x = 0;
	}

	if( cScroll != cInitialScroll )
		ScrollTo( cScroll );
}

/*Gets the adjusted width of line y, characters [0, x]
*/
float InnerEdit::GetW(uint x, uint y) const
{
	y=min(y, buffer.size()-1);

	const char *line=buffer[y].text.c_str();
	
	uint dx=0;
/*	if(buffer[y].text.size()>x){
		dx=x-buffer[y].text.size();
		x-=dx;
	}	
*/	
	uint first=0, last;
	float w=0;
	
	while(first<x){
		last=first;
		if(line[first]=='\t'){
			float tab=spaceWidth*tabSize;
			w=tab*ceil((w+1)/tab);
			
			while(last+1<x && line[last+1]=='\t'){
				++last;
				w+=tab;
			}
		}else{
			while(last+1<x && line[last+1] && line[last+1]!='\t')
				++last;
				
			w+=GetStringWidth(line+first, last-first+1);
		}
		first=last+1;
	}
	
//	cout << "GetW:\t\t" << x << "\t" << y << "\t" << w+spaceWidth*dx << endl;
	return w+spaceWidth*dx;
}

uint InnerEdit::GetChar(float targetX, uint y) const
{
	y=min(y, buffer.size()-1);

	const char *line=buffer[y].text.c_str();

	uint c=0;
	uint max=buffer[y].text.size();
	
	float x=0;

	while(c<max && x<targetX){
		if(line[c]=='\t'){
			float tab=spaceWidth*tabSize;
			x=tab*ceil((x+1)/tab);
			++c;
		}else{
			int len=os::utf8_char_length(line[c]);
			x+=GetStringWidth(line+c, len);
			c+=len;
		}
	}
	if(x<targetX)
		c+=(int)((targetX-x)/spaceWidth);
		
//	cout << "GetChar:\t" << c << "\t" << y << "\t" << targetX << endl;
	return c;
}

void InnerEdit::SetSelectionStart(uint x, uint y)
{
	y=min(y, buffer.size()-1);
	x=min(x, buffer[ _TranslateBufferIndex(y) ].text.size());

	if(selectionValid){
		uint oldy=selStart.y;

		selStart.y=y;
		selStart.x=x;

		InvalidateLines(min(oldy, (uint)selStart.y), max(oldy, (uint)selStart.y));
	}else{
		selEnd.y=selStart.y=y;
		selEnd.x=selStart.x=x;
		selectionValid=true;		
	}
	eventBuffer |= CodeView::EI_SELECTION_CHANGED;
}

void InnerEdit::SetSelectionEnd(uint x, uint y)
{
	y=min(y, buffer.size()-1);
	x=min(x, buffer[ _TranslateBufferIndex(y) ].text.size());

	if(selectionValid){
		uint oldy=selEnd.y;

		selEnd.y=y;
		selEnd.x=x;

		InvalidateLines(min(oldy, (uint)selEnd.y), max(oldy, (uint)selEnd.y));
	}else{
		selEnd.y=selStart.y=y;
		selEnd.x=selStart.x=x;
		selectionValid=true;		
	}
	eventBuffer |= CodeView::EI_SELECTION_CHANGED;
}

void InnerEdit::ClearSelection()
{
	if(selectionValid){
		selectionValid=false;
		InvalidateLines(min(selStart.y, selEnd.y), max(selStart.y, selEnd.y));
	}
	eventBuffer |= CodeView::EI_SELECTION_CHANGED;
}

void InnerEdit::ScrollLeft()
{
	os::Point p=GetScrollOffset();

	p.x=min(0.0f, p.x+spaceWidth);

	ScrollTo(p);
}

void InnerEdit::ScrollRight()
{
	os::Point p=GetScrollOffset();

	p.x-=spaceWidth;

	ScrollTo(p);
}

void InnerEdit::ScrollUp()
{
	os::Point p=GetScrollOffset();

	p.y=min(0.0f, p.y+lineHeight);

	ScrollTo(p);
}

void InnerEdit::ScrollDown()
{
	os::Point p=GetScrollOffset();

	p.y=max(-(buffer.size()*lineHeight-Height()), p.y-lineHeight);
	if(p.y>0)
		p.y=0;

	ScrollTo(p);
}

void InnerEdit::ScrollPageUp()
{
	os::Point p=GetScrollOffset();

	p.y = min( 0.0f, p.y + Height() + lineHeight );

	ScrollTo( p );
}

void InnerEdit::ScrollPageDown()
{
	os::Point p = GetScrollOffset();

	p.y = max( -( _TranslateLineNumber( buffer.size() ) * lineHeight - Height() ), p.y - Height() + lineHeight );
	if( p.y > 0 )
		p.y = 0;

	ScrollTo( p );
}

void InnerEdit::UpdateBackBuffer()
{
	if(backBM){
		if(backView)
			backBM->RemoveChild(backView);
		delete backBM;
	}

	if(!backView)
		backView=new os::View(os::Rect(), "");

	backBM=new os::Bitmap((int)Width(), (int)lineHeight, os::CS_RGB16, os::Bitmap::ACCEPT_VIEWS);

	backView->SetFrame(backBM->GetBounds());
	os::Font* f=GetFont();
	backView->SetFont(f);
	backBM->AddChild(backView);	
}

void InnerEdit::Reformat(uint first, uint last)
{
	if(format==NULL)
		return;

	uint32 max=buffer.size()-1;
			
	if(last>max)
		last=max;
	if(first>last)
		first=last;			

	uint32 line=first;
	CodeViewContext oldCookie, newCookie;

	do{
		oldCookie=buffer[line].cookie;
		if(line==0)
			newCookie = format->Parse(buffer[line].text, buffer[line].style, CodeViewContext(0));
		else
			newCookie = format->Parse(buffer[line].text, buffer[line].style, buffer[line-1].cookie);
		buffer[line].cookie=newCookie;
		++line;
	}while(line<=max && (!(newCookie==oldCookie) || line<=last) );

	InvalidateLines(first, line);
	Flush();
}

void InnerEdit::Copy() const
{
	if(!selectionValid)
		return;

	os::Clipboard clip;
	clip.Lock();
	clip.Clear();
	os::String tmp;
	GetText(&tmp, selStart, selEnd);
	clip.GetData()->AddString("text/plain", tmp);
	clip.Commit();
	clip.Unlock();
}

void InnerEdit::Paste()
{
	os::Clipboard clip;
	os::String str;

	clip.Lock();
	
	if(clip.GetData()->FindString("text/plain", &str)==0){
		if(selectionValid)
			Del();

		InsertText( str, GetChar( cursorX, _TranslateBufferIndex( cursorY ) ), cursorY );
	}
	clip.Unlock();
}

void InnerEdit::Cut()
{
	if(!selectionValid)
		return;

	Copy();
	Del();
}

void InnerEdit::Del()
{
	if(selectionValid) {
		RemoveText(selStart, selEnd);
		ClearSelection();
	} else {
		uint y = _TranslateBufferIndex( cursorY );
		uint x = min( GetChar( cursorX, y ), buffer[y].text.size() );
		if( x < buffer[y].text.size() ) {
			RemoveText( x, cursorY, x + os::utf8_char_length( buffer[y].text[x] ), cursorY );
		}else if(cursorY<buffer.size()-1){
			RemoveText( x, cursorY, 0, cursorY+1 );
		}
	}
}

void InnerEdit::SetLine(const os::String &line, uint y, bool addUndo)
{
	if(addUndo){
		AddUndoNode(UndoNode::SET_LINE, buffer[y].text, 0, y);
	}

	buffer[y].text=line;
	buffer[y].style.resize(0);
	UpdateWidth(y);
	InvalidateLines(y, y);


	Reformat(y, y);
	eventBuffer |= CodeView::EI_CONTENT_CHANGED;
}

void InnerEdit::SetTabSize(uint i)
{
	uint y = _TranslateBufferIndex( cursorY );
	uint chr = GetChar( cursorX, y );
	tabSize = i;
	cursorX = GetW( chr, y );
	UpdateAllWidths();
	Invalidate();
}

os::IPoint InnerEdit::GetCursor() const
{
	return os::IPoint(min(buffer[cursorY].text.size(), GetChar(cursorX, cursorY)), cursorY);
}


void InnerEdit::SetEnable(bool b)
{
	if(b!=enabled){
		enabled=b;
		Invalidate();
	}
}

void InnerEdit::SetReadOnly(bool b)
{
	if(b!=readOnly){
		readOnly=b;
		Invalidate();
	}
}

void InnerEdit::CommitEvents()
{
	if(cursorX!=old_cursorX || cursorY!=old_cursorY)
		eventBuffer|=CodeView::EI_CURSOR_MOVED;
		
	old_cursorX=cursorX;
	old_cursorY=cursorY;
	
	if(control && (eventBuffer&eventMask))
		control->Invoke();	
	eventBuffer=0;
}

size_t InnerEdit::GetLength()
{
	size_t tmp=buffer.size();
	if(tmp>0)
		--tmp;

	for(uint a=0;a<buffer.size();++a)
		tmp+=buffer[a].text.size();

	return tmp;
}

void InnerEdit::SetMaxUndoSize(int size)
{
//TODO: implement
	maxUndo=size;
}

int InnerEdit::GetMaxUndoSize()
{
	return maxUndo;
}

void InnerEdit::AddUndoNode(uint mode, const os::String &str, uint x, uint y)
{
	UndoNode *node=new UndoNode();
	node->mode=mode;
	node->text=str;
	node->x=x;
	node->y=y;

	if(undoCurrent==NULL){
		undoHead=undoTail=undoCurrent=node;
		undoCount=1;
		redoCount=0;
	}else{
		if(undoCurrent->next!=NULL){
			UndoNode *tmp=undoCurrent->next;
			while(tmp->next){
				tmp=tmp->next;
				delete tmp->previous;
			}
			delete tmp;
			redoCount=0;
		}
		undoCurrent->next=node;
		node->previous=undoCurrent;
		undoCurrent=node;
		undoTail=undoCurrent;

		++undoCount;
	}

	while(undoCount>maxUndo){
		undoHead=undoHead->next;
		delete undoHead->previous;
		undoHead->previous=NULL;
		--undoCount;
	}
}

void InnerEdit::ClearUndo()
{
	if(!undoHead)
		return;

	while(undoHead->next){
		undoHead=undoHead->next;
		delete undoHead->previous;
	}
	delete undoHead;

	undoHead=undoTail=undoCurrent=NULL;
	undoCount=redoCount=0;		
}

bool InnerEdit::UndoAvailable()
{
	return undoCount>0;
}

bool InnerEdit::RedoAvailable()
{
	return redoCount>0;
}

bool InnerEdit::Undo()
{
	if(undoCurrent==NULL || undoCount==0)
		return false;

	InvalidateLines( cursorY, cursorY );

	switch(undoCurrent->mode){
		case UndoNode::ADDED:
		{
			os::String& str=undoCurrent->text;
			uint y=undoCurrent->y;
			uint x=undoCurrent->x+str.size();
		
			while( x > buffer[ _TranslateBufferIndex(y) ].text.size() ) {
				x -= buffer[ _TranslateBufferIndex(y) ].text.size() + 1;
				++y;
			}

			RemoveText(undoCurrent->x, undoCurrent->y, x, y, false);
			cursorX = undoCurrent->x;
			cursorY = undoCurrent->y;
			break;
		}
		case UndoNode::REMOVED:
			InsertText(undoCurrent->text, undoCurrent->x, undoCurrent->y, false);
			cursorX = undoCurrent->x;
			cursorY = undoCurrent->y;
			break;
		case UndoNode::SET_LINE:
		{
			os::String tmp=buffer[undoCurrent->y].text;
			SetLine(undoCurrent->text, undoCurrent->y, false);
			undoCurrent->text=tmp;
			cursorX = undoCurrent->x;
			cursorY = undoCurrent->y;
			break;
		}
		default:
			assert(false);
	}

	if(undoCurrent->previous!=NULL)
		undoCurrent=undoCurrent->previous;
	
	--undoCount;
	++redoCount;

	return true;
}

bool InnerEdit::Redo()
{
	if(undoCurrent==NULL || redoCount==0)
		return false;
	
	UndoNode* node=undoCurrent;
	if(undoCount>0)
		node=node->next;

	InvalidateLines( cursorY, cursorY );

	switch(node->mode){
		case UndoNode::REMOVED:
		{
			os::String& str=node->text;
			uint y=node->y;
			uint x=node->x+str.size();
		
			while( x > buffer[ _TranslateBufferIndex(y) ].text.size() ) {
				x -= buffer[ _TranslateBufferIndex(y) ].text.size() + 1;
				++y;
			}

			RemoveText(node->x, node->y, x, y, false);
			cursorX = node->x;
			cursorY = node->y;
			break;
		}
		case UndoNode::ADDED:
			InsertText(node->text, node->x, node->y, false);
			cursorX = node->x;
			cursorY = node->y;
			break;
		case UndoNode::SET_LINE:
		{
			os::String tmp=buffer[node->y].text;
			SetLine(node->text, node->y, false);
			node->text=tmp;
			cursorX = node->x;
			cursorY = node->y;
			break;
		}
		default:
			assert(false);
	}

	if(undoCount>0)
		undoCurrent=undoCurrent->next;

	++undoCount;
	--redoCount;

	return true;
}

void InnerEdit::UpdateWidth(uint y)
{
	float ow = buffer[y].w + m_nMargin;
	float nw = GetW( buffer[y].text.size(), y ) + m_nMargin;

	buffer[y].w=nw;

	if( ow >= maxWidth && nw < ow ) {
		UpdateAllWidths();
	} else {
		if(nw>maxWidth){
			maxWidth=nw;
			UpdateScrollbars();
		}
	}	
}

void InnerEdit::UpdateAllWidths()
{
	maxWidth = 0;
	for(uint a=0;a<buffer.size()-1;++a){
		buffer[a].w=GetW(buffer[a].text.size(), a);
		if(buffer[a].w>maxWidth)
			maxWidth=buffer[a].w;
	}
	maxWidth += m_nMargin;
	UpdateScrollbars();					
}


void InnerEdit::IndentSelection(bool unindent)
{
	uint top, bot;
	
	assert(selectionValid);
	selectionValid=false;
	
	if(selStart.y>selEnd.y){
		bot=selStart.y;
		if(selStart.x==0)
			--bot;		
		top=selEnd.y;
	}else{
		top=selStart.y;
		bot=selEnd.y;
		if(bot>top && selEnd.x==0)
			--bot;
	}
	
	for(uint a=top;a<=bot;++a){
		if(unindent){			
			const os::String &line=buffer[a].text;
			
			if(line.size()==0)
				continue;
				
			if(line[0]=='\t'){
				RemoveText(0, a, 1, a);
			}else if(line[0]==' '){
				uint len=max(tabSize, line.size());
				uint spaces=1;
				for(uint b=1;b<len;++b)
					if(line[b]==' ')
						++spaces;
					else
						break;
				RemoveText(0, a, spaces, a);
			}//else ignore
		}else{//indent
			if(useTab)
				InsertText("\t", 0, a);
			else{
				os::String pad;
				pad.resize(tabSize, ' ');
				InsertText(pad, 0, a);
			}
		}
	}
	
	selectionValid=true;
	selStart.y=top;
	selStart.x=0;
	selEnd.y=bot;
	selEnd.x=buffer[bot].text.size();
	float x=GetW(selEnd.x, bot);
	if(cursorY!=bot || cursorX!=x)
		eventBuffer |= CodeView::EI_CURSOR_MOVED;
	
	cursorY=bot;
	cursorX=x;
	
	Reformat(top, bot);
	InvalidateLines(top, bot);
	eventBuffer |= CodeView::EI_CONTENT_CHANGED;
}

/** Look for foldable parts of code within the specified range and fold them */
void InnerEdit::FoldSection( uint nFirst, uint nLast )
{
    if( format == NULL ) return;

    uint32 nMax = buffer.size()-1;
			
    if( nLast > nMax )
	nLast = nMax;
    if( nFirst > nLast )
	nFirst = nLast;

    uint32 nLine = nFirst, nFoldStart = 0;

    int nFoldLevel = 0, nOldFold;

//    cout << "FoldSection: " << nFirst << ", " << nLast << endl;

    do {
		nOldFold = nFoldLevel;
		nFoldLevel = format->GetFoldLevel( buffer[ nLine ].text, nOldFold );
		if( nFoldLevel == 1 && nOldFold == 0 ) {
			nFoldStart = nLine;
		}
		if( nFoldLevel == 0 && nOldFold == 1 ) {
//	        cout << "** _FoldSection: " << nFoldStart << ", " << nLine << endl;
	        if( nLine-1 > nFoldStart )
				_FoldSection( nFoldStart, nLine-1 );
		}
		++nLine;
	} while( nLine <= nMax && ( nFoldLevel > 0 || nLine <= nLast ) );

    InvalidateLines( 0, buffer.size()-1 );
    Flush();
}

/** Translate a visible line number to a buffer index */
uint InnerEdit::_TranslateBufferIndex( uint32 nLineIndex ) const
{
	std::list<Fold>::iterator iter;
	uint nLastEnd = 0;

	for( iter = cFoldedSections.begin(); iter != cFoldedSections.end() && (*iter).nStart < nLineIndex; iter++ ) {
		if( (*iter).nStart >= nLastEnd ) {
			nLineIndex += (*iter).nEnd - (*iter).nStart;
			nLastEnd = (*iter).nEnd;
		}
	}

	return nLineIndex;
}

/** Translate a buffer index to a visible line number */
uint InnerEdit::_TranslateLineNumber( uint32 nBufferIndex ) const
{
	std::list<Fold>::iterator iter;
	uint nLastEnd = 0, nLineIndex = nBufferIndex;

	for( iter = cFoldedSections.begin(); iter != cFoldedSections.end() && (*iter).nStart < nBufferIndex; iter++ ) {
		if( (*iter).nStart >= nLastEnd ) {
			nLineIndex -= (*iter).nEnd - (*iter).nStart;
			nLastEnd = (*iter).nEnd;
		}
	}

	return nLineIndex;
}

/** Check if a line is folded */
uint InnerEdit::_LineIsFolded( uint32 nBufferIndex )
{
	std::list<Fold>::iterator iter;

	for( iter = cFoldedSections.begin(); iter != cFoldedSections.end(); iter++ ) {
		if( nBufferIndex > (*iter).nStart && nBufferIndex <= (*iter).nEnd ) {
			return 1;
		}
		if( nBufferIndex == (*iter).nStart ) {
			return 2;
		}
	}

	return 0;
}

/** Add a section of code to the invisible list */
void InnerEdit::_FoldSection( uint32 nStart, uint32 nEnd )
{
	std::list<Fold>::iterator iter;

	for( iter = cFoldedSections.begin(); iter != cFoldedSections.end(); iter++ ) {
		if( nStart < (*iter).nStart ) {
			cFoldedSections.insert( iter, Fold( nStart, nEnd ) );
			return;
		}
	}

	cFoldedSections.push_back( Fold( nStart, nEnd ) );
}

/** Make a hidden (folded) section of code visible again */
void InnerEdit::_UnfoldSection( uint32 nStart )
{
	std::list<Fold>::iterator iter;

	for( iter = cFoldedSections.begin(); iter != cFoldedSections.end(); iter++ ) {
		if( (*iter).nStart == nStart ) {
			cFoldedSections.erase( iter );
			return;
		}
	}
}

/** Adjust positions of folded sections */
void InnerEdit::_AdjustFoldedSections( uint nStart, int nLen )
{
	std::list<Fold>::iterator iter;

	for( iter = cFoldedSections.begin(); iter != cFoldedSections.end(); iter++ ) {
/*		if( ( nStart >= (*iter).nStart ) && ( (*iter).nEnd - (*iter).nStart <= nLen ) ) {
			cFoldedSections.erase( iter );
		} else {*/
			if( nStart <= (*iter).nStart ) {
				(*iter).nStart += nLen;
			}
			if( nStart < (*iter).nEnd ) {
				(*iter).nEnd += nLen;
			}
//		}
	}
}
