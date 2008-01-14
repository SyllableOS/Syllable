// Font Preferences :: (C)opyright 2002 Daryl Dudey
// Inspiration from code by Kurt Skuaen
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

#include <stdio.h>
#include <gui/image.h>
#include <util/application.h>
#include <util/message.h>
#include <util/resources.h>
#include "main.h"
#include "mainwindow.h"
#include "messages.h"
#include "resources/Font.h"

MainWindow::MainWindow(const os::Rect& cFrame) : os::Window(cFrame, "MainWindow", MSG_MAINWND_TITLE, /*os::WND_NOT_RESIZABLE*/ 0)
{
  os::Rect cBounds = GetBounds();
  os::Rect cRect = os::Rect(0,0,0,0);

  // Create the layouts/views
  pcLRoot = new os::LayoutView(cBounds, "L", NULL, os::CF_FOLLOW_ALL);
  pcVLRoot = new os::VLayoutNode("VL");
  pcVLRoot->SetBorders( os::Rect(10,5,10,5) );

  // Normal
  pcHLNormal = new os::HLayoutNode("HLNormal");
  pcHLNormal->AddChild( new os::HLayoutSpacer("") );
  pcHLNormal->AddChild( new os::StringView(cRect, "SVNormal", MSG_MAINWND_FONTSELECTION_NORMAL) );
  pcHLNormal->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
  pcHLNormal->AddChild( pcDDMNormal = new os::DropdownMenu(cRect, "DDMNormal") );
  pcDDMNormal->SetShortcut( os::ShortcutKey( "O" ) );
  pcDDMNormal->SetMinPreferredSize(18);
  pcDDMNormal->SetTarget(this);
  pcDDMNormal->SetEditMessage( new os::Message(M_MW_DCNOR) );
  pcHLNormal->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
  pcHLNormal->AddChild( pcSPNormal = new os::Spinner(cRect, "SPNormal", (double)8.0, new os::Message(M_MW_SPNORMAL)) );
  pcSPNormal->SetStep( (double)0.5 );
  pcSPNormal->SetMinPreferredSize(4);
  pcHLNormal->AddChild( new os::HLayoutSpacer("") );

  // Bold
  pcHLBold = new os::HLayoutNode("HLBold");
  pcHLBold->AddChild( new os::HLayoutSpacer("") );
  pcHLBold->AddChild( new os::StringView(cRect, "SVBold", MSG_MAINWND_FONTSELECTION_BOLD) );
  pcHLBold->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
  pcHLBold->AddChild( pcDDMBold = new os::DropdownMenu(cRect, "DDMBold") );
  pcDDMBold->SetShortcut( os::ShortcutKey( "B" ) );
  pcDDMBold->SetMinPreferredSize(18);
  pcDDMBold->SetTarget(this);
  pcDDMBold->SetSelectionMessage( new os::Message(M_MW_DCBLD) );
  pcHLBold->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
  pcHLBold->AddChild( pcSPBold = new os::Spinner(cRect, "SPBold", (double)8.0, new os::Message(M_MW_SPBOLD)) );
  pcSPBold->SetStep( (double)0.5 );
  pcSPBold->SetMinPreferredSize(4);
  pcHLBold->AddChild( new os::HLayoutSpacer("") );

  // Fixed
  pcHLFixed = new os::HLayoutNode("HLFixed");
  pcHLFixed->AddChild( new os::HLayoutSpacer("") );
  pcHLFixed->AddChild( new os::StringView(cRect, "SVFixed", MSG_MAINWND_FONTSELECTION_FIXED) );
  pcHLFixed->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
  pcHLFixed->AddChild( pcDDMFixed = new os::DropdownMenu(cRect, "DDMFixed") );
  pcDDMFixed->SetShortcut( os::ShortcutKey( "F" ) );
  pcDDMFixed->SetMinPreferredSize(18);
  pcDDMFixed->SetTarget(this);
  pcDDMFixed->SetSelectionMessage( new os::Message(M_MW_DCFXD) );
  pcHLFixed->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
  pcHLFixed->AddChild( pcSPFixed = new os::Spinner(cRect, "SPFixed", (double)8.0, new os::Message(M_MW_SPFIXED)) );
  pcSPFixed->SetStep( (double)0.5 );
  pcSPFixed->SetMinPreferredSize(4);
  pcHLFixed->AddChild( new os::HLayoutSpacer("") );

  // Window
  pcHLWindow = new os::HLayoutNode("HLWindow");
  pcHLWindow->AddChild( new os::HLayoutSpacer("") );
  pcHLWindow->AddChild( new os::StringView(cRect, "SVWindow", MSG_MAINWND_FONTSELECTION_WINDOW) );
  pcHLWindow->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
  pcHLWindow->AddChild( pcDDMWindow = new os::DropdownMenu(cRect, "DDMWindow") );
  pcDDMWindow->SetShortcut( os::ShortcutKey( "W" ) );
  pcDDMWindow->SetMinPreferredSize(18);
  pcDDMWindow->SetTarget(this);
  pcDDMWindow->SetSelectionMessage( new os::Message(M_MW_DCWND) );
  pcHLWindow->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
  pcHLWindow->AddChild( pcSPWindow = new os::Spinner(cRect, "SPWindow", (double)8.0, new os::Message(M_MW_SPWINDOW)) );
  pcSPWindow->SetStep( (double)0.5 );
  pcSPWindow->SetMinPreferredSize(4);
  pcHLWindow->AddChild( new os::HLayoutSpacer("") );
  
  // Toolwindow
  pcHLTWindow = new os::HLayoutNode("HLTWindow");
  pcHLTWindow->AddChild( new os::HLayoutSpacer("") );
  pcHLTWindow->AddChild( new os::StringView(cRect, "SVTWindow", MSG_MAINWND_FONTSELECTION_TOOLWIN) );
  pcHLTWindow->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
  pcHLTWindow->AddChild( pcDDMTWindow = new os::DropdownMenu(cRect, "DDMTWindow") );
  pcDDMTWindow->SetShortcut( os::ShortcutKey( "T" ) );
  pcDDMTWindow->SetMinPreferredSize(18);
  pcDDMTWindow->SetTarget(this);
  pcDDMTWindow->SetSelectionMessage( new os::Message(M_MW_DCTWD) );
  pcHLTWindow->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
  pcHLTWindow->AddChild( pcSPTWindow = new os::Spinner(cRect, "SPTWindow", (double)8.0, new os::Message(M_MW_SPTWINDOW)) );
  pcSPTWindow->SetStep( (double)0.5 );
  pcSPTWindow->SetMinPreferredSize(4);
  pcHLTWindow->AddChild( new os::HLayoutSpacer("") );
  
  // Anti-aliasing option (global for now, should probably be a per-font setting or based on font size)
  os::LayoutNode *pcHLTAntiAliasing = new os::HLayoutNode("HLTAntiAliasing");
  pcHLTAntiAliasing->AddChild( new os::HLayoutSpacer("", 100.0f) );
  pcHLTAntiAliasing->AddChild( ( pcCBAntiAliasing = new os::CheckBox( cRect, "CBAntiAliasing", MSG_MAINWND_FONTSELECTION_TEXT, new os::Message( M_MW_ANTIALIASING ) ) ), true );
  pcHLTAntiAliasing->AddChild( new os::HLayoutSpacer("", 100.0f) );

  // Add types to types layout node
  pcVLTypes = new os::VLayoutNode("VLTypes");
  pcVLTypes->SetBorders( os::Rect(5,5,5,5) );
  pcVLTypes->AddChild( new os::VLayoutSpacer("") );
  pcVLTypes->AddChild( pcHLNormal );
  pcVLTypes->AddChild( new os::VLayoutSpacer("", 5.0f, 5.0f) );
  pcVLTypes->AddChild( pcHLBold);
  pcVLTypes->AddChild( new os::VLayoutSpacer("", 5.0f, 5.0f) );
  pcVLTypes->AddChild( pcHLFixed);
  pcVLTypes->AddChild( new os::VLayoutSpacer("", 5.0f, 5.0f) );
  pcVLTypes->AddChild( pcHLWindow);
  pcVLTypes->AddChild( new os::VLayoutSpacer("", 5.0f, 5.0f) );
  pcVLTypes->AddChild( pcHLTWindow);
  pcVLTypes->AddChild( new os::VLayoutSpacer("", 5.0f, 5.0f) );
  if (bRoot)
	  pcVLTypes->AddChild( pcHLTAntiAliasing );
  pcVLTypes->AddChild( new os::VLayoutSpacer("") );
  pcFVTypes = new os::FrameView( cBounds, "FVTypes", MSG_MAINWND_FONTSELECTION, os::CF_FOLLOW_ALL);
  pcFVTypes->SetRoot(pcVLTypes);

  // Set widths the same
  pcVLTypes->SameWidth( "SVNormal", "SVBold", "SVFixed", "SVWindow", "SVTWindow", NULL);
  pcVLTypes->SameWidth( "DDMNormal", "DDMBold", "DDMFixed", "DDMWindow", "DDMTWindow", NULL );
  pcVLTypes->SameWidth( "SPNormal", "SPBold", "SPFixed", "SPWindow", "SPTWindow", NULL );

  // Add types to main node
  pcVLRoot->AddChild(pcFVTypes);
  pcVLRoot->AddChild(new os::VLayoutSpacer("", 5.0f, 5.0f) );

  // Add example area (if root)
  if (bRoot) {
    pcHLExample = new os::HLayoutNode("HLExample");
    pcHLExample->SetBorders( os::Rect(10,10,10,10) );
    pcHLExample->AddChild( pcSVExample = new os::StringView(cRect, "SVExample", "") );
    pcSVExample->SetMinPreferredSize(30);
    pcSVExample->SetMaxPreferredSize(30);
    pcHLExample->AddChild( new os::HLayoutSpacer("") );
    pcFVExample = new os::FrameView( cBounds, "FVExample", MSG_MAINWND_EXAMPLE, os::CF_FOLLOW_ALL);
    pcFVExample->SetRoot(pcHLExample);
    pcVLRoot->AddChild(pcFVExample);
  }

  // Add buttons  (if root)
  if (bRoot) {
    pcHLButtons = new os::HLayoutNode("HLButtons");
    pcVLRoot->AddChild( new os::VLayoutSpacer("", 10.f, 10.0f));
    pcHLButtons->AddChild( new os::HLayoutSpacer(""));
    pcHLButtons->AddChild( pcBApply = new os::Button(cRect, "BApply", MSG_MAINWND_BUTTON_APPLY, new os::Message(M_MW_APPLY) ));
    pcHLButtons->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
    pcHLButtons->AddChild( pcBRevert = new os::Button(cRect, "BRevert", MSG_MAINWND_BUTTON_REVERT, new os::Message(M_MW_REVERT) ));
    pcHLButtons->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
    pcHLButtons->AddChild( pcBDefault = new os::Button(cRect, "BDefault", MSG_MAINWND_BUTTON_DEFAULT, new os::Message(M_MW_DEFAULT) ));
    pcHLButtons->SameWidth( "BApply", "BRevert", "BDefault", NULL );
    pcHLButtons->SameHeight( "BApply", "BRevert", "BDefault", NULL );
    pcVLRoot->AddChild( pcHLButtons );
  }

  // Set root and add to window
  pcLRoot->SetRoot(pcVLRoot);
  AddChild(pcLRoot);

  // Set Default button and initial focus
  if (bRoot) {
    this->SetDefaultButton(pcBApply);
    pcDDMNormal->MakeFocus();
  }
  
  // Set tab order
  int iTabOrder = 0;
  if (bRoot) {
    pcDDMNormal->SetTabOrder(iTabOrder++);
    pcSPNormal->SetTabOrder(iTabOrder++);
    pcDDMBold->SetTabOrder(iTabOrder++);
    pcSPBold->SetTabOrder(iTabOrder++);
    pcDDMFixed->SetTabOrder(iTabOrder++);
    pcSPFixed->SetTabOrder(iTabOrder++);
    pcDDMWindow->SetTabOrder(iTabOrder++);
    pcSPWindow->SetTabOrder(iTabOrder++);
    pcDDMTWindow->SetTabOrder(iTabOrder++);
    pcSPTWindow->SetTabOrder(iTabOrder++);
	pcCBAntiAliasing->SetTabOrder(iTabOrder++);
    pcBApply->SetTabOrder(iTabOrder++);
    pcBRevert->SetTabOrder(iTabOrder++);
    pcBDefault->SetTabOrder(iTabOrder++);
  }

  // Set fields to disabled for non-root
  if (!bRoot) {
    this->SetTitle("Font (View)");
    pcDDMNormal->SetEnable(false);
    pcSPNormal->SetEnable(false);
    pcDDMBold->SetEnable(false);
    pcSPBold->SetEnable(false);
    pcDDMFixed->SetEnable(false);
    pcSPFixed->SetEnable(false);
    pcDDMWindow->SetEnable(false);
    pcSPWindow->SetEnable(false);
    pcDDMTWindow->SetEnable(false);
    pcSPTWindow->SetEnable(false);
  }
  
  
  // Set Icon
  os::Resources cCol( get_image_id() );
  os::ResStream *pcStream = cCol.GetResourceStream( "icon48x48.png" );
  os::BitmapImage *pcIcon = new os::BitmapImage( pcStream );
  SetIcon( pcIcon->LockBitmap() );
  delete( pcIcon );

  // Show data
  GetCurrentValues();
  ShowData();
  ResizeTo(pcVLRoot->GetPreferredSize(false));  //simple way of making sure the window is no bigger than it needs to be

}

void MainWindow::GetCurrentValues()
{
  // Get defaults
  std::vector<os::String> cConfigList;
  os::Font::GetConfigNames( &cConfigList );
  for (uint i=0;i< cConfigList.size(); i++) {

    // Get the default font
    os::font_properties pcConfig;
    os::Font::GetDefaultFont( cConfigList[i], &pcConfig);

    // Figure out what the internal name applies to i.e System/Regular = Normal
    if (cConfigList[i]=="System/Regular") {
      cNorDefFam = pcConfig.m_cFamily;
      cNorDefSty = pcConfig.m_cStyle;
      cNorDefSiz = pcConfig.m_vSize;
      if( pcConfig.m_nFlags & os::FPF_SMOOTHED ) {
      		m_bAntiAliasing = true;
      }
    } else if (cConfigList[i]=="System/Bold") {
      cBldDefFam = pcConfig.m_cFamily;
      cBldDefSty = pcConfig.m_cStyle;
      cBldDefSiz = pcConfig.m_vSize;
    } else if (cConfigList[i]=="System/Fixed") {
      cFxdDefFam = pcConfig.m_cFamily;
      cFxdDefSty = pcConfig.m_cStyle;
      cFxdDefSiz = pcConfig.m_vSize;
    } else if (cConfigList[i]=="System/Window") {
      cWndDefFam = pcConfig.m_cFamily;
      cWndDefSty = pcConfig.m_cStyle;
      cWndDefSiz = pcConfig.m_vSize;
    } else if (cConfigList[i]=="System/ToolWindow") {
      cTWdDefFam = pcConfig.m_cFamily;
      cTWdDefSty = pcConfig.m_cStyle;
      cTWdDefSiz = pcConfig.m_vSize;
    } else {
      fprintf(stdout, "Error - Unknown font type [%s], can not continue, contact daryl.dudey@ntlworld.com\n", cConfigList[i].c_str() );
      exit(1);
    }
  }
}

MainWindow::~MainWindow()
{
}

void MainWindow::ShowData() 
{
  // Get number of font families
  int iFamilyCount = os::Font::GetFamilyCount();

  // Clear dropdowns
  pcDDMNormal->Clear();
  pcDDMBold->Clear();
  
  bool bUpdateSet = false;

  for ( int i=0 ; i<iFamilyCount; i++ ) {
    char zFamily[FONT_FAMILY_LENGTH+1];

    os::Font::GetFamilyInfo( i, zFamily );
    
    // See if fixed/bold etc available
    int iStyleCount = os::Font::GetStyleCount(zFamily);

    // Get styles i.e. bold etc
    for ( int j=0; j<iStyleCount; j++) {
      char zStyle[FONT_STYLE_LENGTH+1];
      char zCombined[FONT_FAMILY_LENGTH+FONT_STYLE_LENGTH+2];
      uint32 nFlags;
      bool bFixed = true;

      os::Font::GetStyleInfo( zFamily, j, zStyle, &nFlags);
      sprintf(zCombined, "%s-%s", zFamily, zStyle);
      

      // Check if fixed font
      if ( !(nFlags & os::FONT_IS_FIXED)) {
	bFixed = false;
      }
      
    
      // Add to normal
      pcDDMNormal->AppendItem(zCombined);
      if ( strcmp(zFamily,cNorDefFam.c_str())==0 && strcmp(zStyle,cNorDefSty.c_str())==0 ) {
	pcDDMNormal->SetSelection( pcDDMNormal->GetItemCount()-1, !bUpdateSet );
	bUpdateSet = true;
      }

      // And bold
      pcDDMBold->AppendItem(zCombined);
      if ( strcmp(zFamily,cBldDefFam.c_str())==0 && strcmp(zStyle,cBldDefSty.c_str())==0 ) {
	pcDDMBold->SetSelection( pcDDMBold->GetItemCount()-1, !bUpdateSet );
	bUpdateSet = true;
      }

      // And Fixed
      if (bFixed==true || bFixed==false) {
	pcDDMFixed->AppendItem(zCombined);
	if ( strcmp(zFamily,cFxdDefFam.c_str())==0 && strcmp(zStyle,cFxdDefSty.c_str())==0) {
	  pcDDMFixed->SetSelection( pcDDMFixed->GetItemCount()-1, !bUpdateSet );
	  bUpdateSet = true;
	}
      }
      
      // And Window
      pcDDMWindow->AppendItem(zCombined);
      if ( strcmp(zFamily,cWndDefFam.c_str())==0 && strcmp(zStyle,cWndDefSty.c_str())==0) {
	pcDDMWindow->SetSelection( pcDDMWindow->GetItemCount()-1, !bUpdateSet );
	bUpdateSet = true;
      }

      // And ToolWindow
      pcDDMTWindow->AppendItem(zCombined);
      if ( strcmp(zFamily,cTWdDefFam.c_str())==0 && strcmp(zStyle,cTWdDefSty.c_str())==0) {
	pcDDMTWindow->SetSelection( pcDDMTWindow->GetItemCount()-1, !bUpdateSet );
	bUpdateSet = true;
      }
    }  
  }

  // Now set the spinners for size properly
  pcSPNormal->SetValue( (double)cNorDefSiz );
  pcSPBold->SetValue( (double)cBldDefSiz );
  pcSPFixed->SetValue( (double)cFxdDefSiz );
  pcSPWindow->SetValue( (double)cWndDefSiz );
  pcSPTWindow->SetValue( (double)cTWdDefSiz );
  
  pcCBAntiAliasing->SetValue( m_bAntiAliasing );

}

void MainWindow::Apply() 
{
  // Create a font properties variable and populate as needed
  os::font_properties cFontProps;

  // Normal
  if (DecodeFamilyStyle(pcDDMNormal->GetCurrentString().c_str(), (float)pcSPNormal->GetValue(), &cFontProps)==true) {
    os::Font::SetDefaultFont("System/Regular", cFontProps);
  }

  // Bold
  if (DecodeFamilyStyle(pcDDMBold->GetCurrentString().c_str(), (float)pcSPBold->GetValue(), &cFontProps)==true) {
    os::Font::SetDefaultFont("System/Bold", cFontProps);
  }

  // Fixed
  if (DecodeFamilyStyle(pcDDMFixed->GetCurrentString().c_str(), (float)pcSPFixed->GetValue(), &cFontProps)==true) {
    os::Font::SetDefaultFont("System/Fixed", cFontProps);
  }

  // Window
  if (DecodeFamilyStyle(pcDDMWindow->GetCurrentString().c_str(), (float)pcSPWindow->GetValue(), &cFontProps)==true) {
    os::Font::SetDefaultFont("System/Window", cFontProps);
  }

  // Toolwindow
  if (DecodeFamilyStyle(pcDDMTWindow->GetCurrentString().c_str(), (float)pcSPTWindow->GetValue(), &cFontProps)==true) {
    os::Font::SetDefaultFont("System/ToolWindow", cFontProps);
  }

}

void MainWindow::Revert()
{
  GetCurrentValues();
  ShowData();
}

void MainWindow::Default()
{
  // Set sane system defaults
  cNorDefFam = "DejaVu Sans";
  cNorDefSty = "Book";
  cNorDefSiz = 8.5f;

  cBldDefFam = "DejaVu Sans";
  cBldDefSty = "Bold";
  cBldDefSiz = 8.0f;

  cFxdDefFam = "DejaVu Sans Mono";
  cFxdDefSty = "Book";
  cFxdDefSiz = 8.5f;

  cWndDefFam = "DejaVu Sans";
  cWndDefSty = "Condensed";
  cWndDefSiz = 8.0f;

  cTWdDefFam = "DejaVu Sans";
  cTWdDefSty = "Condensed";
  cTWdDefSiz = 7.0f;

  // And redisplay drop downs
  ShowData();
}

void MainWindow::UpdateExample(const char *pzFont , float fNewSize) 
{
  static std::string cFontName = "";
  static float fSize = 0;

  if( fNewSize != 0 ) {
  	fSize = fNewSize;
  	cFontName = pzFont;
  }

  if (bRoot) {
    // Create properties variable to populate
    os::font_properties cFontProps;

    // Now populate from dropdown and spinner
    if (DecodeFamilyStyle(cFontName.c_str(), fSize, &cFontProps)==true) {

      // Create font
      os::Font *pcFont = new os::Font(cFontProps);

      // Now update stringview
      pcSVExample->SetFont(pcFont);
      pcSVExample->SetString("aAbBcCdD 1234 !$%^ []{}");
      pcFont->Release();      
    }
  }
}

bool MainWindow::DecodeFamilyStyle(const char *pzFont, float fSize, os::font_properties *pcFontProps)
{
  // This whole section of code could probably do with optimising, its continually repeating the same task
 
  if (strlen(pzFont)==0) 
    return false;
    
  // Get end of first parameter (family)
  unsigned int iEnd;
  for (iEnd = 0; iEnd<strlen(pzFont) ; iEnd++) {
    if (iEnd==strlen(pzFont) || pzFont[iEnd]=='-')
      break;
  }    

  // If end of string, quit, malformed family string
  if (iEnd==strlen(pzFont))
    return false;

  // Create a font properties struct with all needed data
  char pzFamily[FONT_FAMILY_LENGTH+1];
  char pzStyle[FONT_STYLE_LENGTH+1];

  // Copy family string
  strncpy((char *)pzFamily, pzFont, iEnd);
  pzFamily[iEnd] = 0;
  pcFontProps->m_cFamily = pzFamily;

  // Copy style string
  strncpy((char *)pzStyle, pzFont+iEnd+1, strlen(pzFont)-iEnd);
  pcFontProps->m_cStyle = pzStyle;

  // Set size
  pcFontProps->m_vSize = fSize;
  
  if( m_bAntiAliasing ) {
  	pcFontProps->m_nFlags |= os::FPF_SMOOTHED;
  }

  return true;
}

void MainWindow::HandleMessage(os::Message* pcMessage)
{
  int iMessage = pcMessage->GetCode();

  // Get message code and act on it
  switch(iMessage) {

  case M_MW_DCNOR:
    UpdateExample(pcDDMNormal->GetCurrentString().c_str(), (float)pcSPNormal->GetValue());
    break;

  case M_MW_DCBLD:
    UpdateExample(pcDDMBold->GetCurrentString().c_str(), (float)pcSPBold->GetValue());
    break;

  case M_MW_DCFXD:
    UpdateExample(pcDDMFixed->GetCurrentString().c_str(), (float)pcSPFixed->GetValue());
    break;

  case M_MW_DCWND:
    UpdateExample(pcDDMWindow->GetCurrentString().c_str(), (float)pcSPWindow->GetValue());
    break;

  case M_MW_DCTWD:
    UpdateExample(pcDDMTWindow->GetCurrentString().c_str(), (float)pcSPTWindow->GetValue());
    break;

  case M_MW_SPNORMAL:
    UpdateExample(pcDDMNormal->GetCurrentString().c_str(), (float)pcSPNormal->GetValue());
    break;

  case M_MW_SPBOLD:
    UpdateExample(pcDDMBold->GetCurrentString().c_str(), (float)pcSPBold->GetValue());
    break;

  case M_MW_SPFIXED:
    UpdateExample(pcDDMFixed->GetCurrentString().c_str(), (float)pcSPFixed->GetValue());
    break;

  case M_MW_SPWINDOW:
    UpdateExample(pcDDMWindow->GetCurrentString().c_str(), (float)pcSPWindow->GetValue());
    break;

  case M_MW_SPTWINDOW:
    UpdateExample(pcDDMTWindow->GetCurrentString().c_str(), (float)pcSPTWindow->GetValue());
    break;

  case M_MW_ANTIALIASING:
	m_bAntiAliasing = pcCBAntiAliasing->GetValue();
	UpdateExample();
	break;

  case M_MW_APPLY:
    Apply();
    break;

  case M_MW_REVERT:
    Revert();
    break;

  case M_MW_DEFAULT:
    Default();
    break;

  default:
    os::Window::HandleMessage(pcMessage); 
    break;
  }

}

bool MainWindow::OkToQuit()
{
  os::Application::GetInstance()->PostMessage(os::M_QUIT);
  return true;
}






