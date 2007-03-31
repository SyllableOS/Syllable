#include <atheos/filesystem.h>
#include <gui/dropdownmenu.h>
#include <gui/image.h>
#include <util/resources.h>
#include <util/looper.h>
#include <storage/file.h>

#include <appserver/dockplugin.h>
#include "Settings.h"

using namespace os;

class AddressDrop : public DropdownMenu
{
public:	
	AddressDrop(View*);
	
	void KeyUp(const char* pzString, const char* pzRawString, uint32 nRaw);
	void KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );

private:
	View* pcParentView;
};
	
class Address : public View
{
public:
	Address(os::DockPlugin* pcPlugin, os::Looper* pcDock);
	~Address();
	
	Point GetPreferredSize( bool bLargest ) const;
		
	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	virtual void HandleMessage(Message* pcMessage);
private:
	void DisplayAbout();
	void DisplaySettings();
	void DisplayHelp();
	void ExecuteBrowser(const String& cUrl);
	void ExportHelpFile();
	
	void InitDefaultAddresses();
	void LoadAddresses();
	void SaveAddresses();
	void ReloadAddresses();
	
	/*Buffer functions*/
	void AddToBuffer(String);
	void SaveBuffer();
	void ClearBuffer();
	void LoadBuffer();
	
	void LoadSettings();
	void SaveSettings();
	
	DropdownMenu* pcAddressDrop;
	
	os::DockPlugin* m_pcPlugin;
	BitmapImage* m_pcIcon;
	os::Looper* m_pcDock;  
	bool bExportHelpFile;
	int32 nDefault;
	
	std::vector< std::pair<String,String> > cSites;
	std::vector< String > m_cBuffer;
	
	SettingsWindow* pcSettingsWindow;
};






















