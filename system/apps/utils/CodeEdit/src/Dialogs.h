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
#ifndef _DIALOGS_H_
#define _DIALOGS_H_

#include <stdarg.h>

#include <vector>
#include <string>

#include <gui/window.h>
#include <gui/view.h>
#include <util/invoker.h>
#include <util/message.h>
#include "messages.h"
//You didn't see this...
#define private protected
#include <gui/filerequester.h>
#undef private

namespace os
{
class Button;
class RadioButton;
class TextView;
class Incoker;
class Message;
class HLayoutNode;
class VLayoutNode;
class CheckBox;
class StringView;
class LayoutView;
};

#include "KeyMap.h"

class DialogBase : public os::Window, public KeyMap
{
public:
    DialogBase(const string& ="", const string& ="",
               int32 =   /*os::WND_NO_CLOSE_BUT |*/ os::WND_NO_ZOOM_BUT
                                                    | os::WND_NO_DEPTH_BUT | os::WND_NOT_RESIZABLE
                                                    | os::WND_FRONTMOST | os::WND_MODAL/* | os::WND_SYSTEM*/);
    virtual ~DialogBase();

    void center(os::Window* =NULL);

    void DispatchMessage(os::Message* message, os::Handler *h);

    virtual void Go(os::Invoker* invoker=NULL);
protected:
    os::Invoker *invoker;
};



class MsgView : public os::View
{
public:
    MsgView(const string& text, va_list buttons);

    virtual os::Point GetPreferredSize(bool largest);
    virtual void Paint(const os::Rect& updateRect);
    virtual void AllAttached();

    uint getButtonCount();
private:
    vector<std::string> lines;
    vector<os::Button*> buttons;
    os::Point buttonSize;
    float lineHeight;
    os::Point minSize;
};



class MsgDialog : public DialogBase
{
public:
    MsgDialog(const string& title, const string& text, int unused, ... );

    virtual void HandleMessage(os::Message* message);

private:
    MsgView* view;
};


class FileReq : public os::FileRequester, protected KeyMap
{
public:
    FileReq(os::FileRequester::file_req_mode_t =os::FileRequester::LOAD_REQ,
            os::Messenger* =NULL, const char* =NULL, uint32 =os::FileRequester::NODE_FILE,
            bool =true, os::Message* =NULL, os::FileFilter* =NULL, bool =false,
            bool =true, String ="", String ="");

    bool OkToQuit();
    void DispatchMessage(os::Message*, os::Handler*);
    void HandleMessage(os::Message*);

private:
		 Messenger *m_pcTarget;
};



class GotoLineDialog : public DialogBase
{
public:
    GotoLineDialog();

    virtual void HandleMessage(os::Message* message);

    virtual void Go(os::Invoker* invoker);
private:
    os::Button *okButton, *cancelButton;
    os::TextView *text;
};



class FindDialog : public DialogBase
{
public:
    FindDialog(const string& ="", bool=true, bool=true, bool=false);

    virtual void HandleMessage(os::Message* message);

    void Go(os::Invoker* invoker);

private:

    os::Button *findButton, *closeButton;
    os::RadioButton *rUp, *rDown, *rCase, *rNoCase, *rBasic, *rExtended;
    os::TextView *text;
};

class ReplaceDialog : public os::Window
{
public:
    ReplaceDialog(const os::Rect& cFrame, os::Window* pcParent);
    void HandleMessage(os::Message* pcMessage);

private:

    os::Window* pcParentWindow;

    os::LayoutView* pcMainLayoutView;

    os::HLayoutNode* pcHLayoutNode;
    os::VLayoutNode* pcButtonNode;
    os::VLayoutNode* pcInputNode;

    os::Button* pcFindButton;
    os::Button* pcNextButton;
    os::Button* pcReplaceButton;
    os::Button* pcCloseButton;

    os::TextView* pcFindTextView;
    os::TextView* pcReplaceTextView;

    os::CheckBox* pcCaseCheckBox;

    os::StringView* pcFindLabel;
    os::StringView* pcReplaceLabel;
};

#endif














