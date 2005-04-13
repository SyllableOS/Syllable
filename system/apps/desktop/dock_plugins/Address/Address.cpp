#include "Address.h"
#include "messages.h"

#include <util/looper.h>
#include <gui/requesters.h>
#include <util/settings.h>
#include <storage/tempfile.h>

#include <stdio.h>
#include <unistd.h>

AddressDrop::AddressDrop(View* pcParent) : DropdownMenu(Rect(0,3,150,20),"address_drop")
{
	pcParentView = pcParent; 
}


void AddressDrop::KeyUp(const char* pzString, const char* pzRawString,uint32 nKeyCode)
{
	if (pzString[0] == VK_ENTER)
	{
		Looper* pcParentLooper = pcParentView->GetLooper();
		pcParentLooper->PostMessage(new Message(M_SELECTION),pcParentView);
	}
}

void AddressDrop::KeyDown(const char* pzString,const char* pzRawString,uint32 nKeyCode)
{
	printf("this will never work!!!!!!\n");
}

Address::Address(os::Path cPath, os::Looper* pcDock) : DockPlugin()
{
	m_pcDock = pcDock;
	m_cPath = cPath;

	pcAddressDrop = new AddressDrop(this);
	AddChild(pcAddressDrop);
}

Address::~Address()
{
}

void Address::DetachedFromWindow()
{	
	SaveSettings();
	SaveBuffer();
	SaveAddresses();
}

void Address::InitDefaultAddresses()
{
	cSites.push_back(std::make_pair("g:","http://www.google.com/search?hl=en&q="));
	cSites.push_back(std::make_pair("w:","http://en.wikipedia.org/wiki/"));	
	cSites.push_back(std::make_pair("y:","http://search.yahoo.com/search?fr=FP-pull-web-t&p="));
	cSites.push_back(std::make_pair("th:","http://thesaurus.reference.com/search?q="));
	cSites.push_back(std::make_pair("dic:","http://dictionary.reference.com/search?q="));
	cSites.push_back(std::make_pair("kam:","http://kamidake.other-space.com/search.php?search_names="));
}

void Address::LoadAddresses()
{
	String cSettingsPath = String(getenv("HOME")) + String("/Settings/Address");
	mkdir( cSettingsPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO );
	
	try
	{
		cSettingsPath += "/Addresses";
		os::File* pcFile = new File(cSettingsPath);
		os::Settings* pcSettings = new Settings(pcFile);
		pcSettings->Load();
	
		for (int i=0; i<pcSettings->CountNames(); i++)
		{
			String cTag = pcSettings->GetName(i);
			
			String cAddress;
			pcSettings->FindString(cTag.c_str(),&cAddress);
			cSites.push_back(std::make_pair(cTag,cAddress));
		}
		delete (pcSettings);	
	}
	
	catch (...)
	{
		printf("cannot load\n");
	}	
}

void Address::ReloadAddresses()
{
	cSites.clear();
	InitDefaultAddresses();
	LoadAddresses();
}

void Address::LoadBuffer()
{
	String cSettingsPath = String(getenv("HOME")) + String("/Settings/Address");
	
	try
	{
		cSettingsPath += "/Buffer";
		os::File* pcFile = new File(cSettingsPath);
		os::Settings* pcSettings = new Settings(pcFile);
		pcSettings->Load();
	
		for (int i=0; i<pcSettings->CountNames(); i++)
		{
			String cName = pcSettings->GetName(i);
			m_cBuffer.push_back(cName);
			pcAddressDrop->AppendItem(cName);
		}
		delete (pcSettings);	
	}
	
	catch (...)
	{
		printf("cannot load\n");
	}	
}

void Address::SaveBuffer()
{
	String cSettingsPath = String(getenv("HOME")) + String("/Settings/Address");
	cSettingsPath += "/Buffer";
	
	os::File* pcFile = new File(cSettingsPath,O_RDWR | O_CREAT);
	os::Settings* pcSettings = new Settings(pcFile);
	
	
	for (int i=0; i<m_cBuffer.size(); i++)
	{
		pcSettings->AddString(m_cBuffer[i].c_str(),m_cBuffer[i]);
	}
	pcSettings->Save();
	delete (pcSettings);	
}

void Address::SaveAddresses()
{
	String cSettingsPath = String(getenv("HOME")) + String("/Settings/Address");
	cSettingsPath += "/Addresses";
	
	
	os::File* pcFile = new File(cSettingsPath,O_RDWR | O_CREAT);
	os::Settings* pcSettings = new Settings(pcFile);
	
	
	for (int i=cSites.size(); i>6; i--)
	{
		pcSettings->AddString(cSites[i-1].first.c_str(),cSites[i-1].second);
	}
	pcSettings->Save();
	delete (pcSettings);
}


void Address::LoadSettings()
{
	String cSettingsPath = String(getenv("HOME")) + String("/Settings/Address");
	
	try
	{
		cSettingsPath += "/Settings";
		os::File* pcFile = new File(cSettingsPath);
		os::Settings* pcSettings = new Settings(pcFile);
		pcSettings->Load();
	
		nDefault = pcSettings->GetInt32("default",nDefault);
		bExportHelpFile = pcSettings->GetBool("help",&bExportHelpFile);
	}
	
	catch (...)
	{
		bExportHelpFile = false;
		nDefault = 0;
	}
}

void Address::SaveSettings()
{
	String cSettingsPath = String(getenv("HOME")) + String("/Settings/Address");
	cSettingsPath += "/Settings";
	
	
	os::File* pcFile = new File(cSettingsPath,O_RDWR | O_CREAT);
	os::Settings* pcSettings = new Settings(pcFile);
	
	pcSettings->AddInt32("default",nDefault);
	pcSettings->AddBool("help",true);
	
	pcSettings->Save();
	delete (pcSettings);
}

	
void Address::AttachedToWindow()
{
	os::File* pcFile = new os::File( m_cPath );
	os::Resources cCol( pcFile );
	pcStream = cCol.GetResourceStream( "icon48x48.png" );
	m_pcIcon = new os::BitmapImage( pcStream );
	delete pcFile;
		
	pcSettingsWindow = NULL;
	
	InitDefaultAddresses();
	LoadAddresses();
	LoadBuffer();
	LoadSettings();
	ExportHelpFile();
	
	pcAddressDrop->SetTarget(this);
}

String Address::GetIdentifier()
{
	return( PLUGIN_NAME );
}

Point Address::GetPreferredSize( bool bLargest )
{
	return Point(pcAddressDrop->GetBounds().Width(),pcAddressDrop->GetBounds().Height());
}

void Address::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_SELECTION:
			{
				String cUrl;
				
				String cCurrent = pcAddressDrop->GetCurrentString();
				
				if (cCurrent.size() > 0)
				{
					if (cCurrent == "help")
					{
						DisplayHelp();
						return;
					}
					else if (cCurrent == "about")
					{
						DisplayAbout();
						return;
					}
					else if (cCurrent == "config")
					{
						DisplaySettings();
						return;
					}
					
					else if (cCurrent == "clear")
					{
						ClearBuffer();
					}
					
					else if (cCurrent.substr(0,11) == "http://www." || cCurrent.substr(0,7) == "http://" || cCurrent.substr(0,1) == "/")
					{
						cUrl = cCurrent;
						ExecuteBrowser(cUrl);
						AddToBuffer(cUrl);
					}
					
					else if (cCurrent.substr(0,4) == "www.")
					{
						cUrl = String("http://") + cCurrent;
						ExecuteBrowser(cUrl);
						AddToBuffer(cUrl);
					} 
					
					else
					{
						for (int i=0; i<cSites.size(); i++)
						{
							if (cCurrent.substr(0,cSites[i].first.size()) == cSites[i].first)
							{
								cUrl = cSites[i].second + String(cCurrent.substr(cSites[i].first.size(),cCurrent.size()));
								ExecuteBrowser(cUrl);
								AddToBuffer(cCurrent);
								return;
							}
						}
						cUrl = cSites[nDefault].second + cCurrent;
						ExecuteBrowser(cUrl);
						AddToBuffer(cUrl);
					}
				}
				break;
			}
			
			case M_HELP:
				DisplayHelp();
				break;
				
			case M_SETTINGS:
				DisplaySettings();
				break;
				
			case M_ADDRESS_ABOUT:
				DisplayAbout();
				break;
				
			case M_SETTINGS_PASSED:
				pcMessage->FindInt32("default",&nDefault);
				ReloadAddresses();
				pcSettingsWindow->PostMessage(new Message(M_TERMINATE),pcSettingsWindow);
				pcSettingsWindow = NULL;
				break;
				
			case M_SETTINGS_CANCEL:
				pcSettingsWindow->PostMessage(M_TERMINATE,pcSettingsWindow);
				pcSettingsWindow = NULL;
				break;
	}
}


void Address::DisplayAbout()
{
	String cTitle = (String)"About " + (String)PLUGIN_NAME + (String)"...";
	String cInfo = (String)"Version:  " +  (String)PLUGIN_VERSION + (String)"\n\nAuthor:   " + (String)PLUGIN_AUTHOR + (String)"\n\nDesc:      " + (String)PLUGIN_DESC;	
	
	Alert* pcAlert = new Alert(cTitle.c_str(),cInfo.c_str(),m_pcIcon->LockBitmap(),0,"_O.K",NULL);
	m_pcIcon->UnlockBitmap();
	pcAlert->Go(new Invoker());
	pcAlert->MakeFocus();
}

void Address::DisplaySettings()
{
	if (pcSettingsWindow == NULL)
	{
		pcSettingsWindow = new SettingsWindow(this,nDefault);
		pcSettingsWindow->CenterInScreen();
		pcSettingsWindow->Show();
		pcSettingsWindow->MakeFocus();
	}
	else
		pcSettingsWindow->MakeFocus();
}

void Address::DisplayHelp()
{
	ExecuteBrowser("/atheos/Documentation/address.html");
}

void Address::ExecuteBrowser(const String& cUrl)
{
	int nError=0;
	
	if (fork() == 0)
	{
		nError = execlp("/Applications/ABrowse/ABrowse","/Applications/ABrowse/ABrowse",cUrl.c_str(),NULL);	
	}	
	
	if (nError == -1)
	{
		if (errno == ENOENT)
		{
			Alert* pcAlert = new Alert("Address...","ABrowse does not exist, please install ABrowse first!!!",m_pcIcon->LockBitmap(),0,"_O.K",NULL);
			m_pcIcon->UnlockBitmap();
			pcAlert->CenterInScreen();
			pcAlert->Go(new Invoker());	
		}
		else
		{
			Alert* pcAlert = new Alert("Address...","Unforseen error with ABrowse, please install ABrowse again!!!!",m_pcIcon->LockBitmap(),0,"_O.K",NULL);
			m_pcIcon->UnlockBitmap();
			pcAlert->CenterInScreen();
			pcAlert->Go(new Invoker());	
		}
	}
	
}

void Address::AddToBuffer(String cUrl)
{
	pcAddressDrop->AppendItem(cUrl);
	m_cBuffer.push_back(cUrl);
}

void Address::ClearBuffer()
{
	pcAddressDrop->Clear();
	m_cBuffer.clear();
	SaveBuffer();
}

void Address::ExportHelpFile()
{
	if (!bExportHelpFile)
	{
		char buffer[8192];
	
		os::File* pcResourceFile = new os::File( m_cPath );	
		os::Resources cCol( pcResourceFile );
		ResStream* pcHelpStream = cCol.GetResourceStream( "address.html" );
	
		ssize_t nSize = pcHelpStream->GetSize();
		pcHelpStream->Read(buffer,nSize);
		delete pcHelpStream;
	
		File* pcFileTwo = new File("/atheos/Documentation/address.html",O_CREAT | O_RDWR);
		pcFileTwo->Write(buffer,nSize);
		pcFileTwo->WriteAttr("os::MimeType",O_TRUNC,ATTR_TYPE_STRING,"text/html",0,10);
		delete pcFileTwo;
	}
}

extern "C"
{
DockPlugin* init_dock_plugin( os::Path cPluginFile, os::Looper* pcDock )
{
	return( new Address( cPluginFile, pcDock ) );
}
}

























