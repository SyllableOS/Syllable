#include <gui/image.h>
#include <gui/desktop.h>
#include <storage/file.h>
#include <util/resources.h>
#include <util/resource.h>
#include <util/event.h>
#include <storage/path.h>
#include <gui/menu.h>
#include <gui/font.h>
#include <util/catalog.h>
#include "resources/ColdFishRemote.h"

#include <appserver/dockplugin.h>
#include "messages.h"

enum
{
	MSG_PLAY_STATE,
	MSG_UPDATE,
	MSG_ADD_FILE,
	MSG_ABOUT,
	GET_INFO_TIMER_CODE
};

using namespace os;

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

class ColdFishRemoteScrollView : public View
{
public:
	ColdFishRemoteScrollView(const Rect&,const String&);
	
	void Paint(const Rect&);
	void SetTrackName(const String&);
private:
	String cName;
	int nCount;
};

class ColdFishRemote : public os::View
{
public:
	ColdFishRemote( os::DockPlugin* pcPlugin, os::Looper* pcDock);
	~ColdFishRemote();
	
	Point GetPreferredSize(bool bLargest) const;
	
	virtual void Paint( const Rect &cUpdateRect );
	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	virtual void MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData );
	virtual void MouseUp( const os::Point & cPosition, uint32 nButton, os::Message * pcData );
	virtual void MouseDown( const os::Point& cPosition, uint32 nButtons );
	virtual void HandleMessage(os::Message* pcMessage);
private:
	void LoadImages();
	
	void PauseOrPlay();
	void NextSong();
	void PrevSong();
	void StopPlaying();
	
	os::DockPlugin* m_pcPlugin;	
	os::BitmapImage* m_pcPrevImage, *m_pcStopImage, *m_pcPlayImage, *m_pcNextImage, *m_pcPauseImage;
	os::Looper* m_pcDock;
	os::String cTrackName;
	os::File* pcFile;
	os::ResStream *pcStream;
	ColdFishRemoteScrollView* pcScrollView;
	os::Menu* m_pcContextMenu;
	bool m_bCanDrag;
	bool m_bDragging;
	bool m_bHover;
	bool m_bMouseDown;
	bool m_bIsPlaying;
	bool m_bIsPaused;
	int m_nState;
};

ColdFishRemoteScrollView::ColdFishRemoteScrollView(const Rect& cRect,const String& cString) : View(cRect,"cold_fish_remote_scroll_view")
{
	GetFont()->SetSize(7.5);
	cName = cString;
	nCount = 0;
}

void ColdFishRemoteScrollView::Paint(const Rect& cUpdateRect)
{

	FillRect(GetBounds(),get_default_color(COL_NORMAL));
	
	SetDrawingMode(DM_OVER);
	SetFgColor(100,100,100);
	
	if (nCount >= cName.Length()-9)
	{
		nCount = 0;
	}
	
	Rect cBounds = GetBounds();

	if (cName.Length() < 9)
	{
		DrawText(cBounds,cName.c_str(),DTF_ALIGN_CENTER);
	}	
	else
		DrawText(cBounds,cName.substr(nCount,nCount+9).c_str(),DTF_ALIGN_CENTER);
	
	nCount++;
}

void ColdFishRemoteScrollView::SetTrackName(const String& cString)
{
	if (cString != cName)
	{
		nCount = 0;
		cName = cString;
	}
	Invalidate();
	Flush();
}


ColdFishRemote::ColdFishRemote( os::DockPlugin* pcPlugin, os::Looper* pcDock) : View( os::Rect(), "coldfish" )
{	
	m_pcDock = pcDock;
	m_pcPlugin = pcPlugin;
	
	pcScrollView = new ColdFishRemoteScrollView(Rect(0,15,70,25),"Unknown");
	AddChild( pcScrollView );
	
	m_bHover = false;	
	m_bMouseDown = false;
	m_bIsPlaying = false;
	m_bIsPaused = false;
	m_nState = CF_STATE_STOPPED;
	
	LoadImages();
}

ColdFishRemote::~ColdFishRemote()
{
	delete( m_pcPrevImage );
	delete( m_pcStopImage );
	delete( m_pcPlayImage );
	delete( m_pcNextImage );
	delete( m_pcPauseImage );
}

void ColdFishRemote::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case MSG_UPDATE:
		{
	
			if (pcMessage->FindString("track_name",&cTrackName)==0)
			{
				pcScrollView->SetTrackName(cTrackName);
				Invalidate();
				Flush();
			}

			if (pcMessage->FindInt32("play_state",&m_nState) == 0)
			{				
				if (m_nState == CF_STATE_PAUSED)
				{
					m_bIsPlaying = true;
					m_bIsPaused = true;
				}
				else if (m_nState == CF_STATE_PLAYING)
				{
					m_bIsPlaying = true;
					m_bIsPaused = false;
				}
				else
				{
					m_bIsPlaying = false;
					m_bIsPaused = false;
				}					
			}
			
			break;
		}

		case MSG_ADD_FILE:
		{
			
			os::Event cEvent;
			if( cEvent.SetToRemote( "media/Coldfish/AddFile", 0 ) == 0 )
			{
				Message cMsg(CF_GUI_ADD_FILE);
				cEvent.PostEvent(&cMsg,NULL, 1000000);
			}
			break;
		}

		case MSG_ABOUT:
		{
			os::Event cEvent;
			if( cEvent.SetToRemote( "media/Coldfish/About", 0 ) == 0 )
			{
				Message cMsg(CF_GUI_ABOUT);
				cEvent.PostEvent(&cMsg,NULL, 1000000);
			}
			break;
		}		
	}
}

void ColdFishRemote::Paint(const Rect& cUpdateRect)
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

	SetDrawingMode( os::DM_BLEND );
	if (!m_bHover)
	{	
		os::Point cPoint(0,0);
		
		m_pcPrevImage->Draw(cPoint, this );
		cPoint.x += m_pcPrevImage->GetSize().x+2;
	
		if (m_nState == CF_STATE_STOPPED || m_nState == CF_STATE_PAUSED)
		{
			m_pcPlayImage->Draw(cPoint,this);
			cPoint.x += m_pcPlayImage->GetSize().x+2;
		}
		else
		{
			m_pcPauseImage->Draw(cPoint,this);
			cPoint.x += m_pcPauseImage->GetSize().x+2;
		}
		
		m_pcStopImage->Draw(cPoint,this);
		cPoint.x += m_pcStopImage->GetSize().x+2;
		
		m_pcNextImage->Draw(cPoint,this);
	}
}

void ColdFishRemote::AttachedToWindow()
{
	m_pcContextMenu = new os::Menu(Rect(0,0,1,1),"remote_plugin_context_menu",ITEMS_IN_COLUMN);
	m_pcContextMenu->AddItem("About",new Message(MSG_ABOUT));
	m_pcContextMenu->AddItem("Add File",new Message(MSG_ADD_FILE));
	m_pcContextMenu->SetTargetForItems(this);
}

void ColdFishRemote::DetachedFromWindow()
{
	delete( m_pcContextMenu );
}

void ColdFishRemote::MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData )
{
	os::View::MouseMove( cNewPos, nCode, nButtons, pcData );
}

void ColdFishRemote::MouseUp( const os::Point & cPosition, uint32 nButton, os::Message * pcData )
{
	if (nButton == 0x01 && m_bMouseDown)
	{
		m_bMouseDown = false;
				
		os::Point cPoint(0,0);
		
		if (cPosition.x <= m_pcPrevImage->GetSize().x && cPosition.x >=0)
		{
			PrevSong();
			return;
		}
		
		cPoint.x += m_pcPrevImage->GetSize().x + 2;
		
		if (cPosition.x >= cPoint.x && (cPosition.x <= cPoint.x+m_pcPlayImage->GetSize().x || cPosition.x <= cPoint.x+m_pcPauseImage->GetSize().x))
		{
			PauseOrPlay();	
			return;
		}
		
		cPoint.x += m_pcPlayImage->GetSize().x +2;
		
		if (cPosition.x >= cPoint.x && cPosition.x <= cPoint.x+m_pcStopImage->GetSize().x )
		{
			StopPlaying();
			return;
		}
		
		cPoint.x += m_pcStopImage->GetSize().x+2;
		
		if (cPosition.x >= cPoint.x && cPosition.x <= cPoint.x+m_pcNextImage->GetSize().x )
		{
			NextSong();
			return;
		}
		
		else;
	}
	else if (nButton == 0x02)
	{
		m_pcContextMenu->Open(ConvertToScreen(cPosition));
	}
	os::View::MouseUp( cPosition, nButton, pcData );
}



void ColdFishRemote::MouseDown( const os::Point& cPosition, uint32 nButtons )
{
	if (nButtons == 0x01)
	{
		m_bMouseDown = true;
	}
	os::View::MouseDown( cPosition, nButtons );
}

os::Point ColdFishRemote::GetPreferredSize(bool bSize) const
{
	return os::Point(70,0); 
}

void ColdFishRemote::PauseOrPlay()
{
	if (m_bIsPlaying)
	{
		if (m_bIsPaused)
		{
			os::Event cEvent;
			if( cEvent.SetToRemote( "media/Coldfish/Play", 0 ) == 0 )
			{
				Message cMsg(CF_GUI_PLAY);
				cEvent.PostEvent(&cMsg,NULL);
			}

			m_bIsPlaying = true;
			m_bIsPaused = false;
			m_nState = CF_STATE_PLAYING;
		}
		else
		{
			os::Event cEvent;
			if( cEvent.SetToRemote( "media/Coldfish/Pause", 0 ) == 0 )
			{
				Message cMsg(CF_GUI_PAUSE);
				cEvent.PostEvent(&cMsg,NULL);
			}
				
			m_bIsPlaying = false;
			m_bIsPaused = true;
			m_nState = CF_STATE_PAUSED;
		}
	}
	
	else
	{
		os::Event cEvent;
		if( cEvent.SetToRemote( "media/Coldfish/Play", 0 ) == 0 )
		{
			Message cMsg(CF_GUI_PLAY);
			cEvent.PostEvent(&cMsg,NULL);
		}
		m_bIsPlaying = true;
		m_bIsPaused = false;
		m_nState = CF_STATE_PLAYING;		
	}
	Invalidate();
	Flush();
}

void ColdFishRemote::NextSong()
{
	os::Event cEvent;
	if( cEvent.SetToRemote( "media/Coldfish/Next", 0 ) == 0 )
	{
		Message cMsg(CF_PLAY_NEXT);
		cEvent.PostEvent(&cMsg,NULL);
	}
}

void ColdFishRemote::PrevSong()
{
	os::Event cEvent;
	if( cEvent.SetToRemote( "media/Coldfish/Previous", 0 ) == 0 )
	{
		Message cMsg(CF_PLAY_PREVIOUS);
		cEvent.PostEvent(&cMsg,NULL);
	}
}
	
void ColdFishRemote::StopPlaying()
{
	os::Event cEvent;
	if( cEvent.SetToRemote( "media/Coldfish/Stop", 0 ) == 0 )
	{
		Message cMsg(CF_GUI_STOP);
		cEvent.PostEvent(&cMsg,NULL);
	}
	
	m_bIsPlaying = false;
	m_bIsPaused = false;
	m_nState = CF_STATE_STOPPED;
	
	Invalidate();
	Flush();
}


void ColdFishRemote::LoadImages()
{
	/* Load default icons */
	pcFile = new os::File( m_pcPlugin->GetPath() );
	os::Resources cCol( pcFile );
	
	os::ResStream* pcPlayStream = cCol.GetResourceStream( "play.png" );
	m_pcPlayImage = new os::BitmapImage( pcPlayStream );
	delete( pcPlayStream );
	
	os::ResStream* pcStopStream = cCol.GetResourceStream( "stop.png" );
	m_pcStopImage = new os::BitmapImage( pcStopStream );
	delete( pcStopStream );
	
	os::ResStream* pcNextStream = cCol.GetResourceStream( "next.png" );
	m_pcNextImage = new os::BitmapImage( pcNextStream );
	delete( pcNextStream );
	
	os::ResStream* pcPrevStream = cCol.GetResourceStream( "prev.png" );
	m_pcPrevImage = new os::BitmapImage( pcPrevStream );
	delete( pcPrevStream );
	
	os::ResStream* pcPauseStream = cCol.GetResourceStream( "pause.png" );
	m_pcPauseImage = new os::BitmapImage( pcPauseStream );
	delete( pcPauseStream );
	
	delete pcFile;
}

class ColdFishRemoteLooper : public os::Looper
{
public:
	ColdFishRemoteLooper(os::DockPlugin* pcPlugin);
	~ColdFishRemoteLooper();
	
	void TimerTick(int);
	void HandleMessage( os::Message* pcMsg );
private:
	os::DockPlugin* m_pcPlugin;
	ColdFishRemote* m_pcView;
};

ColdFishRemoteLooper::ColdFishRemoteLooper(os::DockPlugin* pcPlugin) : os::Looper("cold_fish_remote_looper")
{
	m_pcPlugin = pcPlugin;
	m_pcView = NULL;
}

ColdFishRemoteLooper::~ColdFishRemoteLooper()
{
}

void ColdFishRemoteLooper::TimerTick(int nCode)
{
	if ( nCode == GET_INFO_TIMER_CODE )
	{
		if( m_pcView != NULL )
		{
			os::Event cEvent;
			os::Message cMsg;
			if (cEvent.SetToRemote("media/Coldfish/GetSong",0) ==0)
			{
				cEvent.PostEvent(&cMsg,m_pcView, MSG_UPDATE);
			}
			if (cEvent.SetToRemote("media/Coldfish/GetPlayState",0) ==0)
			{
				cEvent.PostEvent(&cMsg,m_pcView, MSG_UPDATE);
			}
		}
	}
}

void ColdFishRemoteLooper::HandleMessage( os::Message* pcMsg )
{
	switch( pcMsg->GetCode() )
	{
		case MSG_PLAY_STATE:
		{
			bool bDummy;
			if( pcMsg->FindBool( "event_registered", &bDummy ) == 0 )
			{
				if( m_pcView == NULL ) {
					m_pcView = new ColdFishRemote( m_pcPlugin, m_pcPlugin->GetApp() );
					m_pcPlugin->AddView( m_pcView );
				}
			}
			else if( pcMsg->FindBool( "event_unregistered", &bDummy ) == 0 )
			{
				if( m_pcView != NULL )
					m_pcPlugin->RemoveView( m_pcView );
				m_pcView = NULL;
			}
		}
		break;
		default:
		break;
	}
	os::Looper::HandleMessage( pcMsg );
}

//*************************************************************************************

class ColdFishPlugin : public os::DockPlugin
{
public:
	ColdFishPlugin()
	{
		m_pcLooper = NULL;
	}
	~ColdFishPlugin()
	{
		if (m_pcLooper != NULL)
		{
			delete( m_pcNotifier );
			m_pcLooper->RemoveTimer(m_pcLooper,GET_INFO_TIMER_CODE);
			m_pcLooper->Terminate();
			m_pcLooper = NULL;
		}
	}
	status_t Initialize()
	{
		m_pcLooper = new ColdFishRemoteLooper(this);
		m_pcLooper->AddTimer(m_pcLooper,GET_INFO_TIMER_CODE,1000000,false);
		m_pcLooper->Run();
		m_pcNotifier = new os::Event();
		m_pcNotifier->SetToRemote( "media/Coldfish/GetSong", -1 );
		m_pcNotifier->SetMonitorEnabled( true, m_pcLooper, MSG_PLAY_STATE );
		return( 0 );
	}
	void Delete()
	{
		if ( GetViewCount() == 0 && m_pcLooper != NULL)
		{
			delete( m_pcNotifier );
			m_pcLooper->RemoveTimer(m_pcLooper,GET_INFO_TIMER_CODE);
			m_pcLooper->Terminate();
			m_pcLooper = NULL;
		}
	}
	os::String GetIdentifier()
	{
		return( "ColdFish" );
	}
private:
	ColdFishRemoteLooper* m_pcLooper;
	os::Event* m_pcNotifier;
};

extern "C"
{
DockPlugin* init_dock_plugin()
{
	return( new ColdFishPlugin() );
}
}






























