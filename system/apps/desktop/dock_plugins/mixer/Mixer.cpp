/*
    A Mixer plugin for the Syllable Dock

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
#include <gui/image.h>
#include <util/looper.h>
#include <util/resources.h>
#include <storage/file.h>
#include <media/manager.h>
#include <media/server.h>

#include "../../dock/dockplugin.h"

using namespace os;

class DockMixer : public DockPlugin
{
	public:
		DockMixer( os::Path cPath, os::Looper* pcDock );
		~DockMixer();

		os::String GetIdentifier() ;
		Point GetPreferredSize( bool bLargest );
		os::Path GetPath() { return( m_cPath ); }

        virtual void Paint( const Rect &cUpdateRect );

		virtual void MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData );
		virtual void MouseUp( const os::Point & cPosition, uint32 nButton, os::Message * pcData );
		virtual void MouseDown( const os::Point& cPosition, uint32 nButtons );
        
	private:
		os::Path m_cPath;
		os::BitmapImage* m_pcDockIcon;
		os::BitmapImage* m_pcDragIcon;
		os::Looper* m_pcDock;
		os::MediaManager * m_pcManager;

		bool m_bCanDrag;
		bool m_bDragging;
};

//*************************************************************************************

DockMixer::DockMixer( os::Path cPath, os::Looper* pcDock ) : DockPlugin()
{
	os::File* pcFile;
	os::ResStream *pcStream;

	m_pcDock = pcDock;
	m_bCanDrag = m_bDragging = false;
	m_cPath = cPath;

	/* Load default icons */
	pcFile = new os::File( cPath );
	os::Resources cDockCol( pcFile );
	pcStream = cDockCol.GetResourceStream( "icon24x24.png" );
	
	m_pcDockIcon = new os::BitmapImage( pcStream );
	delete( pcFile );

	pcFile = new os::File( cPath );
	os::Resources cDragCol( pcFile );
	pcStream = cDragCol.GetResourceStream( "icon48x48.png" );
	
	m_pcDragIcon = new os::BitmapImage( pcStream );
	delete( pcFile );

	// Create media manager 
	m_pcManager = new os::MediaManager();
}

DockMixer::~DockMixer( )
{
	delete( m_pcDockIcon );
	delete( m_pcDragIcon );
	delete( m_pcManager );
}

String DockMixer::GetIdentifier()
{
	return( "Mixer" );
}

void DockMixer::Paint( const Rect &cUpdateRect )
{
    FillRect( GetBounds(), get_default_color( COL_NORMAL ) );
	SetBgColor( get_default_color( COL_NORMAL ) );
	SetDrawingMode( DM_BLEND );
	m_pcDockIcon->Draw( Point(0,0), this);
}

Point DockMixer::GetPreferredSize( bool bLargest )
{
	return m_pcDockIcon->GetSize();
}

void DockMixer::MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData )
{
	if( nCode != MOUSE_ENTERED && nCode != MOUSE_EXITED )
	{
		/* Create dragging operation */
		if( m_bCanDrag )
		{
			m_bDragging = true;
			os::Message* pcMsg = new os::Message();
			BeginDrag( pcMsg, os::Point( m_pcDragIcon->GetBounds().Width() / 2,
											m_pcDragIcon->GetBounds().Height() / 2 ), m_pcDragIcon->LockBitmap() );
			m_bCanDrag = false;
		}
	}
	os::DockPlugin::MouseMove( cNewPos, nCode, nButtons, pcData );
}


void DockMixer::MouseUp( const os::Point & cPosition, uint32 nButtons, os::Message * pcData )
{
	if( m_bDragging && ( cPosition.y > 30 ) )
	{
		/* Remove ourself from the dock */
		os::Message cMsg( os::DOCK_REMOVE );
		cMsg.AddPointer( "plugin", this );
		m_pcDock->PostMessage( &cMsg, m_pcDock );
		return;
	}
	m_bDragging = false;
	m_bCanDrag = false;
	os::DockPlugin::MouseUp( cPosition, nButtons, pcData );
}

void DockMixer::MouseDown( const os::Point& cPosition, uint32 nButtons )
{
	MakeFocus( true );
	if( nButtons == os::MOUSE_BUT_LEFT )
		m_bCanDrag = true;

	/* Display the mixer window */
	m_pcManager->GetInstance()->GetServerLink().SendMessage( os::MEDIA_SERVER_SHOW_CONTROLS );

	os::DockPlugin::MouseDown( cPosition, nButtons );
}
//*************************************************************************************

extern "C"
{
DockPlugin* init_dock_plugin( os::Path cPluginFile, os::Looper* pcDock )
{
	return( new DockMixer( cPluginFile, pcDock ) );
}
}

