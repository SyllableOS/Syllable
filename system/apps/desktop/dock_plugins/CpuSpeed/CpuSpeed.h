#include <atheos/filesystem.h>
#include <gui/dropdownmenu.h>
#include <gui/image.h>
#include <util/resources.h>
#include <util/looper.h>

#include <appserver/dockplugin.h>

using namespace os;


class CpuSpeedDrop : public DropdownMenu
{
public:	
	CpuSpeedDrop(View*);

private:
	View* pcParentView;
};
	
class CpuSpeed : public View
{
public:
	CpuSpeed(os::DockPlugin* pcPlugin, os::Looper* pcDock);
	~CpuSpeed();
	
	Point GetPreferredSize( bool bLargest ) const;
		
	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	virtual void HandleMessage(Message* pcMessage);
private:
	void DisplayAbout();
	
	void LoadCpuSpeeds();
	void SetSpeed(int nIndex);
	int  GetSpeed();
	
	DropdownMenu* pcCpuSpeedDrop;
	
	os::DockPlugin* m_pcPlugin;
	BitmapImage* m_pcIcon;
	os::Looper* m_pcDock;  
	int32 nDefault;

	os::String m_cDeviceFileName;
	int m_nFd;
	int m_nLastIndex;
	std::vector< std::pair<int,String> > m_cFreqs;	
};



