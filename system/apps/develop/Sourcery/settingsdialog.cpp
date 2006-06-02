//  Sourcery -:-  (C)opyright 2003-2004 Rick Caudill
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

#include "settingsdialog.h"
#include "messages.h"
#include "commonfuncs.h"
#include "strings.h"
#include "fonthandle.h"

/*************************************************************
* Description: DialogHighlighter is a little view that Shows the name of the
*			   Settings section that you are in.
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 21:46:05 
*************************************************************/
DialogHighlighter::DialogHighlighter(const String& cName) : View(Rect(),"highlighter")
{
	cHighlight = cName;
	Font* pcFont = GetFont();
	pcFont->SetSize(14.0f);
	SetFont(pcFont);	
}

/*************************************************************
* Description: Simple Paint()
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 21:47:16 
*************************************************************/
void DialogHighlighter::Paint(const Rect& cUpdateRect)
{
	SetFgColor(get_default_color(COL_SHADOW));
	SetBgColor(get_default_color(COL_SHADOW));
	FillRect(GetBounds());	
	SetDrawingMode(DM_OVER);
	SetFgColor(Color32_s(0,0,0));
	DrawText(GetBounds(),cHighlight,DTF_CENTER);
}


/*************************************************************
* Description: Simple window that contains a colorselector and two buttons
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 21:47:35 
*************************************************************/
ColorSelectorWindow::ColorSelectorWindow(int nType,Color32_s sColor,Window* pcWindow) : Window(Rect(0,0,250,200),"color_window","Choose a color...")
{
	pcParentWindow = pcWindow;
	nControl = nType;
	sControlColor = sColor;
	
	pcLayoutView = new LayoutView(GetBounds(),"color_selector_layout");
	
	Layout();
	SetDefaultButton(pcOkButton);
}

/*************************************************************
* Description: Lays out the colorselector window
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 21:49:32 
*************************************************************/
void ColorSelectorWindow::Layout()
{
	HLayoutNode* pcButtonNode = new HLayoutNode("colors_button_node");
	pcButtonNode->AddChild(pcOkButton = new Button(Rect(),"colorselector_ok_but","_Okay",new Message(M_COLOR_SELECTOR_WINDOW_OK)));
	pcButtonNode->AddChild(new HLayoutSpacer("",5,5));
	pcButtonNode->AddChild(pcCancelButton = new Button(Rect(),"colorselector_cancel_but","_Cancel",new Message(M_COLOR_SELECTOR_WINDOW_CANCEL)));
	pcButtonNode->SameWidth("colorselector_ok_but","colorselector_cancel_but",NULL);	
	pcButtonNode->LimitMaxSize(pcButtonNode->GetPreferredSize(false));
	
	VLayoutNode* pcNode = new VLayoutNode("col_node");
	pcNode->AddChild(pcSelector = new ColorSelector(GetBounds(),"col_select",NULL));
	pcSelector->SetValue(sControlColor);
	pcNode->AddChild(new VLayoutSpacer("",5,5));
	pcNode->AddChild(pcButtonNode);

	pcLayoutView->SetRoot(pcNode);
	AddChild(pcLayoutView);
}

/*************************************************************
* Description: Simple function that gets called when the selector wants to
*			   close.
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 21:49:53 
*************************************************************/
bool ColorSelectorWindow::OkToQuit()
{
	pcParentWindow->PostMessage(new Message(M_COLOR_SELECTOR_WINDOW_CANCEL),pcParentWindow);
	return true;
}

/*************************************************************
* Description: HandleMessage() Handles all the messages passed
*			   though the window.
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 21:51:32 
*************************************************************/
void ColorSelectorWindow::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_COLOR_SELECTOR_WINDOW_OK:
		{
			Message* pcMsg = new Message(M_COLOR_SELECTOR_WINDOW_REQUESTED);
			pcMsg->AddInt32("control",nControl);
			pcMsg->AddColor32("color",pcSelector->GetValue().AsColor32());
			pcParentWindow->PostMessage(pcMsg,pcParentWindow);			
			break;
		}
		
		case M_COLOR_SELECTOR_WINDOW_CANCEL:
		{
			OkToQuit();
			break;
		}
	}
}


SettingsDialogFont::SettingsDialogFont(FontHandle* pcHandle) : LayoutView(Rect(),"settings_view",NULL,CF_FOLLOW_ALL)
{
	pcFontHandle = GetNewFontHandle(pcHandle);
		
	LayoutSystem();
	SetupFonts();

	pcFontListView->SetSelChangeMsg(new Message(M_FONT_SETTINGS_FONT_CHANGED));
	pcFontFaceListView->SetSelChangeMsg(new Message(M_FONT_SETTINGS_FACE_CHANGED));
	pcFontSizeListView->SetSelChangeMsg(new Message(M_FONT_SETTINGS_SIZE_CHANGED));					

	InitialFont();
}

void SettingsDialogFont::InitialFont()
{
	font_properties sProps;
	sProps = GetFontProperties(pcFontHandle);
	
	String cFamily = sProps.m_cFamily;
	String cStyle = sProps.m_cStyle;
	float vSize = sProps.m_vSize;
	
	if ( sProps.m_nFlags  & FPF_SMOOTHED)
		pcAntiCheck->SetValue(true);
	
	for (int i=0; i<pcFontListView->GetRowCount(); i++)
	{
		ListViewStringRow* pcRow = (ListViewStringRow*)pcFontListView->GetRow(i);
		if (cFamily == pcRow->GetString(0))
		{
			pcFontListView->Select(i,true);
			LoadFontTypes();
			for (int j=0; j<pcFontFaceListView->GetRowCount(); j++)
			{
				ListViewStringRow* pcRowTwo = (ListViewStringRow*)pcFontFaceListView->GetRow(j);
				if (cStyle == pcRowTwo->GetString(0))
				{
					pcFontFaceListView->Select(j);
				}
			}
		}
	}
	
	for (int i=0; i<pcFontSizeListView->GetRowCount(); i++)
	{
		
		ListViewStringRow* pcRow = (ListViewStringRow*)pcFontSizeListView->GetRow(i);
		String cSize;
		cSize.Format("%.1f",ceil(vSize));
		if (cSize == pcRow->GetString(0))
		{
			pcFontSizeListView->Select(i);
		}
	}
	
	sProps.m_vSize = ceil(sProps.m_vSize);  //must run ceiling of it
	Font* pcFont = new Font(sProps);
	pcTextStringView->SetFont(pcFont);
	pcFont->Release();		
}

void SettingsDialogFont::LayoutSystem()
{
	pcRoot = new VLayoutNode("root_node");
	
	DialogHighlighter* pcHighlighter = new DialogHighlighter(M_SETTINGS_FONTS);
	pcRoot->AddChild(pcHighlighter,0.45);
	
	pcFontListView = new ListView(Rect(),"font_types_list",ListView::F_RENDER_BORDER | ListView::F_NO_HEADER | ListView::F_NO_AUTO_SORT);
	pcFontListView->InsertColumn("Font",(int)GetBounds().Height());

	pcFontFaceListView = new ListView(Rect(),"font_face_list",ListView::F_RENDER_BORDER | ListView::F_NO_HEADER | ListView::F_NO_AUTO_SORT);
	pcFontFaceListView->InsertColumn("Face",(int)GetBounds().Height());

	pcFontSizeListView = new ListView(Rect(),"font_size_list",ListView::F_RENDER_BORDER | ListView::F_NO_HEADER | ListView::F_NO_AUTO_SORT);
	pcFontSizeListView->InsertColumn("Size",(int)GetBounds().Height());

	HLayoutNode* pcRotationNode = new HLayoutNode("rotation_node");
	pcRotationNode->AddChild(pcRotationString = new StringView(Rect(),"rotation_string","Rotate:"));
	pcRotationNode->AddChild(new HLayoutSpacer("",2,2));
	pcRotationNode->AddChild(pcRotationSpinner = new Spinner(Rect(),"",0,new Message(M_FONT_SETTINGS_ROTATE_CHANGED)));
	pcRotationSpinner->SetMinValue(0);
	pcRotationSpinner->SetMaxValue(360);
	pcRotationSpinner->SetStep(1.0);
	pcRotationSpinner->SetValue(pcFontHandle->vRotation);
	
	HLayoutNode* pcShearNode = new HLayoutNode("shear_node");
	pcShearNode->AddChild(pcShearString = new StringView(Rect(),"shear_string","Shear:"));
	pcShearNode->AddChild(new HLayoutSpacer("",2,2));
	pcShearNode->AddChild(pcShearSpinner = new Spinner(Rect(),"",0,new Message(M_FONT_SETTINGS_SHEAR_CHANGED)));
	pcShearSpinner->SetMinValue(0);
	pcShearSpinner->SetMaxValue(360);
	pcShearSpinner->SetStep(1.0);
	pcShearSpinner->SetValue(pcFontHandle->vShear);
	
	VLayoutNode* pcOtherFontNode = new VLayoutNode("other_node");
	pcOtherFontNode->AddChild(pcAntiCheck = new CheckBox(Rect(),"anti_check","A_ntiAlias",new Message(M_FONT_SETTINGS_ALIAS_CHANGED)));
	pcAntiCheck->SetValue(pcFontHandle->nFlags & FPF_SMOOTHED ? true : false);
	pcOtherFontNode->AddChild(new VLayoutSpacer("",30,30));
	pcOtherFontNode->AddChild(pcRotationNode);
	pcOtherFontNode->AddChild(new VLayoutSpacer("",10,10));
	pcOtherFontNode->AddChild(pcShearNode);
	pcOtherFontNode->LimitMaxSize(Point(pcOtherFontNode->GetPreferredSize(false).x+70,pcOtherFontNode->GetPreferredSize(false).y));
	
	HLayoutNode* pcFontNode = new HLayoutNode("");
	pcFontNode->AddChild(new HLayoutSpacer("",5,5));
	pcFontNode->AddChild(pcFontListView);
	pcFontNode->AddChild(new HLayoutSpacer("",2,2));
	pcFontNode->AddChild(pcFontFaceListView);
	pcFontNode->AddChild(new HLayoutSpacer("",2,2));
	pcFontNode->AddChild(pcFontSizeListView);
	pcFontNode->AddChild(new HLayoutSpacer("",5,5));
	pcFontNode->AddChild(pcOtherFontNode);
	
	pcTextFrameView = new FrameView(Rect(),"text_farme","");
	HLayoutNode* pcTextNode = new HLayoutNode("");
	pcTextNode->SetHAlignment(ALIGN_CENTER);
	pcTextNode->AddChild(pcTextStringView = new StringView(Rect(),"","This Is A Test String",ALIGN_CENTER));
	pcTextNode->LimitMaxSize(pcTextNode->GetPreferredSize(true));
	pcTextFrameView->SetRoot(pcTextNode);
	
	pcRoot->AddChild(new VLayoutSpacer("",5,5));
	pcRoot->AddChild(pcFontNode);
	pcRoot->AddChild(new VLayoutSpacer("",5,5));
	pcRoot->AddChild(pcTextFrameView);	
	
	SetRoot( pcRoot );
}


void SettingsDialogFont::SetupFonts()
{
	float sz,del=1.0;
	char zSizeLabel[8];

	for (sz=5.0; sz < 30.0; sz += del)
	{
		ListViewStringRow* pcRow = new ListViewStringRow();
		sprintf(zSizeLabel,"%.1f",sz);
		pcRow->AppendString(zSizeLabel);
		pcFontSizeListView->InsertRow(pcRow);
		
		if (sz == 10.0)
			del = 2.0;
		else if (sz == 16.0)
			del = 4.0;
   }
   
   	// Add Family
	int fc = Font::GetFamilyCount();
	int i=0;
	char zFFamily[FONT_FAMILY_LENGTH];

	for (i=0; i<fc; i++)
	{
		if (Font::GetFamilyInfo(i,zFFamily) == 0)
		{
			Font::GetStyleCount(zFFamily);
			ListViewStringRow* pcRow = new ListViewStringRow();
			pcRow->AppendString(zFFamily);
			pcFontListView->InsertRow(pcRow);			
		}
	}	
}

void SettingsDialogFont::LoadFontTypes()
{
	ListViewStringRow* pcFamilyRow = (ListViewStringRow*)pcFontListView->GetRow(pcFontListView->GetLastSelected());
	String cFamily = pcFamilyRow->GetString(0);
	char zFStyle[FONT_STYLE_LENGTH];
	int i; 
	int nStyleCount=0;
	uint32 nFlags;
	nStyleCount = Font::GetStyleCount(cFamily.c_str());
	
	pcFontFaceListView->Clear();	
	
	for (i=0; i<nStyleCount; i++)
	{
		if (Font::GetStyleInfo(cFamily.c_str(),i,zFStyle,&nFlags) == 0)
		{
			ListViewStringRow* pcRow = new ListViewStringRow();
			pcRow->AppendString(zFStyle);
			pcFontFaceListView->InsertRow(pcRow);
		}
	}
}

void SettingsDialogFont::FontHasChanged()
{
	pcFontFaceListView->SetSelChangeMsg(NULL);
	pcFontFaceListView->Select(0);
	
	
	ListViewStringRow* pcRowTwo = (ListViewStringRow*) pcFontListView->GetRow(pcFontListView->GetLastSelected());
	strcpy(pcFontHandle->zFamilyName,pcRowTwo->GetString(0).c_str());
	
	ListViewStringRow* pcRowThree = (ListViewStringRow*) pcFontFaceListView->GetRow(0);
	strcpy(pcFontHandle->zStyleName,pcRowThree->GetString(0).c_str());
	
	Font* pcFont = new Font(GetFontProperties(pcFontHandle));
	pcTextStringView->SetFont(pcFont);
	pcTextStringView->SetString("This Is A Test String");
	pcFont->Release();
	
	pcFontFaceListView->SetSelChangeMsg(new Message(M_FONT_SETTINGS_FACE_CHANGED));
}

void SettingsDialogFont::SizeHasChanged()
{
	ListViewStringRow* pcRow = (ListViewStringRow*)pcFontSizeListView->GetRow(pcFontSizeListView->GetLastSelected());
	pcFontHandle->vSize = atof(pcRow->GetString(0).c_str());
	Font* pcFont = new Font(GetFontProperties(pcFontHandle));	
	
	pcTextStringView->SetFont(pcFont);
	pcTextStringView->SetString("This Is A Test String");
}

void SettingsDialogFont::FaceHasChanged()
{
	ListViewStringRow* pcRow = (ListViewStringRow*)pcFontFaceListView->GetRow(pcFontFaceListView->GetLastSelected());
	strcpy(pcFontHandle->zStyleName,pcRow->GetString(0).c_str());
	
	Font* pcFont = new Font(GetFontProperties(pcFontHandle));
	
	pcTextStringView->SetFont(pcFont);
	pcTextStringView->SetString("This Is A Test String");			
}

void SettingsDialogFont::AliasChanged()
{
	if (pcAntiCheck->GetValue().AsBool() == true)
		pcFontHandle->nFlags = FPF_SYSTEM | FPF_SMOOTHED;
	else
		pcFontHandle->nFlags = FPF_SYSTEM;
	
	Font* pcFont = new Font(GetFontProperties(pcFontHandle));
	
	pcTextStringView->SetFont(pcFont);
	pcTextStringView->SetString("This Is A Test String");		
}

void SettingsDialogFont::ShearChanged()
{
	pcFontHandle->vShear = pcShearSpinner->GetValue().AsFloat();
	
	Font* pcFont = new Font(GetFontProperties(pcFontHandle));
	
	pcTextStringView->SetFont(pcFont);
	pcTextStringView->SetString("This Is A Test String");		
}

void SettingsDialogFont::RotateChanged()
{
	pcFontHandle->vRotation = pcRotationSpinner->GetValue().AsFloat();
	
	Font* pcFont = new Font(GetFontProperties(pcFontHandle));
	
	pcTextStringView->SetFont(pcFont);
	pcTextStringView->SetString("This Is A Test String");		
}

FontHandle* SettingsDialogFont::GetFontHandle()
{
	return pcFontHandle;
}

SettingsDialogFont::~SettingsDialogFont()
{
}

SettingsDialogGeneral::SettingsDialogGeneral() : LayoutView(Rect(),"settings_view",NULL,CF_FOLLOW_ALL)
{
	LayoutSystem();
}

SettingsDialogGeneral::~SettingsDialogGeneral()
{
}

void SettingsDialogGeneral::LayoutSystem()
{
	pcRoot = new VLayoutNode("root_node");
	
	pcHighlighter = new DialogHighlighter(M_SETTINGS_GENERAL);
	pcRoot->AddChild(pcHighlighter,1.0);
	
	HLayoutNode* pcLayoutOne = new HLayoutNode("hlayout_one");
	pcLayoutOne->AddChild(new HLayoutSpacer("",5,5));
	pcLayoutOne->AddChild(pcSplashCheck = new CheckBox(Rect(),"splash_check","Display Splash Screen",NULL));
	pcLayoutOne->AddChild(new HLayoutSpacer("",20,20));
	pcLayoutOne->AddChild(pcAdvancedGoToCheck = new CheckBox(Rect(),"advanced_goto","Show Advanced GoTo Dialog",NULL));	
	pcRoot->AddChild(pcLayoutOne);
	
	HLayoutNode* pcLayoutTwo = new HLayoutNode("hlayout_two");
	pcLayoutTwo->AddChild(new HLayoutSpacer("",5,5));
	pcLayoutTwo->AddChild(pcBackupCheck = new CheckBox(Rect(),"backup_check","Backup the opened file",NULL));
	pcLayoutTwo->AddChild(new HLayoutSpacer("",20,20));
	pcLayoutTwo->AddChild(pcMonitorCheck = new CheckBox(Rect(),"monitor_check","Monitor the opened file",NULL));
	pcRoot->AddChild(pcLayoutTwo);
	
	HLayoutNode* pcLayoutThree = new HLayoutNode("hlayout_three");
	pcLayoutThree->AddChild(new HLayoutSpacer("",5,5));
	pcLayoutThree->AddChild(pcSaveCursorCheck = new CheckBox(Rect(),"cursor_check","Save cursor position",NULL));
	pcLayoutThree->AddChild(new HLayoutSpacer("",20,20));
	pcLayoutThree->AddChild(pcFoldCheck = new CheckBox(Rect(),"fold_check","Fold code when opening",NULL));
	pcRoot->AddChild(pcLayoutThree);
	
	HLayoutNode* pcLayoutFour = new HLayoutNode("hlayout_four");
	pcLayoutFour->AddChild(new HLayoutSpacer("",5,5));
	pcLayoutFour->AddChild(pcConvertLFCheck = new CheckBox(Rect(),"convert_lf_check","Convert CRLF to LF",NULL));
	pcLayoutFour->AddChild(new HLayoutSpacer("",20,20));
	pcLayoutFour->AddChild(pcSaveWindowCheck = new CheckBox(Rect(),"window_check","Save Window position and size",NULL));
	pcRoot->AddChild(pcLayoutFour);
	
	HLayoutNode* pcLayoutFive = new HLayoutNode("hlayout_five");
	pcLayoutFive->AddChild(new HLayoutSpacer("",5,5));
	pcLayoutFive->AddChild(pcFileMinCheck = new CheckBox(Rect(),"file_min_check","Save the file every minute",NULL));
	pcLayoutFive->AddChild(new HLayoutSpacer("",20,20));
	pcLayoutFive->AddChild(pcLineCheck = new CheckBox(Rect(),"line_check","Show line numbers",NULL));
	pcRoot->AddChild(pcLayoutFive);
	
	pcRoot->AddChild(new VLayoutSpacer("",10,10));
	
	pcRoot->SameWidth("splash_check","backup_check","cursor_check","convert_lf_check","file_min_check",NULL);	

	SetRoot( pcRoot );
}

void SettingsDialogGeneral::Init(AppSettings* pcSettings)
{	
	pcSplashCheck->SetValue(pcSettings->GetSplashScreen());
	pcAdvancedGoToCheck->SetValue(pcSettings->GetAdvancedGoToDialog());
	pcBackupCheck->SetValue(pcSettings->GetBackup());
	pcMonitorCheck->SetValue(pcSettings->GetMonitorFile());
	pcSaveCursorCheck->SetValue(pcSettings->GetCursorPosition());
	pcFoldCheck->SetValue(pcSettings->GetFoldCode());
	pcSaveWindowCheck->SetValue(pcSettings->GetSizeAndPositionOption());
	pcConvertLFCheck->SetValue(pcSettings->GetConvertToLf());
	pcFileMinCheck->SetValue(pcSettings->GetSaveFileEveryMin());
	pcLineCheck->SetValue(pcSettings->GetShowLineNumbers()); 
}


void SettingsDialogGeneral::SaveSettings(AppSettings* pcSettings)
{
	pcSettings->SetSplashScreen(pcSplashCheck->GetValue().AsBool());
	pcSettings->SetAdvancedGoToDialog(pcAdvancedGoToCheck->GetValue());
	pcSettings->SetBackup(pcBackupCheck->GetValue());
	pcSettings->SetMonitorFile(pcMonitorCheck->GetValue());
	pcSettings->SetCursorPosition(pcSaveCursorCheck->GetValue());
	pcSettings->SetFoldCode(pcFoldCheck->GetValue());
	pcSettings->SetSizeAndPositionOption(pcSaveWindowCheck->GetValue());
	pcSettings->SetConvertToLf(pcConvertLFCheck->GetValue());
	pcSettings->SetSaveFileEveryMin(pcFileMinCheck->GetValue());
	pcSettings->SetShowLineNumbers(pcLineCheck->GetValue());
}

SettingsDialogShortcuts::SettingsDialogShortcuts() : LayoutView(Rect(),"settings_view",NULL,CF_FOLLOW_ALL)
{
	pcRoot = new VLayoutNode("root_node");
	
	DialogHighlighter* pcHighlighter = new DialogHighlighter(M_SETTINGS_SHORTCUTS);
	pcRoot->AddChild(pcHighlighter,0.14);
	pcRoot->AddChild(pcError = new StringView(Rect(),"error_plugins","User defined shortcuts will be in the next version.",ALIGN_CENTER));
	pcRoot->SetVAlignment(ALIGN_TOP);
	pcRoot->SetHAlignment(ALIGN_CENTER);
	
	SetRoot( pcRoot );
}

SettingsDialogShortcuts::~SettingsDialogShortcuts()
{
}

SettingsDialogPlugins::SettingsDialogPlugins() : LayoutView(Rect(),"settings_view",NULL,CF_FOLLOW_ALL)
{
	pcRoot = new VLayoutNode("root_node");
	
	DialogHighlighter* pcHighlighter = new DialogHighlighter(M_SETTINGS_PLUGINS);
	pcRoot->AddChild(pcHighlighter,0.14);
	pcRoot->AddChild(pcError = new StringView(Rect(),"error_plugins","Plugins have not been implemented yet.",ALIGN_CENTER));
	pcRoot->SetVAlignment(ALIGN_TOP);
	pcRoot->SetHAlignment(ALIGN_CENTER);
	
	SetRoot( pcRoot );
}


SettingsDialogPlugins::~SettingsDialogPlugins()
{
}


SettingsDialogColors::SettingsDialogColors() : LayoutView(Rect(),"settings_view",NULL,CF_FOLLOW_ALL)
{
	LayoutSystem();
	pcColorSelectorWindow = NULL;
}

SettingsDialogColors::~SettingsDialogColors()
{
}

void SettingsDialogColors::LayoutSystem()
{
	pcRoot = new VLayoutNode("root_node");
	
	DialogHighlighter* pcHighlighter = new DialogHighlighter(M_SETTINGS_COLORS);
	pcRoot->AddChild(pcHighlighter,0.1);
		
	HLayoutNode* pcNumberColorNode = new HLayoutNode("line_layout");
	Message* pcMessageOne = new Message(M_COLOR_SELECTOR);	
	pcMessageOne->AddInt32("control",1);	
	pcNumberColorNode->AddChild(pcNumberColorButton = new ColorButton(Rect(),"line_color",pcMessageOne,Color32_s(255,255,255)));
	pcNumberColorNode->AddChild(new HLayoutSpacer("",10,10));
	pcNumberColorNode->AddChild(pcNumberColorString = new StringView(Rect(),"line_string","Line Number Color"));
	pcNumberColorNode->AddChild(new HLayoutSpacer("",150,150));	
	pcNumberColorNode->LimitMaxSize(Point(pcNumberColorNode->GetPreferredSize(false).x+25,pcNumberColorNode->GetPreferredSize(false).y+15));	
		
	HLayoutNode* pcBackColorNode = new HLayoutNode("back_layout");
	Message* pcMessageTwo = new Message(M_COLOR_SELECTOR);	
	pcMessageTwo->AddInt32("control",2);	
	pcBackColorNode->AddChild(pcBackColorButton = new ColorButton(Rect(),"back_color",pcMessageTwo,Color32_s(255,255,255)));
	pcBackColorNode->AddChild(new HLayoutSpacer("",10,10));
	pcBackColorNode->AddChild(pcBackColorString = new StringView(Rect(),"back_string","Background Color"));
	pcBackColorNode->AddChild(new HLayoutSpacer("",150,150));
	pcBackColorNode->LimitMaxSize(Point(pcBackColorNode->GetPreferredSize(false).x+25,pcBackColorNode->GetPreferredSize(false).y+15));
	
	HLayoutNode* pcTextColorNode = new HLayoutNode("text_layout");
	Message* pcMessageThree = new Message(M_COLOR_SELECTOR);	
	pcMessageThree->AddInt32("control",3);
	pcTextColorNode->AddChild(pcTextColorButton = new ColorButton(Rect(),"text_color",pcMessageThree,Color32_s(255,255,255)));
	pcTextColorNode->AddChild(new HLayoutSpacer("",10,10));
	pcTextColorNode->AddChild(pcTextColorString = new StringView(Rect(),"text_string","Text Color"));
	pcTextColorNode->AddChild(new HLayoutSpacer("",150,150));
	pcTextColorNode->LimitMaxSize(Point(pcTextColorNode->GetPreferredSize(false).x+25,pcTextColorNode->GetPreferredSize(false).y+15));
		
	HLayoutNode* pcHighlightColorNode = new HLayoutNode("highlight_layout");
	Message* pcMessageFour = new Message(M_COLOR_SELECTOR);	
	pcMessageFour->AddInt32("control",4);	
	pcHighlightColorNode->AddChild(pcHighlightColorButton = new ColorButton(Rect(),"highlight_color",pcMessageFour,Color32_s(255,255,255)));
	pcHighlightColorNode->AddChild(new HLayoutSpacer("",10,10));
	pcHighlightColorNode->AddChild(pcHighlightColorString = new StringView(Rect(),"highlight_string","Highlight Color"));
	pcHighlightColorNode->AddChild(new HLayoutSpacer("",150,150));
	pcHighlightColorNode->LimitMaxSize(Point(pcHighlightColorNode->GetPreferredSize(false).x+25,pcHighlightColorNode->GetPreferredSize(false).y+15));	
		
	pcRoot->AddChild(new VLayoutSpacer("",17,17));
	pcRoot->AddChild(pcBackColorNode);
	pcRoot->AddChild(new VLayoutSpacer("",15,15));
	pcRoot->AddChild(pcTextColorNode);
	pcRoot->AddChild(new VLayoutSpacer("",15,15));
	pcRoot->AddChild(pcHighlightColorNode);
	pcRoot->AddChild(new VLayoutSpacer("",15,15));
	pcRoot->AddChild(pcNumberColorNode);
	pcRoot->AddChild(new VLayoutSpacer("",15,15));
				
	pcRoot->SameWidth("line_color","text_color","highlight_color","back_color",NULL);			
	pcRoot->SameWidth("line_string","text_string","highlight_string","back_string",NULL);
	
	SetRoot( pcRoot );		
}

void SettingsDialogColors::LaunchColorSelectorWindow(int nColor,Color32_s sColor, Window* pcWindow)
{
	if (pcColorSelectorWindow == NULL)
	{
		pcColorSelectorWindow = new ColorSelectorWindow(nColor,sColor,pcWindow);
		pcColorSelectorWindow->CenterInWindow(pcWindow);
		pcColorSelectorWindow->Show();
		pcColorSelectorWindow->MakeFocus();
	}
}

void SettingsDialogColors::CancelColorSelectorWindow()
{
	pcColorSelectorWindow->Close();
	pcColorSelectorWindow = NULL;
}

void SettingsDialogColors::SetColors(AppSettings* pcSettings)
{
	pcBackColorButton->SetColor(pcSettings->GetBackColor());
	pcNumberColorButton->SetColor(pcSettings->GetNumberColor());
	pcHighlightColorButton->SetColor(pcSettings->GetHighColor());
	pcTextColorButton->SetColor(pcSettings->GetTextColor());
}

void SettingsDialogColors::ColorChanged(int nColor,Color32_s sColor)
{
	switch (nColor)
		{
			case 1:
			pcNumberColorButton->SetColor(sColor);			
			break;
				
			case 2:	
			pcBackColorButton->SetColor(sColor);	
			break;
				
			case 3:	
			pcTextColorButton->SetColor(sColor);	
			break;
			
			case 4:
			pcHighlightColorButton->SetColor(sColor);	
			break;
		}
}



SettingsDialog::SettingsDialog(Window* pcParent, AppSettings* pcAppSettings) : Window(Rect(0,0,500,360),"set_dialog","Sourcery Settings...",WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT | WND_NOT_RESIZABLE)
{
	pcSettings = pcAppSettings;
	pcParentWindow = pcParent;
	
	pcGeneral = new SettingsDialogGeneral();
	pcGeneral->Init(pcSettings);

	pcPlugins = new SettingsDialogPlugins();
	pcShortcuts = new SettingsDialogShortcuts();
	
	pcFonts = new SettingsDialogFont(pcSettings->GetFontHandle());
	
	pcColors = new SettingsDialogColors();
	pcColors->SetColors(pcSettings);

	Layout();
}

SettingsDialog::~SettingsDialog()
{
	pcLayoutNode->RemoveChild( pcLayoutNode->FindNode( "settings_view", false ) );
	delete( pcGeneral );
	delete( pcPlugins );
	delete( pcShortcuts );
	delete( pcFonts );
	delete( pcColors );
}

void SettingsDialog::Layout()
{
	
	pcButtonView = new LayoutView(GetBounds(),"button_view",NULL,CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_BOTTOM);
	pcButtonNode = new HLayoutNode("dialog_button_node");
	pcButtonNode->SetHAlignment(ALIGN_LEFT);
	pcButtonNode->SetVAlignment(ALIGN_LEFT);
	pcButtonNode->AddChild(new HLayoutSpacer("",340));
	pcButtonNode->AddChild(pcSaveButton = new Button(Rect(),"save_but","_Save",new Message(M_SETTINGS_SAVE),CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT));
	pcButtonNode->AddChild(new HLayoutSpacer("",5));
	pcButtonNode->AddChild(pcDefaultButton = new Button(Rect(),"default_but","_Default", new Message(M_SETTINGS_DEFAULT),CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT));
	pcButtonNode->AddChild(new HLayoutSpacer("",5));
	pcButtonNode->AddChild(pcCancelButton = new Button(Rect(),"cancel_but","_Cancel", new Message(M_SETTINGS_CANCEL),CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT));
	pcButtonNode->SameWidth("cancel_but","default_but","save_but",NULL);
	pcButtonView->SetRoot(pcButtonNode);	
	
	pcLayoutNode = new HLayoutNode("layoutnode");
	pcLayoutNode->SetVAlignment(ALIGN_CENTER);
	pcView = new FrameView(Rect(0,0,GetBounds().Width(),GetBounds().Height()),"layoutview","",CF_FOLLOW_ALL);
	pcRoot = new HLayoutNode("dialog_root");
	
	pcIconView = new IconView(Rect(0,0,48,GetBounds().Height()),"iconview",CF_FOLLOW_ALL);
	pcIconView->SetAutoSort(false);
	pcIconView->SetBackgroundColor(Color32_s(255,255,255));
	pcIconView->SetView(IconView::VIEW_ICONS);
	AddIcons();
	pcIconView->Layout();
	pcIconView->SetIconSelected(0,true);	
	pcIconView->SetSelChangeMsg(new Message(M_SETTINGS_ICONVIEW_CHANGED));	
	

	os::LayoutNode* pcNode = pcLayoutNode->AddChild(pcIconView);
	pcNode->ExtendMinSize( os::Point( 80, 48 ) );
	pcNode->LimitMaxSize( os::Point( 80, 1000 ) );
	pcLayoutNode->AddChild(pcGeneral);

	
	pcView->SetRoot(pcLayoutNode);
	pcView->ResizeTo(Point(GetBounds().Width(),360-pcButtonView->GetPreferredSize(false).y-4));
	
	pcButtonView->SetFrame(Rect(0,pcView->GetBounds().Height()+2,GetBounds().Width(),GetBounds().Height()));	
	AddChild(pcView);
	
	AddChild(pcButtonView);
	
	pcSaveButton->SetTabOrder(NEXT_TAB_ORDER);
	pcDefaultButton->SetTabOrder(NEXT_TAB_ORDER);
	pcCancelButton->SetTabOrder(NEXT_TAB_ORDER);
	
	SetFocusChild(pcSaveButton);
	SetDefaultButton(pcSaveButton);
	
}

void SettingsDialog::AddIcons()
{
	pcIconView->AddIcon(LoadImageFromResource("general.png"),new IconData());
	pcIconView->AddIconString(0,M_SETTINGS_GENERAL);
	
	pcIconView->AddIcon(LoadImageFromResource("fonts.png"),new IconData());
	pcIconView->AddIconString(1,M_SETTINGS_FONTS);	

	pcIconView->AddIcon(LoadImageFromResource("shortcuts.png"),new IconData());
	pcIconView->AddIconString(2,M_SETTINGS_SHORTCUTS);
		
	pcIconView->AddIcon(LoadImageFromResource("colors.png"),new IconData());
	pcIconView->AddIconString(3,M_SETTINGS_COLORS);
	
	pcIconView->AddIcon(LoadImageFromResource("plugins.png"),new IconData());
	pcIconView->AddIconString(4,M_SETTINGS_PLUGINS);
}

bool SettingsDialog::OkToQuit(void)
{
	pcParentWindow->PostMessage(new Message(M_SETTINGS_CANCEL),pcParentWindow);
	return true;
}

void SettingsDialog::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_SETTINGS_ICONVIEW_CHANGED:
		{
			pcView->SetRoot( NULL );
			
			if (pcIconView->GetIconSelected(0) == true)
			{
				pcLayoutNode->RemoveChild( pcLayoutNode->FindNode( "settings_view", false ) );
				pcLayoutNode->AddChild(pcGeneral);
			}
			
			if (pcIconView->GetIconSelected(1) == true)
			{
				pcLayoutNode->RemoveChild( pcLayoutNode->FindNode( "settings_view", false ) );
				pcLayoutNode->AddChild(pcFonts);	
			}

			if (pcIconView->GetIconSelected(2) == true)
			{
				pcLayoutNode->RemoveChild( pcLayoutNode->FindNode( "settings_view", false ) );
				pcLayoutNode->AddChild(pcShortcuts);			
			}
			
			if (pcIconView->GetIconSelected(3) == true)
			{
				pcLayoutNode->RemoveChild( pcLayoutNode->FindNode( "settings_view", false ) );
				pcLayoutNode->AddChild(pcColors);								
			}

			if (pcIconView->GetIconSelected(4) == true)
			{
				pcLayoutNode->RemoveChild( pcLayoutNode->FindNode( "settings_view", false ) );
				pcLayoutNode->AddChild(pcPlugins);					
			}
			pcView->SetRoot( pcLayoutNode );
			break;		
		}
		
		case M_SETTINGS_SAVE:
		{
			Save();
			break;
		}
		
		case M_SETTINGS_CANCEL:
		{
			OkToQuit();
			break;
		}
		
		case M_SETTINGS_DEFAULT:
		{
			pcSettings->LoadDefaults();
			Message* pcMessage = new Message(M_SETTINGS_SAVE);
			pcParentWindow->PostMessage(pcMessage, pcParentWindow);
			break;
		}
		
		case M_FONT_SETTINGS_FONT_CHANGED:
		{
			pcFonts->LoadFontTypes();
			pcFonts->FontHasChanged();
			pcFonts->Flush();
			break;
		}
		
		case M_FONT_SETTINGS_SIZE_CHANGED:
		{
			pcFonts->SizeHasChanged();
			break;
		}
		
		case M_FONT_SETTINGS_FACE_CHANGED:
		{
			pcFonts->FaceHasChanged();
			break;
		}
		
		case M_FONT_SETTINGS_ALIAS_CHANGED:
		{
			pcFonts->AliasChanged();
			break;
		}
		
		case M_FONT_SETTINGS_ROTATE_CHANGED:
		{
			pcFonts->RotateChanged();
			break;
		}
		
		case M_FONT_SETTINGS_SHEAR_CHANGED:
		{
			pcFonts->ShearChanged();
			break;
		}
		
		case M_COLOR_SELECTOR:
		{
			int nControl;
			pcMessage->FindInt("control",&nControl);
			Color32_s sColor=Color32_s(255,255,255);
			
			switch (nControl)
			{
				case 1:
				sColor = pcSettings->GetNumberColor();			
				break;
				
				case 2:	
				sColor = pcSettings->GetBackColor();
				break;
				
				case 3:	
				sColor = pcSettings->GetTextColor();
				break;
			
				case 4:
				sColor = pcSettings->GetHighColor();
				break;
			}
			
			pcColors->LaunchColorSelectorWindow(nControl,sColor,this);
			break;
		}
		
		case M_COLOR_SELECTOR_WINDOW_CANCEL:
		{
			pcColors->CancelColorSelectorWindow();
			break;
		}
		
		case M_COLOR_SELECTOR_WINDOW_REQUESTED:
		{
			int nControl;
			Color32_s sColor;
						
			pcMessage->FindInt("control",&nControl);
			pcMessage->FindColor32("color",&sColor);
			
			switch (nControl)
			{
				case 1:
				pcSettings->SetNumberColor(sColor);
				pcColors->ColorChanged(1,sColor);			
				break;
				
				case 2:	
				pcSettings->SetBackColor(sColor);
				pcColors->ColorChanged(2,sColor);
				break;
				
				case 3:	
				pcSettings->SetTextColor(sColor);
				pcColors->ColorChanged(3,sColor);
				break;
			
				case 4:
				pcSettings->SetHighColor(sColor);
				pcColors->ColorChanged(4,sColor);
				break;
			}
			pcColors->CancelColorSelectorWindow();			
			break;
		}		
	}
}

void SettingsDialog::Save()
{
	pcGeneral->SaveSettings(pcSettings);
	pcSettings->SetFontHandle(pcFonts->GetFontHandle());
	
	Message* pcMessage = new Message(M_SETTINGS_SAVE);
	pcParentWindow->PostMessage(pcMessage, pcParentWindow);
}

















