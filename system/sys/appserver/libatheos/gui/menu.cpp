/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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

#include <stdio.h>
#include <assert.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <gui/menu.h>
#include <gui/font.h>
#include <util/message.h>

#include <atheos/kernel.h>


namespace os
{
    class MenuWindow : public Window
    {
    public:
	MenuWindow( const Rect& cFrame, Menu* pcMenu );
	~MenuWindow();

	void ShutDown() {
	    if ( m_pcMenu != NULL ) {
		RemoveChild( m_pcMenu );
		m_pcMenu = NULL;
	    }
	    PostMessage( M_TERMINATE );
	}
    private:
	Menu* m_pcMenu;
    };
    
}
using namespace os;

Locker Menu::s_cMutex( "menu_mutex" );







class Menu::Private
{
public:
    Private() : m_cMutex( "menu_mutex" ) {}
    std::string m_cTitle;
    Message	m_cCloseMsg;
    Messenger   m_cCloseMsgTarget;
    Locker	m_cMutex;
};

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MenuItem::MenuItem( const char* pzLabel, Message* pcMsg ) : Invoker( pcMsg, NULL, NULL )
{
    m_pcNext	   = NULL;
    m_pcSuperMenu	   = NULL;
    m_pcSubMenu	   = NULL;
    m_bIsHighlighted = false;
  
    if ( NULL != pzLabel )
    {
	m_pzLabel = new char[ strlen( pzLabel ) + 1 ];
	strcpy( m_pzLabel, pzLabel );
    }
    else
    {
	m_pzLabel = NULL;
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MenuItem::MenuItem( Menu* pcMenu, Message* pcMsg ) : Invoker( pcMsg, NULL, NULL )
{
    m_pcNext	   = NULL;
    m_pcSuperMenu  = NULL;
    m_pcSubMenu	   = pcMenu;
    m_bIsHighlighted = false;

    pcMenu->m_pcSuperItem = this;

    std::string cLabel = pcMenu->GetLabel();

    if ( cLabel.empty() == false ) {
	m_pzLabel = new char[ cLabel.size() + 1 ];
	strcpy( m_pzLabel, cLabel.c_str() );
    } else {
	m_pzLabel = NULL;
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MenuItem::~MenuItem()
{
    delete[] m_pzLabel;
    delete   m_pcSubMenu;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool MenuItem::Invoked( Message* pcMessage )
{
    return( true );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Menu*	MenuItem::GetSubMenu() const
{
    return( m_pcSubMenu );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Menu*	MenuItem::GetSuperMenu() const
{
    return( m_pcSuperMenu );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Rect MenuItem::GetFrame() const
{
    return( m_cFrame );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point MenuItem::GetContentSize()
{
    Menu* pcMenu = GetSuperMenu();

    if ( m_pzLabel != NULL && pcMenu != NULL ) {
	font_height sHeight;
	pcMenu->GetFontHeight( &sHeight );

	return( Point( pcMenu->GetStringWidth( GetLabel() ) + 4, sHeight.ascender + sHeight.descender + sHeight.line_gap ) );
    }
    return( Point( 0.0f, 0.0f ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

const char* MenuItem::GetLabel() const
{
    return( m_pzLabel );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point MenuItem::GetContentLocation() const
{
    if ( m_pcSuperMenu ) {
	return( Point( m_cFrame.left, m_cFrame.top ) );
    } else {
	return( Point( m_cFrame.left, m_cFrame.top ) );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void MenuItem::Draw()
{
    Menu* pcMenu = GetSuperMenu();

    if ( pcMenu == NULL ) {
	return;
    }
    const char* pzLabel = GetLabel();
    if ( pzLabel == NULL ) {
	return;
    }
    Rect   cFrame = GetFrame();
  
    font_height sHeight;

    pcMenu->GetFontHeight( &sHeight );

    if ( m_bIsHighlighted ) {
	pcMenu->SetFgColor( get_default_color( COL_SEL_MENU_BACKGROUND ) );
    } else {
	pcMenu->SetFgColor( get_default_color( COL_MENU_BACKGROUND ) );
    }

    float vStrWidth = pcMenu->GetStringWidth( pzLabel );

    pcMenu->FillRect( GetFrame() );

    if ( m_bIsHighlighted ) {
	pcMenu->SetFgColor( get_default_color( COL_SEL_MENU_TEXT ) );
	pcMenu->SetBgColor( get_default_color( COL_SEL_MENU_BACKGROUND ) );
    } else {
	pcMenu->SetFgColor( get_default_color( COL_MENU_TEXT ) );
	pcMenu->SetBgColor( get_default_color( COL_MENU_BACKGROUND ) );
    }

    float vCharHeight = sHeight.ascender + sHeight.descender + sHeight.line_gap;
    float y = cFrame.top + (cFrame.Height()+1.0f) / 2 - vCharHeight / 2 + sHeight.ascender + sHeight.line_gap * 0.5f;
  
    pcMenu->DrawString( pzLabel, Point( cFrame.left + 2, y ) );
      
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void MenuItem::DrawContent()
{
}

void MenuItem::Highlight( bool bHighlight )
{
    m_bIsHighlighted = bHighlight;
    Draw();
}



MenuSeparator::MenuSeparator() : MenuItem( static_cast<char*>(NULL), NULL )
{
}

MenuSeparator::~MenuSeparator()
{
}

Point MenuSeparator::GetContentSize()
{
    Menu* pcMenu = GetSuperMenu();

    if ( pcMenu != NULL ) {
	font_height sHeight;
	pcMenu->GetFontHeight( &sHeight );

	if ( pcMenu->GetLayout() == ITEMS_IN_ROW ) {
	    return( Point( 6.0f, 0.0f ) );
	} else {
	    return( Point( 0.0f, 6.0f ) );
	}
    }
    return( Point( 0.0f, 0.0f ) );
}

void MenuSeparator::Draw()
{
    Menu* pcMenu = GetSuperMenu();

    if ( pcMenu == NULL ) {
	return;
    }
    Rect cFrame = GetFrame();
    Rect cMFrame = pcMenu->GetBounds();

    if ( pcMenu->GetLayout() == ITEMS_IN_ROW ) {
	cFrame.top    = cMFrame.top;
	cFrame.bottom = cMFrame.bottom;
    } else {
	cFrame.left  = cMFrame.left;
	cFrame.right = cMFrame.right;
    }
    pcMenu->SetFgColor( get_default_color( COL_MENU_BACKGROUND ) );
    pcMenu->FillRect( cFrame );
    
    if ( pcMenu->GetLayout() == ITEMS_IN_ROW ) {
	float x = floor( cFrame.left + (cFrame.Width() + 1.0f) * 0.5f );
	pcMenu->SetFgColor( get_default_color( COL_SHADOW ) );
	pcMenu->DrawLine( Point( x, cFrame.top + 2.0f ), Point( x, cFrame.bottom - 2.0f ) );
	x += 1.0f;
	pcMenu->SetFgColor( get_default_color( COL_SHINE ) );
	pcMenu->DrawLine( Point( x, cFrame.top + 2.0f ), Point( x, cFrame.bottom - 2.0f ) );
    } else {
	float y = floor( cFrame.top + (cFrame.Height() + 1.0f) * 0.5f );
	pcMenu->SetFgColor( get_default_color( COL_SHADOW ) );
	pcMenu->DrawLine( Point( cFrame.left + 4.0f, y ), Point( cFrame.right - 4.0f, y ) );
	y += 1.0f;
	pcMenu->SetFgColor( get_default_color( COL_SHINE ) );
	pcMenu->DrawLine( Point( cFrame.left + 4.0f, y ), Point( cFrame.right - 4.0f, y ) );
    }
    
}

void MenuSeparator::DrawContent()
{
}

void MenuSeparator::Highlight( bool bHighlight )
{
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MenuWindow::MenuWindow( const Rect& cFrame, Menu* pcMenu ) :
    Window( cFrame, "menu", "menu", WND_NO_BORDER | WND_SYSTEM )
{
      // If the machine is very loaded the previous window hosting the menu
      // might still own it, so we must wait til it terminates.
    while( pcMenu->GetWindow() != NULL ) {
	snooze( 50000 );
    }
    m_pcMenu = pcMenu;
    AddChild( pcMenu );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MenuWindow::~MenuWindow()
{
    if ( m_pcMenu != NULL ) {
	RemoveChild( m_pcMenu );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Menu::Menu( Rect cFrame, const char* pzTitle, MenuLayout_e eLayout, uint32 nResizeMask, uint32 nFlags )
    : View( cFrame, "_menu_", nResizeMask, nFlags & ~WID_FULL_UPDATE_ON_RESIZE )
{
    m = new Private;
    m_bIsTracking	 = false;
    m_cItemBorders = Rect( 3, 2, 3, 2 );
    m_eLayout	 = eLayout;
    m_pcWindow	 = NULL;
    m_pcFirstItem     = NULL;
    m_nItemCount      = 0;
    m_pcSuperItem     = NULL;
    m_bHasOpenChilds  = false;
    m_bCloseOnMouseUp = false;
    m_hTrackPort      = -1;

    if ( NULL != pzTitle ) {
	m->m_cTitle = pzTitle;
//	m_pzTitle	=	new char[ strlen( pzTitle ) + 1 ];
//	strcpy( m_pzTitle, pzTitle );
    }	else {
//	m_pzTitle = NULL;
    }
    m_pcRoot = this;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Menu::~Menu()
{
//    delete[] m_pzTitle;

    while( m_pcFirstItem != NULL ) {
	MenuItem* pcItem = m_pcFirstItem;
	m_pcFirstItem = pcItem->m_pcNext;
	delete pcItem;
    }
    delete m;
}

int  Menu::Lock() const
{
    return( m_pcRoot->m->m_cMutex.Lock() );
}

void Menu::Unlock() const
{
    m_pcRoot->m->m_cMutex.Unlock();
}

std::string Menu::GetLabel() const
{
    return( m->m_cTitle );
}

MenuLayout_e Menu::GetLayout() const
{
    return( m_eLayout );
}

void Menu::FrameSized( const Point& cDelta )
{
    if ( cDelta.x > 0.0f ) {
	Rect cBounds = GetBounds();

	cBounds.left = cBounds.right - cDelta.x;
	Invalidate( cBounds );
	Flush();
    } else if ( cDelta.x < 0.0f ) {
	Rect cBounds = GetBounds();

	cBounds.left = cBounds.right - 2.0f;
	Invalidate( cBounds );
	Flush();
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::SetRoot( Menu* pcRoot )
{
    m_pcRoot = pcRoot;

    MenuItem* pcTmp;
  
    for ( pcTmp = m_pcFirstItem ; NULL != pcTmp ; pcTmp = pcTmp->m_pcNext ) {
	if ( pcTmp->m_pcSubMenu != NULL  ) {
	    pcTmp->m_pcSubMenu->SetRoot( pcRoot );
	}
    }
  
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::StartOpenTimer( bigtime_t nDelay )
{
    Looper* pcLooper = GetLooper();
    if ( pcLooper != NULL ) {
	pcLooper->Lock();
	pcLooper->AddTimer( this, 123, nDelay, true );
	pcLooper->Unlock();
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::OpenSelection()
{
    MenuItem* pcItem = FindMarked();
  
    if ( pcItem == NULL ) {
	return;
    }

    MenuItem* pcTmp;
  
    for ( pcTmp = m_pcFirstItem ; NULL != pcTmp ; pcTmp = pcTmp->m_pcNext ) {
	if ( pcTmp == pcItem ) {
	    continue;
	}
	if ( pcTmp->m_pcSubMenu != NULL && pcTmp->m_pcSubMenu->m_pcWindow != NULL ) {
	    pcTmp->m_pcSubMenu->Close( true, false );
	    break;
	}
    }
  
    if ( pcItem->m_pcSubMenu == NULL || pcItem->m_pcSubMenu->m_pcWindow != NULL ) {
	return;
    }
  
    Rect cFrame = pcItem->GetFrame();
    Point cScrPos;

    Point cScreenRes( Desktop().GetResolution() );

    Point cLeftTop(0,0);
    
    ConvertToScreen( &cLeftTop );
  
    if ( m_eLayout == ITEMS_IN_ROW ) {
	cScrPos.x = cLeftTop.x + cFrame.left - m_cItemBorders.left;

	float nHeight = pcItem->m_pcSubMenu->m_cSize.y;
	if ( cLeftTop.y + GetBounds().Height() + 1.0f + nHeight <= cScreenRes.y ) {
	    cScrPos.y = cLeftTop.y + GetBounds().Height()+1.0f;
	} else {
	    cScrPos.y = cLeftTop.y - nHeight;
	}
    } else {
	float nHeight = pcItem->m_pcSubMenu->m_cSize.y;

	cScrPos.x = cLeftTop.x + GetBounds().Width() + 1.0f - 4.0f;
	if ( cLeftTop.y + cFrame.top - m_cItemBorders.top + nHeight <= cScreenRes.y ) {
	    cScrPos.y = cLeftTop.y + cFrame.top - m_cItemBorders.top;
	} else {
	    cScrPos.y = cLeftTop.y + cFrame.bottom - nHeight + m_cItemBorders.top + pcItem->m_pcSubMenu->m_cItemBorders.bottom;
	}
    }
    m_bHasOpenChilds = true;
    pcItem->m_pcSubMenu->Open( cScrPos );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::TimerTick( int nID )
{
    AutoLocker __lock__( &m_pcRoot->m->m_cMutex );
//    s_cMutex.Lock();
    OpenSelection();
//    s_cMutex.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::SelectItem( MenuItem* pcItem )
{
    MenuItem* pcPrev = FindMarked();

    if ( pcItem == pcPrev ) {
	return;
    }
    if ( pcPrev != NULL ) {
	pcPrev->Highlight( false );
    }
    if ( pcItem != NULL ) {
	pcItem->Highlight( true );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::SelectPrev()
{
    MenuItem* pcTmp;
    MenuItem* pcPrev = NULL;

    for ( pcTmp = m_pcFirstItem ; NULL != pcTmp ; pcTmp = pcTmp->m_pcNext ) {
	if ( pcTmp->m_bIsHighlighted ) {
	    if ( pcPrev != NULL ) {
		SelectItem( pcPrev );
		Flush();
	    }
	    return;
	}
	pcPrev = pcTmp;
    }
    if ( m_eLayout == ITEMS_IN_COLUMN ) {
	Menu* pcSuper = GetSuperMenu();
	if ( pcSuper != NULL && pcSuper->m_eLayout == ITEMS_IN_ROW ) {
	    Close( false, false );
	}
    } else {
	SelectItem( m_pcFirstItem );
	Flush();
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::SelectNext()
{
    MenuItem* pcTmp;

    for ( pcTmp = m_pcFirstItem ; NULL != pcTmp ; pcTmp = pcTmp->m_pcNext ) {
	if ( pcTmp->m_bIsHighlighted || pcTmp->m_pcNext == NULL ) {
	    if ( pcTmp->m_pcNext != NULL ) {
		SelectItem( pcTmp->m_pcNext );
	    } else {
		SelectItem( pcTmp );
	    }
	    Flush();
	    return;
	}
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::Open( Point cScrPos )
{
    AutoLocker __lock__( &m_pcRoot->m->m_cMutex );
//    s_cMutex.Lock();
    if ( m_pcWindow != NULL ) {
//	s_cMutex.Unlock();
	return;
    }
    Point  cSize = GetPreferredSize( false );
    Rect   cBounds( 0, 0, cSize.x - 1, cSize.y - 1 );

    m_pcWindow = new MenuWindow( cBounds + cScrPos, this );

    m_pcRoot->m->m_cMutex.Lock();
    m_pcWindow->SetMutex( &m_pcRoot->m->m_cMutex );
    SetFrame( cBounds );
//    if ( m_pcWindow->Lock() == 0 ) {
	m_bIsTracking = true;
	MakeFocus( true );
//	m_pcWindow->Unlock();
//    }
    SelectItem( m_pcFirstItem );
    m_pcWindow->Show();

    Flush();
//    s_cMutex.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::Close( bool bCloseChilds, bool bCloseParent )
{
    AutoLocker __lock__( &m_pcRoot->m->m_cMutex );
    MenuItem* pcTmp;

    for ( pcTmp = m_pcFirstItem ; NULL != pcTmp ; pcTmp = pcTmp->m_pcNext ) {
	if ( pcTmp->m_bIsHighlighted ) {
	    pcTmp->Highlight( false );
	}
    }  
    if ( bCloseChilds ) {
	for ( pcTmp = m_pcFirstItem ; NULL != pcTmp ; pcTmp = pcTmp->m_pcNext ) {
	    if ( pcTmp->m_pcSubMenu != NULL /*&& pcTmp->m_pcSubMenu->m_pcWindow != NULL*/ ) {
		pcTmp->m_pcSubMenu->Close( bCloseChilds, bCloseParent );
	    }
	}
    }
 
    if ( m_pcWindow != NULL ) {
	MenuWindow* pcWnd = m_pcWindow;
	m_pcWindow = NULL; // Must clear this before we shutdown.
	pcWnd->ShutDown();

	Menu* pcParent = GetSuperMenu();
	if ( pcParent != NULL ) {
	      // NOTE: We activate the parents window even if was not
	      //       opened by the menu-system (GetWindow() vs m_pcWindow)
	    Window* pcWnd = pcParent->GetWindow();
	    if ( pcWnd != NULL ) {
		pcParent->MakeFocus( true );
	    }
	    pcParent->m_bHasOpenChilds = false;
	    pcParent->m_bIsTracking = true;
	}
    }
    if ( m->m_cCloseMsgTarget.IsValid() ) {
	m->m_cCloseMsgTarget.SendMessage( &m->m_cCloseMsg );
    }
    
    if ( bCloseParent ) {
	Menu* pcParent = GetSuperMenu();
	if ( pcParent != NULL ) {
	    pcParent->Close( false, true );
	}
    }
    m_bIsTracking = false;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::AttachedToWindow()
{
    AutoLocker __lock__( &m_pcRoot->m->m_cMutex );
//    s_cMutex.Lock();
    InvalidateLayout();
//    s_cMutex.Unlock();
//  SetTargetForItems( GetWindow() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::DetachedFromWindow()
{
    AutoLocker __lock__( &m_pcRoot->m->m_cMutex );
    Close( true, false );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::WindowActivated( bool bIsActive )
{
    AutoLocker __lock__( &m_pcRoot->m->m_cMutex );
//    s_cMutex.Lock();
    if ( false == bIsActive && m_bHasOpenChilds == false ) {
	Close( false, true );
    
	if ( m_hTrackPort != -1 ) {
	    MenuItem* pcItem = NULL;
	    if ( send_msg( m_hTrackPort, 1, &pcItem, sizeof(pcItem) ) < 0 ) {
		dbprintf( "Error: Menu::WindowActivated() failed to send message to m_hTrackPort\n" );
	    }
	}
    }
//    s_cMutex.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::MouseDown( const Point& cPosition, uint32 nButtons )
{
    AutoLocker __lock__( &m_pcRoot->m->m_cMutex );

    if ( GetBounds().DoIntersect( cPosition ) ) {
//	s_cMutex.Lock();
	MenuItem* pcItem = GetItemAt( cPosition );
	SetCloseOnMouseUp( false );
	
	if ( pcItem != NULL ) {
	    MakeFocus( true );
	    m_bIsTracking = true;
	    SelectItem( pcItem );
	    if ( m_eLayout == ITEMS_IN_ROW ) {
		OpenSelection();
	    } else {
		StartOpenTimer( 200000 );
	    }
	    Flush();
	}
	
	OpenSelection();
//	s_cMutex.Unlock();
    } else {
//	s_cMutex.Lock();
	if ( m_bHasOpenChilds == false )
	{
	    Close( false, true );
	    if ( m_hTrackPort != -1 ) {
		MenuItem* pcItem = NULL;
		if ( send_msg( m_hTrackPort, 1, &pcItem, sizeof(pcItem) ) < 0 ) {
		    dbprintf( "Error: Menu::WindowActivated() failed to send message to m_hTrackPort\n" );
		}
	    }
	}
//	s_cMutex.Unlock();
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::EndSession( MenuItem* pcSelItem )
{
    Menu* pcSuper = GetSuperMenu();

    if ( NULL != pcSuper )
    {
	Window* pcWindow = pcSuper->GetWindow();

	if ( NULL != pcWindow )
	{
	    Message	cMsg( M_MENU_EVENT );

	    cMsg.AddPointer( "_item", pcSelItem /* GetSuperItem()*/ );

	    pcWindow->PostMessage( &cMsg, pcSuper );
	}
    }
    if ( m_hTrackPort != -1 ) {
	if ( send_msg( m_hTrackPort, 1, &pcSelItem, sizeof(&pcSelItem) ) < 0 ) {
	    dbprintf( "Error: Menu::EndSession() failed to send message to m_hTrackPort\n" );
	}
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData )
{
    AutoLocker __lock__( &m_pcRoot->m->m_cMutex );
//    s_cMutex.Lock();

    if ( GetBounds().DoIntersect( cPosition ) ) {
    
	MenuItem*	pcItem	= FindMarked();

	if ( NULL != pcItem && pcItem == GetItemAt( cPosition ) && NULL == pcItem->m_pcSubMenu ) {
	    pcItem->Invoke();
	    Close( false, true );
	}
    } else {
	if ( m_bCloseOnMouseUp )
	{
	    Close( false, true );
	    if ( m_hTrackPort != -1 ) {
		MenuItem* pcItem = NULL;
		if ( send_msg( m_hTrackPort, 1, &pcItem, sizeof(pcItem) ) < 0 ) {
		    dbprintf( "Error: Menu::WindowActivated() failed to send message to m_hTrackPort\n" );
		}
	    }
	}
    }
    SetCloseOnMouseUp( false );
//    s_cMutex.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::MouseMove( const Point& cPosition, int nCode, uint32 nButtons, Message* pcData )
{
    AutoLocker __lock__( &m_pcRoot->m->m_cMutex );
//    s_cMutex.Lock();
    if ( nButtons & 0x01 ) {
	SetCloseOnMouseUp( true );
    }
    if ( GetBounds().DoIntersect( cPosition ) == false ) {
	Menu* pcParent = GetSuperMenu();
	if ( pcParent != NULL ) {
	    pcParent->MouseMove( pcParent->ConvertFromScreen( ConvertToScreen( cPosition ) ), nCode, nButtons, pcData );
	}
    } else {
	if ( m_bIsTracking ) {
	    MenuItem*	pcItem	= GetItemAt( cPosition );

	    if ( pcItem != NULL )
	    {
		SelectItem( pcItem );
		if ( m_eLayout == ITEMS_IN_ROW ) {
		    OpenSelection();
		} else {
		    StartOpenTimer( 200000 );
		}
		Flush();
	    }
	}
    }
//    s_cMutex.Unlock();
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers )
{
    AutoLocker __lock__( &m_pcRoot->m->m_cMutex );
//    s_cMutex.Lock();
  
    switch( pzString[0] )
    {
	case VK_UP_ARROW:
	    if ( m_eLayout == ITEMS_IN_ROW ) {
	    } else {
		SelectPrev();
		StartOpenTimer( 1000000 );
	    }
	    break;
	case VK_DOWN_ARROW:
	    if ( m_eLayout == ITEMS_IN_ROW ) {
		OpenSelection();
	    } else {
		SelectNext();
		StartOpenTimer( 1000000 );
	    }
	    break;
	case VK_LEFT_ARROW:
	    if ( m_eLayout == ITEMS_IN_ROW ) {
		SelectPrev();
		OpenSelection();
//	StartOpenTimer( 1000000 );
	    } else {
		Menu* pcSuper = GetSuperMenu();
		if ( pcSuper != NULL ) {
		    if ( pcSuper->m_eLayout == ITEMS_IN_COLUMN ) {
			Close( false, false );
		    } else {
			pcSuper->SelectPrev();
			pcSuper->OpenSelection();
		    }
		}
	    }
	    break;
	case VK_RIGHT_ARROW:
	    if ( m_eLayout == ITEMS_IN_ROW ) {
		SelectNext();
		OpenSelection();
//	StartOpenTimer( 1000000 );
	    } else {
		Menu* pcSuper = GetSuperMenu();
		if ( pcSuper != NULL ) {
		    MenuItem* pcItem = FindMarked();
		    if ( pcItem != NULL && pcItem->m_pcSubMenu != NULL ) {
			OpenSelection();
		    } else if ( pcSuper->m_eLayout == ITEMS_IN_ROW ) {
			pcSuper->SelectNext();
			pcSuper->OpenSelection();
		    }
		}
	    }
	    break;
	case VK_ENTER:
	{
	    MenuItem* pcItem = FindMarked();
	    if ( pcItem != NULL ) {
		if ( pcItem->m_pcSubMenu != NULL ) {
		    OpenSelection();
		} else  {
		    pcItem->Invoke();
		    Close( false, true );
		}
	    }
	    break;
	}
	case 27: // ESC
	    Close( false, false );
	    Flush();
	    break;
	default:
	    View::KeyDown( pzString, pzRawString, nQualifiers );
    }
//    s_cMutex.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::Paint( const Rect& cUpdateRect )
{
    AutoLocker __lock__( &m_pcRoot->m->m_cMutex );
    MenuItem*	pcItem;

    SetFgColor( get_default_color( COL_MENU_BACKGROUND ) );
    FillRect( GetBounds() );

//    s_cMutex.Lock();
    for ( pcItem = m_pcFirstItem ; NULL != pcItem ; pcItem = pcItem->m_pcNext )	{
	pcItem->Draw();
    }
//    s_cMutex.Unlock();

    DrawFrame( GetBounds(), FRAME_RAISED | FRAME_THIN | FRAME_TRANSPARENT );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool Menu::AddItem( const char* pzLabel, Message* pcMessage )
{
    return( AddItem( new MenuItem( pzLabel, pcMessage ) ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool Menu::AddItem( MenuItem* pcItem )
{
    return( AddItem( pcItem, m_nItemCount ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool Menu::AddItem( MenuItem* pcItem, int nIndex )
{
    bool	bResult = false;

    pcItem->m_pcSuperMenu = this;

    if ( nIndex == 0 ) {
	pcItem->m_pcNext = m_pcFirstItem;
	m_pcFirstItem	= pcItem;
	bResult = true;
    } else {
	MenuItem*	pcPrev;
	int		i;

	for ( i = 1, pcPrev = m_pcFirstItem ; i < nIndex && NULL != pcPrev ; ++i ) {
	    pcPrev = pcPrev->m_pcNext;
	}
	if ( pcPrev ) {
	    pcItem->m_pcNext = pcPrev->m_pcNext;
	    pcPrev->m_pcNext = pcItem;
	    bResult = true;
	}
    }

    if ( bResult ) {
	if ( pcItem->m_pcSubMenu != NULL ) {
	    pcItem->m_pcSubMenu->SetRoot( m_pcRoot );
	}
	m_nItemCount++;
	InvalidateLayout();
    }

    return( bResult );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool Menu::AddItem( Menu* pcMenu )
{
    pcMenu->SetRoot( m_pcRoot );
    return( AddItem( pcMenu, m_nItemCount ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool Menu::AddItem( Menu* pcMenu, int nIndex )
{
    pcMenu->SetRoot( m_pcRoot );
    return( AddItem( new MenuItem( pcMenu, NULL ), nIndex ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MenuItem* Menu::RemoveItem( int nIndex )
{
    MenuItem** ppcTmp;
    MenuItem*  pcItem = NULL;
  
    for ( ppcTmp = &m_pcFirstItem ; *ppcTmp != NULL ; ppcTmp = &(*ppcTmp)->m_pcNext ) {
	if ( nIndex == 0 ) {
	    pcItem = *ppcTmp;
	    *ppcTmp = pcItem->m_pcNext;
	    if ( pcItem->m_pcSubMenu != NULL ) {
		pcItem->m_pcSubMenu->SetRoot( pcItem->m_pcSubMenu );
	    }
	    m_nItemCount--;
	    InvalidateLayout();
	    break;
	}
	nIndex--;
    }
    return( pcItem );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool Menu::RemoveItem( MenuItem* pcItem )
{
    MenuItem** ppcTmp;
  
    for ( ppcTmp = &m_pcFirstItem ; *ppcTmp != NULL ; ppcTmp = &(*ppcTmp)->m_pcNext ) {
	if ( *ppcTmp == pcItem ) {
	    *ppcTmp = pcItem->m_pcNext;
	    m_nItemCount--;
	    InvalidateLayout();
	    if ( pcItem->m_pcSubMenu != NULL ) {
		pcItem->m_pcSubMenu->SetRoot( pcItem->m_pcSubMenu );
	    }
	    return( true );
	}
    }
    return( false );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool Menu::RemoveItem( Menu* pcMenu )
{
    MenuItem** ppcTmp;
    MenuItem*  pcItem = NULL;
  
    for ( ppcTmp = &m_pcFirstItem ; *ppcTmp != NULL ; ppcTmp = &(*ppcTmp)->m_pcNext ) {
	pcItem = *ppcTmp;
	if ( pcItem->m_pcSubMenu == pcMenu ) {
	    *ppcTmp = pcItem->m_pcNext;
	    pcMenu->SetRoot( pcMenu );
	    m_nItemCount--;
	    InvalidateLayout();
	    return( true );
	}
    }
    return( false );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MenuItem* Menu::GetItemAt( Point cPosition ) const
{
    MenuItem* pcItem;

    for ( pcItem = m_pcFirstItem ; NULL != pcItem ; pcItem = pcItem->m_pcNext )	{
	if ( pcItem->m_cFrame.DoIntersect( cPosition ) ) {
	    break;
	}
    }
    return( pcItem );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MenuItem* Menu::GetItemAt( int nIndex ) const
{
    MenuItem*	pcItem = NULL;

    if ( nIndex >= 0 && nIndex < m_nItemCount ) {
	for ( pcItem = m_pcFirstItem ; NULL != pcItem ; pcItem = pcItem->m_pcNext ) {
	    if ( 0 == nIndex ) {
		return( pcItem );
	    }
	    nIndex--;
	}
    }
    return( NULL );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Menu*	Menu::GetSubMenuAt( int nIndex ) const
{
    MenuItem*	pcItem = GetItemAt( nIndex );

    if ( NULL != pcItem ) {
	return( pcItem->m_pcSubMenu );
    } else {
	return( NULL );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int Menu::GetItemCount() const
{
    return( m_nItemCount );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int Menu::GetIndexOf( MenuItem* pcItem ) const
{
    MenuItem*	pcPtr;
    int		nIndex = 0;

    for ( pcPtr = m_pcFirstItem ; NULL != pcPtr ; pcPtr = pcPtr->m_pcNext )
    {
	if ( pcPtr == pcItem ) {
	    return( nIndex );
	}
	nIndex++;
    }
    return( -1 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int Menu::GetIndexOf( Menu* pcMenu ) const
{
    MenuItem*	pcPtr;
    int		nIndex = 0;

    for ( pcPtr = m_pcFirstItem ; NULL != pcPtr ; pcPtr = pcPtr->m_pcNext )
    {
	if ( pcPtr->m_pcSubMenu == pcMenu ) {
	    return( nIndex );
	}
	nIndex++;
    }
    return( -1 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MenuItem* Menu::FindItem( int nCode ) const
{
    MenuItem*	pcItem = NULL;

    for ( pcItem = m_pcFirstItem ; NULL != pcItem ; pcItem = pcItem->m_pcNext )
    {
	if ( pcItem->m_pcSubMenu != NULL ) {
	    MenuItem* pcTmp = pcItem->m_pcSubMenu->FindItem( nCode );
	    if ( pcTmp != NULL ) {
		return( pcTmp );
	    }
	} else {
	    Message* pcMsg = pcItem->GetMessage();
	    if ( pcMsg != NULL && pcMsg->GetCode() == nCode ) {
		return( pcItem );
	    }
	}
    }
    return( NULL );
}

void Menu::SetCloseOnMouseUp( bool bFlag )
{
    MenuItem* pcItem;

    m_bCloseOnMouseUp = bFlag;

    for ( pcItem = m_pcFirstItem ; NULL != pcItem ; pcItem = pcItem->m_pcNext ) {
	if ( pcItem->m_pcSubMenu != NULL ) {
	    pcItem->m_pcSubMenu->SetCloseOnMouseUp( bFlag );
	}
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MenuItem* Menu::FindItem( const char* pzName ) const
{
    MenuItem* pcItem;

    for ( pcItem = m_pcFirstItem ; NULL != pcItem ; pcItem = pcItem->m_pcNext )
    {
	if ( pcItem->m_pcSubMenu != NULL ) {
	    MenuItem* pcTmp = pcItem->m_pcSubMenu->FindItem( pzName );
	    if ( pcTmp != NULL ) {
		return( pcTmp );
	    }
	} else {
	    if ( NULL != pcItem->m_pzLabel && strcmp( pcItem->m_pzLabel, pzName ) == 0 ) {
		break;
	    }
	}
    }
    return( pcItem );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t Menu::SetTargetForItems( Handler* pcTarget )
{
    MenuItem*	pcItem;

    for ( pcItem = m_pcFirstItem ; NULL != pcItem ; pcItem = pcItem->m_pcNext )
    {
	if ( NULL == pcItem->m_pcSubMenu ) {
	    pcItem->SetTarget( pcTarget );
	} else {
	    pcItem->m_pcSubMenu->SetTargetForItems( pcTarget );
	}
    }
    return( 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t Menu::SetTargetForItems( Messenger cMessenger )
{
    MenuItem*	pcItem;

    for ( pcItem = m_pcFirstItem ; NULL != pcItem ; pcItem = pcItem->m_pcNext )
    {
	if ( NULL == pcItem->m_pcSubMenu ) {
	    pcItem->SetTarget( cMessenger );
	} else {
	    pcItem->m_pcSubMenu->SetTargetForItems( cMessenger );
	}
    }
    return( 0 );
}

void Menu::SetCloseMessage( const Message& cMsg )
{
    m->m_cCloseMsg = cMsg;
}

void Menu::SetCloseMsgTarget( const Messenger& cTarget )
{
    m->m_cCloseMsgTarget = cTarget;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MenuItem* Menu::FindMarked() const
{
    MenuItem* pcItem;

    for ( pcItem = m_pcFirstItem ; NULL != pcItem ; pcItem = pcItem->m_pcNext ) {
	if ( pcItem->m_bIsHighlighted ) {
	    break;
	}
    }
    return( pcItem );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Menu*	Menu::GetSuperMenu() const
{
    if ( NULL != m_pcSuperItem ) {
	return( m_pcSuperItem->m_pcSuperMenu );
    } else {
	return( NULL );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MenuItem* Menu::GetSuperItem()
{
    return( m_pcSuperItem );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point Menu::GetPreferredSize( bool bLargest ) const
{
    return( m_cSize + Point( 2.0f, 2.0f ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::InvalidateLayout()
{
    MenuItem*	pcItem;

    m_cSize = Point( 0.0f, 0.0f );

    switch( m_eLayout )
    {
	case ITEMS_IN_ROW:
	    for ( pcItem = m_pcFirstItem ; NULL != pcItem ; pcItem = pcItem->m_pcNext )
	    {
		Point	cSize = pcItem->GetContentSize();

		pcItem->m_cFrame = Rect( m_cSize.x + m_cItemBorders.left,
					 m_cItemBorders.top,
					 m_cSize.x + cSize.x + m_cItemBorders.left - 1,
					 cSize.y + m_cItemBorders.bottom - 1 );

		pcItem->m_cFrame += Point( 1.0f, 1.0f );	// Menu border

		m_cSize.x += cSize.x + m_cItemBorders.left + m_cItemBorders.right;

		if ( cSize.y + m_cItemBorders.top + m_cItemBorders.bottom + 2 > m_cSize.y ) {
		    m_cSize.y = cSize.y + m_cItemBorders.top + m_cItemBorders.bottom + 2;
		}
	    }
	    break; 
	case ITEMS_IN_COLUMN:
	    for ( pcItem = m_pcFirstItem ; NULL != pcItem ; pcItem = pcItem->m_pcNext )
	    {
		Point	cSize = pcItem->GetContentSize();

		pcItem->m_cFrame = Rect( m_cItemBorders.left,
					 m_cSize.y + m_cItemBorders.top,
					 cSize.x + m_cItemBorders.right - 1,
					 m_cSize.y + cSize.y + m_cItemBorders.top - 1 );

		m_cSize.y += cSize.y + m_cItemBorders.top + m_cItemBorders.bottom;

		if ( cSize.x + m_cItemBorders.left + m_cItemBorders.right > m_cSize.x ) {
		    m_cSize.x = cSize.x + m_cItemBorders.left + m_cItemBorders.right;
		}
	    }
	    for ( pcItem = m_pcFirstItem ; NULL != pcItem ; pcItem = pcItem->m_pcNext ) {
		pcItem->m_cFrame.right = m_cSize.x - 2;
	    }
	    break;
	case ITEMS_CUSTOM_LAYOUT:
	    dbprintf( "Error: ITEMS_CUSTOM_LAYOUT not supported\n" );
	    break;
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MenuItem* Menu::Track( const Point& cScreenPos )
{
    m_hTrackPort = create_port( "menu_track_port", 1 );
    if ( m_hTrackPort < 0 ) {
	return( NULL );
    }
    Point cSize = GetPreferredSize( false );
    Rect   cBounds( 0, 0, cSize.x - 1, cSize.y - 1 );

    m_pcWindow = new MenuWindow( cBounds + cScreenPos, this );
    m_pcRoot->m->m_cMutex.Lock();
    m_pcWindow->SetMutex( &m_pcRoot->m->m_cMutex );
    
    SetFrame( cBounds );

    m_pcWindow->Show();

//    if ( m_pcWindow->Lock() == 0 )
//    {
	MakeFocus( true );
//	m_pcWindow->Unlock();
//    }
    m_bIsTracking = true;

    uint32 nCode;
    MenuItem* pcSelItem;
    get_msg( m_hTrackPort, &nCode, &pcSelItem, sizeof( &pcSelItem ) );
  
    assert( m_pcWindow == NULL );
    delete_port( m_hTrackPort );
    m_hTrackPort = -1;
    return( pcSelItem );
}
