// dhcpc : A DHCP client for Syllable
// (C)opyright 2002-2003 Kristian Van Der Vliet
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

#ifndef __F_DHCP_DEBUG_H__
#define __F_DHCP_DEBUG_H__

enum err_levels{
	INFO,
	ERROR,
	WARNING,
	PANIC
};

#if defined(ENABLE_DEBUG) && defined(DEBUG_LEVEL)
#  include <stdio.h>
#  define debug(level,func,format,arg...) if(level>=DEBUG_LEVEL){fprintf(stderr,"%s : ",func);fprintf(stderr,format, ## arg);}
#else
#  define debug(level,func,format,arg...)
#endif

#endif		/*__F_DHCP_DEBUG_H__*/

