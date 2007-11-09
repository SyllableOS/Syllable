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

#ifndef _NRUSLAN_ETEXTVIEW_H_
#define _NRUSLAN_ETEXTVIEW_H_

#include <gui/textview.h>
#include <gui/menu.h>

class EtextView : public os::TextView
{
    public:
        EtextView(const os::Rect &cFrame, const os::String &cTitle, const char *pzBuffer,
	          uint32 nResizeMask = os::CF_FOLLOW_LEFT | os::CF_FOLLOW_TOP,
	          uint32 nFlags = os::WID_WILL_DRAW | os::WID_FULL_UPDATE_ON_RESIZE);
        virtual void MouseDown(const os::Point &cPosition, uint32 nButtons);
        virtual void HandleMessage(os::Message *pcMessage);
        virtual ~EtextView();
    private:
        os::Menu *m_pcMenu, *m_pcReadOnlyMenu, *m_pcPasswordMenu;

        enum m_eTextManip {
            M_ETEXT_CUT,
            M_ETEXT_COPY,
            M_ETEXT_PASTE,
            M_ETEXT_CLEAR,
            M_ETEXT_SELECTALL
        };
};

#endif /* _NRUSLAN_ETEXTVIEW_H_ */
