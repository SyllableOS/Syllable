/* POSIX threads for Syllable - A simple PThreads implementation             */
/*                                                                           */
/* Copyright (C) 2002 Kristian Van Der Vliet (vanders@users.sourceforge.net) */
/* Copyright (C) 2000 Kurt Skauen                                            */
/*                                                                           */
/* This program is free software; you can redistribute it and/or             */
/* modify it under the terms of the GNU Library General Public License       */
/* as published by the Free Software Foundation; either version 2            */
/* of the License, or (at your option) any later version.                    */
/*                                                                           */
/* This program is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/* GNU Library General Public License for more details.                      */

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>

#include "inc/debug.h"

void __pt_backtrace( bool do_abort )
{
	void *addrs[TRACE_BUF_SIZE] = { 0 };
	char **syms;
	int size, cur_sym;

	size = backtrace( addrs, TRACE_BUF_SIZE );
	syms = backtrace_symbols( addrs, size );

	for( cur_sym = 0; cur_sym < size; cur_sym++ )
		fprintf( stderr, "%s\n", syms[cur_sym] );

	if( do_abort )
		abort();
}

