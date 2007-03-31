/*
    A Printer plugin for the Syllable Dock

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

#include <atheos/time.h>
#include <gui/menu.h>
#include <util/looper.h>
#include <util/resources.h>
#include <appserver/dockplugin.h>

using namespace os;

enum
{
	M_PREF,
	M_QUEUE,
};


class DockPrinter : public View
{
	public:
		DockPrinter( os::DockPlugin* pcPlugin, os::Looper* pcDock );
		~DockPrinter();
		Point GetPreferredSize( bool bLargest ) const;
		virtual void Paint( const Rect &cUpdateRect );
		virtual void AttachedToWindow();
		virtual void MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData );
		virtual void MouseUp( const os::Point & cPosition, uint32 nButton, os::Message * pcData );
		virtual void MouseDown( const os::Point& cPosition, uint32 nButtons );
		virtual void HandleMessage(Message* pcMessage);    
	private:
		os::DockPlugin* m_pcPlugin;
		os::BitmapImage* m_pcDockIcon;
		os::BitmapImage* m_pcDragIcon;
		os::Looper* m_pcDock;
		bool m_bWindowShown;
		bool m_bCanDrag;
		bool m_bDragging;
		Menu* pcContextMenu;
		bigtime_t m_nHitTime;
};


DockPrinter::DockPrinter( DockPlugin* pcPlugin, os::Looper* pcDock ) : View( os::Rect(), "dock_printer" )
{
	os::File* pcFile;
	os::ResStream *pcStream;

	m_pcPlugin = pcPlugin;
	m_pcDock = pcDock;
	m_bCanDrag = m_bDragging = false;
	m_nHitTime = 0;

	pcContextMenu = new Menu(Rect(0,0,10,10),"",ITEMS_IN_COLUMN);
	pcContextMenu->AddItem("Preferences...", new Message(M_PREF));
	pcContextMenu->AddItem("Print Queue...", new Message(M_QUEUE));

	/* Load default icons */
	pcFile = new os::File( m_pcPlugin->GetPath() );
	os::Resources cDockCol( pcFile );
	pcStream = cDockCol.GetResourceStream( "icon24x24.png" );

	m_pcDockIcon = new os::BitmapImage( pcStream );
	delete( pcStream );
	delete( pcFile );

	pcFile = new os::File( m_pcPlugin->GetPath() );
	os::Resources cDragCol( pcFile );
	pcStream = cDragCol.GetResourceStream( "icon48x48.png" );

	m_pcDragIcon = new os::BitmapImage( pcStream );
	delete( pcStream );
	delete( pcFile );
}


void DockPrinter::AttachedToWindow()
{
	pcContextMenu->SetTargetForItems(this);
}


DockPrinter::~DockPrinter( )
{
	delete( pcContextMenu );
	delete( m_pcDockIcon );
	delete( m_pcDragIcon );
}


void DockPrinter::Paint( const Rect &cUpdateRect )
{
	FillRect( GetBounds(), get_default_color( COL_NORMAL ) );
	SetBgColor( get_default_color( COL_NORMAL ) );
	SetDrawingMode( DM_BLEND );
	m_pcDockIcon->Draw( Point(0,0), this);
}


Point DockPrinter::GetPreferredSize( bool bLargest ) const
{
	return m_pcDockIcon->GetSize();
}


void DockPrinter::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_PREF:
		{
			if( fork() == 0 )
			{
				system("/Applications/Preferences/Printers");
				exit(0);
			}
			break;
		}

		case M_QUEUE:
		{
			if( fork() == 0 )
			{
				system("/system/bin/print-queue");
				exit(0);
			}
			break;
		}
	}
}


void DockPrinter::MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData )
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


void DockPrinter::MouseUp( const os::Point & cPosition, uint32 nButtons, os::Message * pcData )
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


void DockPrinter::MouseDown( const os::Point& cPosition, uint32 nButtons )
{
	MakeFocus( true );
	if( nButtons == os::MOUSE_BUT_LEFT )
	{
		if ( m_nHitTime + 500000 >= get_system_time() )
		{
			if( fork() == 0 )
			{
				system("/system/bin/print-queue");
				exit(0);
			}
		}
		else
			m_bCanDrag = true;
		m_nHitTime = get_system_time();
	}

	if( nButtons == 2 ) {
		pcContextMenu->Open(ConvertToScreen(cPosition));
	}
	os::View::MouseDown( cPosition, nButtons );
}


class DockPrinterPlugin : public os::DockPlugin
{
public:
	DockPrinterPlugin()
	{
		m_pcView = NULL;
	}
	~DockPrinterPlugin()
	{
	}
	status_t Initialize()
	{
		m_pcView = new DockPrinter( this, GetApp() );
		AddView( m_pcView );
		return( 0 );
	}
	void Delete()
	{
		RemoveView( m_pcView );
	}
	os::String GetIdentifier()
	{
		return( "Printer" );
	}
private:
	DockPrinter* m_pcView;
};

extern "C"
{
DockPlugin* init_dock_plugin()
{
	return( new DockPrinterPlugin() );
}
}

