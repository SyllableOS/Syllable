// Keyboard Preferences :: (C)opyright 2002 Daryl Dudey
// Based on work by Kurt Skuaen
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <stdarg.h>
#include <stdio.h>
#include <dirent.h>
#include <util/application.h>
#include <util/message.h>
#include <util/resources.h>
#include <gui/image.h>
#include <appserver/keymap.h>
#include "mainwindow.h"
#include "messages.h"
#include "main.h"
#include "resources/Keyboard.h"

MainWindow::MainWindow(const os::Rect& cFrame) : os::Window(cFrame, "MainWindow", MSG_MAINWND_TITLE, os::WND_NOT_RESIZABLE)
{
  os::Rect cBounds = GetBounds();
  os::Rect cRect = os::Rect(0,0,0,0);

  // Create the layouts/views
  pcLRoot = new os::LayoutView(cBounds, "L", NULL, os::CF_FOLLOW_ALL);
  pcVLRoot = new os::VLayoutNode("VL");
  pcVLRoot->SetBorders( os::Rect(10,5,10,5) );

  // Top row for two frameview
  pcHLTop = new os::HLayoutNode("HLTop");

  // Frameview for settings
  pcVLSettings = new os::VLayoutNode("VLSettings");
  pcVLSettings->SetBorders( os::Rect(5,5,5,5) );
  pcVLSettings->AddChild(new os::VLayoutSpacer(""));

  // Get current delay and repeat settings
  os::Application::GetInstance()->GetKeyboardConfig( &cKeymap, &iDelay, &iRepeat );
  iDelay2 = iDelay;
  iRepeat2 = iRepeat;

  // Add delay to settings 
  pcHLDelay = new os::HLayoutNode("HLDelay");
  pcHLDelay->AddChild( pcSVDelay = new os::StringView(cRect, "SVDelay", MSG_MAINWND_SETTINGS_INITIALDELAY) );
  pcHLDelay->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
  pcHLDelay->AddChild( pcSLDelay = new os::Slider(cRect, "SLDelay", new os::Message(M_MW_SLDELAY), os::Slider::TICKS_BELOW, 11) );
  pcSLDelay->SetStepCount(11);
  pcSLDelay->SetLimitLabels(MSG_MAINWND_SETTINGS_INITIALDELAY_SHORT, MSG_MAINWND_SETTINGS_INITIALDELAY_LONG);
  pcSLDelay->SetMinMax(0,1000);
  pcVLSettings->AddChild(pcHLDelay);

  // Bit of space
  pcVLSettings->AddChild(new os::VLayoutSpacer("", 10.0f) );

  // And add repeat to settings
  pcHLRepeat = new os::HLayoutNode("HLRepeat");
  pcHLRepeat->AddChild( pcSVRepeat = new os::StringView(cRect, "SVRepeat", MSG_MAINWND_SETTINGS_REPEATDELAY) );
  pcHLRepeat->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
  pcHLRepeat->AddChild( pcSLRepeat = new os::Slider(cRect, "SLRepeat", new os::Message(M_MW_SLREPEAT), os::Slider::TICKS_BELOW, 11) );
  pcSLRepeat->SetStepCount(11);
  pcSLRepeat->SetLimitLabels(MSG_MAINWND_SETTINGS_REPEATDELAY_FAST, MSG_MAINWND_SETTINGS_REPEATDELAY_SLOW);
  pcSLRepeat->SetMinMax(0,200);
  pcVLSettings->AddChild(pcHLRepeat);
  pcVLSettings->AddChild( new os::VLayoutSpacer("") );

  // Set same widths
  pcVLSettings->SameWidth( "SVDelay", "SVRepeat", NULL );
  pcVLSettings->SameWidth( "SLDelay", "SLRepeat", NULL );

  // Create settings frameview and add to windows
  pcFVSettings = new os::FrameView(cBounds, "FVSettings", MSG_MAINWND_SETTINGS, os::CF_FOLLOW_ALL);
  pcFVSettings->SetRoot(pcVLSettings);
  pcHLTop->AddChild(pcFVSettings);
  pcHLTop->AddChild(new os::HLayoutSpacer("", 10.0f, 10.0f) );

  // Set slider values
  pcSLDelay->SetProgStrFormat(MSG_MAINWND_SETTINGS_MILISECONDS.c_str());
  pcSLRepeat->SetProgStrFormat(MSG_MAINWND_SETTINGS_MILISECONDS.c_str());
  pcSLDelay->SetValue( (float)iDelay, true);
  pcSLRepeat->SetValue( (float)iRepeat, true);

  // Now add frameview containing list of layouts
  pcHLLayout = new os::HLayoutNode("HLLayout");
  pcHLLayout->SetBorders( os::Rect(5,5,5,5) );
  pcHLLayout->AddChild( new os::HLayoutNode("") );
  pcHLLayout->AddChild( pcLVLayout = new os::ListView(cBounds, "LVLayout", os::ListView::F_RENDER_BORDER | os::ListView::F_NO_HEADER, os::CF_FOLLOW_ALL) );
  pcHLLayout->AddChild( new os::HLayoutNode("") );
  pcLVLayout->InsertColumn("Keyboard Layouts", 1);
  pcFVLayout = new os::FrameView(cBounds, "FVLayout", MSG_MAINWND_KEYBOARDLAYOUT, os::CF_FOLLOW_ALL);
  pcFVLayout->SetRoot(pcHLLayout);
  pcHLTop->AddChild(pcFVLayout);
  pcVLRoot->AddChild(pcHLTop);
  pcVLRoot->AddChild( new os::VLayoutSpacer("", 10.0f, 10.0f) );

  // And add a frameview with test area (if root)
  if (bRoot) {
    pcHLTest = new os::HLayoutNode("HLTest");
    pcHLTest->SetBorders( os::Rect(5,5,5,5) );
    pcHLTest->AddChild( new os::HLayoutNode("") );
    pcHLTest->AddChild( pcTVTest = new os::TextView(cRect, "TVTest", "") );
    pcHLTest->AddChild( new os::HLayoutNode("") );
    pcFVTest = new os::FrameView(cBounds, "FVTest", MSG_MAINWND_TESTAREA, os::CF_FOLLOW_ALL);
    pcFVTest->SetRoot(pcHLTest);
    pcVLRoot->AddChild(pcFVTest);
    pcVLRoot->AddChild( new os::VLayoutSpacer("", 10.0f, 10.0f) );
  }

  // Create apply/revert/close buttons
  pcHLButtons = new os::HLayoutNode("HLButtons");
  if (bRoot) {
    pcHLButtons->AddChild( new os::HLayoutSpacer(""));
    pcHLButtons->AddChild( pcBApply = new os::Button(cRect, "BApply", MSG_MAINWND_BUTTON_APPLY, new os::Message(M_MW_APPLY)) );
    pcHLButtons->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f) );
    pcHLButtons->AddChild( pcBUndo = new os::Button(cRect, "BUndo", MSG_MAINWND_BUTTON_UNDO, new os::Message(M_MW_UNDO)) );
    pcHLButtons->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
    pcHLButtons->AddChild( pcBDefault = new os::Button(cRect, "BDefault", MSG_MAINWND_BUTTON_DEFAULT, new os::Message(M_MW_DEFAULT)) );
    pcHLButtons->SameWidth( "BApply", "BUndo", "BDefault", NULL );
  }
  pcVLRoot->AddChild(pcHLButtons);

  // Set root and add to window
  pcLRoot->SetRoot(pcVLRoot);
  AddChild(pcLRoot);

  // Set default button and inital focus (if root)
  if (bRoot) {
    this->SetDefaultButton(pcBApply);
    pcSLDelay->MakeFocus();
  }
  
  // Set tab order
  int iTabOrder = 0;
  pcSLDelay->SetTabOrder(iTabOrder++);
  pcSLRepeat->SetTabOrder(iTabOrder++);
  pcLVLayout->SetTabOrder(iTabOrder++);
  if (bRoot) {
    pcBDefault->SetTabOrder(iTabOrder++);
    pcBUndo->SetTabOrder(iTabOrder++);
    pcBApply->SetTabOrder(iTabOrder++);
  }

  // Set to view if not root
  if (!bRoot) {
    this->SetTitle("Keyboard (View)");
    pcSLDelay->SetEnable(false);
    pcSLRepeat->SetEnable(false);
    pcLVLayout->SetEnable(false);
  }
  
  // Set Icon
  os::Resources cCol( get_image_id() );
  os::ResStream *pcStream = cCol.GetResourceStream( "icon48x48.png" );
  os::BitmapImage *pcIcon = new os::BitmapImage( pcStream );
  SetIcon( pcIcon->LockBitmap() );
  delete( pcIcon );

  ShowData();
}

MainWindow::~MainWindow()
{
}

void MainWindow::ShowData() 
{
  // Open up keymaps directory and check it actually contains something
  DIR *pDir = opendir("/system/keymaps");
  if (pDir == NULL) {
    return;
  }

  // Clear listview
  pcLVLayout->Clear();

  // Create a directory entry object
  dirent *psEntry;

  // Loop through directory and add each keymap to list
  int i = 0;
  while ( (psEntry = readdir( pDir )) != NULL) {

    // If special directory (i.e. dot(.) and dotdot(..) then ignore
    if (strcmp( psEntry->d_name, ".") == 0 || strcmp( psEntry->d_name, ".." ) == 0) {
      continue;
    }
    
    // Its a valid file, so open it
    FILE *hFile = fopen( (std::string("/system/keymaps/")+psEntry->d_name).c_str(), "r" );
    if (hFile == NULL) {
      continue;
    }

    // Check the first few numbers of the file (the magic number) match that for a keymap
    uint iMagic = 0;
    fread( &iMagic, sizeof(iMagic), 1, hFile);
    fclose( hFile);
    if (iMagic != KEYMAP_MAGIC ) {
      continue;
    }

    // Its valid, add to the listview
    os::ListViewStringRow* pcRow = new os::ListViewStringRow();
    pcRow->AppendString( psEntry->d_name );
    pcLVLayout->InsertRow( pcRow );
    
    // Check if row same as currently loaded, if so select it
    if ( cKeymap == psEntry->d_name ) {
      pcLVLayout->Select(i);
      iOrigRow = i;
      iOrigRow2 = i;
    }

    // See if american, so we can set the default row
    if ( std::string("American") == psEntry->d_name ) {
      iAmericanRow = i;
    }
    
    ++i;

  }
  closedir( pDir );

}

void MainWindow::Apply() 
{
  // Apply delay and repeat
  os::Application::GetInstance()->SetKeyboardTimings( int(pcSLDelay->GetValue()), int(pcSLRepeat->GetValue()) );

  // And apply keymap
  os::ListViewStringRow *pcRow = static_cast<os::ListViewStringRow*>(pcLVLayout->GetRow( pcLVLayout->GetFirstSelected() ));
  os::Application::GetInstance()->SetKeymap( pcRow->GetString(0).c_str() );

  // Save settings for undo
  iDelay2 = iDelay;
  iRepeat2 = iRepeat;
  iOrigRow2 = iOrigRow;
  iDelay = pcSLDelay->GetValue();
  iRepeat = pcSLRepeat->GetValue();
  iOrigRow = pcLVLayout->GetFirstSelected();
}

void MainWindow::Undo()
{
  // Revert to loaded settings
  pcSLDelay->SetValue( (float)iDelay2 );
  pcSLRepeat->SetValue( (float)iRepeat2 );
  pcLVLayout->Select(iOrigRow2);
}

void MainWindow::Default()
{
  // Set to defaults 300/40/American
  pcSLDelay->SetValue( (float)300.0f );
  pcSLRepeat->SetValue( (float)40.0f );
  pcLVLayout->Select(iAmericanRow);
}

void MainWindow::HandleMessage(os::Message* pcMessage)
{

  // Get message code and act on it
  switch(pcMessage->GetCode()) {

  case M_MW_APPLY:
    Apply();
    break;

  case M_MW_UNDO:
    Undo();
    break;

  case M_MW_DEFAULT:
    Default();
    break;
 
  default:
    os::Window::HandleMessage(pcMessage); break;
  }

}

bool MainWindow::OkToQuit()
{
  os::Application::GetInstance()->PostMessage(os::M_QUIT);
  return true;
}





