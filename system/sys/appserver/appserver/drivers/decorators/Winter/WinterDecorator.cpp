/*
 *  "Winter" Window-Decorator
 *  Copyright (C) 2007 John Aspras
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

#include "WinterDecorator.h"
#include <layer.h>
 
#include <gui/window.h>
using namespace os;

#define WND_NO_MAX_RESTORE_BUT  WND_NO_ZOOM_BUT
#define HIT_MAX_RESTORE 		HIT_ZOOM

#define WND_NO_MINIM_BUT        WND_NO_DEPTH_BUT

static SrvBitmap* g_pcDecor = NULL;

static SrvBitmap* g_pcButtonMinim  = NULL;
static SrvBitmap* g_pcButtonMxRstr = NULL;
static SrvBitmap* g_pcButtonClose  = NULL;

static SrvBitmap* g_pcButtonMinimPressed  = NULL;
static SrvBitmap* g_pcButtonMxRstrPressed = NULL;
static SrvBitmap* g_pcButtonClosePressed  = NULL;

static SrvBitmap* g_pcButtonMinimInactive  = NULL;
static SrvBitmap* g_pcButtonMxRstrInactive = NULL;
static SrvBitmap* g_pcButtonCloseInactive  = NULL;


WinterDecorator::WinterDecorator( Layer* pcLayer, uint32 nWndFlags )
:	WindowDecorator( pcLayer, nWndFlags )
{
    m_nFlags = nWndFlags;

    m_bHasFocus    = false;
    m_bCloseState  = false;
    m_bMxRstrState = false; 
    m_bMinimState  = false;

    m_sFontHeight = pcLayer->GetFontHeight();
  
    CalculateBorderSizes();
}


WinterDecorator::~WinterDecorator()
{
} 


void WinterDecorator::LoadBitmap (SrvBitmap* bmp,uint8* raw,IPoint size)
{
	int c=0;
	int sz=size.x*size.y*3;
	for (int i=0; i<sz; i+=3)
	{	
  		bmp->m_pRaster[c+0]=raw[i+2];
  		bmp->m_pRaster[c+1]=raw[i+1];
  		bmp->m_pRaster[c+2]=raw[i+0];

		bmp->m_pRaster[c+3]=0xff; // Should use ALPHA 

		c+=4;
	}
}
  

void WinterDecorator::DrawDecor (void)
{
	Layer* pcView = GetLayer();
	  
	pcView->SetDrawingMode (DM_OVER);
	pcView->DrawBitMap (g_pcDecor, Rect (0,0,23,8),
						Rect (7,8,7+23,8+8));
	pcView->SetDrawingMode (DM_COPY);
}


void WinterDecorator::CalculateBorderSizes()
{
	if ( m_nFlags & WND_NO_BORDER )
	{
		m_vLeftBorder   = 0;
		m_vRightBorder  = 0;
		m_vTopBorder    = 0;
		m_vBottomBorder = 0;
	}
	else
	{
		if ( (m_nFlags & (WND_NO_TITLE | WND_NO_CLOSE_BUT | WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT | WND_NOT_MOVEABLE)) ==
		     (WND_NO_TITLE | WND_NO_CLOSE_BUT | WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT | WND_NOT_MOVEABLE) )
		{
		    m_vTopBorder = 8;
		}
		else
		{
			m_vTopBorder = 24;
		}
		m_vLeftBorder   = 8;
		m_vRightBorder  = 8;
		m_vBottomBorder = 8;
	}
}

void WinterDecorator::FontChanged()
{
	Layer* pcView = GetLayer();
	m_sFontHeight = pcView->GetFontHeight();
	CalculateBorderSizes();
	pcView->Invalidate();
	Layout();
}

void WinterDecorator::SetFlags( uint32 nFlags )
{
	Layer* pcView = GetLayer();
	m_nFlags = nFlags;
	CalculateBorderSizes();
	pcView->Invalidate();
	Layout();
}

Point WinterDecorator::GetMinimumSize()
{
	Point cMinSize( 0.0f, m_vTopBorder + m_vBottomBorder );

	cMinSize.x += m_cCloseRect.Width();
	cMinSize.x += m_cMinimizeRect.Width();
	cMinSize.x += m_cMaxRestoreRect.Width();
	
	if ( cMinSize.x < m_vLeftBorder + m_vRightBorder )
	{
		cMinSize.x = m_vLeftBorder + m_vRightBorder;
	}
	cMinSize.x+=80;
	
	return( cMinSize);
}

WindowDecorator::hit_item WinterDecorator::HitTest( const Point& cPosition )
{

	if ( (m_nFlags & WND_NOT_RESIZABLE) != WND_NOT_RESIZABLE )
	{
		if ( cPosition.y < 4 )
		{
			if ( cPosition.x < 16 )
			{
				return( HIT_SIZE_LT );
			}
			else if ( cPosition.x > m_cBounds.right - 16 )
			{
				return( HIT_SIZE_RT );
			}
			else
			{
				return( HIT_SIZE_TOP );
			}
		}
		else if ( cPosition.y > m_cBounds.bottom - 8 )
		{
			if ( cPosition.x < 16 )
			{
				return( HIT_SIZE_LB );
			}
			else if ( cPosition.x > m_cBounds.right - 16 )
			{
				return( HIT_SIZE_RB );
			}
			else
			{
				return( HIT_SIZE_BOTTOM );
			}
		}
		else if ( cPosition.x < 8 )
		{
			return( HIT_SIZE_LEFT );
		}
		else if ( cPosition.x > m_cBounds.right - 8 )
		{
			return( HIT_SIZE_RIGHT );
		}
	}

	if ( (m_nFlags & WND_NO_CLOSE_BUT) == 0)
		if ( m_cCloseRect.DoIntersect( cPosition ) ) return( HIT_CLOSE );
	
	
	if ( (m_nFlags & WND_NO_MAX_RESTORE_BUT) == 0 )
		if ( m_cMaxRestoreRect.DoIntersect( cPosition ) ) return( HIT_MAX_RESTORE );
	
	
	if ( (m_nFlags & WND_NO_MINIM_BUT) == 0 )
		if ( m_cMinimizeRect.DoIntersect( cPosition ) ) return( HIT_MINIMIZE );
	
	 
	if ( m_cDragRect.DoIntersect( cPosition ) )
		return( HIT_DRAG );
	
	return( HIT_NONE );
}

Rect WinterDecorator::GetBorderSize()
{
	return( Rect( m_vLeftBorder, m_vTopBorder, m_vRightBorder, m_vBottomBorder ) );
}

void WinterDecorator::SetTitle( const char* pzTitle )
{
	m_cTitle = pzTitle;
	Render( m_cBounds );
}

void WinterDecorator::SetFocusState( bool bHasFocus )
{
	m_bHasFocus = bHasFocus;
	Render( m_cBounds );
}

void WinterDecorator::SetWindowFlags( uint32 nFlags )
{
	m_nFlags = nFlags;
}

void WinterDecorator::FrameSized( const Rect& cFrame )
{
	Layer* pcView = GetLayer();
	Point cDelta( cFrame.Width() - m_cBounds.Width(), cFrame.Height() - m_cBounds.Height() );
	m_cBounds = cFrame.Bounds();

	Layout();
	if ( cDelta.x != 0.0f )
	{
		Rect cDamage = m_cBounds;
		cDamage.left = m_cMinimizeRect.left - fabs(cDelta.x)  - 50.0f;
		pcView->Invalidate( cDamage );
	}
	if ( cDelta.y != 0.0f )
	{
		Rect cDamage = m_cBounds;
		cDamage.top = cDamage.bottom - std::max( m_vBottomBorder, m_vBottomBorder + cDelta.y ) - 1.0f;
		pcView->Invalidate( cDamage );
	}	

	pcView->Invalidate( m_cBounds );
}

void WinterDecorator::Layout()
{
	m_cCloseRect.left   = m_cBounds.right-50;
	m_cCloseRect.right  = (m_cBounds.right-50)+43;
	m_cCloseRect.top    = 0;
	m_cCloseRect.bottom = 21;

	m_cMaxRestoreRect.left   = m_cBounds.right-76;// m_cBounds.right-76;
	m_cMaxRestoreRect.right  = m_cMaxRestoreRect.left+25;//(m_cBounds.right-76)+21;
	m_cMaxRestoreRect.top    = 0;
	m_cMaxRestoreRect.bottom = 21;

	m_cMinimizeRect.left   = m_cBounds.right-102;//m_cBounds.right-102;
	m_cMinimizeRect.right  = m_cMinimizeRect.left+26;
	m_cMinimizeRect.top    = 0;
	m_cMinimizeRect.bottom = 21;

	// DRAG RECT
	m_cDragRect.left   = 0;
	m_cDragRect.right   = m_cBounds.right-8;
	m_cDragRect.right-=m_cMinimizeRect.Width();
	m_cDragRect.right-=m_cMaxRestoreRect.Width();
	m_cDragRect.right-=m_cCloseRect.Width();	
    m_cDragRect.top    = 0;
    m_cDragRect.bottom = 23;
}

void WinterDecorator::SetButtonState( uint32 nButton, bool bPushed )
{
	switch( nButton )
	{
		case HIT_CLOSE:
			SetCloseButtonState( bPushed );
			break;
		case HIT_MAX_RESTORE:
			SetMaxRestoreState( bPushed );
			break;
		case HIT_MINIMIZE:
			SetMinimizeButtonState( bPushed );			
			break;
	}	
}

void WinterDecorator::SetCloseButtonState( bool bPushed )
{
	m_bCloseState = bPushed;
	if ( (m_nFlags & WND_NO_CLOSE_BUT) == 0 )
	{
		DrawClose( m_cCloseRect, m_bHasFocus, m_bCloseState == 1 );
	}
}

void WinterDecorator::SetMaxRestoreState( bool bPushed )
{
	m_bMxRstrState = bPushed;
	if ( (m_nFlags & WND_NO_MAX_RESTORE_BUT) == 0 )
	{
		DrawMaxRestore( m_cMaxRestoreRect, m_bHasFocus, m_bMxRstrState == 1 );
	}
}

void WinterDecorator::SetMinimizeButtonState( bool bPushed )
{
	
	m_bMinimState = bPushed;
	if ( (m_nFlags & WND_NO_MINIM_BUT) == 0 )
	{
		DrawMinimize( m_cMinimizeRect, m_bHasFocus, m_bMinimState == 1 );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void WinterDecorator::DrawMinimize( const Rect& cRect, bool bActive, bool bRecessed )
{
	Layer* pcView = GetLayer();

	pcView->SetDrawingMode (DM_OVER);
	if ((m_nFlags & WND_NO_MINIM_BUT)==0)	
	{		
		if (bRecessed)
			pcView->DrawBitMap (g_pcButtonMinimPressed,Rect (0,0,25,21),cRect);
		else if (bActive)
			pcView->DrawBitMap (g_pcButtonMinim,Rect (0,0,25,21),cRect);
		else
			pcView->DrawBitMap (g_pcButtonMinimInactive,Rect (0,0,25,21),cRect);
	}
	else
		pcView->DrawBitMap (g_pcButtonMinimInactive,Rect (0,0,25,21),cRect);
		
	pcView->SetDrawingMode (DM_COPY);
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void WinterDecorator::DrawMaxRestore(  const Rect& cRect, bool bActive, bool bRecessed )
{
	Layer* pcView = GetLayer();

	pcView->SetDrawingMode (DM_OVER);

	if ((m_nFlags & WND_NO_MAX_RESTORE_BUT)==0)
	{	
		if (bRecessed)
			pcView->DrawBitMap (g_pcButtonMxRstrPressed,Rect (0,0,25,21),cRect);
		else if (bActive)
			pcView->DrawBitMap (g_pcButtonMxRstr,Rect (0,0,25,21),cRect);
		else
			pcView->DrawBitMap (g_pcButtonMxRstrInactive,Rect (0,0,25,21),cRect);
	}
	else
		pcView->DrawBitMap (g_pcButtonMxRstrInactive,Rect (0,0,25,21),cRect);
	
	pcView->SetDrawingMode (DM_COPY);
}



//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void WinterDecorator::DrawClose(  const Rect& cRect, bool bActive, bool bRecessed  )
{
	Layer* pcView = GetLayer();

	pcView->SetDrawingMode (DM_OVER);
	
	if ((m_nFlags & WND_NO_CLOSE_BUT)==0)	
	{
		if (bRecessed)
			pcView->DrawBitMap (g_pcButtonClosePressed,Rect (0,0,43,21),cRect);
		else if (bActive)
			pcView->DrawBitMap (g_pcButtonClose,Rect (0,0,43,21),cRect);
		else
			pcView->DrawBitMap (g_pcButtonCloseInactive,Rect (0,0,43,21),cRect);
	}
	else
		pcView->DrawBitMap (g_pcButtonCloseInactive,Rect (0,0,43,21),cRect);
		
	pcView->SetDrawingMode (DM_COPY);
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void WinterDecorator::FillBackGround(void )
{
	Layer* pcView = GetLayer();
	
	Rect  cOBounds = pcView->GetBounds();	
	
	
	Color32_s sFillColor =  m_bHasFocus ? Color32_s(195,195,195,255) : Color32_s(214,214,214,255);
	
	cOBounds.Resize (1,1,-1,-1);
	
	// top
	pcView->FillRect( Rect( cOBounds.left,cOBounds.top, 
	                        cOBounds.right ,cOBounds.top + m_vTopBorder ), sFillColor );
	// left                        
	pcView->FillRect( Rect(cOBounds.left,cOBounds.top+m_vTopBorder,
	                       cOBounds.left+m_vLeftBorder,cOBounds.bottom+m_vLeftBorder),sFillColor); 
	// right                       
	pcView->FillRect( Rect(cOBounds.right-m_vRightBorder,cOBounds.top+m_vTopBorder,
	                       cOBounds.right,cOBounds.bottom),sFillColor); 
	// bottom                       
	pcView->FillRect( Rect(cOBounds.left,cOBounds.bottom-m_vBottomBorder,
	                       cOBounds.right,cOBounds.bottom),sFillColor); 
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void WinterDecorator::DrawFrameBorders (void)
{
	Layer* pcView = GetLayer();
	Rect  cOBounds = pcView->GetBounds();
	
	// outside                        
	pcView->SetFgColor( 77, 77, 77, 255 );	
	pcView->DrawLine (Point (cOBounds.left,cOBounds.top),Point (cOBounds.right,cOBounds.top)); 
	pcView->DrawLine (Point (cOBounds.right,cOBounds.bottom)); 
	pcView->DrawLine (Point (cOBounds.left,cOBounds.bottom)); 
	pcView->DrawLine (Point (cOBounds.left,cOBounds.top)); 


	// outside white
	pcView->SetFgColor( 255, 255, 255, 255 );		
	pcView->DrawLine (Point (cOBounds.left+1,cOBounds.top+1),Point (cOBounds.right-1,cOBounds.top+1)); 
	pcView->DrawLine (Point (cOBounds.left+1,cOBounds.top+1),Point (cOBounds.left+1,cOBounds.bottom-1)); 

	// outside gray
	pcView->SetFgColor( 146, 146, 146, 255 );		
	pcView->DrawLine (Point (cOBounds.left+1,cOBounds.bottom-1),Point (cOBounds.right-1,cOBounds.bottom-1)); 
	pcView->DrawLine (Point (cOBounds.right-1,cOBounds.bottom-1),Point (cOBounds.right-1,cOBounds.top-1)); 

	// inside
	pcView->SetFgColor( 99, 99, 99, 255 );
	pcView->DrawLine (Point (cOBounds.left+m_vLeftBorder,      cOBounds.top+(m_vTopBorder-1)),
	                  Point (cOBounds.right-(m_vRightBorder-1),cOBounds.top+m_vTopBorder-1));
	pcView->DrawLine (Point (cOBounds.right-(m_vRightBorder-1),cOBounds.bottom-(m_vBottomBorder-1))); 
	pcView->DrawLine (Point (cOBounds.left+(m_vLeftBorder-1),  cOBounds.bottom-(m_vBottomBorder-1))); 
	pcView->DrawLine (Point (cOBounds.left+(m_vLeftBorder-1),  cOBounds.top+(m_vTopBorder-1))); 
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void WinterDecorator::DrawTitle ()
{
	if ( (m_nFlags & WND_NO_TITLE) == 0 )
	{
		Layer* pcView = GetLayer();
		Color32_s sTextColor =  m_bHasFocus ? Color32_s(0,0,0,255) : Color32_s(128,128,128,255);
	
		pcView->SetFgColor(sTextColor);
		pcView->SetDrawingMode (DM_OVER);

		float tw=pcView->m_pcFont->GetInstance()->GetStringWidth (m_cTitle.c_str(),strlen(m_cTitle.c_str()));

		Rect rc=m_cBounds;
		rc.bottom=25;
		rc.left=40;
		
		
		rc.right-=m_cCloseRect.Width();
		rc.right-=m_cMinimizeRect.Width();
		rc.right-=m_cMaxRestoreRect.Width();
		rc.right-=24;
		
		if (tw>(rc.Width()))
		{	
			std::string tmpstr;
			char txt[300];			
			float nWinWidth=rc.Width();
			int nNumOfChars=strlen (m_cTitle.c_str());
			float nCharWidth=ceil (tw/nNumOfChars);
			int rem=(int)floor (nWinWidth/nCharWidth);
			
			snprintf (txt,rem,"%s",m_cTitle.c_str());	
			tmpstr=m_cTitle.substr(0,rem);
			tmpstr+="...";
			
			pcView->DrawText (rc,tmpstr.c_str(),-1,DTF_ALIGN_LEFT);
		}
		else
		{
			pcView->DrawText (rc,m_cTitle.c_str(),-1,DTF_ALIGN_LEFT);
		}

		pcView->SetDrawingMode (DM_COPY);

	}

}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void WinterDecorator::Render( const Rect& cUpdateRect )
{
	FillBackGround();
	DrawFrameBorders ();

	// DECORATION*
	DrawDecor ();
	                        	
	DrawClose( m_cCloseRect, m_bHasFocus, m_bCloseState == 1 );
	DrawMaxRestore( m_cMaxRestoreRect, m_bHasFocus, m_bMxRstrState == 1 );
	DrawMinimize( m_cMinimizeRect, m_bHasFocus, m_bMinimState == 1 );
	DrawTitle ();
}

extern "C" int get_api_version()
{
    return( WNDDECORATOR_APIVERSION );
}

extern "C" WindowDecorator* create_decorator( Layer* pcLayer, uint32 nFlags )
{
	if( g_pcDecor == NULL )
	{
		// NORMAL ******************************
		static uint8 g_buttonMinim[] = {
		#include "pixmaps/normal/buttonMin.h"
		};

		static uint8 g_buttonMxRstr[] = {
		#include "pixmaps/normal/buttonRestore.h"
		};

		static uint8 g_buttonClose[] = {
		#include "pixmaps/normal/buttonClose.h"
	   	};

		// PRESSED ******************************
		static uint8 g_buttonMinimPressed[] = {
		#include "pixmaps/pressed/buttonMin.h"
		};

		static uint8 g_buttonMxRstrPressed[] = {
		#include "pixmaps/pressed/buttonRestore.h"
		};

		static uint8 g_buttonClosePressed[] = {
		#include "pixmaps/pressed/buttonClose.h"
		};

		// INACTIVE ******************************
		static uint8 g_buttonMinimInactive[] = {
		#include "pixmaps/inactive/buttonMin.h"
		};

		static uint8 g_buttonMxRstrInactive[] = {
		#include "pixmaps/inactive/buttonRestore.h"
		};

		static uint8 g_buttonCloseInactive[] = {
		#include "pixmaps/inactive/buttonClose.h"
		};


		static uint8 g_decor[] = {
		#include "pixmaps/decor/deco.h"	
		};

		g_pcButtonMinim  = new SrvBitmap (26,22,CS_RGB32);
		g_pcButtonMxRstr = new SrvBitmap (26,22,CS_RGB32);
		g_pcButtonClose  = new SrvBitmap (44,22,CS_RGB32);

		g_pcButtonMinimPressed  = new SrvBitmap (26,22,CS_RGB32);
		g_pcButtonMxRstrPressed = new SrvBitmap (26,22,CS_RGB32);
		g_pcButtonClosePressed  = new SrvBitmap (44,22,CS_RGB32);

		g_pcButtonMinimInactive  = new SrvBitmap (26,22,CS_RGB32);
		g_pcButtonMxRstrInactive = new SrvBitmap (26,22,CS_RGB32);
		g_pcButtonCloseInactive  = new SrvBitmap (44,22,CS_RGB32);

		g_pcDecor = new SrvBitmap (24,9,CS_RGB32);

		WinterDecorator::LoadBitmap (g_pcButtonMinim,g_buttonMinim,IPoint (26,22));
		WinterDecorator::LoadBitmap (g_pcButtonMxRstr,g_buttonMxRstr,IPoint (26,22));
		WinterDecorator::LoadBitmap (g_pcButtonClose,g_buttonClose,IPoint (44,22));

		WinterDecorator::LoadBitmap (g_pcButtonMinimPressed,g_buttonMinimPressed,IPoint (26,22));
		WinterDecorator::LoadBitmap (g_pcButtonMxRstrPressed,g_buttonMxRstrPressed,IPoint (26,22));
		WinterDecorator::LoadBitmap (g_pcButtonClosePressed,g_buttonClosePressed,IPoint (44,22));

		WinterDecorator::LoadBitmap (g_pcButtonMinimInactive,g_buttonMinimInactive,IPoint (26,22));
		WinterDecorator::LoadBitmap (g_pcButtonMxRstrInactive,g_buttonMxRstrInactive,IPoint (26,22));
		WinterDecorator::LoadBitmap (g_pcButtonCloseInactive,g_buttonCloseInactive,IPoint (44,22));

		WinterDecorator::LoadBitmap (g_pcDecor,g_decor,IPoint (24,9));
	}

    return( new WinterDecorator( pcLayer, nFlags ) );
}

/****
 Free the button image bitmaps when the decorator is being unloaded, via Release().
****/
extern "C" int unload_decorator()
{
	if( g_pcDecor ) g_pcDecor->Release();

	if( g_pcButtonMinim ) g_pcButtonMinim->Release();
	if( g_pcButtonMxRstr ) g_pcButtonMxRstr->Release();
	if( g_pcButtonClose ) g_pcButtonClose->Release();

	if( g_pcButtonMinimPressed ) g_pcButtonMinimPressed->Release();
	if( g_pcButtonMxRstrPressed ) g_pcButtonMxRstrPressed->Release();
	if( g_pcButtonClosePressed ) g_pcButtonClosePressed->Release();

	if( g_pcButtonMinimInactive ) g_pcButtonMinimInactive->Release();
	if( g_pcButtonMxRstrInactive ) g_pcButtonMxRstrInactive->Release();
	if( g_pcButtonCloseInactive ) g_pcButtonCloseInactive->Release();
	
	return( 0 );
}
