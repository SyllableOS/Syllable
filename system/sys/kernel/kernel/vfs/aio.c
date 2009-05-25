/*
 *  The Syllable kernel
 *  Copyright (C) 2009 Kristian Van Der Vliet
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

#include <posix/errno.h>

#include "inc/aio.h"

int aio_insert_op( struct aiocb *pAiocb )
{
	/* Setup new AIO buffer & worker thread (if required) */
	/* Put the pointer to the aiocb into the ring buffer */

	return -ENOSYS;
}

int sys_aio_op( struct aiocb *pAiocb )
{
	int nError = -EINVAL;

	if ( pAiocb && ( ( pAiocb->aio_lio_opcode == AIO_READ ) || ( pAiocb->aio_lio_opcode == AIO_WRITE ) ) )
		nError = aio_insert_op( pAiocb );

	return nError;
}
