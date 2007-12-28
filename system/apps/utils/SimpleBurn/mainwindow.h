#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/button.h>
#include <gui/image.h>
#include <gui/frameview.h>
#include <gui/imagebutton.h>
#include <gui/imageview.h>
#include <gui/stringview.h>
#include <gui/menu.h>
#include <gui/separator.h>
#include <gui/tabview.h>
#include <gui/listview.h>
#include <gui/textview.h>
#include <gui/dropdownmenu.h>
#include <gui/slider.h>
#include <gui/checkbox.h>
#include <gui/spinner.h>
#include <gui/radiobutton.h>
#include <gui/requesters.h>
#include <gui/checkmenu.h>
#include <gui/filerequester.h>
#include <util/resources.h>
#include <util/application.h>
#include <util/message.h>
#include <util/settings.h>
#include <media/manager.h>
#include "erasewindow.h"
#include "splash.h"

using namespace os;

class MainWindow : public os::Window
{
public:
	MainWindow();
	void EraseDisk();
	void CopyDisk();
	void DataDisk();
	void AudioDisk();
	void VideoDisk();
	void BackupDisk();
	void UpdateTime();
	void LoadSettings();
	void SaveSettings();
	void DeviceScan();
	void SetMediaType();
	void HandleMessage( os::Message* );
private:
	bool OkToQuit();
	BitmapImage* LoadImageFromResource( os::String zResource );
	BitmapImage* m_pcIcon;
	EraseWindow* pcEraseWin;
	#include "mainwindowLayout.h"
	#include "burnLayout.h"
	#include "dataLayout.h"
	#include "audioLayout.h"
	#include "videoLayout.h"
	#include "backupLayout.h"

	os::FileRequester* m_pcFileRequesterISO;
	os::FileRequester* m_pcFileRequesterAdd;
	MediaManager* pcManager;

	os::String cMakeISO1;
	os::String cMakeISO2;

	os::String cAudioSelect;
	os::String cVideoSelect;

	bool bDummy;
	bool bBackupWarning;
	os::String cDiskAuthor;
};

#endif

