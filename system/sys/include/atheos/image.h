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

#ifndef	__F_ATHEOS_IMAGE_H__
#define	__F_ATHEOS_IMAGE_H__

#ifdef __cplusplus
extern "C" {
#endif
#if 0
} /* Make emacs indention work */
#endif

#include <atheos/types.h>

typedef int image_entry( char** argv, char** envv );
typedef int image_init( int nImageID );
typedef void image_term( int nImageID );

typedef struct
{
    int		 ii_image_id;		// image id.
    int		 ii_type;		// image type (IM_APP_SPACE, IM_ADDON_SPACE or IM_LIBRARY_SPACE).
    int		 ii_open_count;		// open count private to the owner process.
    int		 ii_sub_image_count;	// number of dll's linked to this image.
    int		 ii_init_order;		// the larger the later to be initiated.
    image_entry* ii_entry_point;	// entry point (_start())
    image_init*	 ii_init;		// init routine. (global contructors)
    image_term*	 ii_fini;		// terminate routine. (global destructors)
    dev_t	 ii_device;		// device number for file.
    int		 ii_inode;		// inode number for file.
    char	 ii_name[ 256 ];  	// full pathname of image.
    void*	 ii_text_addr;		// address of text segment.
    void*	 ii_data_addr;		// address of data segment.
    int		 ii_text_size;		// size of text segment.
    int		 ii_data_size;		// size of data segment (including BSS).
} image_info;

enum
{
    IM_KERNEL_SPACE,
    IM_APP_SPACE,
    IM_ADDON_SPACE,
    IM_LIBRARY_SPACE,
};

int load_library( const char* pzPath, uint32 nFlags );
int unload_library( int nLibrary );
int get_library_info( int nLibrary, image_info* psInfo );
int get_symbol_address( int nLibrary, const char* pzName, int nIndex, void** pPtr );
int get_image_id( void );

#ifdef __KERNEL__
int get_image_info( bool bKernel, int nImage, int nSubImage, image_info* psInfo );
#else
int get_image_info( int nImage, int nSubImage, image_info* psInfo );
#endif

int get_symbol_address( int nLibrary, const char* pzName, int nIndex, void** pPtr );

int get_symbol_by_address( int nLibrary, const char* pAddress, char* pzName, int nMaxNamLen, void** ppAddress );
int find_module_by_address( const void* pAddress );

int load_kernel_driver( const char* pzPath );
int unload_kernel_driver( int nLibrary );


#ifdef __cplusplus
}
#endif

#endif	/* __F_ATHEOS_IMAGE_H__ */
