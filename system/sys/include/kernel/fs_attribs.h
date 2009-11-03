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

#ifndef __F_KERNEL_ATTRIBS_H__
#define __F_KERNEL_ATTRIBS_H__

#include <kernel/types.h>
#include <syllable/fs_attribs.h>
#include <posix/dirent.h>

struct index_info
{
    int	   ii_type;		/* Data type */
    off_t  ii_size;		/* Bytes used by index directory */
    time_t ii_mtime;	/* Modification time */
    time_t ii_ctime;	/* Creation time */
    uid_t  ii_uid;		/* User id of index owner */
    gid_t  ii_gid;		/* Group id of index owner */
};

int write_attr( WriteAttrParams_s* psParams );
int read_attr( WriteAttrParams_s* psParams );

/* Kernel attributes API */
int open_attrdir( int nFile );
int close_attrdir( int nFile );
int rewind_attrdir( int nFile );
int read_attrdir( int nFile, struct kernel_dirent* psBuffer, int nCount );

#endif /* __F_KERNEL_ATTRIBS_H__ */
