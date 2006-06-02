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
#include "statusbar.h"

#include <gui/font.h>
#include <util/looper.h>

//holds private information
class StatusPanel
{
public:
    String cText;
    StatusBar::panelmode mode;
    float vWidth;

    StatusPanel()
    {
        cText="";
        mode=StatusBar::CONSTANT;
        vWidth=50;
    }
};

/********************************************************************
* Description: Statusbar constructor
* Author: Andreas Engh-Halstvedt(with modifications by Rick Caudill)
* Date: Thu Mar 18 20:07:11 2004
*********************************************************************/
StatusBar::StatusBar(const os::Rect& cRect, const String& cName, int nCount, uint32 nResizeMask, uint32 nFlags): os::View(cRect,cName, nResizeMask, nFlags),nPanelCount(nCount)
{
    m_pcPanels=new StatusPanel[nPanelCount];
}

/********************************************************************
* Description: Statusbar destructor
* Author: Andreas Engh-Halstvedt(with modifications by Rick Caudill)
* Date: Thu Mar 18 20:07:11 2004
*********************************************************************/
StatusBar::~StatusBar()
{
    delete[] m_pcPanels;
}

/********************************************************************
* Description: Returns the preferred size of the StatusBar
* Author: Andreas Engh-Halstvedt(with modifications by Rick Caudill)
* Date: Thu Mar 18 20:07:11 2004
*********************************************************************/
os::Point StatusBar::GetPreferredSize(bool bLargest) const
{
    os::font_height sHeight;
    GetFont()->GetHeight(&sHeight);

    return os::Point(bLargest?10000.0f:20.0f, sHeight.ascender+sHeight.descender+sHeight.line_gap+10);
}

/********************************************************************
* Description: Paints the statusbar
* Author: Andreas Engh-Halstvedt(with modifications by Rick Caudill)
* Date: Thu Mar 18 20:07:11 2004
*********************************************************************/
void StatusBar::Paint(const os::Rect& cUpdateRect)
{
    os::Rect cBounds=GetBounds();
    float vPanel=3;
    os::font_height sHeight;
    GetFont()->GetHeight(&sHeight);


    SetFgColor(GetBgColor());
    FillRect(cUpdateRect);

    SetFgColor(255, 255, 255);
    DrawLine(os::Point(0, 0), os::Point(cBounds.right, 0));



    for(int a=0;a<nPanelCount;++a)
    {
        switch(m_pcPanels[a].mode)
        {
        case RELATIVE:
            {
                if(m_pcPanels[a].vWidth<0)
                    continue;

                float w=(cBounds.right-cBounds.left)*m_pcPanels[a].vWidth;
                DrawPanel(m_pcPanels[a].cText, vPanel, 3, vPanel+w, cBounds.bottom-3, sHeight.ascender+sHeight.line_gap);
                vPanel+=w;
                break;
            }
        case FILL://TODO: handle FILL other places than at the end
            {
                float w=cBounds.right-cBounds.left-3-vPanel;
                if(w<0)
                    continue;

                DrawPanel(m_pcPanels[a].cText, vPanel, 3, vPanel+w, cBounds.bottom-3, sHeight.ascender+sHeight.line_gap);
                vPanel+=w;
                break;
            }
        case CONSTANT:
        default:
            if(m_pcPanels[a].vWidth<0)
                continue;

            DrawPanel(m_pcPanels[a].cText, vPanel, 3, vPanel+m_pcPanels[a].vWidth, cBounds.bottom-3, sHeight.ascender+sHeight.line_gap);
            vPanel+=m_pcPanels[a].vWidth;
            break;
        }
        vPanel+=5;
    }
}

/********************************************************************
* Description: Draws a certain panel at a certain position
* Author: Andreas Engh-Halstvedt(with modifications by Rick Caudill)
* Date: Thu Mar 18 20:07:11 2004
*********************************************************************/
void StatusBar::DrawPanel(const String& cLabel, float vLeft, float vTop, float vRight, float vBottom, float vTable)
{
    SetFgColor(255, 255, 255);

    DrawLine(os::Point(vLeft, vTop), os::Point(vLeft, vBottom));
    DrawLine(os::Point(vLeft, vBottom), os::Point(vRight, vBottom));

    SetFgColor(0, 0, 0);
    DrawLine(os::Point(vLeft+1, vTop), os::Point(vRight, vTop));
    DrawLine(os::Point(vRight, vTop), os::Point(vRight, vBottom-1));

    DrawString(os::Point(vLeft+4,vTop+2+vTable),cLabel);
}

/********************************************************************
* Description: Sets the text of a certain panel
* Author: Andreas Engh-Halstvedt(with modifications by Rick Caudill)
* Date: Thu Mar 18 20:07:11 2004
*********************************************************************/
void StatusBar::SetText(const String& cLabel, int nPanel, bigtime_t nTimeout)
{
    if(nPanel<0 || nPanel>=nPanelCount)
        return;

    m_pcPanels[nPanel].cText=cLabel;
    if(nTimeout>0)
    {
        os::Looper* pcLooper=GetLooper();
        if(pcLooper)
        {
            pcLooper->RemoveTimer(this, nPanel);
            pcLooper->AddTimer(this, nPanel, nTimeout);
        }
    }
    Invalidate();
    Flush();
}

/********************************************************************
* Description: Configures a certain panel
* Author: Andreas Engh-Halstvedt(with modifications by Rick Caudill)
* Date: Thu Mar 18 20:07:11 2004
*********************************************************************/
void StatusBar::ConfigurePanel(int nPanel, panelmode mode, float vWidth)
{
    if(nPanel<0 || nPanel>=nPanelCount)
        return;

    if(mode!=RELATIVE && mode!=FILL)
        mode=CONSTANT;

    m_pcPanels[nPanel].mode=mode;
    m_pcPanels[nPanel].vWidth=vWidth;
    Invalidate();
}

/********************************************************************
* Description: TimerTick
* Author: Andreas Engh-Halstvedt(with modifications by Rick Caudill)
* Date: Thu Mar 18 20:07:11 2004
*********************************************************************/
void StatusBar::TimerTick(int i)
{
    if(i<0 || i>=nPanelCount)
        return;

    m_pcPanels[i].cText="";
    Invalidate();
    Flush();
}
