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

#include "launcher_plugin.h"

#include <vector>
#include <gui/font.h>
#include <gui/layoutview.h>
#include <gui/menu.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <atheos/time.h>
#include <atheos/kernel.h>
#include <stdio.h>

#include "gbar.h"

#define PLUGIN_NAME    "Launcher Battery Meter"
#define PLUGIN_VERSION "1.0"
#define PLUGIN_AUTHOR  "Hilary Cheng"
#define PLUGIN_DESC    "A Battery Meter"

#define TIMER 200000

#define AC_OFFLINE  0x00
#define AC_ONLINE   0x01
#define AC_ONBACKUP 0x02

#define BATTERY_VOLT_HIGH     0x01
#define BATTERY_VOLT_LOW      0x02
#define BATTERY_VOLT_CRITICAL 0x04
#define BATTERY_CHARGING      0x08
#define BATTERY_NO_SELECT     0x10
#define BATTERY_NOT_AVAIL     0x80

typedef struct {
  uint8 ac_status;
  uint8 battery_status;
  uint8 battery_flag;
  uint8 battery_percent;
  uint32 remain_seconds;
} POWER_STATUS;

using namespace os;

class LauncherBattery;
class BatteryMeter;

std::vector<LauncherBattery *>g_apcLauncherBattery;
uint32 g_nOldSize = 0;
LauncherMessage *g_pcInfoMessage;

class LauncherBattery : public LayoutView
{
public:
  LauncherBattery( string zName, LauncherMessage *pcPrefs );
  ~LauncherBattery( );
  void RemoveView( void );
  void HandleMessage( LauncherMessage *pcMessage );
  void MouseDown( const Point &cPosition, uint32 nButtons  );
  void AttachedToWindow( void );

private:
  LauncherPlugin *m_pcPlugin;
  string m_zName;
  bool m_bAboutAlertOpen;
  Menu *m_pcMenu;
  int m_nCpuCount;
		
  void UpdateMeters( void );		
  std::vector<BatteryMeter *>m_apcMeter;
};

class BatteryMeter : public GradientBar
{

public:
  BatteryMeter( int nCpuId );
  ~BatteryMeter();
  void UpdateMeter( void );

  virtual void Paint( const Rect &cUpdateRect );

private:
  int apm_file;

  POWER_STATUS power;

  int m_nCpuId;
  float m_vValue;
  float m_vOldValue;
  bigtime_t m_nLastIdle;
  bigtime_t m_nLastSys;

  status_t status;
  char power_str[15];

  uint8 ac_status;
};

//*************************************************************************************


int init( LauncherMessage *pcPrefs )
{
  if( ! g_pcInfoMessage ) {
    g_pcInfoMessage = new LauncherMessage( );
    g_pcInfoMessage->AddString( "Name",        PLUGIN_NAME    );
    g_pcInfoMessage->AddString( "Version",     PLUGIN_VERSION );
    g_pcInfoMessage->AddString( "Author",      PLUGIN_AUTHOR  );
    g_pcInfoMessage->AddString( "Description", PLUGIN_DESC    );
  }
    
  char zID[8];
  sprintf( zID, "%ld", g_nOldSize );
  string zName = (string)"LauncherBatteryII_" + (string)zID;
  g_apcLauncherBattery.push_back( new LauncherBattery( zName, pcPrefs ) );
    
  if( g_apcLauncherBattery.size( ) <= g_nOldSize )
    return INIT_FAIL;
      
  g_nOldSize = g_apcLauncherBattery.size( );
    
  return g_nOldSize - 1;
}


void finish( int nId )
{
  if( g_apcLauncherBattery[nId] )
    delete g_apcLauncherBattery[nId];
}

LauncherMessage *get_info( void )
{
  return g_pcInfoMessage;
}

View *get_view( int nId )
{
  return (View *)g_apcLauncherBattery[nId];
}

void remove_view( int nId )
{
  g_apcLauncherBattery[nId]->RemoveView( );
}

LauncherMessage *get_prefs( int nId )
{
  return new LauncherMessage( );
}

void set_prefs( int nId, LauncherMessage *pcPrefs )
{

}

void handle_message( int nId, LauncherMessage *pcMessage )
{
  g_apcLauncherBattery[nId]->HandleMessage( pcMessage );
}

/*
** name:       get_prefs_view
** purpose:    Returns a View* to be displayed in the PrefsWindow
** parameters: The ID number of the plugin.
** returns:    A pointer to the plugin's Prefs View.
*/
View *get_prefs_view( int nId )
{
  return NULL;
}

//*************************************************************************************

LauncherBattery::LauncherBattery( string zName, LauncherMessage *pcPrefs ) : LayoutView( Rect( ), zName.c_str() )
{
	
  m_pcPlugin = pcPrefs->GetPlugin( );
    
  m_zName = zName;

  m_bAboutAlertOpen = false;

  m_pcMenu = new Menu( Rect(0,0,10,10), "PopUp", ITEMS_IN_COLUMN );
  m_pcMenu->AddItem( "About Launcher Battery", new LauncherMessage( LW_PLUGIN_OPEN_ABOUT, m_pcPlugin ) );
  m_pcMenu->AddItem( "Preferences...", new LauncherMessage( LW_PLUGIN_OPEN_PREFS, m_pcPlugin ) );
  m_pcMenu->AddItem( new MenuSeparator( ) );
  m_pcMenu->AddItem( "Move Up", new LauncherMessage( LW_PLUGIN_MOVE_UP, m_pcPlugin ) );
  m_pcMenu->AddItem( "Move Down", new LauncherMessage( LW_PLUGIN_MOVE_DOWN, m_pcPlugin ) );
  m_pcMenu->AddItem( new MenuSeparator( ) );
  m_pcMenu->AddItem( "Delete", new LauncherMessage( LW_PLUGIN_DELETE, m_pcPlugin ) );
  Message cCloseMessage( LW_ZOOM_UNLOCK );
  m_pcMenu->SetCloseMessage( cCloseMessage );

  VLayoutNode *pcRoot = new VLayoutNode( "Root" );
	
  m_apcMeter.push_back( new BatteryMeter( 0 ) );
  pcRoot->AddChild( m_apcMeter[0] );
	
  SetRoot( pcRoot );
}

LauncherBattery::~LauncherBattery( )
{

}

void LauncherBattery::AttachedToWindow( void )
{
  m_pcMenu->SetCloseMsgTarget( GetWindow( ) ); 
  m_pcMenu->SetTargetForItems( GetWindow( ) );
  m_pcPlugin->AddTimer( TIMER, false );
  UpdateMeters( );
}

void LauncherBattery::MouseDown( const Point &cPosition, uint32 nButtons  )
{
  if( nButtons == 2 ) {
    m_pcPlugin->LockWindow( );
    Point cPos = ConvertToScreen( cPosition );
    Point cSize = m_pcMenu->GetPreferredSize( false );
    Rect cFrame = frame_in_screen( Rect( cPos.x, cPos.y, cPos.x + cSize.x, cPos.y + cSize.y ) ); 
    m_pcMenu->Open( Point( cFrame.left, cFrame.top) );
  } 
}

void LauncherBattery::RemoveView( void )
{
  m_pcPlugin->RemoveTimer( );
  RemoveThis( );
}


void LauncherBattery::HandleMessage( LauncherMessage *pcMessage )
{
  int nId = pcMessage->GetCode( );

  switch( nId ) {
    case LW_PLUGIN_PREFS_BTN_OK:
      m_pcPlugin->RequestWindowRefresh( );
      break;
			
    case LW_PLUGIN_PREFS_BTN_CANCEL:
      m_pcPlugin->RequestWindowRefresh( );
      break;	

    case LW_PLUGIN_TIMER:
      UpdateMeters( );
      break;
		
    default:
      // pcParentInvoker = new Invoker( new LauncherMessage( pcMessage->GetCode( ), m_pcPlugin), GetWindow() );
      // pcParentInvoker->Invoke();
      break;
    } 
}


void LauncherBattery::UpdateMeters( void )
{
  for( uint n = 0; n < m_apcMeter.size(); n++ )
    m_apcMeter[n]->UpdateMeter();

  m_apcMeter[0]->UpdateMeter();
}

//*************************************************************************************

BatteryMeter::BatteryMeter( int nCpuId ) : GradientBar( Rect(), "BatteryMeter" )
{
  apm_file = 0;
  apm_file = open("/dev/apm", O_RDONLY);

  if (apm_file > 0) {
    m_nCpuId = nCpuId;
    m_vValue = 0;
	
    SetStartCol(Color32_s(0,100,0));
    SetEndCol(Color32_s(255,180,0));

    status = ioctl(apm_file, 0, &power);

    ac_status = power.ac_status;
    m_vOldValue = power.battery_percent;
  } else {
    apm_file = 0;
    status   = -1;
  }
}

BatteryMeter::~BatteryMeter( )
{
  if (apm_file != 0) close(apm_file);
}

void BatteryMeter::UpdateMeter( void )
{
  if (apm_file == 0) return;

  status = ioctl(apm_file, 0, &power);
  if (status != 0) return;

  m_vValue = (float) (power.battery_percent) / 100.0f;

  SetValue(m_vValue);

  if (ac_status != power.ac_status || m_vOldValue != m_vValue) {
    ac_status = power.ac_status;
    m_vOldValue = m_vValue;
    Paint(GetBounds());
  }
}

void BatteryMeter::Paint( const Rect &cUpdateRect )
{
  float fWidth;
  font_height sHeight;
  float fHeight;
  float x, y;
  string value;
  Rect cBounds = GetBounds( );

  GradientBar::Paint(cUpdateRect);

  if (apm_file == 0 || status != 0) return;

  SetFgColor(100, 100, 100);
  MovePenTo(cBounds.left, cBounds.top);

  if (power.ac_status == AC_ONLINE) {
    sprintf(power_str, "AC %3d%c", power.battery_percent, '%'); 
  } else {
    sprintf(power_str, "Battery %3d%c", power.battery_percent, '%'); 
  }

  value = (string) power_str;
  
  fWidth = GetStringWidth( power_str );
  GetFontHeight( &sHeight );
  fHeight = /*sHeight.ascender +*/ sHeight.descender;

  x = ( cBounds.Width() / 2.0f) - (fWidth / 2.0f);
  y = 4.0f + (cBounds.Height()+1.0f)*0.5f - fHeight*0.5f + sHeight.descender;

  SetFgColor( 255, 255, 255);
  MovePenTo( x,y );
  DrawString( power_str );
}
