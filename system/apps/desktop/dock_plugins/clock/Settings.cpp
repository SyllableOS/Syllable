#include "Settings.h"
#include "messages.h"
#include <util/datetime.h>
#include <atheos/time.h>

DockClockSettings::DockClockSettings(Message* pcMessage,View* pcPlugin) : Window(Rect(0,0,550,150),"clock_settings","Clock Settings...")
{
	pcSendMessage = pcMessage;
	pcParentPlugin = pcPlugin;
	pcSelector = NULL;
	
	Layout();
	LoadSettings();
}

void DockClockSettings::LoadSettings()
{
	String cFormat;
	pcSendMessage->FindString("time_format",&cTimeFormat);

	
	if (cTimeFormat == "%H:%m")
		pcTimeDrop->SetSelection(0);
	else if(cTimeFormat == "%H:%m:%s")
		pcTimeDrop->SetSelection(1);
	else if(cTimeFormat == "%h:%m")
		pcTimeDrop->SetSelection(2);
	else
		pcTimeDrop->SetSelection(3);
		
	pcSendMessage->FindString("date_format",&cFormat);
	
	if (cFormat == "%a %b %e, %Y")
		pcDateDrop->SetSelection(0);
	else if(cFormat == "%b %e, %Y")
		pcDateDrop->SetSelection(1);
	else if(cFormat == "e %b %Y")
		pcDateDrop->SetSelection(2);
	else if(cFormat == "%m/%e/%Y")
		pcDateDrop->SetSelection(3);
	else
		pcDateDrop->SetSelection(4);
		
	pcSendMessage->FindString("font",&cFormat);
	
	for (int i=0; i<pcFontTypeDrop->GetItemCount(); i++)
	{
		if (pcFontTypeDrop->GetItem(i) == cFormat)
			pcFontTypeDrop->SetSelection(i);
	}
	
	String cFontSize;
	pcSendMessage->FindFloat("size",&vFontSize);
	cFontSize.Format("%.1f",vFontSize);
	
	for (int i=0; i<pcFontSizeDrop->GetItemCount(); i++)
	{
		if (pcFontSizeDrop->GetItem(i) == cFontSize)
			pcFontSizeDrop->SetSelection(i);
	}
	
	Color32_s cTextColor;
	pcSendMessage->FindColor32("color",&cTextColor);
	pcFontColorButton->SetColor(cTextColor);
	
	pcSendMessage->FindBool("frame",&bFrame);
	pcFrameCheck->SetValue(bFrame);
}

bool DockClockSettings::OkToQuit()
{
	pcParentPlugin->GetLooper()->PostMessage(M_CANCEL,pcParentPlugin);
	return false;
}

void DockClockSettings::Layout()
{
	pcLayoutView = new LayoutView(GetBounds(),"settings_layout",NULL);
	
	VLayoutNode* pcRoot = new VLayoutNode("root_node");

	pcRoot->SetBorders(Rect(10,5,10,5));
	
	HLayoutNode* pcButtonNode = new HLayoutNode("button_node");
	pcButtonNode->AddChild(pcOkButton = new Button(Rect(),"but_ok","_Save",new Message(M_APPLY)));
	pcButtonNode->AddChild(new HLayoutSpacer("",10,10));
	pcButtonNode->AddChild(pcDefaultButton = new Button(Rect(),"but_default","_Default",new Message(M_DEFAULT)));
	pcButtonNode->AddChild(new HLayoutSpacer("",10,10));	
	pcButtonNode->AddChild(pcCancelButton = new Button(Rect(),"but_cancel","_Cancel",new Message(M_CANCEL)));

	HLayoutNode* pcFontNode = new HLayoutNode("font_node");
	pcFontNode->SetHAlignment(os::ALIGN_LEFT);
	pcFontNode->AddChild(pcFontString = new StringView(Rect(),"font_string","Font:"));
	pcFontNode->AddChild(new HLayoutSpacer("",5,5));
	
	pcFontTypeDrop = new DropdownMenu(Rect(),"DDMType");//,CF_FOLLOW_ALL);
	pcFontTypeDrop->SetMaxPreferredSize(15);
	pcFontTypeDrop->SetMinPreferredSize(15);
	pcFontTypeDrop->SetReadOnly(true);
	SetFontTypes();
	pcFontNode->AddChild(pcFontTypeDrop);
	pcFontNode->AddChild(new HLayoutSpacer("",2,2));
	pcFontNode->AdjustPrefSize( new Point(550,20),new Point(550,20));
	pcFontSizeDrop = new DropdownMenu(Rect(),"DDMSize");//,CF_FOLLOW_ALL);
	pcFontSizeDrop->SetMaxPreferredSize(15);
	pcFontSizeDrop->SetMinPreferredSize(15);
	pcFontSizeDrop->SetReadOnly(true);

	for (int i=5; i<=28; i++)
	{
		char cItem[6];
		sprintf(cItem,"%d.0",i);
		pcFontSizeDrop->AppendItem(cItem);
		sprintf(cItem,"%d.5",i);
		pcFontSizeDrop->AppendItem(cItem);
	}
	
	pcFontNode->AddChild(pcFontSizeDrop);
	pcFontNode->AddChild(new HLayoutSpacer("",2,2));
	pcFontNode->AddChild(pcFontColorButton = new ColorButton(Rect(),"color",new Message(M_COLOR_SELECTOR_WINDOW),Color32_s(0,0,0,0)));
	
	HLayoutNode* pcTimeNode = new HLayoutNode("time_node");
	pcTimeNode->AdjustPrefSize( new Point(550,20),new Point(550,20));	
	pcTimeNode->SetHAlignment(os::ALIGN_LEFT);	
	pcTimeNode->AddChild(pcTimeString = new StringView(Rect(),"time_string","Display Time As:"));
	pcTimeNode->AddChild(new HLayoutSpacer("",5,5));
	pcTimeNode->AddChild(pcTimeDrop = new DropdownMenu(Rect(),"DDMTime",CF_FOLLOW_ALL));
	pcTimeDrop->SetMaxPreferredSize(15);
	pcTimeDrop->SetMinPreferredSize(15);
	pcTimeDrop->SetReadOnly(true);
	LoadTimes();
	
	HLayoutNode* pcDateNode = new HLayoutNode("date_node");
	pcDateNode->AdjustPrefSize( new Point(550,20),new Point(550,20));	
	pcDateNode->SetHAlignment(os::ALIGN_LEFT);	
	pcDateNode->AddChild(pcDateString = new StringView(Rect(),"date_string","Display Date As:"));
	pcDateNode->AddChild(new HLayoutSpacer("",5,5));
	pcDateNode->AddChild(pcDateDrop = new DropdownMenu(Rect(),"DDMDate",CF_FOLLOW_ALL));
	pcDateDrop->SetMaxPreferredSize(15);
	pcDateDrop->SetMinPreferredSize(15);
	pcDateDrop->SetReadOnly(true);
	LoadDates();
	
	pcRoot->AddChild(pcFontNode);
	pcRoot->AddChild(new VLayoutSpacer("",5,5));
	pcRoot->AddChild(pcTimeNode);
	pcRoot->AddChild(new VLayoutSpacer("",5,5));
	pcRoot->AddChild(pcDateNode);
	pcRoot->AddChild(new VLayoutSpacer("",5,5));
	pcRoot->AddChild(pcFrameCheck = new CheckBox(Rect(),"frame","Draw Frame Around Clock",NULL));		
	pcRoot->AddChild(pcButtonNode);
	pcRoot->AddChild(new VLayoutSpacer("",5,5));
	
	pcRoot->SameWidth("DDMDate","DDMTime",NULL);
	pcRoot->SameWidth("but_ok","but_cancel","but_default",NULL);
	pcRoot->SameHeight("but_ok","but_cancel","but_default",NULL);
	
	pcLayoutView->SetRoot(pcRoot);
	AddChild(pcLayoutView);	
}

void DockClockSettings::LoadTimes()
{
	long nCurSysTime = get_real_time( ) / 1000000;

    struct tm *psTime = gmtime( &nCurSysTime );

    long anTimes[] = 
    { 
      psTime->tm_mday,
      psTime->tm_hour,
      (psTime->tm_hour > 12) ? psTime->tm_hour - 12 : psTime->tm_hour,
      psTime->tm_mon + 1,
      psTime->tm_min,
      psTime->tm_sec,
      psTime->tm_year - 100
    };
    
	String cTime;
	
	cTime.Format("%02ld:%d",anTimes[1],anTimes[4]);
	pcTimeDrop->AppendItem(cTime);
	cTime.Format("%02ld:%d:%02ld",anTimes[1],anTimes[4],anTimes[5]);
	pcTimeDrop->AppendItem(cTime);
	
	
	cTime.Format("%d:%d",anTimes[2],anTimes[4]);
    pcTimeDrop->AppendItem(cTime);
    cTime.Format("%d:%d:%02ld",anTimes[2],anTimes[4],anTimes[5]);
    pcTimeDrop->AppendItem(cTime);
}

void DockClockSettings::LoadDates()
{
	String cDate;
	DateTime d = DateTime::Now();
	char bfr[256];
	struct tm *pstm;
	time_t nTime = d.GetEpoch();

	pstm = localtime( &nTime );

	strftime( bfr, sizeof( bfr ), "%a %b %e, %Y", pstm );
	cDate = String(bfr);
	pcDateDrop->AppendItem(cDate);
	
	strftime( bfr, sizeof( bfr ), "%b %e, %Y", pstm );
	cDate = String(bfr);
	pcDateDrop->AppendItem(cDate);
	
	strftime( bfr, sizeof( bfr ), "%e %b %Y", pstm );
	cDate = String(bfr);
	pcDateDrop->AppendItem(cDate);
	
	strftime( bfr, sizeof( bfr ), "%m/%e/%Y", pstm );
	cDate = String(bfr);
	pcDateDrop->AppendItem(cDate);

	strftime( bfr, sizeof( bfr ), "%e.%m.%Y", pstm );
	cDate = String(bfr);
	pcDateDrop->AppendItem(cDate);
}

void DockClockSettings::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_COLOR_SELECTOR_WINDOW:
		{
			if (pcSelector == NULL)
			{
				pcSelector = new ColorSelectorWindow(0,pcFontColorButton->GetColor(),this);
				pcSelector->CenterInWindow(this);
				pcSelector->Show();
				pcSelector->MakeFocus();
			}
			else
				pcSelector->MakeFocus();
			
		}
		break;
		
		case M_COLOR_SELECTOR_WINDOW_REQUESTED:
		{
			pcMessage->FindColor32("color",&cTextColor);
			pcFontColorButton->SetColor(cTextColor);
			pcSelector->Close();
			pcSelector = NULL;
			break;
		}
		
		case M_COLOR_SELECTOR_WINDOW_CANCEL:
		{
			pcSelector->Close();
			pcSelector = NULL;
			break;
		}
		
		case M_CANCEL:
			OkToQuit();
			break;
			
		case M_APPLY:
			SaveSettings();
			break;
		
		case M_DEFAULT:
			break;
	}
}

void DockClockSettings::SaveSettings()
{
	Message* pcMsg = new Message(M_APPLY);
	pcMsg->AddString("font",pcFontTypeDrop->GetCurrentString());
	pcMsg->AddFloat("size",atof(pcFontSizeDrop->GetCurrentString().c_str()));
	pcMsg->AddColor32("color",pcFontColorButton->GetColor());
	pcMsg->AddString("time_format",GetTimeFormat());
	pcMsg->AddString("date_format",GetDateFormat());
	pcMsg->AddBool("frame",pcFrameCheck->GetValue().AsBool());
	pcParentPlugin->GetLooper()->PostMessage(pcMsg,pcParentPlugin);
	delete( pcMsg );
}

String DockClockSettings::GetTimeFormat()
{
	String cReturn="%H:%m";
	
	switch (pcTimeDrop->GetSelection())
	{
		case 0:
			cReturn = "%H:%m";
			break;
			
		case 1:
			cReturn = "%H:%m:%s";
			break;
			
		case 2:
			cReturn = "%h:%m";
			break;
			
		case 3:
			cReturn = "%h:%m:%s";
	}
	return cReturn;
}

String DockClockSettings::GetDateFormat()
{
	String cReturn = "%a %b %e, %Y";
	
	switch (pcDateDrop->GetSelection())
	{
		case 0:
			cReturn = "%a %b %e, %Y";
			break;
			
		case 1:
			cReturn = "%b %e, %Y";
			break;
			
		case 2:
			cReturn = "%e %b %Y";
			break;
			
		case 3:
			cReturn = "%m/%e/%Y";
			break;
		case 4:
			cReturn = "%e.%m.%Y";
			break;
	}
	return cReturn;
}

void DockClockSettings::SetFontTypes()
{
	char zFontFamily[FONT_FAMILY_LENGTH];
	char zFontStyle[FONT_STYLE_LENGTH];
	int nFamilyCount = Font::GetFamilyCount();
	int nStyleCount;
	uint32 nFlags;

	for(int i=0; i<nFamilyCount; i++)
	{
		if (Font::GetFamilyInfo(i, zFontFamily) == 0)
		{
			if (strcmp(zFontFamily,"Dingbats") != 0)
			{
				nStyleCount = Font::GetStyleCount(zFontFamily);
			
				for (int j=0; j<nStyleCount; j++)
				{
					if (Font::GetStyleInfo(zFontFamily,j,zFontStyle,&nFlags) == 0 && ( ( strcmp(zFontStyle,"Regular") == 0 ) || ( strcmp(zFontStyle,"Roman") == 0 ) ) )
					{
						pcFontTypeDrop->AppendItem(zFontFamily);
					}
				}
			}
		}
	}
}











