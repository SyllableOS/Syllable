#include <gui/window.h>
#include <gui/button.h>
#include <gui/desktop.h>
#include <gui/textview.h>
#include <gui/checkbox.h>
#include <gui/view.h>
#include <gui/frameview.h>
#include <gui/stringview.h>
#include <util/message.h>
#include <fstream.h>
#include <storage/filereference.h>
#include <stdio.h>
#include <unistd.h>


using namespace os;



class OptionsPathView : public FrameView

{
public:
    OptionsPathView();


};


OptionsPathView::OptionsPathView(): FrameView(Rect(0,0,285,100),"opt_view1","Default Paths")
{

}

class OptionsSettingsView : public FrameView
{

public:

    OptionsSettingsView();

private:


};


OptionsSettingsView::OptionsSettingsView() : FrameView(Rect(0,0,235,75),"opt_set_view","Misc Settings")
{


}

class OptionsGeneralView : public View

{
public:
    OptionsGeneralView(Rect& csBounds);
private:

    OptionsPathView* m_pcOpt1View;
    OptionsSettingsView* m_pcSet2View;
};

OptionsGeneralView::OptionsGeneralView(Rect & csBounds): View(csBounds,"opt_view2",CF_FOLLOW_ALL)
{

    m_pcOpt1View = new OptionsPathView();
    m_pcSet2View = new OptionsSettingsView();
    m_pcOpt1View->SetFrame(Rect(0,0,285,100) + Point(5,5));
    m_pcSet2View->SetFrame(Rect(0,0,285,75) + Point(5, 110));
    AddChild(m_pcOpt1View);
    AddChild(m_pcSet2View);
}




class OptionsWindow : public Window
{
public:
    OptionsWindow(Rect & r, string zOpenPath, string zExtractPath, bool bCloseNew, bool bCloseExtract);
private:
    Desktop sDesktop;
    Rect csBounds;
    void HandleMessage(Message* pcMessage);
    OptionsGeneralView* m_pcOptGenView;
    Button* m_pcCancelButton;
    Button* m_pcSaveButton;
    bool Write();
    char zConfigFile[128];
    String cExtractPath;
    String cOpenPath;
    bool bCloseN, bCloseExt;
    bool Read();
    void Tab();

    //Extract Path Option Controls
    Button* m_pcExtractToBut;
    StringView* m_pcExtractToView;
    TextView* m_pcExtractToTextView;

    //Open Path Option Controls
    Button* m_pcOpenToBut;
    StringView* m_pcOpenToView;
    TextView* m_pcOpenToTextView;
    FileRequester* m_pcPathRequester;

    CheckBox* m_pcExtractClose;
    CheckBox* m_pcRecentOption;

    const char* pzOptPath;

};

OptionsWindow::OptionsWindow(Rect & r,string zOpenPath, string zExtractPath, bool bCloseNew, bool bCloseExtract):Window(r,"opt_window","Archiver Settings",WND_NOT_RESIZABLE)
{

    csBounds = GetBounds();
    
    cOpenPath = zOpenPath;
    cExtractPath = zExtractPath;
    bCloseN = bCloseNew;
    bCloseExt = bCloseExtract;
    
    m_pcOptGenView = new OptionsGeneralView(csBounds);
    AddChild(m_pcOptGenView);

    m_pcCancelButton = new Button(Rect(0,0,0,0),"Cancel","Cancel", new Message(ID_OPT_QUIT));
    m_pcCancelButton->SetFrame(Rect(0,0,60,20) + Point(60,195));
    AddChild(m_pcCancelButton);

    m_pcSaveButton = new Button(Rect(0,0,0,0),"Save","Save", new Message(ID_OPT_WRITE));
    m_pcSaveButton->SetFrame(Rect(0,0,60,20) + Point(160,195));
    AddChild(m_pcSaveButton);

    MoveTo(sDesktop.GetResolution().x /2 - GetBounds().Width() /2, sDesktop.GetResolution().y /2 - GetBounds().Height() /2);
    sprintf( zConfigFile, "%s/config/Archiver/Archiver.cfg", getenv("HOME") );

    m_pcOpenToView = new StringView(Rect(0,0,0,0),"Open_To:","Open Path:");
    m_pcOpenToBut = new Button(Rect(0,0,0,0),"Select_But","Select",new Message(ID_OPT_SELECT_1));
    m_pcOpenToTextView = new TextView(Rect(0,0,0,0),"Open_Text","");
    m_pcOpenToView->SetFrame(Rect(0,0,60,20) + Point(15,20));
    m_pcOpenToTextView->SetFrame(Rect(0,0,130,20)+Point(80,20));
    m_pcOpenToBut->SetFrame(Rect(0,0,60,20) + Point(215,20));
    AddChild(m_pcOpenToView);
    AddChild(m_pcOpenToTextView);
    AddChild(m_pcOpenToBut);

    m_pcExtractToBut = new Button(Rect(0,0,60,15),"Select","Select",new Message(ID_OPT_SELECT_2));
    m_pcExtractToView = new StringView(Rect(0,0,60,20),"ExtractToView","Extract Path:");
    m_pcExtractToTextView = new TextView(Rect(0,0,0,0),"","");
    
    
    m_pcExtractToTextView->SetFrame(Rect(0,0,130,20)+ Point(80,60));
    m_pcExtractToBut->SetFrame(Rect(0,0,60,20) + Point(215,60));
    m_pcExtractToView->SetFrame(Rect(0,0,60,20) + Point(15,60));
    
    AddChild(m_pcExtractToView);
    AddChild(m_pcExtractToTextView);
    AddChild(m_pcExtractToBut);

    m_pcExtractClose = new CheckBox(Rect(0,0,0,0),"close_box","Close Extract Window after extracting",NULL);
    m_pcExtractClose->SetFrame(Rect(0,0,210,20) + Point(15,120));
    AddChild(m_pcExtractClose);

    m_pcRecentOption = new CheckBox(Rect(0,0,0,0),"recent_box","Close the new window after creating a file",NULL);
    m_pcRecentOption->SetFrame(Rect(0,0,230,20) + Point(15,150));
    AddChild(m_pcRecentOption);

    //Function that sets tab orders
    Tab();
    
    m_pcRecentOption->SetValue(bCloseNew);
    m_pcExtractClose->SetValue(bCloseExt);
    m_pcExtractToTextView->Set(cExtractPath.c_str());
    m_pcOpenToTextView->Set(cOpenPath.c_str());
}

void OptionsWindow::Tab()
{
    m_pcSaveButton->SetTabOrder(6);
    m_pcCancelButton->SetTabOrder(5);
    m_pcExtractToBut->SetTabOrder(4);
    m_pcExtractToTextView->SetTabOrder(3);
    m_pcOpenToBut->SetTabOrder(2);
    m_pcOpenToTextView->SetTabOrder(1);
    OptionsWindow::SetDefaultButton(m_pcSaveButton);
    OptionsWindow:SetFocusChild(m_pcOpenToTextView);
}


void OptionsWindow::HandleMessage(Message* pcMessage)
{
    switch(pcMessage->GetCode()){
    
    case ID_OPT_QUIT:
        {
            PostMessage(M_QUIT);
        }
        break;


    case ID_OPT_WRITE:
        {
            Write();

        }

        break;


    case ID_OPT_SELECT_1:
        {
            Alert* error = new Alert("Not Implemented","The Select Option has not \nbeen implemented yet",0x00,"Sorry",NULL);
            error->Go(new Invoker());

        }
        break;


    case ID_OPT_SELECT_2:
        {
            Alert* error = new Alert("Not Implemented","The Select Option has not \nbeen implemented yet",0x00,"Sorry",NULL);
            error->Go(new Invoker());

        }
        break;


    default:
        Window::HandleMessage(pcMessage);
    }
}


bool OptionsWindow::Write()
{
	string zOpen = (string) m_pcOpenToTextView->GetBuffer()[0];
	string zExtract = (string) m_pcExtractToTextView->GetBuffer()[0];
	bool bCloseN =  m_pcRecentOption->GetValue();
	bool bCloseE = m_pcExtractClose->GetValue();
	
	Message *pcPrefs = new Message( );
    pcPrefs->AddString ( "Open", zOpen);
    pcPrefs->AddString ( "Extract", zExtract);
    pcPrefs->AddBool   ( "Close_Extract",bCloseE   );
    pcPrefs->AddBool   ( "Close_New",  bCloseN);


    File *pcConfig = new File("/tmp/Archiver.cfg", O_WRONLY | O_CREAT );
    if( pcConfig )
    {
        uint32 nSize = pcPrefs->GetFlattenedSize( );
        void *pBuffer = malloc( nSize );
        pcPrefs->Flatten( (uint8 *)pBuffer, nSize );

        pcConfig->Write( pBuffer, nSize );

        delete pcConfig;
        free( pBuffer );
    }

    system("mv -f /tmp/Archiver.cfg ~/config/Archiver");

}


