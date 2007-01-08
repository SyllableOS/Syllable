#include "WallpaperChangerSettings.h"
#include <gui/guidefines.h>
#include "bitmapscale.h"
#include "messages.h"

using namespace os;

/*************************************************
* Description: Settings Dialog
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
WallpaperChangerSettings::WallpaperChangerSettings(View* pcParent,Message* pcMessage) : Window(Rect(0,0,300,300),"wallpaper_settings","Wallpaper Changer Settings")
{
	pcParentView = pcParent;
	pcParentLooper = pcParent->GetLooper();
	pcParentMessage = pcMessage;
	
	Init();
	Layout();
	LoadSettings();
}

/*************************************************
* Description: Initialization
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
void WallpaperChangerSettings::Init()
{
	pcLargeBitmap = new Bitmap(200,90,CS_RGB32);
	pcImage = new BitmapImage(pcLargeBitmap->LockRaster(),IPoint((int)pcLargeBitmap->GetBounds().Width(),(int)pcLargeBitmap->GetBounds().Height()),CS_RGBA32);
	pcLargeBitmap->UnlockRaster();
}

/*************************************************
* Description: Lays out the Dialog
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
void WallpaperChangerSettings::Layout()
{
	pcImageView = new ImageView(Rect(50,5,GetBounds().Width()-50,95),"imgview",NULL);

	pcImageView->SetImage(pcImage);
	AddChild(pcImageView);
	
	
	pcLayoutView = new LayoutView(Rect(0,105,GetBounds().Width(),GetBounds().Height()),"LayoutView");
	
	VLayoutNode* pcRoot = new VLayoutNode("root");

	VLayoutNode* pcOptionsNode = new VLayoutNode("options");
	pcOptionsNode->AddChild(new VLayoutSpacer("",30,30));
	pcOptionsNode->AddChild(pcRandom = new CheckBox(Rect(),"random_check","Display Random",NULL));
	pcRandom->SetEnable(false);
	pcOptionsNode->AddChild(new VLayoutSpacer("",10,10));
	pcOptionsNode->AddChild(pcTimeString = new StringView(Rect(),"time_string","Load Every:"));
	pcOptionsNode->AddChild(pcTimeDrop = new DropdownMenu(Rect(),"time_drop"));
	pcTimeDrop->SetReadOnly(true);
	LoadTimeSettings();
	pcOptionsNode->AddChild(new VLayoutSpacer("",50,50));
	
	HLayoutNode* pcHLayoutNode = new HLayoutNode("directory_layout");
	pcHLayoutNode->AddChild(pcDirectoryList = new ListView(Rect(),"DirectoryList",ListView::F_RENDER_BORDER));
	pcDirectoryList->InsertColumn("File",(int)GetBounds().Width());
	pcDirectoryList->SetHasColumnHeader(false);
	
	pcDirectoryList->SetSelChangeMsg(new Message(M_CHANGE_IMAGE));
	pcHLayoutNode->AddChild(new HLayoutSpacer("",15,15));
	pcHLayoutNode->AddChild(pcOptionsNode);
	LoadDirectoryList();
	
	HLayoutNode* pcButtonNode = new HLayoutNode("buttons_layout");
	pcButtonNode->AddChild(pcApplyButton = new Button(Rect(),"ok_but","_Apply",new Message(M_APPLY)));
	pcButtonNode->AddChild(new HLayoutSpacer("",20,20));
	pcButtonNode->AddChild(pcDefaultButton = new Button(Rect(),"def_but","_Default",new Message(M_DEFAULT)));
	pcButtonNode->AddChild(new HLayoutSpacer("",20,20));
	pcButtonNode->AddChild(pcCancelButton = new Button(Rect(),"cancel_but","_Cancel",new Message(M_CANCEL)));
	pcButtonNode->SameWidth("ok_but","def_but","cancel_but",NULL);
	pcButtonNode->SameHeight("ok_but","def_but","cancel_but",NULL);
	
	pcRoot->AddChild(pcHLayoutNode);
	pcRoot->AddChild(new VLayoutSpacer("",15,15));
	pcRoot->AddChild(pcButtonNode);
	pcRoot->AddChild(new VLayoutSpacer("",5,5));
	
	pcLayoutView->SetRoot(pcRoot);
	AddChild(pcLayoutView);
	
	SetDefaultButton(pcApplyButton);
}

/*************************************************
* Description: Handles messages passed to it
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
void WallpaperChangerSettings::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		/*case M_NODE_MONITOR:
		{
			pcDirectoryList->Clear();
			LoadDirectoryList();
			break;
		}*/
		
		case M_APPLY:
		{
			SaveSettings();
			break;
		}
		
		case M_CANCEL:
		{
			Message* pcMsg = new Message(M_PREFS_CANCEL);
			pcParentLooper->PostMessage(pcMsg,pcParentView);
			break;
		}
		
		case M_DEFAULT:
		{
			break;
		}
		
		case M_CHANGE_IMAGE:
		{
			ListViewStringRow* pcRow = (ListViewStringRow*)pcDirectoryList->GetRow(pcDirectoryList->GetLastSelected());
			String cImage = String("/system/resources/wallpapers/") + pcRow->GetString(0);
			BitmapImage* pcNewImage = new BitmapImage(new File(cImage));
			Bitmap* pcNewBitmap = new Bitmap(201,90,CS_RGB32);
			Scale(pcNewImage->LockBitmap(),pcNewBitmap,filter_box,0);
			pcNewImage->SetBitmapData(pcNewBitmap->LockRaster(),IPoint(201,90),CS_RGB32);
			pcImageView->SetImage(pcNewImage);
			break;
		}
	}
}

/*************************************************
* Description: Called when closing the window
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
bool WallpaperChangerSettings::OkToQuit()
{
	PostMessage(new Message(M_PREFS_CANCEL),this);
	return true;
}

/*************************************************
* Description: Loads the settings
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
void WallpaperChangerSettings::LoadSettings()
{
	bool bFlag = false;
	
	pcParentMessage->FindBool("random",&bRandom);
	pcParentMessage->FindInt32("time",&nTime);
	pcParentMessage->FindString("currentimage",&cCurrentImage);
	
	pcRandom->SetValue(bRandom);
	pcTimeDrop->SetSelection(nTime);
	
	for (uint i=0;i<pcDirectoryList->GetRowCount();i++)
	{
		ListViewStringRow* pcRow = (ListViewStringRow*)pcDirectoryList->GetRow(i);
		
		if (pcRow->GetString(0) == cCurrentImage)
		{
			bFlag = true;
			pcDirectoryList->Select(i);
		}
	}
	
	if (bFlag == false)
		pcDirectoryList->Select(0);
	
}

/*************************************************
* Description: Saves the settings
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
void WallpaperChangerSettings::SaveSettings()
{
	ListViewStringRow* pcRow = (ListViewStringRow*)pcDirectoryList->GetRow(pcDirectoryList->GetLastSelected());
	Message* pcMessage = new Message(M_PREFS_SEND_TO_PARENT);
	bRandom = pcRandom->GetValue().AsBool();				
	pcMessage->AddBool("random",bRandom);
	pcMessage->AddInt32("time",pcTimeDrop->GetSelection());
	pcMessage->AddString("currentimage",pcRow->GetString(0));	
	pcParentLooper->PostMessage(pcMessage,pcParentView);
}

/*************************************************
* Description: Adds time settings to dropdownmenu
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
void WallpaperChangerSettings::LoadTimeSettings()
{
	pcTimeDrop->AppendItem("No Time");
	pcTimeDrop->AppendItem("1 Minute");
	pcTimeDrop->AppendItem("3 Minutes");
	pcTimeDrop->AppendItem("5 Minutes");
	pcTimeDrop->AppendItem("10 Minutes");
	pcTimeDrop->AppendItem("30 Minutes");
	pcTimeDrop->AppendItem("60 Minutes");
}

/*************************************************
* Description: Loads the directoy contents of ~/Documents/Pictures into listview
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
void WallpaperChangerSettings::LoadDirectoryList()
{
	String cDir = String("/system/resources/wallpapers/");
	String cFile;
	
	Directory* pcDir = new Directory(cDir);
	pcDir->Rewind();
	
	while (pcDir->GetNextEntry(&cFile))
	{
		FSNode* pcNode = new FSNode(String(cDir + cFile));
		if (pcNode->IsFile())		//if not a dir, link and just a regular file
		{
			ListViewStringRow* pcRow = new ListViewStringRow();
			pcRow->AppendString(cFile);
			pcDirectoryList->InsertRow(pcRow);
		}
		delete pcNode;
	}
	delete pcDir;
}























