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
#include <gui/window.h>
#include <gui/slider.h>
#include <util/looper.h>
#include <util/resources.h>
#include <util/thread.h>
#include <storage/file.h>
#include <media/manager.h>
#include <media/server.h>

#include <appserver/dockplugin.h>

using namespace os;

class MixerWindow;

class DockMixer : public View
{
	public:
		DockMixer( os::DockPlugin* pcPlugin, os::Looper* pcDock );
		~DockMixer();

		Point GetPreferredSize( bool bLargest ) const;
				
		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();

        virtual void Paint( const Rect &cUpdateRect );

		virtual void MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData );
		virtual void MouseUp( const os::Point & cPosition, uint32 nButton, os::Message * pcData );
		virtual void MouseDown( const os::Point& cPosition, uint32 nButtons );
        void MixerWindowHidden() { m_bWindowShown = false; }
        MixerWindow* GetMixerWindow() { return( m_pcWindow ); }
        void Remove( thread_id nThread );
	private:
		os::DockPlugin* m_pcPlugin;
		os::BitmapImage* m_pcDockIcon;
		os::BitmapImage* m_pcDragIcon;
		os::Looper* m_pcDock;
		os::MediaManager * m_pcManager;
		MixerWindow* m_pcWindow;
		bool m_bWindowShown;
		bool m_bCanDrag;
		bool m_bDragging;
};

enum { SET_VALUE };

class MixerView : public View
{
	public:
		MixerView( Rect cFrame ) : View( cFrame, "view" )
		{
		}
		void	WindowActivated( bool bIsActive )
		{
			if( !bIsActive )
				GetWindow()->PostMessage( M_QUIT, GetWindow() );
		}
		void Paint( const Rect& cUpdateRect )
		{
			DrawFrame( GetBounds(), FRAME_RECESSED | FRAME_THIN );
		}
};

class MixerWindow : public Window
{
	public:
		MixerWindow( MediaManager* pcManager, DockMixer* pcPlugin, const Rect& cFrame ) : Window( cFrame, "mixer", "mixer", os::WND_NO_TITLE | os::WND_NOT_RESIZABLE | os::WND_NO_BORDER )
		{
			m_pcPlugin = pcPlugin;
			m_pcManager = pcManager;
			m_pcView = new MixerView( GetBounds() );
			m_pcSlider = new Slider( GetBounds().Resize( 3, 3, -3, -3 ), "slider", new os::Message( SET_VALUE ), Slider::TICKS_RIGHT, 10, Slider::KNOB_SQUARE, VERTICAL );
			m_pcSlider->SetMinMax( 0, 100 );
			m_pcSlider->SetTarget( this );
			m_pcView->AddChild( m_pcSlider );
			AddChild( m_pcView );
			m_pcView->MakeFocus();
		}
		~MixerWindow()
		{
		}
		void HandleMessage( Message* pcMessage )
		{
			switch( pcMessage->GetCode() )
			{
				case SET_VALUE:
				{
					int32 nValue = m_pcSlider->GetValue().AsInt32();
					nValue = nValue;
					Message cMsg( MEDIA_SERVER_SET_MASTER_VOLUME );
					cMsg.AddInt32( "volume", nValue );
					m_pcManager->GetInstance()->GetServerLink().SendMessage( &cMsg );
				}
				break;
				default:
					Window::HandleMessage( pcMessage );
			}
		}
		bool OkToQuit()
		{
			Hide();
			m_pcPlugin->MixerWindowHidden();
			return( false );
		}
		void Update()
		{
			int32 nVolume;
			Message cMsg( MEDIA_SERVER_GET_MASTER_VOLUME );
			Message cReply;
			m_pcManager->GetInstance()->GetServerLink().SendMessage( &cMsg, &cReply );
			if( cReply.FindInt32( "volume", &nVolume ) == 0 )
			{
				m_pcSlider->SetValue( nVolume, false );
			}
		}
	private:
		MediaManager* m_pcManager;
		DockMixer* m_pcPlugin;
		MixerView* m_pcView;
		Slider* m_pcSlider;
};

//*************************************************************************************

DockMixer::DockMixer( DockPlugin* pcPlugin, os::Looper* pcDock ) : View( os::Rect(), "dock_mixer" )
{
	os::File* pcFile;
	os::ResStream *pcStream;

	m_pcPlugin = pcPlugin;
	m_pcDock = pcDock;
	m_bCanDrag = m_bDragging = false;
	

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

DockMixer::~DockMixer( )
{
	delete( m_pcDockIcon );
	delete( m_pcDragIcon );
}

void DockMixer::Paint( const Rect &cUpdateRect )
{
    FillRect( GetBounds(), get_default_color( COL_NORMAL ) );
	SetBgColor( get_default_color( COL_NORMAL ) );
	SetDrawingMode( DM_BLEND );
	m_pcDockIcon->Draw( Point(0,0), this);
}


void DockMixer::AttachedToWindow()
{
	// Create media manager 
	m_pcManager = os::MediaManager::Get();
	
	// Create window
	m_pcWindow = new MixerWindow( m_pcManager, this, Rect( 0, 0, 50, 100 ) );
	m_pcWindow->Start();
	m_bWindowShown = false;
}


void DockMixer::DetachedFromWindow()
{
	m_pcWindow->Close();
	m_pcManager->Put();
}

void DockMixer::Remove( thread_id nThread )
{
	m_pcPlugin->RemoveView( this, nThread );
}

Point DockMixer::GetPreferredSize( bool bLargest ) const
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
			os::Message cMsg;
			BeginDrag( &cMsg, os::Point( m_pcDragIcon->GetBounds().Width() / 2,
											m_pcDragIcon->GetBounds().Height() / 2 ), m_pcDragIcon->LockBitmap() );
			m_bCanDrag = false;
		}
	}
	os::View::MouseMove( cNewPos, nCode, nButtons, pcData );
}


void DockMixer::MouseUp( const os::Point & cPosition, uint32 nButtons, os::Message * pcData )
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

void DockMixer::MouseDown( const os::Point& cPosition, uint32 nButtons )
{
	MakeFocus( true );
	if( nButtons == os::MOUSE_BUT_LEFT )
		m_bCanDrag = true;

	/* Display the mixer window */
	if( !m_bWindowShown )
	{
		m_pcWindow->MoveTo( ConvertToScreen( GetBounds() ).left - 25 + GetBounds().Width() / 2, ConvertToScreen( GetBounds() ).bottom );
		m_pcWindow->Lock();
		m_pcWindow->Update();
		m_pcWindow->Show();
		m_pcWindow->MakeFocus();
		m_pcWindow->Unlock();
		m_bWindowShown = true;
	} else
		m_pcWindow->MakeFocus();
	os::View::MouseDown( cPosition, nButtons );
}

//*************************************************************************************

class DockMixerPlugin : public os::DockPlugin
{
public:
	DockMixerPlugin()
	{
		m_pcView = NULL;
	}
	~DockMixerPlugin()
	{
	}
	status_t Initialize()
	{
		m_pcView = new DockMixer( this, GetApp() );
		AddView( m_pcView );
		return( 0 );
	}
	void Delete()
	{
		RemoveView( m_pcView );
	}
	os::String GetIdentifier()
	{
		return( "Mixer" );
	}
private:
	DockMixer* m_pcView;
};

extern "C"
{
DockPlugin* init_dock_plugin()
{
	return( new DockMixerPlugin() );
}
}
















