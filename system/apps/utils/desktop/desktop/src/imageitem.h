#ifndef IMAGEITEMH
#define IMAGEITEMH

#include <gui/menu.h>
#include <gui/bitmap.h>
#include <gui/font.h>
#include <gui/image.h>

#define SUB_MENU_ARROW_W 10
#define SUB_MENU_ARROW_H 10




namespace os
{

class ImageItem : public MenuItem
{
public:
    ImageItem( const char* pzLabel, Message* pcMsg, const char *shortcut = NULL, BitmapImage *bmap = NULL, float nNumber=8);
    ImageItem( Menu* pcMenu, Message* pcMsg = NULL, BitmapImage *bmap = NULL, float nNumber=8);
    ~ImageItem();

    virtual void  Draw();
    virtual void  DrawContent();
    virtual void  SetHighlighted( bool bHighlight );
    virtual Point GetContentSize();
    void SetEnable();
    void SetBitmap(BitmapImage *bm);
    BitmapImage *GetBitmap()
    {
        return m_Bitmap;
    }

    void SetShortcut(const char *shortcut);
    const char *GetShortcut()
    {
        return m_Shortcut;
    }

private:

    void Init();

    bool	m_Enabled;
    bool	m_Highlighted;
    bool	m_HasIcon;
    int	m_IconWidth;
    int	m_IconHeight;
    char	*m_Shortcut;
    BitmapImage	*m_Bitmap;
    float nDivNum;
};

}
#endif

















