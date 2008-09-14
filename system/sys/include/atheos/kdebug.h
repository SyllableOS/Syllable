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

#ifndef __F_KDEBUG_H__
#define __F_KDEBUG_H__

#include <atheos/types.h>
#include <atheos/threads.h>

#ifdef __cplusplus
extern "C" {
#if 0  
} /*make emacs indention work */
#endif
#endif

#define DB_PACKET_SIZE 128
#define DB_PORT_COUNT  16

#define DBP_PRINTK   0
#define DBP_DEBUGGER 2

typedef void dbg_fnc( int argc, char** argv );

#ifndef __KERNEL__

int debug_write( int nPort, const char* pBuffer, int nSize );
int debug_read( int nPort, char* pBuffer, int nSize );
int  dbprintf( const char* pzFmt, ... ) __attribute__ ((format (printf, 1, 2)));

#define CALLED()	dbprintf("CALLED %s\n", __PRETTY_FUNCTION__)

#else /* __KERNEL__ */

void trace_stack( uint32 nEIP, uint32* pStack );

int  sys_debug_write( int nPort, const char* pBuffer, int nSize );
int  sys_debug_read( int nPort, char* pBuffer, int nSize );
void debug_write( const char* pBuffer, int nSize );
int  dbprintf( int nPort, const char* pzFmt, ... ) __attribute__ ((format (printf, 2, 3)));
int  printk( const char* pzFmt, ... ) __attribute__ ((format (printf, 1, 2)));
void panic( const char* pzFmt, ... ) __attribute__ ((format (printf, 1, 2)));
void set_debug_port_params( int nBaudRate, int nPort, bool bPlainTextDebug );
void init_debugger( int nBaudRate, int nSerialPort );
void init_debugger_locks( void );
int  register_debug_cmd( const char* pzName, const char* pzDesc, dbg_fnc* pFunc );

void dbcon_write( const char* pData, int nSize );
void dbcon_clear( void );
void dbcon_set_color( int nColor, int nR, int nG, int nB );


enum debug_level{
	KERN_DEBUG_LOW,
	KERN_DEBUG,
	KERN_INFO,
	KERN_WARNING,
	KERN_FATAL,
	KERN_PANIC
};

#ifndef DEBUG_LIMIT
#define DEBUG_LIMIT	KERN_INFO	/* Default debug level */
#endif

#ifdef __ENABLE_DEBUG__
#define kerndbg(level,format,arg...) if(level>=DEBUG_LIMIT)printk(format, ## arg);
#define CALLED()					 if(level>=DEBUG_LIMIT)printk("CALLED %s\n", __PRETTY_FUNCTION__)
#else
#define kerndbg(level,format,arg...)
#define CALLED()
#endif	/* __ENABLE_DEBUG__ */

#endif /* __KERNEL__ */


#ifdef __cplusplus
}
#endif

#endif /* __F_KDEBUG_H__ */

