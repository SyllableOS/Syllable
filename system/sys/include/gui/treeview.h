/*  TreeView
 *  Copyright (C) 2003 Syllable Team
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

#ifndef __F_GUI_TREEVIEW_H__
#define __F_GUI_TREEVIEW_H__

#include "listview.h"

#include <gui/image.h>

#include <vector>
#include <string>
#include <functional>

namespace os
{
#if 0	// Fool Emacs auto-indent
}
#endif

class TreeView;

/** Baseclass for TreeView nodes
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
class TreeViewNode : public ListViewRow
{
public:
    TreeViewNode();
    virtual ~TreeViewNode();

	// From ListViewRow
    void		     	AttachToView( View* pcView, int nColumn ) = 0;
    void		     	SetRect( const Rect& cRect, int nColumn ) = 0;
    virtual float 	    GetWidth( View* pcView, int nColumn ) = 0;
    virtual float  	    GetHeight( View* pcView ) = 0;
    virtual void 	    Paint( const Rect& cFrame, View* pcView, uint nColumn,
				    bool bSelected, bool bHighlighted, bool bHasFocus ) = 0;
    virtual bool	    IsLessThan( const ListViewRow* pcOther, uint nColumn ) const = 0;
    virtual bool		HitTest( View* pcView, const Rect& cFrame, int nColumn, Point cPos );

	// From TreeViewNode
	void				SetIndent( uint nIndent );
	uint				GetIndent() const;
	
	void				SetExpanded( bool bExpanded );
	bool				IsExpanded() const;
	
	TreeView*			GetOwner() const;
	bool				IsLastSibling() const;
	
protected:
	// Rendering support functions
	void				_DrawExpanderCross( View *pcView, const Rect& cRect );
	Rect 				_ExpanderCrossPos( const Rect& cFrame ) const;

	friend class TreeView;

	void				_SetOwner( TreeView* pcOwner );

	void				_SetLinePositions( uint32 nLPos );
	uint32				_GetLinePositions( ) const;

	void				_SetLastSibling( bool bIsLast );

private:
	class Private;
	Private *m;
};

/** TreeView node containing strings
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
class TreeViewStringNode : public TreeViewNode
{
public:
    TreeViewStringNode();
    virtual ~TreeViewStringNode();

	// From ListViewRow
    void		     	AttachToView( View* pcView, int nColumn );
    void		     	SetRect( const Rect& cRect, int nColumn );
    virtual float 	    GetWidth( View* pcView, int nColumn );
    virtual float  	    GetHeight( View* pcView );
    virtual void 	    Paint( const Rect& cFrame, View* pcView, uint nColumn,
				    bool bSelected, bool bHighlighted, bool bHasFocus );
    virtual bool	    IsLessThan( const ListViewRow* pcOther, uint nColumn ) const;

	// From TreeViewStringNode
	
    void		     	AppendString( const std::string& cString );
    void	 	     	SetString( int nIndex, const std::string& cString );
    const std::string&	GetString( int nIndex ) const;

	void				SetIcon( Image* pcIcon );
	Image*				GetIcon() const;

private:
	class Private;
	Private *m;
};

/** Flexible multicolumn tree view
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
class TreeView : public ListView
{
public:  
	TreeView( const Rect& cFrame, const char* pzTitle, uint32 nModeFlags = F_MULTI_SELECT | F_RENDER_BORDER,
                    uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP,
                    uint32 nFlags = WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );

    ~TreeView();     

    void			InsertNode( TreeViewNode* pcNode, bool bUpdate = true );
	void			InsertNode( int nPos, TreeViewNode* pcNode, bool bUpdate = true );
    TreeViewNode*	RemoveNode( int nIndex, bool bUpdate = true );
    ListViewRow*	RemoveRow( int nIndex, bool bUpdate = true );

	void			Expand( TreeViewNode* pcNode );
	void			Collapse( TreeViewNode* pcNode );

	bool			HasChildren( TreeViewNode* pcRow );

	TreeViewNode*	GetParent( TreeViewNode* pcNode = NULL );
	TreeViewNode*	GetChild( TreeViewNode* pcNode = NULL );
	TreeViewNode*	GetNext( TreeViewNode* pcNode = NULL );
	TreeViewNode*	GetPrev( TreeViewNode* pcNode = NULL );

	void			GetChildren( std::vector<TreeViewNode*>& cvecChildren, TreeViewNode* pcNode = NULL );
	void			GetSiblings( std::vector<TreeViewNode*>& cvecSiblings, TreeViewNode* pcNode = NULL );

	void			SetExpandedMessage( Message *pcMsg );
	Message*		GetExpandedMessage() const;

	void			SetExpandedImage( Image* pcImage );
	void			SetCollapsedImage( Image* pcImage );
	Image*			GetExpandedImage() const;
	Image*			GetCollapsedImage() const;

	bool			GetDrawExpanderBox() const;
	void			SetDrawExpanderBox( const bool bDraw );

	uint			GetIndentWidth() const;
	void			SetIndentWidth( uint nIndentWidth );

	bool			GetDrawTrunk() const;
	void			SetDrawTrunk( bool bDraw );

	const Rect&		GetExpanderImageBounds( ) const;

    virtual void	SortRows( std::vector<ListViewRow*>* pcRows, int nColumn );

    virtual void	Paint( const Rect& cUpdateRect );

private:

	// It would not be a good idea to add ListViewRows to our tree, so we'll disable these methods:
    void			InsertRow( int nPos, ListViewRow* pcRow, bool bUpdate = true );
    void			InsertRow( ListViewRow* pcRow, bool bUpdate = true );

	void			_ConnectLines();

	class Private;
	Private *m;
};

} // end of namespace


#endif // __F_GUI_TREEVIEW_H__


