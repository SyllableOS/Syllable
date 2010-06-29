#include <appserver/dockplugin.h>
#include <gui/view.h>
#include <gui/image.h>
#include <gui/point.h>
#include <gui/requesters.h>
#include <storage/path.h>
#include <util/looper.h>
#include <util/event.h>
#include <gui/font.h>
#include <storage/file.h>
#include <util/resources.h>
#include <gui/desktop.h>
#include <gui/menu.h>


#include <cstdlib>
#include <cstdio>

#define PLUGIN_NAME "Switcher"
#define PLUGIN_VERSION  "1.0"
#define PLUGIN_DESC     "A Switcher Plugin for Dock."
#define PLUGIN_AUTHOR   "The Knights of Syllable"

using namespace os;

#define DRAG_THRESHOLD 4

enum
{
	M_MINIMIZE_ALL,
	M_ABOUT,
	
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

class Switcher : public View
{
public:
	Switcher(os::Path cPath, DockPlugin* pcPlugin, os::Looper* pcDock);
	~Switcher();
	
	os::String	GetIdentifier();
	os::Path	GetPath() {return m_cPath;}
	void 		Paint(const Rect&);
	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	virtual void MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData );
    virtual void MouseDown( const Point& cPosition, uint32 nButtons );
	virtual void MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData );
	virtual void HandleMessage(Message*);
    void		LoadImages();
    os::Point 	GetPreferredSize(bool) const; 
    void 		SwitchDesktop(int);
    void 		Forward();
    void		Backward();
	void		UpdateDesktop( int nDesktop );    

private:
	int m_nFirstVisibleDesktop;
	int m_nDesktop;
	os::BitmapImage* pcBackImage, *pcBackGreyImage;
	os::BitmapImage* pcForwardImage, *pcForwardGreyImage;
	os::BitmapImage* m_pcIcon;
	os::Menu* m_pcContextMenu;
	os::Path m_cPath;
	
	os::DockPlugin* m_pcPlugin;
	os::Looper* m_pcDock;

	bool m_bCanDrag;
	bool m_bDragging;
	
	os::Event* m_pcDockPositionEv;
	bool m_bHorizontal;

	os::Point m_cPos;
};

	
Switcher::Switcher(os::Path cPath, DockPlugin* pcPlugin, os::Looper* pcDock) : View(Rect(0,0,1,1),"switcher", WID_WILL_DRAW | WID_TRANSPARENT)
{
	m_pcPlugin = pcPlugin;
	m_pcDock = pcDock;
	m_bCanDrag = m_bDragging = false;

	os::Desktop cDesktop( os::Desktop::ACTIVE_DESKTOP );
	m_nDesktop = cDesktop.GetDesktop();
	m_nFirstVisibleDesktop = (m_nDesktop/4)*4;    /* "greatest multiple of 4 <= current desktop" */

	m_cPath = cPath;
	
	m_pcContextMenu = new Menu(Rect(0,0,1,1),"context_menu",ITEMS_IN_COLUMN);
	m_pcContextMenu->AddItem("Minimize All",new Message(M_MINIMIZE_ALL));
	m_pcContextMenu->AddItem(new MenuSeparator());
	m_pcContextMenu->AddItem("About Switcher...",new Message(M_ABOUT));
	
	m_pcDockPositionEv = NULL;
	m_bHorizontal = true;
	
	LoadImages();
}

Switcher::~Switcher()
{
	delete( pcBackImage );
	delete( pcBackGreyImage );
	delete( pcForwardImage );
	delete( pcForwardGreyImage );
	delete( m_pcContextMenu );
	delete( m_pcIcon );
}

void Switcher::AttachedToWindow()
{
 	m_pcContextMenu->SetTargetForItems(this);
 	
	/* Set up the Dock position event and get initial orientation */
	int nPosition = -1;
	if( m_pcDockPositionEv ) delete( m_pcDockPositionEv );
	try {
		m_pcDockPositionEv = new Event();
		m_pcDockPositionEv->SetToRemote( "os/Dock/GetPosition", 0 );
		Message cMsg;
		m_pcDockPositionEv->GetLastEventMessage( &cMsg );
		cMsg.FindInt32( "position", &nPosition );
		
		m_pcDockPositionEv->SetMonitorEnabled( true, this, M_DOCK_POSITION_EV );
	} catch( ... ) {
		printf( "Switcher dockplugin: caught exception while trying to access Dock GetPosition event!\n" );
	}
	if( nPosition == os::ALIGN_TOP || nPosition == os::ALIGN_BOTTOM ) {
		m_bHorizontal = true;
	} else if( nPosition == os::ALIGN_LEFT || nPosition == os::ALIGN_RIGHT ) {
		m_bHorizontal = false;
	} else {
		printf( "Switcher dockplugin: could not get dock orientation!\n" );
		m_bHorizontal = true;
	}
}

void Switcher::DetachedFromWindow()
{
	if( m_pcDockPositionEv ) delete( m_pcDockPositionEv );
	m_pcDockPositionEv = NULL;
}

os::Point Switcher::GetPreferredSize(bool bSize) const
{
	os::Point cPoint(64,GetBounds().Height()); 
	return cPoint;
}

void Switcher::HandleMessage( Message* pcMessage )
{
	switch ( pcMessage->GetCode() )
	{
		case M_MINIMIZE_ALL:
		{
			os::Desktop cDesktop( os::Desktop::ACTIVE_DESKTOP );
 			cDesktop.MinimizeAll();
		}
		break;
		case M_ABOUT:
		{
			String cTitle = (String)"About " + (String)PLUGIN_NAME + (String)"...";
			String cInfo = (String)"Version:  " +  (String)PLUGIN_VERSION + (String)"\n\nAuthor:   " + (String)PLUGIN_AUTHOR + (String)"\n\nDesc:      " + (String)PLUGIN_DESC;	
			Alert* pcAlert = new Alert(cTitle.c_str(),cInfo.c_str(),m_pcIcon->LockBitmap(),0,"Close",NULL);
			m_pcIcon->UnlockBitmap();
			pcAlert->Go(new Invoker());
			pcAlert->MakeFocus();
		}
		break;
		case M_DOCK_POSITION_EV:
		{
			int nPosition = -1;
			pcMessage->FindInt32( "position", &nPosition );
			if( nPosition == os::ALIGN_TOP || nPosition == os::ALIGN_BOTTOM ) {
				m_bHorizontal = true;
			} else if( nPosition == os::ALIGN_LEFT || nPosition == os::ALIGN_RIGHT ) {
				m_bHorizontal = false;
			} else {
				printf( "Switcher dockplugin: could not get dock orientation from dock position event!\n" );
			}
		}
		break;
		default:
		{
			View::HandleMessage( pcMessage );
		}
	}
}

void Switcher::Paint(const Rect& cRect)
{
	
	os::Color32_s sCurrentColor = BlendColours(get_default_color(os::COL_SHINE),  get_default_color(os::COL_NORMAL_WND_BORDER), 0.5f);
	SetFgColor( Tint( get_default_color( os::COL_SHADOW ), 0.5f ) );
   	os::Color32_s sBottomColor = BlendColours(get_default_color(os::COL_SHADOW), get_default_color(os::COL_NORMAL_WND_BORDER), 0.5f);
   
   	os::Color32_s sColorStep = os::Color32_s( ( sCurrentColor.red-sBottomColor.red ) / 30,
   											( sCurrentColor.green-sBottomColor.green ) / 30,
   											( sCurrentColor.blue-sBottomColor.blue ) / 30, 0 );
   
   	if( m_bHorizontal )
	{
		if( cRect.DoIntersect( os::Rect( 0, 0, GetBounds().right, 29 ) ) )
		{
			sCurrentColor.red -= (int)cRect.top * sColorStep.red;
			sCurrentColor.green -= (int)cRect.top * sColorStep.green;
			sCurrentColor.blue -= (int)cRect.top * sColorStep.blue;
			for( int i = (int)cRect.top; i < ( (int)cRect.bottom < 30 ? (int)cRect.bottom + 1 : 30 ); i++ )
			{				
				SetFgColor( sCurrentColor );
				DrawLine( os::Point( cRect.left, i ), os::Point( cRect.right, i ) );
				sCurrentColor.red -= sColorStep.red;
				sCurrentColor.green -= sColorStep.green;
				sCurrentColor.blue -= sColorStep.blue;
			}
		}
	}
	else
	{
		if( cRect.DoIntersect( os::Rect( 0, 0, 29, GetBounds().bottom ) ) )
		{
			sCurrentColor.red -= (int)cRect.left * sColorStep.red;
			sCurrentColor.green -= (int)cRect.left * sColorStep.green;
			sCurrentColor.blue -= (int)cRect.left * sColorStep.blue;
			for( int i = (int)cRect.left; i < ( (int)cRect.right < 30 ? (int)cRect.right + 1 : 30 ); i++ )
			{
				SetFgColor( sCurrentColor );
				DrawLine( os::Point( i, cRect.top ), os::Point( i, cRect.bottom ) );
				sCurrentColor.red -= sColorStep.red;
				sCurrentColor.green -= sColorStep.green;
				sCurrentColor.blue -= sColorStep.blue;
			}
		}
	}
	SetDrawingMode(DM_BLEND);
	
	//FillRect(GetBounds(),get_default_color(COL_NORMAL));
	FillRect(Rect(13,0,GetBounds().Width()-12,GetBounds().Height()),get_default_color(COL_SEL_WND_BORDER));
	
	if (m_nFirstVisibleDesktop == 0)
		pcBackGreyImage->Draw(os::Point(0,GetBounds().Height()/2-6),this);
	else
		pcBackImage->Draw(os::Point(0,GetBounds().Height()/2-6),this);
	
	if (m_nFirstVisibleDesktop >= 28)
		pcForwardGreyImage->Draw(os::Point(GetBounds().Width()-14,GetBounds().Height()/2-7),this);
	else
		pcForwardImage->Draw(os::Point(GetBounds().Width()-14,GetBounds().Height()/2-7),this);
	
	/*high light the current desktop*/	
	if (m_nDesktop == m_nFirstVisibleDesktop)
		FillRect(Rect(14,0,32,12),Color32_s(250,0,0));
	else if (m_nDesktop == m_nFirstVisibleDesktop+1)
		FillRect(Rect(34,0,GetBounds().Width()-12,12),Color32_s(250,0,0));
	else if (m_nDesktop == m_nFirstVisibleDesktop+2)
		FillRect(Rect(14,13,32,GetBounds().Height()-1),Color32_s(250,0,0));
	else if (m_nDesktop == m_nFirstVisibleDesktop+3)
		FillRect(Rect(34,13,GetBounds().Width()-12,GetBounds().Height()-1),Color32_s(250,0,0));;
	
	SetFgColor(100,160,255,0xff);
	DrawLine(os::Point(13,0),os::Point(13,GetBounds().Height()));
	SetFgColor(0,0,150,0xff);
	DrawLine(os::Point(GetBounds().Width()-12,0),os::Point(GetBounds().Width()-12,GetBounds().Height()));
	SetFgColor(100,160,255,0xff);
	DrawLine(os::Point(13,0),os::Point(GetBounds().Width()-12,0));
	SetFgColor(0,0,150,0xff);
	DrawLine(os::Point(13,GetBounds().Height()),os::Point(GetBounds().Width()-12,GetBounds().Height()));
	DrawLine(os::Point(13,GetBounds().Height()/2),os::Point(GetBounds().Width()-12,GetBounds().Height()/2));
	DrawLine(os::Point(GetBounds().Width()/2,0),os::Point(GetBounds().Width()/2,GetBounds().Height()));
	SetFgColor(100,160,255,0xff);
	DrawLine(os::Point(13,GetBounds().Height()/2+.5f),os::Point(GetBounds().Width()-12,GetBounds().Height()/2+.5f));
	DrawLine(os::Point(GetBounds().Width()/2+1,0),os::Point(GetBounds().Width()/2+1,GetBounds().Height()));
	
	SetDrawingMode(DM_OVER);
	SetFgColor(255,255,255);
	GetFont()->SetSize(7.5);
	
	char zLabel[10];
	snprintf( zLabel, sizeof(zLabel), "%i", m_nFirstVisibleDesktop+1 );  /* System counts desktops from 0, but user counts from 1 */
	DrawText(Rect(14,0,32,14),zLabel,DTF_CENTER);
	snprintf( zLabel, sizeof(zLabel), "%i", m_nFirstVisibleDesktop+2 );
	DrawText(Rect(34,0,GetBounds().Width()-12,14),zLabel,DTF_CENTER);
	snprintf( zLabel, sizeof(zLabel), "%i", m_nFirstVisibleDesktop+3 );
	DrawText(Rect(13,15,33,GetBounds().Height()),zLabel,DTF_CENTER);
	snprintf( zLabel, sizeof(zLabel), "%i", m_nFirstVisibleDesktop+4 );
	DrawText(Rect(34,15,GetBounds().Width()-12,GetBounds().Height()),zLabel,DTF_CENTER);

}

void Switcher::LoadImages()
{
	/* Load default icons */
	os::File* pcFile = new os::File( m_cPath );
	os::Resources cCol( pcFile );
	
	os::ResStream *pcStream = cCol.GetResourceStream( "icon48x48.png" );
	m_pcIcon = new os::BitmapImage( pcStream );
	delete( pcStream );

	os::ResStream* pcBackStream = cCol.GetResourceStream( "back.png" );
	pcBackImage = new os::BitmapImage( pcBackStream );
	delete( pcBackStream );
	
	os::ResStream* pcBackGreyStream = cCol.GetResourceStream( "back.png" );
	pcBackGreyImage = new BitmapImage(pcBackGreyStream);
	pcBackGreyImage->GrayFilter();
	delete( pcBackGreyStream );
	
	os::ResStream* pcForwardStream = cCol.GetResourceStream( "forward.png" );
	pcForwardImage = new os::BitmapImage( pcForwardStream );
	delete( pcForwardStream );
	
	os::ResStream* pcForwardGreyStream = cCol.GetResourceStream( "forward.png" );
	pcForwardGreyImage = new BitmapImage(pcForwardGreyStream);
	pcForwardGreyImage->GrayFilter();
	delete( pcForwardGreyStream );
	
	delete pcFile;
}



void Switcher::MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData )
{
	if( nCode != MOUSE_ENTERED && nCode != MOUSE_EXITED )
	{
		/* Create dragging operation */
		if( m_bCanDrag )
		{
			m_bDragging = true;
			os::Message* pcMsg = new os::Message();
			BeginDrag( pcMsg, os::Point( m_pcIcon->GetBounds().Width() / 2,
											m_pcIcon->GetBounds().Height() / 2 ), m_pcIcon->LockBitmap() );
		
			delete( pcMsg );
			m_bCanDrag = false;
		}
	}

	os::View::MouseMove( cNewPos, nCode, nButtons, pcData );
}

void Switcher::MouseDown( const Point& cPosition, uint32 nButtons )
{
	if( nButtons == os::MOUSE_BUT_LEFT )
	{
		MakeFocus( true );
		m_bCanDrag = true;

		// Store these coordinates for later use in the MouseUp procedure
		m_cPos.x = cPosition.x;
		m_cPos.y = cPosition.y;
	} else if ( nButtons == os::MOUSE_BUT_RIGHT )
		m_pcContextMenu->Open(ConvertToScreen(cPosition));	

	os::View::MouseDown( cPosition, nButtons );
}


void Switcher::MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData )
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
	} else if ( nButtons == os::MOUSE_BUT_LEFT ) {
		// Check to see if the coordinates passed match when the left mouse button was pressed
		// if so, then it was a single click and not a drag
		if ( abs( (int)(m_cPos.x - cPosition.x) ) < DRAG_THRESHOLD && abs( (int)(m_cPos.y - cPosition.y) ) < DRAG_THRESHOLD )
		{
	if (cPosition.x <= pcBackImage->GetSize().x)
		Backward();
	
	else if (cPosition.x >= 14 && cPosition.x <=32 && cPosition.y  < 13)
			SwitchDesktop(m_nFirstVisibleDesktop);
	
	else if (cPosition.x >=32 && cPosition.x <= 51 && cPosition.y < 13)
			SwitchDesktop(m_nFirstVisibleDesktop+1);
	
	else if (cPosition.x >= 14 && cPosition.x <=32  && cPosition.y >= 14)
			SwitchDesktop(m_nFirstVisibleDesktop+2);
	
	else if(cPosition.x >=32 && cPosition.x <= 51 && cPosition.y >=14)
			SwitchDesktop(m_nFirstVisibleDesktop+3);
	
	else if (cPosition.x >= 53 && cPosition.x < 61)
		Forward();
}
	}

	m_bDragging = false;
	m_bCanDrag = false;

	os::View::MouseUp( cPosition, nButtons, pcData );
}


void Switcher::SwitchDesktop(int nDesktop)
{
	m_nDesktop = nDesktop;
	Desktop cDesktop(m_nDesktop);
	cDesktop.Activate();
	Paint(GetBounds());
}

void Switcher::Forward()
{
	if (m_nFirstVisibleDesktop < 28)
	{
		m_nFirstVisibleDesktop += 4;
		Paint(GetBounds());
		GetParent()->Flush();
		GetParent()->Sync();
		GetParent()->Invalidate();		
	}
}

void Switcher::Backward()
{
	if (m_nFirstVisibleDesktop > 3)
	{
		m_nFirstVisibleDesktop -=4;
		Paint(GetBounds());
		GetParent()->Flush();
		GetParent()->Sync();
		GetParent()->Invalidate();		
	}
}

void Switcher::UpdateDesktop( int nDesktop )
{
	if( nDesktop < 0 || nDesktop > 31 ) return;  /* Passed out-of-range desktop!? Just in case! */
	
	/* User has switched desktops; update the view */
	m_nDesktop = nDesktop;
	
	if( !(m_nFirstVisibleDesktop <= m_nDesktop && m_nDesktop < m_nFirstVisibleDesktop + 4) ) {  /* If new desktop's button isn't currently displayed... */
		/* Set m_nFirstVisibleDesktop to "the greatest multiple of 4 <= new desktop" */
		m_nFirstVisibleDesktop = (((m_nDesktop)/4)*4);
	}
	Paint( GetBounds() );
}


class SwitcherPlugin : public DockPlugin
{
public:
		SwitcherPlugin() {};
		
		void DesktopActivated( int nDesktop, bool bActive )
		{
			if( bActive ) {m_pcView->UpdateDesktop( nDesktop ); }
		}
		
		os::String GetIdentifier()
		{
			return (PLUGIN_NAME);
		}
		
		status_t Initialize()
		{
			m_pcView = new Switcher(GetPath(),this,GetApp());
			AddView(m_pcView);
			return 0;
		}
		
		void Delete()
		{
			RemoveView(m_pcView);
		}
private:
	Switcher* m_pcView;
};


extern "C"
{
DockPlugin* init_dock_plugin()
{
	return( new SwitcherPlugin());
}
}
