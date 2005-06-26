/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 2005 Syllable Team
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */

#ifndef __F_GUI_FONT_REQUESTER_H__
#define __F_GUI_FONT_REQUESTER_H__

#include <gui/font.h>
#include <gui/layoutview.h>
#include <gui/button.h>
#include <gui/window.h>

#include <util/messenger.h>
#include <util/message.h>

class FontRequesterView;

namespace os
{

/** Font Requester.
 * \ingroup gui
 * \par Description:
 * The FontRequester is the default way to pick fonts in Syllable.
 * The FontRequester is very easy to use.  The following messages
 * are sent to the given message target:
 * M_FONT_REQUESTER_CANCELED - If the FontRequester is closed without any action.
 * M_FONT_REQUESTED - If a Font has been selected to be loaded.
 * 
 * \par Note:
 * The FontRequester is sent an os::Messenger telling it where to send its
 * messages to.  If you send NULL to or use the default parameters, the
 * FontRequester will send all messages to the instance of the application 
 * calling it.
 *
 * \par Note:
 * The FontRequester is centered in the middle of the screen by default, you
   can override this at anytime after calling an instance of the dialog.
 *
 * \par Example:
 * \code
 *  FontRequester* pcFontRequester = new FontRequester(NULL,true); //enable shear/rotation
 *  pcFontRequester->Show();
 *
 *  ...
 *
 *  MyApplication::HandleMessage(Message* pcMessage)
 *  {
 *     switch (pcMessage->GetCode())
 *     {
 *        case M_FONT_REQUESTED:
 *        {
 *            os::Font* pcFont = new Font();
 *            pcMessage->FindObject("font",*pcFont);  //get the font from the requester and then do anything you want with it
 *            pcMyView->SetFont( pcFont );
 *            pcFont->Release();
 *            break;
 *        }
          
 *        case M_FONT_REQUESTER_CANCELED:
 *        {
 *            break; //do whatever you need to do when cancelled is called.
 *        }
 *     }
 *  }
 * \endcode
 *      
 * \sa os::Font, os::Message, os::Application, os::Window, os::FileRequester
 * \author	Rick Caudill(syllable_desktop@hotpop.com)
 *****************************************************************************/
class FontRequester : public Window
{
public:

		/** Constructor.
 		 * \par Description:
 		 * FontRequester constructor. 
 		 * \param pcTarget - The target where all messages will be sent.
 		 * \param bEnableAdvanced - Determines whether or not Shear/Rotation will be enabled.
 		 *
 		 * \author	Rick Caudill (syllable_desktop@hotpop.com)
 		 *****************************************************************************/
		FontRequester(Messenger* pcTarget=NULL, bool bEnableAdvanced=false);
		virtual ~FontRequester();

		void HandleMessage(Message* pcMessage);
		virtual bool OkToQuit(void);

private:
		/*disable the default constructor and copy constructor*/
		FontRequester();
		FontRequester(const FontRequester&);
       	
		/*private class contains everything else*/
		class Private;
		Private *m;
        
		FontRequesterView* pcRequesterView;
		Button* pcOkButton;
		Button* pcCancelButton;
};

}
#endif  //__F_GUI_FONT_REQUESTER_H__

