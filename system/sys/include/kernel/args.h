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

#ifndef __F_KERNEL_ARGS_H__
#define __F_KERNEL_ARGS_H__

#include <kernel/types.h>

bool get_str_arg( char* pzValue, const char* pzName, const char* pzArg, int nArgLen );
bool get_num_arg( uint32* pnValue, const char* pzName, const char* pzArg, int nArgLen );
bool get_bool_arg( bool* pnValue, const char* pzName, const char* pzArg, int nArgLen );

#endif	/* __F_KERNEL_ARGS_H__ */
