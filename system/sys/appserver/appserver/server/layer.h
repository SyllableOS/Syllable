/*
 *  The AtheOS application server
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef	__F_LAYER_H__
#define	__F_LAYER_H__

#include <atheos/types.h>

#include <util/resource.h>
#include <gui/region.h>
#include <gui/view.h>
#include <gui/font.h>
#include <gui/point.h>
#include <string>

#include "fontnode.h"

namespace os {
	class Gate;
	struct GRndCopyRect_s;
};

class FontNode;
class SrvBitmap;
class SrvWindow;


os::Color32_s GetDefaultColor( int nIndex );

class LayerGate
{
public:
	LayerGate( const char* pzName )
	{
		m_hSema = create_semaphore( pzName, 100000000, SEM_GLOBAL );
	}
	~LayerGate() { delete_semaphore( m_hSema ); }
	int Lock() const       { return( lock_semaphore( m_hSema ) ); }
	int Unlock() const     { return( unlock_semaphore( m_hSema ) ); }

	int Close() const     { return( lock_semaphore_x( m_hSema, 100000000, 0, INFINITE_TIMEOUT ) ); }
	int Open() const      { return( unlock_semaphore_x( m_hSema, 100000000, 0 ) ); }
	sem_id GetSem() const { return( m_hSema ); }
private:
	sem_id m_hSema;
};


extern LayerGate g_cLayerGate;

#define NUM_FONT_GRAYS 256

class Layer
{
public:
    Layer( SrvWindow* pcWindow, Layer* pcParent, const char* pzName,
	   const os::Rect& cFrame, int nFlags, void* pUserObj );
    Layer( SrvBitmap* pcBitmap );
    void	Init();
  
    virtual ~Layer();

    const char*		GetName( void ) const 		{ return( m_cName.c_str() ); }

    int			GetHandle( void ) const		{ return( m_hHandle );		}
    void*		GetUserObject( void ) const	{ return( m_pUserObject );	}
    void		SetUserObject( void* pObj )	{ m_pUserObject = pObj; 	}
    void		SetWindow( SrvWindow* pcWindow );
    SrvWindow*		GetWindow( void ) const { return( m_pcWindow ); }

    void		Show( bool bFlag );
    bool		IsVisible() const { return( m_nHideCount == 0 ); }
    virtual void AddChild( Layer* pcChild, bool bTopmost );
    virtual void RemoveChild( Layer* pcChild );
    void		RemoveThis( void );
    void		Added( int nHideCount );
    int			GetLevel() const 		{ return( m_nLevel ); }
    Layer*		GetChildAt( os::Point cPos );
    Layer*		GetBottomChild( void ) const	{ return( m_pcBottomChild ); }
    Layer*		GetTopChild( void ) const	{ return( m_pcTopChild ); }
    Layer*		GetHigherSibling( void ) const	{ return( m_pcHigherSibling ); }
    Layer*		GetLowerSibling( void ) const	{ return( m_pcLowerSibling ); }

    Layer*		GetParent( void ) const		{ return( m_pcParent );	}
    Layer*		GetNonTransparentParent( os::IRect& cFrame );

    virtual void UpdateIfNeeded();
	SrvBitmap*	GetBitmap() const { return( m_pcBitmap ); }
    void		SetBitmap( SrvBitmap* pcBitmap );
    

    os::Region*		GetRegion();
    void		PutRegion( os::Region* pcReg );
    bool		IsBackdrop() const { return( m_bBackdrop ); }
    void		MoveToFront();
    void		MoveToBack();
    int			ToggleDepth();

    virtual void	SetFrame( const os::Rect& cRect );
    os::Rect		GetFrame( void ) const 		{ return( m_cFrame ); }
    os::IRect		GetIFrame( void ) const 	{ return( m_cIFrame ); }
    os::Rect		GetBounds( void ) const 	{ return( m_cFrame.Bounds() ); }
    os::IRect		GetIBounds( void ) const 	{ return( m_cIFrame.Bounds() ); }

    void		ScrollBy( const os::Point& cOffset );
  
    os::Point		GetLeftTop( void ) const	{ return( os::Point( m_cFrame.left, m_cFrame.top ) );	}
    os::IPoint		GetILeftTop( void ) const	{ return( os::IPoint( m_cIFrame.left, m_cIFrame.top ) );	}

    void		Invalidate( const os::IRect& cRect, bool bRecursive = false );
    void		Invalidate( bool bRecursive = false );
    void		SetDirtyRegFlags();
	void		SetDirtyRegFlags( const os::IRect& cRect );
	
    void		SetDrawRegion( os::Region* pcReg );
    void		SetShapeRegion( os::Region* pcReg );
    
    void		DeleteRegions();
    virtual void UpdateRegions( bool bForce = true );

    void		BeginUpdate( void );
    void		EndUpdate( void );
    
  	void		ScreenModeChanged();

      // Render functions:
    virtual void	Paint( const os::IRect& cUpdateRect, bool bUpdate = false );


    void		SetFont( FontNode* pcFont );
    os::font_height	GetFontHeight() const;
    void		SetFgColor( int nRed, int nGreen, int nBlue, int nAlpha );
    void		SetFgColor( os::Color32_s sColor );
    void		SetBgColor( int nRed, int nGreen, int nBlue, int nAlpha );
    void		SetBgColor( os::Color32_s sColor );
    void		SetEraseColor( int nRed, int nGreen, int nBlue, int nAlpha );
    void					SetEraseColor( os::Color32_s sColor );
    void		SetDrawingMode( int nMode ) { m_nDrawingMode = nMode; }
    void		DrawFrame( const os::Rect& a_cRect, uint32 nStyle );
    void		MovePenTo( float x, float y )		{ m_cPenPos.x = x; m_cPenPos.y = y;  }
    void		MovePenTo( const os::Point& cPos )	{ m_cPenPos = cPos;  }
    void		MovePenBy( const os::Point& cPos )	{ m_cPenPos += cPos; }
    os::Point		GetPenPosition() const { return( m_cPenPos ); }
    void		DrawLine( const os::Point& cFromPnt, const os::Point& cToPnt ) { m_cPenPos = cFromPnt; DrawLine( cToPnt ); }
    void		DrawLine( const os::Point& cToPnt );

    void		DrawString( const char* pzString, int nLength );
	void		DrawText( const os::Rect& cPos, const char *pzString, int nLength, uint32 nFlags, const os::IPoint& cSel1 = os::IPoint(), const os::IPoint& cSel2 = os::IPoint(), uint32 nMode = 0 );

	void		GetSelection( os::String &cSelection );

    void		CopyRect( os::GRndCopyRect_s* psCmd );

    void		FillRect( os::Rect cRect );
    void		FillRect( os::Rect cRect, os::Color32_s sColor );
    void		EraseRect( os::Rect cRect );
    void		DrawBitMap( SrvBitmap* pcSrcBitmap, os::Rect cSrcRect, os::Rect cDstRect );
    void		ScrollRect( SrvBitmap* pcBitmap, os::Rect cSrcRect, os::Point cDstPos );

      // Coordinate conversions:
    os::Point		ConvertToParent( const os::Point& cPoint ) const;
    void		ConvertToParent( os::Point* pcPoint ) const;
    os::Rect		ConvertToParent( const os::Rect& cRect ) const;
    void		ConvertToParent( os::Rect* pcRect ) const;
    os::Point		ConvertFromParent( const os::Point& cPoint ) const;
    void		ConvertFromParent( os::Point* pcPoint ) const;
    os::Rect		ConvertFromParent( const os::Rect& cRect ) const;
    void		ConvertFromParent( os::Rect* pcRect ) const;
    os::Point		ConvertToRoot( const os::Point& cPoint )	const;
    void		ConvertToRoot( os::Point* pcPoint ) const;
    os::Rect		ConvertToRoot( const os::Rect& cRect ) const;
    void 		ConvertToRoot( os::Rect* pcRect ) const;
    os::Point		ConvertFromRoot( const os::Point& cPoint ) const;
    void		ConvertFromRoot( os::Point* pcPoint ) const;
    os::Rect		ConvertFromRoot( const os::Rect& cRect ) const;
    void		ConvertFromRoot( os::Rect* pcRect ) const;
    os::Point		ConvertToBitmap( const os::Point& cPoint )	const;
    os::Point		GetScrollOffset() const { return( m_cScrollOffset ); }
    os::IPoint		GetIScrollOffset() const { return( m_cIScrollOffset ); }
private:
    friend class SrvWindow;
    friend class TopLayer;
  

    void ClearDirtyRegFlags();
	virtual void RebuildRegion( bool bForce );
    virtual void MoveChilds();
    virtual void InvalidateNewAreas( void );
    void RebuildRedrawRegion( Layer* pcBackbufferedLayer, bool bClearRedrawFlagOnly = false );
protected:
public:
    std::string	m_cName;
    int		m_hHandle;		// The handle passed to applications.
    void*	m_pUserObject;		// Pointer to our partner in the application (in the apps address space)
    int		m_nLevel;		// Hierarchy level. Starting with root as 0
    int		m_nHideCount;
    SrvWindow*	m_pcWindow;		// The windows that own us, or NULL if we are at screen level
    SrvBitmap*	m_pcBitmap;		// The bitmap we are supposed to render to

    os::Rect	m_cFrame;
    os::IRect	m_cIFrame;		// Frame rectangle relative to our parent
    os::Point	m_cScrollOffset;
    os::IPoint	m_cIScrollOffset;

    Layer*	m_pcTopChild;		// Top level child
    Layer*	m_pcBottomChild;	// Bottom level child

    Layer*	m_pcLowerSibling;	// Sibling directly below us
    Layer*	m_pcHigherSibling;	// Sibling directly above us

    Layer*	m_pcParent;		// Daddy
    
    os::IPoint	m_cDeltaMove;		// Relative movement since last region update
    os::IPoint	m_cDeltaSize;		// Relative sizing since last region update

    os::Region* m_pcDrawConstrainReg;	// User specified "local" clipping list.
    os::Region* m_pcShapeConstrainReg;	// User specified clipping list specifying the shape of the layer.

    os::Region* m_pcBitmapReg;		// Region this layer can draw to
    os::Region*	m_pcBitmapFullReg;		// Like the bitmap region, including childrens
    
    os::Region*	m_pcPrevBitmapReg;	// Temporary storage for m_pcBitmapReg during region rebuild
    os::Region*	m_pcPrevBitmapFullReg;	// Temporary storage for m_pcBitmapFullReg during region rebuild
    os::Region*	m_pcDrawReg;		// Only valid between BeginPaint()/EndPaint()
    os::Region*	m_pcDamageReg;		// Containes areas made visible since the last M_PAINT message sendt
    os::Region*	m_pcActiveDamageReg;
    
    uint32	m_nFlags;
    bool	m_bHasInvalidRegs; // True if something made our bitmap region
    bool	m_bIsUpdating;		// True while we paint areas from the damage list
    bool	m_bBackdrop;
    
    bool	m_bNeedsRedraw;
    
    // Members for layers connected to the top screen layer:
    SrvBitmap* m_pcBackbuffer; // If != NULL then this layer has its own backbuffer.
    							// This is only possible for window borders.
    							// The backbuffer is managed by the TopLayer class
    os::Region* m_pcVisibleFullReg; // Visible areas, including child areas
    os::Region* m_pcPrevVisibleFullReg; // The previous visible area. Only valid during updates.
    os::Region* m_pcVisibleDamageReg; // The visible damage region
    bool m_bForceRedraw; // If set then the layer will be redrawn even if not all drawing operations are finished
    
	// Render state:
    os::Point		m_cPenPos;
    os::Color32_s	m_sFgColor;
    os::Color32_s	m_sBgColor;
    os::Color32_s	m_sEraseColor;
    os::Color32_s	m_asFontPallette[NUM_FONT_GRAYS];
	uint32 			m_anFontPalletteConverted[NUM_FONT_GRAYS];
    int				m_nDrawingMode;
    FontNode*		m_pcFont;
    bool			m_bFontPalletteValid;
    bool			m_bIsAddedToFont;
    FontNode::DependencyList_t::iterator	m_cFontViewListIterator;

	os::String m_cSelectionBuffer;
};

Layer* FindLayer( int nToken );

#endif	//	__F_LAYER_H__

