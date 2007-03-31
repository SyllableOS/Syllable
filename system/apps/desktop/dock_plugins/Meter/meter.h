//	Meter - A resource monitor for Syllable
//	Copyright (C) 2004 William Hoggarth
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#include <unistd.h>
#include <atheos/filesystem.h>
#include <atheos/kernel.h>
#include <atheos/time.h>
#include <gui/image.h>
#include <gui/requesters.h>
#include <storage/file.h>
#include <util/looper.h>
#include <util/resources.h>

#include <appserver/dockplugin.h>

using namespace os;

enum
{
	TIMER_UPDATE
};

class Meter : public os::View
{
	public:
		Meter(os::DockPlugin* pcPlugin, os::Looper* pcDock);
		~Meter();
	
		//dock plugin methods
		Point GetPreferredSize(bool bLargest) const;

		//view methods
		void AttachedToWindow();
		void DetachedFromWindow();
		void FrameSized(const Point &cDelta);
		void MouseDown(const Point& cPosition, uint32 nButtons);
		void Paint(const Rect& cUpdateRect);
		void TimerTick(int nID);
	private:
		//dock data
		os::DockPlugin* m_pcPlugin;
		Looper* m_pcDock;

		//dock alignment
		bool m_bHorizontal;

		//bitmaps
		BitmapImage* m_pcCPUImage;
		BitmapImage* m_pcDiskImage;
		BitmapImage* m_pcMemoryImage;

		BitmapImage* m_pcHorizontalBackground;
		BitmapImage* m_pcVerticalBackground;
		BitmapImage* m_pcBuffer;

		//meter data
		float vCPU;
		float vDisk;
		float vMemory;

		bigtime_t m_nOldRealtime;
		bigtime_t m_nOldIdletime;
		int m_nCounter;
};
