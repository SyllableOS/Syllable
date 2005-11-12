// Whisper -:-  (C)opyright 2001-2004 Kristian Van Der Vliet
//              (C)opyright 2004 Jonas Jarvoll
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "status_bar.h"

using namespace os;

StatusBar::StatusBar( const os::Rect& cFrame, const std::string& cTitle) : View(cFrame, cTitle, CF_FOLLOW_LEFT|CF_FOLLOW_RIGHT|CF_FOLLOW_BOTTOM)
{
    GetFontHeight( &m_sFontHeight );
}

Point StatusBar::GetPreferredSize(bool bLargest) const
{
    if(bLargest)
    {
        return(Point(COORD_MAX, 12.0f + m_sFontHeight.ascender + m_sFontHeight.descender + m_sFontHeight.line_gap));
    }
    else
    {
        return(Point(12.0f, 12.0f + m_sFontHeight.ascender + m_sFontHeight.descender + m_sFontHeight.line_gap));
    }
}

void StatusBar::FrameSized(const Point& cDelta)
{
    if (cDelta.x < 0.0f)
    {
        Rect cBounds = GetBounds();
        cBounds.left = cBounds.right - 5.0f;
        Invalidate(cBounds);
    }
    else if ( cDelta.x > 0.0f )
    {
        Rect cBounds = GetBounds();
        cBounds.left = cBounds.right - 5.0f - cDelta.x;
        Invalidate(cBounds);
    }

    if (cDelta.y < 0.0f)
    {
        Rect cBounds = GetBounds();
        cBounds.top = cBounds.bottom - 5.0f;
        Invalidate(cBounds);
    }
    else if (cDelta.y > 0.0f)
    {
        Rect cBounds = GetBounds();
        cBounds.top = cBounds.bottom - 5.0f - cDelta.y;
        Invalidate(cBounds);
    }

    Flush();
}

void StatusBar::Paint(const Rect& /*cUpdateRect*/)
{
    Rect cBounds = GetBounds();
    DrawFrame(cBounds, FRAME_RAISED);
    cBounds.Resize(4.0f, 4.0f, -4.0f, -4.0f);
    DrawFrame(cBounds, FRAME_RECESSED);
    SetFgColor(0, 0, 0);
    DrawString(Point(8.0f, 6.0f + ceil(m_sFontHeight.ascender + m_sFontHeight.line_gap * 0.5f)),m_cStatusStr);
}

void StatusBar::SetStatus(const std::string& cText)
{
    m_cStatusStr = cText;
    Rect cBounds = GetBounds();
    cBounds.Resize(6.0f, 6.0f, -6.0f, -6.0f);
    Invalidate(cBounds);
    Flush();
}

