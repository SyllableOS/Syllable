/*
 * Albert 0.4 * Copyright (C)2002 Henrik Isaksson
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 *
 * AboutWindow class - window that shows a 'splash' image and version
 * information.
 *
 */

#include "aboutwin.h"
#include <gui/font.h>
#include <gui/desktop.h>

#include <stdio.h>

class BitmapView : public View {
	public:
	BitmapView(const Rect &cFrame, const char *cName, Image *img)
		: View(cFrame, cName, CF_FOLLOW_NONE, WID_WILL_DRAW)
	{
		m_pcImage = img;
		m_Font = new Font;
		m_Font->SetFamilyAndStyle(DEFAULT_FONT_REGULAR, "Regular");
		m_Font->SetSize(7);
	}

	~BitmapView()
	{
		m_Font->Release();
		delete m_pcImage;
	}

	void Paint(const Rect &cUpdateRect)
	{
		char *text1 = "Albert V0.5 2004-02-07 Copyright (C) 2002-2004 Henrik Isaksson.";
		char *text2 = "Albert is released under the GNU General Public License.";
		SetDrawingMode(DM_COPY);
		m_pcImage->Draw( Point( 0, 0 ), this  );

		SetDrawingMode(DM_OVER);
		SetFont(m_Font);
		
		font_height fh;
		GetFontHeight(&fh);
		
		MovePenTo(GetBounds().Width()/2 - GetStringWidth(text1)/2, 2 + fh.ascender);
		DrawString(text1);

		MovePenTo(GetBounds().Width()/2 - GetStringWidth(text2)/2, GetBounds().Height() - 2 - fh.descender);
		DrawString(text2);

	}

	private:
	Image		*m_pcImage;
	Font		*m_Font;
};

AboutWindow::AboutWindow(const Rect & r)
	:Window(r, "AboutAlbert", "About Albert",
//	WND_NO_CLOSE_BUT | WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT |
//	WND_NO_BORDER | WND_NO_TITLE |
	WND_NOT_RESIZABLE,
	CURRENT_DESKTOP)
{
    os::Resources cRes(get_image_id());
	os::ResStream* pcStream = cRes.GetResourceStream("albert.png");
	if(pcStream == NULL)
		throw(os::errno_exception("Can't find resource albert.png!"));

	m_pcImage = new BitmapImage( pcStream );
	BitmapView* imv = new BitmapView( m_pcImage->GetBounds(), "bitmap", m_pcImage );
	AddChild(imv);

	Rect	frame = m_pcImage->GetBounds();
	Desktop	desktop;
	Point offset(desktop.GetResolution());

	offset.x = offset.x/2 - frame.Width()/2;
	offset.y = offset.y/2 - frame.Height()/2;

	SetFrame(frame + offset);
	
	delete pcStream;
}

AboutWindow::~AboutWindow()
{
}

bool AboutWindow::OkToQuit()
{
	return true;
}

void AboutWindow::HandleMessage(Message *msg)
{
	Window::HandleMessage(msg);
}



