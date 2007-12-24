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

#include "Interface.h"
#include "StringReader.h"
#include "StringWriter.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/nettypes.h>
#include <net/if.h>
#include <netinet/in.h>
#include <storage/file.h>

#define CONFIG_FILE "/system/config/net.cfg"
#define NETINIT_FILE "/system/network-init.sh"
#define APPLY_CMD "/bin/bash /system/network-init.sh"

bool ParseIPAddress( const String &cSrc, IPAddress_t * pnDest )
{
	uint32 n1, n2, n3, n4;
	int nResult = sscanf( cSrc.c_str(), "%u.%u.%u.%u", &n1, &n2, &n3, &n4 );

	if( nResult != 4 )
		return false;

	if( n1 > 255 || n2 > 255 || n3 > 255 || n4 > 255 )
		return false;

	*pnDest = ToIPAddress( ( uint8 )n1, ( uint8 )n2, ( uint8 )n3, ( uint8 )n4 );
	return true;
}

IPAddress_t ToIPAddress( uint8 n1, uint8 n2, uint8 n3, uint8 n4 )
{
	return ( IPAddress_t ) ( ( n4 & 0xFF ) << 24 ) | ( ( n3 & 0xFF ) << 16 ) | ( ( n2 & 0xFF ) << 8 ) | ( n1 & 0xFF );
}

String IPAddressToString( IPAddress_t nAddr )
{
	char zBuf[16];

	sprintf( zBuf, "%d.%d.%d.%d", nAddr & 0xFF, ( nAddr >> 8 ) & 0xFF, ( nAddr >> 16 ) & 0xFF, ( nAddr >> 24 ) & 0xFF );
	return String ( ( const char * )&zBuf );
}

void InterfaceManager::DiscoverInterfaces( InterfaceList_t & cInterfaces )
{
	ClearInterfaceList( cInterfaces );
	int nSock = socket( AF_INET, SOCK_STREAM, 0 );

	int nIFaceCount = ioctl( nSock, SIOCGIFCOUNT, NULL );

	if( nIFaceCount == 0 )
		return;

	struct ifreq *psIFReqs = ( struct ifreq * )calloc( nIFaceCount, sizeof( struct ifreq ) );
	struct ifconf sIFConf;
	sIFConf.ifc_len = nIFaceCount * sizeof( struct ifreq );
	sIFConf.ifc_req = psIFReqs;

	ioctl( nSock, SIOCGIFCONF, &sIFConf );
	char zMACBuf[18];

	for( int i = 0; i < nIFaceCount; i++ )
	{
		struct ifreq sReq;

		memcpy( &sReq, &( sIFConf.ifc_req[i] ), sizeof( sReq ) );
		ioctl( nSock, SIOCGIFFLAGS, &sReq );
		short nFlags = sReq.ifr_flags;

		if( !( nFlags & IFF_LOOPBACK ) )
		{
			ioctl( nSock, SIOCGIFHWADDR, &sReq );
			uint8 *pnMac = ( uint8 * )( sReq.ifr_hwaddr.sa_data );

			sprintf( zMACBuf, "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X", ( uint )pnMac[0], ( uint )pnMac[1], ( uint )pnMac[2], ( uint )pnMac[3], ( uint )pnMac[4], ( uint )pnMac[5] );

			ioctl( nSock, SIOCGIFFLAGS, &sReq );

			Interface *pcIFace = new Interface( Static, sIFConf.ifc_req[i].ifr_name, zMACBuf );

			pcIFace->SetEnabled( nFlags & IFF_UP );

			ioctl( nSock, SIOCGIFADDR, &sReq );
			IPAddress_t *pnAddr = ( IPAddress_t * ) & ( ( struct sockaddr_in * )&( sReq.ifr_addr ) )->sin_addr;

			pcIFace->SetAddress( *pnAddr );

			ioctl( nSock, SIOCGIFNETMASK, &sReq );
			pcIFace->SetNetmask( *pnAddr );

			cInterfaces.push_back( pcIFace );
		}
	}
}

void InterfaceManager::LoadConfig( InterfaceList_t & cInterfaces )
{
	ClearInterfaceList( cInterfaces );
	File cFile;

	if( cFile.SetTo( CONFIG_FILE ) == 0 )
	{
		StringReader cSr( &cFile );
		std::vector < String >cLines;

		cLines.push_back( cSr.ReadLine() );
		while( cSr.HasLines() )
		{
			cLines.push_back( cSr.ReadLine() );
		}

		String cName;
		String cMAC;
		IPAddress_t nAddress;
		IPAddress_t nNetmask;
		IPAddress_t nGateway;
		bool bEnabled = false;
		bool bDefault = false;
		IFaceType_t nType = Static;
		bool bInIFace = false;
		Interface *pcIFace = NULL;

		for( std::vector < String >::iterator i = cLines.begin(); i != cLines.end(  ); i++ )
		{

			if( *i == "interface" )
			{
				bInIFace = true;
				continue;
			}

			if( *i == "" )
			{
				if( bInIFace )
				{
					pcIFace = new Interface( nType, cName, cMAC );
					pcIFace->SetEnabled( bEnabled );
					pcIFace->SetDefault( bDefault );
					if( nType == Static )
					{
						pcIFace->SetAddress( nAddress );
						pcIFace->SetNetmask( nNetmask );
						pcIFace->SetGateway( nGateway );
					}
					cInterfaces.push_back( pcIFace );
				}
				bInIFace = false;
				continue;
			}

			if( bInIFace )
			{
				if( ( *i ).find( "type" ) == 0 )
				{
					nType = ( *i ).substr( 5 ) == "Static" ? Static : DHCP;
				}
				else if( ( *i ).find( "name" ) == 0 )
				{
					cName = ( *i ).substr( 5 );
				}
				else if( ( *i ).find( "mac" ) == 0 )
				{
					cMAC = ( *i ).substr( 4 );
				}
				else if( ( *i ).find( "address" ) == 0 )
				{
					ParseIPAddress( ( *i ).substr( 8 ), &nAddress );
				}
				else if( ( *i ).find( "netmask" ) == 0 )
				{
					ParseIPAddress( ( *i ).substr( 8 ), &nNetmask );
				}
				else if( ( *i ).find( "gateway" ) == 0 )
				{
					ParseIPAddress( ( *i ).substr( 8 ), &nGateway );
				}
				else if( ( *i ).find( "enabled" ) == 0 )
				{
					bEnabled = ( *i ).substr( 8 ) == "true";
				}
				else if( ( *i ).find( "default" ) == 0 )
				{
					bDefault = ( *i ).substr( 8 ) == "true";
				}
			}
		}
	}
}

void InterfaceManager::SaveConfig( InterfaceList_t & cInterfaces )
{
	File cFile;

	if( cFile.SetTo( CONFIG_FILE, O_CREAT | O_TRUNC ) == 0 )
	{
		StringWriter cSw( &cFile );

		for( InterfaceList_t::iterator i = cInterfaces.begin(); i != cInterfaces.end(  ); i++ )
		{
			cSw.WriteLine( "interface" );
			cSw.Write( "type " );
			cSw.WriteLine( ( *i )->GetType() == Static ? "Static" : "DHCP" );
			cSw.Write( "name " );
			cSw.WriteLine( ( *i )->GetName() );
			cSw.Write( "mac " );
			cSw.WriteLine( ( *i )->GetMAC() );
			cSw.Write( "enabled " );
			cSw.WriteLine( ( *i )->GetEnabled()? "true" : "false" );
			cSw.Write( "default " );
			cSw.WriteLine( ( *i )->IsDefault()? "true" : "false" );

			switch ( ( *i )->GetType() )
			{
			case Static:
				{
					cSw.Write( "address " );
					cSw.WriteLine( IPAddressToString( ( *i )->GetAddress() ) );
					cSw.Write( "netmask " );
					cSw.WriteLine( IPAddressToString( ( *i )->GetNetmask() ) );
					cSw.Write( "gateway " );
					cSw.WriteLine( IPAddressToString( ( *i )->GetGateway() ) );
					break;
				}
			case DHCP:
				break;
			}
			cSw.WriteLine( "" );
		}
	}
}

void InterfaceManager::ClearInterfaceList( InterfaceList_t & cInterfaces )
{
	for( InterfaceList_t::iterator i = cInterfaces.begin(); i != cInterfaces.end(  ); i++ )
	{
		delete( *i );
	}

	cInterfaces.clear();
}

bool InterfaceManager::InterfacesHaveChanged( const InterfaceList_t & cConfigIF, const InterfaceList_t & cDetectIF )
{
	if( cConfigIF.size() != cDetectIF.size(  ) )
		return true;

	for( uint32 i = 0; i < cConfigIF.size(); i++ )
		if( cConfigIF[i]->GetName() != cDetectIF[i]->GetName(  ) )
			return true;

	return false;
}

void InterfaceManager::WriteNetworkInit( InterfaceList_t & cInterfaces )
{
	File cFile;

	if( cFile.SetTo( NETINIT_FILE, O_CREAT | O_TRUNC ) == 0 )
	{
		StringWriter cSw( &cFile );

		cSw.WriteLine( "#!/bin/sh" );

		for( InterfaceList_t::iterator i = cInterfaces.begin(); i != cInterfaces.end(  ); i++ )
		{
			cSw.Write( "ifconfig " );
			cSw.Write( ( *i )->GetName() );
			cSw.WriteLine( " down" );

			if( ( *i )->GetEnabled() )
			{
				cSw.Write( "echo \"Enabling interface " );
				cSw.Write( ( *i )->GetName() );
				cSw.WriteLine( "\" >> /var/log/kernel" );
				switch ( ( *i )->GetType() )
				{
				case Static:
					cSw.Write( "ifconfig " );
					cSw.Write( ( *i )->GetName() );
					cSw.Write( " up inet " );
					cSw.Write( IPAddressToString( ( *i )->GetAddress() ) );
					cSw.Write( " netmask " );
					cSw.WriteLine( IPAddressToString( ( *i )->GetNetmask() ) );

					if( ( *i )->GetGateway() > 0 )
					{
						cSw.Write( "route add net " );
						cSw.Write( IPAddressToString( ( *i )->GetAddress() ) );
						cSw.Write( " mask " );
						if( ( *i )->IsDefault() )
						{
							cSw.Write( "0.0.0.0" );
						}
						else
						{
							cSw.Write( IPAddressToString( ( *i )->GetNetmask() ) );
						}
						cSw.Write( " gw " );
						cSw.WriteLine( IPAddressToString( ( *i )->GetGateway() ) );
					}
					break;
				case DHCP:
					cSw.Write( "dhcpc -d " );
					cSw.Write( ( *i )->GetName() );
					cSw.WriteLine( " 2>>/var/log/dhcpc" );
					break;
				}
			}
		}
	}
}

void InterfaceManager::InvokeNetworkInit( void )
{
	chmod( NETINIT_FILE, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );
	system( APPLY_CMD );
}
