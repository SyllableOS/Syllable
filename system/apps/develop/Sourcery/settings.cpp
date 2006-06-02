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

#include "settings.h"
#include "fonthandle.h"

/*************************************************************
* Description: AppSettings Constructor
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 11:03:38 
*************************************************************/
AppSettings::AppSettings()
{
}

/*************************************************************
* Description: Loads the settings from the Settings file
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 11:04:00 
*************************************************************/
void AppSettings::LoadSettings()
{
	os::Settings* pcSettings = new os::Settings();
	pcSettings->Load();
	
	if (pcSettings->FindBool("save_size",&bSaveSizeAndPosition) != 0)
		bSaveSizeAndPosition = true;
		
	if (pcSettings->FindBool("save_folded_attributes",&bSaveFoldedAttributes) != 0)
		bSaveFoldedAttributes = true;	 
		
	if (pcSettings->FindBool("save_cursor_position",&bSaveCursorPosition) != 0)
		bSaveCursorPosition = true;	 
		
	if (pcSettings->FindBool("save_backup",&bSaveBackup) != 0)
		bSaveBackup = false;
	
	if (pcSettings->FindBool("show_splash",&bShowSplash) != 0)
		bShowSplash = false;
		
	if (pcSettings->FindBool("convert_lf",&bConvertLF) != 0)
		bConvertLF = true;
	
	if (pcSettings->FindString("backup_extension",&cBackupExtension) != 0)
		cBackupExtension = ".bak";
		
	if (pcSettings->FindRect("position",&cRect) != 0)
		cRect = Rect(-1,-1,-1,-1);
		
	if (pcSettings->FindBool("save_file_every_min",&bSaveFileEveryMin) != 0)
		bSaveFileEveryMin = false;
	
	if (pcSettings->FindBool("advanced_goto",&bAdvancedGoTo) != 0)
		bAdvancedGoTo = false;
		
	if (pcSettings->FindBool("monitor_file",&bMonitorFile) != 0)
		bMonitorFile = true;
	
	if (pcSettings->FindBool("fold_code",&bFoldCodeOnLoad) != 0)
		bFoldCodeOnLoad = false;
		
	if (pcSettings->FindBool("show_lines",&bShowLines) != 0)
		bShowLines = true;
	
	if (pcSettings->FindColor32("text_color",&sTextColor) != 0)
		sTextColor = Color32_s(0,0,0);
		
	if (pcSettings->FindColor32("back_color",&sBackColor) != 0)
		sBackColor = Color32_s(255,255,255);
		
	if (pcSettings->FindColor32("line_color",&sLineColor) !=0)
		sLineColor = os::Color32_s(220,220,250);
		
	if (pcSettings->FindColor32("high_color",&sHighColor) != 0)
		sHighColor = Color32_s(255,235,186);
	
	if (pcSettings->FindData("fonthandle",T_RAW,(const void**)&pcFontHandle, &nFontHandleSize) != 0)
		pcFontHandle = GetDefaultFontHandle();
}

/*************************************************************
* Description: Loads the default settings
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 11:04:25 
*************************************************************/
void AppSettings::LoadDefaults()
{
	bSaveSizeAndPosition = true;
	bSaveFoldedAttributes = true;	
	bSaveCursorPosition = true;	 					
	bSaveBackup = true;
	bShowSplash = true;
	bConvertLF = true;
	cBackupExtension = ".bak";
	cRect = Rect(-1,-1,-1,-1);
	bSaveFileEveryMin = false;
	bAdvancedGoTo = false;	
	bMonitorFile = true;
	bFoldCodeOnLoad = false;
	bShowLines = true;
	sTextColor = Color32_s(0,0,0);
	sBackColor = Color32_s(255,255,255);
	sLineColor = os::Color32_s(220,220,250);
	sHighColor = Color32_s(255,235,186);
	pcFontHandle = GetDefaultFontHandle();
}

/*************************************************************
* Description: Saves the settings
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 11:04:45 
*************************************************************/
void AppSettings::SaveSettings()
{
	os::Settings* pcSettings = new os::Settings();
	
	pcSettings->AddBool("save_size",bSaveSizeAndPosition);
	pcSettings->AddBool("save_position_attributes",bSaveCursorPosition);
	pcSettings->AddBool("save_backup",bSaveBackup);
	pcSettings->AddBool("show_splash",bShowSplash);
	pcSettings->AddBool("convert_lf", bConvertLF);
	pcSettings->AddBool("save_file_every_min",bSaveFileEveryMin);
	pcSettings->AddBool("monitor_file",bMonitorFile);
	pcSettings->AddBool("advanced_goto",bAdvancedGoTo);
	pcSettings->AddBool("fold_code",bFoldCodeOnLoad);
	pcSettings->AddBool("show_lines",bShowLines);
	pcSettings->SetData("fonthandle",T_RAW, (void*)pcFontHandle,sizeof(FontHandle));
	pcSettings->AddColor32("high_color",sHighColor);
	pcSettings->AddColor32("text_color",sTextColor);
	pcSettings->AddColor32("line_color",sLineColor);
	pcSettings->AddColor32("back_color",sBackColor);			
	pcSettings->AddRect("position",cRect);
	pcSettings->Save();
	delete pcSettings;
}
