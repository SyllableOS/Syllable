#include "SettingsTab_Font.h"

#include <gui/button.h>
#include <gui/textview.h>
#include <gui/listview.h>
#include <gui/dropdownmenu.h>
#include <gui/stringview.h>
#include <gui/spinner.h>
#include <gui/font.h>
#include <gui/checkbox.h>
#include <codeview/codeview.h>
using namespace cv;

const float pageW=400;
const float pageH=400;
const float border=5;

SettingsTab_Fonts::SettingsTab_Fonts(App* a,EditWin* code) : View(os::Rect(0,0,pageW, pageH),"SettingsTab Font", os::CF_FOLLOW_ALL)
{
    os::Rect r=GetBounds();
    pcFonts =  new os::DropdownMenu(os::Rect(), "Formatlist", os::CF_FOLLOW_LEFT | os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_TOP);
    pcFonts->SetReadOnly(true);
    //pcFonts->SetSelectionMessage();
    pcFonts->SetFrame(os::Rect(r.left+border+70, border, r.right-10*border, border+19));
    AddChild(pcFonts);
    
    
    pcSFonts = new os::StringView(os::Rect(), "", "Fonts:");
    pcSFonts->SetFrame(os::Rect(r.left+5, border, 70, border+19));
    AddChild(pcSFonts);

	pcSSize = new  os::StringView(os::Rect(), "", "Size:");
    pcSSize->SetFrame(os::Rect(r.left+5, border+30, 70, border+49));
    AddChild(pcSSize);
    
    pcDSize = new os::DropdownMenu(os::Rect(), "Sizetlist", os::CF_FOLLOW_LEFT | os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_TOP);
    pcDSize->SetReadOnly(true);
    //pcFonts->SetSelectionMessage();
    pcDSize->SetFrame(os::Rect(r.left+border+70, border+30, r.right-10*border, border+49));
    AddChild(pcDSize);
    
    pcSheared = new os::CheckBox(os::Rect(), "", "Shear text",NULL);
    pcSheared->SetFrame(os::Rect(r.left+5, border+60,GetBounds().Width()-border, border+90));
    AddChild(pcSheared);
    
    pcAliased = new os::CheckBox(os::Rect(), "", "Antialias text",NULL);
    pcAliased->SetFrame(os::Rect(r.left+5, border+100,GetBounds().Width()-border, border+130));
    AddChild(pcAliased);
    
	_Init();

}


void SettingsTab_Fonts::_Init()
{
	int nFamCount = os::Font::GetFamilyCount();
    for ( int i = 0 ; i < nFamCount ; ++i )
    {
        char zFamily[FONT_FAMILY_LENGTH+1];
        os::Font::GetFamilyInfo( i, zFamily );
        pcFonts->AppendItem( zFamily );
    }
    
    for (float vI = 5.0; vI < 25.0;  vI+=1.0){
    	char zI[4];
    	sprintf(zI, "%.1f",vI);
    	pcDSize->AppendItem(zI);
    }
}


void SettingsTab_Fonts::getFonts()
{}

void SettingsTab_Fonts::HandleMessage(os::Message* pcMessage)
{
    switch (pcMessage->GetCode())
    {
    }
}

void SettingsTab_Fonts::AllAttached(void)
{}






