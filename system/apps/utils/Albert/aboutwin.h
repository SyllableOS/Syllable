/*
 * Albert 0.5 * Copyright (C)2002-2004 Henrik Isaksson
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

#ifndef ABOUTWIN_H
#define ABOUTWIN_H

#include <gui/window.h>
#include <gui/image.h>
#include <util/message.h>
#include <util/settings.h>
#include <atheos/time.h>
#include <gui/bitmap.h>
#include <gui/guidefines.h>
#include <util/message.h>
#include <util/string.h>
#include <util/locker.h>
#include <util/resources.h>
#include <storage/file.h>

using namespace os;

class AboutWindow: public Window {
	public:
	AboutWindow(const Rect & r);
	~AboutWindow();
	bool OkToQuit();

	Bitmap* LoadImage(StreamableIO* pcStream);

	void HandleMessage(Message *);

	private:
	Image		*m_pcImage;
};

#endif
