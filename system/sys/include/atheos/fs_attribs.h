/*
 *  The AtheOS kernel
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

#ifndef __F_FS_ATTRIBS_H__
#define __F_FS_ATTRIBS_H__

#include <atheos/kernel.h>
#include <atheos/types.h>
#ifdef __cplusplus
extern "C" {
#if 0  
} /*make emacs indention work */
#endif
#endif


  /* Parameter block used internally for sys_read_attr() and sys_write_attr(). */
typedef struct
{
    int		wa_nFile;
    const char*	wa_pzName;
    int		wa_nFlags;
    int		wa_nType;
    void*	wa_pBuffer;
    off_t	wa_nPos;
    size_t	wa_nLen;
} WriteAttrParams_s;



typedef enum fsattr_type
{
    ATTR_TYPE_NONE,	/*! untyped "raw" data */
    ATTR_TYPE_INT32,	/*! 32-bit integer value */
    ATTR_TYPE_INT64,	/*! 64-bit integer value (also used for time-values) */
    ATTR_TYPE_FLOAT,	/*! 32-bit floating point value */
    ATTR_TYPE_DOUBLE,	/*! 64-bit floating point value */
    ATTR_TYPE_STRING,	/*! UTF8 string */
    ATTR_TYPE_COUNT	/*! not a valid type, just gives the number of valid types */
} fsattr_type;

typedef struct attr_info
{
    off_t ai_size;
    int	  ai_type;
} attr_info_s;

struct index_info
{
    int	   ii_type;	/* Data type */
    off_t  ii_size;	/* Bytes used by index directory */
    time_t ii_mtime;	/* Modification time		 */
    time_t ii_ctime;	/* Creation time		 */
    uid_t  ii_uid;	/* User id of index owner	 */
    gid_t  ii_gid;	/* Group id of index owner	 */
};

#ifdef __KERNEL__
#include <posix/dirent.h>

int open_attrdir( int nFile );
int close_attrdir( int nFile );
int rewind_attrdir( int nFile );
int read_attrdir( int nFile, struct kernel_dirent* psBuffer, int nCount );

int write_attr( WriteAttrParams_s* psParams );
int read_attr( WriteAttrParams_s* psParams );

#else /* __KERNEL__ */
#include <dirent.h>
#include <sys/types.h>

DIR*   open_attrdir( int nFile );
int    close_attrdir( DIR* pDir );
void   rewind_attrdir( DIR* pDir );
struct dirent* read_attrdir( DIR* pDir );

ssize_t write_attr( int nFile, const char *pzAttrName, int nFlags, int nType,
		    const void* pBuffer, off_t nPos, size_t nLen );
ssize_t read_attr( int nFile, const char *pzAttrName, int nType,
		   void* pBuffer, off_t nPos, size_t nLen );


#endif /* __KERNEL__ */

int remove_attr( int nFile, const char* pzName );
int stat_attr( int nFile, const char* pzName, struct attr_info* psBuffer );



#ifdef __cplusplus
}
#endif


#endif /* __F_FS_ATTRIBS_H__ */
