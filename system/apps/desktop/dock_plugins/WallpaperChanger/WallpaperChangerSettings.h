#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/listview.h>
#include <gui/checkbox.h>
#include <gui/button.h>
#include <gui/stringview.h>
#include <gui/dropdownmenu.h>
#include <storage/nodemonitor.h>
#include <storage/directory.h>
#include <storage/fsnode.h>
#include <gui/imageview.h>
#include <util/string.h>
#include <storage/file.h>
#include <util/settings.h>
#include <gui/view.h>
#include <util/looper.h>

using namespace os;

class WallpaperChangerSettings : public Window
{
public:
	
	enum
	{
		M_APPLY,
		M_DEFAULT,
		M_CANCEL,
		M_CHANGE_IMAGE
	};
	
	WallpaperChangerSettings(View*,Message*);
	
	void HandleMessage(Message*);
	bool OkToQuit();
	
	void LoadSettings();
	void SaveSettings();
	void Init();
	void Layout();
	void LoadTimeSettings();
	void LoadDirectoryList();	
private:
	bool bRandom;
	String cCurrentImage;
	int32 nTime;
	
	Button* pcDefault;
	Button* pcSave;
	Button* pcCancel;
	
	LayoutView* pcLayoutView;
	ListView* pcDirectoryList;
	ImageView* pcImageView;
	CheckBox* pcRandomCheck;
	StringView* pcTimeString;
	DropdownMenu* pcTimeDrop;
	CheckBox* pcRandom;
	NodeMonitor* pcMonitor;
	Button* pcApplyButton;
	Button* pcCancelButton;
	Button* pcDefaultButton;
	
	BitmapImage* pcImage;
	Bitmap* pcLargeBitmap;
	
	View* pcParentView;
	Looper* pcParentLooper;
	Message* pcParentMessage;
};	








