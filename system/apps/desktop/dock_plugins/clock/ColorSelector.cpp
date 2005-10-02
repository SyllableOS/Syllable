#include "ColorSelector.h"
#include "messages.h"

/*************************************************************
* Description: Simple window that contains a colorselector and two buttons
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 21:47:35 
*************************************************************/
ColorSelectorWindow::ColorSelectorWindow(int nType,Color32_s sColor,Window* pcWindow) : Window(Rect(0,0,250,200),"color_window","Choose a color...")
{
	pcParentWindow = pcWindow;
	nControl = nType;
	sControlColor = sColor;
	
	pcLayoutView = new LayoutView(GetBounds(),"color_selector_layout");
	
	Layout();
	SetDefaultButton(pcOkButton);
}

/*************************************************************
* Description: Lays out the colorselector window
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 21:49:32 
*************************************************************/
void ColorSelectorWindow::Layout()
{
	HLayoutNode* pcButtonNode = new HLayoutNode("colors_button_node");
	pcButtonNode->AddChild(pcOkButton = new Button(Rect(),"colorselector_ok_but","_Okay",new Message(M_COLOR_SELECTOR_WINDOW_OK)));
	pcButtonNode->AddChild(new HLayoutSpacer("",5,5));
	pcButtonNode->AddChild(pcCancelButton = new Button(Rect(),"colorselector_cancel_but","_Cancel",new Message(M_COLOR_SELECTOR_WINDOW_CANCEL)));
	pcButtonNode->SameWidth("colorselector_ok_but","colorselector_cancel_but",NULL);	
	pcButtonNode->LimitMaxSize(pcButtonNode->GetPreferredSize(false));
	
	VLayoutNode* pcNode = new VLayoutNode("col_node");
	pcNode->AddChild(pcSelector = new ColorSelector(GetBounds(),"col_select",NULL));
	pcSelector->SetValue(sControlColor);
	pcNode->AddChild(new VLayoutSpacer("",5,5));
	pcNode->AddChild(pcButtonNode);

	pcLayoutView->SetRoot(pcNode);
	AddChild(pcLayoutView);
}

/*************************************************************
* Description: Simple function that gets called when the selector wants to
*			   close.
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 21:49:53 
*************************************************************/
bool ColorSelectorWindow::OkToQuit()
{
	pcParentWindow->PostMessage(M_COLOR_SELECTOR_WINDOW_CANCEL,pcParentWindow);
	return true;
}

/*************************************************************
* Description: HandleMessage() Handles all the messages passed
*			   though the window.
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 21:51:32 
*************************************************************/
void ColorSelectorWindow::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_COLOR_SELECTOR_WINDOW_OK:
		{
			Message cMsg(M_COLOR_SELECTOR_WINDOW_REQUESTED);
			cMsg.AddInt32("control",nControl);
			cMsg.AddColor32("color",pcSelector->GetValue().AsColor32());
			pcParentWindow->PostMessage(&cMsg,pcParentWindow);			
			break;
		}
		
		case M_COLOR_SELECTOR_WINDOW_CANCEL:
		{
			OkToQuit();
			break;
		}
	}
}
