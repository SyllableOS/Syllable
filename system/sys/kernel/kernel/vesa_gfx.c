
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


#include <posix/errno.h>
#include <atheos/kernel.h>
#include <atheos/vesa_gfx.h>

int sys_get_vesa_mode_info( VESA_Mode_Info_s * psVesaModeInfo, uint32 nModeNr )
{
	struct RMREGS rm;

	VESA_Mode_Info_s *psTmp;

	psTmp = alloc_real( sizeof( VESA_Mode_Info_s ), MEMF_CLEAR );
	if ( psTmp == NULL )
	{
		printk( "Error: get_vesa_mode_info() out of real-memory\n" );
		return ( -ENOMEM );
	}
	memset( &rm, 0, sizeof( struct RMREGS ) );

	rm.EAX = 0x4f01;
	rm.ECX = nModeNr;
	rm.EDI = ( ( uint32 )psTmp ) & 0x0f;
	rm.ES = ( ( uint32 )psTmp ) >> 4;

	realint( 0x10, &rm );

	memcpy( psVesaModeInfo, psTmp, sizeof( VESA_Mode_Info_s ) );
	//*psVesaModeInfo = *psTmp;

	free_real( psTmp );

	if ( ( rm.EAX & 0xff ) != 0x4f )
	{
		printk( "Error: sys_get_vesa_mode_info() function 0x4f01 not supported by BIOS\n" );
		return ( -ENOSYS );
	}
	if ( ( ( rm.EAX >> 8 ) & 0xff ) != 0 )
	{
		printk( "Error: sys_get_vesa_mode_info() function 0x4f01 failed with error code %d\n", ( ( rm.EAX >> 8 ) & 0xff ) );
		return ( -EINVAL );
	}
	return ( 0 );
}


int get_vesa_mode_info( VESA_Mode_Info_s * psVesaModeInfo, uint32 nModeNr )
{
	return ( sys_get_vesa_mode_info( psVesaModeInfo, nModeNr ) );
}

/******************************************************************************
 ******************************************************************************/

int sys_get_vesa_info( Vesa_Info_s * psVesaInfo, uint16 *pnModeList, int nMaxModeCount )
{
	struct RMREGS rm;
	Vesa_Info_s *psTmp;
	uint16 *pnTmpList;
	int i;

	memset( &rm, 0, sizeof( struct RMREGS ) );

	psTmp = alloc_real( sizeof( Vesa_Info_s ), 0 );
	if ( psTmp == NULL )
	{
		printk( "Error: get_vesa_info() out of real-memory\n" );
		return ( -ENOMEM );
	}
	memcpy( psTmp, psVesaInfo, sizeof( Vesa_Info_s ) );
	//*psTmp = *psVesaInfo;

	rm.EAX = 0x4f00;
	rm.ECX = 0;
	rm.EDI = ( ( uint32 )psTmp ) & 0x0f;
	rm.ES = ( ( uint32 )psTmp ) >> 4;

	realint( 0x10, &rm );

	memcpy( psVesaInfo, psTmp, sizeof( Vesa_Info_s ) );
	//*psVesaInfo = *psTmp;

	pnTmpList = ( uint16 * )( ( ( ( ( uint32 )psTmp->VideoModePtr ) & 0xffff0000 ) >> 12 ) + ( ( ( uint32 )psTmp->VideoModePtr ) & 0x0ffff ) );

	for ( i = 0; pnTmpList[i] != 0xffff && i < nMaxModeCount; ++i )
	{
		pnModeList[i] = pnTmpList[i];
	}

	free_real( psTmp );

	return ( i );
}

int get_vesa_info( Vesa_Info_s * psVesaInfo, uint16 *pnModeList, int nMaxModeCount )
{
	return ( sys_get_vesa_info( psVesaInfo, pnModeList, nMaxModeCount ) );
}
