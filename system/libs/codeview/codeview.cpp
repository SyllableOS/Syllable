/*	libcodeview.so - A programmers editor widget for Atheos
	Copyright (c) 2001 Andreas Engh-Halstvedt

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

#include "codeview.h"
#include "inneredit.h"

#include <gui/scrollbar.h>

using namespace cv;
using namespace std;

/**Constructor
 * \par Description:
 * The constructor will initialize the CodeView and set all properties
 * to default values.
 *	See the documentaition of each Setxxx() members to see
 *	what the various default values might be.
 *
 * \param r Passed on to the os::View constructor.
 * \param s Passed on to the os::View constructor (only used to 
 * identify the view. Newer rendered anywhere).
 * \param text The initial content of the TextView.
 * \param rm Passed on to the os::View constructor.
 * \param f Passed on to the os::View constructor.
 */
CodeView::CodeView(const os::Rect &r, const os::String &s, const os::String &text, uint32 rm, uint32 f)
: os::Control(r, s, "", NULL, rm, f){
	
	os::Rect bounds=GetBounds();
	bounds.Resize(2, 2, -2, -2);

	vScroll=new os::ScrollBar(os::Rect(), "vScroll", NULL, 0, 0, os::VERTICAL, 
		os::CF_FOLLOW_RIGHT|os::CF_FOLLOW_TOP|os::CF_FOLLOW_BOTTOM);
	hScroll=new os::ScrollBar(os::Rect(), "hScroll", NULL, 0, 0, os::HORIZONTAL, 
		os::CF_FOLLOW_RIGHT|os::CF_FOLLOW_LEFT|os::CF_FOLLOW_BOTTOM);

	float w=vScroll->GetPreferredSize(false).x;

	edit=new InnerEdit(this);
	edit->SetFrame(os::Rect(bounds.left, bounds.top, bounds.right-w, bounds.bottom-w-1));
	AddChild(edit);
	
	vScroll->SetFrame(os::Rect(bounds.right-w, bounds.top, bounds.right, bounds.bottom-w));
	hScroll->SetFrame(os::Rect(bounds.left, bounds.bottom-w-1, bounds.right-w, bounds.bottom));
	AddChild(vScroll);
	AddChild(hScroll);

	vScroll->SetScrollTarget(edit);
	hScroll->SetScrollTarget(edit);

	edit->SetText(text);
	ClearEvents();
}


/**Paints the CodeView.
 * Only the scrollbars and the frame is painted by the CodeView. The
 * text is painted by a child view.
 */
void CodeView::Paint(const os::Rect &r){
	FillRect(r, GetBgColor());

	DrawFrame(GetBounds(), os::FRAME_RECESSED);
}

///Overridden from os::Control
bool CodeView::Invoked(os::Message *m){
	uint32 ev=edit->eventBuffer & edit->eventMask;
	if(ev){
		m->AddInt32("events", ev);
		return true;
	}else
		return false;
}


//******************************
//  TextView emulation methods
//******************************


///Overridden from os::Control
void CodeView::EnableStatusChanged(bool enable){
	edit->SetEnable(enable); 
}

///Overridden from os::View
void CodeView::MakeFocus(bool focus){
	edit->MakeFocus(focus);
}

///Overridden from os::View
bool CodeView::HasFocus(){
	return edit->HasFocus();
}

///Overridden from os::View
void CodeView::FontChanged(os::Font *f){
	edit->SetFont(f); 
}
	
//void CodeView::SetValue(os::Variant value, bool invoke=true);
//os::Variant CodeView::GetValue() const;

/**Sets the readonly flag.
 * <p>Default: false
 */
void CodeView::SetReadOnly(bool flag){
	edit->SetReadOnly(flag); 
}

/**Gets the readonly flag.
 * \sa SetReadOnly()
 */
bool CodeView::GetReadOnly() const{
	return edit->GetReadOnly(); 
}

/**Gets the maximum numer of undo steps kept.
 * \sa SetMaxUndoSize()
 */
int CodeView::GetMaxUndoSize() const{
	return edit->GetMaxUndoSize();
}

/**Sets the maximum number of undo steps to keep.
 * <p>Default: 100
 * <p>Each undo step requires some memory. With a large number of
 * large edit operations this may add up to quite a lot!
 */
void CodeView::SetMaxUndoSize(int size){
	edit->SetMaxUndoSize(size);
}

/**Gets the event mask
 * \sa SetEventMask()
 */
uint32 CodeView::GetEventMask() const{
	return edit->GetEventMask();
}

/**Sets the event mask.
 * <p>Default: 0
 * <p>The event mask determines what events causes the CodeView to invoke
 * itself and send out a message.
 * \sa os::TextView
 */
void CodeView::SetEventMask(uint32 mask){
	edit->SetEventMask(mask);
}

/**Gets the currently selected region.
 * \param buffer The string to put the text into.
 */
void CodeView::GetRegion(os::String *buffer){
	if(!buffer)
		return;

	if(!edit->selectionValid){
		buffer->resize(0);
		return;
	}

	edit->GetText(buffer, edit->selStart, edit->selEnd);
}
		
//void CodeView::SetMinPreferredSize(int widthChars, int heightChars){ edit->setMinPreferredSize(widthChars, heightChars); }
//os::IPoint CodeView::GetMinPreferredSize(){ return edit->getMinPreferredSize(); }
//void CodeView::SetMaxPreferredSize(int widthChars, int heightChars){ edit->setMaxPreferredSize(widthChars, heightChars); }
//os::IPoint CodeView::GetMaxPreferredSize(){ return edit->getMaxPreferredSize(); }

///Ensure the cursor is visible in the view
void CodeView::MakeCsrVisible(){
	edit->ShowCursor();
}

/**Removes all text from the view.
 * \param sendNotify Set to false if no event should be sent.
 */
void CodeView::Clear(bool sendNotify){
	edit->SetText("");
	if(sendNotify) edit->CommitEvents();
}

/**Sets the text in the view.
 * \param text The text to set.
 * \param sendNotify Set to false if no event should be sent.
 */
void CodeView::Set(const char *text, bool sendNotify){
	edit->SetText(text);
	if(sendNotify) edit->CommitEvents();
}

/**Inserts text into the view
 * The text is inserted at the current cursor position.
 * \param text The text to insert.
 * \param sendNotify Set to false if no event should be sent.
 */
void CodeView::Insert(const char *text, bool sendNotify){
	edit->InsertText(text, edit->GetCursor());
	if(sendNotify) edit->CommitEvents();
}			

/**Inserts text into the view
 * \param pos The position to insert the text at.
 * \param text The text to insert.
 * \param sendNotify Set to false if no event should be sent.
 */
void CodeView::Insert(const os::IPoint &pos, const char *text, bool sendNotify){
	edit->InsertText(text, pos);
	if(sendNotify) edit->CommitEvents();
}	
			
/**Selects text in the view.
 * \param start Where to start the selection.
 * \param end Where to end the selection.
 * \param sendNotify Set to false if no event should be sent.
 */
void CodeView::Select(const os::IPoint &start, const os::IPoint &end, bool sendNotify){
	edit->SetSelection(start, end);
	if(sendNotify) edit->CommitEvents();
}

/**Select all text in the view.
 * \param sendNotify Set to false if no event should be sent.
 */
void CodeView::SelectAll(bool sendNotify){
	edit->SelectAll();
	if(sendNotify) edit->CommitEvents();
}

/**Unselects the current selection.
 * \param sendNotify Set to false if no event should be sent.
 */
void CodeView::ClearSelection(bool sendNotify){
	edit->ClearSelection();
	if(sendNotify) edit->CommitEvents();
}

/**Sets the cursor position.
 * \param x The character position to set to.
 * \param y The line to set to.
 * \param select Set to true if the move should select text.
 * \param sendNotify Set to false if no event should be sent.
 */
void CodeView::SetCursor(int x, int y, bool select, bool sendNotify){
	edit->SetCursor(x, y, select);
	if(sendNotify) edit->CommitEvents();
}

/**Sets the cursor position.
 * \param pos The position to set
 * \param select Set to true if the move should select text.
 * \param sendNotify Set to false if no event should be sent.
 */
void CodeView::SetCursor(const os::IPoint &pos, bool select, bool sendNotify){
	edit->SetCursor(pos, select);
	if(sendNotify) edit->CommitEvents();
}

/**Gets the current cursor position.
 * \return The current cursor position.
 */
os::IPoint CodeView::GetCursor() const{
	return edit->GetCursor();
}

/**Gets the length of the text in the view.
 * \return The sum of all line lengths plus line shifts.
 */
size_t CodeView::GetCurrentLength() const{
	return edit->GetLength();
}

/**Cut selected text, and put it on the ClipBoard.
 * \param sendNotify Set to false if no event should be sent.
 */		
void CodeView::Cut(bool sendNotify){
	edit->Cut();
	if(sendNotify) edit->CommitEvents();
}

/**Copy the selected text and put it on the ClipBoard.
 */
void CodeView::Copy(){
	edit->Copy();
}

/**Paste the content of the ClipBoard into the view, replacing any selected text.
 * \param sendNotify Set to false if no event should be sent.
 */		
void CodeView::Paste(bool sendNotify){
	edit->Paste();
	if(sendNotify) edit->CommitEvents();
}

/**Delete the selected text, or current character.
 * \param sendNotify Set to false if no event should be sent.
 */		
void CodeView::Delete(bool sendNotify){
	edit->Del();
	if(sendNotify) edit->CommitEvents();
}

/**Delete text.
 * \param start Where to start the deletion.
 * \param end Where to end the deletion.
 * \param sendNotify Set to false if no event should be sent.
 */		
void CodeView::Delete(const os::IPoint &start, const os::IPoint &end, bool sendNotify){
	edit->RemoveText(start, end);
	if(sendNotify) edit->CommitEvents();
}

///Overridden from os::View or os::Control .
void CodeView::SetTabOrder(int order){
	edit->SetTabOrder(order); 
}
		
//os::Point CodeView::GetPreferredSize(bool largest) const;
//virtual bool CodeView::FilterKeyStroke(const string*);
		
//******************************
//  CodeView special methods
//******************************

/**Sets the current format.
 * <p>Default: NULL
 * <p>The format is not deleted when the view is deleted, so the same %Format 
 * can be used by several views.
 * \sa Format
 */
void CodeView::SetFormat(Format* f){
	edit->SetFormat(f);
}

/**Gets the current format.
 * \sa SetFormat()
 */
Format* CodeView::GetFormat(){
	return edit->GetFormat();
}

/**Sets the color of the text.
 * If a format is set, it will override this.
 *
 * \sa SetFormat()
 */
void CodeView::SetTextColor(os::Color32_s c){
	edit->SetFgColor(c);
}

/**Gets the current text color.
 *
 * \sa SetTextColor()
 */
os::Color32_s CodeView::GetTextColor(){
	return edit->GetFgColor();
}

/**Sets the text background color.
 *
 * SetBgColor() will set the color of the parts of the view that is
 * not covered by editor or scroll bar (i.e. lower right corner).
 */
void CodeView::SetBackground(os::Color32_s c){
	edit->SetBgColor(c);
}

/**Gets the current text background color.
 */
os::Color32_s CodeView::GetBackground(){
	return edit->GetBgColor();
}


///Gets the number of lines in the view.
uint CodeView::GetLineCount() const {
	return edit->GetLineCount();
}

/**Gets a line from the view.
 * \param i The line to get
 * \return Const reference to the given line.
 */
const os::String& CodeView::GetLine(uint i)const{
	return edit->GetLine(i);
}

/**Sets a line.
 * \param text The text to set
 * \param line Which line to set.
 */
void CodeView::SetLine(const os::String &text, uint line){
	edit->SetLine(text.const_str(), line);
}

/**Undoes a previous action - if possible.
 * \param sendNotify Set to false if no event should be sent.
 * \return true if succesfull.
 * \sa UndoAvailable() Redo()
 */		
bool CodeView::Undo(bool sendNotify)
{
	bool r = edit->Undo(); 
	if(sendNotify) edit->CommitEvents();
	return r;
}

/**Redoes an action previously undone - if possible.
 * \param sendNotify Set to false if no event should be sent.
 * \return true if succesfull.
 * \sa RedoAvailable() Undo()
 */		
bool CodeView::Redo(bool sendNotify)
{
	bool r = edit->Redo(); 
	if(sendNotify) edit->CommitEvents();
	return r;
}

/**Is it possible to undo an action?
 * \return true if possible.
 * \sa Undo()
 */
bool CodeView::UndoAvailable() const{
	return edit->UndoAvailable(); 
}

/**Is it possible to redo an action?
 * \return true if possible.
 * \sa Redo() Undo()
 */
bool CodeView::RedoAvailable() const{
	return edit->RedoAvailable(); 
}

/**Sets the tab size.
 * Default: 4
 * \param size How many spaces a tab character should be shown as.
 */
void CodeView::SetTabSize(uint size){
	edit->SetTabSize(size);
}

/**Gets the tab size.
 * \return How many spaces a tab character is shown as.
 * \sa SetTabSize()
 */
uint CodeView::GetTabSize(){
	return edit->GetTabSize();
}

/**Sets if indenting uses tabs or spaces.
 * Default: true
 */
void CodeView::SetUseTab(bool b){
	edit->SetUseTab(b);
}

/**Gets the indent mode.
 * \sa SetUseTab()
 */
bool CodeView::GetUseTab(){
	return edit->GetUseTab();
}

void CodeView::MouseUp(const os::Point& cPoint, uint32 nButton, os::Message* pcMessage)
{
}

/**Clears any buffered events - no notifies will be sent
*/
void CodeView::ClearEvents(){
	edit->eventBuffer=0;
}

void CodeView::SetShowLineNumbers( bool bShowLN )
{
	edit->SetShowLineNumbers( bShowLN );
}

bool CodeView::GetShowLineNumbers()
{
	return edit->GetShowLineNumbers();
}

/**Sets the highlight color.
 *
 * \sa GetHighlightColor()
 */
void CodeView::SetHighlightColor(os::Color32_s c)
{
	edit->SetHighlightColor(c);
}

/**Gets the highlight color.
 *
 * \sa SetHighlightColor()
 */
os::Color32_s CodeView::GetHighlightColor()
{
	return edit->GetHighlightColor();
}

/*Gets the editors font*/
os::Font* CodeView::GetEditorFont() const
{
	return edit->GetFont();
}

/*Sets the editors context menu*/
void CodeView::SetContextMenu( os::Menu * pcMenu )
{
	edit->SetContextMenu( pcMenu );
}

void CodeView::SetLineNumberFgColor(os::Color32_s c)
{
	edit->SetLineNumberFgColor(c);
}

os::Color32_s CodeView::GetLineNumberFgColor()
{
	return edit->GetLineNumberFgColor();
}

void CodeView::SetLineNumberBgColor(os::Color32_s c)
{
	edit->SetLineNumberBgColor(c);
}

os::Color32_s CodeView::GetLineNumberBgColor()
{
	return edit->GetLineNumberBgColor();
}

void CodeView::SetLineBackColor(os::Color32_s c)
{
	edit->SetLineBackColor(c);
}

os::Color32_s CodeView::GetLineBackColor()
{
	return edit->GetLineBackColor();
}

void CodeView::FoldAll()
{
	edit->FoldSection(0,GetLineCount()-1);
}




