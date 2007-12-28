//  Sourcery -:-  (C)opyright 2003-2004 Rick Caudill
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "splash.h"

using namespace os;

/*************************************************************
* Description: SplashView Constructor... SplashView is just an image in a view
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 11:05:09 
*************************************************************/
SplashView::SplashView(os::BitmapImage* pcImage, const os::String& cText, bool bEnableProgress, float vProgress) : View(Rect(0,0,1,1),"",WID_WILL_DRAW)
{
	m_pcImage = pcImage;
	
	if (m_pcImage)
	{
		m_pcGrayImage = new BitmapImage( *( dynamic_cast < BitmapImage * >( m_pcImage ) ) );
		m_pcGrayImage->ApplyFilter( Image::F_GRAY );
	}
	 
	m_cText = cText;
	m_bEnableProgress = bEnableProgress;
	m_vProgress = vProgress;
	m_sTextColor = Color32_s(0,0,0,255);
	m_sBgColor = Color32_s(255,255,255,255);
	m_bEnabled = true;
	
	ResizeTo(GetPreferredSize(false));
	
	
	if (m_pcImage == NULL)	
	{
		m_nTextStart = 4;
		m_nTextEnd = (int)GetFont()->GetSize()+4;
	}
	else
	{
		m_nTextStart = (int)m_pcImage->GetSize().y+4;
		m_nTextEnd = m_nTextStart + (int)GetFont()->GetSize()+4;
	}
	

	if (m_bEnableProgress)
	{
		m_pcProgressBar = new os::ProgressBar(os::Rect(4, GetPreferredSize(false).y-20,GetBounds().Width()-4,GetPreferredSize(false).y-4),"progress");
		m_pcProgressBar->SetProgress(m_vProgress);
		AddChild(m_pcProgressBar);
	}
	
}

/*************************************************************
* Description: SplashView Destructor... deletes the image
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 11:06:10 
*************************************************************/
SplashView::~SplashView()
{
	delete m_pcImage;
	delete m_pcGrayImage;
}

/*************************************************************
* Description: Typical Paint()
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 11:05:49 
*************************************************************/
void SplashView::Paint(const Rect& cUpdateRect)
{
	if (m_bEnabled)
	{
		SetFgColor(m_sBgColor);
		SetBgColor(m_sTextColor);
		SetEraseColor(m_sBgColor);	
	}
	
	else
	{
		SetFgColor(get_default_color(COL_NORMAL));
	}
	
	FillRect(GetBounds());
	
	if (m_pcImage)
	{
		SetDrawingMode(DM_BLEND);
		
		if (!m_bEnabled)
		{
			m_pcGrayImage->Draw(Point(0,0),this);
		}
		else
		{
			m_pcImage->Draw(Point(0,0),this);
		}
		SetDrawingMode(DM_COPY);
	}	
	
	SetDrawingMode(DM_OVER);
	
	if (m_bEnabled)
		SetFgColor(m_sTextColor);
	else
	{
		SetFgColor(get_default_color(COL_SHADOW));
	}
	DrawText(os::Rect(4,m_nTextStart,GetBounds().Width()-4,m_nTextEnd),m_cText);
}

void SplashView::SetImage(os::BitmapImage* pcImage)
{
	m_pcImage = pcImage;
	
	if (pcImage)
	{
		m_pcGrayImage = new BitmapImage( *( dynamic_cast < BitmapImage * >( m_pcImage ) ) );
		m_pcGrayImage->ApplyFilter( Image::F_GRAY );		
	}
	
	Flush();
	Sync();
	Invalidate();
}

void SplashView::SetText(const os::String& cText)
{
	m_cText = cText;
	
	Flush();
	Sync();
	Invalidate();
}

void SplashView::SetTextColor(const Color32& sColor)
{
	m_sTextColor = sColor;
	
	Flush();
	Sync();
	Invalidate();
}

void SplashView::SetEnableProgressBar(bool bEnable)
{
	if (m_bEnableProgress == bEnable && bEnable == true)
		return;
	else
	{
		m_bEnableProgress = bEnable;
		ResizeTo(GetPreferredSize(false));
		
		if (bEnable)
		{
			m_pcProgressBar = new os::ProgressBar(os::Rect(4, GetPreferredSize(false).y-20,GetBounds().Width()-4,GetPreferredSize(false).y-4),"progress");
			m_pcProgressBar->SetProgress(m_vProgress);
			AddChild(m_pcProgressBar);		
		}
		else
		{
			RemoveChild(m_pcProgressBar);	
		} 
	}
}

void SplashView::SetProgress(float vProgress)
{
	m_vProgress = vProgress;
	m_pcProgressBar->SetProgress(vProgress);
}

void SplashView::SetEnable(bool bEnable)
{
	m_bEnabled = bEnable;
	
	Flush();
	Sync();
	Invalidate();
}

void SplashView::SetFont(os::Font* pcFont)
{
	View::SetFont(pcFont);
	Flush();
	Sync();
	Invalidate();
}

os::Point SplashView::GetPreferredSize(bool bLargest) const
{
	os::Point m_pcPoint;
	
	if (m_pcImage == NULL)
	{
		m_pcPoint = os::Point(0,0);
		m_pcPoint.x = 150;
		m_pcPoint.y += (int)GetFont()->GetSize()+4;
	}
	
	else
	{
		m_pcPoint = m_pcImage->GetSize();
		m_pcPoint.y+=20;
		m_pcPoint.y += (int)GetFont()->GetSize();
	}	
		
	if (m_bEnableProgress)
	{
		m_pcPoint.y+=4;
		m_pcPoint.y += 24;
	}
	return m_pcPoint;			
}

/*************************************************************
* Description: Splash Constructor... nothing major
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 11:06:48 
*************************************************************/
Splash::Splash(os::BitmapImage* pcImage, const os::String& cText, bool bEnableProgress, float vProgress) : Window(Rect(0,0,1,1),"title","title",WND_NOT_RESIZABLE | WND_NO_BORDER)
{
	m_pcView = new SplashView(pcImage,cText,bEnableProgress,vProgress);
	AddChild(m_pcView);
	
	ResizeTo(m_pcView->GetPreferredSize(false).x,m_pcView->GetPreferredSize(false).y);
}

/*************************************************************
* Description: Called when the window closes
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 11:07:16 
*************************************************************/
bool Splash::OkToQuit()
{
	PostMessage(M_QUIT,this);
	return true;
}

void Splash::SetText(const os::String& cText)
{
	m_pcView->SetText(cText);
}

void Splash::SetImage(os::BitmapImage* pcImage)
{
	m_pcView->SetImage(pcImage);
}

void Splash::SetTextColor(const os::Color32_s& sColor)
{
	m_pcView->SetTextColor(sColor);
}

void Splash::SetFont(os::Font* pcFont)
{
	m_pcView->SetFont(pcFont);
	
	m_pcView->Flush();
	m_pcView->Sync();
	m_pcView->Invalidate();
}

void Splash::SetBgColor(const os::Color32_s& sColor)
{
	m_pcView->SetBgColor(sColor);
	
	m_pcView->Flush();
	m_pcView->Sync();
	m_pcView->Invalidate();
}

void Splash::SetEnableProgressBar(bool bEnable)
{
	m_pcView->SetEnableProgressBar(bEnable);
	ResizeTo(m_pcView->GetPreferredSize(false));
}

void Splash::SetProgress(float vProgress)
{
	m_pcView->SetProgress(vProgress);
}

void Splash::SetEnable(bool bEnable)
{
	m_pcView->SetEnable(bEnable);
}

void Splash::Go()
{
	CenterInScreen();	
	Show();
	MakeFocus();
}

/* void Splash::Quit(os::Window* pcWindow) */
void Splash::Quit()
{
	Hide();
/*	pcWindow->Show();
	pcWindow->MakeFocus(); */
	OkToQuit();
}

