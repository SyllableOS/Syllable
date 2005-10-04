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

#ifndef	__F_ATHEOS_MSGPORTS_H__
#define	__F_ATHEOS_MSGPORTS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <atheos/types.h>

#define	DEFAULT_PORT_SIZE		15

#define	MSG_PORT_PUBLIC			1<<1

#ifdef __KERNEL__

port_id		sys_create_port( const char* const pzName, int nMaxCount );
status_t	sys_delete_port( port_id hPort );

status_t	send_msg_x( port_id hPort, uint32 nCode, const void* pBuffer, int nSize, bigtime_t nTimeOut );
status_t	get_msg_x( port_id hPort, uint32* pnCode, void* pBuffer, int nSize, bigtime_t nTimeOut );

status_t	sys_send_msg( port_id hPort, uint32 nCode, const void* pBuffer, int nSize );
status_t	sys_get_msg( port_id hPort, uint32* pnCode, void* pBuffer, int nSize );
status_t	sys_raw_send_msg_x( port_id hPort, uint32 nCode, const void* pBuffer,
				    int nSize, const bigtime_t* pnTimeOut );
status_t	sys_raw_get_msg_x( port_id hPort, uint32* pnCode, void* pBuffer,
				   int nSize, const bigtime_t* pnTimeOut );

status_t sys_make_port_public( port_id hPort );
status_t sys_make_port_private( port_id hPort );

port_id sys_find_port( const char* pzPortname );

size_t sys_get_msg_size( port_id hPort );

#endif /* __KERNEL__ */



typedef struct
{
    port_id	pi_port_id;
    uint32	pi_flags;
    int		pi_max_count;
    int		pi_cur_count;
    proc_id	pi_owner;
    char	pi_name[OS_NAME_LENGTH];
} port_info;


port_id		create_port( const char* const pzName, int nMaxCount );
status_t	delete_port( port_id hPort );

status_t	get_port_info( port_id hPort, port_info* psInfo );
status_t	get_next_port_info( port_info* psInfo );


status_t	send_msg( port_id hPort, uint32 nCode, const void* pBuffer, int nSize );
status_t	get_msg( port_id hPort, uint32* pnCode, void* pBuffer, int nSize );

status_t	send_msg_x( port_id hPort, uint32 nCode, const void* pBuffer, int nSize, bigtime_t nTimeOut );
status_t	get_msg_x( port_id hPort, uint32* pnCode, void* pBuffer, int nSize, bigtime_t nTimeOut );

status_t	raw_send_msg_x( port_id hPort, uint32 nCode, const void* pBuffer,
				int nSize, const bigtime_t* pnTimeOut );
status_t	raw_get_msg_x( port_id hPort, uint32* pnCode, void* pBuffer,
			       int nSize, const bigtime_t* pnTimeOut );

status_t make_port_public( port_id hPort );
status_t make_port_private( port_id hPort );

port_id find_port( const char* pzPortname );

size_t get_msg_size( port_id hPort );

#ifdef __cplusplus
}
#endif


#endif	/* __F_ATHEOS_MSGPORTS_H__ */

