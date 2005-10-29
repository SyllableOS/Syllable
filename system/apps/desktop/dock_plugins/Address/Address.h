#include "dockplugin.h"

#include <gui/dropdownmenu.h>
#include <gui/image.h>
#include <util/resources.h>
#include <storage/file.h>

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
	Address(os::Path cPath, os::Looper* pcDock);
	~Address();
	
	os::String GetIdentifier() ;
	Point GetPreferredSize( bool bLargest ) const;
	os::Path GetPath() { return( m_cPath ); }
	
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
	
	Path m_cPath;
	BitmapImage* m_pcIcon;
	os::Looper* m_pcDock;  
	os::ResStream* pcStream;
	bool bExportHelpFile;
	int32 nDefault;
	
	std::vector< std::pair<String,String> > cSites;
	std::vector< String > m_cBuffer;
	
	SettingsWindow* pcSettingsWindow;
};


class AddressDockPlugin : public DockPlugin
{
public:
	AddressDockPlugin();

	status_t	Initialize();
	void 		Delete();
	String		GetIdentifier();
private:
	Address* m_pcView;
};













