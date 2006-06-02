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

#ifndef _SETTINGS_DIALOG_H
#define _SETTINGS_DIALOG_H

#include <gui/window.h>
#include <gui/stringview.h>
#include <gui/view.h>
#include <gui/font.h>
#include <gui/frameview.h>
#include <gui/button.h>
#include <gui/checkbox.h>
#include <gui/splitter.h>
#include <gui/textview.h>
#include <gui/listview.h>
#include <gui/font.h>
#include <gui/iconview.h>
#include <gui/listview.h>
#include <gui/spinner.h>
#include <gui/colorselector.h>

#include "settings.h"
#include "colorbutton.h"

#include <cmath>

using namespace os;

class DialogHighlighter : public View
{
public:	
	DialogHighlighter(const String& cName);

	void Paint(const Rect&);
	
private:
	String cHighlight;
};

class ColorSelectorWindow : public Window
{
public:
	ColorSelectorWindow(int,Color32_s, Window*);
	
	bool OkToQuit();
	void HandleMessage(Message*);
	void Layout();
private:
	ColorSelector* pcSelector;
	Button* pcOkButton;
	Button* pcCancelButton;
	
	LayoutView* pcLayoutView;
	Window* pcParentWindow;
	
	int nControl;
	Color32_s sControlColor;
};	

class SettingsDialogGeneral : public LayoutView
{
public:
	SettingsDialogGeneral();
	~SettingsDialogGeneral();
	
	void LayoutSystem();
	void Init(AppSettings*);
	void SaveSettings(AppSettings*);	
private:
	VLayoutNode* pcRoot;
	CheckBox* pcSplashCheck;
	CheckBox* pcAdvancedGoToCheck;
	CheckBox* pcBackupCheck;
	CheckBox* pcMonitorCheck;
	CheckBox* pcSaveCursorCheck;
	CheckBox* pcFoldCheck;
	CheckBox* pcSaveWindowCheck;
	CheckBox* pcConvertLFCheck;
	CheckBox* pcFileMinCheck;
	CheckBox* pcLineCheck;
	DialogHighlighter* pcHighlighter;
};

class SettingsDialogFont : public LayoutView
{
public:
	SettingsDialogFont(struct FontHandle*);
	~SettingsDialogFont();
	
	void LoadFontTypes();
	void FontHasChanged();
	void FaceHasChanged();
	void SizeHasChanged();
	void AliasChanged();
	void ShearChanged();
	void RotateChanged();
	
	FontHandle* GetFontHandle();
private:
	void SetupFonts();
	void LayoutSystem();
	void InitialFont();
	
	VLayoutNode* pcRoot;
	ListView* pcFontListView;
	ListView* pcFontFaceListView;
	ListView* pcFontSizeListView;
	
	CheckBox* pcAntiCheck;
	
	StringView* pcTextStringView;
	FrameView* pcTextFrameView;
	
	StringView* pcRotationString;
	StringView* pcShearString;
	
	Spinner* pcRotationSpinner;
	Spinner* pcShearSpinner;	
	
	struct FontHandle* pcFontHandle;
};

class SettingsDialogShortcuts : public LayoutView
{
public:
	SettingsDialogShortcuts();
	~SettingsDialogShortcuts();
private:
	VLayoutNode* pcRoot;
	StringView* pcError;
};

class SettingsDialogPlugins : public LayoutView
{
public:
	SettingsDialogPlugins();
	~SettingsDialogPlugins();
private:
	VLayoutNode* pcRoot;
	StringView* pcError;
};

class SettingsDialogColors : public LayoutView
{
public:
	SettingsDialogColors();
	~SettingsDialogColors();
	
	void SetColors(AppSettings*);
	void SaveColors(AppSettings*);
	void LaunchColorSelectorWindow(int,Color32_s,Window*);
	void CancelColorSelectorWindow();
	void ColorSelectorWindowRequested(Color32_s sColor,int nControl);	
	void ColorChanged(int nColor,Color32_s);
private:
	void LayoutSystem();
	void Init();
	
	VLayoutNode* pcRoot;
	
	ColorButton* pcBackColorButton;
	ColorButton* pcNumberColorButton;
	ColorButton* pcHighlightColorButton;
	ColorButton* pcTextColorButton;
	
	StringView* pcBackColorString;
	StringView* pcNumberColorString;
	StringView* pcHighlightColorString;
	StringView* pcTextColorString;
	
	Color32_s sBackColor;
	Color32_s sNumberColor;
	Color32_s sHighlightColor;
	Color32_s sTextColor;
	
	ColorSelectorWindow* pcColorSelectorWindow;
};

class SettingsDialog : public Window
{
public:
	SettingsDialog(Window*, AppSettings*);
	~SettingsDialog();
	
	void HandleMessage(Message* pcMessage);
	bool OkToQuit(void);
private:
	void AddIcons();
	void Layout();
	void Save();
	
	FrameView* pcView;
	
	Button* pcSaveButton, *pcCancelButton, *pcDefaultButton;
	
	SettingsDialogGeneral* pcGeneral;
	SettingsDialogFont* pcFonts;
	SettingsDialogPlugins* pcPlugins;
	SettingsDialogShortcuts* pcShortcuts;
	SettingsDialogColors* pcColors;
	//LayoutView* pcSettingsView;
	LayoutView* pcButtonView;
	HLayoutNode* pcButtonNode;
	HLayoutNode* pcLayoutNode; 		
	HLayoutNode* pcRoot;
	IconView* pcIconView;	
	AppSettings* pcSettings;
	Window* pcParentWindow;
	VLayoutNode* pcClearNode;
	Splitter* pcSplitter;
};
#endif //_SETTINGS_DIALOG_H





































