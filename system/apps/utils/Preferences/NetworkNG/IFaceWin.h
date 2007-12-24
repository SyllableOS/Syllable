// Syllable Network Preferences - Copyright 2006 Andrew Kennan
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

#ifndef __IFACEWIN_H__
#define __IFACEWIN_H__

#include <gui/window.h>
#include <gui/dropdownmenu.h>
#include <gui/textview.h>
#include <gui/button.h>
#include <gui/checkbox.h>

#include "Interface.h"

using namespace os;

class IFaceWin:public Window
{
      public:
	IFaceWin( Window * pcParent, Interface * pcIFace, bool bReadOnly );
	 ~IFaceWin();

	virtual bool OkToQuit( void );
	virtual void HandleMessage( Message * pcMessage );

      private:
	enum
	{
		ChangeType = 0x12000,
		ChangeEnabled,
		Apply,
		Close
	};

	Interface *m_pcIFace;
	bool m_bReadOnly;
	Window *m_pcParent;

	DropdownMenu *m_pcDdmType;
	TextView *m_pcTxtAddress;
	TextView *m_pcTxtNetmask;
	TextView *m_pcTxtGateway;
	CheckBox *m_pcCbEnabled;

	void Load( void );
	void ChangeIFaceType( IFaceType_t nType );
	void ApplyChanges( void );
	void SendCloseMessage( void );
};

#endif
