/*
 *  "Blue" Window-Decorator 1.1
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

#include "BlueDecorator.h"
#include <layer.h>
 
#include <gui/window.h>
using namespace os;

#define WND_NO_MAX_RESTORE_BUT  WND_NO_ZOOM_BUT
#define WND_NO_MINIM_BUT        WND_NO_DEPTH_BUT

#define HIT_MAX_RESTORE 		HIT_ZOOM

static SrvBitmap* g_pcDecor = NULL;
static SrvBitmap* g_pcButtons = NULL;


BlueDecorator::BlueDecorator( Layer* pcLayer, uint32 nWndFlags )
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


BlueDecorator::~BlueDecorator()
{
} 


void BlueDecorator::LoadBitmap (SrvBitmap* bmp,uint8* raw,IPoint size)
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


void BlueDecorator::CalculateBorderSizes()
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
		    m_vTopBorder = 5;
		}
		else
		{
			m_vTopBorder = 25;
		}
		m_vLeftBorder   = 5;
		m_vRightBorder  = 5;
		m_vBottomBorder = 5;
	}
}

void BlueDecorator::FontChanged()
{
	Layer* pcView = GetLayer();
	m_sFontHeight = pcView->GetFontHeight();
	CalculateBorderSizes();
	pcView->Invalidate();
	Layout();
}

void BlueDecorator::SetFlags( uint32 nFlags )
{
	Layer* pcView = GetLayer();
	m_nFlags = nFlags;
	CalculateBorderSizes();
	pcView->Invalidate();
	Layout();
}

Point BlueDecorator::GetMinimumSize()
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

WindowDecorator::hit_item BlueDecorator::HitTest( const Point& cPosition )
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
		
	
	if ( (m_nFlags & WND_NO_DEPTH_BUT) == 0 )		
	if ( m_cDepthRect.DoIntersect( cPosition ) ) return( HIT_DEPTH );
	 
	if ( m_cDragRect.DoIntersect( cPosition ) )
		return( HIT_DRAG );
		
	return( HIT_NONE );
}

Rect BlueDecorator::GetBorderSize()
{
	return( Rect( m_vLeftBorder, m_vTopBorder, m_vRightBorder, m_vBottomBorder ) );
}

void BlueDecorator::SetTitle( const char* pzTitle )
{
	m_cTitle = pzTitle;
	Render( m_cBounds );
}

void BlueDecorator::SetFocusState( bool bHasFocus )
{
	m_bHasFocus = bHasFocus;
	Render( m_cBounds );
}

void BlueDecorator::SetWindowFlags( uint32 nFlags )
{
	m_nFlags = nFlags;
}

void BlueDecorator::FrameSized( const Rect& cFrame )
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

void BlueDecorator::Layout()
{
	m_cCloseRect.left   = m_cBounds.right-28;
	m_cCloseRect.right  = m_cCloseRect.left+23;
	m_cCloseRect.top    = 4;
	m_cCloseRect.bottom = 20;

	m_cMaxRestoreRect.left   = m_cBounds.right-55;
	m_cMaxRestoreRect.right  = m_cMaxRestoreRect.left+23;
	m_cMaxRestoreRect.top    = 4;
	m_cMaxRestoreRect.bottom = 20;

	m_cMinimizeRect.left   = m_cBounds.right-80;
	m_cMinimizeRect.right  = m_cMinimizeRect.left+23;
	m_cMinimizeRect.top    = 4;
	m_cMinimizeRect.bottom = 20;

    // Depth 
    m_cDepthRect.left=4;
    m_cDepthRect.right=m_cDepthRect.left+23;
    m_cDepthRect.top=4;
    m_cDepthRect.bottom=20;
    
	// DRAG RECT
	m_cDragRect.left   = 0;
	m_cDragRect.right   = m_cBounds.right;
    m_cDragRect.top    = 0;
    m_cDragRect.bottom = 23;
}

void BlueDecorator::SetButtonState( uint32 nButton, bool bPushed )
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
		case HIT_DEPTH:
			SetDepthButtonState( bPushed );			
			break;
	}	
}

void BlueDecorator::SetDepthButtonState( bool bPushed )
{
	m_bDepthState = bPushed;
	if ( (m_nFlags & WND_NO_DEPTH_BUT) == 0 )		
	{
		DrawDepth( m_cDepthRect, m_bHasFocus, m_bDepthState == 1 );
	}
}

void BlueDecorator::SetCloseButtonState( bool bPushed )
{
	m_bCloseState = bPushed;
	if ( (m_nFlags & WND_NO_CLOSE_BUT) == 0 )
	{
		DrawClose( m_cCloseRect, m_bHasFocus, m_bCloseState == 1 );
	}
}

void BlueDecorator::SetMaxRestoreState( bool bPushed )
{
	m_bMxRstrState = bPushed;
	if ( (m_nFlags & WND_NO_MAX_RESTORE_BUT) == 0 )
	{
		DrawMaxRestore( m_cMaxRestoreRect, m_bHasFocus, m_bMxRstrState == 1 );
	}
}

void BlueDecorator::SetMinimizeButtonState( bool bPushed )
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
void BlueDecorator::DrawDecor (void)
{
	Layer* pcView = GetLayer();
	
	pcView->SetDrawingMode (DM_OVER);	
	pcView->DrawBitMap (g_pcDecor, Rect (0,0,23,8),
						Rect (7,8,7+23,8+8));
	pcView->SetDrawingMode (DM_COPY);
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BlueDecorator::DrawMinimize( const Rect& cRect, bool bActive, bool bRecessed )
{
	Layer* pcView = GetLayer();

	pcView->SetDrawingMode (DM_OVER);
	if ((m_nFlags & WND_NO_MINIM_BUT)==0)	
	{		
		if (bRecessed)
			pcView->DrawBitMap (g_pcButtons,Rect (75,0,99,17),cRect);
		else if (bActive)
			pcView->DrawBitMap (g_pcButtons,Rect (0,0,23,17),cRect);
		else
			// inactive
			pcView->DrawBitMap (g_pcButtons,Rect (150,0,174,17),cRect);
	}
		
	pcView->SetDrawingMode (DM_COPY);
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BlueDecorator::DrawMaxRestore(  const Rect& cRect, bool bActive, bool bRecessed )
{
	Layer* pcView = GetLayer();

	pcView->SetDrawingMode (DM_OVER);

	if ((m_nFlags & WND_NO_MAX_RESTORE_BUT)==0)
	{	
		if (bRecessed)
			pcView->DrawBitMap (g_pcButtons,Rect (100,0,124,17),cRect);		
		else if (bActive)
			pcView->DrawBitMap (g_pcButtons,Rect (25,0,49,17),cRect);
		else
			//inactive
			pcView->DrawBitMap (g_pcButtons,Rect (175,0,199,17),cRect);
	}

	pcView->SetDrawingMode (DM_COPY);
}



//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BlueDecorator::DrawClose(  const Rect& cRect, bool bActive, bool bRecessed  )
{
	Layer* pcView = GetLayer();

	pcView->SetDrawingMode (DM_OVER);
	
	if ((m_nFlags & WND_NO_CLOSE_BUT)==0)	
	{
		if (bRecessed)
			pcView->DrawBitMap (g_pcButtons,Rect (125,0,149,17),cRect);
		else if (bActive)
			pcView->DrawBitMap (g_pcButtons,Rect (50,0,74,17),cRect);
		else
			//inactive
			pcView->DrawBitMap (g_pcButtons,Rect (200,0,223,17),cRect);
	}
	
	pcView->SetDrawingMode (DM_COPY);
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BlueDecorator::DrawDepth( const Rect& cRect, bool bActive, bool bRecessed )
{
	Layer* pcView = GetLayer();

	pcView->SetDrawingMode (DM_OVER);
	
	if ((m_nFlags & WND_NO_DEPTH_BUT)==0)	
	{
		if (bRecessed)
			pcView->DrawBitMap (g_pcButtons,Rect (250,0,274,17),cRect);
		else if (bActive)
			pcView->DrawBitMap (g_pcButtons,Rect (225,0,249,17),cRect);
		else
			//inactive
			pcView->DrawBitMap (g_pcButtons,Rect (275,0,299,17),cRect);
	}
	
	pcView->SetDrawingMode (DM_COPY);
}



//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BlueDecorator::FillBackGround(void )
{
	Layer* pcView = GetLayer();
	
	Rect  cOBounds = pcView->GetBounds();	

	Color32_s sFillColor =  m_bHasFocus ? Color32_s(62,101,172,255):Color32_s(191,191,191,255) ; 


	Color32_s col_Light  = m_bHasFocus ? Color32_s (32,71,142,255):Color32_s(191,191,191,255) ;	
	Color32_s col_Dark   = m_bHasFocus ? Color32_s (62,101,172,255):Color32_s(221,221,221,255);
	
	Point Left  = Point(cOBounds.left+1,  cOBounds.top+1);
	Point Right = Point(cOBounds.right-1, cOBounds.top+1);

	Color32_s col_Grad = col_Light;
	Color32_s col_Step = Color32_s( (col_Light.red-col_Dark.red)/15, 
									(col_Light.green-col_Dark.green)/15,
									(col_Light.blue-col_Dark.blue)/15, 0 );

	for (int i=0; i<25; i++)
	{
		pcView->SetFgColor( col_Grad );
		pcView->DrawLine( Left, Right );
		Left.y  += 1;
		Right.y += 1;
		col_Grad.red   -= col_Step.red;
		col_Grad.green -= col_Step.green;
		col_Grad.blue  -= col_Step.blue;
	}
	
		
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

void BlueDecorator::DrawFrameBorders (void)
{
	Layer* pcView = GetLayer();
	Rect  cOBounds = pcView->GetBounds();
	
	// outside                        
	pcView->SetFgColor( 15, 32, 50, 255 );	
	pcView->DrawLine (Point (cOBounds.left,cOBounds.top),Point (cOBounds.right,cOBounds.top)); 
	pcView->DrawLine (Point (cOBounds.right,cOBounds.bottom)); 
	pcView->DrawLine (Point (cOBounds.left,cOBounds.bottom)); 
	pcView->DrawLine (Point (cOBounds.left,cOBounds.top)); 

	// outside light
	Color32_s sLightColor =  m_bHasFocus ? Color32_s(100,144,223,255):Color32_s(244,244,244,255) ; 
	
	pcView->SetFgColor(sLightColor);		
	pcView->DrawLine (Point (cOBounds.left+1,cOBounds.top+1),Point (cOBounds.right-1,cOBounds.top+1)); 
	pcView->DrawLine (Point (cOBounds.left+1,cOBounds.top+1),Point (cOBounds.left+1,cOBounds.bottom-1)); 

	// outside dark
	Color32_s sDarkColor =  m_bHasFocus ? Color32_s(25,62,80,255):Color32_s(184,184,184,255) ; 
	pcView->SetFgColor( sDarkColor );		
	pcView->DrawLine (Point (cOBounds.left+1,cOBounds.bottom-1),Point (cOBounds.right-1,cOBounds.bottom-1)); 
	pcView->DrawLine (Point (cOBounds.right-1,cOBounds.bottom-1),Point (cOBounds.right-1,cOBounds.top+2)); 

	// inside
	pcView->SetFgColor( 29, 64, 98, 255 );
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

void BlueDecorator::DrawTitle ()
{
	if ( (m_nFlags & WND_NO_TITLE) == 0 )
	{
		Layer* pcView = GetLayer();
		Color32_s sTextColor =  m_bHasFocus ? Color32_s(255,255,255,255) : Color32_s(128,128,128,255);
	
		pcView->SetFgColor(sTextColor);
		pcView->SetDrawingMode (DM_OVER);

		float tw=pcView->m_pcFont->GetInstance()->GetStringWidth (m_cTitle.c_str(),strlen(m_cTitle.c_str()));

		Rect rc=m_cBounds;
		rc.bottom=26;
	
		if ( (m_nFlags & WND_NO_DEPTH_BUT) == 0 )		
			rc.left=35;
		else
			rc.left=7;
		
		
		rc.right-=m_cCloseRect.Width();
		rc.right-=m_cMinimizeRect.Width();
		rc.right-=m_cMaxRestoreRect.Width();
		rc.right-=16;
		
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

void BlueDecorator::Render( const Rect& cUpdateRect )
{
	FillBackGround();
	DrawFrameBorders ();

	// DECORATION*
//	DrawDecor ();
	                        	
	DrawClose( m_cCloseRect, m_bHasFocus, m_bCloseState == 1 );
	DrawMaxRestore( m_cMaxRestoreRect, m_bHasFocus, m_bMxRstrState == 1 );
	DrawMinimize( m_cMinimizeRect, m_bHasFocus, m_bMinimState == 1 );
	DrawDepth( m_cDepthRect, m_bHasFocus, m_bMinimState == 1 );

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
		static uint8 g_buttons[] = {
		#include "pixmaps/buttons.h"	
		};

		static uint8 g_decor[] = {
		#include "pixmaps/decor/deco.h"	
		};
   	   	 
		g_pcButtons = new SrvBitmap (300,18,CS_RGB32);
		g_pcDecor   = new SrvBitmap (24,9,CS_RGB32); 
	
		BlueDecorator::LoadBitmap (g_pcButtons,g_buttons,IPoint (300,18));
		BlueDecorator::LoadBitmap (g_pcDecor,g_decor,IPoint (24,9));
	}

    return( new BlueDecorator( pcLayer, nFlags ) );
}

extern "C" int unload_decorator()
{
	if( g_pcButtons != NULL ) g_pcButtons->Release();
	if( g_pcDecor != NULL ) g_pcDecor->Release();
	return( 0 );
}

