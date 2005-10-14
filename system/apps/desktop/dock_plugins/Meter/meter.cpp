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

Meter::Meter(DockPlugin* pcPlugin, os::Looper* pcDock) : os::View( os::Rect(), "meter" )
{
	//store dock data
	m_pcPlugin = pcPlugin;
	m_pcDock = pcDock;

	//load in bitmaps

	File cFile(m_pcPlugin->GetPath());
	Resources cResources(&cFile);
	ResStream* pcResStream;

	pcResStream = cResources.GetResourceStream("cpu.png");
	m_pcCPUImage = new os::BitmapImage(pcResStream );
	delete pcResStream;

	pcResStream = cResources.GetResourceStream("disk.png");
	m_pcDiskImage = new os::BitmapImage(pcResStream );
	delete pcResStream;

	pcResStream = cResources.GetResourceStream("memory.png");
	m_pcMemoryImage = new os::BitmapImage(pcResStream );
	delete pcResStream;
	
	//create background bitmaps
	m_pcHorizontalBackground = new os::BitmapImage( Bitmap::SHARE_FRAMEBUFFER | Bitmap::ACCEPT_VIEWS );
	m_pcHorizontalBackground->ResizeCanvas(Point(72, 24));

	View* pcView = m_pcHorizontalBackground->GetView();
	pcView->FillRect(pcView->GetBounds(), get_default_color(COL_NORMAL));
	pcView->SetDrawingMode(DM_BLEND);
	m_pcCPUImage->Draw(Point(0, 24 - 16), pcView);
	m_pcDiskImage->Draw(Point(24, 24 - 16), pcView);
	m_pcMemoryImage->Draw(Point(48, 24 - 16), pcView);
	pcView->Sync();

	m_pcVerticalBackground = new os::BitmapImage( Bitmap::SHARE_FRAMEBUFFER | Bitmap::ACCEPT_VIEWS );
	m_pcVerticalBackground->ResizeCanvas(Point(24, 72));
	pcView = m_pcVerticalBackground->GetView();
	pcView->FillRect(pcView->GetBounds(), get_default_color(COL_NORMAL));
	pcView->SetDrawingMode(DM_BLEND);
	m_pcCPUImage->Draw(Point(0, 8), pcView);
	m_pcDiskImage->Draw(Point(0, 32), pcView);
	m_pcMemoryImage->Draw(Point(0, 56), pcView);
	pcView->Sync();
	
	m_pcBuffer = new os::BitmapImage( Bitmap::SHARE_FRAMEBUFFER | Bitmap::ACCEPT_VIEWS );

	//initialise meter data
	vMemory = 0;
	vCPU = 0;
	vDisk = 0;

	m_nOldRealtime = get_real_time();
	m_nOldIdletime = get_idle_time(0);
	
	delete( m_pcCPUImage );
	delete( m_pcDiskImage );
	delete( m_pcMemoryImage );

}

Meter::~Meter()
{
	delete( m_pcHorizontalBackground );
	delete( m_pcVerticalBackground );
}

Point Meter::GetPreferredSize(bool bLargest) const
{
	return Point(71, 71);
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
	m_pcBuffer->ResizeCanvas(Point(GetBounds().Width() + 1, GetBounds().Height() + 1));
	View* pcView = m_pcBuffer->GetView();
	
	if(m_bHorizontal)
		m_pcHorizontalBackground->Draw(Point(0, 0), pcView);
	else
		m_pcVerticalBackground->Draw(Point(0, 0), pcView);

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
	m_pcBuffer->Draw(cUpdateRect, cUpdateRect, this);
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
		int nFD = open(String(m_pcPlugin->GetPath()).c_str(), O_RDONLY);
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
	View* pcView = m_pcBuffer->GetView();
	
	if(m_bHorizontal)
		m_pcHorizontalBackground->Draw(Point(0, 0), pcView);
	else
		m_pcVerticalBackground->Draw(Point(0, 0), pcView);

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

//*************************************************************************************

class MeterPlugin : public os::DockPlugin
{
public:
	MeterPlugin()
	{
		m_pcView = NULL;
	}
	~MeterPlugin()
	{
	}
	status_t Initialize()
	{
		m_pcView = new Meter( this, GetApp() );
		AddView( m_pcView );
		return( 0 );
	}
	void Delete()
	{
		RemoveView( m_pcView );
	}
	os::String GetIdentifier()
	{
		return( "Meter" );
	}
private:
	Meter* m_pcView;
};

extern "C"
{
DockPlugin* init_dock_plugin()
{
	return( new MeterPlugin() );
}
}





























