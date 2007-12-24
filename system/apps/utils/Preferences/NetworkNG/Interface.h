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

#ifndef __INTERFACE_H__
#define __INTERFACE_H__

#include <vector>
#include <util/string.h>

using namespace os;

// Defines the different types of available interfaces.
enum IFaceType_t
{ Static, DHCP };

typedef uint32 IPAddress_t;

bool ParseIPAddress( const String &cSrc, IPAddress_t * pnDest );

IPAddress_t ToIPAddress( uint8 n1, uint8 n2, uint8 n3, uint8 n4 );

String IPAddressToString( IPAddress_t nAddr );

// Represents a network interface.
class Interface
{
      public:
	Interface( IFaceType_t nType, const String &cName, const String &cMAC ):m_bIsDefault( false ), m_bEnabled( true ), m_nAddress( 0 ), m_nNetmask( 0 ), m_nGateway( 0 )
	{
		m_nType = nType;
		m_cName = cName;
		m_cMAC = cMAC;
	}
	 ~Interface()
	{
	};

	bool GetEnabled( void ) const
	{
		return m_bEnabled;
	}
	void SetEnabled( bool bEnabled )
	{
		m_bEnabled = bEnabled;
	}

	bool IsDefault( void ) const
	{
		return m_bIsDefault;
	}
	void SetDefault( bool bIsDefault )
	{
		m_bIsDefault = bIsDefault;
	}

	const String &GetName( void ) const
	{
		return m_cName;
	}

	const String &GetMAC( void ) const
	{
		return m_cMAC;
	}

	IFaceType_t GetType( void )const
	{
		return m_nType;
	}
	void SetType( IFaceType_t nType )
	{
		m_nType = nType;
	}

	IPAddress_t GetAddress( void ) const
	{
		return m_nAddress;
	}
	void SetAddress( IPAddress_t nAddress )
	{
		m_nAddress = nAddress;
	}

	IPAddress_t GetNetmask( void ) const
	{
		return m_nNetmask;
	}
	void SetNetmask( IPAddress_t nNetmask )
	{
		m_nNetmask = nNetmask;
	}

	IPAddress_t GetGateway( void ) const
	{
		return m_nGateway;
	}
	void SetGateway( IPAddress_t nGateway )
	{
		m_nGateway = nGateway;
	}

      private:
	bool m_bIsDefault;
	bool m_bEnabled;
	IFaceType_t m_nType;
	String m_cName;
	String m_cMAC;
	IPAddress_t m_nAddress;
	IPAddress_t m_nNetmask;
	IPAddress_t m_nGateway;
};

typedef std::vector < Interface * >InterfaceList_t;

class InterfaceManager
{
      public:
	void DiscoverInterfaces( InterfaceList_t & cInterfaces );
	void LoadConfig( InterfaceList_t & cInterfaces );
	void SaveConfig( InterfaceList_t & cInterfaces );
	void WriteNetworkInit( InterfaceList_t & cInterfaces );
	void InvokeNetworkInit( void );
	void ClearInterfaceList( InterfaceList_t & cInterfaces );
	bool InterfacesHaveChanged( const InterfaceList_t & cConfigIF, const InterfaceList_t & cDetectIF );
};

#endif
