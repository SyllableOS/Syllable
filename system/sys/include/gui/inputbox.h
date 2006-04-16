#ifndef __GUI_F_INPUT_BOX_H__
#define __GUI_F_INPUT_BOX_H__

#include <gui/textview.h>
#include <gui/stringview.h>
#include <gui/layoutview.h>
#include <gui/button.h>
#include <gui/window.h>
#include <util/string.h>
#include <util/variant.h>
#include <util/message.h>
#include <util/application.h>
#include <gui/guidefines.h>

namespace os
{
	/** Simple InputBox class.
 	* \ingroup gui
 	* \par Description:
 	*	An InputBox is a way to get user input from a user.
 	*	Use an InputBox when you need some data from a user(e.g., a date, a number, a string, and anything else).
 	*
 	*   There are two ways you can use InputBox:
 	*	
 	*	The first way is by catching the os::Message code(M_BOX_REQUESTED) like
 	*
 	*	YourHandler::HandleMessage(os::Message* pcMessage)
 	*	{	
 	*		switch (pcMessage->GetCode())
 	*		{
 	*			case M_BOX_REQUESTED:
 	*			{
 	*				os::Variant cVariant = inputBox->GetValue();
 	*				
 	*				//do something with it
 	*				
 	*				break;
 	*			}
 	*		}
 	*	}
 	*	
 	*	And the second way starts out like the first, except that when the message is passed to HandleMessage(),
 	*	it passes the value along with it. So You could do:
 	*	
	*	YourHandler::HandleMessage(os::Message* pcMessage)
 	*	{	
 	*		switch (pcMessage->GetCode())
 	*		{
 	*			case M_BOX_REQUESTED:
 	*			{
 	*				os::Variant cVariant;
 	*				pcMessage->GetVariant("value",&cVariant);
 	*				
 	*				//do something with it
 	*				
 	*				break;
 	*			}
 	*		}
 	*	}	
 	*	 	
 	* \sa os::Window, os::String, os::Messenger, os::Message, os::Variant, os::Handler
 	* \author	Rick Caudill(rick@syllable.org)
 	*****************************************************************************/
	
    class InputBox : public os::Window
    {
    public:
        InputBox(os::Messenger* pcTarget=NULL, const os::String& cTitle="InputBox",const os::String& cLabel="Please enter your value here:", const os::String& cText="",const os::String& cOkBut="_Okay", const os::String& cCancelBut="_Cancel",bool bNumeric=false,uint32 nFlags= os::WND_MODAL | os::WND_NOT_RESIZABLE | os::WND_NO_DEPTH_BUT | os::WND_NO_ZOOM_BUT);
        ~InputBox();

        os::Variant GetValue() const;
        
		void SetNumeric(bool);
		bool GetNumeric() const;
	
		void SetLabelText(const os::String&);
		os::String GetLabelText() const;

		void SetInputText(const os::String&);
		os::String GetInputText() const;

		void SetOkButtonText(const os::String&);
		os::String GetOkButtonText() const;

		void SetCancelButtonText(const os::String&);
		os::String GetCancelButtonText() const;
    
		void HandleMessage(os::Message* pcMessage);
		bool OkToQuit();
	
	private:
		    
		/**internal*/
        class Private;
        Private* m; 
    };
}


#endif




