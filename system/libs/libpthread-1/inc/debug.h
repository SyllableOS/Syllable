/* POSIX threads for Syllable - A simple PThreads implementation             */
/*                                                                           */
/* Copyright (C) 2002 Kristian Van Der Vliet (vanders@users.sourceforge.net) */
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

#ifndef __F_SYLLABLE_PTHREAD_DEBUG_H_
#define __F_SYLLABLE_PTHREAD_DEBUG_H_

#ifdef __ENABLE_DEBUG__

#include <atheos/types.h>
#include <stdio.h>

#define TRACE_BUF_SIZE	4096
void __pt_backtrace( bool do_abort );

#define debug(format,arg...); fprintf(stderr,"%s: ",__FUNCTION__);fprintf(stderr,format, ## arg);
#define __pt_assert(expr); if(!(expr)){fprintf(stderr, "%s: Assertion '"#expr"' failed at line %d in %s\n", __FUNCTION__, __LINE__, __FILE__);__pt_backtrace(true);}

#else

#define debug(format,arg...);
#define __pt_assert(expr);

#endif	/* __ENABLE_DEBUG__ */

#endif	/* __F_SYLLABLE_PTHREAD_DEBUG_H_ */

