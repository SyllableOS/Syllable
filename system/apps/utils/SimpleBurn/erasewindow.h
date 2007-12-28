#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/button.h>
#include <gui/image.h>
#include <gui/imagebutton.h>
#include <gui/dropdownmenu.h>
#include <gui/stringview.h>
#include <gui/requesters.h>
#include <util/resources.h>
#include <util/application.h>
#include <util/message.h>

enum
{
	M_ERASE_ERASE,
	M_ERASE_CANCEL,
};

class EraseWindow : public os::Window
{
public:
	EraseWindow( const os::String& cArgv );
	void EraseDisk();
	void DeviceScan();
	void HandleMessage( os::Message* );
private:
	bool OkToQuit();
	os::BitmapImage* LoadImageFromResource( os::String zResource );
	os::BitmapImage* m_pcIcon;
	os::VLayoutNode* m_pcRoot;
	os::VLayoutNode* m_pcVRoot;
	os::HLayoutNode* m_pcDeviceNode;
	os::StringView* m_pcDeviceString;
	os::DropdownMenu* m_pcDeviceDropdown;
	os::HLayoutNode* m_pcButtonNode;
	os::HLayoutSpacer* m_pcButtonSpacer;
	os::ImageButton* m_pcEraseButton;
	os::String cEraseMode;
};

