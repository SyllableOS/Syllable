/*
 *  Syllable Driver for VMware virtual video card
 *
 *  Based on the the xfree86 VMware driver.
 *
 *  Syllable specific code is
 *  Copyright (C) 2004, James Hayhurst (james_hayhurst@hotmail.com)
 *
 *  Other code is
 *  Copyright (C) 1998-2001 VMware, Inc.
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
 *
 */


#ifndef _GUEST_OS_H_
#define _GUEST_OS_H_

#define INCLUDE_ALLOW_USERLEVEL

#define GUEST_OS_BASE		0x5000
#define GUEST_OS_DOS        (GUEST_OS_BASE+1)
#define GUEST_OS_WIN31      (GUEST_OS_BASE+2)
#define GUEST_OS_WINDOWS95  (GUEST_OS_BASE+3)
#define GUEST_OS_WINDOWS98  (GUEST_OS_BASE+4)
#define GUEST_OS_WINDOWSME  (GUEST_OS_BASE+5)
#define GUEST_OS_NT         (GUEST_OS_BASE+6)
#define GUEST_OS_WIN2000    (GUEST_OS_BASE+7)
#define GUEST_OS_LINUX      (GUEST_OS_BASE+8)
#define GUEST_OS_OS2        (GUEST_OS_BASE+9)
#define GUEST_OS_OTHER      (GUEST_OS_BASE+10)
#define GUEST_OS_FREEBSD    (GUEST_OS_BASE+11)
#define GUEST_OS_WHISTLER   (GUEST_OS_BASE+12)


#endif
