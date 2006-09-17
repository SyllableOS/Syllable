/* Copyright (C) 1994,95,97,2000,02 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/* Put the name of the current YP domain in no more than LEN bytes of NAME.
   The result is null-terminated if LEN is large enough for the full
   name and the terminator.  */

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <sys/param.h>

#define RESOLVER_FILE "/etc/resolv.conf"
#define BUF_SIZE 1024

int
getdomainname (name, len)
     char *name;
     size_t len;
{
  if( len > 0 )
  {
    int i, l, file;
    char buf[BUF_SIZE], *s, *e;

    memset( name, '\0', len );

    file = open( RESOLVER_FILE, O_RDONLY );
    if( file < 0 )
    {
      return -1;
    }
    else
    {
      read( file, buf, BUF_SIZE );
      close( file );

      s = buf;
      for( i=0; i < BUF_SIZE; i++, s++ )
      {
        if( strncmp( s, "domain", 6) == 0 || strncmp( s, "search", 6) == 0 )
        {
          s+=6;
          while ( *s != '\0' && isspace(*s) )
            s++;
          if ((e = strchr(s, ' ')) != NULL)
            *e = '\0';
          if ((e = strchr(s, '\t')) != NULL)
            *e = '\0';
          if ((e = strchr(s, '\n')) != NULL)
            *e = '\0';

          strncpy(name, s, len);
          break;
        }
      }
    }

    return 0;
  }
  else
  {
    __set_errno(EINVAL);
    return -1;
  }
}
libc_hidden_def (getdomainname)
