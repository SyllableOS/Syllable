/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999 - 2001  Kurt Skauen
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

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class LayoutNode
{
public:
    LayoutNode( const std::string& cName, float vWheight = 1.0f, LayoutNode* pcParent = NULL, View* pcView = NULL );
    virtual ~LayoutNode();

    virtual void		    SetView( View* pcView );
    virtual void		    Layout();
    
    virtual void		    SetBorders( const Rect& cBorder );
    virtual Rect		    GetBorders() const;

    float			    GetWheight() const;
    void			    SetWheight( float vWheight );
    virtual void 		    SetFrame( const Rect& cFrame );
    virtual Rect		    GetFrame() const;
    virtual Rect		    GetBounds() const;
    Rect		    	    GetAbsFrame() const;

    void			    ExtendMinSize( const Point& cMinSize );
    void			    LimitMaxSize( const Point& cMaxSize );
    void			    ExtendMaxSize( const Point& cMaxSize );

    void			    SetHAlignment( alignment eAlignment );
    void			    SetVAlignment( alignment eAlignment );
    alignment			    GetHAlignment() const;
    alignment			    GetVAlignment() const;
    
    void			    AdjustPrefSize( Point* pcMinSize, Point* pcMaxSize );
    virtual Point		    GetPreferredSize( bool bLargest );
    void			    AddChild( LayoutNode* pcChild );
    LayoutNode*			    AddChild( View* pcChild, float vWheight = 1.0f );
    void			    RemoveChild( LayoutNode* pcChild );
    void			    RemoveChild( View* pcChild );

    std::string			    GetName() const;
    const std::vector<LayoutNode*>& GetChildList() const;
    LayoutNode*			    GetParent() const;
    LayoutView*			    GetLayoutView() const;
    LayoutNode* FindNode( const std::string& cName, bool bRequrcive = true, bool bIncludeSelf = false );

    void SameWidth( const char* pzName1, ... );
    void SameHeight( const char* pzName1, ... );
    
    void SetBorders( const Rect& cBorders, const char* pzFirstName, ... );
    void SetWheights( float vWheight, const char* pzFirstName, ... );
    void SetHAlignments( alignment eAlign, const char* pzFirstName, ... );
    void SetVAlignments( alignment eAlign, const char* pzFirstName, ... );

    void AddToWidthRing( LayoutNode* pcRing );
    void AddToHeightRing( LayoutNode* pcRing );
    
protected:    
    virtual Point    CalculatePreferredSize( bool bLargest );
private:
    friend class LayoutView;
    void	_AddedToView( LayoutView* pcView );
    void	_AddedToParent( LayoutNode* pcParent );
    void	_RemovedFromParent();

    struct ShareNode {
	LayoutNode* m_pcNext;
	LayoutNode* m_pcPrev;
    };
    std::string		     m_cName;
    Rect		     m_cFrame;
    Rect		     m_cBorders;
    View*		     m_pcView;
    LayoutNode*		     m_pcParent;
    LayoutView*		     m_pcLayoutView;
    std::vector<LayoutNode*> m_cChildList;
    float		     m_vWheight;
    Point		     m_cMinSize;
    Point		     m_cMaxSizeExtend;
    Point		     m_cMaxSizeLimit;

    alignment		     m_eHAlign;
    alignment		     m_eVAlign;
    
    ShareNode		     m_sWidthRing;
    ShareNode		     m_sHeightRing;
};

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class LayoutSpacer : public LayoutNode
{
public:
    LayoutSpacer( const std::string& cName, float vWheight = 1.0f, LayoutNode* pcParent = NULL,
		  const Point& cMinSize = Point(0.0f,0.0f), const Point& cMaxSize = Point(MAX_SIZE,MAX_SIZE) );

    void SetMinSize( const Point& cSize );
    void SetMaxSize( const Point& cSize );

private:
    virtual Point    CalculatePreferredSize( bool bLargest );

    Point m_cMinSize;
    Point m_cMaxSize;
};

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class VLayoutSpacer : public LayoutSpacer
{
public:
    VLayoutSpacer( const std::string& cName, float vMinHeight = 0.0f, float vMaxHeight = MAX_SIZE,
		   LayoutNode* pcParent = NULL, float vWheight = 1.0f ) :
	    LayoutSpacer( cName, vWheight, pcParent, Point( 0.0f, vMinHeight ), Point( 0.0f, vMaxHeight ) )
    {}
};

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class HLayoutSpacer : public LayoutSpacer
{
public:
    HLayoutSpacer( const std::string& cName, float vMinWidth = 0.0f, float vMaxWidth = MAX_SIZE,
		   LayoutNode* pcParent = NULL, float vWheight = 1.0f ) :
	    LayoutSpacer( cName, vWheight, pcParent, Point( vMinWidth, 0.0f ), Point( vMaxWidth, 0.0f ) )
    {}
};

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class HLayoutNode : public LayoutNode
{
public:
    HLayoutNode( const std::string& cName, float vWheight = 1.0f, LayoutNode* pcParent = NULL, View* pcView = NULL );
//    virtual Point GetPreferredSize( bool bLargest );
    virtual void  Layout();
private:
    virtual Point    CalculatePreferredSize( bool bLargest );
};

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class VLayoutNode : public LayoutNode
{
public:
    VLayoutNode( const std::string& cName, float vWheight = 1.0f, LayoutNode* pcParent = NULL, View* pcView = NULL );
//    virtual Point GetPreferredSize( bool bLargest );
    virtual void  Layout();

protected:
    virtual Point    CalculatePreferredSize( bool bLargest );
private:
};

/** Main class in the AtheOS dynamic layout system
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class LayoutView : public View
{
public:
    LayoutView( const Rect& cFrame, const std::string& cTitle, LayoutNode* pcRoot = NULL,
		uint32 nResizeMask = CF_FOLLOW_ALL, uint32 nFlags = WID_WILL_DRAW | WID_CLEAR_BACKGROUND );

    LayoutNode* GetRoot() const;
    void	SetRoot( LayoutNode* pcRoot );

    LayoutNode* FindNode( const std::string& cName, bool bRequrcive = true );

    void	InvalidateLayout();
    
    virtual Point	GetPreferredSize( bool bLargest ) const;
    virtual void	AllAttached();
    virtual void	FrameSized( const Point& cDelta );
private:
    LayoutNode* m_pcRoot;
};



} // end of namespace


#endif // __F_GUI_LAYOUTVIEW_H__
