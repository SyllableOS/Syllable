#include <appserver/dockplugin.h>
#include <gui/view.h>
#include <gui/image.h>
#include <gui/point.h>
#include <storage/path.h>
#include <util/looper.h>
#include <gui/font.h>
#include <storage/file.h>
#include <util/resources.h>
#include <gui/desktop.h>
#include <gui/menu.h>


#include <cstdlib>
#include <cstdio>

#define PLUGIN_NAME "Switcher"

using namespace os;

class Switcher : public View
{
public:
	Switcher(os::Path cPath, os::Looper* pcDock);
	~Switcher();
	
	os::String	GetIdentifier();
	os::Path	GetPath() {return m_cPath;}
	void 		Paint(const Rect&);
	void		MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData );
    void		MouseDown( const Point& cPosition, uint32 nButtons );
	void		MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData );
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
	os::Menu* m_pcContextMenu;
	os::Path m_cPath;
};

	
Switcher::Switcher(os::Path cPath, os::Looper* pcDock) : View(Rect(0,0,1,1),"switcher")
{
	os::Desktop cDesktop( os::Desktop::ACTIVE_DESKTOP );
	m_nDesktop = cDesktop.GetDesktop();
	m_nFirstVisibleDesktop = (m_nDesktop/4)*4;    /* "greatest multiple of 4 <= current desktop" */

	m_cPath = cPath;
	
	m_pcContextMenu = new Menu(Rect(0,0,1,1),"context_menu",ITEMS_IN_COLUMN);
	m_pcContextMenu->AddItem("Minimize All",NULL);
	
	LoadImages();
}

Switcher::~Switcher()
{
	delete( pcBackImage );
	delete( pcBackGreyImage );
	delete( pcForwardImage );
	delete( pcForwardGreyImage );
	delete( m_pcContextMenu );
}

os::Point Switcher::GetPreferredSize(bool bSize) const
{
	os::Point cPoint(64,GetParent()->GetBounds().Height());
	return cPoint;
}

void Switcher::Paint(const Rect& cRect)
{
	FillRect(GetBounds(),get_default_color(COL_NORMAL));
	FillRect(Rect(13,0,GetBounds().Width()-12,GetBounds().Height()),get_default_color(COL_SEL_WND_BORDER));
	
	SetDrawingMode(DM_BLEND);
	
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
	
	SetFgColor(0,0,0,0xff);
	DrawLine(os::Point(13,0),os::Point(13,GetBounds().Height()));
	DrawLine(os::Point(GetBounds().Width()-12,0),os::Point(GetBounds().Width()-12,GetBounds().Height()));
	DrawLine(os::Point(13,0),os::Point(GetBounds().Width()-12,0));
	DrawLine(os::Point(13,GetBounds().Height()),os::Point(GetBounds().Width()-12,GetBounds().Height()));
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
}

void Switcher::MouseDown( const Point& cPosition, uint32 nButtons )
{
}


void Switcher::MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData )
{
	if (cPosition.x <= pcBackImage->GetSize().x)
		Backward();
	
	else if (cPosition.x >= 14 && cPosition.x <=32 && cPosition.y  < 13)
		if(nButtons == 2)
			m_pcContextMenu->Open(ConvertToScreen(cPosition));
		else
			SwitchDesktop(m_nFirstVisibleDesktop);
	
	else if (cPosition.x >=32 && cPosition.x <= 51 && cPosition.y < 13)
		if (nButtons == 2)
			m_pcContextMenu->Open(ConvertToScreen(cPosition));
		else
			SwitchDesktop(m_nFirstVisibleDesktop+1);
	
	else if (cPosition.x >= 14 && cPosition.x <=32  && cPosition.y >= 14)
		if (nButtons == 2)
			m_pcContextMenu->Open(ConvertToScreen(cPosition));
		else
			SwitchDesktop(m_nFirstVisibleDesktop+2);
	
	else if(cPosition.x >=32 && cPosition.x <= 51 && cPosition.y >=14)
		if (nButtons == 2)
			m_pcContextMenu->Open(ConvertToScreen(cPosition));
		else
			SwitchDesktop(m_nFirstVisibleDesktop+3);
	
	else if (cPosition.x >= 53 && cPosition.x < 61)
		Forward();
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
			m_pcView = new Switcher(GetPath(),GetApp());
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







