/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2000 Kurt Skauen
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

#ifndef	__F_V86_H__
#define	__F_V86_H__

#include <atheos/types.h>

#ifdef __cplusplus
extern "C" {
#if 0  
} /*make emacs indention work */
#endif
#endif

struct RMREGS
{
  uint32	EDI;
  uint32	ESI;
  uint32	EBP;
  uint32	reserved_by_system;
  uint32	EBX;
  uint32	EDX;
  uint32	ECX;
  uint32	EAX;
  uint16	flags;
  uint16	ES,DS,FS,GS,IP,CS,SP,SS;
};

#ifdef __cplusplus
}
#endif

#endif	/* __F_V86_H__ */
