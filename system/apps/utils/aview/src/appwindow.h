#include <gui/window.h>
#include <gui/scrollbar.h>
#include <util/message.h>
#include <gui/menu.h>
#include <gui/filerequester.h>
#include "loadbitmap.h"
#include "bitmapview.h"
#include "statusbar.h"
#include "mainapp.h"
#include <gui/view.h>
#include <gui/desktop.h>
#include <gui/requesters.h>
#include <gui/checkmenu.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <vector>
#include <util/string.h>
#include <util/application.h>


using namespace os;

class AppWindow : public Window
{
public:
    AppWindow(ImageApp*,const Rect& cFrame, const String&);
    void AddItems();
    void DispatchMessage(Message* pcMsg, Handler* pcHandler);
    virtual void HandleMessage(Message* pcMessage);
    virtual bool OkToQuit( void );

    void Load(const char *cFileName);
    void HandleKeyEvent( int nKeyCode );
    void BuildDirList(const char* pzFPath);

private:
    Rect cWindowBounds;
    Rect cMenuBounds;

    FileRequester* m_pcLoadRequester;
    const char* pzFPath;
    struct stat FPath_buf;
    struct dirent *FDir_entry;
    DIR* FDir_handle;
    std::vector <String> file_list;
    uint32 current_image;
    const char* next_image;

    Window* MainWindow;
    Bitmap* main_bitmap;
    StatusBar* pcStatusBar;
   	Menu* pcMenuBar;
    BitmapView* main_bitmap_view;
    CheckMenu *pcSizeFitAll, *pcSizeFitWindow;
    Rect pcBitmapRect;
    void SetupMenus();
    void SetupStatusBar();
    float winWidth;
    float winHeight;
    ScrollBar* g_pcVScrollBar;
    ScrollBar* g_pcHScrollBar;
    int nKeyCode;
    bool bSetTitle;
    String sFileRequester;
    ImageApp* pcApp;
};











