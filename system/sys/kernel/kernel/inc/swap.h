
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

#ifndef __F_ATHEOS_SWAP_H__
#define __F_ATHEOS_SWAP_H__

#include <atheos/types.h>

#ifdef __cplusplus
extern "C"
{
#if 0
}				/*make emacs indention work */
#endif
#endif

static const size_t MAX_PAGE_AGE	= 127;
static const size_t PAGE_ADVANCE	= 5;
static const size_t PAGE_DECLINE	= 1;
static const size_t INITIAL_PAGE_AGE	= 20;

typedef struct
{
	uint32 si_nTotSize;
	uint32 si_nFreeSize;
	int si_nPageIn;
	int si_nPageOut;
} SwapInfo_s;


void init_swapper( void );
void dup_swap_page( int nPage );
void free_swap_page( int nPage );
int swap_in( pte_t * pPte );
int swap_out_pages( int nCount );

void register_swap_page( uint32 nAddress );
void unregister_swap_page( uint32 nAddress );

int get_swap_info( SwapInfo_s * psInfo );

#ifdef __cplusplus
}
#endif

#endif /* __F_ATHEOS_SWAP_H__ */
