#include "ImageItem.h"

#include <stdio.h>

Bitmap* s_pcMenuBitmap = NULL;



uint8 nSubMenuArrowData[]={
                              0x00, 0x00, 0x00, 0x00,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0x00, 0x00, 0x00, 0x00,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff
                          };


ImageItem::ImageItem( Menu* pcMenu, Message* pcMsg, Bitmap* bmap, float nNumber )
        :MenuItem(pcMenu, pcMsg)
{
    nDivNum = nNumber;
    if( s_pcMenuBitmap == NULL )
    {
        Rect cSubMenuBitmapRect;

        cSubMenuBitmapRect.left = 0;
        cSubMenuBitmapRect.top = 0;
        cSubMenuBitmapRect.right = SUB_MENU_ARROW_W;
        cSubMenuBitmapRect.bottom = SUB_MENU_ARROW_H;

        s_pcMenuBitmap = new Bitmap( cSubMenuBitmapRect.Width(), cSubMenuBitmapRect.Height(), CS_RGBA32 , Bitmap::SHARE_FRAMEBUFFER);
        memcpy( s_pcMenuBitmap->LockRaster(), nSubMenuArrowData, (unsigned int) ( cSubMenuBitmapRect.Width() * cSubMenuBitmapRect.Height() * 4 ) );
    }

    Init();
    SetBitmap(bmap);
}

ImageItem::ImageItem( const char* pzLabel, Message* pcMsg, const char *shortcut, Bitmap *bmap, float nNumber )
        :MenuItem(pzLabel, pcMsg)
{

    nDivNum = nNumber;
    if( s_pcMenuBitmap == NULL )
    {
        Rect cSubMenuBitmapRect;

        cSubMenuBitmapRect.left = 0;
        cSubMenuBitmapRect.top = 0;
        cSubMenuBitmapRect.right = SUB_MENU_ARROW_W;
        cSubMenuBitmapRect.bottom = SUB_MENU_ARROW_H;

        s_pcMenuBitmap = new Bitmap( cSubMenuBitmapRect.Width(), cSubMenuBitmapRect.Height(), CS_RGBA32 , Bitmap::SHARE_FRAMEBUFFER);
        memcpy( s_pcMenuBitmap->LockRaster(), nSubMenuArrowData, (unsigned int) ( cSubMenuBitmapRect.Width() * cSubMenuBitmapRect.Height() * 4 ) );
    }
    Init();
    SetBitmap(bmap);
    SetShortcut(shortcut);
}

void ImageItem::Init()
{
    m_Highlighted = false;
    m_IconWidth = 18;
    m_IconHeight = 18;
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

    if(shortcut)
    {
        m_Shortcut = new char[strlen(shortcut)+1];
        if(m_Shortcut)
            strcpy(m_Shortcut, shortcut);
    }
}

Point ImageItem::GetContentSize()
{
    Menu *m = GetSuperMenu();
    if(!m || GetLabel().empty())
        return Point(0, 0);
    font_height fh;
    m->GetFontHeight(&fh);
    float x = m->GetStringWidth(GetLabel()) + m_IconWidth + 5 + 2;
    if(m_Shortcut)
        x+=m->GetStringWidth(m_Shortcut) + 15;
    float y = fh.ascender + fh.descender + 4;
    y = y > m_IconHeight ? y : m_IconHeight;

    return Point(x + 10, y);
}

void ImageItem::Draw()
{
    Menu *m = GetSuperMenu();
    if(!m)
        return;

    const char *label = GetLabel().c_str();

    Rect bounds = GetFrame();

    m->SetDrawingMode(DM_OVER);

    m->SetEraseColor(get_default_color(m_Highlighted ? COL_SEL_MENU_BACKGROUND : COL_MENU_BACKGROUND));
    m->SetBgColor(get_default_color(m_Highlighted ? COL_SEL_MENU_BACKGROUND : COL_MENU_BACKGROUND));
    m->SetFgColor(get_default_color(m_Highlighted ? COL_SEL_MENU_TEXT : COL_MENU_TEXT));

    Rect textrect(bounds);
    m->EraseRect(textrect);
    textrect.left += m_IconWidth + 3;

    font_height fh;
    m->GetFontHeight(&fh);

    float x = textrect.left + 2;
    float y = textrect.top + 2 + textrect.Height()/2 - (fh.ascender + fh.descender)/2 + fh.ascender;

    m->MovePenTo(x, y);
    if(label)
        m->DrawString(label);

    if(m_Shortcut)
    {
        x = textrect.right - 2 - m->GetStringWidth(m_Shortcut);
        m->MovePenTo(x, y);
        m->DrawString(m_Shortcut);
    }

    if(m_HasIcon && m_Bitmap)
    {
        m->SetEraseColor(get_default_color(COL_MENU_BACKGROUND));
        Rect bmrect(m_Bitmap->GetBounds());
        bounds.right = bounds.left + m_IconWidth;

        bmrect.left = bounds.left + bounds.Width()/2 - bmrect.Width()/2;
        bmrect.right += bmrect.left;
        bmrect.top = bounds.top + bounds.Height()/2 - bmrect.Height()/2;
        bmrect.bottom += bmrect.top;

        m->SetDrawingMode(DM_BLEND);
        m->DrawBitmap(m_Bitmap, m_Bitmap->GetBounds(), bmrect);
    }

    if (GetSubMenu())
    {

        Rect cSourceRect = s_pcMenuBitmap->GetBounds();
        Rect cTargetRect;

        if (m_HasIcon)
            cTargetRect = cSourceRect.Bounds() + Point( m->GetBounds().right - SUB_MENU_ARROW_W - m_Bitmap->GetBounds().Width() + 9, m->GetBounds().top + ( m->GetBounds().Height() / nDivNum) - 4.0f);



        else
            cTargetRect = cSourceRect.Bounds() + Point( m->GetBounds().right - SUB_MENU_ARROW_W, m->GetBounds().top + ( m->GetBounds().Height() / nDivNum) - 4.0f);


        m->SetDrawingMode( DM_OVER );
        m->DrawBitmap(s_pcMenuBitmap,cSourceRect,cTargetRect);
        m->SetDrawingMode( DM_COPY );

        /*if ( GetSubMenu() != NULL) //was commented
        {

        m->SetDrawingMode( DM_OVER );
        m->DrawBitmap( s_pcMenuBitmap, cSourceRect, cTargetRect );
        m->SetDrawingMode( DM_COPY );
        }*/
    }
}

void ImageItem::DrawContent()
{}

void ImageItem::SetHighlighted(bool bHighlight)
{
    m_Highlighted = bHighlight;
    MenuItem::SetHighlighted(bHighlight);
}

void ImageItem::SetEnable()
{
    //this->SetBgColor(get_default_color(COL_NORMAL));
}











































