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

#include "meter.h"

Meter::Meter(Path cPluginFile, os::Looper* pcDock)
{
	//store dock data
	m_cPath = cPluginFile;
	m_pcDock = pcDock;

	//load in bitmaps
	File cFile(m_cPath);
	Resources cResources(&cFile);
	ResStream* pcResStream;

	pcResStream = cResources.GetResourceStream("cpu.png");
	m_cCPUImage.Load(pcResStream, "image/png");
	delete pcResStream;

	pcResStream = cResources.GetResourceStream("disk.png");
	m_cDiskImage.Load(pcResStream, "image/png");
	delete pcResStream;

	pcResStream = cResources.GetResourceStream("memory.png");
	m_cMemoryImage.Load(pcResStream, "image/png");
	delete pcResStream;

	//create background bitmaps
	m_cHorizontalBackground.ResizeCanvas(Point(72, 24));
	View* pcView = m_cHorizontalBackground.GetView();
	pcView->FillRect(pcView->GetBounds(), get_default_color(COL_NORMAL));
	pcView->SetDrawingMode(DM_BLEND);
	m_cCPUImage.Draw(Point(0, 24 - 16), pcView);
	m_cDiskImage.Draw(Point(24, 24 - 16), pcView);
	m_cMemoryImage.Draw(Point(48, 24 - 16), pcView);
	pcView->Sync();

	m_cVerticalBackground.ResizeCanvas(Point(24, 72));
	pcView = m_cVerticalBackground.GetView();
	pcView->FillRect(pcView->GetBounds(), get_default_color(COL_NORMAL));
	pcView->SetDrawingMode(DM_BLEND);
	m_cCPUImage.Draw(Point(0, 8), pcView);
	m_cDiskImage.Draw(Point(0, 32), pcView);
	m_cMemoryImage.Draw(Point(0, 56), pcView);
	pcView->Sync();

	//initialise meter data
	vMemory = 0;
	vCPU = 0;
	vDisk = 0;

	m_nOldRealtime = get_real_time();
	m_nOldIdletime = get_idle_time(0);
}

String Meter::GetIdentifier()
{
	return "Meter";
}

Point Meter::GetPreferredSize(bool bLargest)
{
	return Point(71, 71);
}

Path Meter::GetPath()
{
	return m_cPath;
}

void Meter::AttachedToWindow()
{
	m_pcDock->AddTimer(this, TIMER_UPDATE, 100000, false);
}

void Meter::DetachedFromWindow()
{
	m_pcDock->RemoveTimer(this, TIMER_UPDATE);
}

void Meter::FrameSized(const Point& cDelta)
{
	//detect orientation
	m_bHorizontal = (GetBounds().Height() == 23);

	//resize and redraw buffer
	m_cBuffer.ResizeCanvas(Point(GetBounds().Width() + 1, GetBounds().Height() + 1));
	View* pcView = m_cBuffer.GetView();
	
	if(m_bHorizontal)
		m_cHorizontalBackground.Draw(Point(0, 0), pcView);
	else
		m_cVerticalBackground.Draw(Point(0, 0), pcView);

	//make sure disk meter is updated
	m_nCounter = 0;

	pcView->Sync();
}

void Meter::MouseDown(const Point& cPosition, uint32 nButtons)
{
	//Display copyright notice
	Alert *pcAlert = new Alert("Copyright Notice",
		"Meter 1.0\n"
		"Copyright (C) 2004 William Hoggarth\n\n"
		"Meter comes with ABSOLUTELY NO WARRANTY.\n"
		"This is free software, and is distributed under the\n"
		"GNU General Public License.\n\n"
		"See http://www.gnu.org for details.",
		Alert::ALERT_INFO, 0, "Ok", NULL );
	pcAlert->Go(new Invoker());
}

void Meter::Paint(const Rect& cUpdateRect)
{
	//copy buffer
	m_cBuffer.Draw(cUpdateRect, cUpdateRect, this);
}

void Meter::TimerTick(int nID)
{
	//update info

	//cpu
	bigtime_t nNewRealtime = get_real_time();
	bigtime_t nNewIdletime = get_idle_time(0);
	float vActual = 1 - float(nNewIdletime - m_nOldIdletime) / float(nNewRealtime - m_nOldRealtime);
	vCPU = (vActual + 2 * vCPU) / 3; //smooth out chnages
	m_nOldRealtime = nNewRealtime;
	m_nOldIdletime = nNewIdletime;

	//disk
	if(m_nCounter == 0) //only check drive space occasionally
	{
		fs_info sFSInfo;
		int nFD = open(String(m_cPath).c_str(), O_RDONLY);
		get_fs_info(nFD, &sFSInfo);
		vDisk = float(sFSInfo.fi_total_blocks - sFSInfo.fi_free_blocks)  / sFSInfo.fi_total_blocks;
		close(nFD);

		m_nCounter = 600;
	}
	m_nCounter--;

	//memory
	system_info sSystemInfo;
	get_system_info(&sSystemInfo);
	vMemory = sSystemInfo.nFreePages * PAGE_SIZE;
	vMemory += sSystemInfo.nBlockCacheSize;
	vMemory = 1 - (vMemory / (sSystemInfo.nMaxPages * PAGE_SIZE));

	//update buffer

	//copy background bitmap
	View* pcView = m_cBuffer.GetView();
	
	if(m_bHorizontal)
		m_cHorizontalBackground.Draw(Point(0, 0), pcView);
	else
		m_cVerticalBackground.Draw(Point(0, 0), pcView);

	//draw meters
	float vFraction;
	for(int nY = 0; nY < 24; nY++)
	{
		vFraction = (float)nY / 23;

		if(vFraction < 0.5)
			pcView->SetFgColor(Color32_s(int(255 * (vFraction * 2)), 255, 0));	
		else
			pcView->SetFgColor(Color32_s(255, int(255 * (2 - vFraction * 2)), 0));	

		if(vFraction < vCPU && m_bHorizontal)
			pcView->DrawLine(Point(16, 23 - nY),Point(21, 23 - nY));

		if(vFraction < vCPU && !m_bHorizontal)
			pcView->DrawLine(Point(16, 23 - nY),Point(21, 23 - nY));

		if(vFraction < vDisk && m_bHorizontal)
			pcView->DrawLine(Point(40, 23 - nY),Point(45, 23 - nY));

		if(vFraction < vDisk && !m_bHorizontal)
			pcView->DrawLine(Point(16, 47 - nY),Point(21, 47 - nY));

		if(vFraction < vMemory && m_bHorizontal)
			pcView->DrawLine(Point(64, 23 - nY),Point(69, 23 - nY));

		if(vFraction < vMemory && !m_bHorizontal)
			pcView->DrawLine(Point(16, 71 - nY),Point(21, 71 - nY));
	}

	pcView->Sync();

	//update screen
	Invalidate();
	Flush();
}

extern "C"
{
	DockPlugin* init_dock_plugin(os::Path cPluginFile, os::Looper* pcDock)
	{
		return new Meter(cPluginFile, pcDock);
	}
}
