/*
    A Clipboard utility for Dock

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
                                                                                                                                                       
#include <vector>
#include <iostream>

#include <gui/layoutview.h>
#include <gui/frameview.h>
#include <gui/treeview.h>
#include <gui/textview.h>
#include <gui/dropdownmenu.h>
#include <gui/stringview.h>
#include <gui/requesters.h>
#include <gui/menu.h>
#include <gui/image.h>
#include <gui/popupmenu.h>

#include <util/resources.h>
#include <util/settings.h>
#include <util/clipboard.h>
#include <util/messenger.h>

#include <storage/file.h>
#include <storage/streamableio.h>

#include <atheos/time.h>

#include <appserver/dockplugin.h>
                    
#define PLUGIN_NAME       "Clipper"
#define PLUGIN_VERSION    "0.1"
#define PLUGIN_DESC       "A Clipboard Manager Plugin for Dock"
#define PLUGIN_AUTHOR     "Rick Caudill"
                                                                                                                                                                                                        
using namespace os;
using namespace std;                                                                                                                                                                                                     

/*message ids*/                                                                                                                                                                                                        
enum
{
	M_CLIPPER_ABOUT,
	M_CLIPPER_SETTINGS,
	M_CLIPPER_CLEAR,
	M_HELP,
	M_CLIPPER_COPY,
	M_CLIPPER_SEND,
	M_PREFS_APPLY,
	M_PREFS_CANCEL,
	M_PREFS_CLICKED,
	M_PREFS_VIEW,
	M_PREFS_DELETE,
	M_CLICKER_ERROR_OK,
};


/*looper*/
class ClipperLooper : public os::Looper
{
public:
	ClipperLooper(os::Looper*, os::View*);
	virtual void TimerTick(int);
private:
	os::Looper* pcParentLooper;
	os::View* pcParentView;
	os::String cString;
};


/*view window*/
class ClipperWindow : public Window
{
public:
	ClipperWindow(os::Looper*);
	void ChangeText(const String& cText);
	bool OkToQuit() 
	{ 
		pcParentLooper->PostMessage(new Message(M_CLIPPER_SEND),pcParentLooper);
		return true;
	}	
private:
	TextView* pcViewTextView;
	os::Looper* pcParentLooper;
};

/*settings window*/
class ClipperSettingsWindow : public Window
{
public:
	ClipperSettingsWindow(View*, std::vector<os::String>,int32);
	
	void Layout();
	bool OkToQuit();
	void HandleMessage(Message*);
	void UpdateClips(std::vector<os::String>);
	void UpdateTreeView();
	void SetTime();

private:
	std::vector<os::String> m_cCopiedTextVector;
	int32 nTime;
	
	View* pcParentView;
	String cWindowText;
	
	TreeView* pcTreeView;
	TextView* pcTextView;
	DropdownMenu* pcTimeDrop;
	StringView* pcTimeString;
	LayoutView* pcLayoutView;
	FrameView* pcFrameView;
	Button *pcOkButton, *pcCancelButton, *pcDelButton,*pcViewButton;
	ClipperWindow* pcWindow;
	
};

/*actual plugin*/
class DockClipper : public View
{
	public:
		DockClipper( os::Path cPath, os::Looper* pcDock );
		~DockClipper();
		
		os::String GetIdentifier() ;
		Point GetPreferredSize( bool bLargest ) const;
		os::Path GetPath() { return( m_cPath ); }
		
		virtual void AttachedToWindow();
		void InitializeMenu();
		virtual void HandleMessage(Message* pcMessage);  
		
	private:
		void UpdateItems(const String&);
		void ClearContent();
		bool IsInCopyVector(const os::String& cNewItem);
		void LoadSettings();
		void SaveSettings();
		os::Path m_cPath;
		os::Looper* m_pcDock;
		
		vector<String> m_cCopiedTextVector;
		int m_nCount;
		int32 nTime; 
		bool bNotShown;

		ClipperSettingsWindow* pcWindow;
		Menu* pcContextMenu;
		BitmapImage* m_pcIcon;
		os::File* pcFile;
		os::ResStream *pcStream;
		os::PopupMenu* pcPopup;
		ClipperLooper* pcClipperLooper;
		Alert* pcError;
		os::String cPrevious;
};

ClipperWindow::ClipperWindow(Looper* pcParent) : Window(Rect(0,0,200,200),"view_window","Viewing Clip...",WND_NO_DEPTH_BUT | WND_NOT_RESIZABLE | WND_NO_ZOOM_BUT)
{
	pcParentLooper = pcParent;
	
	AddChild(pcViewTextView = new TextView(Rect(5,5,195,195),"text",""));
	pcViewTextView->SetReadOnly(true);
	pcViewTextView->SetMultiLine(true);
}

void ClipperWindow::ChangeText(const String& cText)
{
	pcViewTextView->Set(cText.c_str());
}


ClipperSettingsWindow::ClipperSettingsWindow(View* pcParent,std::vector <os::String> cVector, int32 Time) : Window(Rect(0,0,380,304),"settings_window","Clipper Settings...",WND_NOT_RESIZABLE | WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT)
{
	pcParentView = pcParent;
	m_cCopiedTextVector = cVector;
	nTime = Time;
	pcWindow = NULL;
	
	Layout();
}

void ClipperSettingsWindow::Layout()
{
	pcLayoutView = new LayoutView(GetBounds(),"root_layout_for_clipper",NULL,CF_FOLLOW_ALL);
	
	VLayoutNode* pcRoot = new VLayoutNode("root_layout");
	pcRoot->SetBorders(Rect(5,5,5,5));
	
	HLayoutNode* pcTreeViewLayout = new HLayoutNode("treeview_layout");
	pcTreeViewLayout->AddChild(pcTreeView = new TreeView(Rect(0,0,1,1),""));//ListView::F_NO_HEADER));
	pcTreeView->InsertColumn("Clips",(int)GetBounds().Width());
	pcTreeView->SetSelChangeMsg(new Message(M_PREFS_CLICKED));
	UpdateTreeView();
	pcTreeViewLayout->SetWeight(120);	
	
	HLayoutNode* pcTreeViewButtonLayout = new HLayoutNode("treeview_button_layout");
	pcTreeViewButtonLayout->AddChild(new HLayoutSpacer("",20));	
	pcTreeViewButtonLayout->AddChild(pcViewButton=new Button(Rect(0,0,1,1),"BTView","_View",new Message(M_PREFS_VIEW)));
   	pcTreeViewButtonLayout->AddChild(new os::HLayoutSpacer("",5.0f,5.0f)); 	
  	pcTreeViewButtonLayout->AddChild(pcDelButton=new Button(Rect(0,0,1,1),"BTDelete","_Delete",new Message(M_PREFS_DELETE)));
  	pcTreeViewButtonLayout->AddChild(new os::HLayoutSpacer("",5.0f,5.0f));
	pcTreeViewButtonLayout->AddChild(new HLayoutSpacer("",20));  	
	pcTreeViewButtonLayout->SameWidth("BTDelete","BTView",NULL);
	
	HLayoutNode* pcTimeLayout = new HLayoutNode("time_layout");
	pcTimeLayout->AddChild(pcTimeString = new StringView(Rect(),"time_string","Check Every:"));
	pcTimeLayout->AddChild(new HLayoutSpacer("",5,5));
	pcTimeLayout->AddChild(pcTimeDrop = new DropdownMenu(Rect(),"time_drop"));
	pcTimeDrop->SetMinPreferredSize(12);
	pcTimeDrop->SetMaxPreferredSize(12);
	pcTimeLayout->SameHeight("time_string","time_drop",NULL);
	pcTimeDrop->AppendItem(" 1 second");
	pcTimeDrop->AppendItem(" 3 seconds");
	pcTimeDrop->AppendItem(" 5 seconds");
	pcTimeDrop->AppendItem("10 seconds");
	pcTimeDrop->AppendItem("20 seconds");
	pcTimeDrop->AppendItem("30 seconds");
	pcTimeDrop->AppendItem("45 seconds");
	pcTimeDrop->AppendItem("60 seconds");
	pcTimeDrop->SetReadOnly(true);	
	pcTimeLayout->AddChild(new HLayoutSpacer("",5,5));
	SetTime();
  	
  	HLayoutNode* pcHLButton = new os::HLayoutNode("");
    pcHLButton->AddChild(new os::HLayoutSpacer("",5.0f,5.0f));	
  	pcHLButton->AddChild(pcCancelButton=new Button(Rect(0,0,2,2),"BTDefault","_Cancel",new Message(M_PREFS_CANCEL)));
   	pcHLButton->AddChild(new os::HLayoutSpacer("",5.0f,5.0f)); 	
  	pcHLButton->AddChild(pcOkButton=new Button(Rect(0,0,2,2),"BTApply","_Okay",new Message(M_PREFS_APPLY)));
  	pcHLButton->AddChild(new os::HLayoutSpacer("",5.0f,5.0f));
  	pcHLButton->SameWidth("BTApply","BTDefault",NULL);
  	
  	pcRoot->AddChild(pcTreeViewLayout);
  	pcRoot->AddChild(new VLayoutSpacer("",2,2));
	pcRoot->AddChild(pcTreeViewButtonLayout); 	
  	pcRoot->AddChild(new VLayoutSpacer("",10,10));
	pcRoot->AddChild(pcTimeLayout);
  	pcRoot->AddChild(new VLayoutSpacer("",15,15));	
	pcRoot->AddChild(pcHLButton);
  	pcRoot->AddChild(new VLayoutSpacer("",5,5));

	pcLayoutView->SetRoot(pcRoot);

	AddChild(pcLayoutView);
}

void ClipperSettingsWindow::SetTime()
{
	switch (nTime)
	{
		case 1:
			pcTimeDrop->SetSelection(0);
			break;
		case 3:
			pcTimeDrop->SetSelection(1);
			break;
		case 5:
			pcTimeDrop->SetSelection(2);
			break;
		case 10:
			pcTimeDrop->SetSelection(3);
			break;
		case 20:
			pcTimeDrop->SetSelection(4);
			break;
		case 30:
			pcTimeDrop->SetSelection(5);
			break;
		case 45:
			pcTimeDrop->SetSelection(6);
			break;
		case 60:
			pcTimeDrop->SetSelection(7);
			break;
	}
}

void ClipperSettingsWindow::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_PREFS_CLICKED:
		{
			int nSelected = pcTreeView->GetLastSelected();
			
			if (nSelected > -1)
			{
				TreeViewStringNode* pcNode = (TreeViewStringNode*)pcTreeView->GetRow(nSelected);
				Variant cCookie = pcNode->GetCookie();
							
				cWindowText = pcNode->GetCookie().AsString();
			}
			break;
		}
		
		case M_PREFS_CANCEL:
		{
			if (pcWindow != NULL)
				pcWindow->Close();
			pcWindow = NULL;	
			pcParentView->GetLooper()->PostMessage(new Message(M_PREFS_CANCEL),pcParentView);
			break;
		}
		
		case M_PREFS_APPLY:
		{
			if(pcWindow != NULL)
				pcWindow->Close();
			pcWindow = NULL;
			
			Message* pcMsg = new Message(M_PREFS_APPLY);
			
			int32 nTime = pcTimeDrop->GetSelection();
			
			switch (nTime)
			{
				case 0:
					nTime = 1;
					break;
					
				case 1:
					nTime = 3;
					break;
					
				case 2:
					nTime = 5;
					break;
					
				case 3:
					nTime = 10;
					break;
					
				case 4:
					nTime = 20;
					break;
					
				case 5:
					nTime = 30;
					break;
					
				case 6:
					nTime = 45;
					break;
					
				case 7:
					nTime = 60;
					break;
			}
			pcMsg->AddInt32("time",nTime);
			pcMsg->AddInt32("count",m_cCopiedTextVector.size());
			
			for (int i=0;i<m_cCopiedTextVector.size();i++)
			{
				String cClip;
				cClip.Format("Clip %d",i);
				pcMsg->AddString(cClip.c_str(),m_cCopiedTextVector[i]);
			}
			pcParentView->GetLooper()->PostMessage(pcMsg,pcParentView);
			break;
		}
		
		case M_PREFS_VIEW:
		{
			int nSelected = pcTreeView->GetLastSelected();

			if (nSelected > -1)
			{
				if (pcWindow == NULL)
				{
					pcWindow = new ClipperWindow(this);
					pcWindow->Start();
					pcWindow->ChangeText(cWindowText);
				}
				else
					pcWindow->ChangeText(cWindowText);
				
				pcWindow->CenterInWindow(this);
				pcWindow->Show(true);
				pcWindow->MakeFocus();
			}
			break;
		}
		
		case M_PREFS_DELETE:
		{
			int nSelected = pcTreeView->GetLastSelected();
			
			if (nSelected >-1)
			{
				pcTreeView->RemoveRow(nSelected);
			
				std::vector<String> m_cClips;
			
				for (uint i=0; i<pcTreeView->GetRowCount(); i++)
				{
					TreeViewStringNode* pcNode = (TreeViewStringNode*)pcTreeView->GetRow(i);
					m_cClips.push_back(pcNode->GetCookie().AsString());
				}
				m_cCopiedTextVector = m_cClips;
				UpdateTreeView();
			}
			break;
		}
		
		case M_CLIPPER_SEND:
		{
			pcWindow->Close();
			pcWindow = NULL;
			break;
		}
	}
}

bool ClipperSettingsWindow::OkToQuit()
{
	pcParentView->GetLooper()->PostMessage(new Message(M_PREFS_CANCEL),pcParentView);
	return true;
}

void ClipperSettingsWindow::UpdateClips(std::vector<os::String> m_cClips)
{
	m_cCopiedTextVector = m_cClips;
	UpdateTreeView();
}

void ClipperSettingsWindow::UpdateTreeView()
{
	pcTreeView->Clear();
	
	for (uint i=0; i<m_cCopiedTextVector.size(); i++)
	{
		String cString;
		cString.Format("%s %d","Clip", i+1);
		TreeViewStringNode* pcNode = new TreeViewStringNode();
		pcNode->AppendString(cString);
		pcNode->SetCookie(Variant(m_cCopiedTextVector[i]));
		pcTreeView->InsertNode(i,pcNode,false);	
	}
}

ClipperLooper::ClipperLooper(Looper* pcParent, os::View* pcView) : Looper("ClipperLooper")
{
	pcParentLooper = pcParent;
	pcParentView = pcView;
}

void ClipperLooper::TimerTick(int nID)
{
	Clipboard cClip;
	cClip.Lock();
	Message* pcMessage = cClip.GetData();
	
	if (pcMessage->FindString("text/plain", &cString) == 0)
	{
		Message* pcTargetMessage = new Message(M_CLIPPER_SEND);
		pcTargetMessage->AddString("text",cString);
		pcParentLooper->PostMessage(pcTargetMessage,pcParentView); //pcParentView);
	}
	cClip.Unlock();
}




//*************************************************************************************
DockClipper::DockClipper( os::Path cPath, os::Looper* pcDock ) : View(Rect(0,0,0,0),"DockClipperView")
{
	LoadSettings();
	
	m_pcDock = pcDock;
	m_cPath = cPath;	
	bNotShown = false;
	pcWindow = NULL;


	/* Load default icons */
	pcFile = new os::File( m_cPath );
	os::Resources cCol( pcFile );
	pcStream = cCol.GetResourceStream( "icon24x24.png" );
	m_pcIcon = new os::BitmapImage( pcStream );
	delete pcFile;
	
	InitializeMenu();
	

	
	pcPopup = new PopupMenu(Rect(0,0,1,1),"clipper_popup","",pcContextMenu,m_pcIcon);
	pcPopup->SetFrame(Rect(0,0,pcPopup->GetPreferredSize(false).x, pcPopup->GetPreferredSize(false).y));
	AddChild(pcPopup);
}

DockClipper::~DockClipper( )
{
	SaveSettings();
}

void DockClipper::AttachedToWindow()
{
	pcClipperLooper = new ClipperLooper(m_pcDock,this);
	pcClipperLooper->Run();
	pcClipperLooper->AddTimer(pcClipperLooper,123,nTime*1000000,false);
		
	pcContextMenu->SetTargetForItems(this);
	View::AttachedToWindow();
}

String DockClipper::GetIdentifier()
{
	return( PLUGIN_NAME );
}


void DockClipper::LoadSettings()
{
	/* Load settings */
	os::String zPath = getenv( "HOME" );
	zPath += "/Settings/Clipper/Settings";

	try
	{
		os::Settings* pcSettings = new os::Settings(new File(zPath));
		pcSettings->Load();
		
		nTime = pcSettings->GetInt32("time",0);
		m_nCount = pcSettings->GetInt32("count",0);

			
		for (int i = 0; i<m_nCount; i++)
		{	
			String cClip = pcSettings->GetString("clip","",i);
			
			if (cClip != String(""))
				m_cCopiedTextVector.push_back(cClip);
		}
		delete( pcSettings );
	}
	catch(...)
	{
	}
}

void DockClipper::SaveSettings()
{
	/* Save settings */
	os::String zPath = getenv( "HOME" );
	zPath += "/Settings/Clipper";

	try
	{
		mkdir( zPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO );
		zPath += "/Settings";
		os::File cFile( zPath, O_RDWR | O_CREAT );	
		os::Settings* pcSettings = new os::Settings(new File(zPath));
		pcSettings->SetInt32("time",nTime);
		pcSettings->SetInt32("count",m_cCopiedTextVector.size());
		
		for (uint i=0; i<m_cCopiedTextVector.size(); i++)
		{
			pcSettings->SetString("clip",m_cCopiedTextVector[i],i);
		}
		pcSettings->Save();
		delete( pcSettings );
	}
	catch(...)
	{
	}
}

Point DockClipper::GetPreferredSize( bool bLargest ) const
{
	return os::Point(pcPopup->GetBounds().Width(), pcPopup->GetBounds().Height());
}

void DockClipper::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_CLIPPER_ABOUT:
		{
			String cTitle = (String)"About " + (String)PLUGIN_NAME + (String)"...";
			String cInfo = (String)"Version:  " +  (String)PLUGIN_VERSION + (String)"\n\nAuthor:   " + (String)PLUGIN_AUTHOR + (String)"\n\nDesc:      " + (String)PLUGIN_DESC;	
			Alert* pcAlert = new Alert(cTitle.c_str(),cInfo.c_str(),m_pcIcon->LockBitmap(),0,"OK",NULL);
			m_pcIcon->UnlockBitmap();
			pcAlert->Go(new Invoker());
			pcAlert->MakeFocus();
			break;
		}
		
		case M_CLIPPER_SEND:
		{
			String cString;
			pcMessage->FindString("text",&cString);			
			
			if (m_nCount < 10)
			{				
				UpdateItems(cString);
				SaveSettings();
			}
			else
			{
				if (!IsInCopyVector(cString) && cString != cPrevious)
				{
					if (!bNotShown)
					{
						pcError = new Alert("Delete...","You have exceeded the maximum amount of clips, please delete some old clips for future use...",m_pcIcon->LockBitmap(),0,"OK",NULL);
						m_pcIcon->UnlockBitmap();					
						bNotShown = true;					
						pcError->Go(new os::Invoker(new os::Message(M_CLICKER_ERROR_OK),this));
						pcError->MakeFocus();
					}
					else
						pcError->MakeFocus();
					
				}
				else if (bNotShown == true)
				{
					pcError->MakeFocus();
				}
				
			}
			cPrevious = cString;	
			break;
		}

		case M_CLIPPER_COPY:
		{
			String cString;
			pcMessage->FindString("text",&cString);
			Clipboard cClipboard;

			cClipboard.Lock();
			cClipboard.Clear();
			Message* pcMsg = cClipboard.GetData();
			pcMsg->AddString("text/plain",cString);
			cClipboard.Commit();
			cClipboard.Unlock();
			break;
		}

		case M_CLIPPER_CLEAR:
		{
			ClearContent();
			break;
		}

		case M_CLIPPER_SETTINGS:
		{
			if (pcWindow == NULL)
			{
				pcWindow = new ClipperSettingsWindow(this,m_cCopiedTextVector,nTime);
				pcWindow->CenterInScreen();
				pcWindow->Show();
				pcWindow->MakeFocus();
			}
			else
			{
				pcWindow->UpdateClips(m_cCopiedTextVector);
				pcWindow->MakeFocus();
			}
			break;
		}
		
		case M_PREFS_CANCEL:
		{
			if (pcWindow != NULL)
			{
				pcWindow->Close();
				pcWindow = NULL;
			}
			break;
		}
		
		case M_PREFS_APPLY:
		{
			if (pcWindow != NULL)
			{
				pcClipperLooper->RemoveTimer(pcClipperLooper,123);
				m_cCopiedTextVector.clear();
				int nCount;
				ClearContent();
				
				pcMessage->FindInt32("time",&nTime);
				pcMessage->FindInt32("count",&nCount);

				for (int i=0; i<nCount; i++)
				{
					String cText;
					String cString;
					cString.Format("Clip %d",i);
					
					pcMessage->FindString(cString.c_str(),&cText);
					UpdateItems(cText);
				}
			
				SaveSettings();
				pcClipperLooper->AddTimer(pcClipperLooper,123,nTime*1000000,false);				
				pcWindow->Close();
				pcWindow = NULL;
			}
			break;
		}
		
		case M_CLICKER_ERROR_OK:
		{
			bNotShown = false;
			break;
		}	 	
	}
}

void DockClipper::UpdateItems(const String& cNewItem)
{
	//test if item is in vector all ready.
	if ( IsInCopyVector(cNewItem) )
	{
		return;
	}


	else if (m_nCount < 10)
	{
		String cClip;
		cClip.Format("Clip %d",m_nCount+1);
		Message* pcMessage = new Message(M_CLIPPER_COPY);
		pcMessage->AddString("text",cNewItem);
		pcContextMenu->RemoveItem(m_nCount);
		os::MenuItem* pcItem = new MenuItem(cClip,pcMessage);
		pcContextMenu->AddItem(pcItem,m_nCount);
		m_nCount++;			
		m_cCopiedTextVector.push_back(cNewItem);
		pcContextMenu->SetTargetForItems(this);
		if (pcWindow != NULL)
			pcWindow->UpdateClips(m_cCopiedTextVector);
	}
	return;
}

void DockClipper::ClearContent()
{

	for (int i=0; i<10; i++)
	{
		pcContextMenu->RemoveItem(0);
	}

	for (int i=9;i>=0;i--)
	{
		String cString;
		cString.Format("%s %d","Clip",i+1);

		os::MenuItem* pcItem = new MenuItem(cString,NULL);
		pcContextMenu->AddItem(pcItem,0);
		pcItem->SetEnable(false);
	}
	bNotShown = true;
	pcContextMenu->SetTargetForItems(this);
	m_cCopiedTextVector.clear();
	m_nCount = 0;
}

void DockClipper::InitializeMenu()
{
	pcContextMenu = new Menu(Rect(0,0,1,1),"menu",ITEMS_IN_COLUMN);
	
	for (int i=0; i<10; i++)
	{
		Message* pcMessage = NULL;
		String cString;
		bool bEnabled = false;
		cString.Format("%s %d","Clip", i+1);
		
		if (i<m_cCopiedTextVector.size())
		{
				pcMessage = new Message(M_CLIPPER_COPY);
				pcMessage->AddString("text",m_cCopiedTextVector[i]);
				bEnabled = true;
		}				
		
		os::MenuItem* pcItem = new MenuItem(cString,NULL);
		pcContextMenu->AddItem(pcItem);
		pcItem->SetEnable(bEnabled);
	}

	pcContextMenu->AddItem(new os::MenuSeparator());
	pcContextMenu->AddItem("Clear Content",new Message(M_CLIPPER_CLEAR));
	pcContextMenu->AddItem("About",new Message(M_CLIPPER_ABOUT));
	pcContextMenu->AddItem("Settings",new Message(M_CLIPPER_SETTINGS));
	pcContextMenu->SetTargetForItems(this);
}

bool DockClipper::IsInCopyVector(const os::String& cNewItem)
{
	for (uint i=0; i<m_cCopiedTextVector.size(); i++)
	{
		if (cNewItem == m_cCopiedTextVector[i])
		{
			return true;
		}
	}
	return false;
}

class DockPluginClipper : public DockPlugin
{
public:
	DockPluginClipper() {};
	
	status_t Initialize()
	{
		AddView(pcView = new DockClipper(GetPath(),GetApp()));
		return 0;
	}
	void Delete()
	{
		RemoveView(pcView);
	}
	String GetIdentifier()
	{
		return "Clipper";
	}
private:
	DockClipper* pcView;
};
 
extern "C"
{
DockPlugin* init_dock_plugin()
{
	return(new DockPluginClipper());
}
}


