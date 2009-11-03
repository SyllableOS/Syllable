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

#ifndef	__F_KERNEL_MSGPORTS_H__
#define	__F_KERNEL_MSGPORTS_H__

#include <kernel/types.h>
#include <syllable/msgport.h>

port_id sys_create_port( const char* const pzName, int nMaxCount );
status_t sys_delete_port( port_id hPort );

status_t send_msg_x( port_id hPort, uint32 nCode, const void* pBuffer, int nSize, bigtime_t nTimeOut );
status_t get_msg_x( port_id hPort, uint32* pnCode, void* pBuffer, int nSize, bigtime_t nTimeOut );

status_t sys_send_msg( port_id hPort, uint32 nCode, const void* pBuffer, int nSize );
status_t sys_get_msg( port_id hPort, uint32* pnCode, void* pBuffer, int nSize );
status_t sys_raw_send_msg_x( port_id hPort, uint32 nCode, const void* pBuffer, int nSize, const bigtime_t* pnTimeOut );
status_t sys_raw_get_msg_x( port_id hPort, uint32* pnCode, void* pBuffer, int nSize, const bigtime_t* pnTimeOut );

status_t sys_make_port_public( port_id hPort );
status_t sys_make_port_private( port_id hPort );

port_id sys_find_port( const char* pzPortname );

size_t sys_get_msg_size( port_id hPort );

port_id	 sys_get_app_server_port( void );

#endif	/* __F_KERNEL_MSGPORTS_H__ */
