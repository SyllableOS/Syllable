
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

#include "winselect.h"
#include "swindow.h"
#include "server.h"
#include "sfont.h"
#include "bitmap.h"
#include "config.h"

#include <gui/desktop.h>
#include <util/messenger.h>
static Color32_s Tint( const Color32_s & sColor, float vTint )
{
    int r = int ( ( float ( sColor.red ) * vTint + 127.0f * ( 1.0f - vTint ) ) );
    int g = int ( ( float ( sColor.green ) * vTint + 127.0f * ( 1.0f - vTint ) ) );
    int b = int ( ( float ( sColor.blue ) * vTint + 127.0f * ( 1.0f - vTint ) ) );

    if ( r < 0 )
        r = 0;
    else if ( r > 255 )
        r = 255;
    if ( g < 0 )
        g = 0;
    else if ( g > 255 )
        g = 255;
    if ( b < 0 )
        b = 0;
    else if ( b > 255 )
        b = 255;
    return ( Color32_s( r, g, b, sColor.alpha ) );
}

WinSelect::WinSelect():Layer( NULL, g_pcTopView, "", Rect( 0, 0, 1, 1 ), 0, NULL )
{
    FontNode * pcNode = new FontNode();
    const font_properties *psProp = AppserverConfig::GetInstance()->GetFontConfig( DEFAULT_FONT_REGULAR );

    pcNode->SetFamilyAndStyle( psProp->m_cFamily, psProp->m_cStyle );
    pcNode->SetProperties( int ( psProp->m_vSize * 64.0f ), int ( psProp->m_vShear * 64.0f ), int ( psProp->m_vRotation * 64.0f ), os::FPF_SMOOTHED );

    SetFont( pcNode );
    pcNode->Release();
    SFontInstance * pcFontInst = m_pcFont->GetInstance();
    int nAscender = 5;
    int nDescender = 2;

    if ( pcFontInst != NULL )
    {
        nAscender = pcFontInst->GetAscender();
        nDescender = pcFontInst->GetDescender();
    }
    int nHeight = 10;
    int nWidth = 50;

    m_pcOldFocusWindow = get_active_window( true );
    Layer * pcLayer;
    for ( pcLayer = g_pcTopView->GetTopChild(); pcLayer != NULL; pcLayer = pcLayer->GetLowerSibling(  ) )

    {
        if ( pcLayer->IsBackdrop() || pcLayer->IsVisible(  ) == false )
        {
            continue;
        }
        SrvWindow * pcWindow = pcLayer->GetWindow();
        if ( pcWindow == NULL )
        {
            continue;
        }
        m_cWindows.push_back( pcWindow );
        nHeight += nAscender + ( -nDescender ) + 2;
        const char *pzStr = pcWindow->GetTitle();

        if ( pzStr[0] == '\0' )
        {
            pzStr = "*unnamed*";
        }
        SFontInstance *pcFontInst = m_pcFont->GetInstance();
        int nStrWidth = 0;

        if ( pcFontInst != NULL )
        {
            nStrWidth = pcFontInst->GetStringWidth( pzStr, strlen( pzStr ) );
        }
        if ( nStrWidth > nWidth )
        {
            nWidth = nStrWidth;
        }
    }
    m_nCurSelect = ( m_cWindows.size() > 1 ) ? 1 : 0;
    if ( m_cWindows.size() > 0 )
    {
        m_cWindows[m_nCurSelect]->MakeFocus( true );
    }
    int nDesktop = os::Desktop::ACTIVE_DESKTOP;

    screen_mode sMode;
    get_desktop_config( &nDesktop, &sMode, NULL );
    SetFrame( Rect( 0, 0, nWidth + 10, nHeight ) + Point( sMode.m_nWidth / 2 - nWidth / 2, sMode.m_nHeight / 2 - nHeight / 2 ) );
}
WinSelect::~WinSelect()
{
    g_pcTopView->RemoveChild( this );
}
void WinSelect::UpdateWinList( bool bMoveToFront, bool bSetFocus )
{
    if ( m_cWindows.size() == 0 )
    {
        return;
    }
    if ( m_nCurSelect != 0 || m_cWindows.size() == 1 )

    {
        SrvWindow * pcWindow = m_cWindows[m_nCurSelect];

        // We search for the window to assert it's not been closed,
        // or moved to another desctop since we built the list.
        if ( bMoveToFront )
        {
            for ( Layer * pcLayer = g_pcTopView->GetTopChild(); NULL != pcLayer; pcLayer = pcLayer->GetLowerSibling(  ) )
            {
                if ( pcWindow == pcLayer->GetWindow() )
                {
                    g_pcTopView->RemoveChild( pcWindow->GetTopView() );
                    g_pcTopView->AddChild( pcWindow->GetTopView(), true );

                    // Move the window inside screen boundaries if necesarry.
                    if ( pcWindow->GetFrame( false ).DoIntersect( Rect( 0, 0, g_pcScreenBitmap->m_nWidth - 1, g_pcScreenBitmap->m_nHeight - 1 ) ) == false )

                    {
                        Rect cFrame = pcWindow->GetFrame();
                        cFrame = Rect( 0, 0, cFrame.Width(), cFrame.Height(  ) ) + Point( g_pcScreenBitmap->m_nWidth / 2 - ( cFrame.Width(  ) + 1.0f ) / 2, g_pcScreenBitmap->m_nHeight / 2 - ( cFrame.Height(  ) + 1.0f ) / 2 );
                        pcWindow->SetFrame( cFrame );
                        if ( pcWindow->GetAppTarget() != NULL )
                        {
                            Message cMsg( M_WINDOW_FRAME_CHANGED );
                            cMsg.AddRect( "_new_frame", cFrame );
                            if ( pcWindow->GetAppTarget()->SendMessage( &cMsg ) < 0 )
                            {
                                dbprintf( "WinSelect::UpdateWinList() failed to send M_WINDOW_FRAME_CHANGED to %s\n", pcWindow->GetTitle() );
                            }
                        }
                    }
                    break;
                }
            }
        }
    }
    if ( bSetFocus == false && m_pcOldFocusWindow != NULL )
    {
        for ( Layer * pcLayer = g_pcTopView->GetTopChild(); NULL != pcLayer; pcLayer = pcLayer->GetLowerSibling(  ) )
        {
            if ( m_pcOldFocusWindow == pcLayer->GetWindow() )
            {
                m_pcOldFocusWindow->MakeFocus( true );
                break;
            }
        }
    }
}
void WinSelect::Paint( const IRect & cUpdateRect, bool bUpdate )
{
    if ( bUpdate )
    {
        BeginUpdate();
    }
    SFontInstance * pcFontInst = m_pcFont->GetInstance();
    int nAscender = 5;
    int nDescender = 2;

    if ( pcFontInst != NULL )
    {
        nAscender = pcFontInst->GetAscender();
        nDescender = pcFontInst->GetDescender();
    }
    Rect cRect = GetBounds();
    SetFgColor( get_default_color( COL_NORMAL ) );
    FillRect( cRect );
    Rect cOBounds = GetBounds();
    Rect cIBounds = cOBounds;
    cIBounds.left += 1;
    cIBounds.right -= 1;
    cIBounds.top += 1;
    cIBounds.bottom -= 1;
    SetFgColor( get_default_color( COL_NORMAL ) );
    DrawFrame( cOBounds, FRAME_RAISED | FRAME_THIN | FRAME_TRANSPARENT );
    SetFgColor( Tint( get_default_color( COL_NORMAL ), 4.5 ) );
    DrawFrame( cIBounds, FRAME_RECESSED | FRAME_THIN | FRAME_TRANSPARENT );
    cRect.left = 4;
    cRect.right = cRect.left + nAscender + ( -nDescender ) + 2;
    for ( uint i = 0; i < m_cWindows.size(); ++i )

    {
        int nItem = ( m_nCurSelect + i ) % m_cWindows.size();

        SrvWindow * pcWindow = m_cWindows[nItem];
        SetFgColor( 0, 0, 0, 0 );
        MovePenTo( cRect.left + 5, cRect.top + 5 + nAscender );
        const char *pzStr = pcWindow->GetTitle();

        if ( pzStr[0] == '\0' )
        {
            pzStr = "*unnamed*";
        }
        DrawString( pzStr, -1 );
        cRect += Point( 0, nAscender + ( -nDescender ) + 2 );
    }
    if ( bUpdate )
    {
        EndUpdate();
    }
}
void WinSelect::Step( bool bForward )
{
    if ( m_cWindows.size() == 0 )
    {
        return;
    }
    if ( bForward )
    {
        m_nCurSelect = ( m_nCurSelect + 1 ) % m_cWindows.size();
    }
    else
    {
        m_nCurSelect = ( m_nCurSelect + m_cWindows.size() - 1 ) % m_cWindows.size(  );
    }
    Paint( GetBounds(), false );
    if ( m_cWindows.size() > 0 )
    {
        m_cWindows[m_nCurSelect]->MakeFocus( true );
    }
}

std::vector < SrvWindow * >WinSelect::GetWindows()
{
    return m_cWindows;
}
