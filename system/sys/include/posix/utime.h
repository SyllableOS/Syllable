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

#ifndef __F_POSIX_UTIME_H__
#define __F_POSIX_UTIME_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <posix/types.h>

struct utimbuf
{
    time_t actime;
    time_t modtime;
};

int utime( const char* path, const struct utimbuf* times );

#ifdef __cplusplus
}
#endif

#endif /* __F_POSIX_UTIME_H__ */
