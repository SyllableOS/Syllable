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
#include "SettingsTab_Formats.h"

#include <gui/button.h>
#include <gui/textview.h>
#include <gui/listview.h>
#include <gui/dropdownmenu.h>
#include <gui/stringview.h>
#include <gui/spinner.h>

#include <codeview/format.h>
#include <codeview/format_c.h>
#include <codeview/format_java.h>
#include <codeview/format_perl.h>
#include <codeview/format_html.h>

#include <iostream>

using namespace cv;
using namespace std;

const float pageW=400;
const float pageH=400;
const float border=5;

inline static os::Color32_s invert(os::Color32_s c)
{
    c.red=255-c.red;
    c.green=255-c.green;
    c.blue=255-c.blue;
    return c;
}

//very quick hack to get colored rows
//code mostly stolen from os::ListViewStringRow
class ColorStringRow : public os::ListViewStringRow
{
public:
    os::Color32_s fgcolor, bgcolor;
    string text;

    ColorStringRow()
    {
        AppendString("");
    }

    virtual void Paint(const os::Rect &frame, os::View *view, uint col, bool sel, bool highlight, bool focus)
    {
        os::font_height sHeight;
        view->GetFontHeight( &sHeight );

        if(sel)
        {
            view->FillRect(frame, fgcolor);
            view->SetFgColor(bgcolor);
            view->SetBgColor(fgcolor);
        }
        else
        {
            view->FillRect(frame, bgcolor);
            view->SetFgColor(fgcolor);
            view->SetBgColor(bgcolor);
        }

        float vFontHeight = sHeight.ascender + sHeight.descender;
        float vBaseLine = frame.top + (frame.Height()+2.0f) / 2 - vFontHeight / 2 + sHeight.ascender;
        view->MovePenTo( frame.left + 3.0f, vBaseLine );

        view->DrawString(text.c_str());

    }
};

SettingsTab_Formats::SettingsTab_Formats(App* a)
        : View(os::Rect(0, 0, pageW, pageH), "SettingsTab Formats", os::CF_FOLLOW_ALL),
        app(a),
        iFormat(0)
{
    formatList=app->getFormatList();
    os::Rect r=GetBounds();

    mFormat=new os::DropdownMenu(os::Rect(), "Formatlist",
                                 os::CF_FOLLOW_LEFT | os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_TOP);
    mFormat->SetReadOnly(true);
    mFormat->SetSelectionMessage(new os::Message(ID_FORMAT_CHANGE));

    bAdd=new os::Button(os::Rect(), "AddButton", "Add", new os::Message(ID_ADD),
                        os::CF_FOLLOW_TOP | os::CF_FOLLOW_RIGHT);
    bRemove=new os::Button(os::Rect(), "RemoveButton", "Remove", new os::Message(ID_REMOVE),
                           os::CF_FOLLOW_TOP | os::CF_FOLLOW_RIGHT);
    os::Point p0=bAdd->GetPreferredSize(false);
    os::Point p1=bRemove->GetPreferredSize(false);
    float w=max(p0.x, p1.x);

    bRemove->SetFrame(os::Rect(r.right-border-w, border, r.right-border, border+p0.y));
    bAdd->SetFrame(os::Rect(r.right-2*border-2*w, border, r.right-2*border-w, border+p0.y));
    mFormat->SetFrame(os::Rect(r.left+border, border, r.right-3*border-2*w, border+p0.y));
    r.top+=p0.y+2*border;
    AddChild(mFormat);
    AddChild(bAdd);
    AddChild(bRemove);


    float left=100;

    tName=new os::TextView(os::Rect(), "", "",
                           os::CF_FOLLOW_TOP | os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_LEFT);
    p0=tName->GetPreferredSize(false);
    tName->SetMultiLine(false);
    tName->SetFrame(os::Rect(left, r.top, pageW-border, r.top+p0.y));
    os::StringView *sv=new os::StringView(os::Rect(border, r.top, left-border, r.top+p0.y),
                                          "", "Format name",
                                          os::ALIGN_LEFT, os::CF_FOLLOW_LEFT | os::CF_FOLLOW_TOP);
    AddChild(tName);
    AddChild(sv);
    r.top+=p0.y+border;

    tFilter=new os::TextView(os::Rect(left, r.top, pageW-border, r.top+p0.y), "", "",
                             os::CF_FOLLOW_TOP | os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_LEFT);
    tFilter->SetMultiLine(false);
    sv=new os::StringView(os::Rect(border, r.top, left-border, r.top+p0.y),
                          "", "File name filter",
                          os::ALIGN_LEFT, os::CF_FOLLOW_LEFT | os::CF_FOLLOW_TOP);
    AddChild(tFilter);
    AddChild(sv);
    r.top+=p0.y+border;

    mFormater=new os::DropdownMenu(os::Rect(left, r.top, pageW-border, r.top+p0.y), "",
                                   os::CF_FOLLOW_TOP | os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_LEFT);
    //TODO: dynamically load these (plugins?)
    mFormater->AppendItem("Null formater");
    mFormater->AppendItem("C formater");
    mFormater->AppendItem("Java formater");
    mFormater->AppendItem("Perl formater");
    mFormater->AppendItem("HTML formater");
    mFormater->SetReadOnly(true);
    mFormater->SetSelectionMessage(new os::Message(ID_FORMATER_CHANGE));
    sv=new os::StringView(os::Rect(border, r.top, left-border, r.top+p0.y),
                          "", "Format using",
                          os::ALIGN_LEFT, os::CF_FOLLOW_LEFT | os::CF_FOLLOW_TOP);
    AddChild(mFormater);
    AddChild(sv);
    r.top+=p0.y+border;

    tTabsize=new os::TextView(os::Rect(left, r.top, pageW-border, r.top+p0.y), "", "",
                              os::CF_FOLLOW_TOP | os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_LEFT);
    tTabsize->SetMultiLine(false);
    tTabsize->SetNumeric(true);
    sv=new os::StringView(os::Rect(border, r.top, left-border, r.top+p0.y),
                          "", "Tab size",
                          os::ALIGN_LEFT, os::CF_FOLLOW_LEFT | os::CF_FOLLOW_TOP);
    AddChild(tTabsize);
    AddChild(sv);
    r.top+=p0.y+border;

    mUseTabs=new os::DropdownMenu(os::Rect(left, r.top, pageW-border, r.top+p0.y), "",
                                  os::CF_FOLLOW_TOP | os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_LEFT);
    mUseTabs->AppendItem("Tabs");
    mUseTabs->AppendItem("Spaces");
    mUseTabs->SetReadOnly(true);
    sv=new os::StringView(os::Rect(border, r.top, left-border, r.top+p0.y),
                          "", "Indent using",
                          os::ALIGN_LEFT, os::CF_FOLLOW_LEFT | os::CF_FOLLOW_TOP);
    AddChild(mUseTabs);
    AddChild(sv);
    r.top+=p0.y+border;

    sv=new os::StringView(os::Rect(border, r.top, left-border, r.top+p0.y),
                          "", "Colors",
                          os::ALIGN_LEFT, os::CF_FOLLOW_LEFT | os::CF_FOLLOW_TOP);
    AddChild(sv);
    r.top+=p0.y+border;


    lStyles=new os::ListView(os::Rect(border, r.top, (pageW-border)/2, r.bottom-border),
                             "", os::ListView::F_NO_AUTO_SORT | os::ListView::F_RENDER_BORDER |
                             os::ListView::F_NO_HEADER, os::CF_FOLLOW_LEFT | os::CF_FOLLOW_TOP | os::CF_FOLLOW_BOTTOM);
    lStyles->InsertColumn("Style", 100);
    lStyles->SetSelChangeMsg(new os::Message(ID_STYLE_CHANGE));
    AddChild(lStyles);

    r.left=(pageW+border)/2;
    left=(r.right+r.left)/2;

    sRed=new os::Spinner(os::Rect(left, r.top, pageW-border, r.top+p0.y), "", 0,
                         new os::Message(ID_COLOR_CHANGE),
                         os::CF_FOLLOW_TOP | os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_LEFT);
    sRed->SetMinMax(0, 255);
    sRed->SetStep(1);
    sRed->SetFormat("%0g");
    sv=new os::StringView(os::Rect(r.left, r.top, left-border, r.top+p0.y),
                          "", "Red", os::ALIGN_LEFT, os::CF_FOLLOW_LEFT | os::CF_FOLLOW_TOP);
    AddChild(sRed);
    AddChild(sv);
    r.top+=p0.y+border;

    sGreen=new os::Spinner(os::Rect(left, r.top, pageW-border, r.top+p0.y), "", 0,
                           new os::Message(ID_COLOR_CHANGE),
                           os::CF_FOLLOW_TOP | os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_LEFT);
    sGreen->SetMinMax(0, 255);
    sGreen->SetStep(1);
    sGreen->SetFormat("%0g");
    sv=new os::StringView(os::Rect(r.left, r.top, left-border, r.top+p0.y),
                          "", "Green", os::ALIGN_LEFT, os::CF_FOLLOW_LEFT | os::CF_FOLLOW_TOP);
    AddChild(sGreen);
    AddChild(sv);
    r.top+=p0.y+border;

    sBlue=new os::Spinner(os::Rect(left, r.top, pageW-border, r.top+p0.y), "", 0,
                          new os::Message(ID_COLOR_CHANGE),
                          os::CF_FOLLOW_TOP | os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_LEFT);
    sBlue->SetMinMax(0, 255);
    sBlue->SetStep(1);
    sBlue->SetFormat("%0g");
    sv=new os::StringView(os::Rect(r.left, r.top, left-border, r.top+p0.y),
                          "", "Blue", os::ALIGN_LEFT, os::CF_FOLLOW_LEFT | os::CF_FOLLOW_TOP);
    AddChild(sBlue);
    AddChild(sv);
    r.top+=p0.y+border;

    loadValues(0);
}

void SettingsTab_Formats::loadValues(int i)
{
    mFormat->Clear();
    for(uint a=0;a<formatList.size();++a)
        mFormat->AppendItem(formatList[a].name.c_str());
    mFormat->SetSelection(i);
    tName->Set(formatList[i].name.c_str());
    tFilter->Set(formatList[i].filter.c_str());
    mFormater->SetCurrentString(formatList[i].formatName.c_str());

    char tmp[20];
    sprintf(tmp, "%i", (int)formatList[i].tabSize);
    tTabsize->Set(tmp);

    mUseTabs->SetSelection(formatList[i].useTab?0:1);

    lStyles->Clear();


    ColorStringRow *r=new ColorStringRow();
    r->bgcolor=formatList[i].bgColor;
    r->text="Background";
    if(formatList[i].format)
        r->fgcolor=formatList[i].format->GetStyle(0).sColor;
    else
        r->fgcolor=formatList[i].fgColor;
    lStyles->InsertRow(r);

    if(formatList[i].format)
    {
        for(uint a=0;a<formatList[i].format->GetStyleCount();++a)
        {
            r=new ColorStringRow();
            r->text=formatList[i].format->GetStyleName(a);
            r->fgcolor=formatList[i].format->GetStyle(a).sColor;
            r->bgcolor=formatList[i].bgColor;
            lStyles->InsertRow(r);
        }
    }
    else
    {
        r=new ColorStringRow();
        r->text="Foreground";
        r->fgcolor=formatList[i].fgColor;
        r->bgcolor=formatList[i].bgColor;
        lStyles->InsertRow(r);
    }

    if(i==0)
    {
        bRemove->SetEnable(false);
        tName->SetEnable(false);
        tFilter->SetEnable(false);
    }
    else
    {
        bRemove->SetEnable(true);
        tName->SetEnable(true);
        tFilter->SetEnable(true);
    }
    lStyles->Select(0);
    os::Message m(ID_STYLE_CHANGE);
    HandleMessage(&m);
}

void SettingsTab_Formats::storeValues(int i)
{
    formatList[i].name=tName->GetBuffer()[0];
    formatList[i].filter=tFilter->GetBuffer()[0];
    formatList[i].formatName=mFormater->GetCurrentString();
    formatList[i].tabSize=atoi(tTabsize->GetBuffer()[0].c_str());
    formatList[i].useTab=(mUseTabs->GetSelection()==0);
}

void SettingsTab_Formats::apply()
{
    storeValues(iFormat);
    app->Lock();
    app->setFormatList(formatList);
    app->Unlock();
}

void SettingsTab_Formats::HandleMessage(os::Message *m)
{
    switch(m->GetCode())
    {
    case ID_ADD:
        storeValues(iFormat);
        formatList.push_back(FormatSet());
        iFormat=formatList.size()-1;
        loadValues(iFormat);
        break;
    case ID_REMOVE:
        formatList.erase(vector<FormatSet>::iterator(&formatList[iFormat]));
        iFormat=0;
        loadValues(0);
        break;
    case ID_FORMAT_CHANGE:
        {
            bool b=false;
            m->FindBool("final", &b);
            if(b)
            {
                int32 sel;
                if(m->FindInt32("selection", &sel)==0)
                {
                    storeValues(iFormat);
                    iFormat=sel;
                    loadValues(iFormat);
                }
            }
            break;
        }
    case ID_FORMATER_CHANGE:
        {
            storeValues(iFormat);
            if(formatList[iFormat].format)
                delete formatList[iFormat].format;

            if(formatList[iFormat].formatName=="C formater")
                formatList[iFormat].format=new Format_C();
            else if(formatList[iFormat].formatName=="Java formater")
                formatList[iFormat].format=new Format_java();
            else if(formatList[iFormat].formatName=="Perl formater")
                formatList[iFormat].format=new Format_Perl();
            else if(formatList[iFormat].formatName=="HTML formater")
                formatList[iFormat].format=new Format_HTML();
            else
                formatList[iFormat].format=NULL;

            loadValues(iFormat);
            break;
        }
    case ID_STYLE_CHANGE:
        {
            int sel=lStyles->GetFirstSelected();
            os::Color32_s col;

            if(sel==0)
            {
                col=formatList[iFormat].bgColor;
            }
            else
            {
                if(formatList[iFormat].format)
                {
                    col=formatList[iFormat].format->GetStyle((char)(sel-1)).sColor;
                }
                else
                {
                    col=formatList[iFormat].fgColor;
                }
            }

            sRed->SetValue(col.red);
            sGreen->SetValue(col.green);
            sBlue->SetValue(col.blue);
            break;
        }
    case ID_COLOR_CHANGE:
        {
            os::Color32_s c;

            c.red=sRed->GetValue().AsInt8();
            c.green=sGreen->GetValue().AsInt8();
            c.blue=sBlue->GetValue().AsInt8();

            int sel=lStyles->GetFirstSelected();

            if(sel==0)
            {//background
                formatList[iFormat].bgColor=c;

                for(uint a=0;a<lStyles->GetRowCount();++a)
                {
                    dynamic_cast<ColorStringRow*>(lStyles->GetRow(a))->bgcolor=c;
                    lStyles->InvalidateRow(a, 0);
                }
                Flush();
            }
            else
            {//not background
                if(formatList[iFormat].format)
                {
                    CodeViewStyle cs;
                    cs.sColor = c;
                    formatList[iFormat].format->SetStyle((char)(sel-1), cs);
                }
                else
                {
                    formatList[iFormat].fgColor=c;
                }
                if(sel==1)
                {//update bg row
                    dynamic_cast<ColorStringRow*>(lStyles->GetRow(0))->fgcolor=c;
                    lStyles->InvalidateRow(0, 0);
                }
                dynamic_cast<ColorStringRow*>(lStyles->GetRow(sel))->fgcolor=c;
                lStyles->InvalidateRow(sel, 0);
                lStyles->Flush();
            }
            break;
        }
    default:
        cout << "unhandled code: " << m->GetCode() << endl;
    }
}

void SettingsTab_Formats::AllAttached()
{
    mFormat->SetTarget(this);
    bAdd->SetTarget(this);
    bRemove->SetTarget(this);
    mFormater->SetTarget(this);
    lStyles->SetTarget(this);
    sRed->SetTarget(this);
    sGreen->SetTarget(this);
    sBlue->SetTarget(this);
}

