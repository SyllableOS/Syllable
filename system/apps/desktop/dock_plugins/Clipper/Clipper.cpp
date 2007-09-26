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
#include <util/datetime.h>
#include <util/event.h>
#include <util/resources.h>
#include <util/settings.h>
#include <util/clipboard.h>
#include <util/messenger.h>

#include <storage/file.h>
#include <storage/streamableio.h>

#include <atheos/time.h>

#include <appserver/dockplugin.h>
                    
#define PLUGIN_NAME       "Clipper"
#define PLUGIN_VERSION    "1.0"
#define PLUGIN_DESC       "A Clipboard Manager Plugin for Dock"
#define PLUGIN_AUTHOR     "Rick Caudill"
                                                                                                                                                                                                        
using namespace os;
using namespace std;                                                                                                                                                                                                     

class ClipperSettingsWindow;

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

typedef std::pair<std::string,std::vector<std::string> >  ClipperData;


/*looper*/
class ClipperLooper : public os::Looper
{
public:
	ClipperLooper(os::Looper*, os::View*);
	virtual void HandleMessage(os::Message*);
public:
	enum
	{
		CLIP_CHANGED = 0x1111f
	};

private:
	os::String GetDateString(os::DateTime data);
private:
	os::Looper* pcParentLooper;
	os::View* pcParentView;
	os::Event* pcEvent;
	bool bFirst;
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
	
	public:
		void RefreshMenu();	
		os::Menu* GetMenu(ClipperData data);
		
	public:
		vector<ClipperData>::const_iterator GetClipperDataBegin() const
		{
			return m_cCopiedTextVector.begin();
		}
		vector<ClipperData>::const_iterator GetClipperDataEnd() const
		{
			return m_cCopiedTextVector.end();
		}
		uint GetClipperDataSize()
		{
			return m_cCopiedTextVector.size();
		}
		
		uint GetClipSize();
		
	private:
		void UpdateItems(const String&);
		void ClearContent();
		bool IsInCopyVector(const os::String& cNewItem);
		int  Contains(const os::String& cDate);
		void LoadSettings();
		void SaveSettings();
		void Insert(const os::String& cDate, const os::String& cClip);


	private:
		vector<ClipperData> m_cCopiedTextVector;
		
		os::Path m_cPath;
		os::Looper* m_pcDock;
		
		int m_nCount;
	
		ClipperSettingsWindow* pcWindow;
		Menu* pcContextMenu;
		Menu* pcExportMenu;
		
		BitmapImage* m_pcIcon;
		os::File* pcFile;
		os::ResStream *pcStream;
		os::PopupMenu* pcPopup;
		ClipperLooper* pcClipperLooper;
};

/*settings window*/
class ClipperSettingsWindow : public Window
{
public:
	ClipperSettingsWindow(DockClipper*);
	
	void Layout();
	bool OkToQuit();
	void HandleMessage(Message*);
	void UpdateTreeView();
	void SetTime();
	int Contains(os::String date);
private:

	/*view window*/
	class ClipperWindow : public Window
	{
	public:
	ClipperWindow(os::Looper* pcParent) : Window(Rect(0,0,200,200),"view_window","Viewing Clip...",WND_NO_DEPTH_BUT | WND_NOT_RESIZABLE | WND_NO_ZOOM_BUT)
	{
		pcParentLooper = pcParent;
		AddChild(pcViewTextView = new TextView(Rect(5,5,195,195),"text",""));
		pcViewTextView->SetReadOnly(true);
		pcViewTextView->SetMultiLine(true);
	}
	void ChangeText(const String& cText)
	{
		pcViewTextView->Set(cText.c_str());
	}
	
	bool OkToQuit() 
	{ 
		pcParentLooper->PostMessage(new Message(M_CLIPPER_SEND),pcParentLooper);
		return true;
	}	
	private:
		TextView* pcViewTextView;
		os::Looper* pcParentLooper;
	};	


private:

	DockClipper* pcParentView;
	String cWindowText;
	
	TreeView* pcTreeView;
	TextView* pcTextView;

	LayoutView* pcLayoutView;
	FrameView* pcFrameView;
	Button *pcOkButton, *pcViewButton;
	ClipperWindow* pcWindow;

};




ClipperLooper::ClipperLooper(Looper* pcParent, os::View* pcView) : Looper("ClipperLooper")
{
	pcParentLooper = pcParent;
	pcParentView = pcView;

	bFirst = true;

	pcEvent = new os::Event();
	pcEvent->SetToRemote("os/System/ClipboardHasChanged");
	pcEvent->SetMonitorEnabled(true,this,CLIP_CHANGED);
}

void ClipperLooper::HandleMessage(os::Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case CLIP_CHANGED:
		{
			if (bFirst)
			{
				bFirst = false;
			}
			else
			{
				Clipboard cClip;
				cClip.Lock();
				Message* pcMsg = cClip.GetData();
				os::String cString;
			
				if (pcMsg->FindString("text/plain", &cString) == 0)
				{
					Message* pcTargetMessage = new Message(M_CLIPPER_SEND);
					pcTargetMessage->AddString("clip",cString);
					pcTargetMessage->AddString("date",GetDateString(os::DateTime::Now()));
					pcParentLooper->PostMessage(pcTargetMessage,pcParentView);
				}
				cClip.Unlock();
			}
			break;
		}
	}
}

os::String ClipperLooper::GetDateString(os::DateTime data)
{
	os::String cReturn = os::String().Format("%d-%d-%d",data.GetYear(),data.GetMonth(),data.GetDay());
	return cReturn;
}


ClipperSettingsWindow::ClipperSettingsWindow(DockClipper* pcParent) : Window(Rect(0,0,380,304),"settings_window","Viewing Clips...",WND_NOT_RESIZABLE | WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT)
{
	pcParentView = pcParent;
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
	pcTreeView->SetInvokeMsg(new Message(M_PREFS_VIEW));

  	HLayoutNode* pcHLButton = new os::HLayoutNode("");
    pcHLButton->AddChild(new os::HLayoutSpacer("",5.0f,5.0f));	
  	pcHLButton->AddChild(pcOkButton=new Button(Rect(0,0,2,2),"BTApply","_Okay",new Message(M_PREFS_APPLY)));
  	pcHLButton->AddChild(new os::HLayoutSpacer("",5.0f,5.0f));
  	
  	pcRoot->AddChild(pcTreeViewLayout);
  	pcRoot->AddChild(new VLayoutSpacer("",10,10));
	pcRoot->AddChild(pcHLButton);
  	pcRoot->AddChild(new VLayoutSpacer("",5,5));

	pcLayoutView->SetRoot(pcRoot);

	AddChild(pcLayoutView);
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
			if (pcWindow != NULL)
				pcWindow->Close();
			pcWindow = NULL;	
			pcParentView->GetLooper()->PostMessage(new Message(M_PREFS_CANCEL),pcParentView);			
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

void ClipperSettingsWindow::UpdateTreeView()
{
	pcTreeView->Clear();

	vector<ClipperData>::const_iterator iter = pcParentView->GetClipperDataBegin();
	uint size = pcParentView->GetClipperDataSize();
	
	for (uint i=0; i<size; i++)
	{
		
			TreeViewStringNode* node = new TreeViewStringNode();
			node->SetIndent(1);
			node->AppendString(iter[i].first);
			pcTreeView->InsertNode(node,true);
			
			
			for (uint j=0; j<iter[i].second.size(); j++)
			{
				node = new TreeViewStringNode();
				node->SetIndent(2);
				node->AppendString(os::String().Format("Clip %d",j+1));
				node->SetCookie(os::Variant(iter[i].second[j]));
				pcTreeView->InsertNode(node,true);
			}
	}
}

//*************************************************************************************
DockClipper::DockClipper( os::Path cPath, os::Looper* pcDock ) : View(Rect(0,0,0,0),"DockClipperView")
{
	m_pcDock = pcDock;
	m_cPath = cPath;	

	pcContextMenu = NULL;
	pcWindow = NULL;

	/* Load default icons */
	pcFile = new os::File( m_cPath );
	os::Resources cCol( pcFile );
	pcStream = cCol.GetResourceStream( "icon24x24.png" );
	m_pcIcon = new os::BitmapImage( pcStream );
	delete pcFile;
	
	InitializeMenu();
	LoadSettings();

	
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
		
		m_nCount = pcSettings->GetInt32("count",0);

			
		for (int i = 0; i<m_nCount; i++)
		{
			int contains = -1;
				
			os::String cClip;
			os::String cDate;
			
			cClip = pcSettings->GetString("clip","",i);
			cDate = pcSettings->GetString("date","",i);
			
			Insert(cDate,cClip);

		}
		
		RefreshMenu();
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

		pcSettings->AddInt32("count",GetClipSize());
		
		for (uint i=0; i<m_cCopiedTextVector.size(); i++)
		{
			vector<std::string>::const_iterator iter=m_cCopiedTextVector[i].second.begin();
			
			for (uint j=0; j<m_cCopiedTextVector[i].second.size(); j++)
			{	
				pcSettings->SetString("date",m_cCopiedTextVector[i].first,j);
				pcSettings->SetString("clip",iter[j],j);
			}
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
			os::String date;
			os::String clip;
			if (pcMessage->FindString("date",&date) == 0  && pcMessage->FindString("clip",&clip) == 0)
			{
				if(!IsInCopyVector(clip))
				{
					Insert(date,clip);
					RefreshMenu();
					SaveSettings();
				}
			}
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
				pcWindow = new ClipperSettingsWindow(this);
				pcWindow->CenterInScreen();
				pcWindow->Show();
				pcWindow->MakeFocus();
			}
			else
			{
				//pcWindow->UpdateClips(m_cCopiedTextVector);
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
	}
}


void DockClipper::ClearContent()
{
	m_cCopiedTextVector.clear();
	SaveSettings();
	RefreshMenu();	
}

void DockClipper::RefreshMenu()
{
	std::sort(m_cCopiedTextVector.begin(),m_cCopiedTextVector.end());

	for (int i=pcContextMenu->GetItemCount(); i >0; i--)
		pcContextMenu->RemoveItem(i-1);
		
	delete pcContextMenu;
	pcContextMenu = NULL;
	
	InitializeMenu();
}

void DockClipper::InitializeMenu()
{
	
	if (pcContextMenu == NULL)
		pcContextMenu = new Menu(Rect(0,0,1,1),"menu",ITEMS_IN_COLUMN);
	
	
	for (int i=0; i<m_cCopiedTextVector.size(); i++)
	{
		Menu* pcMenu = GetMenu(m_cCopiedTextVector[i]);
		
		
		for (uint j=0; j<m_cCopiedTextVector[i].second.size(); j++)
		{
			Message* pcMessage = new os::Message(M_CLIPPER_COPY);
			pcMessage->AddString("text",m_cCopiedTextVector[i].second[j]);
		
			os::String menuItemString = os::String().Format("Clip %d",pcMenu->GetItemCount()+1);
		
			os::MenuItem* pcItem = new MenuItem(menuItemString,pcMessage);
			pcMenu->AddItem(pcItem);
		}
	}

	pcContextMenu->AddItem(new os::MenuSeparator());

	pcContextMenu->AddItem("View Clips",new Message(M_CLIPPER_SETTINGS));
	pcContextMenu->AddItem("Clear Clips",new Message(M_CLIPPER_CLEAR));

	pcContextMenu->AddItem(new os::MenuSeparator());
	pcContextMenu->AddItem("About",new Message(M_CLIPPER_ABOUT));	
	pcContextMenu->SetTargetForItems(this);
}


os::Menu* DockClipper::GetMenu(ClipperData data)
{
	os::String menuString = data.first;
	os::Menu* pcMenu = NULL;
	

	for (int i=0; i<pcContextMenu->GetItemCount(); i++)
	{
		os::MenuItem* tempItem = pcContextMenu->GetItemAt(i);
		if (tempItem->GetLabel() == menuString)
		{
			pcMenu = tempItem->GetSubMenu();
		} 
	}
	
	
	if (pcMenu == NULL)
	{
		pcMenu = new os::Menu(os::Rect(0,0,1,1),data.first,ITEMS_IN_COLUMN);
		pcContextMenu->AddItem(pcMenu);
	}
	return pcMenu;
}

bool DockClipper::IsInCopyVector(const os::String& cNewItem)
{
	for (uint i=0; i<m_cCopiedTextVector.size(); i++)
	{
		for (int j=0; j<m_cCopiedTextVector[i].second.size(); j++)
		{
			if (cNewItem == m_cCopiedTextVector[i].second[j])
			{
				return true;
			}
		}
	}
	return false;
}

int DockClipper::Contains(const os::String& cDate)
{
	int contains = -1;
	
	for (int i=0; i<m_cCopiedTextVector.size(); i++)
	{
		if (cDate == m_cCopiedTextVector[i].first)
		{
			contains = i;
			break;
		}
	}
	return contains;
}


void DockClipper::Insert(const os::String& cDate, const os::String& cClip)
{
	int contains = -1;
	if ( (contains = Contains(cDate)) == -1)
	{
		std::vector<std::string> list;
		list.push_back(cClip);
		m_cCopiedTextVector.push_back(make_pair(cDate,list));
	}
	else
	{
		m_cCopiedTextVector[contains].second.push_back(cClip);
	}
}

uint DockClipper::GetClipSize()
{
	uint size=0;
	
	for (int i=0; i<m_cCopiedTextVector.size(); i++)
	{
		size += m_cCopiedTextVector[i].second.size();
	}
	return size;
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












































