
/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 2003 Rick Caudill
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

#include <gui/popupmenu.h>
#include <gui/font.h>
#include <typeinfo>
using namespace os;

uint8 arrow_image[] = {
#include "pixmaps/arrow.hex"
};
	

/** \internal */
class PopupMenu::Private
{
public:
	Private()
	{
		m_pcArrow = NULL;
		m_pcGrayArrow = NULL;
		m_pcHoverArrow = NULL;
		m_pcIcon = NULL;
		m_pcGrayIcon = NULL;
		m_bFocus = false;
		m_bEnabled = true;
		m_bArrow = true;
		m_bTracking = true;
		SetArrow();
	}
	~Private()
	{
		if (m_pcIcon) delete m_pcIcon;
		if (m_pcArrow) delete m_pcArrow;
		if (m_pcGrayIcon) delete m_pcGrayIcon;
		if (m_pcHoverArrow) delete m_pcHoverArrow;
		if (m_pcGrayArrow) delete m_pcGrayArrow;
		if (m_pcMenu) delete m_pcMenu;
	}
	
	Image* GetIcon()
	{
		return m_pcIcon;
	}
	
	void SetIcon(Image* pcIcon)
	{
		m_pcIcon = pcIcon;
	}
	
	Menu* GetMenu()
	{
		return m_pcMenu;
	}
	
	void SetMenu(Menu* pcMenu)
	{
		m_pcMenu = pcMenu;
	}
	
	void SetLabel(String cLabel)
	{
		m_cLabel = cLabel;
	}
	
	String GetLabel()
	{
		return m_cLabel;
	}
	
	bool GetFocus()
	{
		return m_bFocus;
	}
	
	void SetFocus(bool bFocus)
	{
		m_bFocus = bFocus;
	}
	
	void SetEnabled(bool bEnabled)
	{
		m_bEnabled = bEnabled;
	}
	
	bool GetEnabled()
	{
		return m_bEnabled;
	}
	
	bool IsArrow()
	{
		return m_bArrow;
	}
	
	void DrawArrow(bool bArrow)
	{
		m_bArrow = bArrow;
	}
	
	void SetTracking(bool bTracking)
	{
		m_bTracking = bTracking;
	}
	
	bool GetTracking()
	{
		return m_bTracking;
	}
	
	Image* GetArrow()
	{
		return m_pcArrow;
	}
	
	void SetArrow()
	{
		m_pcArrow = new BitmapImage(arrow_image,IPoint(10,7),CS_RGB32);
	}
	
	Image *GetGreyIcon()
	{
		if( !m_pcGrayIcon )
		{
			/* Note: This is not a good method, since it will only work on Bitmaps. */
			/* We have to add some method to Image for copying Images of unknown type! */
			if( m_pcIcon && ( typeid( *m_pcIcon ) == typeid( BitmapImage ) ) )
			{
				m_pcGrayIcon = new BitmapImage( *( dynamic_cast < BitmapImage * >( m_pcIcon ) ) );
				m_pcGrayIcon->ApplyFilter( Image::F_GRAY );
			}
		}
		return m_pcGrayIcon ? m_pcGrayIcon : m_pcIcon;
	}
	
	Image *GetGreyArrow()
	{
		if( !m_pcGrayArrow )
		{
			/* Note: This is not a good method, since it will only work on Bitmaps. */
			/* We have to add some method to Image for copying Images of unknown type! */
			if( m_pcArrow && ( typeid( *m_pcArrow ) == typeid( BitmapImage ) ) )
			{
				m_pcGrayArrow = new BitmapImage( *( dynamic_cast < BitmapImage * >( m_pcArrow ) ) );
				m_pcGrayArrow->ApplyFilter( Image::F_GRAY );
			}
		}
		return m_pcGrayArrow ? m_pcGrayArrow : m_pcArrow;
	}
	
	Image *GetHoverArrow()
	{
		if( !m_pcHoverArrow )
		{
			/* Note: This is not a good method, since it will only work on Bitmaps. */
			/* We have to add some method to Image for copying Images of unknown type! */
			if( m_pcArrow && ( typeid( *m_pcArrow ) == typeid( BitmapImage ) ) )
			{
				m_pcHoverArrow = new BitmapImage( *( dynamic_cast < BitmapImage * >( m_pcArrow ) ) );
				m_pcHoverArrow->ApplyFilter(ColorizeFilter(Color32_s(255,255,255)));
			}
		}
		return m_pcHoverArrow ? m_pcHoverArrow : m_pcArrow;
	}
	
	

private:
	Private(const Private&){}  //just wanted to disable copy constructor
	Image* 			m_pcIcon;
	Image*			m_pcGrayIcon;
	Image*			m_pcArrow;
	Image*			m_pcGrayArrow;
	Image*			m_pcHoverArrow;
	Menu*  			m_pcMenu;
	String			m_cLabel;
	bool			m_bFocus;
	bool			m_bEnabled;
	bool			m_bArrow;
	bool			m_bTracking;
};

/** Initialize the popupmenu.
 * \par Description:
 *	The popupmenu constructor initializes the local object.
 * \param cFrame		-	The size and position of the popupmenu.
 * \param cName			-	The name of the popupmenu.
 * \param cLabel		-	The label written on the popupmenu.
 * \param pcMenu		-	The menu that will be displayed when you click the popupmenu.  Caveat: You must set the menu to a handler or a meessenger(Menu::SetTargetForItems())
 * \param pcIcon		-	The image displyed on popupmenu.
 * \param nResizeMask 	-	Determines what way the popupmenu will follow the rest of the window.  Default is CF_FOLLOW_LEFT|CF_FOLLOW_TOP.
 * \param nFlags		-	Deteremines what flags will be sent to the popupmenu. Default is WID_WILL_DRAW | WID_CLEAR_BACKGROUND | WID_FULL_UPDATE_ON_RESIZE
 * \author	Rick Caudill (syllable_desktop@hotpop.com)
 *****************************************************************************/
PopupMenu::PopupMenu(const Rect& cFrame, const String& cName, const String& cLabel, Menu* pcMenu, Image* pcIcon,uint32 nResizeMask,uint32 nFlags):View(cFrame,cName.c_str(),nResizeMask,nFlags)
{	
	m = new Private;
	m->SetIcon(pcIcon);
	m->SetMenu(pcMenu);
	m->SetLabel(cLabel);
} /*end constructor*/

/** \internal */
PopupMenu::~PopupMenu()
{
	delete m;
}/*end destructor*/

/** Gets the label of the PopupMenu...
 * \par Description:
 *	Gets the label of the PopupMenu.  Returns it as an os::String
 * \author	Rick Caudill (syllable_desktop@hotpop.com)
 *****************************************************************************/
String PopupMenu::GetLabel()
{
	return m->GetLabel();
}

/** Sets the label of the PopupMenu...
 * \par Description:
 *	Sets the label of the PopupMenu.
 * \param cString - The string that you want to set it to.
 * \author	Rick Caudill (syllable_desktop@hotpop.com)
 *****************************************************************************/
void PopupMenu::SetLabel(const String& cString)
{
	m->SetLabel(cString);
	_RefreshDisplay();
}

/** Tells the system to disable or enable this element...
 * \par Description:
 *	This method will tell the system to disable or enable this element.
 * \param bEnabled - To enable this element set this to true(default) and to disable set this to false.
 * \sa IsEnabled()
 * \author	Rick Caudill (syllable_desktop@hotpop.com)
 *****************************************************************************/
void PopupMenu::SetEnabled(bool bEnabled,bool bValidate)
{
	m->SetEnabled(bEnabled);
	if (bValidate)
		_RefreshDisplay();
} /* end SetEnabled() */

/** Tells the programmer whether this element is enabled or disabled...
 * \par Description:
 *	This method will tell programmer whether or not this element is enabled.
 * \sa SetEnabled()
 * \author	Rick Caudill (syllable_desktop@hotpop.com)
 *****************************************************************************/
bool PopupMenu::IsEnabled()
{
	return m->GetEnabled();
} /* end IsEnabled() */

/** Sets the icon that will be attached to the PopupMenu...
 * \par Description:
 *	Sets the icon that will be attached to the PopupMenu.
 * \param pcIcon - The image that will become the icon that is attached to the PopupMenu.
 * \sa GetIcon()
 * \author	Rick Caudill (syllable_desktop@hotpop.com)
 *****************************************************************************/
void PopupMenu::SetIcon(Image* pcIcon)
{
	m->SetIcon(pcIcon);
} /*end SetIcon() */

/** Gets the icon that is attached to the PopupMenu...
 * \par Description:
 *	Gets the icon that is attached to the PopupMenu.
 * \sa SetIcon()
 * \author	Rick Caudill (syllable_desktop@hotpop.com)
 *****************************************************************************/
Image* PopupMenu::GetIcon()
{
	return m->GetIcon();
} /*end GetIcon() */

/** Tells the element whether you want the arrow icon attached to the PopupMenu...
 * \par Description:
 *	Tells the element whether you want the arrow icon attached to the PopupMenu...
 * \param bArrow - To enable the arrow icon set this to true(default) and to disable set this to false.
 * \author	Rick Caudill (syllable_desktop@hotpop.com)
 *****************************************************************************/
void PopupMenu::SetDrawArrow(bool bArrow)
{
	m->DrawArrow(bArrow);
} /*end DrawArrow() */

/* GetPreferredSize() */
Point PopupMenu::GetPreferredSize( bool bLargest) const
{
	font_height sHeight;
	float vWidth=0.0f;
	float vHeight=0.0f;
	Point cReturn;
	float vStrWidth = GetStringWidth(m->GetLabel().c_str() );
	GetFontHeight( &sHeight );
	vHeight = sHeight.ascender + sHeight.descender;
	if (m->GetIcon())
	{
		vWidth = m->GetIcon()->GetSize().x+3;
		if (vHeight < m->GetIcon()->GetSize().y)
			vHeight = m->GetIcon()->GetSize().y;
			
	}
	
	if (m->GetArrow())
	{
		Image* pcImage = m->GetArrow();
		vWidth += pcImage->GetSize().x;
	}
	
	vWidth+= vStrWidth;
	cReturn.x = vWidth;
	cReturn.y = vHeight;
	return cReturn;
} /* end GetPreferredSize() */

/* Paint() */
void PopupMenu::Paint(const Rect& cRect)
{
	Rect cBounds = GetBounds();
	float x;
	font_height sHeight;
	float vStrWidth = GetStringWidth( GetLabel().c_str() );
	Point cImgPos;
	Image* pcIcon=NULL;
	Image* pcArrow=NULL;
	GetFontHeight( &sHeight );
	
	if(m->GetIcon())
		pcIcon = IsEnabled()? m->GetIcon() : m->GetGreyIcon();

	
	if (m->IsArrow())
	{
		if (m->GetFocus())
			pcArrow = m->GetHoverArrow();
		else if (!IsEnabled())
	    	pcArrow = m->GetGreyArrow();
		else
			pcArrow = m->GetArrow();
	}	 
	SetEraseColor( get_default_color( COL_NORMAL ) );
	SetBgColor( get_default_color( COL_NORMAL ) );
	SetFgColor( get_default_color( COL_NORMAL ) );
	EraseRect(cBounds);
	FillRect( cBounds, get_default_color( COL_NORMAL ) );
	SetDrawingMode(DM_COPY);
	
	if (!IsEnabled())
		SetFgColor(100,100,100);
	else if (!m->GetFocus())
		SetFgColor(0,0,0,0);
	else
		SetFgColor(get_default_color(COL_SEL_MENU_BACKGROUND));
	
	if (GetIcon())
		x = GetIcon()->GetSize().x+3;
	else
		x = 0.0f;
	
	DrawString( Point( x, ( cBounds.Height() + 1.0f ) / 2 - ( sHeight.ascender + sHeight.descender ) / 2 + sHeight.ascender ), GetLabel().c_str() );

	if (GetIcon())
	{
		SetDrawingMode(DM_BLEND);
		cImgPos = Point(0,cBounds.Height()/2 - pcIcon->GetSize().y/2 + cBounds.top );
        pcIcon->Draw( cImgPos,this);
    }
    if (m->IsArrow())
    {
    	SetDrawingMode(DM_BLEND);
    	cImgPos = Point(x+vStrWidth,cBounds.Height()/2 - pcArrow->GetSize().y/2 + cBounds.top );
       	pcArrow->Draw( cImgPos,this);
    }
} /*end Paint() */

/* MouseDown()*/
void PopupMenu::MouseDown(const Point& cPos, uint32 nButtons)
{
	if (IsEnabled())
	{
		if( nButtons != 1 || IsEnabled() == false )
		{
			View::MouseDown( cPos, nButtons );
			return;
		}
		if (m->GetIcon())
		{
			if (cPos.x > GetIcon()->GetSize().x+4.0f)
				_OpenMenu(cPos);
		}
		else
			_OpenMenu(cPos);
	}
} /*end MouseDown() */

/* MouseUp() */
void PopupMenu::MouseUp( const Point & cPos, uint32 nButtons, Message * pcData )
{
	if (IsEnabled())
	{
		if( nButtons != 1 || IsEnabled() == false )
		{
			View::MouseUp( cPos, nButtons, pcData );
			return;
		}
		MakeFocus( false );
	}
} /*end MouseUp() */

/* MouseDown() */
void PopupMenu::MouseMove( const Point & cPos, int nCode, uint32 nButtons, Message * pcData )
{
	if( !IsEnabled() )
	{
		View::MouseMove( cPos, nCode, nButtons, pcData );
		return;
	}
	
	if( nCode == MOUSE_INSIDE)
	{ 
		if (m->GetTracking() )
		{
			m->SetFocus(true);
			m->SetTracking(false);
			_RefreshDisplay();
		}
	}
	
	else
	{
		m->SetFocus(false);
		m->SetTracking(true);
		_RefreshDisplay();
	}
} /* end MouseMove() */

/** \internal */
void PopupMenu::_RefreshDisplay()
{
	if( GetWindow() == NULL )
	{
		return;
	}
	Sync();
	Invalidate();
	Flush();
}

/** \internal */
void PopupMenu::_OpenMenu(Point cPos)
{
	Menu* pcMenu = m->GetMenu();
	if(pcMenu)
	{
		Point cPoint = ConvertToScreen(cPos);
		cPoint.y-=20;
		pcMenu->Open(cPoint);
	}
}

/*reserved*/
void PopupMenu::__pm_reserved1()
{
}

void PopupMenu::__pm_reserved2()
{
}

void PopupMenu::__pm_reserved3()
{
}

void PopupMenu::__pm_reserved4()
{
}

void PopupMenu::__pm_reserved5()
{
}

void PopupMenu::__pm_reserved6()
{
}

void PopupMenu::__pm_reserved7()
{
}

void PopupMenu::__pm_reserved8()
{
}

void PopupMenu::__pm_reserved9()
{
}

void PopupMenu::__pm_reserved10()
{
}
