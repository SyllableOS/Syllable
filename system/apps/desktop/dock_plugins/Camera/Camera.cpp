/*
    A Screenshot utility for Dock

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
#include <gui/button.h>
#include <gui/imagebutton.h>
#include <gui/layoutview.h>
#include <gui/frameview.h>
#include <gui/stringview.h>
#include <util/resources.h>
#include <util/settings.h>
#include <gui/requesters.h>
#include <gui/dropdownmenu.h>
#include <gui/menu.h>
#include <util/datetime.h>
#include <gui/desktop.h>
#include <gui/image.h>
#include <storage/file.h>
#include <atheos/time.h>
#include <storage/streamableio.h>
#include <iostream>

#include <appserver/dockplugin.h>
#include <util/event.h>


#define PLUGIN_NAME       "Camera"
#define PLUGIN_VERSION    "2.0"
#define PLUGIN_DESC       "A ScreenShot Plugin for Dock"
#define PLUGIN_AUTHOR     "Rick Caudill"
                                                                                                                                                                                                        
using namespace os;
using namespace std;                                                                                                                                                                                                     
                                                                                                                                                                                                        
enum
{
	M_PREFS,
	M_PREFS_APPLY,
	M_PREFS_UNDO,
	M_PREFS_DEFAULT,
	M_PREFS_TICK,
	M_PREFS_SEND_TO_PARENT,
	M_PARENT_SEND_TO_PREFS,
	M_INSTANT_SCREENSHOT,
	M_DELAYED_SCREENSHOT,
	M_PRINT_SCREEN,
	M_CAMERA_ABOUT,
	M_HELP,
	CAMERA_ID = 0x0167
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

class CameraDelayedLooper : public os::Looper
{
public:
	CameraDelayedLooper();
	void TimerTick(int);
};


class CameraPrefsWin : public Window
{
public:
	CameraPrefsWin(os::Path,Message* pcMessage,View* pcParent);
private:
	void HandleMessage(Message*);
	void SetStates();
	bool OkToQuit();
	void LoadSettings();
	void LoadDefaults();
	
	std::vector <String> GetKeyboardKeys();
	void SetDropMenus();
		
	Message* m_pcMessage;
	View* m_pcParent;
	
	FrameView* m_pcDefaultFrame;
	FrameView* m_pcShortcutFrame;
	FrameView* m_pcDirectoryFrame;


	DropdownMenu* m_pcKeySecondDrop;
	DropdownMenu* m_pcInstantDrop;
	DropdownMenu* m_pcDelayDrop;
	DropdownMenu* m_pcSingleClickDrop;
	
	ImageButton* pcFileReqButton;
	TextView* 	pcDirectoryTextView;
	BitmapImage* m_pcFolderIcon;
	
	Button* pcApplyButton;
	Button* pcDefaultButton;
	Button* pcUndoButton;
	
	StringView* m_pcSecsString;
	StringView* m_pcSecsSliderString;
	StringView* m_pcDirectoryString;
	StringView* m_pcInstantString;
	StringView* m_pcDelayString;
	StringView* m_pcSingleClickString;

	LayoutView* pcLRoot;
	VLayoutNode* pcVLRoot;
	
	
	VLayoutNode* pcVLDelay;
	HLayoutNode* pcHLDelayOne;
	HLayoutNode* pcHLDelayTwo;
	FrameView* 	 pcFVDelay;
	
	VLayoutNode* pcVLKeyboard;
	HLayoutNode* pcHLKeyboardOne;
	HLayoutNode* pcHLKeyboardTwo;
	FrameView*   pcFVKeyboard;


	VLayoutNode* pcVLSingleClick;
	HLayoutNode* pcHLSingleClickOne;
	FrameView*   pcFVSingleClick;
	
	VLayoutNode* pcVLSaveTo;
	HLayoutNode* pcHLSaveToOne;
	FrameView*   pcFVSaveTo;
	
	VLayoutNode* pcVLButton;
	HLayoutNode* pcHLButton;
	
	float vDelayTime;
	float vInstant;
	float vClick;
	float vDelay;
	Message* pcPassToWindow;
	std::vector<String> cKeyboardKeys;
};	

class DockCamera : public View
{
	public:
		DockCamera( os::DockPlugin* pcPlugin, os::Looper* pcDock );
		~DockCamera();
		
		
		Point			GetPreferredSize( bool bLargest ) const;	
        virtual void	Paint( const Rect &cUpdateRect );
		virtual void	AttachedToWindow();
		virtual void	DetachedFromWindow();
		virtual void	MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData );
		virtual void	MouseUp( const os::Point & cPosition, uint32 nButton, os::Message * pcData );
		virtual void	MouseDown( const os::Point& cPosition, uint32 nButtons );
		virtual void	HandleMessage(Message* pcMessage);    

	private:
		int 			ScanForScreenshotFiles();
		void			LoadSettings();
		void			SaveSettings();
		void			DoDelayedScreenShot();
		void			DoInstantScreenShot();
		void			ShowPrefs(os::Path);
		void			OnSingleClick();			
		void			RefreshIcons();

	private:
		BitmapImage*	m_pcIcon;
		BitmapImage*	m_pcHoverIcon;
		BitmapImage*	pcPaint;
		Bitmap*			cm_pcBitmap;
		os::File*		pcFile;
		os::ResStream*	pcStream;		
		std::string		cScreenShotPath;
		
		bool			m_bCanDrag;
		bool			m_bDragging;
		bool			m_bHover;
		bool			bFirst;
		
		bigtime_t		m_nHitTime;
		float			vDelayTime;
		float			vInstant;
		float			vClick;
		float			vDelay;

		os::DockPlugin*			m_pcPlugin;
		os::Looper*				m_pcDock;
		Menu*					pcContextMenu;
		Message*				m_pcPassedToPrefsMessage;
		os::Event*				m_pcPrintKeyEvent;
		CameraPrefsWin*			pcPrefsWin;

public:
		CameraDelayedLooper*	pcCameraDelayedLooper;
		
};



CameraPrefsWin::CameraPrefsWin(os::Path cPath, Message* pcMessage, View* pcParent) : Window(Rect(0,0,300,300),"prefs_win","Camera Settings...",WND_NOT_RESIZABLE | WND_NO_DEPTH_BUT | WND_NO_ZOOM_BUT)
{
	CenterInScreen();
  	os::Rect cBounds = GetBounds();
  	os::Rect cRect = os::Rect(0,0,0,0);

	m_pcMessage = pcMessage;
	m_pcParent = pcParent;
	cKeyboardKeys = GetKeyboardKeys();
	
	pcLRoot = new os::LayoutView(cBounds, "L", NULL, os::CF_FOLLOW_ALL);
  	pcVLRoot = new os::VLayoutNode("VL");
  	pcVLRoot->SetBorders( os::Rect(10,5,10,5) );
	
	pcHLDelayTwo = new os::HLayoutNode("HLDelayTwo");
	pcHLDelayTwo->AddChild(m_pcKeySecondDrop=new DropdownMenu(cRect,"DDMKeySecond"));
	m_pcKeySecondDrop->SetReadOnly(true);
	m_pcKeySecondDrop->SetMinPreferredSize(15);
	m_pcKeySecondDrop->SetMaxPreferredSize(15);
	m_pcKeySecondDrop->SetSelectionMessage(new Message(M_PREFS_TICK));
	
	pcVLDelay= new os::VLayoutNode("VLDelay");	
	pcVLDelay->SetBorders( os::Rect(5,5,5,5) );
	pcVLDelay->AddChild(pcHLDelayTwo);
	
	pcFVDelay = new os::FrameView( cBounds, "FVDelay", "Screenshot Delay Time...", os::CF_FOLLOW_ALL);
  	pcFVDelay->SetRoot(pcVLDelay);
	
	
	pcHLKeyboardOne = new os::HLayoutNode("HLKeyboardOne");
	pcHLKeyboardOne->AddChild(m_pcInstantString=new StringView(cRect,"SVInstant","Instant:"));
	pcHLKeyboardOne->AddChild( new os::HLayoutSpacer("",2.0f,2.0f ));
	pcHLKeyboardOne->AddChild(m_pcInstantDrop=new DropdownMenu(cRect,"DDMInstant"));
	m_pcInstantDrop->SetReadOnly(true);
	m_pcInstantDrop->SetMinPreferredSize(15);
	m_pcInstantDrop->SetMaxPreferredSize(15);
	m_pcInstantDrop->SetEnable(false);
		
	pcHLKeyboardTwo = new os::HLayoutNode("HLKeyboardTwo");
	pcHLKeyboardTwo->AddChild(m_pcDelayString=new StringView(cRect,"SVDelay","Delay:"));
	pcHLKeyboardTwo->AddChild( new os::HLayoutSpacer("",2.0f,2.0f ));
	pcHLKeyboardTwo->AddChild(m_pcDelayDrop=new DropdownMenu(cRect,"DDMDelay"));
	m_pcDelayDrop->SetReadOnly(true);
	m_pcDelayDrop->SetMinPreferredSize(15);
	m_pcDelayDrop->SetMaxPreferredSize(15);
	m_pcDelayDrop->SetEnable(false);
	
	pcVLKeyboard= new os::VLayoutNode("VLKeyboard");	
	pcVLKeyboard->SetBorders( os::Rect(5,5,5,5) );
  	pcVLKeyboard->AddChild( new os::VLayoutSpacer("",2.0f,2.0f));
	pcVLKeyboard->AddChild(pcHLKeyboardOne);
	pcVLKeyboard->AddChild(pcHLKeyboardTwo);
	pcVLKeyboard->SameWidth("DDMInstant","DDMDelay","DDMKeySecond",NULL);
	pcVLKeyboard->SameWidth("SVInstant",NULL);	
	pcVLKeyboard->SameWidth("HLKeyboardOne","HLKeyboardTwo",NULL);

	
	pcFVKeyboard= new os::FrameView( cBounds, "FVKeyboard", "Keyboard Shortcuts...", os::CF_FOLLOW_ALL);
  	pcFVKeyboard->SetRoot(pcVLKeyboard);
  	
	pcHLSingleClickOne = new HLayoutNode("HLSingleClick");
  	pcHLSingleClickOne->AddChild(m_pcSingleClickString=new StringView(cRect,"SVSingleClick","Takes:"));
  	pcHLSingleClickOne->AddChild( new os::HLayoutSpacer("",4.0f,4.0f ));
  	pcHLSingleClickOne->AddChild(m_pcSingleClickDrop=new DropdownMenu(cRect,"DDMSingleClick"));
  	m_pcSingleClickDrop->SetReadOnly(true);
  	m_pcSingleClickDrop->SetMinPreferredSize(15);
  	m_pcSingleClickDrop->SetMaxPreferredSize(15);
  	m_pcSingleClickDrop->AppendItem("Instant Screenshot");
  	m_pcSingleClickDrop->AppendItem("Delayed Screenshot");
  	  	
	pcVLSingleClick = new VLayoutNode("VLSingleClick");
  	pcVLSingleClick->SetBorders(os::Rect(5,5,5,5));
  	pcVLSingleClick->AddChild(pcHLSingleClickOne);
  	pcFVSingleClick = new FrameView(cBounds,"FVSingleClick","Clicking icon...",os::CF_FOLLOW_ALL);
  	pcFVSingleClick->SetRoot(pcVLSingleClick);
  	
  	pcHLButton = new os::HLayoutNode("");
  	pcHLButton->AddChild(pcApplyButton=new Button(cRect,"BTApply","Apply",new Message(M_PREFS_APPLY)));
  	pcHLButton->AddChild(new os::HLayoutSpacer("",2.0f,2.0f));
  	pcHLButton->AddChild(pcUndoButton=new Button(cRect,"BTUndo","Undo",new Message(M_PREFS_UNDO)));
  	pcHLButton->AddChild(new os::HLayoutSpacer("",2.0f,2.0f));
  	pcHLButton->AddChild(pcDefaultButton=new Button(cRect,"BTDefault","Default",new Message(M_PREFS_DEFAULT)));
  	
  	pcVLButton = new os::VLayoutNode("");
  	pcVLButton->AddChild(pcHLButton);
  	pcVLButton->SameWidth("BTApply","BTUndo","BTDefault",NULL);
  	pcVLRoot->AddChild(pcFVDelay);
	pcVLRoot->AddChild(new os::VLayoutSpacer("",5.0f,5.0f));	
	pcVLRoot->AddChild(pcFVKeyboard);
	pcVLRoot->AddChild(new os::VLayoutSpacer("",5.0f,5.0f));
	pcVLRoot->AddChild(pcFVSingleClick);
	pcVLRoot->AddChild(new os::VLayoutSpacer("",5.0f,5.0f));
	pcVLRoot->SameWidth("FVSingleClick","FVDelay","FVKeyBoard",NULL);
	pcVLRoot->AddChild(pcVLButton);
	pcLRoot->SetRoot(pcVLRoot);
	AddChild(pcLRoot);
	ResizeTo(pcLRoot->GetPreferredSize(false));
	
	SetStates();
	LoadSettings();
}

bool CameraPrefsWin::OkToQuit()
{
	PostMessage(M_PREFS_APPLY,this);
	return false;
}


void CameraPrefsWin::SetStates()
{
	SetFocusChild(m_pcKeySecondDrop);
	SetDefaultButton(pcApplyButton);
	m_pcKeySecondDrop->SetTabOrder(0);
	m_pcInstantDrop->SetTabOrder(1);
	m_pcDelayDrop->SetTabOrder(2);
	pcApplyButton->SetTabOrder(3);
	pcUndoButton->SetTabOrder(4);
	pcDefaultButton->SetTabOrder(5);
	
	SetDropMenus();
}

void CameraPrefsWin::SetDropMenus()
{
	for (uint i=0;i<cKeyboardKeys.size(); i++)
	{
		String cSeconds = String();
		
		if (i == 0)
		{
			cSeconds = "1 Second";
			m_pcKeySecondDrop->AppendItem(cSeconds);	
		}
		else if (i < 10)
		{
			cSeconds.Format("%d Seconds",i+1);
			m_pcKeySecondDrop->AppendItem(cSeconds);
		}
		
		m_pcInstantDrop->AppendItem(cKeyboardKeys[i].c_str());
		m_pcDelayDrop->AppendItem(cKeyboardKeys[i].c_str());
	}
}

std::vector <String> CameraPrefsWin::GetKeyboardKeys()
{
	std::vector<String> cKeys;
	char zKey[4];
	
	for (int i=0; i<12; i++)
	{
		sprintf(zKey,"F%d",i+1);
		cKeys.push_back(zKey);
	}
	
	cKeys.push_back("Print Screen");
	return cKeys;	
}

void CameraPrefsWin::LoadSettings()
{
	if (m_pcMessage->FindFloat("delay",&vDelayTime) != 0)
		vDelayTime = 0;
		
	if (m_pcMessage->FindFloat("key/instant",&vInstant)!=0)
		vInstant = 12;
	
	if (m_pcMessage->FindFloat("key/delay",&vDelay)!=0)
		vDelay = 8;
			
	if (m_pcMessage->FindFloat("click",&vClick)!=0)
		vClick=0;
		
	m_pcKeySecondDrop->SetSelection((int)vDelayTime);
	m_pcInstantDrop->SetSelection((int)vInstant);
	m_pcDelayDrop->SetSelection((int)vDelay);
	m_pcSingleClickDrop->SetSelection((int)vClick);
	PostMessage(M_PREFS_TICK,this);
}
	
void CameraPrefsWin::LoadDefaults()
{
	vDelayTime = 0;
	vInstant = 12;
	vDelay = 8;
	vClick = 0;
	
	m_pcKeySecondDrop->SetSelection((int)vDelayTime);
	m_pcInstantDrop->SetSelection((int)vInstant);
	m_pcDelayDrop->SetSelection((int)vDelay);
	m_pcSingleClickDrop->SetSelection((int)vClick);

	PostMessage(M_PREFS_APPLY);
}

void CameraPrefsWin::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_PREFS_APPLY:
			{
				int nKeyInstant = m_pcInstantDrop->GetSelection();
				int nKeyDelay = m_pcDelayDrop->GetSelection();
				int nClickPreform = m_pcSingleClickDrop->GetSelection();

				pcPassToWindow = new Message(M_PREFS_SEND_TO_PARENT);
				pcPassToWindow->AddFloat("delay",(float)m_pcKeySecondDrop->GetSelection()+1);
				pcPassToWindow->AddFloat("key/instant",(float)nKeyInstant);
				pcPassToWindow->AddFloat("key/delay",(float)nKeyDelay);
				pcPassToWindow->AddFloat("click",(float)nClickPreform);
				m_pcParent->GetLooper()->PostMessage(pcPassToWindow,m_pcParent);
				delete( pcPassToWindow );
				break;
			}
			
		case M_PREFS_UNDO:
			LoadSettings();
			break;
		
		case M_PREFS_DEFAULT:
			LoadDefaults();
			break;
	}
}

CameraDelayedLooper::CameraDelayedLooper() : Looper("DelayedLooper")
{
}

void CameraDelayedLooper::TimerTick(int nID)
{
	
	if (nID == CAMERA_ID)
	{
		DateTime pcTime = DateTime::Now();
		
		BitmapImage* pcBitmapImage;
		BitmapImage* pcSaveImage;
		os::String cScreenShotPath;
		os::Desktop m_Screen;
	
	
		pcSaveImage = new os::BitmapImage( Bitmap::SHARE_FRAMEBUFFER | Bitmap::ACCEPT_VIEWS );
		pcSaveImage->SetColorSpace( CS_RGBA32 );
		pcSaveImage->ResizeCanvas( os::Point( m_Screen.GetResolution() ) );
		
		
		pcBitmapImage = new os::BitmapImage((uint8*)m_Screen.GetFrameBuffer(),m_Screen.GetResolution(),m_Screen.GetColorSpace());
		pcBitmapImage->Draw( os::Point( 0, 0 ), pcSaveImage->GetView() );
		pcSaveImage->Sync();
	
		cScreenShotPath = os::String().Format("%s/Pictures/Screenshot-%d-%d-%d-%d_%d_%d.png",getenv("HOME"),pcTime.GetYear(),pcTime.GetMonth(),pcTime.GetDay(),pcTime.GetHour(),pcTime.GetMin(),pcTime.GetSec());
		File vNewFile = os::File(cScreenShotPath,O_CREAT | O_TRUNC | O_WRONLY);
		vNewFile.WriteAttr("os::MimeType", O_TRUNC, ATTR_TYPE_STRING,"image/png", 0,10 );
		pcSaveImage->Save(&vNewFile,"image/png");
		vNewFile.Flush();
		
		delete( pcSaveImage );
		delete pcBitmapImage;
		RemoveTimer(this,nID);
	}
}


//*************************************************************************************
DockCamera::DockCamera( os::DockPlugin* pcPlugin, os::Looper* pcDock ) : View( os::Rect(), "camera" )
{
	m_pcDock = pcDock;
	m_pcPlugin = pcPlugin;
	m_bCanDrag = m_bDragging = false;
	m_bHover = false;
	pcPaint = NULL;
	pcPrefsWin = NULL;
	cm_pcBitmap = 0;
	m_nHitTime = 0;
	bFirst = true;
	
	pcContextMenu = new Menu(Rect(0,0,10,10),"",ITEMS_IN_COLUMN);
	pcContextMenu->AddItem("Preferences...",new Message(M_PREFS));
	pcContextMenu->AddItem(new MenuSeparator());	
	pcContextMenu->AddItem("Help...", new Message(M_HELP));
	pcContextMenu->AddItem(new MenuSeparator());	
	pcContextMenu->AddItem("About Camera...",new Message(M_CAMERA_ABOUT));
	
}



DockCamera::~DockCamera( )
{
	delete( pcContextMenu );
}

void DockCamera::OnSingleClick()
{
	if ((int)vClick==0)
	{
		pcCameraDelayedLooper->AddTimer(pcCameraDelayedLooper,CAMERA_ID,0);
	}
	
	else
	{
		int nTime = (int)(vDelayTime)*1000000;
		pcCameraDelayedLooper->AddTimer(pcCameraDelayedLooper,CAMERA_ID,nTime);
	}	
	return;
}

void DockCamera::AttachedToWindow()
{
	
	pcCameraDelayedLooper = new CameraDelayedLooper();
	pcCameraDelayedLooper->Run();
	
	/* Load default icons */
	pcFile = new os::File( m_pcPlugin->GetPath() );
	os::Resources cCol( pcFile );
	pcStream = cCol.GetResourceStream( "camera.png" );
	m_pcIcon = new os::BitmapImage( pcStream );
	delete( pcStream );
	delete pcFile;
	
	/* Load settings */
	LoadSettings();
	
	pcContextMenu->SetTargetForItems(this);
	
	m_pcPrintKeyEvent = new os::Event();
	m_pcPrintKeyEvent->SetToRemote("os/System/PrintKeyWasHit",-1);
	m_pcPrintKeyEvent->SetMonitorEnabled(true,this,M_PRINT_SCREEN);
}

void DockCamera::DetachedFromWindow()
{
	SaveSettings();
		
	if (m_pcIcon) delete m_pcIcon;
	if (pcPaint)delete pcPaint;
	if (m_pcPrintKeyEvent) delete m_pcPrintKeyEvent;
}


void DockCamera::LoadSettings()
{
	/* Load settings */
	os::String zPath = getenv( "HOME" );
	zPath += "/Settings/Camera/Settings";

	try
	{

		os::Settings* pcSettings = new os::Settings(new File(zPath));
		pcSettings->Load();

		vDelayTime = pcSettings->GetFloat("delay",0);
		vInstant = pcSettings->GetFloat("key/instant",12);
		vDelay = pcSettings->GetFloat("key/delay",8);
		vClick = pcSettings->GetFloat("click",0);
	
		delete( pcSettings );
	}
	catch(...)
	{
	}
}



void DockCamera::SaveSettings()
{
	/* Save settings */
	os::String zPath = getenv( "HOME" );
	zPath += "/Settings/Camera";

	try
	{
		mkdir( zPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO );
		zPath += "/Settings";
		os::File cFile( zPath, O_RDWR | O_CREAT );	
		os::Settings* pcSettings = new os::Settings(new File(zPath));
	
		pcSettings->AddFloat("delay",vDelayTime);
		pcSettings->AddFloat("key/instant",vInstant);
		pcSettings->AddFloat("key/delay",vDelay);
		pcSettings->AddFloat("click",vClick);
		pcSettings->Save();
		delete( pcSettings );
	}
	catch(...)
	{
	}
}


void DockCamera::Paint( const Rect &cUpdateRect )
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
	if (pcPaint && !m_bHover)	
		pcPaint->Draw( Point(0,0), this );
	else
	{
		m_pcIcon->Draw(Point(0,0),this);
	}

	SetDrawingMode(DM_COPY);
}

Point DockCamera::GetPreferredSize( bool bLargest ) const
{
	return m_pcIcon->GetSize();
}

void DockCamera::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_CAMERA_ABOUT:
		{
			String cTitle = (String)"About " + (String)PLUGIN_NAME + (String)"...";
			String cInfo = (String)"Version:  " +  (String)PLUGIN_VERSION + (String)"\n\nAuthor:   " + (String)PLUGIN_AUTHOR + (String)"\n\nDesc:      " + (String)PLUGIN_DESC;	
			Alert* pcAlert = new Alert(cTitle.c_str(),cInfo.c_str(),m_pcIcon->LockBitmap(),0,"OK",NULL);
			pcAlert->Go(new Invoker());
			pcAlert->MakeFocus();
			break;
		}
		case M_PREFS:
		{
			ShowPrefs(m_pcPlugin->GetPath());
			break;
		}
		
		case M_PREFS_SEND_TO_PARENT:
		{
			pcMessage->FindFloat("click",&vClick);
			pcMessage->FindFloat("key/instant",&vInstant);
			pcMessage->FindFloat("key/delay",&vDelay);
			pcMessage->FindFloat("delay",&vDelayTime);
			SaveSettings();
			LoadSettings();
			pcPrefsWin->Close();
			pcPrefsWin = NULL;			
		
			break;
		}
		
		case M_PRINT_SCREEN:
		{
			if (bFirst)
			{
				bFirst = false;
			}
			else
			{	
				pcCameraDelayedLooper->AddTimer(pcCameraDelayedLooper,CAMERA_ID,0);
			}
			break;
		}		
	}
}
void DockCamera::MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData )
{
	if (nCode == MOUSE_INSIDE)
	{
		if (m_bHover)
		{
			Bitmap* pcBitmap = m_pcIcon->LockBitmap();
			m_bHover = false;
			if( pcPaint == NULL )
			{
				pcPaint = new BitmapImage(pcBitmap->LockRaster(),IPoint((int)m_pcIcon->GetSize().x,(int)m_pcIcon->GetSize().y),pcBitmap->GetColorSpace());;
				pcPaint->ApplyFilter(BitmapImage::F_HIGHLIGHT);
				pcBitmap->UnlockRaster();
				pcPaint->UnlockBitmap();
			}
			Invalidate(GetBounds());
			Flush();
		}
	}

	else
	{
		m_bHover = true;
		Invalidate(GetBounds());
		Flush();
	}

	if( nCode != MOUSE_ENTERED && nCode != MOUSE_EXITED )
	{
		/* Create dragging operation */
		if( m_bCanDrag )
		{
			BitmapImage* pcIcon = m_pcIcon;
			m_bDragging = true;
			os::Message* pcMsg = new os::Message();
			BeginDrag( pcMsg, os::Point( pcIcon->GetBounds().Width() / 2,
											pcIcon->GetBounds().Height() / 2 ),pcIcon->LockBitmap() );
			delete( pcMsg );
			m_bCanDrag = false;
			Paint(GetBounds());
		}
	}

	os::View::MouseMove( cNewPos, nCode, nButtons, pcData );
}


void DockCamera::MouseUp( const os::Point & cPosition, uint32 nButtons, os::Message * pcData )
{
	if( m_bDragging && ( cPosition.y > 30 ) )
	{
		/* Remove ourself from the dock */
		os::Message cMsg( os::DOCK_REMOVE_PLUGIN );
		cMsg.AddPointer( "plugin", m_pcPlugin );
		m_pcDock->PostMessage( &cMsg, m_pcDock );
		Paint(GetBounds());
		return;
	}
	m_bDragging = false;
	m_bCanDrag = false;
	os::View::MouseUp( cPosition, nButtons, pcData );
}

void DockCamera::MouseDown( const os::Point& cPosition, uint32 nButtons )
{
	MakeFocus( true );
	if( nButtons == os::MOUSE_BUT_LEFT )
	{
		OnSingleClick();
		m_bCanDrag = true;
	} else if( nButtons == 2 ) {
		pcContextMenu->Open(ConvertToScreen(cPosition));
	}
	os::View::MouseDown( cPosition, nButtons );
}
//*************************************************************************************

void DockCamera::ShowPrefs(Path cPath)
{
	if (!pcPrefsWin)
	{
		m_pcPassedToPrefsMessage = new Message(M_PARENT_SEND_TO_PREFS);
		m_pcPassedToPrefsMessage->AddFloat("delay",vDelayTime-1);
		m_pcPassedToPrefsMessage->AddFloat("key/instant",vInstant);
		m_pcPassedToPrefsMessage->AddFloat("key/delay",vDelay);
		m_pcPassedToPrefsMessage->AddFloat("click",vClick);
		
		pcPrefsWin = new CameraPrefsWin(cPath,m_pcPassedToPrefsMessage,this);
		pcPrefsWin->Show();
		pcPrefsWin->MakeFocus();
	}
	
	else
		pcPrefsWin->MakeFocus();
}

//*************************************************************************************

class CameraPlugin : public os::DockPlugin
{
public:
	CameraPlugin()
	{
		m_pcView = NULL;
	}
	~CameraPlugin()
	{
	}
	status_t Initialize()
	{
		m_pcView = new DockCamera( this, GetApp() );
		AddView( m_pcView );
		return( 0 );
	}
	void Delete()
	{
		m_pcView->pcCameraDelayedLooper->Terminate();
		RemoveView( m_pcView );
	}
	os::String GetIdentifier()
	{
		return( PLUGIN_NAME );
	}
private:
	DockCamera* m_pcView;
};

extern "C"
{
DockPlugin* init_dock_plugin( os::Path cPluginFile, os::Looper* pcDock )
{
	return( new CameraPlugin() );
}
}






























































