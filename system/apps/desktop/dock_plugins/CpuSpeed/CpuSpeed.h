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
	virtual void MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData );
	virtual void MouseUp( const os::Point & cPosition, uint32 nButton, os::Message * pcData );
	virtual void MouseDown( const os::Point& cPosition, uint32 nButtons );
private:
	void DisplayAbout();
	
	void LoadCpuSpeeds();
	void SetSpeed(int nIndex);
	int  GetSpeed();
	
	DropdownMenu* pcCpuSpeedDrop;
	
	os::DockPlugin* m_pcPlugin;
	BitmapImage* m_pcIcon;
	os::BitmapImage* m_pcDragIcon;
	os::Looper* m_pcDock;  

	bool m_bCanDrag;
	bool m_bDragging;
  
	int32 nDefault;

	os::Point m_cPos;

	os::String m_cDeviceFileName;
	int m_nFd;
	int m_nLastIndex;
	std::vector< std::pair<int,String> > m_cFreqs;	
};



