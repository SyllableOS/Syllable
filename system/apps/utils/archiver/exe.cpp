//  Archiver 0.3.0 -:-  (C)opyright 2000-2001 Rick Caudill
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#include <gui/window.h>
#include <gui/view.h>
#include <stdio.h>
#include <util/message.h>
#include <gui/requesters.h>
#include <gui/desktop.h>
#include <gui/button.h>
#include <gui/textview.h>
#include <gui/stringview.h>
#include <fstream.h>
#include <string.h>
#include <unistd.h>
#define ID_BUT_EXE 1
#define ID_BUT_CAN 2
using namespace os;

class Eview : public View

{

public:

    Eview(const Rect & cFrame);

};

Eview::Eview(const Rect &cFrame):View(cFrame,"ExeView")
{

}

class ExeWindow : public Window
{
public:
    ExeWindow();
    Eview* m_pcExeView;
    Button* m_pcMakeBut;
    Button* m_pcCancelBut;
    virtual void HandleMessage(Message* pcMessage);
    virtual bool OkToQuit();

private:
    TextView* m_pcFileView;
    StringView* m_pcFileSView;
    TextView* m_pcNameView;
    StringView* m_pcNameSView;
    TextView* m_pcLabelView;
    StringView* m_pcLabelSView;
};

ExeWindow::ExeWindow() : Window(Rect(0,0,160,170),"EXE","Make Self-Extracting File",WND_NOT_RESIZABLE)
{


    m_pcExeView = new Eview(GetBounds());

    Desktop dDesktop;
    MoveTo(dDesktop.GetResolution().x / 2 - GetBounds().Width() / 2, dDesktop.GetResolution().y / 2 - GetBounds().Height() /2 ); //moves the main window to the center of the desktop
    m_pcMakeBut = new Button(Rect(0,0,0,0),"exe_but","Make File",new Message(ID_BUT_EXE));
    m_pcCancelBut = new Button(Rect(0,0,0,0),"can_but","Cancel",new Message(ID_BUT_CAN));
    m_pcFileView = new TextView(Rect(0,0,0,0),"file_view","",CF_FOLLOW_NONE);

    m_pcFileSView = new StringView(Rect(0,0,0,0),"file_s_view","Archive's Name:");
    m_pcNameView = new TextView(Rect(0,0,0,0),"name_view","",CF_FOLLOW_NONE);
    m_pcNameSView = new StringView(Rect(0,0,0,0),"name_s_view","Directory to Archive:");

    m_pcLabelView = new TextView(Rect(0,0,0,0),"label_view","",CF_FOLLOW_NONE);
    m_pcLabelSView = new StringView(Rect(0,0,0,0),"label_s_view","Label When Extracting:");
    Point cButSize = m_pcMakeBut->GetPreferredSize(false);

    Rect cViewFrame(0,0,150,20);
    Rect cButFrame(0,0,50,50);
    Rect cViewSFrame(0,0,110,12);
    cButFrame.right = cButSize.x ;
    cButFrame.bottom = cButSize.y ;

    m_pcLabelSView->SetFrame(cViewSFrame + Point(2,94));
    m_pcLabelView->SetFrame(cViewFrame + Point(2,107));

    m_pcNameSView->SetFrame(cViewSFrame + Point(2,50));
    m_pcNameView->SetFrame(cViewFrame + Point(2,63));

    m_pcFileSView->SetFrame(cViewSFrame + Point(2,6));
    m_pcFileView->SetFrame(cViewFrame + Point(2,19));

    m_pcCancelBut->SetFrame(cButFrame + Point(91,145));
    m_pcMakeBut->SetFrame(cButFrame + Point(5,145));

    m_pcFileView->SetTabOrder(0);
    m_pcNameView->SetTabOrder(1);
    m_pcLabelView->SetTabOrder(2);
    m_pcMakeBut->SetTabOrder(3);
    m_pcCancelBut->SetTabOrder(4);

    AddChild(m_pcNameView);
    AddChild(m_pcNameSView);
    AddChild(m_pcMakeBut);
    AddChild(m_pcCancelBut);
    AddChild(m_pcFileView);
    AddChild(m_pcFileSView);
    AddChild(m_pcLabelSView);
    AddChild(m_pcLabelView);

    SetFocusChild(m_pcFileView); //
    SetDefaultButton(m_pcMakeBut);
}

void ExeWindow::HandleMessage(Message* pcMessage)
{
    switch(pcMessage->GetCode())
    {
    case ID_BUT_CAN:
        {

            OkToQuit();
        }

        break;

    case ID_BUT_EXE:
        {
            String zfBuf = m_pcFileView->GetBuffer() [0];
            String zlBuf = m_pcLabelView->GetBuffer() [0];
            String znBuf = m_pcNameView->GetBuffer() [0];
            int anPipe[2];
            pipe (anPipe);
            pid_t hPid = fork();
            if (hPid==0)

            {

                close(0);
                close(1);
                close(2);
                dup2(anPipe[2],1);
                dup2(anPipe[2],2);
                close (anPipe[0] );
                execlp("makeself.sh","makeself.sh","--notemp",znBuf.c_str(),zfBuf.c_str(),zlBuf.c_str(),NULL);
                exit (0);

            }

            else

            {

                close (anPipe[2]);

            }

            OkToQuit();

        }

        break;

    default:

        Window::HandleMessage(pcMessage);

        break;
    }
}

bool ExeWindow::OkToQuit(void)
{

    PostMessage(M_QUIT );
    return (true);
}
