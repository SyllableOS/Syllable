/*
 *  The Syllable kernel
 *  Configuration file management
 *  Copyright (C) 2003 The Syllable Team
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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

#ifndef _SYLLABLE_CONFIG_H_
#define _SYLLABLE_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

#ifdef __KERNEL__

status_t write_kernel_config_entry_header( char* pzName, size_t nSize );
status_t write_kernel_config_entry_data( uint8* pBuffer, size_t nSize );
status_t read_kernel_config_entry( char* pzName, uint8** pBuffer, size_t* pnSize );
void write_kernel_config( void );
void init_kernel_config( void );

#endif


#ifdef __cplusplus
}
#endif

#endif /* _SYLLABLE_CONFIG_H_ */













