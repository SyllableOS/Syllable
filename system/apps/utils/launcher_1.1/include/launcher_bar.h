

#include "launcher_plugin.h"

#include <gui/button.h>
#include <gui/layoutview.h>
#include <gui/filerequester.h>
#include <gui/directoryview.h>
#include <gui/frameview.h>
#include <gui/textview.h>
#include <vector>
#include <sys/types.h>
#include <storage/directory.h>
#include <unistd.h>

enum {
  LB_REMOVE_TIMER = LW_ID_LAST, // Message to remove the timer
  LB_OPEN_SUB_WINDOW,           // Message to open the SubWindow
  LB_BUTTON_CLICKED,            // A button has been clicked in a LauncherView
  LB_CLOSE_SUB_WINDOW           // Message to close the SubWindow
};



using namespace os;

class LauncherView;        // Subclass of LayoutView
class LauncherSubWindow;   // Subclass of Window - shows subdirectories
class LauncherButton;      // Subclass of Button. Launches programs.


class LauncherView : public LayoutView
{
	public:
		LauncherView( string zName, LauncherMessage *pcPrefs );
		~LauncherView( );
		virtual void AttachedToWindow( void );	
//		virtual void MouseDown( const os::Point &, long unsigned int );

	private:
		int32 m_nAlign;
		int32 m_nHPos;
		bool m_bLaunchBrowser;
		string m_zDirectory;
		bool m_bDrawBorder;
		
		std::vector<LauncherButton *>m_pcButton;
		std::vector<string>m_zName;
		LayoutNode *m_pcRoot;
		LayoutSpacer *m_pcSpacer;
		LauncherPlugin *m_pcPlugin;
		
		void CreateButtons( void );
};


class LauncherSubWindow : public Window
{
	public:
		LauncherSubWindow( string zName, LauncherMessage *pcPrefs );
		~LauncherSubWindow( );
		virtual void HandleMessage( Message *pcMessage );
		virtual void TimerTick( int nId );
		
	private:
		int32 m_nAlign;
		int32 m_nVPos;
		int32 m_nHPos;
		int32 m_nWindowPos;
		bool m_bDrawBorder;

		LauncherPlugin *m_pcPlugin;
		LauncherView *m_pcView;
		LauncherSubWindow *m_pcSubWindow;
		Window *m_pcParentWindow;
				
		void OpenSubWindow( LauncherMessage *pcMessage );
		void CloseSubWindow( void );
		
};


class LauncherButton : public ImageButton
{
	public:
		LauncherButton( string zLabel, string zDirectory, LauncherMessage *pcMessage, uint32 nTextPos, bool bDrawBorder, bool bLaunchBrowser );
		~LauncherButton( );
		virtual bool Invoked( Message *pcMessage );
		void MouseUp( const Point &cPosition, uint32 nButtons, Message *pcData );
		
	private:
		string m_zLabel;
		string m_zDirectory;
		string m_zDraggedFile;
		bool m_bDropped;
		bool m_bLaunchBrowser;
};


