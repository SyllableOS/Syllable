
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

#ifndef __F_ATHEOS_GLOBAL_H__
#define __F_ATHEOS_GLOBAL_H__

#ifdef __cplusplus
extern "C"
{
#if 0
}				/*make emacs indention work */
#endif
#endif

//extern int g_nDisableTS;	/* Nesting counter for Forbid()/Permit() */
extern bool g_bRootFSMounted;
extern bool g_bKernelInitialized;

#ifdef __cplusplus
}
#endif

#endif /* __F_ATHEOS_GLOBAL_H__ */
