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

#include <map>

#include <util/message.h>
#include <gui/window.h>
#include <gui/textview.h>
#include <gui/listview.h>
#include <gui/dropdownmenu.h>

#include "Hostname.h"
#include "Nameserver.h"
#include "Hosts.h"
#include "Interface.h"

using namespace os;

class NetworkPrefs
{
      public:
	NetworkPrefs();
	~NetworkPrefs();

	void ApplyChanges( void );
	void RevertChanges( void );

	  Hostname & GetHostname( void );
	  Nameserver & GetNameserver( void );
	  Hosts & GetHosts( void );
	const InterfaceList_t & GetInterfaces( void );
	bool IsRoot( void )
	{
		return m_bIsRoot;
	}

      private:
	  Hostname m_cHostname;
	Nameserver m_cNameserver;
	Hosts m_cHosts;
	InterfaceList_t m_cInterfaces;
	bool m_bIsRoot;

	void LoadInterfaces( void );
};

class IFaceWin;

typedef std::map <String, IFaceWin * >IFaceWinMap_t;

class MainWin:public Window
{
      public:
	MainWin();
	~MainWin();

	void HandleMessage( Message * pcMsg );
	bool OkToQuit( void );

      private:

	enum
	{
		ApplyChanges = 0x14000,
		RevertChanges,
		Close,

		AddNameserver,
		DeleteNameserver,

		AddSearchDomain,
		DeleteSearchDomain,

		AddHost,
		DeleteHost,

		InterfaceProperties
	};

	void Load( void );
	void LoadInterfaces( void );
	void Save( void );

	NetworkPrefs m_cPrefs;
	IFaceWinMap_t m_cWindows;

	TextView *m_pcHostname;
	TextView *m_pcDomainName;
	ListView *m_pcLstNs;
	ListView *m_pcLstSrc;
	ListView *m_pcLstHost;
	ListView *m_pcLstIFace;
	DropdownMenu *m_pcDdmDefaultIFace;
};
