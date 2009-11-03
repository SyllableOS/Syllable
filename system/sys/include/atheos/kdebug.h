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

#ifndef __F_ATHEOS_KDEBUG_H__
#define __F_ATHEOS_KDEBUG_H__

#ifdef __cplusplus
extern "C" {
#endif

int debug_read( int nPort, char* pBuffer, int nSize );
int debug_write( int nPort, const char* pBuffer, int nSize );

int dbprintf( const char* pzFmt, ... ) __attribute__ ((format (printf, 1, 2)));

#ifndef CALLED
# define CALLED()	dbprintf("CALLED %s\n", __PRETTY_FUNCTION__)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __F_ATHEOS_KDEBUG_H__ */

