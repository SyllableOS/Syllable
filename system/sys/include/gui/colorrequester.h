#ifndef __F_GUI_COLORREQUESTER_H__
#define __F_GUI_COLORREQUESTER_H__

#include <gui/window.h>
#include <gui/fontrequester.h>
#include <util/messenger.h>

namespace os
{

/** Color Requester.
 * \ingroup gui
 * \par Description:
 * The Color Requester is the default way to pick colors in Syllable.
 * The Color Requester is very easy to use.  The following messages
 * are sent to the given message target:
 * M_COLOR_REQUESTER_CANCELED	-	If the ColorRequester is closed without any action.
 * M_COLOR_REQUESTED			-	If a Color has been selected to be loaded.
 * M_COLOR_REQUESTER_CHANGED	-	The use of this message will allow you to determine when the user has changed the color.  
 *									This can be used to show the color to the user without having them commit to any changes/
 *
 * \par Warning:
 * As with all libSyllable objects, don't use the ColorRequester without an application object. 
 *
 * \par Note:
 * The ColorRequester is sent an os::Messenger telling it where to send its
 * messages to.  If you send NULL to or use the default parameters, the
 * ColorRequester will send all messages to the instance of the application 
 * calling it.
 *
 * \par Note:
 * The ColorRequester is centered in the middle of the screen by default, you
   can override this at anytime after calling an instance of the dialog.
 *
 * \par Example:
 * \code
 *  ColorRequester* pcColorRequester = new ColorRequester(NULL);
 *  pcColorRequester->Show();
 *
 *  ...
 *
 *  MyApplication::HandleMessage(Message* pcMessage)
 *  {
 *		switch (pcMessage->GetCode())
 *     	{
 *			case M_COLOR_REQUESTED:
 *        	{
 *          	os::Color32_s sColor
 *            	pcMessage->FindColor("color",&sColor);  //get the color from the requester and then do anything you want with it
 *            	pcMyView->SetBgColor(sColor);
 *            	break;
 *        	}
 *         
 *			//this message is useful if you want to update a gui without hitting the okay button
 *			case M_COLOR_REQUESTER_CHANGED:
 *  		{
 *           	os::Color32_s sColor
 *            	pcMessage->FindColor("color",&sColor);  //get the color from the requester and then do anything you want with it
 *            	pcMyView->SetBgColor(sColor);
 *            	break;
 *			}
 *        	case M_COLOR_REQUESTER_CANCELED:
 *        	{
 *            	break; //do whatever you need to do when cancelled is called.
 *        	}
 *     	}
 *  }
 * \endcode
 *      
 * \sa os::Color32_s, os::Message, os::Application, os::Window, os::FileRequester, os::FontRequester, os::Messenger
 * \author	Rick Caudill <rick@syllable.org>
 *****************************************************************************/
class ColorRequester : public os::Window
{
public:	
	ColorRequester(os::Messenger* pcTarget=NULL);
	~ColorRequester();

	
protected:
	virtual void HandleMessage(os::Message*);
	bool OkToQuit();
		
private:
	ColorRequester();
	ColorRequester(const os::FontRequester&);
	void _Layout();
	
private:
	class Private;
	Private* m;
};
}

#endif
	



