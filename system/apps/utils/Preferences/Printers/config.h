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

#ifndef PRINTERS_CONFIG_H
#define PRINTERS_CONFIG_H

#include <time.h>
#include <util/string.h>
#include <vector>

#define PRINTERS_CONF		"/system/config/cups/printers.conf"
#define SYLLABLE_CD_PREFIX	"Syllable "

class CUPS_Printer
{
	public:
		CUPS_Printer()
		{
			cName = 
			cOriginalName = 
			cInfo = 
			cDeviceURI = "";

			cState = "Idle";
			cStateTime.Format( "%d", time( NULL ) );
			bAccepting = true;
			bShared = false;
			cJobSheets = "none none";
			cQuotaPeriod = "0";
			cPageLimit = "0";
			cKLimit = "0";
			cOpPolicy = "default";
			cErrorPolicy = "retry-job";
			cRetryInterval = "30";

			bDefault = false;

			cPPD = "";
		};
		void ParseURI( void );
		void BuildURI( void );

		os::String cName;
		os::String cOriginalName;

		os::String cInfo;
		os::String cDeviceURI;
		os::String cState;
		os::String cStateTime;
		bool bAccepting;
		bool bShared;
		os::String cJobSheets;
		os::String cQuotaPeriod;
		os::String cPageLimit;
		os::String cKLimit;
		os::String cOpPolicy;
		os::String cErrorPolicy;
		os::String cRetryInterval;

		bool bDefault;

		std::vector< std::pair <os::String, os::String> > vUnknown;

		os::String cPPD;

		/* Component parts of cDeviceURI */
		os::String cProtocol;
		os::String cUser;
		os::String cPass;
		os::String cDevice;
};

#endif	/* PRINTERS_CONFIG_H */

