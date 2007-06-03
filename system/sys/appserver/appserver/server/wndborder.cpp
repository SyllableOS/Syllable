
/*
 *  The AtheOS application server
 *  Copyright (C) 1999 - 2000 Kurt Skauen
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

#include "wndborder.h"
#include "swindow.h"
#include "sfont.h"
#include "server.h"
#include "sprite.h"
#include "windowdecorator.h"
#include "sapplication.h"
#include "inputnode.h"
#include "bitmap.h"

#include <util/locker.h>
#include <util/message.h>
#include <util/messenger.h>
#include <gui/window.h>

using namespace os;


#define H_POINTER_WIDTH  20
#define H_POINTER_HEIGHT 9
static uint8 g_anHMouseImg[] = {
	0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x03, 0x02, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x02, 0x03, 0x00, 0x00,
	0x00, 0x03, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x02, 0x02, 0x02, 0x03, 0x00,
	0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03,
	0x00, 0x03, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x02, 0x02, 0x02, 0x03, 0x00,
	0x00, 0x00, 0x03, 0x02, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x02, 0x03, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00,
};


#define V_POINTER_WIDTH  9
#define V_POINTER_HEIGHT 20
static uint8 g_anVMouseImg[] = {
	0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x03, 0x02, 0x02, 0x02, 0x03, 0x00, 0x00,
	0x00, 0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x00,
	0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03,
	0x03, 0x03, 0x03, 0x03, 0x02, 0x03, 0x03, 0x03, 0x03,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x03, 0x03, 0x03, 0x03, 0x02, 0x03, 0x03, 0x03, 0x03,
	0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03,
	0x00, 0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x00,
	0x00, 0x00, 0x03, 0x02, 0x02, 0x02, 0x03, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
};

#define DIAG_POINTER_WIDTH  16
#define DIAG_POINTER_HEIGHT 16

static uint8 g_anLTMouseImg[] = {
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x02, 0x02, 0x02, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x02, 0x02, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x02, 0x02, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x02, 0x03, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x03, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00, 0x03, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x03, 0x02, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x02, 0x02, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x02, 0x02, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x02, 0x02, 0x02, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
};


static uint8 g_anLBMouseImg[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x02, 0x02, 0x02, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x02, 0x02, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x02, 0x02, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x03, 0x02, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00, 0x03, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
	0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x03, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x02, 0x03, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x02, 0x02, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x02, 0x02, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x02, 0x02, 0x02, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};


/** This function adds the decorator border to the rect passed to it
 * \par Description: This function takes an os::Rect and adds the borders of
 * 					 the WindowDecorator to it
 * \param cRect - an os::Rect which is used to calculate the new client rect
 * \param pcDecorator - an os::WindowDecorator* that is used to get the border size
 * \author Kurt Skauen
  *****************************************************************************/
static inline Rect RectToClient( const Rect & cRect, WindowDecorator * pcDecorator )
{
	Rect cBorders = pcDecorator->GetBorderSize();
	return ( Rect( cRect.left + cBorders.left, cRect.top + cBorders.top, cRect.right - cBorders.right, cRect.bottom - cBorders.bottom ) );
}


/** Retuns an iteger value based on the two values passed
 * \par Description:
 *  This function does an iteger alignment on two values
 *  This is useful to get the best integer possible.  It takes two values(value, align)
 * and does value/align*align.  
 * \note:
		Remember this is integer division, so 1025/8*8 would give you 1024
 * \author Kurt Skauen
  *****************************************************************************/
static int Align( int nVal, int nAlign )
{
	//align = the value / the alignment * alignment
	//this took me for a loop for a while. 
	//remember this is integer division, so (nVal / nAlign) * nAlign will give you the rounded
	//integer(e.g., 1025/8*8 = 125*8 = 1024)
	int align = (nVal / nAlign) * nAlign;
	return (align);
}

/** 
 * \par This function determines where a value and a delta are together at 
 * \author Kurt Skauen
  *****************************************************************************/
static int Clamp( int nVal, int nDelta, int nMin, int nMax )
{
	int compare = nVal + nDelta;
	int nReturn;
	
	
	if( compare < nMin )
	{
		nReturn = (nMin - nVal);
	}
	else if( compare > nMax )
	{
		nReturn = (nMax - nVal);
	}
	else
	{
		nReturn = nDelta;
	}
	return nReturn;
}

/** Constructor
 * \param SrvWindow*  -  pointer to window (should not be null)
 * \param os::Layer*  -  pointer to parent (usually a app and really shouldn't be null)
 * \param const char* -  the name of border
 * \param bool        -  whether or not the window has a backdrop
 * \author Kurt Skauen
 * \author Rick Caudill(modified and documented)
 *****************************************************************************/
WndBorder::WndBorder( SrvWindow * pcWindow, Layer * pcParent, const char *pzName, bool bBackdrop ) :Layer( pcWindow, pcParent, pzName, Rect( 0, 0, 0, 0 ), 0, NULL ), m_cAlignSize( 1, 1 ), m_cAlignSizeOff( 0, 0 ),m_cAlignPos( 1, 1 ), m_cAlignPosOff( 0, 0 ), m_cMinSize( 0, 0 ), m_cMaxSize( INT_MAX, INT_MAX )
{
	//set both of the hits to none
	m_eHitItem = WindowDecorator::HIT_NONE;
	m_eCursorHitItem = WindowDecorator::HIT_NONE;
	
	//set whether the frame was maximized to false
	m_bFrameMaxHit = m_bWndMovePending = false;
	
	m_bBackdrop = bBackdrop;
	
	//for each item in number of buttons on decorator
	for( int i = 0; i < WindowDecorator::HIT_MAX_ITEMS; i++ )
	{
		m_nButtonDown[i] = 0; //set the button to 0
	}


	SetFont( AppServer::GetInstance()->GetWindowTitleFont(  ) );
	m_pcDecorator = NULL;
	m_pcClient = new Layer( pcWindow, this, "wnd_client", Rect( 0, 0, 0, 0 ), 0, NULL );

}

/** Destructor
 * \par Description:
 *  Destructor
 * \author Kurt Skauen
  *****************************************************************************/
WndBorder::~WndBorder()
{
}

/** Sets a new os::WindowDecorator
 * \par Description:
 *  This function will set the new window decorator.
 * \param os::WindowDecorator* - pointer to the new decorator
 * \author Kurt Skauen
 * \author Rick Caudill(modified and documented)
 *****************************************************************************/
void WndBorder::SetDecorator( os::WindowDecorator * pcDecorator )
{
	//make sure the shaping region is nulled
	SetShapeRegion( NULL );
	
	//then if the decorator is valid, delete it
	if( m_pcDecorator )
		delete m_pcDecorator;

	//set the new decorator
	m_pcDecorator = pcDecorator;
}


/** Gets the current os::WindowDecorator
 * \par Description:
 *  This function will get the window decorator.
 * \author Kurt Skauen
 * \author Rick Caudill(modified and documented)
 *****************************************************************************/
os::WindowDecorator * WndBorder::GetDecorator() const
{
	return ( m_pcDecorator );
}


/** Sets the current window flags
 * \par Description:
 *  This function will set the window's flags.
 * \author Kurt Skauen
 * \author Rick Caudill(modified and documented)
 *****************************************************************************/
void WndBorder::SetFlags( uint32 nFlags )
{
	m_pcDecorator->SetFlags( nFlags );
}

/**internal*/
void WndBorder::DoSetFrame( const Rect & cRect )
{
	//notify the topview that the layer's frame has changed
	g_pcTopView->LayerFrameChanged( this, cRect );
	
	//set the new frame
	Layer::SetFrame( cRect );
	m_pcClient->SetFrame( RectToClient( GetBounds(), m_pcDecorator ) );
	
	//resize the decorator with the new bounds
	m_pcDecorator->FrameSized( GetBounds() );
}

/** Sets the frame of the window
 * \par Description:
 *  This function will set the frame of a window.
 * \author Kurt Skauen
 * \author Rick Caudill(modified and documented)
 *****************************************************************************/
void WndBorder::SetFrame( const Rect & cRect )
{
	//the raw frame becomes the frame we set to
	m_cRawFrame = cRect;
	
	//we call dosetframe with this rectangle
	DoSetFrame( cRect );
}

/** Sets the size limits of the window
 * \par Description:
 *  This function will set the size limits of the window.
 * \author Kurt Skauen
 * \author Rick Caudill(modified and documented)
 *****************************************************************************/
void WndBorder::SetSizeLimits( const Point & cMinSize, const Point & cMaxSize )
{
	//the minimum size of the window is set
	m_cMinSize = static_cast < IPoint > ( cMinSize );
	
	//the maximum size of the window is set
	m_cMaxSize = static_cast < IPoint > ( cMaxSize );
}


/** Gets the size limits of the window
 * \par Description:
 *  This function will get the size limits of a window.
 * \param Point* - a pointer to which we set the minimum size
 * \param Point* - a pointer to which we set the max size
 * \author Kurt Skauen
 * \author Rick Caudill(modified and documented)
 *****************************************************************************/
void WndBorder::GetSizeLimits( Point * pcMinSize, Point * pcMaxSize )
{
	//the passed min size is set to the min size of the window
	*pcMinSize = static_cast < Point > ( m_cMinSize );
	
	//the passed max size is set to the max size of the window
	*pcMaxSize = static_cast < Point > ( m_cMaxSize );
}

/** Sets the alignment of the window
 * \par Description:
 *  This function will set the alignment of the window
 * \param IPoint - a integer point that sets the size
 * \param IPoint - a point to which we set the size offset
 * \param IPoint - a point that controls the alignment position
 * \param IPoint - a point that controls the position offset
 * \author Kurt Skauen
 * \author Rick Caudill(modified and documented)
 *****************************************************************************/
void WndBorder::SetAlignment( const IPoint & cSize, const IPoint & cSizeOffset, const IPoint & cPos, const IPoint & cPosOffset )
{
	m_cAlignSize = cSize;
	m_cAlignPos = cPos;

	m_cAlignSizeOff.x = cSizeOffset.x % cSize.x;
	m_cAlignSizeOff.y = cSizeOffset.y % cSize.y;

	m_cAlignPosOff.x = cPosOffset.x % cPos.x;
	m_cAlignPosOff.y = cPosOffset.y % cPos.y;
}


/** Align the frame of the window.
 * \par Description:
 *  This function returns an aligned frame.
 *  This function calls AlignRect() 
 * \param Rect - an os::Rect containing the fame
 * \author Kurt Skauen
 * \author Rick Caudill(modified and documented)
 *****************************************************************************/
os::Rect WndBorder::AlignFrame( const os::Rect & cFrame )
{
	return( static_cast < IRect > (AlignRect( static_cast < IRect > (cFrame), static_cast < IRect > (m_pcDecorator->GetBorderSize()) ) ) );
}

/**internal*/
void WndBorder::Paint( const IRect & cUpdateRect, bool bUpdate )
{
	//first step is to get the window flags
	uint32 nFlags = m_pcWindow->GetFlags();

	//if there isn't a window border then just exit
	if( nFlags & WND_NO_BORDER )
	{
		return;
	}
	
	//if bUpdate is set then we call
	if( bUpdate )
	{
		BeginUpdate();
	}
	
	//render the decorator with the updated rect
	m_pcDecorator->Render( static_cast < Rect > ( cUpdateRect ) );
	
	//if we pass update then we call endupdate
	if( bUpdate )
	{
		EndUpdate();
	}
	
	//make sure that we say that the window needs a redraw
	m_bNeedsRedraw = true;
}

/** Align the Rect's passed to it.
 * \par Description:
 *  This function returns an aligned frame.
 *  This function calls AlignRect() 
 * \param Rect - an os::Rect containing the fame
 * \author Kurt Skauen
 * \author Rick Caudill(modified and documented)
 *****************************************************************************/
IRect WndBorder::AlignRect( const IRect & cRect, const IRect & cBorders )
{
	IRect cAFrame;

	//the border width is the left side + right side
	int nBorderWidth = cBorders.left + cBorders.right;
	
	//the border height is the top + bottom
	int nBorderHeight = cBorders.top + cBorders.bottom;

	//the left side of the frame is set to 
	cAFrame.left = ( ( cRect.left + cBorders.left ) / m_cAlignPos.x ) * m_cAlignPos.x + m_cAlignPosOff.x - cBorders.left;
	cAFrame.top = ( ( cRect.top + cBorders.top ) / m_cAlignPos.y ) * m_cAlignPos.y + m_cAlignPosOff.y - cBorders.top;

	cAFrame.right = cAFrame.left + ( ( cRect.Width() - nBorderWidth ) / m_cAlignSize.x ) * m_cAlignSize.x + m_cAlignSizeOff.x + nBorderWidth;
	cAFrame.bottom = cAFrame.top + ( ( cRect.Height() - nBorderHeight ) / m_cAlignSize.y ) * m_cAlignSize.y + m_cAlignSizeOff.y + nBorderHeight;
	return ( cAFrame );
}


/** Returns the point that was approximately hit
 * \par Description:
 *  This function returns the point that was approximately hit
 * \author Kurt Skauen
 * \author Rick Caudill(modified and documented)
 *****************************************************************************/
IPoint WndBorder::GetHitPointBase() const
{
	IPoint cPoint;
	
	
	switch ( m_eHitItem )
	{
	case WindowDecorator::HIT_SIZE_LEFT:
		cPoint = IPoint( 0, 0 );
		break;
	case WindowDecorator::HIT_SIZE_RIGHT:
		cPoint =  IPoint( m_cRawFrame.Width() + m_cDeltaSize.x, 0 );
		break;
	case WindowDecorator::HIT_SIZE_TOP:
		cPoint = IPoint( 0, 0 );
		break;
	case WindowDecorator::HIT_SIZE_BOTTOM:
		cPoint = IPoint( 0, m_cRawFrame.Height() + m_cDeltaSize.y );
		break;
	case WindowDecorator::HIT_SIZE_LT:
		cPoint = IPoint( 0, 0 );
		break;
	case WindowDecorator::HIT_SIZE_RT:
		cPoint = IPoint( m_cRawFrame.Width() + m_cDeltaSize.x, 0 );
		break;
	case WindowDecorator::HIT_SIZE_RB:
		cPoint = IPoint( m_cRawFrame.Width() + m_cDeltaSize.x, m_cRawFrame.Height(  ) + m_cDeltaSize.y );
		break;
	case WindowDecorator::HIT_SIZE_LB:
		cPoint = IPoint( 0, m_cRawFrame.Height() + m_cDeltaSize.y );
		break;
	default:
		cPoint = IPoint( 0, 0 );
		break;
	}
	return cPoint;
}

/** Determines if the mouse was moved on the border
 * \par Description:
 *  This function will tell us what part of the border was moved on to
 * \param Messenger* - the application pointer
 * \param Point& - the new position that was moved into
 * \param int - the transit code
 * \author Kurt Skauen
 * \author Rick Caudill(modified and documented)
 *****************************************************************************/
bool WndBorder::MouseMoved( Messenger * pcAppTarget, const Point & cNewPos, int nTransit )
{
	IPoint cHitPos;

	//if we didn't hit a part of the decorator and the transit code is exit
	if( m_eHitItem == WindowDecorator::HIT_NONE && nTransit == MOUSE_EXITED )
	{
		//make the cursor code none
		m_eCursorHitItem = WindowDecorator::HIT_NONE;
		
		//restore the correct cursor
		SrvApplication::RestoreCursor();
		
		//exit
		return ( false );
	}

	// Caclculate the window borders
	IRect cBorders( 0, 0, 0, 0 );
	int nBorderWidth = 0;
	int nBorderHeight = 0;

	//make sure the decorator isn't null then calculate
	if( m_pcDecorator != NULL )
	{
		cBorders = static_cast < IRect > ( m_pcDecorator->GetBorderSize() );
		nBorderWidth = cBorders.left + cBorders.right;
		nBorderHeight = cBorders.top + cBorders.bottom;
	}

	// Figure out which edges the cursor is relative to
	cHitPos = GetHitPointBase();

	// Calculate the delta movement relative to the hit edge/corner
	IPoint cDelta( static_cast < IPoint > ( cNewPos ) - ( cHitPos + m_cHitOffset ) - m_cDeltaMove + GetILeftTop() - m_cRawFrame.LeftTop(  ) );

	
	WindowDecorator::hit_item eHitItem;
	
	//calulate the hit item
	if( GetBounds().DoIntersect( cNewPos ) && m_pcClient->m_cFrame.DoIntersect( cNewPos ) == false )
	{
		eHitItem = m_pcDecorator->HitTest( cNewPos );
	}
	else
	{
		eHitItem = WindowDecorator::HIT_NONE;
	}

	
	if( m_eHitItem == WindowDecorator::HIT_NONE && ( InputNode::GetMouseButtons() == 0 || m_pcWindow->HasFocus(  ) ) )
	{
		// Change the mouse-pointer to indicate that resizeing is possible if above one of the resize-edges
		WindowDecorator::hit_item eCursorHitItem = ( m_eHitItem == WindowDecorator::HIT_NONE ) ? eHitItem : m_eHitItem;
		if( eCursorHitItem != m_eCursorHitItem )
		{
			m_eCursorHitItem = eCursorHitItem;
			switch ( eCursorHitItem )
			{
			case WindowDecorator::HIT_SIZE_LEFT:
			case WindowDecorator::HIT_SIZE_RIGHT:
				SrvApplication::SetCursor( MPTR_MONO, IPoint( H_POINTER_WIDTH / 2, H_POINTER_HEIGHT / 2 ), g_anHMouseImg, H_POINTER_WIDTH, H_POINTER_HEIGHT );
				break;
			case WindowDecorator::HIT_SIZE_TOP:
			case WindowDecorator::HIT_SIZE_BOTTOM:
				SrvApplication::SetCursor( MPTR_MONO, IPoint( V_POINTER_WIDTH / 2, V_POINTER_HEIGHT / 2 ), g_anVMouseImg, V_POINTER_WIDTH, V_POINTER_HEIGHT );
				break;
			case WindowDecorator::HIT_SIZE_LT:
			case WindowDecorator::HIT_SIZE_RB:
				SrvApplication::SetCursor( MPTR_MONO, IPoint( DIAG_POINTER_WIDTH / 2, DIAG_POINTER_HEIGHT / 2 ), g_anLTMouseImg, DIAG_POINTER_WIDTH, DIAG_POINTER_HEIGHT );
				break;
			case WindowDecorator::HIT_SIZE_LB:
			case WindowDecorator::HIT_SIZE_RT:
				SrvApplication::SetCursor( MPTR_MONO, IPoint( DIAG_POINTER_WIDTH / 2, DIAG_POINTER_HEIGHT / 2 ), g_anLBMouseImg, DIAG_POINTER_WIDTH, DIAG_POINTER_HEIGHT );
				break;
			case WindowDecorator::HIT_DRAG:
				SrvApplication::SetCursor( MPTR_MONO, IPoint( 0, 0 ), NULL, 0, 0 );
				break;
			default:
				SrvApplication::RestoreCursor();
				break;
			}
		}

		// If we didnt hit anything interesting with the last mouse-click we are done by now.
		if( m_eHitItem == WindowDecorator::HIT_NONE )
		{
			return ( false );
		}
	}

	// Set the state of the various border buttons.
	bool bNeedRedraw = false;
	for( int i = 0; i < WindowDecorator::HIT_MAX_ITEMS; i++ )
	{
		if( m_nButtonDown[i] > 0 )
		{
			int nNewMode = ( eHitItem == i ) ? 1 : 2;

			if( nNewMode != m_nButtonDown[i] )
			{
				bNeedRedraw = true;
				m_nButtonDown[i] = nNewMode;
				m_pcDecorator->SetButtonState( i, m_nButtonDown[i] == 1 );
			}
		}
	}
	//if the border buttons set a redraw lets do so now
	if( bNeedRedraw )
		g_pcTopView->RedrawLayer( this, this, false );
	

	//if hit drag then we call Align on the Delta
	//and add it to all ready moved
	if( m_eHitItem == WindowDecorator::HIT_DRAG )
	{
		cDelta.x = Align( cDelta.x, m_cAlignPos.x );
		cDelta.y = Align( cDelta.y, m_cAlignPos.y );
		m_cDeltaMove += cDelta;
	}

	uint32 nFlags = m_pcWindow->GetFlags();

	//if the window is not resizable
	if( ( nFlags & WND_NOT_RESIZABLE ) != WND_NOT_RESIZABLE )
	{
		//if horizontally not resizeable then set the x-value to 0
		if( nFlags & WND_NOT_H_RESIZABLE )
		{
			cDelta.x = 0;
		}
		
		//if vertically not resizeable then set the y-value to 0
		if( nFlags & WND_NOT_V_RESIZABLE )
		{
			cDelta.y = 0;
		}
		
		//create to IPoints with the decorator min size and our min size
		IPoint cBorderMinSize( m_pcDecorator->GetMinimumSize() );
		IPoint cMinSize( m_cMinSize );


		//if our minimum size x-axis is less than our border size - border width
		if( cMinSize.x < cBorderMinSize.x - nBorderWidth )
		{
			//our min size x-axis is changed to the border size - width
			cMinSize.x = cBorderMinSize.x - nBorderWidth;
		}
		
		//if our minimum size y-axis is less than bordersize - border height
		if( cMinSize.y < cBorderMinSize.y - nBorderHeight )
		{
			//our min size y-axis is change to the bordersize - height
			cMinSize.y = cBorderMinSize.y - nBorderHeight;
		}

		//determine what has been hit and do the appropriate adjustments to delta
		switch ( m_eHitItem )
		{
		case WindowDecorator::HIT_SIZE_LEFT:
			cDelta.x = -Clamp( m_cRawFrame.Width() + m_cDeltaSize.x - nBorderWidth, -cDelta.x, cMinSize.x, m_cMaxSize.x );
			cDelta.x = Align( Align( cDelta.x, m_cAlignSize.x ), m_cAlignPos.x );
			m_cDeltaMove.x += cDelta.x;
			m_cDeltaSize.x -= cDelta.x;
			break;
		case WindowDecorator::HIT_SIZE_RIGHT:
			cDelta.x = Clamp( m_cRawFrame.Width() + m_cDeltaSize.x - nBorderWidth, cDelta.x, cMinSize.x, m_cMaxSize.x );
			cDelta.x = Align( cDelta.x, m_cAlignSize.x );
			m_cDeltaSize.x += cDelta.x;
			break;
		case WindowDecorator::HIT_SIZE_TOP:
			cDelta.y = -Clamp( m_cRawFrame.Height() + m_cDeltaSize.y - nBorderHeight, -cDelta.y, cMinSize.y, m_cMaxSize.y );
			cDelta.y = Align( Align( cDelta.y, m_cAlignSize.y ), m_cAlignPos.y );
			m_cDeltaMove.y += cDelta.y;
			m_cDeltaSize.y -= cDelta.y;
			break;
		case WindowDecorator::HIT_SIZE_BOTTOM:
			cDelta.y = Clamp( m_cRawFrame.Height() + m_cDeltaSize.y - nBorderHeight, cDelta.y, cMinSize.y, m_cMaxSize.y );
			cDelta.y = Align( cDelta.y, m_cAlignSize.y );
			m_cDeltaSize.y += cDelta.y;
			break;
		case WindowDecorator::HIT_SIZE_LT:
			cDelta.x = -Clamp( m_cRawFrame.Width() + m_cDeltaSize.x - nBorderWidth, -cDelta.x, cMinSize.x, m_cMaxSize.x );
			cDelta.y = -Clamp( m_cRawFrame.Height() + m_cDeltaSize.y - nBorderHeight, -cDelta.y, cMinSize.y, m_cMaxSize.y );
			cDelta.x = Align( Align( cDelta.x, m_cAlignSize.x ), m_cAlignPos.x );
			cDelta.y = Align( Align( cDelta.y, m_cAlignSize.y ), m_cAlignPos.y );
			m_cDeltaMove += cDelta;
			m_cDeltaSize -= cDelta;
			break;
		case WindowDecorator::HIT_SIZE_RT:
			cDelta.x = Clamp( m_cRawFrame.Width() + m_cDeltaSize.x - nBorderWidth, cDelta.x, cMinSize.x, m_cMaxSize.x );
			cDelta.y = -Clamp( m_cRawFrame.Height() + m_cDeltaSize.y - nBorderHeight, -cDelta.y, cMinSize.y, m_cMaxSize.y );
			cDelta.x = Align( cDelta.x, m_cAlignSize.x );
			cDelta.y = Align( Align( cDelta.y, m_cAlignSize.y ), m_cAlignPos.y );
			m_cDeltaSize.x += cDelta.x;
			m_cDeltaSize.y -= cDelta.y;
			m_cDeltaMove.y += cDelta.y;
			break;
		case WindowDecorator::HIT_SIZE_RB:
			cDelta.x = Clamp( m_cRawFrame.Width() + m_cDeltaSize.x - nBorderWidth, cDelta.x, cMinSize.x, m_cMaxSize.x );
			cDelta.y = Clamp( m_cRawFrame.Height() + m_cDeltaSize.y - nBorderHeight, cDelta.y, cMinSize.y, m_cMaxSize.y );
			cDelta.x = Align( cDelta.x, m_cAlignSize.x );
			cDelta.y = Align( cDelta.y, m_cAlignSize.y );
			m_cDeltaSize += cDelta;
			break;
		case WindowDecorator::HIT_SIZE_LB:
			cDelta.x = -Clamp( m_cRawFrame.Width() + m_cDeltaSize.x - nBorderWidth, -cDelta.x, cMinSize.x, m_cMaxSize.x );
			cDelta.y = Clamp( m_cRawFrame.Height() + m_cDeltaSize.y - nBorderHeight, cDelta.y, cMinSize.y, m_cMaxSize.y );
			cDelta.x = Align( Align( cDelta.x, m_cAlignSize.x ), m_cAlignPos.x );
			cDelta.y = Align( cDelta.y, m_cAlignSize.y );
			m_cDeltaMove.x += cDelta.x;
			m_cDeltaSize.x -= cDelta.x;
			m_cDeltaSize.y += cDelta.y;
			break;
		default:
			break;
		}
	}

	//the application is not null and resize/move has happened
	if( pcAppTarget != NULL && ( m_cDeltaSize.x != 0 || m_cDeltaSize.y != 0 || m_cDeltaMove.x != 0 || m_cDeltaMove.y != 0 ) )
	{
		//the raw frame is ajusted
		m_cRawFrame.right += m_cDeltaSize.x + m_cDeltaMove.x;
		m_cRawFrame.bottom += m_cDeltaSize.y + m_cDeltaMove.y;
		m_cRawFrame.left += m_cDeltaMove.x;
		m_cRawFrame.top += m_cDeltaMove.y;

		//we then align the frame with the borders
		IRect cAlignedFrame = AlignRect( m_cRawFrame, cBorders );


		//if there isn't any moves pending
		if( m_bWndMovePending == false )
		{
			//create a message and add the new frame
			Message cMsg( M_WINDOW_FRAME_CHANGED );
			cMsg.AddRect( "_new_frame", RectToClient( cAlignedFrame, m_pcDecorator ) );

			//send the message and if we can't then call dbprintf to append to kernel log
			if( pcAppTarget->SendMessage( &cMsg ) < 0 )
			{
				dbprintf( "WndBorder::MouseMoved() failed to send M_WINDOW_FRAME_CHANGED message to window\n" );
			}
			
			//the frame is not updated because there isn't moves pending
			m_bFrameUpdated = false;
		}
		else
		{
			//the frame is updated because there are moves pending
			m_bFrameUpdated = true;
		}
		
		//there are now moves pending
		m_bWndMovePending = true;
		
		//realign delta move and size to (0,0)
		m_cDeltaMove = IPoint( 0, 0 );
		m_cDeltaSize = IPoint( 0, 0 );
		
		//set the frame, update all regions
		DoSetFrame( cAlignedFrame );
		SrvSprite::Hide();
		m_pcParent->UpdateRegions( false );
		SrvSprite::Unhide();
		
		#if 0
				m_cDeltaMove = IPoint( 0, 0 );
				m_cDeltaSize = IPoint( 0, 0 );
		#endif
	}
	
	//we will return true if m_eHitItem != HIT_NONE and false otherwise 
	return ( m_eHitItem != WindowDecorator::HIT_NONE );
}

/** Determines if the mouse was hit within the borders of the window border
 * \par Description:
 *  This function determines whether or not the mouse was hit inside the window border
 * \param Messenger* - the application target
 * \param Point - the point where the mouse was hit
 * \param int - the actual button that was hit
 * \author Kurt Skauen
 * \author Rick Caudill(modified and documented)
 *****************************************************************************/
bool WndBorder::MouseDown( Messenger * pcAppTarget, const Point & cPos, int nButton )
{
	IPoint cHitPos;

	m_cDeltaMove = IPoint( 0, 0 );
	m_cDeltaSize = IPoint( 0, 0 );
	m_cRawFrame = m_cIFrame;

	//if cPos intersects the bounds and doesn't intersect the frame of the client
	if( GetBounds().DoIntersect( cPos ) && m_pcClient->m_cFrame.DoIntersect( cPos ) == false )
	{
		m_eHitItem = m_pcDecorator->HitTest( cPos ); //we get the hit item
	}
	else
	{
		m_eHitItem = WindowDecorator::HIT_NONE;  //we set the hit item to none
	}

	//I honestly don't know why this has to be put here
	//However that is the way it was :)
	cHitPos = GetHitPointBase();

	
	//calculate the hit offest
	m_cHitOffset = static_cast < IPoint > ( cPos ) + GetILeftTop() - m_cRawFrame.LeftTop(  ) - cHitPos;

	//if dragging
	if( m_eHitItem == WindowDecorator::HIT_DRAG )
	{
		//set the appropriate cursor
		SrvApplication::SetCursor( MPTR_MONO, IPoint( 0, 0 ), NULL, 0, 0 );
	}
	else
	{
		//if hit item is not none and not max
		if( m_eHitItem > 0 && m_eHitItem < WindowDecorator::HIT_MAX_ITEMS )
		{
			m_nButtonDown[m_eHitItem] = 1;
			m_pcDecorator->SetButtonState( m_eHitItem, true );
			g_pcTopView->RedrawLayer( this, this, false );
		}
	}

	return ( m_eHitItem != WindowDecorator::HIT_NONE );
}


/** Determines if the mouse was hit and lifted within the borders of the window border
 * \par Description:
 *  This function determines whether or not the mouse was hit and lifted inside the window border
 * \param Messenger* - the application target
 * \param Point - the point where the mouse was hit
 * \param int - the actual button that was hit
 * \author Kurt Skauen
 * \author Rick Caudill(modified and documented)
 *****************************************************************************/
void WndBorder::MouseUp( Messenger * pcAppTarget, const Point & cPos, int nButton )
{
	//the close button was hit
	if( m_nButtonDown[WindowDecorator::HIT_CLOSE] == 1 )
	{
		if( pcAppTarget != NULL ) //make sure the app is not null
		{
			Message cMsg( M_QUIT );

			//send m_quit to app, but if not sent fail and throw error
			if( pcAppTarget->SendMessage( &cMsg ) < 0 )
			{
				dbprintf( "Error: WndBorder::MouseUp() failed to send M_QUIT to window\n" );
			}
		}
	}
	else if( m_nButtonDown[WindowDecorator::HIT_DEPTH] == 1 ) //we hit the depth button
	{
		ToggleDepth(); //toggle the depth
		if( m_pcParent != NULL )
		{
			m_pcParent->UpdateRegions( false ); //yes update
		}
		SrvWindow::HandleMouseTransaction(); //make the window handle
	}
	else if( m_nButtonDown[WindowDecorator::HIT_MINIMIZE] == 1 ) // we want to minimize
	{
		//if the window is valid and it isn't minimized to start
		if( m_pcWindow != NULL && !m_pcWindow->IsMinimized() )
		{
			m_pcWindow->SetMinimized( true ); //set to true
			m_pcWindow->Show( false ); //then hide the window, boy is this ever a good trick ;)
		}
		if( m_pcParent != NULL )  //if there is a parent
		{
			m_pcParent->UpdateRegions( false ); //update
		}
		SrvWindow::HandleMouseTransaction(); //handle the rest
	}
	else if( m_nButtonDown[WindowDecorator::HIT_ZOOM] == 1 ) //we hit maximize
	{
		/* Maximize window */
		uint32 nFlags = m_pcWindow->GetFlags();

		if( ( nFlags & WND_NOT_RESIZABLE ) != WND_NOT_RESIZABLE )
		{

			//if the frame has been maximized, we want to restore
			if (m_bFrameMaxHit)
			{
				//if there are not any moves pending
				if( m_bWndMovePending == false )
				{
					//create a message and add _new_frame to it
					Message cMsg( M_WINDOW_FRAME_CHANGED );
					cMsg.AddRect( "_new_frame", RectToClient( m_cStoredFrame, m_pcDecorator ) );

					//send the mesage
					if( pcAppTarget->SendMessage( &cMsg ) < 0 )
					{
						dbprintf( "WndBorder::MouseMoved() failed to send M_WINDOW_FRAME_CHANGED message to window\n" );
					}
					//the frame has not been updated
					m_bFrameUpdated = false;
				}
				else
				{
					//the frame has been updated
					m_bFrameUpdated = true;
				}
								
				//restore the old frame
				SetFrame( m_cStoredFrame );
				SrvSprite::Hide();
				m_pcParent->UpdateRegions( false );
				SrvSprite::Unhide();
				
				//make sure to make the frame maximize next round
				m_bFrameMaxHit = false;
			}
			else
			{
				//create a rect for the borders
				IRect cBorders( 0, 0, 0, 0 );

				//make sure the decorator is not null
				if( m_pcDecorator != NULL )
				{
					//then get the borders
					cBorders = static_cast < IRect > ( m_pcDecorator->GetBorderSize() );
				}
				
				//get the max frame
				os::Rect cMaxFrame = get_desktop_max_window_frame( get_active_desktop() );

				//if not horizontally resizeable
				if( nFlags & WND_NOT_H_RESIZABLE )
				{
					cMaxFrame.right = GetFrame().Width(  ) + cMaxFrame.left;
				}
				
				//if not vertically resizable
				if( nFlags & WND_NOT_V_RESIZABLE )
				{
					cMaxFrame.bottom = GetFrame().Height(  )+ cMaxFrame.top;
				}

				//make sure sizes are valid
				//the right should have at least the left plus the x axis
				//the bottom should have at least the top plus y axis
				if( cMaxFrame.right - cMaxFrame.left > m_cMaxSize.x - 1 )
					cMaxFrame.right = cMaxFrame.left + m_cMaxSize.x - 1;
				if( cMaxFrame.bottom - cMaxFrame.top > m_cMaxSize.y - 1 )
					cMaxFrame.bottom = cMaxFrame.top + m_cMaxSize.y - 1;

				//align the rect
				IRect cAlignedFrame = AlignRect( cMaxFrame, cBorders );

				//there isn't any moves pending
				if( m_bWndMovePending == false )
				{
					//create a message and add _new_frame to it
					Message cMsg( M_WINDOW_FRAME_CHANGED );
					cMsg.AddRect( "_new_frame", RectToClient( cAlignedFrame, m_pcDecorator ) );

					//call the send the mesage to the app
					if( pcAppTarget->SendMessage( &cMsg ) < 0 )
					{
						dbprintf( "WndBorder::MouseMoved() failed to send M_WINDOW_FRAME_CHANGED message to window\n" );
					}
					
					//the frame has not been updated
					m_bFrameUpdated = false;
				}
				else
				{
					//the frame is updated
					m_bFrameUpdated = true;
				}
				
				//the frame's maximize has been hit and the frame is stored
				m_bFrameMaxHit = true;
				m_cStoredFrame = m_cRawFrame;
				
				//set the frame and update
				SetFrame( cAlignedFrame );
				SrvSprite::Hide();
				m_pcParent->UpdateRegions( false );
				SrvSprite::Unhide();
			}
			m_bWndMovePending = true; //moves are pending
		}
	}

	//the hit items is now none
	m_eHitItem = WindowDecorator::HIT_NONE;

	//we need to make sure if we need a redraw
	//so we iterate through all the window deocrator items
	bool bNeedRedraw = false;
	for( int i = 0; i < WindowDecorator::HIT_MAX_ITEMS; i++ )
	{
		if( m_nButtonDown[i] != 0 ) // if the button that is down dn equa 0
		{		
			m_pcDecorator->SetButtonState( i, false ); //set the button state to false
			bNeedRedraw = true; //then call for the redray
		}
		m_nButtonDown[i] = 0; //set it to 0
	}
	
	//if we need a redraw then redraw
	if( bNeedRedraw )
		g_pcTopView->RedrawLayer( this, this, false );

	//the raw frame equals IFrame
	m_cRawFrame = m_cIFrame;

	//then we restore the cursor
	SrvApplication::RestoreCursor();
}
/** Determines if there is an app target and then it sends a message to the app
 * \par Description:
 *  This function determines whether there is an app target and then it sends the message
 * 	M_WINDOW_FRAME_CHANGED to the application
 * \param Messenger* - the application target
 * \author Kurt Skauen
 * \author Rick Caudill(modified and documented)
 *****************************************************************************/
void WndBorder::WndMoveReply( Messenger * pcAppTarget )
{
	//close the layer gate
	g_cLayerGate.Close();

	//if the fame has been updated
	if( m_bFrameUpdated )
	{
		m_bFrameUpdated = false; //set to false
		
		//if we had an app target
		if( pcAppTarget != NULL )
		{
			IRect cBorders( 0, 0, 0, 0 );

			//if we have a decorator
			if( m_pcDecorator != NULL )
			{
				//get the borders of the decorator
				cBorders = static_cast < IRect > ( m_pcDecorator->GetBorderSize() );
			}

			//do an alignment 
			IRect cAlignedFrame = AlignRect( m_cRawFrame, cBorders );


			//create a message with M_WINDOW_FRAME_CHANGED and add _new_frame to it
			Message cMsg( M_WINDOW_FRAME_CHANGED );
			cMsg.AddRect( "_new_frame", RectToClient( cAlignedFrame, m_pcDecorator ) );


			//send the message
			if( pcAppTarget->SendMessage( &cMsg ) < 0 )
			{
				dbprintf( "Error: WndBorder::WndMoveReply() failed to send M_WINDOW_FRAME_CHANGED to window\n" );
			}

			//reinit delta
			m_cDeltaMove = IPoint( 0, 0 );
			m_cDeltaSize = IPoint( 0, 0 );
			
			//there is a move pending now
			m_bWndMovePending = true;
		}
	}
	else
	{
		//there isn't move pending
		m_bWndMovePending = false;
	}
	
	//if there isn't a move pending
	if( m_bWndMovePending == false )
	{
		//update the client if it is needed
		m_pcClient->UpdateIfNeeded();
	}
	//close the layer gate
	g_cLayerGate.Open();
}


/** Returns whether there are any pending size events
 * \par Description:
 *  This function determines whether or not there are any pending size events
 * \author Kurt Skauen
 * \author Rick Caudill(modified and documented)
 *****************************************************************************/
bool WndBorder::HasPendingSizeEvents() const
{
	return ( m_bWndMovePending );
}


