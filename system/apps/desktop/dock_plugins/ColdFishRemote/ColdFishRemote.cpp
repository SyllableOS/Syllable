#include <gui/image.h>
#include <gui/desktop.h>
#include <storage/file.h>
#include <util/resources.h>
#include <util/resource.h>
#include <storage/registrar.h>
#include <storage/path.h>
#include <gui/menu.h>
#include <gui/font.h>
#include <util/catalog.h>
#include "resources/ColdFishRemote.h"

#include "dockplugin.h"
#include "messages.h"

enum
{
	GET_INFO_TIMER_CODE = 2016,
	M_GET_INFO = 2017,
	M_ADD_FILE = 2019,
	M_ABOUT = 2020
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


class ColdFishRemoteLooper : public os::Looper
{
public:
	ColdFishRemoteLooper(View* pcParent);
	~ColdFishRemoteLooper(){};
	
	void TimerTick(int);
private:
	View* pcParentView;
};

class ColdFishRemote : public DockPlugin
{
public:
	ColdFishRemote(os::Path cPath, os::Looper* pcDock);
	~ColdFishRemote();
	
	os::String GetIdentifier() {return "ColdFishRemote";}
	Point GetPreferredSize(bool bLargest);
	os::Path GetPath() {return (m_cPath);}
	
	virtual void Paint( const Rect &cUpdateRect );
	virtual void AttachedToWindow();
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
	
	os::BitmapImage* m_pcPrevImage, *m_pcStopImage, *m_pcPlayImage, *m_pcNextImage, *m_pcPauseImage;
	os::Path m_cPath;
	os::Looper* m_pcDock;
	ColdFishRemoteLooper* pcLooper;
	os::String cTrackName;
	os::File* pcFile;
	os::ResStream *pcStream;
	ColdFishRemoteScrollView* pcScrollView;
	os::RegistrarManager* pcManager;
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
	Paint(GetBounds());
}

ColdFishRemoteLooper::ColdFishRemoteLooper(View* pcParent) : os::Looper("cold_fish_remote_looper")
{
	pcParentView = pcParent;
}

void ColdFishRemoteLooper::TimerTick(int nCode)
{
	if (nCode ==  GET_INFO_TIMER_CODE)
	{
		pcParentView->GetLooper()->PostMessage(new Message(M_GET_INFO),pcParentView);
	}
}



ColdFishRemote::ColdFishRemote(os::Path cPath, os::Looper* pcDock) : DockPlugin()
{	
	m_pcDock = pcDock;
	m_cPath = cPath;
	
	pcLooper = new ColdFishRemoteLooper(this);
	/*fist timer is to get the new info every second*/
	pcLooper->AddTimer(pcLooper,GET_INFO_TIMER_CODE,1000000,false);
	pcLooper->Run();
	
	m_bHover = false;	
	m_bMouseDown = false;
	m_bIsPlaying = false;
	m_bIsPaused = false;
	m_nState = CF_STATE_STOPPED;
	pcManager = os::RegistrarManager::Get();
	
	LoadImages();
}

ColdFishRemote::~ColdFishRemote()
{
	if (pcLooper != NULL)
	{
		pcLooper->RemoveTimer(pcLooper,GET_INFO_TIMER_CODE);
		pcLooper->Terminate();
	}
}

void ColdFishRemote::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_GET_INFO:
		{
			RegistrarCall_s callInfo;
			Message cReply(0);	
			
			if (pcManager->QueryCall("media/Coldfish/GetSong",0,&callInfo) ==0)
			{
				if (pcManager->InvokeCall(&callInfo,new Message(),&cReply)== 0)
				{
					if (cReply.FindString("track_name",&cTrackName)!=0)
					{
						cTrackName = "Unknown";
					}
				}
			}
			else
				cTrackName = "Unknown";
			
			Message cReplyTwo(0);
			if (pcManager->QueryCall("media/Coldfish/GetPlayState",0,&callInfo) ==0)
			{
				if (pcManager->InvokeCall(&callInfo,new Message(),&cReplyTwo)== 0)
				{
					if (cReplyTwo.FindInt32("play_state",&m_nState) == 0)
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
				}
			}

			Paint(GetBounds());
			pcScrollView->SetTrackName(cTrackName);
			break;
		}

		case M_ADD_FILE:
		{
			RegistrarCall_s callInfo;

			pcManager->QueryCall("media/Coldfish/AddFile", 0, &callInfo);
			pcManager->InvokeCall(&callInfo, new Message(CF_GUI_ADD_FILE),NULL);
			break;
		}

		case M_ABOUT:
		{
			RegistrarCall_s callInfo;

			pcManager->QueryCall("media/Coldfish/About", 0, &callInfo);
			pcManager->InvokeCall(&callInfo, new Message(CF_GUI_ABOUT),NULL);
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
	pcScrollView = new ColdFishRemoteScrollView(Rect(0,12,70,GetParent()->GetBounds().Height()),"Unknown");
	AddChild(pcScrollView);	
	
	m_pcContextMenu = new os::Menu(Rect(0,0,1,1),"remote_plugin_context_menu",ITEMS_IN_COLUMN);
	m_pcContextMenu->AddItem("About",new Message(M_ABOUT));
	m_pcContextMenu->AddItem("Add File",new Message(M_ADD_FILE));
	m_pcContextMenu->SetTargetForItems(this);
}

void ColdFishRemote::MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData )
{
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
}



void ColdFishRemote::MouseDown( const os::Point& cPosition, uint32 nButtons )
{
	if (nButtons == 0x01)
	{
		m_bMouseDown = true;
	}
}

os::Point ColdFishRemote::GetPreferredSize(bool bSize)
{
	return os::Point(70,GetParent()->GetBounds().Height()); 
}

void ColdFishRemote::PauseOrPlay()
{
	if (m_bIsPlaying)
	{
		if (m_bIsPaused)
		{
			RegistrarCall_s callInfo;

			pcManager->QueryCall("media/Coldfish/Play", 0, &callInfo);
			pcManager->InvokeCall(&callInfo, new Message(CF_GUI_PLAY),NULL);		
			m_bIsPlaying = true;
			m_bIsPaused = false;
			m_nState = CF_STATE_PLAYING;
		}
		else
		{
			RegistrarCall_s callInfo;

			pcManager->QueryCall("media/Coldfish/Pause", 0, &callInfo);
			pcManager->InvokeCall(&callInfo, new Message(CF_GUI_PAUSE),NULL);
				
			m_bIsPlaying = false;
			m_bIsPaused = true;
			m_nState = CF_STATE_PAUSED;
		}
	}
	
	else
	{
		RegistrarCall_s callInfo;

		pcManager->QueryCall("media/Coldfish/Play", 0, &callInfo);
		pcManager->InvokeCall(&callInfo, new Message(CF_GUI_PLAY),NULL);		
		m_bIsPlaying = true;
		m_bIsPaused = false;
		m_nState = CF_STATE_PLAYING;		
	}
	Paint(GetBounds());
	GetParent()->Flush();
	GetParent()->Sync();
	GetParent()->Invalidate();
	
}

void ColdFishRemote::NextSong()
{
	RegistrarCall_s callInfo;

	pcManager->QueryCall("media/Coldfish/Next", 0, &callInfo);
	pcManager->InvokeCall(&callInfo, new Message(CF_PLAY_NEXT),NULL);
}

void ColdFishRemote::PrevSong()
{
	RegistrarCall_s callInfo;
	
	pcManager->QueryCall("media/Coldfish/Previous", 0, &callInfo);
	pcManager->InvokeCall(&callInfo, new Message(CF_PLAY_PREVIOUS),NULL);	
}
	
void ColdFishRemote::StopPlaying()
{
	RegistrarCall_s cCallInfo;
	
	pcManager->QueryCall("media/Coldfish/Stop", 0, &cCallInfo);
	pcManager->InvokeCall(&cCallInfo, new Message(CF_GUI_STOP),NULL);	
	
	m_bIsPlaying = false;
	m_bIsPaused = false;
	m_nState = CF_STATE_STOPPED;
	
	Paint(GetBounds());
	GetParent()->Flush();
	GetParent()->Sync();
	GetParent()->Invalidate();
}


void ColdFishRemote::LoadImages()
{
	/* Load default icons */
	pcFile = new os::File( m_cPath );
	os::Resources cCol( pcFile );
	
	os::ResStream* pcPlayStream = cCol.GetResourceStream( "play.png" );
	m_pcPlayImage = new os::BitmapImage( pcPlayStream );
	
	os::ResStream* pcStopStream = cCol.GetResourceStream( "stop.png" );
	m_pcStopImage = new os::BitmapImage( pcStopStream );
	
	os::ResStream* pcNextStream = cCol.GetResourceStream( "next.png" );
	m_pcNextImage = new os::BitmapImage( pcNextStream );
	
	os::ResStream* pcPrevStream = cCol.GetResourceStream( "prev.png" );
	m_pcPrevImage = new os::BitmapImage( pcPrevStream );
	
	os::ResStream* pcPauseStream = cCol.GetResourceStream( "pause.png" );
	m_pcPauseImage = new os::BitmapImage( pcPauseStream );
	
	delete pcFile;
}



extern "C"
{
DockPlugin* init_dock_plugin( os::Path cPluginFile, os::Looper* pcDock )
{
	return( new ColdFishRemote( cPluginFile, pcDock ) );
}
}
