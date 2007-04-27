#include "WallpaperChanger.h"
#include "messages.h"                                                                                                                                                                                                   
#include <util/event.h>

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

/*************************************************
* Description: Plugin for wallpaper changing
* Author: Rick Caudill
* Date: Wed Mar  9 17:59:59 2005
**************************************************/
DockWallpaperChanger::DockWallpaperChanger( os::Path cPath, os::Looper* pcDock ) : View(Rect(0,0,1,1),"wallpaper_view")
{
	m_pcDock = pcDock;
	m_cPath = cPath;	
	m_bCanDrag = m_bDragging = false;
	m_bHover = false;
	pcPaint = NULL;
	cm_pcBitmap = 0;
	pcPrefsWin = NULL;
	
	
}


/*************************************************
* Description: Destructor for plugin... gets called when dock exits
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
DockWallpaperChanger::~DockWallpaperChanger( )
{
}

/*************************************************
* Description: Makes sure that the plugin is attached to the window, so we can pass messages
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
void DockWallpaperChanger::AttachedToWindow()
{
	/* Load default icons */
	pcFile = new os::File( m_cPath );
	os::Resources cCol( pcFile );
	pcStream = cCol.GetResourceStream( "wallpaper.png" );
	m_pcIcon = new os::BitmapImage( pcStream );
	delete pcFile;
		
	pcChangeLooper = new DockWallpaperChangerLooper(this);
	pcChangeLooper->Run();
	
	LoadSettings();
	
	pcContextMenu = new Menu(Rect(0,0,10,10),"",ITEMS_IN_COLUMN);
	pcContextMenu->AddItem("Preferences...",new Message(M_PREFS));
	pcContextMenu->AddItem(new MenuSeparator());	
	pcContextMenu->AddItem("Help...", new Message(M_HELP));
	pcContextMenu->AddItem(new MenuSeparator());	
	pcContextMenu->AddItem("About WallpaperChanger...",new Message(M_WALLPAPERCHANGER_ABOUT));
	pcContextMenu->SetTargetForItems(this);
	View::AttachedToWindow();
	pcContextMenu->SetTargetForItems(this);
}

void DockWallpaperChanger::DetachedFromWindow()
{
	SaveSettings();  //make sure to save the settings :)
	if (m_pcIcon) delete m_pcIcon;
	if (pcPaint)delete pcPaint;
}



/*************************************************
* Description: Paints the view
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
void DockWallpaperChanger::Paint( const Rect &cUpdateRect )
{
	/*design colors*/
	os::Color32_s sCurrentColor = BlendColours(get_default_color(os::COL_SHINE),  get_default_color(os::COL_NORMAL_WND_BORDER), 0.5f);
	SetFgColor( Tint( get_default_color( os::COL_SHADOW ), 0.5f ) );
	os::Color32_s sBottomColor = BlendColours(get_default_color(os::COL_SHADOW), get_default_color(os::COL_NORMAL_WND_BORDER), 0.5f);
	
	os::Color32_s sColorStep = os::Color32_s( ( sCurrentColor.red-sBottomColor.red ) / 30, 
											( sCurrentColor.green-sBottomColor.green ) / 30, 
											( sCurrentColor.blue-sBottomColor.blue ) / 30, 0 );
	
	/*draw*/
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

/*************************************************
* Description: Returns the size of the plugin
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
Point DockWallpaperChanger::GetPreferredSize( bool bLargest ) const
{
	return m_pcIcon->GetSize();
}

/*************************************************
* Description: Handles messages passed to it :)
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
void DockWallpaperChanger::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_WALLPAPERCHANGER_ABOUT:
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
			ShowPrefs();
			break;
		}
		
		case M_PREFS_CANCEL:
		{
			pcPrefsWin->Close();
			pcPrefsWin = NULL;
			break;
		}
		
		case M_PREFS_SEND_TO_PARENT:
		{
			pcMessage->FindBool("random",&bRandom);
			pcMessage->FindInt32("time",&nTime);
			pcMessage->FindString("currentimage",&cCurrentImage);
			SaveSettings();
			LoadSettings();
			UpdateImage();
			pcPrefsWin->Close();
			pcPrefsWin = NULL;	
			break;
		}
		
		case M_CHANGE_FILE:
		{
			ChangeFile();
			break;
		}		
	}
}

/*************************************************
* Description: Handles mouse movements
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
void DockWallpaperChanger::MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData )
{
	if (nCode == MOUSE_INSIDE)
	{
		if (m_bHover)
		{
			Bitmap* pcBitmap = m_pcIcon->LockBitmap();
			m_bHover = false;
			pcPaint = new BitmapImage(pcBitmap->LockRaster(),IPoint((int)m_pcIcon->GetSize().x,(int)m_pcIcon->GetSize().y),pcBitmap->GetColorSpace());;
			pcPaint->ApplyFilter(BitmapImage::F_HIGHLIGHT);
			pcBitmap->UnlockRaster();
			pcPaint->UnlockBitmap();
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
			m_bCanDrag = false;
			Paint(GetBounds());
		}
	}

	os::View::MouseMove( cNewPos, nCode, nButtons, pcData );
}

/*************************************************
* Description: Handles mouseup events
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
void DockWallpaperChanger::MouseUp( const os::Point & cPosition, uint32 nButtons, os::Message * pcData )
{
	if( m_bDragging && ( cPosition.y > 30 ) )
	{
		/* Remove ourself from the dock */
		os::Message cMsg( os::DOCK_REMOVE_PLUGIN );
		cMsg.AddPointer( "plugin", this );
		m_pcDock->PostMessage( &cMsg, m_pcDock );
		Paint(GetBounds());
		return;
	}
	m_bDragging = false;
	m_bCanDrag = false;
	os::View::MouseUp( cPosition, nButtons, pcData );
}

/*************************************************
* Description: Handles mouse down events
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
void DockWallpaperChanger::MouseDown( const os::Point& cPosition, uint32 nButtons )
{
	MakeFocus( true );
		
	if( nButtons == 0x02) {
		pcContextMenu->Open(ConvertToScreen(cPosition));
	}
	else
		m_bCanDrag = true;
		
	os::View::MouseDown( cPosition, nButtons );
}

/*************************************************
* Description: Lets show the prefs dialog
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
void DockWallpaperChanger::ShowPrefs()
{
	Message* pcMessage = new Message();
	pcMessage->AddBool("random",bRandom);
	pcMessage->AddInt32("time",nTime);
	pcMessage->AddString("currentimage",cCurrentImage);
	
	if (pcPrefsWin == NULL)
	{
		pcPrefsWin = new WallpaperChangerSettings(this,pcMessage);
		pcPrefsWin->CenterInScreen();
		pcPrefsWin->Show();
		pcPrefsWin->MakeFocus();
	}
	
	else
		pcPrefsWin->MakeFocus();
}

/*************************************************
* Description: Load the settings
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
void DockWallpaperChanger::LoadSettings()
{
	/* Load settings */ 
	os::String zPath = getenv( "HOME" );
	zPath += "/Settings/WallpaperChanger/Settings";

	try
	{
		os::File* pcFile = new os::File(zPath,O_CREAT);
		os::Settings* pcSettings = new os::Settings(pcFile);
		pcSettings->Load();

		bRandom = pcSettings->GetBool("random",bRandom);
		if (pcSettings->FindInt32("time",&nTime)!=0)
			nTime = 1;
			
		ConvertTime();
		if (pcSettings->FindString("currentimage",&cCurrentImage)!=0)
			cCurrentImage = "";
		delete( pcFile);
	}
	catch(...)
	{
	}
}

/*************************************************
* Description: Converts nTime to how many minutes needed
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
void DockWallpaperChanger::ConvertTime()
{
	bool bFlag = true;
	
	pcChangeLooper->Lock();
	pcChangeLooper->RemoveTimer(pcChangeLooper,123);
	
	switch (nTime)
	{
		case 0:
			bFlag = false;
			break;
			
		case 1:
			nTimerTime = 60000000;  //1 min
			break;
		
		case 2:
			nTimerTime = 60000000*3; // 3 mins
			break;
			
		case 3:
			nTimerTime = 60000000*5;  //5 min
			break;	
			
		case 4:
			nTimerTime = 60000000*10;  //10 min
			break;
			
		case 5:
			nTimerTime = 60000000*30;  //30 min
			break;
			
		case 6:
			nTimerTime = 60000000*60;  //1 hour
			break;
						
		default:
			nTimerTime = 60000000;
			break;	
	}
	if (bFlag)
		pcChangeLooper->AddTimer(pcChangeLooper,123,nTimerTime,false);
	pcChangeLooper->Unlock();
}	

/*************************************************
* Description: Saves the settings
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
void DockWallpaperChanger::SaveSettings()
{
		/* Save settings */
	os::String zPath = getenv( "HOME" );
	zPath += "/Settings/WallpaperChanger";

	try
	{		
		mkdir( zPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO );
		zPath += "/Settings";
		os::File cFile( zPath, O_RDWR | O_CREAT );	
		os::Settings* pcSettings = new os::Settings(new File(zPath));
	
		
		pcSettings->AddBool("random",bRandom);
		pcSettings->AddInt32("time",nTime);
		pcSettings->AddString("currentimage",cCurrentImage);
		
		pcSettings->Save();
		delete( pcSettings );
	}
	catch(...)
	{
	}
}

/*************************************************
* Description: Change to a specific file
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
void DockWallpaperChanger::ChangeFile()
{
	String cDir = String("/system/resources/wallpapers");
	Directory* pcDir = new Directory(cDir);
	String cFile;
	String cWholeFile;
	pcDir->Rewind();
	int nEntry=0;

	if (cCurrentImage.c_str() != "")
	{
		nEntry=1;
		do
		{
			nEntry = pcDir->GetNextEntry(&cFile);
			if (cFile == cCurrentImage)
			{
				nEntry = pcDir->GetNextEntry(&cFile);
			
				if (nEntry != 0)
				{
					break;
				}
			}
		} while (nEntry != 0);
	}
	
	if (nEntry == 0)
	{
		pcDir->Rewind();
		for (int i=0;i<=2;i++)
		{
			pcDir->GetNextEntry(&cFile);
		}
	}
	
	cCurrentImage = cFile;
	delete pcDir;
	
	UpdateImage();
}

/*************************************************
* Description: Call the registrar to update the desktop image
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
void DockWallpaperChanger::UpdateImage()
{
	String cDir = String("/system/resources/wallpapers");
	String cWholeFile = cDir + "/" + cCurrentImage;
	
	try
	{
		os::Event cEvent;

		if( cEvent.SetToRemote( "os/Desktop/SetBackgroundImage",0 ) == 0 )
		{
			os::Message cMsg;
			cMsg.AddString( "background_image", cCurrentImage );
			cEvent.PostEvent( &cMsg, this, 0 );
		}
	} catch( ... )
	{
	}
}

class WallpaperChangerPlugin : public DockPlugin
{
public:
	WallpaperChangerPlugin(){}

	status_t Initialize()
	{
		m_pcView = new DockWallpaperChanger(GetPath(),GetApp());
		AddView(m_pcView);
		return 0;
	}

	void Delete()
	{
		RemoveView(m_pcView);
	}


	String GetIdentifier()
	{
		return( "WallpaperChanger" );
	}
	
private:
	DockWallpaperChanger* m_pcView;
};

/*************************************************
* Description: Initialize the plugin
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
extern "C"
{
DockPlugin* init_dock_plugin()
{
	return( new WallpaperChangerPlugin() );
}
}

