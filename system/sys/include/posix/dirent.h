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

#ifndef _ATHEOS_DIRENT_H_
#define _ATHEOS_DIRENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef	NAME_MAX
#define	MAXNAMLEN	NAME_MAX
#else
#define	MAXNAMLEN	255
#endif

struct kernel_dirent {
  unsigned char d_namlen;
  char d_name[MAXNAMLEN+1];
  long long d_ino;
};

#ifdef __KERNEL__

int getdents( int nFile, struct kernel_dirent *psDirEnt, int nCount );

#endif /* __KERNEL__ */

#ifdef __cplusplus
}
#endif


#endif /*  _ATHEOS_DIRENT_H_ */
