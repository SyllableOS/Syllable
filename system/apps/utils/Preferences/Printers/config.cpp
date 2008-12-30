/*
 *  Syllable Printer configuration preferences
 *
 *  (C)opyright 2006 - Kristian Van Der Vliet
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <util/string.h>
#include <storage/file.h>
#include <gui/exceptions.h>
#include <fcntl.h>

#include <cups/cups.h>

#include <printers.h>
#include <config.h>

#include "resources/Printers.h"

using namespace os;

/* Read and parse the CUPS printers.conf file (because we can't get some of this information any other way!) */
status_t PrintersWindow::ReadConfig( void )
{
	status_t nError = EOK;

	char *pBuffer = NULL;
	try
	{
		File cFile( PRINTERS_CONF, O_RDONLY | O_CREAT );
		off_t nSize = cFile.GetSize();

		pBuffer = (char*)malloc( nSize );
		if( NULL == pBuffer )
			throw errno_exception( strerror( ENOMEM ), ENOMEM );

		nSize = cFile.Read( pBuffer, nSize );

		char *pNext = pBuffer;
		CUPS_Printer *pcPrinter = NULL;
		while( ( pNext != NULL ) && ( pNext < pBuffer + nSize ) )
		{
			char *pStart, *pEnd, *pData;
			size_t nLen;
			String cKey, cValue;

			if( *pNext == '#' )
			{
				pNext = strchr( pNext, '\n' );
			}
			else if( *pNext == '<' )
			{
				if( NULL == pcPrinter )
				{
					/* Start of section */
					pcPrinter = new CUPS_Printer();
					if( NULL == pcPrinter )
						throw errno_exception( strerror( ENOMEM ), ENOMEM );

					/* Is this the DefaultPrinter? */
					pStart = pNext + 1;
					pEnd = strchr( pNext, ' ' );
					if( NULL == pEnd )
					{
						pNext++;
						continue;
					}
					nLen = ( pEnd - pStart ) + 1;
					if( strncmp( pStart, "DefaultPrinter", pEnd - pStart ) == 0 )
						pcPrinter->bDefault=true;
					else
						pcPrinter->bDefault=false;

					/* Get printer name */
					pStart = pEnd + 1;
					pEnd = strchr( pStart, '>' );
					if( NULL == pEnd )
					{
						pNext++;
						continue;
					}

					nLen = ( pEnd - pStart ) + 1;
					pData = (char*)calloc( 1, nLen );
					if( NULL == pData )
						throw errno_exception( strerror( ENOMEM ), ENOMEM );
					memcpy( pData, pStart, pEnd - pStart );
					cKey = pData;
					free( pData );

					pcPrinter->cName = cKey.Strip();
					pcPrinter->cOriginalName = pcPrinter->cName;
				}
				else
				{
					/* End of section */
					m_vpcPrinters.push_back( pcPrinter );
					pcPrinter = NULL;
				}

				pNext = strchr( pNext, '\n' );
			}
			else
			{
				/* If this is a line within a section, parse it */
				if( pcPrinter != NULL )
				{
					pStart = pNext;
					pEnd = strchr( pStart, ' ' );
					if( NULL == pEnd )
					{
						pNext++;
						continue;
					}

					nLen = ( pEnd - pStart ) + 1;
					pData = (char*)calloc( 1, nLen );
					if( NULL == pData )
						throw errno_exception( strerror( ENOMEM ), ENOMEM );
					memcpy( pData, pStart, pEnd - pStart );
					cKey = pData;
					free( pData );

					pStart = pEnd + 1;
					pEnd = strchr( pStart, '\n' );
					if( NULL == pEnd )
					{
						pNext++;
						continue;
					}

					nLen = ( pEnd - pStart ) + 1;
					pData = (char*)calloc( 1, nLen );
					if( NULL == pData )
						throw errno_exception( strerror( ENOMEM ), ENOMEM );
					memcpy( pData, pStart, pEnd - pStart );
					cValue = pData;
					free( pData );

					/* Store the settings we're interested in */
					PushKeyPair( cKey.Strip(), cValue.Strip(), pcPrinter );

					pNext = pEnd + 1;
				}
				else
					pNext++;	/* Skip */
			}
		}
	}
	catch( errno_exception &e )
	{
		nError = e.error();
	}

	if( pBuffer )
		free( pBuffer );


	return nError;
}

/* Store the value for the given key */
status_t PrintersWindow::PushKeyPair( String cKey, String cValue, CUPS_Printer *pcPrinter )
{
	if( NULL == pcPrinter || cKey == "" )
		return EINVAL;

	if( cKey == "Info" )
	{
		pcPrinter->cInfo = cValue;
	}
	else if( cKey == "DeviceURI" )
	{
		pcPrinter->cDeviceURI = cValue;

		/* Break the URI down */
		pcPrinter->ParseURI();
	}
	else if( cKey == "State" )
	{
		pcPrinter->cState = cValue;
	}
	else if( cKey == "StateTime" )
	{
		pcPrinter->cStateTime = cValue;
	}
	else if( cKey == "Accepting" )
	{
		if( cValue == "Yes" )
			pcPrinter->bAccepting = true;
		else
			pcPrinter->bAccepting = false;
	}
	else if( cKey == "Shared" )
	{
		if( cValue == "Yes" )
			pcPrinter->bShared = true;
		else
			pcPrinter->bShared = false;
	}
	/* We're not so interested in these, but they get stored so we can write them back out */
	else if( cKey == "JobSheets" )
	{
		pcPrinter->cJobSheets = cValue;
	}
	else if( cKey == "QuotaPeriod" )
	{
		pcPrinter->cQuotaPeriod = cValue;
	}
	else if( cKey == "PageLimit" )
	{
		pcPrinter->cPageLimit = cValue;
	}
	else if( cKey == "KLimit" )
	{
		pcPrinter->cKLimit = cValue;
	}
	else if( cKey == "OpPolicy" )
	{
		pcPrinter->cOpPolicy = cValue;
	}
	else if( cKey == "ErrorPolicy" )
	{
		pcPrinter->cErrorPolicy = cValue;
	}
	else if( cKey == "JobRetryInterval" )
	{
		pcPrinter->cRetryInterval = cValue;
	}
	/* Any keys we don't know about get stored too */
	else
	{
		std::pair<String, String>vUnknown;
		vUnknown.first = cKey;
		vUnknown.second = cValue;
		pcPrinter->vUnknown.push_back( vUnknown );
	}

	return EOK;
}

/* Write the CUPS printers.conf file and tell cupsd to re-read it */
status_t PrintersWindow::WriteConfig( void )
{
	status_t nError = EOK;

	try
	{
		String cLine;
		File cFile( PRINTERS_CONF, O_CREAT | O_TRUNC | O_WRONLY );

		cLine.Format( "# Printer configuration file for CUPS v%d.%d.%d\n", CUPS_VERSION_MAJOR, CUPS_VERSION_MINOR, CUPS_VERSION_PATCH );
		cFile.Write( cLine.c_str(), cLine.Length() );
		cLine = "# Written by Syllable Printer Preferences\n";
		cFile.Write( cLine.c_str(), cLine.Length() );

		std::vector <CUPS_Printer*>::iterator i;
		for( i = m_vpcPrinters.begin(); i != m_vpcPrinters.end(); i++ )
		{
			/* XXXKV: We must have some way to check for duplicate queue names */
			if( (*i)->bDefault )
				cLine.Format( "<DefaultPrinter %s>\n", (*i)->cName.c_str() );
			else
				cLine.Format( "<Printer %s>\n", (*i)->cName.c_str() );

			cFile.Write( cLine.c_str(), cLine.Length() );

			cLine.Format( "Info %s\n", (*i)->cInfo.c_str() );
			cFile.Write( cLine.c_str(), cLine.Length() );

			cLine.Format( "DeviceURI %s\n", (*i)->cDeviceURI.c_str() );
			cFile.Write( cLine.c_str(), cLine.Length() );

			cLine.Format( "State %s\n", (*i)->cState.c_str() );
			cFile.Write( cLine.c_str(), cLine.Length() );

			cLine.Format( "StateTime %s\n", (*i)->cStateTime.c_str() );
			cFile.Write( cLine.c_str(), cLine.Length() );

			cLine = "Accepting ";
			if( (*i)->bAccepting == true )
				cLine += "Yes\n";
			else
				cLine += "No\n";
			cFile.Write( cLine.c_str(), cLine.Length() );

			cLine = "Shared ";
			if( (*i)->bShared == true )
				cLine += "Yes\n";
			else
				cLine += "No\n";
			cFile.Write( cLine.c_str(), cLine.Length() );

			cLine.Format( "JobSheets %s\n", (*i)->cJobSheets.c_str() );
			cFile.Write( cLine.c_str(), cLine.Length() );

			cLine.Format( "QuotaPeriod %s\n", (*i)->cQuotaPeriod.c_str() );
			cFile.Write( cLine.c_str(), cLine.Length() );

			cLine.Format( "PageLimit %s\n", (*i)->cPageLimit.c_str() );
			cFile.Write( cLine.c_str(), cLine.Length() );

			cLine.Format( "KLimit %s\n", (*i)->cKLimit.c_str() );
			cFile.Write( cLine.c_str(), cLine.Length() );

			cLine.Format( "OpPolicy %s\n", (*i)->cOpPolicy.c_str() );
			cFile.Write( cLine.c_str(), cLine.Length() );

			cLine.Format( "ErrorPolicy %s\n", (*i)->cErrorPolicy.c_str() );
			cFile.Write( cLine.c_str(), cLine.Length() );

			cLine.Format( "JobRetryInterval %s\n", (*i)->cRetryInterval.c_str() );
			cFile.Write( cLine.c_str(), cLine.Length() );

			/* Anything else */
			std::vector< std::pair <os::String, os::String> >::iterator j;
			for( j = (*i)->vUnknown.begin(); j != (*i)->vUnknown.end(); j++ )
			{
				cLine.Format( "%s %s\n", (*j).first.c_str(), (*j).second.c_str() );
				cFile.Write( cLine.c_str(), cLine.Length() );
			}

			cLine = "</Printer>\n";
			cFile.Write( cLine.c_str(), cLine.Length() );

			/* If the queue name has changed we'll have to rename the PPD */
			if( (*i)->cOriginalName != (*i)->cName )
				nError = RenamePPD( (*i)->cOriginalName, (*i)->cName );

			/* Install any required PPDs */
			if( (*i)->cPPD != "" )
				nError = InstallPPD( (*i)->cPPD, (*i)->cName );
		}

		/* SIGHUP cupsd  This feels like such an utterly rubish way of doing it. */
		system( "kill_all -HUP cupsd" );
	}
	catch( errno_exception &e )
	{
		nError = e.error();
	}

	return nError;
}

/* Copy a PPD to etc/cups/ppd/ */
status_t PrintersWindow::InstallPPD( String cPPD, String cName )
{
	status_t nError = EOK;
	struct stat sStat;
	String cFromPath, cToPath;

	fprintf( stderr, "This printer requires PPD: %s\n", cPPD.c_str() );

	/* Check if the file is already in cups/share/cups/model/ */
	cFromPath = String( "/system/indexes/share/cups/model/" ) + cPPD;
	cToPath = String( "/system/config/cups/ppd/" ) + cName + String( ".ppd" );

	/* If required, copy the PPD from CD first */
	nError = stat( cFromPath.c_str(), &sStat );
	if( nError != EOK )
		nError = GetPPD( cPPD );

	/* Copy from cups/share/cups/model/ to etc/cups/ppd/ */
	if( nError == EOK )
		nError = CopyPPD( cFromPath, cToPath );

	return nError;
}

status_t PrintersWindow::GetPPD( String cPPD )
{
	status_t nError = EOK;
	int nWhich;
	String cFromPath, cToPath;

	cFromPath = cToPath = "";

	Alert *pcAlert = new Alert( MSG_ALRTWND_INSERTCD_TITLE, MSG_ALRTWND_INSERTCD_TEXT, Alert::ALERT_INFO, 0x00, MSG_ALRTWND_INSERTCD_OK.c_str(), MSG_ALRTWND_INSERTCD_CANCEL.c_str(), NULL );
	pcAlert->CenterInWindow( this );
	nWhich = pcAlert->Go();

	if( nWhich == 0 )
	{
		DIR *hRoot;
		struct dirent *psEnt;

		nError = ENOENT;

		hRoot = opendir( "/" );
		psEnt = readdir( hRoot );
		while( psEnt )
		{
			if( psEnt != NULL && psEnt->d_name != NULL )
			{
				if( strncmp( psEnt->d_name, SYLLABLE_CD_PREFIX, strlen( SYLLABLE_CD_PREFIX ) ) == 0 )
				{
					/* XXXKV: stat node and check that it is a CD we're looking at */
					cFromPath = String( "/" ) + String( psEnt->d_name );

					closedir( hRoot );
					nError = EOK;
					break;
				}	
			}
			psEnt = readdir( hRoot );
		}
	}
	else
		nError = EPERM;

	if( nError == EOK )
	{
		cFromPath += String( "/Packages/CUPS/PPD/" ) + cPPD;
		cToPath = String( "/system/indexes/share/cups/model/" ) + cPPD;

		nError = CopyPPD( cFromPath, cToPath );
	}

	return nError;
}

status_t PrintersWindow::RenamePPD( String cOriginal, String cNew )
{
	status_t nError = EOK;
	String cFromPath, cToPath;

	cFromPath = String( "/system/config/cups/ppd/" ) + cOriginal + String( ".ppd" );
	cToPath = String( "/system/config/cups/ppd/" ) + cNew + String( ".ppd" );

	if( rename( cFromPath.c_str(), cToPath.c_str() ) != 0 )
		nError = errno;

	return nError;
}

status_t PrintersWindow::CopyPPD( String cFrom, String cTo )
{
	status_t nError = EOK;
	struct stat sStat;
	off_t nSize;
	FILE *hFrom, *hTo;

	nError = stat( cFrom.c_str(), &sStat );
	if( nError != EOK )
		return errno;

	nSize = sStat.st_size;

	hFrom = fopen( cFrom.c_str(), "r" );
	hTo = fopen( cTo.c_str(), "w" );

	if( hFrom != NULL && hTo != NULL && nSize > 0 )
	{
		char *pBuffer = (char*)malloc( nSize );
		if( pBuffer != NULL )
		{
			/* CUPS can read Gziped PPDs directly, no need to mess with them ourselves */
			if( fread( pBuffer, nSize, 1, hFrom ) == 1 )
				fwrite( pBuffer, nSize, 1, hTo );
			else
				nError = errno;
			free( pBuffer );
		}
		else
			nError = errno;
	}
	else
		nError = errno;

	if( hFrom )
		fclose( hFrom );
	if( hTo )
		fclose( hTo );

	return nError;
}

/* Parse the cDeviceURI string into it's component parts */
void CUPS_Printer::ParseURI( void )
{
	if( cDeviceURI == "" )
		return;

	size_t nLen, nPos, nUserPos, nPassPos;
	nLen = cDeviceURI.size();

	nPos = cDeviceURI.find( ":" );
	if( nPos < nLen )
		cProtocol = cDeviceURI.substr( 0, nPos );

	while( ( cDeviceURI[nPos] == ':' || cDeviceURI[nPos] == '/' ) && nPos <= nLen )
		nPos++;

	nUserPos = cDeviceURI.find( ":", nPos );
	if( nUserPos < nLen )
	{
		cUser = cDeviceURI.substr( nPos, nUserPos - nPos );

		while( cDeviceURI[nUserPos] == ':' && nUserPos <= nLen )
			nUserPos++;

		nPassPos = cDeviceURI.find( "@", nUserPos );
		if( nPassPos < nLen )
			cPass = cDeviceURI.substr( nUserPos, nPassPos - nUserPos );

		while( cDeviceURI[nPassPos] == '@' && nPassPos <= nLen )
			nPassPos++;

		nPos = nPassPos;
	}

	cDevice = cDeviceURI.substr( nPos, nLen - nPos );
}

void CUPS_Printer::BuildURI( void )
{
	cDeviceURI = "";

	if( cProtocol != "" )
	{
		cDeviceURI = cProtocol + ":";
		if( cProtocol == "smb" || cProtocol == "ipp" )
			cDeviceURI += "//";
	}
	else
		cDeviceURI = ":";

	if( cUser != "" )
		cDeviceURI += cUser + ":";

	if( cPass != "" )
		cDeviceURI += cPass + "@";

	if( cDevice != "" )
		cDeviceURI += cDevice;
}
