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

#define DRAG_THRESHOLD 4

static os::Color32_s BlendColours( const os::Color32_s& sColour1, const os::Color32_s& sColour2, float vBlend )
{
	int r = int( (float(sColour1.red)   * vBlend + float(sColour2.red)   * (1.0f - vBlend)) );
 	int g = int( (float(sColour1.green) * vBlend + float(sColour2.green) * (1.0f - vBlend)) );
 	int b = int( (float(sColour1.blue)  * vBlend + float(sColour2.blue)  * (1.0f - vBlend)) );
 	if ( r < 0 ) r = 0; else if (r > 255) r = 255;
 	if ( g < 0 ) g = 0; else if (g > 255) g = 255;
 	if ( b < 0 ) b = 0; else if (b > 255) b = 255;
 	return os::Color32_s(r, g, b, sColour1.alpha);
}

/* From the Photon Decorator */
static os::Color32_s Tint( const os::Color32_s & sColor, float vTint )
{
	int r = int ( ( float ( sColor.red ) * vTint + 127.0f * ( 1.0f - vTint ) ) );
	int g = int ( ( float ( sColor.green ) * vTint + 127.0f * ( 1.0f - vTint ) ) );
	int b = int ( ( float ( sColor.blue ) * vTint + 127.0f * ( 1.0f - vTint ) ) );
 
 	if( r < 0 )
 		r = 0;
 	else if( r > 255 )
 		r = 255;
 	if( g < 0 )
 		g = 0;
 	else if( g > 255 )
 		g = 255;
 	if( b < 0 )
 		b = 0;
 	else if( b > 255 )
 		b = 255;
 	return ( os::Color32_s( r, g, b, sColor.alpha ) );
}

Meter::Meter(DockPlugin* pcPlugin, os::Looper* pcDock) : os::View( os::Rect(), "meter" )
{
	os::File* pcFile;
	os::ResStream *pcResStream;

	//store dock data
	m_pcPlugin = pcPlugin;
	m_pcDock = pcDock;
	m_bCanDrag = m_bDragging = false;

	//load in bitmaps
	pcFile = new os::File( m_pcPlugin->GetPath() );
	os::Resources cResources( pcFile );
	pcResStream = cResources.GetResourceStream( "icon48x48.png" );
	m_pcDragIcon = new os::BitmapImage( pcResStream );
	delete( pcResStream );

	pcResStream = cResources.GetResourceStream("cpu.png");
	m_pcCPUImage = new os::BitmapImage(pcResStream );
	delete( pcResStream );

	pcResStream = cResources.GetResourceStream("disk.png");
	m_pcDiskImage = new os::BitmapImage(pcResStream );
	delete( pcResStream );

	pcResStream = cResources.GetResourceStream("memory.png");
	m_pcMemoryImage = new os::BitmapImage(pcResStream );
	delete( pcResStream );
	delete( pcFile );
	
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

void Meter::MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData )
{
	if( nCode != MOUSE_ENTERED && nCode != MOUSE_EXITED )
	{
		/* Create dragging operation */
		if( m_bCanDrag )
		{
			m_bDragging = true;
			os::Message cMsg;
			BeginDrag( &cMsg, os::Point( m_pcDragIcon->GetBounds().Width() / 2,
											m_pcDragIcon->GetBounds().Height() / 2 ), m_pcDragIcon->LockBitmap() );
			m_bCanDrag = false;
		}
	}
	os::View::MouseMove( cNewPos, nCode, nButtons, pcData );
}

void Meter::MouseUp( const os::Point & cPosition, uint32 nButtons, os::Message * pcData )
{
	// Get the frame of the dock
	// If the plugin is dragged outside of the dock;s frame
	// then remove the plugin
	Rect cRect = ConvertFromScreen( GetWindow()->GetFrame() );

	if( ( m_bDragging && ( cPosition.x < cRect.left ) ) || ( m_bDragging && ( cPosition.x > cRect.right ) ) || ( m_bDragging && ( cPosition.y < cRect.top ) ) || ( m_bDragging && ( cPosition.y > cRect.bottom ) ) ) 
	{
		/* Remove ourself from the dock */
		os::Message cMsg( os::DOCK_REMOVE_PLUGIN );
		cMsg.AddPointer( "plugin", m_pcPlugin );
		m_pcDock->PostMessage( &cMsg, m_pcDock );
		return;
	} /*else if ( nButtons == 1 ) { // left button
		// Check to see if the coordinates passed match when the left mouse button was pressed
		// if so, then it was a single click and not a drag
		if ( abs( (int)(m_cPos.x - cPosition.x) ) < DRAG_THRESHOLD && abs( (int)(m_cPos.y - cPosition.y) ) < DRAG_THRESHOLD )
		{ 
			// Just eat it at this time.
		}
	} */

	m_bDragging = false;
	m_bCanDrag = false;
	os::View::MouseUp( cPosition, nButtons, pcData );
}

void Meter::MouseDown(const Point& cPosition, uint32 nButtons)
{
	if ( nButtons == 1 ) /* left button */
	{
		MakeFocus ( true );
		m_bCanDrag = true;

		// Store these coordinates for later use in the MouseUp procedure
		m_cPos.x = cPosition.x;
		m_cPos.y = cPosition.y;
	} else if ( nButtons == 2 ) /* right button */
	{
		MakeFocus ( false );
	//Display copyright notice
	Alert *pcAlert = new Alert("Copyright Notice",
		"Meter 1.0\n"
		"Copyright (C) 2004 William Hoggarth\n\n"
		"Meter comes with ABSOLUTELY NO WARRANTY.\n"
		"This is free software, and is distributed under the\n"
		"GNU General Public License.\n\n"
		"See http://www.gnu.org for details.",
		m_pcDragIcon->LockBitmap(), 0, "Close", NULL );
		m_pcDragIcon->UnlockBitmap();
	pcAlert->Go(new Invoker());
}
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





























