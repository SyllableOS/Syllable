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
maxUndo(1000), undoCount(0), redoCount(0), undoHead(0), undoTail(0), undoCurrent(0)
{
	SetBgColor(255, 255, 255);
	SetFgColor(0, 0, 0);

	os::font_properties fp;
	os::Font::GetDefaultFont(DEFAULT_FONT_FIXED, &fp);
	os::Font *f=new os::Font(fp);
	SetFont(f);
	f->Release();

	buffer.resize(1);

	updateBackBuffer();
	updateScrollBars();
}

InnerEdit::~InnerEdit()
{
	if(backBM)
		delete backBM;

	if(IcursorActive)
		os::Application::GetInstance()->PopCursor();
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

	if(selectionValid){
		if(selTop>selBot){
			uint tmp=selTop;
			selTop=selBot;
			selBot=tmp;
			tmp=selLeft;
			selLeft=selRight;
			selRight=tmp;
		}else if(selTop==selBot && selLeft>selRight){
			uint tmp=selLeft;
			selLeft=selRight;
			selRight=tmp;
		}
	}

//	const uint nDigits = log10( buffer.size() ) + 1;
//	const uint nMargin = 0; //( nDigits + 1 ) * backView->GetStringWidth( "0" );
	const float xOff = GetScrollOffset().x;
	const float w = Width(); /* - nMargin;	 *** */

	uint32 nVisibleLine = nTopLine;
	uint32 nBufferIndex = _TranslateBufferIndex( nVisibleLine );
	float y = nVisibleLine * lineHeight;
	while( nBufferIndex <= nBotLine && nBufferIndex < buffer.size() ) {
		std::string& pcLineText = buffer[ nBufferIndex ].text;
		uint invStart=0, invEnd=0, nMargin = 0;
		bool bIsFolded = _LineIsFolded( nBufferIndex );

/*		if( bIsFolded ) {
			nMargin = backView->GetStringWidth( "-> " );
			backView->DrawFrame( os::Rect( 0, 0, nMargin, lineHeight ), 0 );
			backView->MovePenTo( 2, lineBase );
			backView->DrawString( "->" );
		}*/

/*		backView->FillRect( os::Rect( 0, 0, nMargin-1, lineHeight ), os::Color32_s( 220, 220, 250 ) );
		backView->MovePenTo( 0, lineBase );
		backView->SetFgColor( GetFgColor() );
		backView->SetBgColor( os::Color32_s( 220, 220, 250 ) );
		if( _LineIsFolded( nBufferIndex ) ) {
			backView->DrawFrame( os::Rect(0, 0, nMargin, lineHeight), os::FRAME_THIN );
			backView->DrawString( "+" );
		} else {
			os::String cLineNo;
			cLineNo.Format( "%d", nBufferIndex );
			backView->DrawString( cLineNo );
		}*/

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
				f0=getW( invStart, nBufferIndex );
				if(enabled)
					backView->FillRect(os::Rect( nMargin + xOff, 0, nMargin + xOff + f0, lineHeight), GetBgColor());
				else
					backView->FillRect(os::Rect( nMargin + xOff, 0, nMargin + xOff + f0, lineHeight), dim(GetBgColor()));
			}
			if(invEnd>0){
				f1=getW(invEnd, nBufferIndex );				
				if(enabled)
					backView->FillRect(os::Rect( nMargin + xOff + f0, 0, nMargin + xOff + f1, lineHeight), invert(GetBgColor()));
				else
					backView->FillRect(os::Rect( nMargin + xOff + f0, 0, nMargin + xOff + f1, lineHeight), dim(invert(GetBgColor())));
			}
			if(xOff+f1<w)
				if(enabled)
					backView->FillRect(os::Rect( nMargin + xOff + f1, 0, nMargin + w, lineHeight), GetBgColor());
				else
					backView->FillRect(os::Rect( nMargin + xOff + f1, 0, nMargin + w, lineHeight), dim(GetBgColor()));
		} else {
			if(enabled) {
				backView->FillRect( os::Rect( nMargin, 0, nMargin + w, lineHeight ), GetBgColor());
			} else {
				backView->FillRect( os::Rect( nMargin, 0, nMargin + w, lineHeight ), dim(GetBgColor()));
			}
		}

		os::Color32_s fg=GetFgColor();
		os::Color32_s bg=GetBgColor();

		backView->SetFgColor(fg);
		backView->SetBgColor(bg);

		backView->MovePenTo( nMargin + xOff, lineBase );

		for( uint a = 0; a < pcLineText.size(); ) {
			char chr = pcLineText[a];

			if(chr=='\t'){
				backView->Sync();//need this to get correct pen pos
				float tab=spaceWidth*tabSize;
				float x=backView->GetPenPosition().x-xOff;
				x = tab*ceil((x+1)/tab);
				
				++a;
				while( a < pcLineText.size() && pcLineText[ a ] == '\t' ) {
					++a;
					x+=tab;
				}
				
				backView->MovePenTo( xOff + x, lineBase );
			}else{
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

				if(nfg!=fg){
					fg=nfg;
					backView->SetFgColor(fg);
				}
				if(nbg!=bg){
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
			backView->DrawString( " (...)" );
		}

		if( HasFocus() && nVisibleLine == cursorY ) {
			int c = min( getChar( cursorX, nBufferIndex ), pcLineText.size() );
			float x = getW( c, nBufferIndex ) + xOff + nMargin;
			backView->SetDrawingMode(os::DM_INVERT);
			backView->DrawLine(os::Point(x, 0), os::Point(x, lineHeight));
			x+=1.0f;
			backView->DrawLine(os::Point(x, 0), os::Point(x, lineHeight));
			backView->SetDrawingMode(os::DM_COPY);
		}


		backView->Sync();
		DrawBitmap( backBM, backBM->GetBounds(), os::Rect( -xOff, y, backBM->GetBounds().Width() - xOff, y + lineHeight ) );
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
void InnerEdit::invalidateLines(int start, int stop)
{
	os::Rect r=GetBounds();
	r.top=start*lineHeight;
	r.bottom=(stop+1)*lineHeight;
	Invalidate(r);	
}

/** Set the entire buffer */
void InnerEdit::setText(const string &s)
{
	buffer.clear();
	vector<string> list;

	splitLine(list, s);
	buffer.resize(list.size());

	for(uint a=0;a<list.size();++a){
		buffer[a].text=list[a];
	}
	updateAllWidths();

	reformat(0, buffer.size()-1);

	// TODO: Remove
	FoldSection( 0, buffer.size()-1 );

	cursorY=0;
	cursorX=0;

	updateScrollBars();
	Invalidate();
	eventBuffer |= CodeView::EI_CONTENT_CHANGED;
	clearUndo();
}

/** Insert text at a certain location and optionally create an UndoNode */
void InnerEdit::insertText(const string &str, uint x, uint y, bool addUndo)
{
	uint nBufferIndex = _TranslateBufferIndex( y );
	nBufferIndex = min( nBufferIndex, buffer.size() );
	x = min( x, buffer[nBufferIndex].text.size() );

	vector<string> list;

	splitLine(list, str);

	uint cursorChar = getChar( cursorX, _TranslateBufferIndex( cursorY ) );

	if(addUndo)
		addUndoNode(UndoNode::ADDED, str, x, y);

	if(list.size()==1){
		buffer[nBufferIndex].text.insert(x, str);
	
		if( _TranslateBufferIndex(cursorY) == nBufferIndex && cursorChar >= x )
			cursorChar += list[0].size();

		updateWidth(y);
		//_AdjustFoldedSections( nBufferIndex, 1 );
		invalidateLines(y, y);
	}else{
		string tmp=buffer[ nBufferIndex ].text.substr(x);
		buffer[nBufferIndex].text.erase(x);
		buffer[nBufferIndex].text+=list[0];

		if(nBufferIndex==buffer.size()-1){
			for(uint a=1;a<list.size();++a){
				buffer.push_back(Line());
				buffer.back().text=list[a];
			}
		}else{
			for(uint a=1;a<list.size();++a){
				buffer.insert(&buffer[nBufferIndex+a], Line());
				buffer[nBufferIndex+a].text=list[a];
			}
		}

		buffer[nBufferIndex+list.size()-1].text+=tmp;
		updateAllWidths();

		if( _TranslateBufferIndex(cursorY) == nBufferIndex && cursorChar >= x ) {
			cursorY+=list.size()-1;
			cursorChar+=list.back().size()-x;
		} else if( _TranslateBufferIndex(cursorY) > nBufferIndex ) {
			cursorY+=list.size()-1;
		}

		_AdjustFoldedSections( nBufferIndex, list.size()-1 );

		invalidateLines( y, buffer.size()-1 );
	}

	cursorX = getW( cursorChar, _TranslateBufferIndex( cursorY ) );

	reformat( nBufferIndex, nBufferIndex+list.size() );

	updateScrollBars();
	eventBuffer |= CodeView::EI_CONTENT_CHANGED;
}

void InnerEdit::getText(string* str, uint left, uint top, uint right, uint bot) const
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
		*str = buffer[top].text.substr(left, right-left);
		return;
	}

	*str=buffer[top].text.substr(left);
	*str+='\n';
	

	for(uint a=top+1;a<bot;++a){
		*str += buffer[a].text;
		*str += '\n';
	}

	*str += buffer[bot].text.substr(0, right);
}

void InnerEdit::removeText( uint nLeft, uint nTop, uint nRight, uint nBot, bool bAddUndo )
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

	uint nCursorChar = getChar( cursorX, _TranslateBufferIndex( cursorY ) );

	if( nBfrTop == nBfrBot ) {
		if(bAddUndo)
			addUndoNode( UndoNode::REMOVED,
				buffer[ nBfrTop ].text.substr( nLeft, nRight - nLeft), nLeft, nTop);

		buffer[ nBfrTop ].text.erase( nLeft, nRight - nLeft );
		updateWidth( nBfrTop );
		
		if( cursorY == nTop ) {
			if( nCursorChar > nRight ) {
				nCursorChar -= nRight - nLeft;
			}else if( nCursorChar > nLeft){
				nCursorChar = nLeft;
			}
		}

	//	_AdjustFoldedSections( nBfrTop, -1 );
		invalidateLines( nTop, nTop );
	} else {
		string cUndoStr;

		if(bAddUndo)
			cUndoStr = buffer[ nBfrTop ].text.substr( nLeft ) + "\n";

		buffer[ nBfrTop ].text.erase( nLeft );
		if(cursorY == nTop && nCursorChar > nLeft)
			nCursorChar = nLeft;

		buffer[ nBfrTop ].text += buffer[ nBfrBot ].text.substr( nRight );

		for( uint a = nBfrTop + 1; a <= nBfrBot; ++a ) {
			if( bAddUndo )
				if( a < nBfrBot )
					cUndoStr += buffer[ nBfrTop + 1 ].text + "\n";
				else
					cUndoStr += buffer[ nBfrTop + 1 ].text.substr( 0, nRight );

			//TODO: erase all lines at once - this is slow
			buffer.erase( &buffer[ nBfrTop + 1 ] );
		}
		updateAllWidths();
		
		_AdjustFoldedSections( nBfrTop, nBfrTop - nBfrBot );

		if(bAddUndo)
			addUndoNode(UndoNode::REMOVED, cUndoStr, nLeft, nTop);
		
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

		updateScrollBars();
		Invalidate();
	}

	cursorX = getW( nCursorChar, _TranslateBufferIndex( cursorY ) );

	reformat( nBfrTop, nBfrTop + 1 );
	eventBuffer |= CodeView::EI_CONTENT_CHANGED;
}

/** Adjust font, if it has changed */
void InnerEdit::FontChanged(os::Font* f)
{
	os::font_height fh;
	f->GetHeight(&fh);

	lineBase=fh.ascender+fh.line_gap;
	lineHeight=fh.ascender+fh.line_gap+fh.descender;
	spaceWidth=f->GetStringWidth(" ");

	if(f->GetDirection()!=os::FONT_LEFT_TO_RIGHT){
		throw "Cannot use fonts with other directions than left-to-right!";
	}

	updateBackBuffer();
	updateScrollBars();
	Invalidate();
	Flush();
}

void InnerEdit::splitLine(vector<string> &list, const string& line, const char splitter)
{
	uint start=0;
	int end;

	list.clear();

	while(start<=line.size()){
		end=line.find(splitter, start);
		if(end==-1)
			end=line.size();

		list.push_back(line.substr(start, end-start));

		start=end+1;
	}
}

void InnerEdit::Activated(bool b)
{
	invalidateLines(cursorY, cursorY);
	Flush();
	
	if(!b){
		eventBuffer |= CodeView::EI_FOCUS_LOST;
		commitEvents();
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
					invalidateLines( cursorY, buffer.size()-1 );
					updateScrollBars();
				} else if( selectionValid && selStart.y != selEnd.y ) {
					uint nStart = min( selStart.y, selEnd.y );
					uint nEnd = max( selStart.y, selEnd.y );
					_FoldSection( _TranslateBufferIndex( nStart ), _TranslateBufferIndex( nEnd ) );
					selectionValid = false;
					cursorY = nStart;
					showCursor();
					invalidateLines( nStart, buffer.size()-1 );
					updateScrollBars();
				} else {
					FoldSection( charY, charY + 1 );
					updateScrollBars();
				}
			}
			break;
		case 'x':
			if(ctrl && !alt && !shift){
				if(readOnly)
					break;
				cut();
				showCursor();
				goto done;
			}
			break;
		case 'c':
			if(ctrl && !alt && !shift){
				copy();
				showCursor();
				goto done;
			}
			break;
		case 'v':
			if(ctrl && !alt && !shift){
				if(readOnly)
					break;
				paste();
				showCursor();
				goto done;
			}
			break;
		case 'y':
			if(ctrl && !alt && !shift){
				if(readOnly)
					break;
				redo();
				showCursor();
				goto done;
			}
			break;
		case 'z':
			if(ctrl && !alt && !shift){
				if(readOnly)
					break;
				undo();
				showCursor();
				goto done;
			}
			if(ctrl && !alt && shift){
				if(readOnly)
					break;
				redo();
				showCursor();
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
			scrollLeft();
		else if(!alt && ctrl){
			moveWordLeft(shift);
			showCursor();
		}else if(!alt && !ctrl){
			moveLeft(shift);
			showCursor();
		}
		break;
	case os::VK_RIGHT_ARROW:
		if(alt && !ctrl)
			scrollRight();
		else if(!alt && ctrl){
			moveWordRight(shift);
			showCursor();
		}else if(!alt && !ctrl){
			moveRight(shift);
			showCursor();
		}
		break;
	case os::VK_UP_ARROW:
		if(alt)
			scrollUp();
		else{
			moveUp(shift);
			showCursor();
		}
		break;
	case os::VK_DOWN_ARROW:
		if(alt)
			scrollDown();
		else{
			moveDown(shift);
			showCursor();
		}
		break;
	case os::VK_HOME:
		if(ctrl)
			moveTop(shift);
		else
			moveLineStart(shift);
		showCursor();
		break;
	case os::VK_END:
		if(ctrl)
			moveBottom(shift);
		else
			moveLineEnd(shift);
		showCursor();
		break;
	case os::VK_PAGE_UP:
		if(alt)
			scrollPageUp();
		else{
			movePageUp(shift);
			showCursor();
		}
		break;
	case os::VK_PAGE_DOWN:
		if(alt)
			scrollPageDown();
		else{
			movePageDown(shift);
			showCursor();
		}
		break;
	case os::VK_TAB:
		if(readOnly)
			break;
			
		if(selectionValid){
			indentSelection(shift);
		}else{
			cursorX=min(cursorX, getW(buffer[charY].text.size(), charY));
			if(useTab)
/***/				insertText("\t", getChar(cursorX, charY), cursorY);
			else{
			    string tmp;
			    tmp.resize(tabSize, ' ');
				insertText(tmp, getChar(cursorX, charY), cursorY);
			}
			    
			showCursor();
		}
		break;
	case os::VK_BACKSPACE:
		if(readOnly)
			break;
		if(alt && !shift && !ctrl)
			undo();
		else if(!alt){
			if(selectionValid){
				del();
			}else if(cursorY!=0 || (getChar(cursorX, 0)!=0 && buffer[0].text.size()!=0)){
				moveLeft(false);
				del();
			}
		}
		showCursor();
		break;		
	case os::VK_DELETE:
		if(readOnly)
			break;
		if(shift)
			cut();
		else
			del();
		showCursor();
		break;
	case os::VK_ENTER:
		{
			if(readOnly)
				break;
			if(selectionValid)
				del();

			cursorX=min(cursorX, getW(buffer[charY].text.size(), charY));
			uint chr=getChar(cursorX, charY);
			if(format){
				insertText("\n"+format->GetIndentString( buffer[charY].text.substr(0, chr), 
					useTab, tabSize), chr, cursorY);
			}else{
				insertText("\n", chr, cursorY);
			}

			showCursor();
			eventBuffer |= CodeView::EI_ENTER_PRESSED;
		}
		break;		
	case os::VK_INSERT:
		if(shift && !ctrl){
			if(readOnly)
				break;
			paste();
			showCursor();
		}
		if(!shift && ctrl){
			copy();
			showCursor();
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
			del();

		cursorX=min(cursorX, getW(buffer[charY].text.size(), charY));
		insertText(str, getChar(cursorX, charY), cursorY);
		if(dead) {
			moveLeft(false);
			moveRight(true);
		}
		showCursor();
	}

done:
	Flush();
	commitEvents();
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

	if(but&1){
		mousePressed=true;

		clearSelection();

		uint line=min((uint)(p.y/lineHeight), buffer.size()-1);

		invalidateLines(cursorY, cursorY);
		cursorY=line;
		cursorX=p.x;
		invalidateLines(cursorY, cursorY);

		showCursor();
	
		Flush();

		commitEvents();
	}
}

void InnerEdit::MouseUp(const os::Point &p, uint32 but, os::Message* m)
{
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
		float x=p.x;
		if(x<0)
			x=0;
		uint chr=getChar(x, _TranslateBufferIndex( line ));

		setCursor(chr, line, true);

		Flush();
		commitEvents();
    }
}

void InnerEdit::setCursor(uint x, uint y, bool select)
{
	preMove(select);
	
	invalidateLines(cursorY, cursorY);
	cursorY=min(y, buffer.size()-1);
	cursorX=getW(x, _TranslateBufferIndex( cursorY ) );
	invalidateLines(cursorY, cursorY);

	showCursor();

	postMove(select);
}

void InnerEdit::preMove(bool select)
{
	if(selectionValid && !select)
		clearSelection();
	else if(!selectionValid && select){
		setSelectionStart(getChar(cursorX, _TranslateBufferIndex( cursorY )), cursorY);
	}
}

void InnerEdit::postMove(bool select)
{
	if(select)
		setSelectionEnd(getChar(cursorX, _TranslateBufferIndex( cursorY )), cursorY);
}

void InnerEdit::moveLeft(bool select)
{
	preMove(select);

	uint y = _TranslateBufferIndex( cursorY );
	uint x = min(getChar(cursorX, y), buffer[y].text.size());

	if(x==0){
		if(cursorY>0){
			--cursorY;
			cursorX=getW(buffer[y].text.size(), y);
			invalidateLines(cursorY, cursorY+1);
		}
	}else{
		--x;
		while(x>0 && !os::is_first_utf8_byte(buffer[y].text[x]))
			--x;
		cursorX=getW(x, y);
		invalidateLines(cursorY, cursorY);
	}
	
	postMove(select);	
}

void InnerEdit::moveRight(bool select)
{
	preMove(select);

	uint y = _TranslateBufferIndex( cursorY );
	uint x = min(getChar(cursorX, y), buffer[y].text.size());

	if(x==buffer[y].text.size()){
		if(y<buffer.size()-1){
			++cursorY;
			y = _TranslateBufferIndex( cursorY );
			cursorX=0;
			invalidateLines(cursorY-1, cursorY);
		}
	}else{
		int len=os::utf8_char_length(buffer[y].text[x]);
		cursorX=getW(x+len, y);
		invalidateLines(cursorY, cursorY);
	}

	postMove(select);
}

void InnerEdit::moveUp(bool select)
{
	preMove(select);

	if(cursorY>0){
		--cursorY;
		invalidateLines(cursorY, cursorY+1);
	}

	postMove(select);
}

void InnerEdit::moveDown(bool select)
{
	preMove(select);

	if( _TranslateBufferIndex( cursorY ) < buffer.size() - 1 ) {
		++cursorY;
		invalidateLines(cursorY-1, cursorY);
	}

	postMove(select);
}

void InnerEdit::moveTop(bool select)
{
	preMove(select);

	cursorX=0;
	invalidateLines(0, cursorY);
	cursorY=0;

	postMove(select);
}

void InnerEdit::moveBottom(bool select)
{
	preMove(select);

	invalidateLines(cursorY, buffer.size());
	uint y = buffer.size()-1;
	cursorY = _TranslateLineNumber( y );
	cursorX=getW(buffer[y].text.size(), y);

	postMove(select);
}

void InnerEdit::movePageUp(bool select)
{
	preMove(select);

	uint lines=((int)Height())/((int)lineHeight);

	invalidateLines(cursorY, cursorY);
	if(lines>cursorY)
		cursorY=0;
	else
		cursorY-=lines;
	invalidateLines(cursorY, cursorY);

	postMove(select);
}

void InnerEdit::movePageDown(bool select)
{
	preMove(select);

	uint lines=((int)Height())/((int)lineHeight);

	invalidateLines(cursorY, cursorY);
	cursorY = min( _TranslateLineNumber( buffer.size()-1 ), cursorY + lines );
	invalidateLines(cursorY, cursorY);

	postMove(select);
}

void InnerEdit::moveLineStart(bool select)
{
	preMove(select);

	cursorX=0;
	invalidateLines(cursorY, cursorY);

	postMove(select);
}

void InnerEdit::moveLineEnd(bool select)
{
	preMove(select);

	uint y = _TranslateBufferIndex( cursorY );
	cursorX=getW(buffer[y].text.size(), y);
	invalidateLines(cursorY, cursorY);

	postMove(select);
}

void InnerEdit::moveWordLeft(bool select){
	uint y = _TranslateBufferIndex( cursorY );
	uint x = min(getChar(cursorX, y), buffer[y].text.size());

	if(x==0 || format==NULL)
		moveLeft(select);
	else{
		preMove(select);

		cursorX=getW(format->GetPreviousWordLimit(buffer[y].text, x), y);
		invalidateLines(cursorY, cursorY);

		postMove(select);
	}
}

void InnerEdit::moveWordRight(bool select)
{
	uint y = _TranslateBufferIndex( cursorY );
	uint x=min(getChar(cursorX, y), buffer[y].text.size());

	if(x==buffer[y].text.size() || format==NULL)
		moveRight(select);
	else{
		preMove(select);

		cursorX=getW(format->GetNextWordLimit(buffer[y].text, x), y);
		invalidateLines(cursorY, cursorY);

		postMove(select);
	}
}

void InnerEdit::setFormat(Format* f)
{
	format=f;

	if(format){
		reformat(0, buffer.size()-1);
	}else{
		for(uint a=0;a<buffer.size();++a)
			buffer[a].style.resize(0);
	}
}

void InnerEdit::FrameSized(const os::Point &p)
{
	updateBackBuffer();
	updateScrollBars();
}

void InnerEdit::updateScrollBars()
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
		float w=Width();
		sb->SetSteps(spaceWidth, w-spaceWidth);
		sb->SetMinMax(0, maxWidth+spaceWidth-w);
		sb->SetProportion(w/(maxWidth+spaceWidth));
	}
}

void InnerEdit::showCursor()
{
	const float offset=5;

	bool changed=false;
	os::Point scroll=GetScrollOffset();
	float w=Width();
	float h=Height();

	uint charY = _TranslateBufferIndex( cursorY );

	if( charY > buffer.size()-1 )
		charY = buffer.size()-1;
	
	float y=cursorY*lineHeight;
	int cx=min(getChar(cursorX, charY), buffer[charY].text.size());
	float x=getW(cx, charY);

	if(y<-scroll.y){
		scroll.y=-(y-offset);
		changed=true;
	}
	if(y+lineHeight+scroll.y>h){
		scroll.y=-(y+lineHeight+offset-h);
		changed=true;
	}

	if(x<-scroll.x){
		scroll.x=-(x-offset);
		changed=true;
	}
	if(x+scroll.x>w){
		scroll.x=-(x+offset-w);
		changed=true;
	}

	if(scroll.y>0){
		scroll.y=0;
		changed=true;
	}
	if(scroll.x>0){
		scroll.x=0;
		changed=true;
	}

	if(changed)
		ScrollTo(scroll);
}

/*Gets the adjusted width of line y, characters [0, x]
*/
float InnerEdit::getW(uint x, uint y) const
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
	
//	cout << "getW:\t\t" << x << "\t" << y << "\t" << w+spaceWidth*dx << endl;
	return w+spaceWidth*dx;
}

uint InnerEdit::getChar(float targetX, uint y) const
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
		
//	cout << "getChar:\t" << c << "\t" << y << "\t" << targetX << endl;
	return c;
}

void InnerEdit::setSelectionStart(uint x, uint y)
{
	y=min(y, buffer.size()-1);
	x=min(x, buffer[ _TranslateBufferIndex(y) ].text.size());

	if(selectionValid){
		uint oldy=selStart.y;

		selStart.y=y;
		selStart.x=x;

		invalidateLines(min(oldy, (uint)selStart.y), max(oldy, (uint)selStart.y));
	}else{
		selEnd.y=selStart.y=y;
		selEnd.x=selStart.x=x;
		selectionValid=true;		
	}
	eventBuffer |= CodeView::EI_SELECTION_CHANGED;
}

void InnerEdit::setSelectionEnd(uint x, uint y)
{
	y=min(y, buffer.size()-1);
	x=min(x, buffer[ _TranslateBufferIndex(y) ].text.size());

	if(selectionValid){
		uint oldy=selEnd.y;

		selEnd.y=y;
		selEnd.x=x;

		invalidateLines(min(oldy, (uint)selEnd.y), max(oldy, (uint)selEnd.y));
	}else{
		selEnd.y=selStart.y=y;
		selEnd.x=selStart.x=x;
		selectionValid=true;		
	}
	eventBuffer |= CodeView::EI_SELECTION_CHANGED;
}

void InnerEdit::clearSelection()
{
	if(selectionValid){
		selectionValid=false;
		invalidateLines(min(selStart.y, selEnd.y), max(selStart.y, selEnd.y));
	}
	eventBuffer |= CodeView::EI_SELECTION_CHANGED;
}

void InnerEdit::scrollLeft()
{
	os::Point p=GetScrollOffset();

	p.x=min(0.0f, p.x+spaceWidth);

	ScrollTo(p);
}

void InnerEdit::scrollRight()
{
	os::Point p=GetScrollOffset();

	p.x-=spaceWidth;

	ScrollTo(p);
}

void InnerEdit::scrollUp()
{
	os::Point p=GetScrollOffset();

	p.y=min(0.0f, p.y+lineHeight);

	ScrollTo(p);
}

void InnerEdit::scrollDown()
{
	os::Point p=GetScrollOffset();

	p.y=max(-(buffer.size()*lineHeight-Height()), p.y-lineHeight);
	if(p.y>0)
		p.y=0;

	ScrollTo(p);
}

void InnerEdit::scrollPageUp()
{
	os::Point p=GetScrollOffset();

	p.y = min( 0.0f, p.y + Height() + lineHeight );

	ScrollTo( p );
}

void InnerEdit::scrollPageDown()
{
	os::Point p = GetScrollOffset();

	p.y = max( -( _TranslateLineNumber( buffer.size() ) * lineHeight - Height() ), p.y - Height() + lineHeight );
	if( p.y > 0 )
		p.y = 0;

	ScrollTo( p );
}

void InnerEdit::updateBackBuffer()
{
	if(backBM){
		if(backView)
			backBM->RemoveChild(backView);
		delete backBM;
	}

	if(!backView)
		backView=new os::View(os::Rect(), "");

	backBM=new os::Bitmap(Width(), lineHeight, os::CS_RGB16, os::Bitmap::ACCEPT_VIEWS);

	backView->SetFrame(backBM->GetBounds());
	os::Font* f=GetFont();
	backView->SetFont(f);
	backBM->AddChild(backView);	
}

void InnerEdit::reformat(uint first, uint last)
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
	}while(line<=max && (newCookie!=oldCookie || line<=last) );

	invalidateLines(first, line);
	Flush();
}

void InnerEdit::copy() const
{
	if(!selectionValid)
		return;

	os::Clipboard clip;
	clip.Lock();
	clip.Clear();
	string tmp;
	getText(&tmp, selStart, selEnd);
	clip.GetData()->AddString("text/plain", tmp);
	clip.Commit();
	clip.Unlock();
}

void InnerEdit::paste()
{
	os::Clipboard clip;
	string str;

	clip.Lock();
	
	if(clip.GetData()->FindString("text/plain", &str)==0){
		if(selectionValid)
			del();

//		uint y = _TranslateBufferIndex( cursorY );
		insertText( str, getChar( cursorX, _TranslateBufferIndex( cursorY ) ), cursorY );
	}
	clip.Unlock();
}

void InnerEdit::cut()
{
	if(!selectionValid)
		return;

	copy();
	del();
}

void InnerEdit::del()
{
	if(selectionValid) {
		removeText(selStart, selEnd);
		clearSelection();
	} else {
		uint y = _TranslateBufferIndex( cursorY );
		uint x = min( getChar( cursorX, y ), buffer[y].text.size() );
		if( x < buffer[y].text.size() ) {
			removeText( x, cursorY, x + os::utf8_char_length( buffer[y].text[x] ), cursorY );
		}else if(cursorY<buffer.size()-1){
			removeText( x, cursorY, 0, cursorY+1 );
		}
	}
}

void InnerEdit::setLine(const string &line, uint y, bool addUndo)
{
	if(addUndo){
		addUndoNode(UndoNode::SET_LINE, buffer[y].text, 0, y);
	}

	buffer[y].text=line;
	buffer[y].style.resize(0);
	updateWidth(y);
	invalidateLines(y, y);


	reformat(y, y);
	eventBuffer |= CodeView::EI_CONTENT_CHANGED;
}

void InnerEdit::setTabSize(uint i)
{
	uint y = _TranslateBufferIndex( cursorY );
	uint chr = getChar( cursorX, y );
	tabSize = i;
	cursorX = getW( chr, y );
	updateAllWidths();
	Invalidate();
}

os::IPoint InnerEdit::getCursor() const
{
	return os::IPoint(min(buffer[cursorY].text.size(), getChar(cursorX, cursorY)), cursorY);
}


void InnerEdit::setEnable(bool b)
{
	if(b!=enabled){
		enabled=b;
		Invalidate();
	}
}

void InnerEdit::setReadOnly(bool b)
{
	if(b!=readOnly){
		readOnly=b;
		Invalidate();
	}
}

void InnerEdit::commitEvents()
{
	if(cursorX!=old_cursorX || cursorY!=old_cursorY)
		eventBuffer|=CodeView::EI_CURSOR_MOVED;
		
	old_cursorX=cursorX;
	old_cursorY=cursorY;
	
	if(control && (eventBuffer&eventMask))
		control->Invoke();	
	eventBuffer=0;
}

size_t InnerEdit::getLength()
{
	size_t tmp=buffer.size();
	if(tmp>0)
		--tmp;

	for(uint a=0;a<buffer.size();++a)
		tmp+=buffer[a].text.size();

	return tmp;
}

void InnerEdit::setMaxUndoSize(int size)
{
//TODO: implement
	maxUndo=size;
}

int InnerEdit::getMaxUndoSize()
{
	return maxUndo;
}

void InnerEdit::addUndoNode(uint mode, const string &str, uint x, uint y)
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

void InnerEdit::clearUndo()
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

bool InnerEdit::undoAvailable()
{
	return undoCount>0;
}

bool InnerEdit::redoAvailable()
{
	return redoCount>0;
}

bool InnerEdit::undo()
{
	if(undoCurrent==NULL || undoCount==0)
		return false;

	switch(undoCurrent->mode){
		case UndoNode::ADDED:
		{
			string& str=undoCurrent->text;
			uint y=undoCurrent->y;
			uint x=undoCurrent->x+str.length();
		
			while(x>buffer[y].text.size()){
				x-=buffer[y].text.size()+1;
				++y;
			}

			removeText(undoCurrent->x, undoCurrent->y, x, y, false);
			break;
		}
		case UndoNode::REMOVED:
			insertText(undoCurrent->text, undoCurrent->x, undoCurrent->y, false);
			break;
		case UndoNode::SET_LINE:
		{
			string tmp=buffer[undoCurrent->y].text;
			setLine(undoCurrent->text, undoCurrent->y, false);
			undoCurrent->text=tmp;
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

bool InnerEdit::redo()
{
	if(undoCurrent==NULL || redoCount==0)
		return false;
	
	UndoNode* node=undoCurrent;
	if(undoCount>0)
		node=node->next;


	switch(node->mode){
		case UndoNode::REMOVED:
		{
			string& str=node->text;
			uint y=node->y;
			uint x=node->x+str.length();
		
			while(x>buffer[y].text.size()){
				x-=buffer[y].text.size()+1;
				++y;
			}
	
			removeText(node->x, node->y, x, y, false);
			break;
		}
		case UndoNode::ADDED:
			insertText(node->text, node->x, node->y, false);
			break;
		case UndoNode::SET_LINE:
		{
			string tmp=buffer[node->y].text;
			setLine(node->text, node->y, false);
			node->text=tmp;
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

void InnerEdit::updateWidth(uint y)
{
	float ow=buffer[y].w;
	float nw=getW(buffer[y].text.size(), y);

	buffer[y].w=nw;

	if(ow>=maxWidth && nw<ow){
		updateAllWidths();
	}else{
		if(nw>maxWidth){
			maxWidth=nw;
			updateScrollBars();
		}
	}	
}

void InnerEdit::updateAllWidths()
{
	maxWidth=0;
	for(uint a=0;a<buffer.size()-1;++a){
		buffer[a].w=getW(buffer[a].text.size(), a);
		if(buffer[a].w>maxWidth)
			maxWidth=buffer[a].w;
	}
	updateScrollBars();					
}


void InnerEdit::indentSelection(bool unindent)
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
			const string &line=buffer[a].text;
			
			if(line.size()==0)
				continue;
				
			if(line[0]=='\t'){
				removeText(0, a, 1, a);
			}else if(line[0]==' '){
				uint len=max(tabSize, line.size());
				uint spaces=1;
				for(uint b=1;b<len;++b)
					if(line[b]==' ')
						++spaces;
					else
						break;
				removeText(0, a, spaces, a);
			}//else ignore
		}else{//indent
			if(useTab)
				insertText("\t", 0, a);
			else{
				string pad;
				pad.resize(tabSize, ' ');
				insertText(pad, 0, a);
			}
		}
	}
	
	selectionValid=true;
	selStart.y=top;
	selStart.x=0;
	selEnd.y=bot;
	selEnd.x=buffer[bot].text.size();
	float x=getW(selEnd.x, bot);
	if(cursorY!=bot || cursorX!=x)
		eventBuffer |= CodeView::EI_CURSOR_MOVED;
	
	cursorY=bot;
	cursorX=x;
	
	reformat(top, bot);
	invalidateLines(top, bot);
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

    invalidateLines( 0, buffer.size()-1 );
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













