/*  ColdFish Music Player
 *  Copyright (C) 2003 Kristian Van Der Vliet
 *  Copyright (C) 2003 Arno Klenke
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

#ifndef _SELECTWIN_H_
#define _SELECTWIN_H_

#include <gui/view.h>
#include <gui/window.h>
#include <gui/button.h>
#include <gui/textview.h>
#include <gui/filerequester.h>
#include <gui/stringview.h>
#include <util/message.h>
#include <util/application.h>

class SelectWin : public os::Window
{
public:
	SelectWin( os::Rect cFrame );
	~SelectWin();
	virtual void HandleMessage( os::Message* pcMessage );
private:
	os::TextView* m_pcFileInput;
	os::StringView* m_pcFileLabel;
	os::Button* m_pcFileButton;
	os::Button* m_pcOpenButton;
	os::StringView* m_pcInfo;
	os::FileRequester* m_pcFileDialog;
};
#endif
