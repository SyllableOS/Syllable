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
#include "Dialogs.h"
#include "messages.h"
#include <gui/requesters.h>
#include <gui/desktop.h>
#include <gui/font.h>
#include <gui/button.h>
#include <gui/radiobutton.h>
#include <gui/textview.h>
#include <gui/frameview.h>
#include <gui/layoutview.h>
#include <util/message.h>
#include <gui/checkbox.h>
#include <util/application.h>


DialogBase::DialogBase(const string& name, const string &title, int32 mode)
        : Window(os::Rect(), name, title, mode),
        KeyMap(),
        invoker(NULL)
{}

DialogBase::~DialogBase()
{
    if(invoker)
    {
        os::Message* m=invoker->GetMessage();
        if(!m)
        {
            dbprintf( "Error: Invoker registered with this Dialog does not have a message!\n" );
        }
        else
        {
            m->RemoveName("closed");
            m->AddBool("closed", true);
            invoker->Invoke();
        }
    }
    delete invoker;
}

void DialogBase::center(os::Window* win)
{
    os::Rect bounds=GetBounds();
    float w=bounds.right-bounds.left;
    float h=bounds.bottom-bounds.top;

    if(win)
    {
        os::Rect r=win->GetFrame();
        MoveTo(r.left+(r.right-r.left-w)/2, r.top+(r.bottom-r.top-h)/2);
    }
    else
    {
        os::Desktop desktop;
        MoveTo((desktop.GetResolution().x-w)/2, (desktop.GetResolution().y-h)/2);
    }
}

void DialogBase::DispatchMessage(os::Message* m, os::Handler *h)
{
    KEYMAP_DISPATCH(m);
    os::Window::DispatchMessage(m, h);
}

void DialogBase::Go(os::Invoker* i)
{
    invoker = i;
    Show();
    MakeFocus();
}
























MsgDialog::MsgDialog(const string& title, const string& text, int unused,  ...)
        : DialogBase("MsgDialog", title,
                     os::WND_NO_CLOSE_BUT | os::WND_NO_ZOOM_BUT | os::WND_NO_DEPTH_BUT |
                     os::WND_NOT_RESIZABLE | os::WND_FRONTMOST | os::WND_MODAL/* | os::WND_SYSTEM*/)
{
    va_list pArgs;

    va_start( pArgs, unused );

    view = new MsgView(text, pArgs);
    view->SetFgColor(0, 0, 0);
    view->SetBgColor(os::get_default_color(os::COL_NORMAL));
    os::Point size = view->GetPreferredSize(false);
    view->ResizeTo(size);
    ResizeTo(size);

    AddChild(view);
    va_end(pArgs);
}

void MsgDialog::HandleMessage(os::Message* message)
{
    if(message->GetCode() < (int)view->getButtonCount())
    {
        if(invoker)
        {
            os::Message* msg=invoker->GetMessage();
            if(!msg)
            {
                dbprintf( "Error: Invoker registered with this MsgDialog requester does not have a message!\n" );
            }
            else
            {
                msg->AddInt32("which", message->GetCode());
                invoker->Invoke();
            }
            PostMessage(os::M_QUIT);
        }
        else
            PostMessage(os::M_QUIT);
    }
    else
    {
        os::Window::HandleMessage(message);
    }
}













MsgView::MsgView(const string& text, va_list list)
        : os::View(os::Rect(0,0,1,1), "MsgView", os::CF_FOLLOW_ALL)
{
    const char* butName;

    uint i=0;
    while((butName = va_arg(list, const char*)) != NULL)
    {
        os::Button* button = new os::Button(os::Rect(0,0,1,1), butName, butName, new os::Message(i++));
        buttons.push_back(button);
        AddChild(button);

        os::Point size = button->GetPreferredSize(false);
        if(size.x > buttonSize.x)
        {
            buttonSize.x = size.x;
        }
        if(size.y > buttonSize.y)
        {
            buttonSize.y = size.y;
        }
    }

    float width = (buttonSize.x + 5) * buttons.size();

    int start = 0;
    for(i=0;i<=text.size();++i)
    {
        if(i==text.size() || text[i]=='\n')
        {
            std::string line(text, start, i-start);
            lines.push_back(line);

            float strLen = GetStringWidth(line.c_str())+20;
            if(strLen>width)
            {
                width=strLen;
            }
            start = i + 1;
        }
    }

    os::font_height fontHeight;
    GetFontHeight(&fontHeight);
    lineHeight = fontHeight.ascender + fontHeight.descender + fontHeight.line_gap;

    float height=lines.size()*lineHeight;
    height+=buttonSize.y + 30;

    float y=height - buttonSize.y - 5;
    float x=width - buttons.size() * (buttonSize.x + 5) - 5;

    for(i=0;i<buttons.size();++i)
    {
        buttons[i]->SetFrame(os::Rect(x, y, x+buttonSize.x-1, y+buttonSize.y-1));
        x+=buttonSize.x+5;
    }
    minSize.x=width;
    minSize.y=height;
}

void MsgView::AllAttached()
{
    buttons[0]->MakeFocus();
}

os::Point MsgView::GetPreferredSize(bool largest)
{
    return minSize;
}

uint MsgView::getButtonCount()
{
    return buttons.size();
}

void MsgView::Paint(const os::Rect& r)
{
    FillRect(r, GetBgColor());

    float y=20.0f;
    for(uint i=0;i<lines.size();++i)
    {
        MovePenTo(10,y);
        DrawString(lines[i].c_str());
        y+=lineHeight;
    }
}










FileReq::FileReq(os::FileRequester::file_req_mode_t p0,
                 os::Messenger* p1, const char* p2, uint32 p3,
                 bool p4, os::Message* p5, os::FileFilter* p6, bool p7,
                 bool p8, String p9, String p10)
        : os::FileRequester(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)
{
    addMapping("0\e", ID_CANCEL);
    addMapping("o\n", ID_OK);
		 m_pcTarget = ( p1 != NULL ) ? p1 : new Messenger( Application::GetInstance() );
}

bool FileReq::OkToQuit()
{
    PostMessage(ID_CANCEL, this);
    return false;
}

void FileReq::DispatchMessage(os::Message* m, os::Handler *h)
{
    KEYMAP_DISPATCH(m);
    os::FileRequester::DispatchMessage(m, h);
}

void FileReq::HandleMessage(os::Message *m)
{
    switch(m->GetCode())
    {
    case ID_ALERT: // User has ansvered the "Are you sure..." requester
        {
            int32 nButton;
            if(m->FindInt32("which", &nButton )!=0 || nButton!=0)
                break;
            /*** FALL THROUGH ***/
        }
    case ID_CANCEL:
        {
            os::Message *msg=new os::Message(os::M_FILE_REQUESTER_CANCELED);
            msg->AddPointer("source", this);
            m_pcTarget->SendMessage(msg);

            break;
        }
    }
    os::FileRequester::HandleMessage(m);
}













GotoLineDialog::GotoLineDialog() : DialogBase("GotoLineDialog", "Go to line")
{
    okButton=new os::Button(os::Rect(), "okButton", "GoTo", new os::Message(ID_OK));
    cancelButton=new os::Button(os::Rect(), "cancelButton", "Cancel", new os::Message(os::M_QUIT));

    text=new os::TextView(os::Rect(), "TextView", "");
    text->SetMultiLine(false);
    text->SetNumeric(true);
    text->SetMinPreferredSize(10, 1);

    os::Point p0=cancelButton->GetPreferredSize(false);
    os::Point p1=text->GetPreferredSize(false);

    const int border=5;

    float h=2*p0.y+3*border;
    float w=p1.x+p0.x+3*border;
    ResizeTo(w, h);
    okButton->SetFrame(os::Rect(p1.x+2*border, border, p1.x+p0.x+2*border, border+p0.y));
    cancelButton->SetFrame(os::Rect(p1.x+2*border, 2*border+p0.y, p1.x+p0.x+2*border, 2*border+2*p0.y));
    text->SetFrame(os::Rect(border, (h-p1.y)/2, border+p1.x, (h+p1.y)/2));

    AddChild(text);
    AddChild(okButton);
    AddChild(cancelButton);

    addMapping("0\n", ID_OK);
    addMapping("0\e", os::M_QUIT);
}

void GotoLineDialog::HandleMessage(os::Message* message)
{
    if(message->GetCode()==ID_OK)
    {
        if(invoker)
        {
            os::Message* msg=invoker->GetMessage();
            if(!msg)
            {
                dbprintf( "Error: Invoker registered with this GotoLineDialog does not have a message!\n" );
            }
            else
            {
                msg->AddInt32("which", atoi(text->GetBuffer()[0].c_str()));
                invoker->Invoke();
            }
            PostMessage(os::M_QUIT);
        }
    }
    else
    {
        os::Window::HandleMessage(message);
    }
}


void GotoLineDialog::Go(os::Invoker* i)
{
    DialogBase::Go(i);
    text->MakeFocus();
}















FindDialog::FindDialog(const string& str, bool down, bool caseS, bool extended)
        : DialogBase("FindDialog", "Find text")
{
    findButton=new os::Button(os::Rect(), "findButton", "Find", new os::Message(ID_FIND));
    closeButton=new os::Button(os::Rect(), "closeButton", "Close", new os::Message(os::M_QUIT));

    rUp=new os::RadioButton(os::Rect(), "rUp", "Up", NULL);
    rDown=new os::RadioButton(os::Rect(), "rDown", "Down", NULL);
    rCase=new os::RadioButton(os::Rect(), "rCase", "Sensitive", NULL);
    rNoCase=new os::RadioButton(os::Rect(), "rNoCase", "Insensitive", NULL);
    rBasic=new os::RadioButton(os::Rect(), "rBasic", "Basic", NULL);
    rExtended=new os::RadioButton(os::Rect(), "rExtended", "Extended", NULL);

    text=new os::TextView(os::Rect(), "TextView", "");
    text->SetMultiLine(false);
    text->SetMinPreferredSize(30, 1);

    os::FrameView *dirFrame=new os::FrameView(os::Rect(), "dirFrame", "Direction");
    os::FrameView *caseFrame=new os::FrameView(os::Rect(), "caseFrame", "Case");
    os::FrameView *syntaxFrame=new os::FrameView(os::Rect(), "syntaxFrame", "Syntax");

    os::LayoutView *rootView=new os::LayoutView(os::Rect(), "");

    os::HLayoutNode *root=new os::HLayoutNode("root");
    root->SetBorders(os::Rect(3, 3, 3, 0));
    os::VLayoutNode *l1=new os::VLayoutNode("l1", 1.0f, root);
    l1->SetBorders(os::Rect(3, 3, 3, 3));
    l1->AddChild(text);
    os::HLayoutNode *l2=new os::HLayoutNode("l2", 1.0f, l1);
    os::VLayoutNode *l2a=new os::VLayoutNode("l2a");
    l2a->SetHAlignment(os::ALIGN_LEFT);
    l2a->AddChild(rUp);
    l2a->AddChild(rDown);
    l2a->SetBorders(os::Rect(3, 3, 3, 3), "rUp", "rDown", NULL);
    dirFrame->SetRoot(l2a);
    l2->AddChild(dirFrame);
    os::VLayoutNode *l2b=new os::VLayoutNode("l2b");
    l2b->SetHAlignment(os::ALIGN_LEFT);
    l2b->AddChild(rCase);
    l2b->AddChild(rNoCase);
    l2b->SetBorders(os::Rect(3, 3, 3, 3), "rCase", "rNoCase", NULL);
    caseFrame->SetRoot(l2b);
    l2->AddChild(caseFrame);
    os::VLayoutNode *l2c=new os::VLayoutNode("l2c");
    l2c->SetHAlignment(os::ALIGN_LEFT);
    l2c->AddChild(rBasic);
    l2c->AddChild(rExtended);
    l2c->SetBorders(os::Rect(3, 3, 3, 3), "rBasic", "rExtended", NULL);
    syntaxFrame->SetRoot(l2c);
    l2->AddChild(syntaxFrame);
    os::VLayoutNode *l3=new os::VLayoutNode("l3", 1.0f, root);
    l3->SetBorders(os::Rect(3, 3, 3, 3));
    l3->AddChild(findButton);
    l3->AddChild(closeButton);
    new os::VLayoutSpacer("", 0, 1000, l3);
    l3->SameWidth("findButton", "closeButton", NULL);
    l3->SetBorders(os::Rect(0, 0, 0, 3), "findButton", NULL);
    root->SameWidth("TextView", "l2", NULL);
    root->SetBorders(os::Rect(0, 0, 0, 3), "TextView", NULL);

    rootView->SetRoot(root);
    root->Layout();

    os::Point p=rootView->GetPreferredSize(false);
    rootView->SetFrame(os::Rect(0, 0, p.x, p.y));
    ResizeTo(p.x, p.y);

    AddChild(rootView);

    //set keyboard shorcuts
    addMapping("0\n", ID_FIND);
    addMapping("0\e", os::M_QUIT);
    addMapping("2u", ID_UP);
    addMapping("2d", ID_DOWN);
    addMapping("2s", ID_CASE);
    addMapping("2i", ID_NOCASE);
    addMapping("2b", ID_BASIC);
    addMapping("2e", ID_EXTENDED);

    //set initial config
    text->Set(str.c_str());
    text->SelectAll();
    if(down)
        rDown->SetValue(true);
    else
        rUp->SetValue(true);
    if(caseS)
        rCase->SetValue(true);
    else
        rNoCase->SetValue(true);
    if(extended)
        rExtended->SetValue(true);
    else
        rBasic->SetValue(true);
}

void FindDialog::HandleMessage(os::Message* msg)
{
    switch(msg->GetCode())
    {
    case ID_FIND:
        if(invoker)
        {
            os::Message* m=invoker->GetMessage();
            if(!m)
            {
                dbprintf( "Error: Invoker registered with this FindDialog requester does not have a message!\n" );
            }
            else
            {
                m->RemoveName("text");
                m->AddString("text", text->GetBuffer()[0]);
                m->RemoveName("down");
                m->AddBool("down", rDown->GetValue().AsBool());
                m->RemoveName("case");
                m->AddBool("case", rCase->GetValue().AsBool());
                m->RemoveName("extended");
                m->AddBool("extended", rExtended->GetValue().AsBool());
                invoker->Invoke();
            }
        }
        break;
    case ID_UP:
        rUp->SetValue(true);
        break;
    case ID_DOWN:
        rDown->SetValue(true);
        break;
    case ID_CASE:
        rCase->SetValue(true);
        break;
    case ID_NOCASE:
        rNoCase->SetValue(true);
        break;
    case ID_BASIC:
        rBasic->SetValue(true);
        break;
    case ID_EXTENDED:
        rExtended->SetValue(true);
        break;
    default:
        os::Window::HandleMessage(msg);
    }
}


void FindDialog::Go(os::Invoker* i)
{
    DialogBase::Go(i);
    text->MakeFocus();
}

ReplaceDialog::ReplaceDialog(const Rect& cFrame, Window* pcParent) : Window(cFrame, "replace_dialog", "Replace", WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT | WND_NOT_RESIZABLE)
{
    pcParentWindow=pcParent;		// We need to know the parent window so we can send messages back to it

    // Create the Layoutviews
    pcMainLayoutView=new LayoutView(GetBounds(),"", NULL,CF_FOLLOW_NONE);

    // Make the base Vertical LayoutNode
    pcHLayoutNode= new HLayoutNode("main_layout_node");

    // Create the InputNode
    pcInputNode= new VLayoutNode("input_layout_node");

    pcInputNode->AddChild(new VLayoutSpacer("spacer", 5.0f, 5.0f, pcInputNode, 1.0f));

    pcFindLabel=new StringView(Rect(0,0,0,0),"find_label","Find",ALIGN_LEFT,CF_FOLLOW_NONE,WID_WILL_DRAW);
    pcInputNode->AddChild(pcFindLabel);

    pcFindTextView= new TextView(Rect(0,0,0,0), "find_text_view", NULL, CF_FOLLOW_NONE);
    pcInputNode->AddChild(pcFindTextView);

    pcInputNode->AddChild(new VLayoutSpacer("spacer", 5.0f, 5.0f, pcInputNode, 1.0f));

    pcReplaceLabel=new StringView(Rect(0,0,0,0),"replace_label","Replace with",ALIGN_LEFT,CF_FOLLOW_NONE,WID_WILL_DRAW);
    pcInputNode->AddChild(pcReplaceLabel);

    pcReplaceTextView= new TextView(Rect(0,0,0,0), "replace_text_view", NULL, CF_FOLLOW_NONE);
    pcInputNode->AddChild(pcReplaceTextView);

    pcInputNode->AddChild(new VLayoutSpacer("spacer", 5.0f, 5.0f, pcInputNode, 1.0f));

    pcCaseCheckBox=new os::CheckBox(Rect(0,0,0,0),"case_checkbox", "Case Sensitive Search", new Message(M_VOID), CF_FOLLOW_NONE, WID_WILL_DRAW);
    pcInputNode->AddChild(pcCaseCheckBox);

    pcInputNode->SameWidth("find_label","find_text_view","replace_label","replace_text_view","case_checkbox",NULL);

    // Add a spacer to force the edit box to the top
    pcInputNode->AddChild(new VLayoutSpacer("spacer", 1));

    // Add a spacer on the far left
    pcHLayoutNode->AddChild(new HLayoutSpacer("spacer", 5.0f, 5.0f, pcHLayoutNode, 1.0f));
    pcHLayoutNode->AddChild(pcInputNode);

    // Add a spacer between the Edit box & the Buttons
    pcHLayoutNode->AddChild(new HLayoutSpacer("spacer", 5.0f, 5.0f, pcHLayoutNode, 1.0f));

    // Create the ButtonNode
    pcButtonNode= new VLayoutNode("button_layout_node");

    pcFindButton=new Button(Rect(0,0,0,0), "find_button", "Find", new Message(M_BUT_FIND_GO), CF_FOLLOW_NONE);
    pcButtonNode->AddChild(pcFindButton);
    pcButtonNode->AddChild(new VLayoutSpacer("spacer", 5.0f, 5.0f, pcButtonNode, 1.0f ) );

    pcNextButton=new Button(Rect(0,0,0,0), "find_next_button", "Find Next", new Message(M_BUT_FIND_NEXT), CF_FOLLOW_NONE);
    pcButtonNode->AddChild(pcNextButton);
    pcButtonNode->AddChild(new VLayoutSpacer("spacer", 5.0f, 5.0f, pcButtonNode, 1.0f ) );

    pcReplaceButton=new Button(Rect(0,0,0,0), "replace_button", "Replace", new Message(M_BUT_REPLACE_DO), CF_FOLLOW_NONE);
    pcButtonNode->AddChild(pcReplaceButton);
    pcButtonNode->AddChild(new VLayoutSpacer("spacer", 5.0f, 5.0f, pcButtonNode, 1.0f ) );

    pcCloseButton=new Button(Rect(0,0,0,0), "close_button", "Close", new Message(M_BUT_REPLACE_CLOSE), CF_FOLLOW_NONE);
    pcButtonNode->AddChild(pcCloseButton);
    pcButtonNode->AddChild(new VLayoutSpacer("spacer", 5.0f, 5.0f, pcButtonNode, 1.0f ) );

    pcButtonNode->SameWidth("find_button","find_next_button","replace_button","close_button", NULL);

    pcHLayoutNode->AddChild(pcButtonNode);

    // Add a spacer on the far right
    pcHLayoutNode->AddChild(new HLayoutSpacer("spacer", 5.0f, 5.0f, pcHLayoutNode, 1.0f));

    // Add the LayoutNodes to the layoutview
    pcMainLayoutView->SetRoot(pcHLayoutNode);

    // Add the View to the Window
    AddChild(pcMainLayoutView);

    // Set the focus
    SetFocusChild(pcFindTextView);
    SetDefaultButton(pcFindButton);
}

void ReplaceDialog::HandleMessage(Message* pcMessage)
{
    switch(pcMessage->GetCode())
    {

    case M_BUT_FIND_GO:
        {
            pcMessage->AddString("find_text",(pcFindTextView->GetBuffer()[0]));
            pcMessage->AddBool("case_check_state",pcCaseCheckBox->GetValue());
            pcParentWindow->PostMessage(pcMessage,pcParentWindow);	// Send the messages from the dialog back to the parent window
            break;
        }

    case M_BUT_REPLACE_DO:
        {
            pcMessage->AddString("find_text",(pcFindTextView->GetBuffer()[0]));
            pcMessage->AddString("replace_text",(pcReplaceTextView->GetBuffer()[0]));

            pcParentWindow->PostMessage(pcMessage,pcParentWindow);	// Send the messages from the dialog back to the parent window
            break;
        }

    default:
        {
            pcParentWindow->PostMessage(pcMessage,pcParentWindow);	// Send the messages from the dialog back to the parent window
            break;
        }
    }

}

















