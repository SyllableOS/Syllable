/*	CodeEdit - A programmers editor for Atheos
	Copyright (c) 2002 Andreas Engh-Halstvedt
 
	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Library General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.
	
	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Library General Public License for more details.
	
	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free
	Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
	MA 02111-1307, USA
*/
#include "SettingsDialog.h"

#include <gui/tabview.h>
#include <gui/button.h>

#include <util/message.h>

#include "App.h"
#include "EditWin.h"
#include "SettingsTab_Key.h"
#include "SettingsTab_Formats.h"
#include "SettingsTab_Font.h"
#include <codeview/codeview.h>
const float pageW=400;
const float pageH=300;

SettingsDialog::SettingsDialog(App* m, EditWin* code)
        : os::Window(os::Rect(100, 100, 200,100), "SettingsDialog", APP_NAME " Common Settings"),
        app(m)
{

    tabView=new os::TabView(os::Rect(), "SettingsTabView", os::CF_FOLLOW_ALL);
    okButton=new os::Button(os::Rect(), "OkButton", "OK", new os::Message(ID_OK),
                            os::CF_FOLLOW_LEFT | os::CF_FOLLOW_BOTTOM);
    applyButton=new os::Button(os::Rect(), "ApplyButton", "Apply", new os::Message(ID_APPLY),
                               os::CF_FOLLOW_LEFT | os::CF_FOLLOW_BOTTOM);
    cancelButton=new os::Button(os::Rect(), "CancelButton", "Cancel", new os::Message(ID_CANCEL),
                                os::CF_FOLLOW_LEFT | os::CF_FOLLOW_BOTTOM);

    keyTab=new SettingsTab_Key(app);
    formatTab=new SettingsTab_Formats(app);
    fontTab = new SettingsTab_Fonts(app, code);
    tabView->AppendTab("Fonts",fontTab);
    tabView->AppendTab("Formats", formatTab);
    tabView->AppendTab("Key Bindings", keyTab);


    os::Point p1=okButton->GetPreferredSize(false);
    os::Point p2=applyButton->GetPreferredSize(false);
    os::Point p3=cancelButton->GetPreferredSize(false);

    float bw=max(max(p1.x, p2.x), p3.x);
    float w=max(pageW, 3*bw+10);
    float h=pageH+30+p1.y+15;
    float y=pageH+40;
    float x=5;

    tabView->SetFrame(os::Rect(5, 5, pageW+5, pageH+30));
    okButton->SetFrame(os::Rect(x, y, x+bw, y+p1.y));
    x+=bw+5;
    applyButton->SetFrame(os::Rect(x, y, x+bw, y+p1.y));
    x+=bw+5;
    cancelButton->SetFrame(os::Rect(x, y, x+bw, y+p1.y));

    ResizeTo(w+10, h);
    AddChild(tabView);
    AddChild(okButton);
    AddChild(applyButton);
    AddChild(cancelButton);
}

void SettingsDialog::apply()
{
    formatTab->apply();
    keyTab->apply();
}

void SettingsDialog::HandleMessage(os::Message* m)
{
    switch(m->GetCode())
    {
    case ID_OK:
        apply();
        app->PostMessage(App::ID_OPTIONS_CLOSED);
        Quit();
        break;
    case ID_APPLY:
        apply();
        break;
    case ID_CANCEL:
        app->PostMessage(App::ID_OPTIONS_CLOSED);
        Quit();
        break;
    }
}

bool SettingsDialog::OkToQuit()
{
    PostMessage(ID_CANCEL, this);
    return false;
}












