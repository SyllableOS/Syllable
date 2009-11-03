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

#ifndef	__F_KERNEL_IMAGE_H__
#define	__F_KERNEL_IMAGE_H__

#include <kernel/types.h>
#include <syllable/image.h>

int get_image_info( bool bKernel, int nImage, int nSubImage, image_info* psInfo );

int get_symbol_by_address( int nLibrary, const char* pAddress, char* pzName, int nMaxNamLen, void** ppAddress );

int load_kernel_driver( const char* pzPath );
int unload_kernel_driver( int nLibrary );

#endif	/* __F_KERNEL_IMAGE_H__ */
