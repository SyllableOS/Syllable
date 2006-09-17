/* Copyright (C) 1996, 1997, 1998, 1999, 2002 Free Software Foundation, Inc.
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

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sysdep.h>
#include <ctype.h>
#include <sys/syscall.h>
#include <sys/debug.h>

int __execve (__const char *__path, char *__const __argv[],
		     char *__const __envp[])
{
	char shebang[_POSIX_PATH_MAX+1];
	int ret, image, bytes;

	image = open( __path, O_RDONLY );
	if( image < 0 )
		return -1;
	
	bytes = read( image, shebang, _POSIX_PATH_MAX );
	close( image );

	if( bytes > 3 && shebang[0] == '#' && shebang[1] == '!' )
	{
		char *handler;
		unsigned int len, argc, i;

		/* Extract the first line of the file, minus the shebang */
		shebang[_POSIX_PATH_MAX] = '\0';

		for( handler = shebang+2; isspace( *handler ); handler++ )
			/* EMPTY */;
		for( len = 0; shebang[len] != '\n' && shebang[len] != '\r' && shebang[len] != '\0'; len++ )
			/* EMPTY */;
		shebang[len] = '\0';

		if( _POSIX_PATH_MAX == len )
		{
			dbprintf( "interpreter arguments too long\n" );
			__set_errno( E2BIG );
			return -1;
		}

		/* Ignore everything between shebang and handler */
		len -= ( handler - shebang );

		/* Count the number of arguments to the interpreter */
		for( i = 0, argc = 1; i < len; i++ )
		{
			if ( isspace( handler[i] ) )
			{
				for( i++; i < len && isspace( handler[i] ); i++ )
					/* EMPTY */;
				if ( i < len )
				{
					argc++;
				}
			}
		}

		/* Add the size of argv to count. */
		for( i = 0; NULL != __argv[i]; i++ )
		{
			argc++;
		}

		/* Allocate argv array, C99 style, including trailing NULL pointer. */
		__const char *argv[argc + 1];
		argv[0] = handler;

		/* Add each interpreter argument to argv array. */
		for( i = 0, argc = 1; i < len; i++ )
		{
			if ( isspace( handler[i] ) )
			{
				handler[i] = '\0';

				for( i++; i < len && isspace( handler[i] ); i++ )
					/* EMPTY */;
				if ( i < len )
				{
					argv[argc++] = &handler[i];
				}
			}
		}

		/* Now add the path of the script to run. */
		argv[argc++] = __path;

		/* Copy each argument from original argv, except argv[0]. */
		for( i = 1; NULL != __argv[i]; i++ )
		{
			argv[argc++] = __argv[i];
		}

		/* Terminate array with NULL pointer. */
		argv[argc] = NULL;

		ret = INLINE_SYSCALL( execve, 3, handler, argv, __envp );
	}
	else
		ret = INLINE_SYSCALL( execve, 3, __path, __argv, __envp );

	return ret;
}
weak_alias(__execve, execve)
