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
#include <gui/desktop.h>
#include <gui/image.h>
#include <gui/window.h>
#include <gui/slider.h>
#include <util/looper.h>
#include <util/resources.h>
#include <util/event.h>
#include <util/thread.h>
#include <storage/file.h>
#include <media/manager.h>
#include <media/server.h>

#include <appserver/dockplugin.h>

using namespace os;

#define DRAG_THRESHOLD 4

enum { SET_VALUE };

enum {
	M_DOCK_POSITION_EV = 100
};

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

class MixerWindow;

class DockMixer : public View
{
	public:
		DockMixer( os::DockPlugin* pcPlugin, os::Looper* pcDock );
		~DockMixer();

		Point GetPreferredSize( bool bLargest ) const;
				
		virtual void HandleMessage( Message* pcMessage );
				
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

		Menu* m_pcContextMenu;
		os::Point m_cPos;
		
		os::Event* m_pcDockPositionEv;
		int m_nPosition;
};

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
			DrawFrame( GetBounds(), FRAME_RAISED | FRAME_THIN );
		}
};

class MixerWindow : public Window
{
	public:
		MixerWindow( MediaManager* pcManager, DockMixer* pcPlugin, const Rect& cFrame ) : Window( cFrame, "mixer", "mixer", os::WND_NO_TITLE | os::WND_NOT_RESIZABLE | os::WND_NO_BORDER, ALL_DESKTOPS )
		{
			m_pcPlugin = pcPlugin;
			m_pcManager = pcManager;
			m_pcView = new MixerView( GetBounds() );
			m_pcSlider = new Slider( GetBounds().Resize( 3, 3, -3, -3 ), "slider", new os::Message( SET_VALUE ), Slider::TICKS_RIGHT, 10, Slider::KNOB_SQUARE, VERTICAL );
			m_pcSlider->SetMinMax( 0, 100 );
			m_pcSlider->SetTarget( this );
			m_pcView->AddChild( m_pcSlider );
			AddChild( m_pcView );
			//m_pcView->MakeFocus();
			m_pcSlider->MakeFocus();
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

DockMixer::DockMixer( DockPlugin* pcPlugin, os::Looper* pcDock ) : View( os::Rect(), "dock_mixer", WID_WILL_DRAW | WID_TRANSPARENT )
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
	
	pcStream = cDockCol.GetResourceStream( "icon48x48.png" );
	m_pcDragIcon = new os::BitmapImage( pcStream );
	delete( pcStream );
	delete( pcFile );
	
	int m_nPosition = -1;
	m_pcDockPositionEv = NULL;
}

DockMixer::~DockMixer( )
{
	delete( m_pcDockIcon );
	delete( m_pcDragIcon );
}

void DockMixer::HandleMessage( Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case M_DOCK_POSITION_EV:
		{
			if( pcMessage->FindInt32( "position", &m_nPosition ) ) {
				printf( "Mixer dockplugin: could not get dock position from event!\n" );
			}
		}
		break;
		default:
		{
			View::HandleMessage( pcMessage );
		}
	}
}

void DockMixer::Paint( const Rect &cUpdateRect )
{
    bool bHorizontal = true;

	if( m_nPosition == os::ALIGN_TOP || m_nPosition == os::ALIGN_BOTTOM ) {
		bHorizontal = true;
	} else if( m_nPosition == os::ALIGN_LEFT || m_nPosition == os::ALIGN_RIGHT ) {
		bHorizontal = false;
	}
	
	os::Color32_s sCurrentColor = BlendColours(get_default_color(os::COL_SHINE),  get_default_color(os::COL_NORMAL_WND_BORDER), 0.5f);
	SetFgColor( Tint( get_default_color( os::COL_SHADOW ), 0.5f ) );
   	os::Color32_s sBottomColor = BlendColours(get_default_color(os::COL_SHADOW), get_default_color(os::COL_NORMAL_WND_BORDER), 0.5f);
   
   	os::Color32_s sColorStep = os::Color32_s( ( sCurrentColor.red-sBottomColor.red ) / 30,
   											( sCurrentColor.green-sBottomColor.green ) / 30,
   											( sCurrentColor.blue-sBottomColor.blue ) / 30, 0 );
   
   	if( bHorizontal )
	{
		if( cUpdateRect.DoIntersect( os::Rect( 0, 0, GetBounds().right, 29 ) ) )
		{
			sCurrentColor.red -= (int)cUpdateRect.top * sColorStep.red;
			sCurrentColor.green -= (int)cUpdateRect.top * sColorStep.green;
			sCurrentColor.blue -= (int)cUpdateRect.top * sColorStep.blue;
			for( int i = (int)cUpdateRect.top; i < ( (int)cUpdateRect.bottom < 30 ? (int)cUpdateRect.bottom + 1 : 30 ); i++ )
			{				
				SetFgColor( sCurrentColor );
				DrawLine( os::Point( cUpdateRect.left, i ), os::Point( cUpdateRect.right, i ) );
				sCurrentColor.red -= sColorStep.red;
				sCurrentColor.green -= sColorStep.green;
				sCurrentColor.blue -= sColorStep.blue;
			}
		}
	}
	else
	{
		if( cUpdateRect.DoIntersect( os::Rect( 0, 0, 29, GetBounds().bottom ) ) )
		{
			sCurrentColor.red -= (int)cUpdateRect.left * sColorStep.red;
			sCurrentColor.green -= (int)cUpdateRect.left * sColorStep.green;
			sCurrentColor.blue -= (int)cUpdateRect.left * sColorStep.blue;
			for( int i = (int)cUpdateRect.left; i < ( (int)cUpdateRect.right < 30 ? (int)cUpdateRect.right + 1 : 30 ); i++ )
			{
				SetFgColor( sCurrentColor );
				DrawLine( os::Point( i, cUpdateRect.top ), os::Point( i, cUpdateRect.bottom ) );
				sCurrentColor.red -= sColorStep.red;
				sCurrentColor.green -= sColorStep.green;
				sCurrentColor.blue -= sColorStep.blue;
			}
		}
	}

	SetDrawingMode( os::DM_BLEND );
	m_pcDockIcon->Draw( Point(0,0), this);
	SetDrawingMode( DM_COPY );
}


void DockMixer::AttachedToWindow()
{
	// Create media manager 
	m_pcManager = os::MediaManager::Get();
	
	// Create window
	m_pcWindow = new MixerWindow( m_pcManager, this, Rect( 0, 0, 50, 100 ) );
	m_pcWindow->Start();
	m_bWindowShown = false;

	/* Set up the Dock position event and get initial position */
	try {
		if( m_pcDockPositionEv ) delete( m_pcDockPositionEv );
		m_pcDockPositionEv = new Event();
		m_pcDockPositionEv->SetToRemote( "os/Dock/GetPosition", 0 );
		Message cMsg;
		m_pcDockPositionEv->GetLastEventMessage( &cMsg );
		cMsg.FindInt32( "position", &m_nPosition );
		
		m_pcDockPositionEv->SetMonitorEnabled( true, this, M_DOCK_POSITION_EV );
	} catch( ... ) {
		printf( "Mixer dockplugin: caught exception while trying to access Dock GetPosition event!\n" );
	}
}


void DockMixer::DetachedFromWindow()
{
	m_pcWindow->Close();
	m_pcManager->Put();
	
	if( m_pcDockPositionEv ) {
		delete( m_pcDockPositionEv );
		m_pcDockPositionEv = NULL;
	}
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
	} else if ( (nButtons == MOUSE_BUT_LEFT) || (nButtons == MOUSE_BUT_RIGHT) ) {
		// Check to see if the coordinates passed match when the left or right mouse button was pressed
		// if so, then it was a single click and not a drag
		if ( abs( (int)(m_cPos.x - cPosition.x) ) < DRAG_THRESHOLD && abs( (int)(m_cPos.y - cPosition.y) ) < DRAG_THRESHOLD )
{
	/* Display the mixer window */
	if( !m_bWindowShown )
	{
				// Get the current desktop's resolution
				os::Desktop cDesktop;
				IPoint dtRes = cDesktop.GetResolution();
				
				// Depending on the dock's position
				// position the mixer window relative the the plugin
				switch( m_nPosition ) {
					case os::ALIGN_TOP:
					{
						// Make sure the mixer window stays on the screen.
						if( ConvertToScreen( m_pcWindow->GetBounds() ).left > ( dtRes.x - m_pcWindow->GetBounds().Width() ) )
							m_pcWindow->MoveTo( dtRes.x - m_pcWindow->GetBounds().Width(), ConvertToScreen( GetBounds() ).bottom);
						else
		m_pcWindow->MoveTo( ConvertToScreen( GetBounds() ).left - 25 + GetBounds().Width() / 2, ConvertToScreen( GetBounds() ).bottom );
						break;
					}
					case os::ALIGN_BOTTOM:
					{
						// Make sure the mixer window stays on the screen.
						if( ConvertToScreen( m_pcWindow->GetBounds() ).left > ( dtRes.x - m_pcWindow->GetBounds().Width() ) )
							m_pcWindow->MoveTo( dtRes.x - m_pcWindow->GetBounds().Width(), ConvertToScreen( GetBounds() ).top - m_pcWindow->GetBounds().Height() );
						else
							m_pcWindow->MoveTo( ConvertToScreen( GetBounds() ).left - 25 + GetBounds().Width() / 2, ConvertToScreen( GetBounds() ).top - m_pcWindow->GetBounds().Height() );
						break;
					}
					case os::ALIGN_LEFT:
					{
						m_pcWindow->MoveTo( ConvertToScreen( GetBounds() ).right, ConvertToScreen( GetBounds() ).bottom - m_pcWindow->GetBounds().Height() );
						break;
					}
					case os::ALIGN_RIGHT:
					{
						m_pcWindow->MoveTo( ConvertToScreen( GetBounds() ).left - m_pcWindow->GetBounds().Width(), ConvertToScreen( GetBounds() ).bottom - m_pcWindow->GetBounds().Height() );
						break;
					}
				}
		m_pcWindow->Lock();
		m_pcWindow->Update();
		m_pcWindow->Show();
		m_pcWindow->MakeFocus();
		m_pcWindow->Unlock();
		m_bWindowShown = true;
	} else
		m_pcWindow->MakeFocus();
		}
	} 

	m_bDragging = false;
	m_bCanDrag = false;
	os::View::MouseUp( cPosition, nButtons, pcData );
}

void DockMixer::MouseDown( const os::Point& cPosition, uint32 nButtons )
{
	os:: Point cPos;

	MakeFocus ( true );
	m_bCanDrag = true;

	// Store these coordinates for later use in the MouseUp procedure
	m_cPos.x = cPosition.x;
	m_cPos.y = cPosition.y;

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
















