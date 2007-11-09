/*
 *  FileExpander 0.7 (GUI files extraction tool)
 *  Copyright (c) 2004 Ruslan Nickolaev (nruslan@hotbox.ru)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#include <gui/image.h>
#include <util/resources.h>
#include "etextview.h"

enum g_eTextType {
    ETEXT_READONLY = 01,
    ETEXT_PASSWORD = 02
};

#define MENUITEMS_COUNT 6

static struct g_sMenuItem {
    char *m_pzName;
    char *m_pzShortCut;
    char *m_pzImagePath;
    char m_nTextType;
} g_asMenuItem[] = {
  { "Cut", "Ctrl+X", "cut16x16.png", 0x00 },
  { "Copy", "Ctrl+C", "copy16x16.png", ETEXT_READONLY },
  { "Paste", "Ctrl+V", "paste16x16.png", ETEXT_PASSWORD },
  { "Clear", "", "clear16x16.png", ETEXT_PASSWORD },
  { NULL, NULL, NULL, ETEXT_READONLY | ETEXT_PASSWORD },
  { "Select all", "", "select16x16.png", ETEXT_READONLY | ETEXT_PASSWORD },
};

EtextView::EtextView(const os::Rect &cFrame, const os::String &cTitle, const char *pzBuffer, uint32 nResizeMask, uint32 nFlags)
  : os::TextView(cFrame, cTitle, pzBuffer, nResizeMask, nFlags)
{

    struct g_sMenuItem *psMenuItem = g_asMenuItem, *psMenuItemEnd = g_asMenuItem + MENUITEMS_COUNT;
    os::Resources pcFEResources(get_image_id());
    os::ResStream *pcResStream;
    os::BitmapImage *pcBitmapImage;
    int i = 0;

    m_pcMenu = new os::Menu(os::Rect(0, 0, 1, 1), "normal_menu", os::ITEMS_IN_COLUMN, os::CF_FOLLOW_NONE);
    m_pcReadOnlyMenu = new os::Menu(os::Rect(0, 0, 1, 1), "readonly_menu", os::ITEMS_IN_COLUMN, os::CF_FOLLOW_NONE);
    m_pcPasswordMenu = new os::Menu(os::Rect(0, 0, 1, 1), "password_menu", os::ITEMS_IN_COLUMN, os::CF_FOLLOW_NONE);

    while (psMenuItem < psMenuItemEnd)
    {
        if (psMenuItem->m_pzName)
        {
            pcResStream = pcFEResources.GetResourceStream(psMenuItem->m_pzImagePath);
            pcBitmapImage = new os::BitmapImage(pcResStream);
            delete pcResStream;
            if ((psMenuItem->m_nTextType) & ETEXT_READONLY)
                m_pcReadOnlyMenu->AddItem(psMenuItem->m_pzName, new os::Message(i), psMenuItem->m_pzShortCut, new os::BitmapImage(*pcBitmapImage));
            if ((psMenuItem->m_nTextType) & ETEXT_PASSWORD)
                m_pcPasswordMenu->AddItem(psMenuItem->m_pzName, new os::Message(i), psMenuItem->m_pzShortCut, new os::BitmapImage(*pcBitmapImage));
            m_pcMenu->AddItem(psMenuItem->m_pzName, new os::Message(i), psMenuItem->m_pzShortCut, pcBitmapImage);
            i++;
        }

        else
        {
            if ((psMenuItem->m_nTextType) & ETEXT_READONLY)
                m_pcReadOnlyMenu->AddItem(new os::MenuSeparator);
            if ((psMenuItem->m_nTextType) & ETEXT_PASSWORD)
                m_pcPasswordMenu->AddItem(new os::MenuSeparator);
            m_pcMenu->AddItem(new os::MenuSeparator);
        }

        psMenuItem++;
    }
}

void EtextView::HandleMessage(os::Message *pcMessage)
{
    switch (pcMessage->GetCode())
    {
        case M_ETEXT_CUT:
            Cut();
            break;

        case M_ETEXT_COPY:
            Copy();
            break;

        case M_ETEXT_PASTE:
            Paste();
            break;

        case M_ETEXT_CLEAR:
            Delete();
            break;

        case M_ETEXT_SELECTALL:
            SelectAll();
            break;

        default:
            os::TextView::HandleMessage(pcMessage);
            break;
    }
}

void EtextView::MouseDown(const os::Point &cPosition, uint32 nButtons)
{
    MakeFocus();
    switch (nButtons)
    {
        case 2:
        {
            bool nPasswMode = GetPasswordMode(), nReadOnly = GetReadOnly();
            os::Menu *pcMenu;

            if (IsEnabled() && !(nPasswMode && nReadOnly))
            {
                if (nReadOnly)
                    pcMenu = m_pcReadOnlyMenu;
                else if (nPasswMode)
                    pcMenu = m_pcPasswordMenu;
                else // normal menu
                    pcMenu = m_pcMenu;

                pcMenu->Open(ConvertToScreen(cPosition));
                pcMenu->SetTargetForItems(this);
            }

            break;
        }

        default:
            os::TextView::MouseDown(cPosition, nButtons);
            break;
    }
}

EtextView::~EtextView()
{
}
