#include "imageitem.h"

#include <stdio.h>

ImageItem::ImageItem( Menu* pcMenu, Message* pcMsg, Bitmap* bmap )
	:MenuItem(pcMenu, pcMsg)
{
	Init();
        SetBitmap(bmap);
}

ImageItem::ImageItem( const char* pzLabel, Message* pcMsg, const char *shortcut, Bitmap *bmap )
	:MenuItem(pzLabel, pcMsg)
{
	Init();
	SetBitmap(bmap);
	SetShortcut(shortcut);
}

void ImageItem::Init()
{
	m_Highlighted = false;
	m_IconWidth = 16;
	m_IconHeight = 16;
	m_Enabled = true;
	m_HasIcon = false;
	m_Bitmap = NULL;
	m_Shortcut = NULL;
}

ImageItem::~ImageItem()
{
	if(m_Bitmap)
		delete m_Bitmap;
	if(m_Shortcut)
		delete m_Shortcut;
}

void ImageItem::SetBitmap(Bitmap *bm)
{
	if(m_Bitmap)
		delete m_Bitmap;
	m_Bitmap = bm;

	m_HasIcon = m_Bitmap ? true : false;
}

void ImageItem::SetShortcut(const char *shortcut)
{
	if(m_Shortcut)
		delete m_Shortcut;

	if(shortcut) {
		m_Shortcut = new char[strlen(shortcut)+1];
		if(m_Shortcut)
			strcpy(m_Shortcut, shortcut);
	}
}

Point ImageItem::GetContentSize()
{
	Menu *m = GetSuperMenu();
	if(!m || !GetLabel()) return Point(0, 0);
	font_height fh;
	m->GetFontHeight(&fh);
	float x = m->GetStringWidth(GetLabel()) + m_IconWidth + 5 + 2;
	if(m_Shortcut)
		x+=m->GetStringWidth(m_Shortcut) + 15;
	float y = fh.ascender + fh.descender + 4;
	y = y > m_IconHeight ? y : m_IconHeight;
	return Point(x, y);
}

void ImageItem::Draw()
{
	Menu *m = GetSuperMenu();
	if(!m)
		return;

	const char *label = GetLabel();

	Rect bounds = GetFrame();

	m->SetDrawingMode(DM_OVER);

	m->SetEraseColor(get_default_color(m_Highlighted ? COL_SEL_MENU_BACKGROUND : COL_MENU_BACKGROUND));
	m->SetBgColor(get_default_color(m_Highlighted ? COL_SEL_MENU_BACKGROUND : COL_MENU_BACKGROUND));
	m->SetFgColor(get_default_color(m_Highlighted ? COL_SEL_MENU_TEXT : COL_MENU_TEXT));
        
	Rect textrect(bounds);
	if(m_HasIcon) {
		textrect.left += m_IconWidth + 3;
		m->EraseRect(textrect);
	} else {
		m->EraseRect(textrect);
		textrect.left += m_IconWidth + 3;
	}

	font_height fh;
	m->GetFontHeight(&fh);

	float x = textrect.left + 4;
	float y = textrect.top  + textrect.Height()/2 - (fh.ascender + fh.descender)/2 + fh.ascender;

	m->MovePenTo(x, y);
	if(label) m->DrawString(label);

	if(m_Shortcut) {
		x = textrect.right - 2 - m->GetStringWidth(m_Shortcut);
		m->MovePenTo(x, y);
		m->DrawString(m_Shortcut);
	}

	if(m_HasIcon && m_Bitmap) {
		m->SetEraseColor(get_default_color(COL_MENU_BACKGROUND));
		Rect bmrect(m_Bitmap->GetBounds());
		bounds.right = bounds.left + m_IconWidth;

		if(m_Highlighted)
			m->DrawFrame(bounds, FRAME_THIN);
		else
			m->EraseRect(bounds);

		bmrect.left = bounds.left + bounds.Width()/2 - bmrect.Width()/2;
		bmrect.right += bmrect.left;
		bmrect.top = bounds.top + bounds.Height()/2 - bmrect.Height()/2;
		bmrect.bottom += bmrect.top;

		m->SetDrawingMode(DM_BLEND);
	        m->DrawBitmap(m_Bitmap, m_Bitmap->GetBounds(), bmrect);
	
	        
		if (GetSubMenu()){
		float x = textrect.right - 2 -m->GetStringWidth(">");
		float y = textrect.top + 1 + textrect.Height()/2 - (fh.ascender + fh.descender)/2 + fh.ascender;
		
		if ( m_Highlighted)
			m->SetFgColor(255,255,255);
		
		else  
			m->SetFgColor(get_default_color(COL_SEL_MENU_BACKGROUND));
        
		m->MovePenTo(x, y);
		m->DrawString(">");
		   
		}
	}
}

void ImageItem::DrawContent()
{
}

void ImageItem::Highlight(bool bHighlight)
{
	m_Highlighted = bHighlight;
	MenuItem::Highlight(bHighlight);
}
































