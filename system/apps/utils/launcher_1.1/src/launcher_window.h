/*
    Launcher - A program launcher for AtheOS
    Copyright (C) 2002 Andrew Kennan (bewilder@stresscow.org)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "../include/launcher_plugin.h"

#include <util/application.h>
#include <gui/desktop.h>

#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/frameview.h>
#include <gui/button.h>
#include <gui/bitmap.h>
#include <gui/gfxtypes.h>
#include <gui/menu.h>
#include <gui/listview.h>
#include <gui/checkbox.h>
#include <gui/dropdownmenu.h>
#include <gui/tabview.h>
#include <gui/stringview.h>
#include <gui/requesters.h>

#include <storage/directory.h>

#include <vector>

#define APP_NAME "Launcher"
#define APP_VERSION "1.1.2 (June 2002)"
#define APP_AUTHOR "Andrew Kennan (bewilder@stresscow.org)"
#define APP_DESC   "An Application Launcher + More for AtheOS

See /Applications/Launcher/README
for general information

Developers should read
/Applications/Launcher/Developers

This program is licenced under the GNU GPL.
For more information see 
/Applications/Launcher/COPYING"


#define AB_DIR_UP    1
#define AB_DIR_DOWN  2
#define AB_DIR_LEFT  3
#define AB_DIR_RIGHT 4

enum {
  LW_EVENT_PREFS = 250,
  LW_EVENT_QUIT,
  LW_EVENT_CLOSE_WINDOW,
  LW_EVENT_ZOOM,
  LW_EVENT_ADD_PLUGIN,
  LW_EVENT_ADD_WINDOW,
  LW_EVENT_ABOUT,
  LW_EVENT_POSITION_WINDOW,
  
  LW_PREFS_UPDATE_POSITION,
  LW_PREFS_PLUGIN_SELECTED,  
  LW_CREATE_PLUGINS,
  LW_PREFS_WINDOW_CLOSED,
  LW_PLUGIN_ABOUT_CLOSED,
  LW_CHECK_MOUSE
};
  
using namespace os;

class ArrowButton;
class PrefWindow;
class PluginPrefWindow;
class LauncherWindow;
class WindowPositioner;

class ArrowButton : public ImageButton
{

  public:
    ArrowButton( int nDirection, const char *pzName, Message *pcMessage );
    ~ArrowButton( );
    void SetArrow( int nDirection );
    void MouseDown( const Point &cPosition, uint32 nButtons );
    void MouseUp( const Point &cPosition, uint32 nButtons, Message *pcData );
    void AttachedToWindow( void );

  private:
    int    m_nDirection;
    Menu   *m_pcMenu;
    Menu   *m_pcPluginMenu;
};


class PrefWindow : public Window
{

  public:
    PrefWindow( LauncherWindow *pcParent );
    ~PrefWindow( );
    void HandleMessage( Message *pcMessage );
    void Close( void );

  private:
    LauncherWindow *m_pcParent;
    DropdownMenu *m_pcDDVertPos;
    DropdownMenu *m_pcDDHorizPos;
    DropdownMenu *m_pcDDAlignPos;
    CheckBox *m_pcCBAutoHide;
    CheckBox *m_pcCBAutoShow;
    ListView *m_pcPluginList;
    StringView *m_pcSVPluginName;
    StringView *m_pcSVPluginVersion;
    StringView *m_pcSVPluginAuthor;
    StringView *m_pcSVPluginInfo;
    LayoutView *m_pcLayout;
    WindowPositioner *m_pcPosition;
            
    int32      m_nVPos;
    int32      m_nHPos;
    int32      m_nAlign;
    bool       m_bAutoHide;
    bool       m_bAutoShow;

    void AdjustWindowSize( Point cSize, bool bCentre = true );
    void AddPlugins( void );
    void UpdatePosition( void );
    void UpdatePluginList( void );
    void ShowPluginInfo( void );
};


class PluginPrefWindow : public Window
{
	public:
		PluginPrefWindow( LauncherPlugin *pcPlugin, Window *pcParent );
		~PluginPrefWindow( );
		void HandleMessage( Message *pcMessage );
		
	private:
		View *m_pcPluginPrefs;
		CheckBox *m_pcCBVisible;
		LauncherPlugin *m_pcPlugin;
		Window *m_pcParent;
};


class LauncherWindow : public Window
{

  public:
    LauncherWindow( Message *pcPrefs, bool bOpenPrefsWindow = false );
    ~LauncherWindow( );
    void HandleMessage( Message *pcMessage );
    void TimerTick( int nId );
    void SetPrefs( Message *pcMessage, bool bDeletePlugins = true );
    Message *GetPrefs( void );
    bool OkToQuit( void );
    int32 GetHPos( void ) { return m_nHPos; };
    int32 GetVPos( void ) { return m_nVPos; };
    int32 GetAlign( void ) { return m_nAlign; };
        
  private:
    LayoutView *m_pcLayout;
    LayoutView *m_pcHLayout;
    LayoutView *m_pcVLayout;
    LayoutNode *m_pcRoot;
    HLayoutNode *m_pcHRoot;
    VLayoutNode *m_pcVRoot;
    int32      m_nWindowId;
    int32      m_nVPos;
    int32      m_nHPos;
    int32      m_nAlign;
    bool       m_bVisible;
    bool       m_bAutoHide;
    bool       m_bAutoShow;
    bool       m_bZoomLocked;

    ArrowButton  *m_pcArrow;

    PrefWindow     *m_pcPluginListWindow;

    std::vector<LauncherPlugin *>m_apcPlugin;

    void InitPlugins( Message *pcPrefs );
    void DeletePlugins( void );
    void PositionWindow( bool bToggleVisibility = false );
    void Log( string zFuncName, string zMessage );
    void MovePluginUp( LauncherMessage *pcMessage );
    void MovePluginDown( LauncherMessage *pcMessage );
    void DeletePlugin( LauncherMessage *pcMessage );
    void InsertPlugins( LauncherMessage *pcMessage );
    void OpenPluginAbout( LauncherPlugin *pcPlugin = 0);
    void OpenPluginPrefs( LauncherPlugin *pcPlugin );
    void BroadcastMessage( LauncherMessage *pcMessage );
};


class WindowPositioner : public Control
{
	public:
		WindowPositioner( int32 nVPos, int32 nHPos, int32 nAlign );
		~WindowPositioner( );
		void MouseDown( const Point &cPosition, uint32 nButtons );
		Point GetPreferredSize( bool bLargest ) const;
		void Paint(  const Rect &cUpdateRect );
		void EnableStatusChanged( bool bIsEnabled ) { };
		int32 GetVPos( void ) { return m_nVPos; };
		int32 GetHPos( void ) { return m_nHPos; };
		int32 GetAlign( void ) { return m_nAlign; };

	private:
		int32 m_nVPos;
		int32 m_nHPos;
		int32 m_nAlign;
};




