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

#ifndef _SPLASH_DISPLAY_H
#define _SPLASH_DISPLAY_H

#include <gui/window.h>
#include <gui/view.h>
#include <gui/image.h>
#include <util/string.h>
#include <gui/progressbar.h>
#include <gui/font.h>

using namespace os;

class SplashView : public View
{
public:
	SplashView(os::BitmapImage*,const os::String& cText, bool bEnableProgress, float vProgress);
	~SplashView();

	virtual void Paint(const Rect& cUpdateRect);
	Point GetPreferredSize(bool) const;
	void SetText(const os::String& cText);
	void SetImage(os::BitmapImage* pcImage);
	void SetTextColor(const os::Color32_s& sColor);
	void SetProgress(float vProgress);
	void SetEnableProgressBar(bool bEnable);
	void SetEnable(bool bEnable);
	void SetFont(os::Font*);
private:
	BitmapImage* m_pcImage;
	BitmapImage* m_pcGrayImage;
	
	os::String m_cText;

	Color32_s m_sTextColor;
	Color32_s m_sBgColor;
	
	os::ProgressBar* m_pcProgressBar;
	bool m_bEnableProgress;
	float m_vProgress;
	int m_nTextStart;
	int m_nTextEnd;
	bool m_bEnabled;
};

class Splash : public Window
{
public:
	Splash(os::BitmapImage* pcImage=NULL,const os::String& cText="", bool bEnableProgress=true, float vProgress=0.0f);
	virtual bool OkToQuit();
	
	void SetText(const os::String& cText);
	void SetImage(os::BitmapImage* pcImage);
	void SetTextColor(const Color32_s& sColor);
	void SetBgColor(const os::Color32_s& sColor);
	void SetFlags(uint nFlags);
	void SetEnableProgressBar(bool bEnable);
	void SetProgress(float vProgress);
	void SetEnable(bool bEnable);
	void SetFont(os::Font*);
	
	void Go();
/*	void Quit(os::Window*); */
	void Quit();
private:
	SplashView* m_pcView;
};
#endif

