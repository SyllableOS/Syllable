#include "iconmenu_prop.h"
#include "iconmenu_messages.h"


/*
** name:       IconProp Constructor
** purpose:    Icon Properties Window
** parameters: String(contains the icon name)
** returns:   
*/
IconProp::IconProp(string cIconName, string cExec, Bitmap* cIconPic) : Window(CRect(250,100), "Icon_Properties", "Icon Properties", WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT | WND_NOT_RESIZABLE)
{
   	
   	/* For The Icon Name. */
	 pcIconNameString = new StringView(Rect(0,0,0,0),"Icon_Name", "Icon Name:");
    pcIconNameString->SetFrame(Rect(0,0,70,20) + Point(10,10));
    AddChild(pcIconNameString);

    pcIconNameText = new TextView(Rect(0,0,0,0),"Icon_NAME_TEXT",cIconName.c_str());
    pcIconNameText->SetFrame(Rect(0,0,110,20) + Point(70,10));
    AddChild(pcIconNameText);
    
    pcIconNameText->SetEnable(false);

    pcIconRenameBut = new Button(Rect(0,0,0,0),"Icon_Rename","Rename",new Message(ID_ICON_PROP_RENAME));
    pcIconRenameBut->SetFrame(Rect(0,0,50,20) + Point(190,10));
    AddChild(pcIconRenameBut);

	/* For the executable that is launched when the icon is clicked. */
	pcIconExecBut = new Button(Rect(0,0,0,0),"IC_EXEC","Change",new Message(ID_ICON_PROP_CHANGE));
	pcIconExecBut->SetFrame(Rect(0,0,50,20) + Point(190, 40));
	AddChild(pcIconExecBut);
	
	pcIconExecTxt = new TextView(Rect(0,0,0,0),"Icon_Exec_Txt", cExec.c_str());
	pcIconExecTxt->SetFrame(Rect(0,0,110,20) + Point(70, 40));
	AddChild(pcIconExecTxt);

	pcIconExecTxt->SetEnable(false);
	
	pcIconExecStr = new StringView(Rect(0,0,0,0),"ICON_EXEC_STR","Execute:");
	pcIconExecStr->SetFrame(Rect(0,0,50,20) + Point(10,40));
	AddChild(pcIconExecStr);
	
    pcIconOkBut = new Button(Rect(0,0,0,0),"Icon_OK","OK",new Message(ID_ICON_PROP_OK) );
    pcIconOkBut->SetFrame(Rect(0,0,40,25) + Point (GetBounds().Width() /2 - 25, GetBounds().Height() - 30) );
    AddChild(pcIconOkBut);
    
}


/*
** name:       IconProp::HandleMessage
** purpose:    Handles the messages between the IconProp Window
** parameters: Message(contains the message that is switched between the window and 
**			   gui components).
** returns:   
*/
void IconProp::HandleMessage(Message* pcMessage)
{
    switch (pcMessage->GetCode())
    {
    	
    case ID_ICON_PROP_RENAME:
        if(pcIconNameText->IsEnabled() == true)
            pcIconNameText->SetEnable(false);

        else
            pcIconNameText->SetEnable(true);

        break;


	case ID_ICON_PROP_CHANGE:
		if (pcIconExecTxt->IsEnabled() == true)
			pcIconExecTxt->SetEnable(false);
			
		else
			pcIconExecTxt->SetEnable(true);
			
		break;
		
    case ID_ICON_PROP_OK:
        PostMessage(M_QUIT);
        break;

    default:
        Window::HandleMessage(pcMessage);
        break;
    }
}















