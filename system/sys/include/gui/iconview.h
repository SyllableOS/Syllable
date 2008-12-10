/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 The Syllable Team
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
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
 

#ifndef __F_GUI_ICONVIEW_H__
#define __F_GUI_ICONVIEW_H__


#include <gui/control.h>
#include <util/string.h>

#include <vector>
#include <string>
#include <stack>

namespace os
{
#if 0	// Fool Emacs auto-indent
}
#endif

class IconData
{
public:
	IconData() {}
	virtual ~IconData() {}
};


 
class Image;


/** Icon view with different views.
 * \ingroup gui
 * \par Description:
 * The iconview can display icons + descriptions in different ways, e.g. it
 * is used by the os::IconDirectoryView class to display the contents of folders.
 *
 * \sa os::IconDirectoryView
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/

class IconView : public os::Control
{
public:
	enum scroll_direction { SCROLL_UP, SCROLL_DOWN, SCROLL_LEFT, SCROLL_RIGHT };
	
	/** The different view types.
	 * \par Description:
	 * \par VIEW_ICONS - Displays icons which are freely moveable.
	 * \par VIEW_LIST - Displays the icons in a multi-column list.
	 * \par VIEW_DETAILS - Displays the icons in a single-column list with the
	 *                icon strings in the other columns.
	 * \author	Arno Klenke (arno_klenke@yahoo.de)
	 *****************************************************************************/
	enum view_type { VIEW_ICONS, VIEW_LIST, VIEW_DETAILS, VIEW_ICONS_DESKTOP };
	
	IconView( const Rect& cFrame, os::String zName, uint32 nResizeMask );
	~IconView();
	
	virtual void EnableStatusChanged( bool bIsEnabled );
	virtual void Invoked( uint nIcon, os::IconData *pcData );
	virtual void SelectionChanged();
	virtual void DragSelection( os::Point cStartPoint );
	virtual void OpenContextMenu( os::Point cPosition, bool bMouseOverIcon );
	
	view_type GetView() const;
	void SetView( view_type eType );
	void SetTextColor( os::Color32_s sColor );
	void SetTextShadowColor( os::Color32_s sColor );
	void SetSelectionColor( os::Color32_s sColor );
	void SetBackgroundColor( os::Color32_s sColor );
	void SetBackground( os::Image* pcImage );
	void SetSingleClick( bool bSingle );
	bool IsSingleClick() const;
	void SetAutoSort( bool bAutoSort );
	bool IsAutoSort() const;
	void SetMultiSelect( bool bMultiSelect );
	bool IsMultiSelect() const;
	
	void Clear();
	uint AddIcon( os::Image* pcIcon, os::IconData* pcData );
	void RemoveIcon( uint nIcon );
	
	uint GetIconCount();
	
	os::String GetIconString( uint nIcon, uint nString );
	os::Image* GetIconImage( uint nIcon );
	os::IconData* GetIconData( uint nIcon );
	bool GetIconSelected( uint nIcon );
	os::Point GetIconPosition( uint nIcon );
	
	void AddIconString( uint nIcon, os::String zName );
	void SetIconString( uint nIcon, uint nString, os::String zString );
	void SetIconImage( uint nIcon, os::Image* pcImage );
	void SetIconSelected( uint nIcon, bool bSelected, bool bDeselectAll = true );
	void SetIconPosition( uint nIcon, os::Point cPosition );
	

	os::Point GetIconSize();
	
	void RenderIcon( uint nIcon, os::View* pcView, os::Point cPosition );
	void RenderIcon( os::String zName, os::Image* pcImage, os::View* pcView, os::Point cPosition );
	
	void Layout();
	
	os::Point ConvertToView( os::Point cPoint );
	
	void StartScroll( scroll_direction eDirection );
	void StopScroll();
	void ScrollToIcon( uint nIcon );
	
	void SetSelChangeMsg( os::Message* pcMessage );
	void SetInvokeMsg( os::Message* pcMessage );
	virtual os::Message* GetSelChangeMsg();
	virtual os::Message* GetInvokeMsg();

	virtual void MakeFocus( bool bFocus = true );
	
	virtual void SetTabOrder( int nOrder = NEXT_TAB_ORDER );
	virtual int GetTabOrder() const;
	virtual int GetTabOrder();		/* NOTE: This should be removed in the next libsyllable version. */
private:
	virtual void __ICV_reserved2__();
	virtual void __ICV_reserved3__();
	virtual void __ICV_reserved4__();
	virtual void __ICV_reserved5__();
private:
	IconView& operator=( const IconView& );
	IconView( const IconView& );

	class MainView;
	class Private;
	Private* m;
};


} // End of namespace

#endif





