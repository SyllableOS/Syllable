#ifndef MAIN_H
#define MAIN_H

#define _GNU_SOURCE

#include "include.h"
#include "iconview.h"
#include "loadbitmap.h"
#include "messages.h"
#include "properties.h"
#include "imageitem.h"
#include "bitmapscale.h"
#include "iconmenu.h"
#include "iconmenu_prop.h"
using namespace os;

typedef vector <std::string> t_Icon;

struct mounted_drives{
	char zMenu[1024];  
	char zSize[64];
    char zUsed[64];   
    char zAvail[64];
    char vol_name[256];
    char zType[64];
    char zPer[1024];
    }drives;

#define M_CONFIG_CHANGE 20000019

char pzConfigFile[1024];
char pzConfigDir[1024];
char pzImageDir[1024];
char pzIconDir[1024];
int32   nSizeImage;


class BitmapView : public View
{
public:
    BitmapView( const Rect& cFrame);
    ~BitmapView();

    Icon*		FindIcon( const Point& cPos );
    void 		Erase( const Rect& cFrame );

    virtual void	MouseDown( const Point& cPosition, uint32 nButtons );
    virtual void	MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData );
    virtual void	MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData );
    virtual void	Paint( const Rect& cUpdateRect);
    void ReadPrefs();
    Color32_s zBgColor, zFgColor;
   
private:
    Point		     m_cLastPos;
    Point		     m_cDragStartPos;
    Rect		     m_cSelRect;
    bigtime_t	     m_nHitTime;
    Bitmap*	     	 m_pcBitmap;
    std::vector<Icon*> m_cIcons;
    bool		     m_bCanDrag;
    bool		     m_bSelRectActive;
    Menu*	     	 pcMountMenu;
    Menu*            pcLineIcons;
    Menu*            pcMainMenu;
    void             HandleMessage(Message* pcMessage);
    mounted_drives	 m_drives;
    PropWin* 		 pcProp;
    string 			 pzSyllableVer;
    string 			 zDImage;
    bool 			 bShow;
    IconMenu* 		 pcIconMenu;
    string 			 cIconName;
    t_Icon 			 IconList();
    void 			 SetIcons();
    
};

bool get_login( std::string* pcName, std::string* pcPassword );

class BitmapWindow : public Window
{
public:
	BitmapWindow();
	//~BitmapWindow();
private:
	virtual void HandleMessage(Message* pcMessage);
	NodeMonitor*     pcConfigChange;
	BitmapView*		 pcBitmapView;
};

#endif

























