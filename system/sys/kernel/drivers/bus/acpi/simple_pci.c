/*
 *  The Syllable kernel
 *  Simple PCI layer for the ACPI busmanager
 *  Copyright (C) 2003 Arno Klenke
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  
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
#include <atheos/device.h>
#include <atheos/kernel.h>
#include <atheos/isa_io.h>
#include <atheos/types.h>
#include <atheos/pci.h>
#include <atheos/kernel.h>
#include <atheos/spinlock.h>


#include <macros.h>


enum
{
	PCI_METHOD_1 = 0x01,
	PCI_METHOD_2 = 0x02,
	PCI_METHOD_BIOS = 0x04
};

uint32 g_nPCIMethod;
SpinLock_s g_sPCILock = INIT_SPIN_LOCK( "simple_pci_lock" );

/** 
 * \par Description: Checks wether a PCI bus is present and selects the access ethod
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void simple_pci_init( void )
{
	g_nPCIMethod = 0;
	struct RMREGS rm;

	/* Do simple register checks to find the way how to access the pci bus */

	/* Check PCI method 2 */
	outb( 0x0, 0x0cf8 );
	outb( 0x0, 0x0cfa );
	if ( inb( 0x0cf8 ) == 0x0 && inb( 0x0cfa ) == 0x0 )
	{
		g_nPCIMethod = PCI_METHOD_2;
		kerndbg( KERN_DEBUG, "PCI: Using access method 2\n" );
		return;
	}

	/* Check PCI method 1 */
	outl( 0x80000000, 0x0cf8 );
	if ( inl( 0x0cf8 ) == 0x80000000 )
	{
		outl( 0x0, 0x0cf8 );
		if ( inl( 0x0cf8 ) == 0x0 )
		{
			g_nPCIMethod = PCI_METHOD_1;
			kerndbg( KERN_DEBUG, "PCI: Using access method 1\n" );
			return;
		}
	}

	/* Check for Virtual PC PCI method 1 */
	outl( 0x80000000, 0x0cf8 );
	if ( inl( 0x0cf8 ) == 0x80000000 )
	{
		outl( 0x0, 0x0cf8 );
		if ( inl( 0x0cf8 ) == 0x80000000 )
		{
			g_nPCIMethod = PCI_METHOD_1;
			kerndbg( KERN_DEBUG, "PCI: Detected Virtual PC, using access method 1\n" );
			return;
		}
	}
	/* Check for PCI BIOS */
	memset( &rm, 0, sizeof( rm ) );
	rm.EAX = 0xb101;
	realint( 0x1a, &rm );
	
	if ( ( rm.flags & 0x01 ) == 0 && ( rm.EAX & 0xff00 ) == 0 )
	{
		g_nPCIMethod = PCI_METHOD_BIOS;
		kerndbg( KERN_INFO, "PCI: PCIBIOS Version %ld.%ld detected\n", rm.EBX >> 8, rm.EBX & 0xff );
		return;
	}
}

/** 
 * \par Description: Reads 1/2/4 byte values from a device.
 * \par Note:
 * \par Warning:
 * \param nBusNum - Number of the bus.
 * \param nDevNum - Number of the device.
 * \param nFncNum - Number of the function.
 * \param nOffset - Offset in the PCI configuration space.
 * \param nSize   - 1, 2 or 4.
 * \return The value.
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/

uint32 simple_pci_read_config( int nBusNum, int nDevNum, int nFncNum, int nOffset, int nSize )
{
	uint32 nFlags;
	uint32 nValue = 0;

	if ( 2 == nSize || 4 == nSize || 1 == nSize )
	{
		if ( g_nPCIMethod & PCI_METHOD_1 )
		{
			spinlock_cli( &g_sPCILock, nFlags );
			outl( 0x80000000 | ( nBusNum << 16 ) | ( nDevNum << 11 ) | ( nFncNum << 8 ) | ( nOffset & ~3 ), 0x0cf8 );
			switch ( nSize )
			{
			case 1:
				nValue = inb( 0x0cfc + ( nOffset & 3 ) );
				break;
			case 2:
				nValue = inw( 0x0cfc + ( nOffset & 2 ) );
				break;
			case 4:
				nValue = inl( 0x0cfc );
				break;
			default:
				break;
			}
			spinunlock_restore( &g_sPCILock, nFlags );
			return ( nValue );
		}
		else if ( g_nPCIMethod & PCI_METHOD_2 )
		{
			if ( nDevNum >= 16 )
			{
				kerndbg( KERN_WARNING, "PCI: read_pci_config() with an invalid device number\n" );
			}
			spinlock_cli( &g_sPCILock, nFlags );
			outb( ( 0xf0 | ( nFncNum << 1 ) ), 0x0cf8 );
			outb( nBusNum, 0x0cfa );
			switch ( nSize )
			{
			case 1:
				nValue = inb( ( 0xc000 | ( nDevNum << 8 ) | nOffset ) );
				break;
			case 2:
				nValue = inw( ( 0xc000 | ( nDevNum << 8 ) | nOffset ) );
				break;
			case 4:
				nValue = inl( ( 0xc000 | ( nDevNum << 8 ) | nOffset ) );
				break;
			default:
				break;
			}
			outb( 0x0, 0x0cf8 );
			spinunlock_restore( &g_sPCILock, nFlags );

			return ( nValue );
		}
		else if ( g_nPCIMethod & PCI_METHOD_BIOS )
		{
			struct RMREGS rm;
			int	anCmd[] = { 0xb108, 0xb109, 0x000, 0xb10a };
			uint32 anMasks[] = { 0x000000ff, 0x0000ffff, 0x00000000, 0xffffffff };
			memset( &rm, 0, sizeof( rm ) );

			rm.EAX = anCmd[nSize - 1];
			rm.EBX = (nBusNum << 8) | (((nDevNum << 3) | nFncNum));
			rm.EDI = nOffset;
		
			realint( 0x1a, &rm );

			if ( 0 == ( ( rm.EAX >> 8 ) & 0xff ) ) {
				return( rm.ECX & anMasks[ nSize- 1 ] );
			} else {
				return( anMasks[ nSize- 1 ] );
			}
		}
		else
		{
			kerndbg( KERN_WARNING, "PCI: read_pci_config() called without PCI present\n" );
		}
	}
	else
	{
		kerndbg( KERN_WARNING, "PCI: Invalid size %d passed to read_pci_config()\n", nSize );
	}
	return ( 0 );
}


/** 
 * \par Description: Writes 1/2/4 byte values to a device.
 * \par Note:
 * \par Warning:
 * \param nBusNum - Number of the bus.
 * \param nDevNum - Number of the device.
 * \param nFncNum - Number of the function.
 * \param nOffset - Offset in the PCI configuration space.
 * \param nSize   - 1, 2 or 4.
 * \param nValue  - Value which is written.
 * \return 0 if successful.
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/

status_t simple_pci_write_config( int nBusNum, int nDevNum, int nFncNum, int nOffset, int nSize, uint32 nValue )
{
	uint32 nFlags;

	if ( 2 == nSize || 4 == nSize || 1 == nSize )
	{
		if ( g_nPCIMethod & PCI_METHOD_1 )
		{
			spinlock_cli( &g_sPCILock, nFlags );
			outl( 0x80000000 | ( nBusNum << 16 ) | ( nDevNum << 11 ) | ( nFncNum << 8 ) | ( nOffset & ~3 ), 0x0cf8 );
			switch ( nSize )
			{
			case 1:
				outb( nValue, 0x0cfc + ( nOffset & 3 ) );
				break;
			case 2:
				outw( nValue, 0x0cfc + ( nOffset & 2 ) );
				break;
			case 4:
				outl( nValue, 0x0cfc );
				break;
			default:
				spinunlock_restore( &g_sPCILock, nFlags );
				return ( -1 );
			}
			spinunlock_restore( &g_sPCILock, nFlags );
			return ( 0 );
		}
		else if ( g_nPCIMethod & PCI_METHOD_2 )
		{
			if ( nDevNum >= 16 )
			{
				kerndbg( KERN_WARNING, "PCI: write_pci_config() with an invalid device number\n" );
			}
			spinlock_cli( &g_sPCILock, nFlags );
			outb( ( 0xf0 | ( nFncNum << 1 ) ), 0x0cf8 );
			outb( nBusNum, 0x0cfa );
			switch ( nSize )
			{
			case 1:
				outb( nValue, ( 0xc000 | ( nDevNum << 8 ) | nOffset ) );
				break;
			case 2:
				outw( nValue, ( 0xc000 | ( nDevNum << 8 ) | nOffset ) );
				break;
			case 4:
				outl( nValue, ( 0xc000 | ( nDevNum << 8 ) | nOffset ) );
				break;
			default:
				spinunlock_restore( &g_sPCILock, nFlags );
				return ( -1 );
			}
			outb( 0x0, 0x0cf8 );
			spinunlock_restore( &g_sPCILock, nFlags );
			return ( 0 );
		}
		else if ( g_nPCIMethod & PCI_METHOD_BIOS )
		{
			struct RMREGS rm;
			int	anCmd[] = { 0xb10b, 0xb10c, 0x000, 0xb10d };
			uint32 anMasks[] = { 0x000000ff, 0x0000ffff, 0x00000000, 0xffffffff };
			memset( &rm, 0, sizeof( rm ) );

			rm.EAX = anCmd[nSize - 1];
			rm.EBX = (nBusNum << 8) | (((nDevNum << 3) | nFncNum));
			rm.EDI = nOffset;
			rm.ECX = nValue & anMasks[ nSize - 1 ];

			realint( 0x1a, &rm );

			return( ( rm.flags & 0x01 ) ? -1 : 0 );
		}
		else
		{
			kerndbg( KERN_WARNING, "PCI: write_pci_config() called without PCI present\n" );
		}
	}
	else
	{
		kerndbg( KERN_WARNING, "PCI: Invalid size %d passed to write_pci_config()\n", nSize );
	}
	return ( -1 );
}











