/*	CodeEdit - A programmers editor for Atheos
	Copyright (c) 2002 Andreas Engh-Halstvedt
 
	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Library General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.
	
	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Library General Public License for more details.
	
	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free
	Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
	MA 02111-1307, USA
*/
#include "StatusBar.h"

#include <gui/font.h>
#include <util/looper.h>


class StatusPanel
{
public:
    string text;
    StatusBar::panelmode mode;
    float width;

    StatusPanel()
    {
        text="";
        mode=StatusBar::CONSTANT;
        width=50;
    }
};


StatusBar::StatusBar(const os::Rect &r, const string &s, int count, uint32 i0, uint32 i1)
        : os::View(r, s, i0, i1),
        panelCount(count)
{
    panels=new StatusPanel[panelCount];
}

StatusBar::~StatusBar()
{
    delete[] panels;
}

os::Point StatusBar::GetPreferredSize(bool largest) const
{
    os::font_height fh;
    GetFont()->GetHeight(&fh);

    return os::Point(largest?10000.0f:20.0f, fh.ascender+fh.descender+fh.line_gap+10);
}

void StatusBar::Paint(const os::Rect &r)
{
    os::Rect b=GetBounds();
    os::font_height fh;
    GetFont()->GetHeight(&fh);


    SetFgColor(GetBgColor());
    FillRect(r);

    SetFgColor(255, 255, 255);
    DrawLine(os::Point(0, 0), os::Point(b.right, 0));


    float x=3;
    for(int a=0;a<panelCount;++a)
    {
        switch(panels[a].mode)
        {
        case RELATIVE:
            {
                if(panels[a].width<0)
                    continue;

                float w=(b.right-b.left)*panels[a].width;
                drawPanel(panels[a].text, x, 3, x+w, b.bottom-3, fh.ascender+fh.line_gap);
                x+=w;
                break;
            }
        case FILL://TODO: handle FILL other places than at the end
            {
                float w=b.right-b.left-3-x;
                if(w<0)
                    continue;

                drawPanel(panels[a].text, x, 3, x+w, b.bottom-3, fh.ascender+fh.line_gap);
                x+=w;
                break;
            }
        case CONSTANT:
        default:
            if(panels[a].width<0)
                continue;

            drawPanel(panels[a].text, x, 3, x+panels[a].width, b.bottom-3, fh.ascender+fh.line_gap);
            x+=panels[a].width;
            break;
        }
        x+=5;
    }

}

void StatusBar::drawPanel(const string& str, float l, float t, float r, float b, float tbl)
{
    SetFgColor(255, 255, 255);

    DrawLine(os::Point(l, t), os::Point(l, b));
    DrawLine(os::Point(l, b), os::Point(r, b));

    SetFgColor(0, 0, 0);
    DrawLine(os::Point(l+1, t), os::Point(r, t));
    DrawLine(os::Point(r, t), os::Point(r, b-1));

    DrawString(os::Point(l+4, t+2+tbl), str);
}

void StatusBar::setText(const string& s, int panel, bigtime_t timeout)
{
    if(panel<0 || panel>=panelCount)
        return;

    panels[panel].text=s;
    if(timeout>0)
    {
        os::Looper *l=GetLooper();
        if(l)
        {
            l->RemoveTimer(this, panel);
            l->AddTimer(this, panel, timeout);
        }
    }
    Invalidate();
    Flush();
}

void StatusBar::configurePanel(int panel, panelmode mode, float width)
{
    if(panel<0 || panel>=panelCount)
        return;

    if(mode!=RELATIVE && mode!=FILL)
        mode=CONSTANT;

    panels[panel].mode=mode;
    panels[panel].width=width;
    Invalidate();
}

void StatusBar::TimerTick(int i)
{
    if(i<0 || i>=panelCount)
        return;

    panels[i].text="";
    Invalidate();
    Flush();
}



