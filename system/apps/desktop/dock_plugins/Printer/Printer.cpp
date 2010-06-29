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
#include <gui/requesters.h>
#include <util/looper.h>
#include <util/resources.h>
#include <appserver/dockplugin.h>

#define PLUGIN_NAME 	"Printer"
#define PLUGIN_VERSION 	"1.0"
#define PLUGIN_AUTHOR 	"The Knights of Syllable"
#define PLUGIN_DESC 	"A Print manager plugin for the Syllable Dock."

using namespace os;

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

enum
{
	M_PREF,
	M_QUEUE,
	M_ABOUT
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

		os::Point m_cPos;
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
	pcContextMenu->AddItem("Print Queue", new Message(M_QUEUE));
	pcContextMenu->AddItem(new MenuSeparator());
	pcContextMenu->AddItem("About Printer...", new Message(M_ABOUT));

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
	os::Color32_s sCurrentColor = BlendColours(get_default_color(os::COL_SHINE),  get_default_color(os::COL_NORMAL_WND_BORDER), 0.5f);
	SetFgColor( Tint( get_default_color( os::COL_SHADOW ), 0.5f ) );
   	os::Color32_s sBottomColor = BlendColours(get_default_color(os::COL_SHADOW), get_default_color(os::COL_NORMAL_WND_BORDER), 0.5f);
   
   	os::Color32_s sColorStep = os::Color32_s( ( sCurrentColor.red-sBottomColor.red ) / 30,
   											( sCurrentColor.green-sBottomColor.green ) / 30,
   											( sCurrentColor.blue-sBottomColor.blue ) / 30, 0 );
   
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

	SetDrawingMode( DM_BLEND );
	m_pcDockIcon->Draw( Point(0,0), this);
	SetDrawingMode( os::DM_COPY );
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
				system("/applications/preferences/Printers");
				exit(0);
			}
			break;
		}

		case M_QUEUE:
		{
			if( fork() == 0 )
			{
				system("/system/programs/print-queue");
				exit(0);
			}
			break;
		}
		case M_ABOUT:
		{
			String cTitle = (String)"About " + (String)PLUGIN_NAME + (String)"...";
			String cInfo = (String)"Version:  " +  PLUGIN_VERSION + "\n\nAuthor:   " + (String)PLUGIN_AUTHOR + "\n\nDesc:      " + (String)PLUGIN_DESC;	
	
			Alert* pcAlert = new Alert(cTitle.c_str(),cInfo.c_str(),m_pcDragIcon->LockBitmap(),0,"Close",NULL);
			m_pcDragIcon->UnlockBitmap();
			pcAlert->Go(new Invoker());
			pcAlert->MakeFocus();
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
			os::Message* pcMsg = new os::Message();
			BeginDrag( pcMsg, os::Point( m_pcDragIcon->GetBounds().Width() / 2,
											m_pcDragIcon->GetBounds().Height() / 2 ), m_pcDragIcon->LockBitmap() );
		
			delete( pcMsg );
			m_bCanDrag = false;
		}
	}
	
	os::View::MouseMove( cNewPos, nCode, nButtons, pcData );
}


void DockPrinter::MouseUp( const os::Point & cPosition, uint32 nButtons, os::Message * pcData )
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
	} /*else if ( nButtons == MOUSE_BUT_LEFT ) {
		// Check to see if the coordinates passed match when the left mouse button was pressed
		// if so, then it was a single click and not a drag
		if ( abs( (int)(m_cPos.x - cPosition.x) ) < DRAG_THRESHOLD && abs( (int)(m_cPos.y - cPosition.y) ) < DRAG_THRESHOLD )
		{
			// Just eat it for the time being.
	}
	}*/

	m_bDragging = false;
	m_bCanDrag = false;
	os::View::MouseUp( cPosition, nButtons, pcData );
}


void DockPrinter::MouseDown( const os::Point& cPosition, uint32 nButtons )
{

	if( nButtons == os::MOUSE_BUT_LEFT )
	{
		MakeFocus( true );
		if ( m_nHitTime + 500000 >= get_system_time() )
		{
			if( fork() == 0 )
			{
				system("/system/programs/print-queue");
				exit(0);
			}
		}
		else {
			m_bCanDrag = true;
			// Store these coordinates for later use in the MouseUp procedure
			m_cPos.x = cPosition.x;
			m_cPos.y = cPosition.y;
		}

		m_nHitTime = get_system_time();
	}

	if( nButtons == MOUSE_BUT_RIGHT ) {
		MakeFocus( false );
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

