/*
    A ACPI Battery plugin for the Syllable Dock

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
                                                                                                                                                       
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <atheos/acpi.h>
#include <gui/image.h>
#include <gui/window.h>
#include <gui/slider.h>
#include <util/looper.h>
#include <util/resources.h>
#include <util/thread.h>
#include <storage/file.h>

#include <appserver/dockplugin.h>

using namespace os;

class DockBattery : public View
{
	public:
		DockBattery( os::DockPlugin* pcPlugin, os::Looper* pcDock );
		~DockBattery();

		Point GetPreferredSize( bool bLargest ) const;
				
		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();

		void Paint( const Rect &cUpdateRect );
		void TimerTick( int nID );

		virtual void MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData );
		virtual void MouseUp( const os::Point & cPosition, uint32 nButton, os::Message * pcData );
		virtual void MouseDown( const os::Point& cPosition, uint32 nButtons );
	private:
		os::DockPlugin* m_pcPlugin;
		os::BitmapImage* m_pcDragIcon;
		os::Looper* m_pcDock;
		bool m_bCanDrag;
		bool m_bDragging;
		int  m_nFd;
		int	 m_nPercentage;
		bool m_bChargingOrFull;
};

//*************************************************************************************

DockBattery::DockBattery( DockPlugin* pcPlugin, os::Looper* pcDock ) : View( os::Rect(), "dock_battery" )
{
	os::File* pcFile;
	os::ResStream *pcStream;

	m_pcPlugin = pcPlugin;
	m_pcDock = pcDock;
	m_bCanDrag = m_bDragging = false;
	m_nFd = -1;
	m_nPercentage = 0;
	m_bChargingOrFull = false;

	/* Load drag icon */
	pcFile = new os::File( m_pcPlugin->GetPath() );
	os::Resources cDragCol( pcFile );
	pcStream = cDragCol.GetResourceStream( "icon48x48.png" );
	
	m_pcDragIcon = new os::BitmapImage( pcStream );
	delete( pcStream );
	delete( pcFile );
}

DockBattery::~DockBattery( )
{
	delete( m_pcDragIcon );
}

void DockBattery::Paint( const Rect &cUpdateRect )
{
    FillRect( cUpdateRect, get_default_color( COL_NORMAL ) );
 
  	/* Draw battery border */
	os::Color32_s sBorderColor( get_default_color( COL_SHADOW ) );
    
	if( m_nFd >= 0 )
	{
		if( m_bChargingOrFull )
			sBorderColor = os::Color32_s( get_default_color( COL_SHINE ) );
		else
			sBorderColor = os::Color32_s( 0, 0, 0 );
	}
    
	SetFgColor( sBorderColor );
    
	MovePenTo( os::Point( 2, 5 ) );
	DrawLine( os::Point( GetBounds().Width() - 4, 5 ) );
	DrawLine( os::Point( GetBounds().Width() - 4, 8 ) );
	DrawLine( os::Point( GetBounds().Width() - 2, 8 ) );
	DrawLine( os::Point( GetBounds().Width() - 2, 15 ) );
	DrawLine( os::Point( GetBounds().Width() - 4, 15 ) );
	DrawLine( os::Point( GetBounds().Width() - 4, 18 ) );
	DrawLine( os::Point( 2, 18 ) );
	DrawLine( os::Point( 2, 5 ) );
    
    /* Fill battery */

	if( m_nFd >= 0 )
    {
    	if( m_nPercentage > 60 )
    		SetFgColor( 138, 215, 174  );
    	else if( m_nPercentage > 20 )
    		SetFgColor( 231, 239, 170 );
    	else
    		SetFgColor( 207, 130, 146 );
    		
    	FillRect( os::Rect( 3, 6, 3 + ( GetBounds().Width() - 8 ) * 
    				( ( m_nPercentage > 95 ? 95 : m_nPercentage ) + 5 ) / 100, 17 ) );
    	if( m_nPercentage > 95 )
    		FillRect( os::Rect( GetBounds().Width() - 4, 9, GetBounds().Width() - 3, 14 ) );
    } 
    else 
	{
   		SetFgColor( get_default_color( COL_SHADOW ) );
   		FillRect( os::Rect( 3, 6, GetBounds().Width() - 5, 17 ) );
   		FillRect( os::Rect( GetBounds().Width() - 4, 9, GetBounds().Width() - 3, 14 ) );
    }
}


void DockBattery::TimerTick( int nID )
{
	/* Read new state */
	if( m_nFd >= 0 )
	{
		BatteryStatus_s sStatus;
		sStatus.bs_nDataSize = sizeof( sStatus );
		if( ioctl( m_nFd, 0, &sStatus ) < 0 )
		{
			/* Close device */
			close( m_nFd );
			m_nFd = -1;
		} else {
			m_nPercentage = sStatus.bs_nPercentage;
			m_bChargingOrFull = sStatus.bs_bChargingOrFull;
		}
	}
	
	Invalidate();
	Flush();
}


void DockBattery::AttachedToWindow()
{
	/* Try to open the device */
	m_nFd = open( "/dev/misc/battery", O_RDWR );
	BatteryStatus_s sStatus;
	sStatus.bs_nDataSize = sizeof( sStatus );
	if( ioctl( m_nFd, 0, &sStatus ) < 0 )
	{
		/* Close device */
		close( m_nFd );
		m_nFd = -1;
	} else {
		m_nPercentage = sStatus.bs_nPercentage;
		m_bChargingOrFull = sStatus.bs_bChargingOrFull;
	}
	m_pcDock->AddTimer(this, 0, 10000000, false);
}


void DockBattery::DetachedFromWindow()
{
	m_pcDock->RemoveTimer(this, 0);
	if( m_nFd >= 0 )
		close( m_nFd );
}

Point DockBattery::GetPreferredSize( bool bLargest ) const
{
	return( os::Point( 40, 22 ) );
}

void DockBattery::MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData )
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


void DockBattery::MouseUp( const os::Point & cPosition, uint32 nButtons, os::Message * pcData )
{
	if( m_bDragging && ( cPosition.y > 30 ) )
	{
		/* Remove ourself from the dock */
		os::Message cMsg( os::DOCK_REMOVE_PLUGIN );
		cMsg.AddPointer( "plugin", m_pcPlugin );
		m_pcDock->PostMessage( &cMsg, m_pcDock );
		return;
	}
	m_bDragging = false;
	m_bCanDrag = false;
	os::View::MouseUp( cPosition, nButtons, pcData );
}

void DockBattery::MouseDown( const os::Point& cPosition, uint32 nButtons )
{
	MakeFocus( true );
	if( nButtons == os::MOUSE_BUT_LEFT )
		m_bCanDrag = true;

	os::View::MouseDown( cPosition, nButtons );
}

//*************************************************************************************

class DockBatteryPlugin : public os::DockPlugin
{
public:
	DockBatteryPlugin()
	{
		m_pcView = NULL;
	}
	~DockBatteryPlugin()
	{
	}
	status_t Initialize()
	{
		m_pcView = new DockBattery( this, GetApp() );
		AddView( m_pcView );
		return( 0 );
	}
	void Delete()
	{
		RemoveView( m_pcView );
	}
	os::String GetIdentifier()
	{
		return( "Battery" );
	}
private:
	DockBattery* m_pcView;
};

extern "C"
{
DockPlugin* init_dock_plugin()
{
	return( new DockBatteryPlugin() );
}
}























