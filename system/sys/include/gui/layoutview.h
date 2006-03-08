/*  libatheos.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001  Kurt Skauen
 *  Copyright (C) 2003 - 2004  The Syllable Team
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

#ifndef	__F_GUI_LAYOUTVIEW_H__
#define	__F_GUI_LAYOUTVIEW_H__

#include <gui/view.h>

#include <vector>

namespace os
{
#if 0	// Fool Emacs auto-indent
}
#endif

#define MAX_SIZE 100000.0f


class LayoutView;

/** Layout node
 * \ingroup gui
 * \par Description:
 *	This class is a base class for those classes that arrange GUI items by
 *	a specific algorithm, such as horizontal layout or vertical layout.
 *	Unless you intend to create such an algorithm yourself, you will not use
 *	this class directly, but instead one of its subclasses (HLayoutNode or
 *	VLayoutNode).
 * \par Example:
 * \code
 *	HLayoutNode* pcRoot = HLayoutNode( "pcRoot" );
 *
 *	pcRoot->AddChild( new Button( Rect(), "Button1", "One", new Message( ID_ONE ) ), 2.0 );
 *	pcRoot->AddChild( new HLayoutSpacer( "Spacer" ) );
 *	pcRoot->AddChild( new Button( Rect(), "Button2", "Two", new Message( ID_TWO ) ), 1.0 );
 *
 *	LayoutView* pcLayoutView = new LayoutView( pcMyWindow->GetBounds(), "pcLayoutView" );
 *	pcLayoutView->SetRoot( pcRoot );
 *	pcMyWindow->AddChild( pcLayoutView );
 * \endcode
 *	This example will result in a layout like this:\n
 *	[One][Two]\n
 *	If the window is made larger, the weights and the spacer will cause the
 *	layout to look like this:\n
 *	[  One  ]   [Two]\n
 *	That is, "One" has twice the weight of the two others, thus it will get
 *	twice the amount of excess screen space.
 * \sa VLayoutNode, HLayoutNode, LayoutSpacer, SameWidth, SameHeight
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
class LayoutNode
{
public:
    LayoutNode( const String& cName, float vWeight = 1.0f, LayoutNode* pcParent = NULL, View* pcView = NULL );
    virtual ~LayoutNode();

    virtual void SetView( View* pcView );
    View* GetView() const;
    virtual void Layout();
    
    virtual void SetBorders( const Rect& cBorder );
    virtual Rect GetBorders() const;

    void ExtendMinSize( const Point& cMinSize );
    void LimitMaxSize( const Point& cMaxSize );
    void ExtendMaxSize( const Point& cMaxSize );

    float GetWeight() const;
    void SetWeight( float vWeight );
    virtual void SetFrame( const Rect& cFrame );
    virtual Rect GetFrame() const;
    virtual Rect GetBounds() const;
    Rect GetAbsFrame() const;

    void SetHAlignment( alignment eAlignment );
    void SetVAlignment( alignment eAlignment );
    alignment GetHAlignment() const;
    alignment GetVAlignment() const;
    
    void AdjustPrefSize( Point* pcMinSize, Point* pcMaxSize );
    virtual Point GetPreferredSize( bool bLargest );
    void AddChild( LayoutNode* pcChild );
    LayoutNode* AddChild( View* pcChild, float vWeight = 1.0f );
    void RemoveChild( LayoutNode* pcChild );
    void RemoveChild( View* pcChild );

    String GetName() const;
    const std::vector<LayoutNode*>& GetChildList() const;
    LayoutNode*	GetParent() const;
    LayoutView* GetLayoutView() const;
    LayoutNode* FindNode( const String& cName, bool bRecursive = true, bool bIncludeSelf = false );

    void SameWidth( const char* pzName1, ... );
    void SameHeight( const char* pzName1, ... );
    
    void SetBorders( const Rect& cBorders, const char* pzFirstName, ... );
    void SetWeights( float vWeight, const char* pzFirstName, ... );
    void SetHAlignments( alignment eAlign, const char* pzFirstName, ... );
    void SetVAlignments( alignment eAlign, const char* pzFirstName, ... );

    void AddToWidthRing( LayoutNode* pcRing );
    void AddToHeightRing( LayoutNode* pcRing );
  
protected:    
    virtual Point    CalculatePreferredSize( bool bLargest );
private:
    virtual void	__LYN_reserved1__();
    virtual void	__LYN_reserved2__();
    virtual void	__LYN_reserved3__();
    virtual void	__LYN_reserved4__();
    virtual void	__LYN_reserved5__();
private:
    LayoutNode& operator=( const LayoutNode& );
    LayoutNode( const LayoutNode& );

    friend class LayoutView;
    void	_AddedToView( LayoutView* pcView );
    void	_AddedToParent( LayoutNode* pcParent );
    void	_RemovedFromParent();
	void	_RemovedFromView();
	
	class Private;
	Private *m;
};

/** Layout spacer
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class LayoutSpacer : public LayoutNode
{
public:
    LayoutSpacer( const String& cName, float vWeight = 1.0f, LayoutNode* pcParent = NULL,
		  const Point& cMinSize = Point(0.0f,0.0f), const Point& cMaxSize = Point(MAX_SIZE,MAX_SIZE) );

    void SetMinSize( const Point& cSize );
    void SetMaxSize( const Point& cSize );
	
	Point GetMinSize() const;
	Point GetMaxSize() const;
private:
    LayoutSpacer& operator=( const LayoutSpacer& );
    LayoutSpacer( const LayoutSpacer& );

    virtual Point    CalculatePreferredSize( bool bLargest );

    Point m_cMinSize;
    Point m_cMaxSize;
    uint32 _reserved[4];
};

/** Vertical layout spacer
 * \ingroup gui
 * \par Description:
 *	Creates a blank space in a vertical layout.
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class VLayoutSpacer : public LayoutSpacer
{
public:
    VLayoutSpacer( const String& cName, float vMinHeight = 0.0f, float vMaxHeight = MAX_SIZE,
		   LayoutNode* pcParent = NULL, float vWeight = 1.0f ) :
	    LayoutSpacer( cName, vWeight, pcParent, Point( 0.0f, vMinHeight ), Point( 0.0f, vMaxHeight ) )
    {}

private:
    VLayoutSpacer& operator=( const VLayoutSpacer& );
    VLayoutSpacer( const VLayoutSpacer& );
};

/** Horizontal layout spacer
 * \ingroup gui
 * \par Description:
 *	Creates a blank space in a horizontal layout.
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class HLayoutSpacer : public LayoutSpacer
{
public:
    HLayoutSpacer( const String& cName, float vMinWidth = 0.0f, float vMaxWidth = MAX_SIZE,
		   LayoutNode* pcParent = NULL, float vWeight = 1.0f ) :
	    LayoutSpacer( cName, vWeight, pcParent, Point( vMinWidth, 0.0f ), Point( vMaxWidth, 0.0f ) )
    {}

private:
    HLayoutSpacer& operator=( const HLayoutSpacer& );
    HLayoutSpacer( const HLayoutSpacer& );
};

/** Horizontal Layout class
 * \ingroup gui
 * \par Description:
 *	This class is used whenever GUI elements are to be laid out alongside each
 *	other (in a horizontal faishon).
 * \sa LayoutNode
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class HLayoutNode : public LayoutNode
{
public:
    HLayoutNode( const String& cName, float vWeight = 1.0f, LayoutNode* pcParent = NULL, View* pcView = NULL );
//    virtual Point GetPreferredSize( bool bLargest );
    virtual void  Layout();

protected:
    virtual Point    CalculatePreferredSize( bool bLargest );

private:
    virtual void	__LHLN_reserved1__();
    virtual void	__LHLN_reserved2__();
    virtual void	__LHLN_reserved3__();
private:
    HLayoutNode& operator=( const HLayoutNode& );
    HLayoutNode( const HLayoutNode& );

	uint32	_reserved[4];
};

/** Vertical Layout class
 * \ingroup gui
 * \par Description:
 *	This class is used whenever GUI elements are to be laid out on top of each
 *	other (in a vertical faishon).
 * \sa LayoutNode
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class VLayoutNode : public LayoutNode
{
public:
    VLayoutNode( const String& cName, float vWeight = 1.0f, LayoutNode* pcParent = NULL, View* pcView = NULL );
//    virtual Point GetPreferredSize( bool bLargest );
    virtual void  Layout();

protected:
    virtual Point    CalculatePreferredSize( bool bLargest );

private:
    virtual void	__LVLN_reserved1__();
    virtual void	__LVLN_reserved2__();
    virtual void	__LVLN_reserved3__();
private:
    VLayoutNode& operator=( const VLayoutNode& );
    VLayoutNode( const VLayoutNode& );

	uint32	_reserved[4];
};

/** Main class in the Syllable dynamic layout system
 * \ingroup gui
 * \par Description:
 *	This is the main layout class, to which a tree of LayoutNodes are added,
 *	using SetRoot().
 *	The LayoutView itself is added to a Window, or another
 *	View, and will display its contents in that Window or View.
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class LayoutView : public View
{
public:
    LayoutView( const Rect& cFrame, const String& cTitle, LayoutNode* pcRoot = NULL,
		uint32 nResizeMask = CF_FOLLOW_ALL, uint32 nFlags = WID_WILL_DRAW | WID_CLEAR_BACKGROUND );
	virtual ~LayoutView();
	
    LayoutNode* GetRoot() const;
    void	SetRoot( LayoutNode* pcRoot );

    LayoutNode* FindNode( const String& cName, bool bRecursive = true );

    void	InvalidateLayout();
    
    virtual Point	GetPreferredSize( bool bLargest ) const;
    virtual void	AllAttached();
    virtual void	FrameSized( const Point& cDelta );

private:
    virtual void	__LYV_reserved1__();
    virtual void	__LYV_reserved2__();
    virtual void	__LYV_reserved3__();
    virtual void	__LYV_reserved4__();
    virtual void	__LYV_reserved5__();

private:
    LayoutView& operator=( const LayoutView& );
    LayoutView( const LayoutView& );

	class Private;
	Private *m;

};



} // end of namespace


#endif // __F_GUI_LAYOUTVIEW_H__

