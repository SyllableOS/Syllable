/*
    launcher_plugin - A plugin interface for the AtheOS Launcher
    Copyright (C) 2002 Andrew Kennan (bewilder@stresscow.org)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _LAUNCHERPLUGIN_H
#define _LAUNCHERPLUGIN_H

#include <gui/desktop.h>
#include <util/message.h>
#include <gui/view.h>
#include <gui/rect.h>
#include <gui/window.h>
#include <gui/requesters.h>
#include <gui/font.h>
#include <storage/fsnode.h>
#include <storage/directory.h>
#include <string>
#include <atheos/image.h>
#include <png.h>
#include <setjmp.h>
#include <translation/translator.h>


#define INIT_FAIL -1


using namespace os;

class LauncherMessage;
class LauncherPlugin;
class ImageButton;


// Some convenience functions
extern "C" {
	
	  // Centers a frame on the screen.
	os::Rect center_frame( const os::Rect &cFrame );
	  // Adjusts the Top-Left of a frame so that it all fits on the screen.
	os::Rect frame_in_screen( const os::Rect &cFrame );
	  // Convert a path with ~ or ^ in it to an absolute.
	std::string convert_path( std::string zPath );
		// Return a path to an icon
	std::string get_icon( std::string zPath );
	std::string get_launcher_path( void );
};

// These are the functions that must be present within a plugin.
extern "C"
{
  int     init( LauncherMessage *pcPrefs );  // Initialises the plugin
  void    finish( int nId );                 // Deletes the plugin
  LauncherMessage *get_info( void );         // Returns info about the plugin
  View    *get_view( int nId );              // Returns a View to display
  void    remove_view( int nId );            // Removes the View from it's Window
  LauncherMessage *get_prefs( int nId );     // Returns Prefs info for the plugin
  void    set_prefs( int nId, LauncherMessage *pcPrefs );        // Sets Prefs info
  void    handle_message( int nId, LauncherMessage *pcMessage ); // For passing Messages to the plugin 
  View    *get_prefs_view( int nId );        // Returns a View for changing preferences
};


enum  // Preferences
{
	// Vertical Window position
  LW_VPOS_TOP = 150,  
  LW_VPOS_BOTTOM,
  
    // Horizontal Window position
  LW_HPOS_LEFT,
  LW_HPOS_RIGHT,
  
    // Window alignment
  LW_ALIGN_VERTICAL,
  LW_ALIGN_HORIZONTAL,
 
    // Plugin visibility options
  LW_VISIBLE_ALWAYS,
  LW_VISIBLE_WHEN_ZOOMED,
  
  LW_ID_PREFS_LAST
};

enum  // Events
{
	// Sent to each plugin when the LauncherWindow's prefs change
  LW_WINDOW_PREFS_CHANGED = LW_ID_PREFS_LAST,
  
  LW_ZOOM_LOCK,   // Used by plugins to prevent or enable auto-hide/show of the LauncherWindow
  LW_ZOOM_UNLOCK,

  LW_PLUGIN_OPEN_PREFS,  // Sent by a plugin wishing to open a prefs window
  LW_PLUGIN_PREFS_BTN_OK,  // Sent to the plugin when OK is clicked in the prefs window
  LW_PLUGIN_PREFS_BTN_CANCEL,  // Send to the plugin when Cancel is clicked in the prefs window

  LW_PLUGIN_MOVE_UP,  // Sent by a plugin wishing to move up or left
  LW_PLUGIN_MOVE_DOWN,  // Sent by a plugin wishing to move down or right
  LW_PLUGIN_DELETE,  // Delete a plugin
  LW_PLUGIN_INVOKED,  // General use "this plugin has been manipulated" message.
  LW_PLUGIN_MESSAGE,  // Used to identify a message as being meant for a plugin rather than the window
  LW_PLUGIN_TIMER,    // Sent when a plugins timer expires
  LW_PLUGIN_OPEN_ABOUT,  // Message to open an About requester for a plugin.
  LW_PLUGIN_WINDOW_POS_CHANGED,  // Sent when a plugins position in the window changes.
  LW_REQUEST_REFRESH,  // Sent by a plugin wishing to get the Window display updated.
  
  LW_SAVE_PREFS, // Sent to the app object when prefs are changed.

  LW_EVENT_BROADCAST, // For broadcasting messages

  LW_ID_LAST
};


enum // ImageButton text position settings
{
	IB_TEXT_RIGHT = 100,
	IB_TEXT_LEFT,
	IB_TEXT_TOP,
	IB_TEXT_BOTTOM
};


typedef int     op_init( LauncherMessage *pcPrefs );
typedef void    op_delete( int nId );
typedef LauncherMessage *op_get_info( void );
typedef View    *op_get_view( int nId );
typedef void    op_remove_view( int nId );
typedef LauncherMessage *op_get_prefs( int nId );
typedef void    op_set_prefs( int nId, LauncherMessage *pcMessage );
typedef void    op_handle_message( int nId, LauncherMessage *pcMessage );
typedef View    *op_get_prefs_view( int nId );


/*
  The LauncherMessage is a Message with one extra piece of information,
  a pointer to a LauncherPlugin.
  
  When the LauncherWindow receives a message with a Code it does not recognise
  it will hand the Message to the plugin pointed to in the Message.
  
  The Plugin is stored internally as normal Message elements - a pointer called 
  __LM_PLUGIN so it is completely compatible with the original Message class.
*/
class LauncherMessage : public Message
{
	public:
		LauncherMessage( int32 nCode1 = 0, LauncherPlugin *pcPlugin = NULL);
		~LauncherMessage( );
/*		void SetCode2( int32 nCode2 );
		int32 GetCode2( void ); */
		void SetPlugin( LauncherPlugin *pcPlugin );
		LauncherPlugin *GetPlugin( );
};


/*
  The LauncherPlugin is the interface between the LauncherWindow and the plugin libraries.
  
  When created the LauncherPlugin will attempt to load the plugin library. If this succeeds
  it will call the "init" function of the plugin.
  
  The plugin must then return an ID number or LW_PLUGIN_INIT_FAIL. This ID will then be used
  in all calls to plugin functions.
  
*/
class LauncherPlugin
{

  public:
    LauncherPlugin( string zName, LauncherMessage *pcPrefs );
    ~LauncherPlugin( );
    View    *GetView( void );                                // calls "get_view"
    void    RemoveView( void );                              // calls "remove_view"
    LauncherMessage *GetInfo( void );                        // calls "get_info"
    int     GetHideStatus( void );                           // returns either LW_VISIBLE_ALWAYS or LW_VISIBLE_WHEN_ZOOMED
    void    SetHideStatus( int nHideStatus );                // Sets the plugin's HideStatus
    LauncherMessage *GetPrefs( void );                       // calls "get_prefs"
    void    SetPrefs( LauncherMessage *pcPrefs );            // calls "set_prefs"
    void    SetWindowPos( int nPos );  	                     // Remembers the position in the window of the plugin
    int     GetWindowPos( void );                            // Returns the window position of the plugin
    int     GetId( void );                                   // Returns the ID number given by "init"
    void    HandleMessage( LauncherMessage *pcMessage );     // calls "handle_message"
    void    AddTimer( int nTimeOut, bool bOneShot = true );  // Adds a timer to the LauncherWindow
    void    RemoveTimer( void );                             // Removes a timer from the LauncherWindow
    void    TimerTick( void );                               // Calls "handle_message" with a LW_PLUGIN_TIMER message.
    View    *GetPrefsView( void );                           // Calls "get_prefs_view"
    string  GetName( void );                                 // Returns the name of the plugin
    void    RequestWindowRefresh( void );                    // Sends an LW_REQUEST_REFRESH message to the window.
    void    LockWindow( void );                              // Sends an LW_ZOOM_LOCK message to the Window
    void    UnlockWindow( void );                            // Sends an LW_ZOOM_UNLOCK message to the Window
	Window *GetWindow( void );                               // Returns a pointer to the LauncherWindow
	void    BroadcastMessage( uint32 nBroadcastCode );       // Broadcasts a LauncherMessage to all other plugins in the Window.
	
  private:
    string  m_zName;        // The name of the plugin library (minus the ".so")
    int     m_nLibHandle;   // The handle for the library returned by "open_library"
    int     m_nId;          // ID of the plugin returned by "init" 
	int32   m_nHideStatus;  // The Hide Status of the plugin
	int32   m_nWindowPos;   // The plugin's position within the window
	bool    m_bTimerActive; // Set to true when the timer is active
	
	Window  *m_pcWindow;    // Pointer to the plugin's Window

      // Functions called in the plugin library
    op_init             *m_pfInit;   
    op_get_info         *m_pfGetInfo;
    op_delete           *m_pfDelete;
    op_get_view         *m_pfGetView;
    op_remove_view      *m_pfRemoveView;    
    op_get_prefs        *m_pfGetPrefs;
    op_set_prefs        *m_pfSetPrefs;
    op_handle_message   *m_pfHandleMessage;
	op_get_prefs_view   *m_pfGetPrefsView;
	
};



class ImageButton : public Button
{
	public:
		ImageButton( Rect cFrame, const char *pzName, const char *pzLabel, Message *pcMessage, uint32 nTextPosition = IB_TEXT_RIGHT, bool bDrawBorder = true,
		             uint32 nResizeMask = CF_FOLLOW_LEFT|CF_FOLLOW_TOP, uint32 nFlags=WID_WILL_DRAW|WID_CLEAR_BACKGROUND|WID_FULL_UPDATE_ON_RESIZE);
		ImageButton( Rect cFrame, const char *pzName, const char *pzLabel, Message *pcMessage, Bitmap *pcBitmap, uint32 nTextPosition = IB_TEXT_RIGHT, bool bDrawBorder = true,
		             uint32 nResizeMask = CF_FOLLOW_LEFT|CF_FOLLOW_TOP, uint32 nFlags=WID_WILL_DRAW|WID_CLEAR_BACKGROUND|WID_FULL_UPDATE_ON_RESIZE);
		ImageButton( Rect cFrame, const char *pzName, const char *pzLabel, Message *pcMessage, uint32 nWidth, uint32 nHeight, uint8 *pnBuffer, uint32 nTextPosition = IB_TEXT_RIGHT, bool bDrawBorder = true,
		             uint32 nResizeMask = CF_FOLLOW_LEFT|CF_FOLLOW_TOP, uint32 nFlags=WID_WILL_DRAW|WID_CLEAR_BACKGROUND|WID_FULL_UPDATE_ON_RESIZE);
		~ImageButton( );
		void SetImage( Bitmap *pcBitmap );
		void SetImage( uint32 nWidth, uint32 nHeight, uint8 *pnBuffer );
		bool SetImageFromFile( string zFile );
		void ClearImage( void );
		Bitmap *GetImage( void );
		void SetTextPosition( uint32 nTextPosition );
		uint32 GetTextPosition( void );
		void Paint( const Rect &cUpdateRect );
		Point GetPreferredSize( bool bLargest ) const;
		bool GetDrawBorder( void ) { return m_bDrawBorder; };
		void SetDrawBorder( bool bDrawBorder );
				
	private:
		Bitmap *m_pcBitmap;
		uint32 m_nTextPosition;
		bool m_bDrawBorder;
};
		             

#endif













