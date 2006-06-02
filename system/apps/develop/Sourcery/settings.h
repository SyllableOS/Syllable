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

#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <util/settings.h>
#include <util/string.h>
#include <gui/rect.h>
#include <gui/gfxtypes.h>

using namespace os;

struct FontHandle;

class AppSettings
{
public:
	AppSettings();
	
	void LoadSettings();
	void SaveSettings();
	void		LoadDefaults();
		
	void 		SetCursorPosition(bool bCursor) {bSaveCursorPosition = bCursor;}
	bool 		GetCursorPosition() {return bSaveCursorPosition;}
	
	void 		SaveFolderAttributes(bool bAttributes) { bSaveFoldedAttributes = bAttributes;}
	bool 		GetFolderAttributes() { return bSaveFoldedAttributes; }
	
	void 		SetBackup(bool bBackup) {bSaveBackup = bBackup;}
	bool		GetBackup() { return bSaveBackup;}

	void		SetSizeAndPositionOption(bool bSizeAndPosition) { bSaveSizeAndPosition = bSizeAndPosition;}
	bool		GetSizeAndPositionOption() {return bSaveSizeAndPosition;}

	void		SetSizeAndPosition(Rect r) { cRect = r;}
	Rect		GetSizeAndPosition() {return cRect;}
	
	void   		SetBackupExtension(String cExtension) { cBackupExtension = cExtension;}
	String 		GetBackupExtension() { return cBackupExtension;}
	
	void		SetSplashScreen(bool bShow) { bShowSplash = bShow;}
	bool		GetSplashScreen() { return bShowSplash;}

	void		SetConvertToLf(bool bLF) { bConvertLF = bLF; }
	bool		GetConvertToLf() { return bConvertLF;}

	void		SetFontHandle(FontHandle* pcHandle){pcFontHandle = pcHandle; }
	FontHandle*	GetFontHandle() { return pcFontHandle; }
	
	void		SetSaveFileEveryMin(bool bSave){ bSaveFileEveryMin = bSave; }
	bool		GetSaveFileEveryMin() { return bSaveFileEveryMin; }
	
	void		SetMonitorFile(bool bMonitor) { bMonitorFile = bMonitor; }
	bool		GetMonitorFile() { return bMonitorFile; }
	
	void		SetAdvancedGoToDialog(bool bGoTo) { bAdvancedGoTo = bGoTo; }
	bool		GetAdvancedGoToDialog() { return bAdvancedGoTo;}
	
	void		SetFoldCode(bool bFold) { bFoldCodeOnLoad = bFold;}
	bool		GetFoldCode() { return bFoldCodeOnLoad; }
	
	void		SetShowLineNumbers(bool bLines) { bShowLines = bLines;}
	bool		GetShowLineNumbers() { return bShowLines; }
	
	void		SetBackColor(Color32_s sBack) { sBackColor = sBack; }
	Color32_s	GetBackColor() {return sBackColor;}
	
	void		SetNumberColor(Color32_s sLine) { sLineColor = sLine; }
	Color32_s	GetNumberColor() {return sLineColor;}
	
	void		SetTextColor(Color32_s sText) { sTextColor = sText; }
	Color32_s	GetTextColor() {return sTextColor;}
	
	void		SetHighColor(Color32_s sHigh) { sHighColor = sHigh; }
	Color32_s	GetHighColor() {return sHighColor;}			

private:

	//private variables
	bool bSaveCursorPosition;
	bool bSaveFoldedAttributes;
	bool bSaveBackup;
	bool bSaveSizeAndPosition;
	bool bShowSplash;
	bool bConvertLF;
	bool bSaveFileEveryMin;
	bool bMonitorFile;
	bool bAdvancedGoTo;
	bool bFoldCodeOnLoad;
	bool bShowLines;
	
	os::Color32_s sBackColor;
	os::Color32_s sTextColor;
	os::Color32_s sLineColor;
	os::Color32_s sHighColor;
	
	String cBackupExtension;
	Rect cRect;
	FontHandle* pcFontHandle;
	
	size_t nFontHandleSize;
};

#endif















