/*
 *  The AtheOS kernel
 *  Copyright (C) 2000 - 2001 Kurt Skauen
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

#ifndef __F_ATHEOS_BOOTMODULE__
#define __F_ATHEOS_BOOTMODULE__

#include <atheos/types.h>

typedef struct
{
    void*	bm_pAddress;
    uint32	bm_nSize;
    const char*	bm_pzModuleArgs;
    area_id	bm_hAreaID;
} BootModule_s;

int	      get_boot_module_count( void );
BootModule_s* get_boot_module( int nIndex );
void	      put_boot_module( BootModule_s* psModule );

int get_kernel_arguments( int* argc, const char* const** argv );



#endif /* __F_ATHEOS_BOOTMODULE__ */
