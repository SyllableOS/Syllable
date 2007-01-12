// Appearance Preferences :: (C)opyright 2002 Daryl Dudey
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

#include <dirent.h>
#include <fstream>
#include <util/application.h>
#include <util/message.h>
#include <util/resources.h>
#include <util/event.h>
#include <gui/desktop.h>
#include <gui/guidefines.h>
#include <gui/requesters.h>
#include <gui/image.h>
#include <storage/filereference.h>

#include "main.h"
#include "mainwindow.h"
#include "messages.h"
#include "resources/Appearance.h"

MainWindow::MainWindow(const os::Rect& cFrame) : os::Window(cFrame, "MainWindow", MSG_MAINWND_TITLE, os::WND_NOT_RESIZABLE)
{
  os::Rect cBounds = GetBounds();
  os::Rect cRect = os::Rect(0,0,0,0);

  // Create the layouts/views
  pcLRoot = new os::LayoutView(cBounds, "L", NULL, os::CF_FOLLOW_ALL);
  pcVLRoot = new os::VLayoutNode("VL");
  pcVLRoot->SetBorders( os::Rect(10,5,10,5) );

  // Build appearance settings
  pcVLAppearance = new os::VLayoutNode("VLAppearance");
  pcVLAppearance->SetBorders( os::Rect(5,5,5,5) );
  pcVLAppearance->AddChild( new os::HLayoutSpacer("") );
 
  // Window Decor
  pcHLDecor = new os::HLayoutNode("HLDecor");
  pcHLDecor->AddChild( new os::StringView(cRect, "SVDecor", MSG_MAINWND_WINDEC_DECOR));
  pcHLDecor->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
  pcHLDecor->AddChild( pcDDMDecor = new os::DropdownMenu(cRect, "DDMDecor" ));
  pcDDMDecor->SetMinPreferredSize(15);
  pcDDMDecor->SetMaxPreferredSize(15);
  pcDDMDecor->SetReadOnly();
  pcDDMDecor->SetSelectionMessage(new os::Message(M_MW_DECOR));
  pcDDMDecor->SetTarget(this);
  pcVLAppearance->AddChild(pcHLDecor);
    
  // Create appearance frameview
  pcFVAppearance = new os::FrameView( cBounds, "FVAppearance", MSG_MAINWND_WINDEC_WINDOWSDECORATION, os::CF_FOLLOW_ALL);
  pcFVAppearance->SetRoot(pcVLAppearance);
  pcVLRoot->AddChild( pcFVAppearance );
  pcVLRoot->AddChild( new os::VLayoutSpacer("", 5.0f, 5.0f));
  
  // Colour choose
  pcVLColour = new os::VLayoutNode("VLColour");
  pcVLColour->SetBorders( os::Rect(5,5,5,5) );
  pcHLTheme = new os::HLayoutNode("HLTheme");
  pcHLTheme->AddChild( new os::StringView(cRect, "SVTheme", MSG_MAINWND_CLRSCM_THEME));
  pcHLTheme->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f));
  pcHLTheme->AddChild( pcDDMTheme = new os::DropdownMenu(cRect, "DDMTheme"));
  pcDDMTheme->SetMinPreferredSize(15);
  pcDDMTheme->SetMaxPreferredSize(15);
  pcDDMTheme->SetSelectionMessage( new os::Message(M_MW_THEME));
  pcDDMTheme->SetTarget(this);
  pcVLColour->AddChild(pcHLTheme);
  pcVLColour->AddChild( new os::VLayoutSpacer("", 10.0f, 10.0f));

  // Create the dropdown listing the items
  pcDDMItem = new os::DropdownMenu(cRect, "LVItem");
  pcDDMItem->SetSelectionMessage( new os::Message(M_MW_ITEM));
  pcDDMItem->SetMinPreferredSize(15);
  pcDDMItem->SetMaxPreferredSize(15);
  pcDDMItem->SetReadOnly();
  pcDDMItem->SetSelectionMessage( new os::Message(M_MW_ITEM));
  pcDDMItem->SetTarget(this);

  // Add colour items to dropdown
  pcDDMItem->AppendItem(MSG_MAINWND_THEMEDROPDOWN_APPLICATION); // NORMAL
  pcDDMItem->AppendItem(MSG_MAINWND_THEMEDROPDOWN_HIGHLIGHT); // SHINE
  pcDDMItem->AppendItem(MSG_MAINWND_THEMEDROPDOWN_SHADOW);  // SHADOW
  pcDDMItem->AppendItem(MSG_MAINWND_THEMEDROPDOWN_ACTIVEWINDOW); // SELECTED WINDOW
  pcDDMItem->AppendItem(MSG_MAINWND_THEMEDROPDOWN_WINDOWBORDER); // WINDOW BORDER
  pcDDMItem->AppendItem(MSG_MAINWND_THEMEDROPDOWN_MENUTEXT); // MENU TEXT
  pcDDMItem->AppendItem(MSG_MAINWND_THEMEDROPDOWN_MENUHIGHLIGHTTEXT); // MENU HIGHLIGHT TEXT
  pcDDMItem->AppendItem(MSG_MAINWND_THEMEDROPDOWN_MENUBACKGROUND); // MENU BACKGROUND
  pcDDMItem->AppendItem(MSG_MAINWND_THEMEDROPDOWN_MENUHIGHLIGHT); // MENU HIGHLIGHT
  pcDDMItem->AppendItem(MSG_MAINWND_THEMEDROPDOWN_SCROLLBARBACKGROUND); // SCROLLBAR BACKGROUND
  pcDDMItem->AppendItem(MSG_MAINWND_THEMEDROPDOWN_SCROLLBARKNOB); // SCROLLBAR KNOB
  pcDDMItem->AppendItem(MSG_MAINWND_THEMEDROPDOWN_LISTVIEWTAB); // LISTVIEW TAB
  pcDDMItem->AppendItem(MSG_MAINWND_THEMEDROPDOWN_LISTVIEWTABTEXT); // LISTVIEW TAB TEXT
  pcDDMItem->AppendItem(MSG_MAINWND_THEMEDROPDOWN_ICONTEXT);
  pcDDMItem->AppendItem(MSG_MAINWND_THEMEDROPDOWN_ICONSELECTION);
  pcDDMItem->AppendItem(MSG_MAINWND_THEMEDROPDOWN_ICONBACKGROUND);
  pcDDMItem->AppendItem(MSG_MAINWND_THEMEDROPDOWN_FOCUSEDITEM);
  pcDDMItem->SetSelection(1);
  pcDDMItem->SetSelection(0);
  
  // Get initial values for all pens
  for (int i=0;i<os::COL_COUNT;i++) {
    ePens[i] = os::get_default_color( (os::default_color_t)i );
  }
  
  // Create customise layout
  pcHLCustomise = new os::HLayoutNode("HLCustomise");
  pcHLCustomise->SetBorders( os::Rect(5,5,5,5) );
  
  // Colour sliders
  pcVLSlider = new os::VLayoutNode("VLSlider");
  pcVLSlider->AddChild( new os::HLayoutSpacer("") );

  // Header
  pcHLHeader = new os::HLayoutNode("HLHeader");
  pcHLHeader->AddChild( new os::StringView(cRect, "SVHeader", ""));
  pcHLHeader->AddChild( new os::StringView(cRect, "SLHeader", ""));
  pcHLHeader->AddChild( new os::StringView(cRect, "TVHeader", MSG_MAINWND_CLRSCM_DEC, os::ALIGN_CENTER));
  pcHLHeader->AddChild( new os::StringView(cRect, "TVHeaderHex", MSG_MAINWND_CLRSCM_HEX, os::ALIGN_CENTER));
  pcVLSlider->AddChild( pcHLHeader );

  // Red
  pcHLRed = new os::HLayoutNode("HLRed");
  pcHLRed->AddChild( new os::StringView(cRect, "SVRed", MSG_MAINWND_CLRSCM_RED));
  pcHLRed->AddChild( pcSLRed = new os::Slider(cRect, "SLRed", new os::Message(M_MW_SLRED)));
  pcSLRed->SetTarget(this);
  pcSLRed->SetStepCount(64);
  pcSLRed->SetTickCount(16);
  pcHLRed->AddChild( pcTVRed = new os::TextView(cRect, "TVRed", "###"));
  pcTVRed->SetMinPreferredSize(3,1);
  pcTVRed->SetMaxLength(3);
  pcTVRed->SetNumeric(true);
  pcHLRed->AddChild( pcTVRedHex = new os::TextView(cRect, "TVRedHex", "#XX"));
  pcTVRedHex->SetMinPreferredSize(3,1);
  pcTVRedHex->SetMaxLength(2);
  pcVLSlider->AddChild( pcHLRed );

  // Green
  pcHLGreen = new os::HLayoutNode("HLGreen");
  pcHLGreen->AddChild( new os::StringView(cRect, "SVGreen", MSG_MAINWND_CLRSCM_GREEN));
  pcHLGreen->AddChild( pcSLGreen = new os::Slider(cRect, "SLGreen", new os::Message(M_MW_SLGREEN)));
  pcSLGreen->SetTarget(this);
  pcSLGreen->SetStepCount(64);
  pcSLGreen->SetTickCount(16);
  pcHLGreen->AddChild( pcTVGreen = new os::TextView(cRect, "TVGreen", "###") );
  pcTVGreen->SetMinPreferredSize(3,1);
  pcTVGreen->SetMaxLength(3);
  pcTVGreen->SetNumeric(true);
  pcHLGreen->AddChild( pcTVGreenHex = new os::TextView(cRect, "TVGreenHex", "#XX"));
  pcTVGreenHex->SetMinPreferredSize(3,1);
  pcTVGreenHex->SetMaxLength(2);
  pcVLSlider->AddChild( pcHLGreen );
  
  // Blue
  pcHLBlue = new os::HLayoutNode("HLBlue");
  pcHLBlue->AddChild( new os::StringView(cRect, "SVBlue", MSG_MAINWND_CLRSCM_BLUE));
  pcHLBlue->AddChild( pcSLBlue = new os::Slider(cRect, "SLBlue", new os::Message(M_MW_SLBLUE)));
  pcSLBlue->SetTarget(this);
  pcSLBlue->SetStepCount(64);
  pcSLBlue->SetTickCount(16);
  pcHLBlue->AddChild( pcTVBlue = new os::TextView(cRect, "TVBlue", "###") );
  pcTVBlue->SetMinPreferredSize(3,1);
  pcTVBlue->SetMaxLength(3);
  pcTVBlue->SetNumeric(true);
  pcHLBlue->AddChild( pcTVBlueHex = new os::TextView(cRect, "TVBlueHex", "#XX"));
  pcTVBlueHex->SetMinPreferredSize(3,1);
  pcTVBlueHex->SetMaxLength(2);
  pcVLSlider->AddChild( pcHLBlue );
  
  // Now align the slider colour controls
  pcVLSlider->SameWidth( "SVHeader", "SVRed", "SVGreen", "SVBlue", NULL );
  pcVLSlider->SameWidth( "SLHeader", "SLRed", "SLGreen", "SLBlue", NULL );
  pcVLSlider->SameWidth( "TVHeader", "TVRed", "TVGreen", "TVBlue", NULL );
  pcVLSlider->SameWidth( "TVHeaderHex", "TVRedHex", "TVGreenHex", "TVBlueHex", NULL );
  pcHLCustomise->AddChild( pcVLSlider );

  // Add preview
  pcVLPreview = new os::VLayoutNode("VLPreview");
  pcVLPreview->SetBorders( os::Rect( 5,15,5,10 ) );
  pcVLPreview->AddChild( pcCPPreview = new ColourPreview(cBounds, "CPPreview") );
  pcHLCustomise->AddChild( pcVLPreview );

  // Create customise frameview
  pcFVCustomise = new os::FrameView( cBounds, "FVCustomise", "Customise: ", os::CF_FOLLOW_ALL);
  pcFVCustomise->SetRoot(pcHLCustomise);
  pcFVCustomise->SetLabel(pcDDMItem, true);
  pcHLCustSpacer = new os::HLayoutNode("HLCustSpacer");
  pcHLCustSpacer->AddChild( new os::HLayoutSpacer("", 10.0f, 10.0f) );
  pcHLCustSpacer->AddChild(pcFVCustomise);
  pcHLCustSpacer->AddChild( new os::HLayoutSpacer("", 10.0f, 10.0f) );
  pcVLColour->AddChild(pcHLCustSpacer);
  pcVLColour->AddChild( new os::VLayoutSpacer("", 10.0f ,10.0f) );

  // Add scheme delete/save
  pcHLSchemeButtons = new os::HLayoutNode("HLSchemeButtons");
  pcHLSchemeButtons->AddChild( new os::HLayoutSpacer("") );
  pcHLSchemeButtons->AddChild( pcBSave = new os::Button(cRect, "BSave", MSG_MAINWND_CLRSCM_BUTTON_SAVE, new os::Message(M_MW_SAVE)) );
  pcHLSchemeButtons->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f));
  pcHLSchemeButtons->AddChild( pcBDelete = new os::Button(cRect, "BDelete", MSG_MAINWND_CLRSCM_BUTTON_DEL, new os::Message(M_MW_DELETE)) );
  pcHLSchemeButtons->SameWidth( "BSave", "BDelete", NULL );
  pcVLColour->AddChild( pcHLSchemeButtons );

  // Create colours frameview
  pcFVColour = new os::FrameView( cBounds, "FVColour", MSG_MAINWND_CLRSCM_COLOURSCHEME, os::CF_FOLLOW_ALL);
  pcFVColour->SetRoot(pcVLColour);
  pcVLRoot->AddChild(pcFVColour);

  // Create apply/revert/close buttons
  if (bRoot) {
    pcVLRoot->AddChild( new os::VLayoutSpacer("", 10.0f, 10.0f));
    pcHLButtons = new os::HLayoutNode("HLButtons");
    pcHLButtons->AddChild( new os::HLayoutSpacer(""));
    pcHLButtons->AddChild( pcBApply = new os::Button(cRect, "BApply", MSG_MAINWND_BUTTON_APPLY, new os::Message(M_MW_APPLY)) );
    pcHLButtons->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
    pcHLButtons->AddChild( pcBDefault = new os::Button(cRect, "BDefault", MSG_MAINWND_BUTTON_DEFAULT, new os::Message(M_MW_DEFAULT)) );
    pcHLButtons->SameWidth( "BApply", "BDefault", NULL );
    pcHLButtons->SameHeight( "BApply", "BDefault", NULL );
    pcVLRoot->AddChild(pcHLButtons);
  }

  // Set root and add to window
  pcLRoot->SetRoot(pcVLRoot);
  AddChild(pcLRoot);

  // Set default button and initial focus (if root)
  if (bRoot) {
    this->SetDefaultButton(pcBApply);
    pcDDMDecor->MakeFocus();
  }

  // Set tab order
  int iTabOrder = 0;
  pcDDMDecor->SetTabOrder(iTabOrder++);
  if (bRoot) {
    pcBDefault->SetTabOrder(iTabOrder++);
    pcBApply->SetTabOrder(iTabOrder++);
  }
  
  
  // Set Icon
  os::Resources cCol( get_image_id() );
  os::ResStream *pcStream = cCol.GetResourceStream( "icon48x48.png" );
  os::BitmapImage *pcIcon = new os::BitmapImage( pcStream );
  SetIcon( pcIcon->LockBitmap() );
  delete( pcIcon );

  // Show data
  ShowData();
}

MainWindow::~MainWindow()
{
}

void MainWindow::ShowData() 
{
  // Create decor dropdown
  pcDDMDecor->Clear();

  // Check decor directory exists
  DIR *pDir = opendir("/system/drivers/appserver/decorators/");
  if (pDir != NULL) {

    // Loop through all entries in directory and add to dropdown
    dirent *psEntry;
    while ( (psEntry = readdir( pDir )) != NULL) {
      if ( strcmp( psEntry->d_name, ".")==0 || strcmp( psEntry->d_name, "..")==0) {
	continue;
      }
      pcDDMDecor->AppendItem( psEntry->d_name );
    }
  } else {
    os::Alert *pcAlert = new os::Alert(MSG_ALERT_NODECORDIR_TITLE, MSG_ALERT_NODECORDIR_TEXT, os::Alert::ALERT_WARNING, os::WND_NOT_RESIZABLE, MSG_ALERT_NODECORDIR_OK.c_str(), NULL);
    pcAlert->Go();
  }
  closedir( pDir );
  pcDDMDecor->SetSelection(1);
  pcDDMDecor->SetSelection(0);

  // Fill the colour scheme list
  pcDDMTheme->Clear();

  // Check theme directory exists
  pDir = opendir("/system/config/appearance/colour-schemes/");
  if (pDir != NULL) {
    
    // Loop through
    dirent *psEntry;
    while ( (psEntry = readdir( pDir )) != NULL) {
      if ( strcmp( psEntry->d_name, ".")==0 || strcmp( psEntry->d_name, "..")==0) {
	continue;
      }
      pcDDMTheme->AppendItem( psEntry->d_name);
    }
  } else {
    os::Alert *pcAlert = new os::Alert(MSG_ALERT_NOCLRSCMDIR_TITLE,MSG_ALERT_NOCLRSCMDIR_TEXT, os::Alert::ALERT_WARNING, os::WND_NOT_RESIZABLE, MSG_ALERT_NOCLRSCMDIR_OK.c_str(), NULL);
    pcAlert->Go();
  }
  closedir( pDir );
  pcDDMTheme->SetSelection(1);
  pcDDMTheme->SetSelection(0);
}

void MainWindow::Apply() 
{
  // Apply decor
  char zPath[256];
  strcpy( zPath, "/system/drivers/appserver/decorators/" );
  strcat( zPath, pcDDMDecor->GetCurrentString().c_str() );
  os::Application::GetInstance()->SetWindowDecorator( zPath );

  // Update all colours
  for (int i=0;i<os::COL_COUNT;i++) {
    os::set_default_color ( (os::default_color_t)i, ePens[i]);
  }

  os::Application::GetInstance()->CommitColorConfig();

  /* Refresh desktop */
  os::Event cEvent;
  if( cEvent.SetToRemote( "os/Desktop/Refresh" ) == 0 )
  {
  	os::Message cDummy;
  	cEvent.PostEvent( &cDummy );
  }
}

void MainWindow::Default()
{
  // Set scheme to default
  pcDDMTheme->SetCurrentString("Default");

  // And load
  Load();
}

void MainWindow::ColourChangePreview()
{
  char pzScratch[4];

  // Get values from sliders
  int iRed = (int)(pcSLRed->GetValue().AsFloat()*256.0f);
  int iGreen = (int)(pcSLGreen->GetValue().AsFloat()*256.0f);
  int iBlue = (int)(pcSLBlue->GetValue().AsFloat()*256.0f);

  if (iRed==256) iRed=255;
  if (iGreen==256) iGreen=255;
  if (iBlue==256) iBlue=255;
		 
  // Create decimal colour values and update on screen
  sprintf((char *)&pzScratch, "%d", iRed);
  pcTVRed->Set(pzScratch, true);
  sprintf((char *)&pzScratch, "%d", iGreen);
  pcTVGreen->Set(pzScratch, true);
  sprintf((char *)&pzScratch, "%d", iBlue);
  pcTVBlue->Set(pzScratch, true);

  // Update hex colour string
  sprintf((char *)&pzScratch, "%X", iRed);
  pcTVRedHex->Set(pzScratch);
  sprintf((char *)&pzScratch, "%X", iGreen);
  pcTVGreenHex->Set(pzScratch);
  sprintf((char *)&pzScratch, "%X", iBlue);
  pcTVBlueHex->Set(pzScratch);

  // And update table in  memory
  int i = pcDDMItem->GetSelection();
  ePens[i].red = iRed;
  ePens[i].green = iGreen;
  ePens[i].blue = iBlue;
  
  // Update value in preview
  pcCPPreview->SetValue( iRed, iGreen, iBlue );
}

void MainWindow::LoadPen()
{
  int i = pcDDMItem->GetSelection();

   // Now set sliders
  pcSLRed->SetValue( (float)ePens[i].red / 255.0f );
  pcSLGreen->SetValue( (float)ePens[i].green / 255.0f );
  pcSLBlue->SetValue( (float)ePens[i].blue / 255.0f );
}

const char newl = '\n';

void MainWindow::Load()
{
  // Work out filename
  char pzFilename[1024];
  strcpy(pzFilename, "/system/config/appearance/colour-schemes/");
  strcat(pzFilename, pcDDMTheme->GetCurrentString().c_str());

  // Open up for reading
  std::ifstream fsIn;
  fsIn.open(pzFilename);

  // Read in magic number first
  if (!fsIn.is_open()) {
    os::Alert *pcAlert = new os::Alert(MSG_ALERT_CLRSCMERR_TITLE, MSG_ALERT_CLRSCMERR_TEXT, os::Alert::ALERT_WARNING, os::WND_NOT_RESIZABLE, MSG_ALERT_CLRSCMERR_OK.c_str(), NULL);
    pcAlert->Go();
  } else { 
    char pzScratch[1024];

    // Drag in magic number, not being checked yet
    fsIn.getline(pzScratch, '\n');

    // Now drag in all the colour values
    for (int i=0;i<os::COL_COUNT;i++) {
      fsIn >> ePens[i].red;
      fsIn >> ePens[i].green;
      fsIn >> ePens[i].blue;
    }
  }

  // And close
  fsIn.close();
}

void MainWindow::Save() 
{
  // Work out filename
  char pzFilename[1024];
  strcpy(pzFilename, "/system/config/appearance/colour-schemes/");
  strcat(pzFilename, pcDDMTheme->GetCurrentString().c_str());
  std::string zName = pcDDMTheme->GetCurrentString(); 

  // Open up file for writing
  std::ofstream fsOut;
  fsOut.open(pzFilename);
  
  // Write out magic number
  fsOut << "SACS" << newl;

  // Write out data
  for (int i=0;i<os::COL_COUNT;i++) {
    fsOut << ePens[i].red << ePens[i].green << ePens[i].blue << newl;
  }

  // And Close
  fsOut.close();

  ShowData();
  pcDDMTheme->SetCurrentString(zName);
}

void MainWindow::Delete()
{
  // Work out filename
  std::string zFilename = "/system/config/appearance/colour-schemes/";
  zFilename += pcDDMTheme->GetCurrentString();

  // Get reference to file
  os::FileReference *pcFile = new os::FileReference(zFilename);

  // Delete file
  pcFile->Delete();

  // Delete reference
  delete pcFile;

  ShowData();
  Default();
}

void MainWindow::HandleMessage(os::Message* pcMessage)
{
  // Get message code and act on it
  switch(pcMessage->GetCode()) {

  case M_MW_SLRED:
  case M_MW_SLGREEN:
  case M_MW_SLBLUE:
    ColourChangePreview();
    break;

  case M_MW_THEME:
    Load();
    break;

  case M_MW_ITEM:
    LoadPen();
    break;

  case M_MW_SAVE:
    Save();
    break;

  case M_MW_DELETE:
    Delete();
    break;

  case M_MW_APPLY:
    Apply();
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

ColourPreview::ColourPreview( const os::Rect &cFrame, const char *pzName ) : os::View( cFrame, pzName)
{
  cColour = os::Color32_s(0,0,0,0);
}

void ColourPreview::FrameSized( const os::Point &cDelta ) 
{
  View::FrameSized (cDelta);
}

void ColourPreview::Paint( const os::Rect &cUpdateRec )
{
  // Get size of control
  os::Rect cBounds = GetBounds();
  os::Rect cInside = cBounds;
  cInside.left+=1; cInside.top+=1; cInside.right-=1; cInside.bottom-=1;

  // And fill it
  SetFgColor( os::Color32_s(0,0,0,0) );
  FillRect( cBounds );
  SetFgColor( cColour );
  FillRect( cInside );
}

void ColourPreview::SetValue(int iRed, int iGreen, int iBlue)
{
  // Change colour
  cColour = os::Color32_s(iRed, iGreen, iBlue, 0);

  // And update control
  Paint(GetBounds());
}








