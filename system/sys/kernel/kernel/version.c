/*
 *  The Syllable kernel
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 Kristian Van Der Vliet
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
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

#include "version.h"

#define MAJOR	0LL
#define MINOR	6LL
#define RELEASE	6LL

const int64 g_nKernelVersion = ( MAJOR << 32 ) | ( MINOR << 16 ) | ( RELEASE );
const char *g_pzKernelName = "kernel.so";
const char *g_pzBuildData = __DATE__;
const char *g_pzBuildTime = __TIME__;
const char *g_pzCpuArch = "i586";
const char *g_pzSystem = "syllable";

/* Anyone checking a kernel revision into CVS must remember to bump this
 * build number.  For private branches or patches backported to older versions
 * of Syllable, add a letter and revision number; for example a patch to
 * Build 1023 becomes Build 1023r1.
 */
const char *g_pzBuildVersion = "0001";


