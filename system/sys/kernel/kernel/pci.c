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
#include <atheos/types.h>
#include <atheos/pci.h>
#include <atheos/kernel.h>

#include <macros.h>

#define	MAX_PCI_DEVICES	255

static int 	    g_nNumPCIDevices = 0;
static PCI_Entry_s* g_apsPCIDevices[ MAX_PCI_DEVICES ];

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void PCI_PrintInfo( PCI_Entry_s* psInfo )
{
  printk( "----------------------------\n" );
	
  printk( "Vendor          = %04x\n", psInfo->nVendorID );
  printk( "Device          = %04x\n", psInfo->nDeviceID );
/*
  printk( "Command         = %04x\n", psInfo->nCommand );
  printk( "nStatus         = %08x\n", psInfo->nStatus );
  */	
  printk( "Revsion         = %01x\n", psInfo->nRevisionID );
/*
  printk( "CacheLineSize   = %d\n", psInfo->nCacheLineSize );
  printk( "LatencyTimer    = %d\n", psInfo->nLatencyTimer );
  printk( "HeaderType      = %x\n", psInfo->nHeaderType );
*/
	
  printk( "Base0           = %08lx\n", psInfo->u.h0.nBase0 );
  printk( "Base1           = %08lx\n", psInfo->u.h0.nBase1 );
  printk( "Base2           = %08lx\n", psInfo->u.h0.nBase2 );
  printk( "Base3           = %08lx\n", psInfo->u.h0.nBase3 );
  printk( "Base4           = %08lx\n", psInfo->u.h0.nBase4 );
  printk( "Base5           = %08lx\n", psInfo->u.h0.nBase5 );
/*
  printk( "CISPointer      = %08x\n", psInfo->u.h0.nCISPointer );
  printk( "SubSystemVendor = %04x\n", psInfo->u.h0.nSubSysVendorID );
  printk( "SubSystem       = %04x\n", psInfo->u.h0.nSubSysID );
  printk( "RomBase         = %08x\n", psInfo->u.h0.nExpROMAddr );
  printk( "CapabilityList  = %01x\n", psInfo->u.h0.nCapabilityList );
*/	
  printk( "InterruptLine   = %01x\n", psInfo->u.h0.nInterruptLine );
  printk( "InterruptPin    = %01x\n", psInfo->u.h0.nInterruptPin );
/*
  printk( "Min DMA time    = %01x\n", psInfo->u.h0.nMinDMATime );
  printk( "Max DMA latency = %01x\n", psInfo->u.h0.nMaxDMALatency );
*/
  printk( "\n" );
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

static int pci_inst_check( void )
{
  struct RMREGS rm;

  memset( &rm, 0, sizeof( rm ) );

  rm.EAX = 0xb101;

  realint( 0x1a, &rm );
	
  if ( (rm.flags & 0x01) == 0 && (rm.EAX & 0xff00) == 0 )
  {
/*		printk( "ID              = %0x (%4s)\n", rm.EDX, &rm.EDX ); */
    printk( "PCI Characteristics = %lx\n", rm.EAX & 0xff );
    printk( "PCI Version         = %ld.%ld\n", rm.EBX >> 8, rm.EBX & 0xff );
    printk( "Last PCI bus        = %ld\n", rm.ECX & 0xff );
		
    return( (rm.ECX & 0xff) + 1 );
  }
  else
  {
    printk( "PCI installation check failed\n" );
    return( -1 );
  }
	
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

uint32 read_pci_config( int nBusNum, int nDevNum, int nFncNum, int nOffset, int nSize )
{
  struct RMREGS rm;

  if ( 2 == nSize || 4 == nSize || 1 == nSize )
  {
    int	anCmd[] = { 0xb108, 0xb109, 0x000, 0xb10a };
    uint32	anMasks[] = { 0x000000ff, 0x0000ffff, 0x00000000, 0xffffffff };
    memset( &rm, 0, sizeof( rm ) );

    rm.EAX = anCmd[nSize - 1];

    rm.EBX = (nBusNum << 8) | (((nDevNum << 3) | nFncNum));
    rm.EDI = nOffset;
		

    realint( 0x1a, &rm );

    if ( 0 == ((rm.EAX >> 8) & 0xff) ) {
      return( rm.ECX & anMasks[ nSize- 1 ] );
    } else {
      return( anMasks[ nSize- 1 ] );
    }
  }
  else
  {
    printk( "ERROR : Invalid size %d passed to read_pci_config()\n", nSize );
    return( 0 );
  }
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t sys_raw_read_pci_config( int nBusNum, int nDevFnc, int nOffset, int nSize, uint32* pnRes )
{
  *pnRes = read_pci_config( nBusNum, nDevFnc >> 16, nDevFnc & 0xffff, nOffset, nSize );
  return( 0 );
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t write_pci_config( int nBusNum, int nDevNum, int nFncNum, int nOffset, int nSize, uint32 nValue )
{
  struct RMREGS rm;

  if ( 2 == nSize || 4 == nSize || 1 == nSize )
  {
    int	anCmd[] = { 0xb10b, 0xb10c, 0x000, 0xb10d };
    uint32	anMasks[] = { 0x000000ff, 0x0000ffff, 0x00000000, 0xffffffff };
    memset( &rm, 0, sizeof( rm ) );

    rm.EAX = anCmd[nSize - 1];

    rm.EBX = (nBusNum << 8) | (((nDevNum << 3) | nFncNum));
    rm.EDI = nOffset;
    rm.ECX = nValue & anMasks[ nSize - 1 ];

    realint( 0x1a, &rm );

    return(  (rm.flags & 0x01) ? -1 : 0 );
  }
  else
  {
    printk( "ERROR : Invalid size %d passed to write_pci_config()\n", nSize );
    return( -1 );
  }
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t sys_raw_write_pci_config( int nBusNum, int nDevFnc, int nOffset, int nSize, uint32 nValue )
{
  return( write_pci_config( nBusNum, nDevFnc >> 16, nDevFnc & 0xffff, nOffset, nSize, nValue ) );
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

static int read_pci_header( PCI_Entry_s* psInfo, int nBusNum, int nDevNum, int nFncNum )
{
  psInfo->nBus		= nBusNum;
  psInfo->nDevice	= nDevNum;
  psInfo->nFunction	= nFncNum;

  psInfo->nVendorID 	= read_pci_config( nBusNum, nDevNum, nFncNum, PCI_VENDOR_ID, 2 );
  psInfo->nDeviceID 	= read_pci_config( nBusNum, nDevNum, nFncNum, PCI_DEVICE_ID, 2 );
  psInfo->nCommand 	= read_pci_config( nBusNum, nDevNum, nFncNum, PCI_COMMAND, 2 );
  psInfo->nStatus 	= read_pci_config( nBusNum, nDevNum, nFncNum, PCI_STATUS, 2 );
  psInfo->nRevisionID	= read_pci_config( nBusNum, nDevNum, nFncNum, PCI_REVISION, 1 );

  psInfo->nClassApi		= read_pci_config( nBusNum, nDevNum, nFncNum, PCI_CLASS_API, 1);
  psInfo->nClassBase	= read_pci_config( nBusNum, nDevNum, nFncNum, PCI_CLASS_BASE, 1);
  psInfo->nClassSub		= read_pci_config( nBusNum, nDevNum, nFncNum, PCI_CLASS_SUB, 1);
	
  psInfo->nCacheLineSize= read_pci_config( nBusNum, nDevNum, nFncNum, PCI_LINE_SIZE, 1 );
  psInfo->nLatencyTimer = read_pci_config( nBusNum, nDevNum, nFncNum, PCI_LATENCY, 1 );
  psInfo->nHeaderType 	= read_pci_config( nBusNum, nDevNum, nFncNum, PCI_HEADER_TYPE, 1 );

	
  psInfo->u.h0.nBase0	= read_pci_config( nBusNum, nDevNum, nFncNum, PCI_BASE_REGISTERS + 0, 4 );
  psInfo->u.h0.nBase1	= read_pci_config( nBusNum, nDevNum, nFncNum, PCI_BASE_REGISTERS + 4, 4 );
  psInfo->u.h0.nBase2	= read_pci_config( nBusNum, nDevNum, nFncNum, PCI_BASE_REGISTERS + 8, 4 );
  psInfo->u.h0.nBase3	= read_pci_config( nBusNum, nDevNum, nFncNum, PCI_BASE_REGISTERS + 12, 4 );
  psInfo->u.h0.nBase4	= read_pci_config( nBusNum, nDevNum, nFncNum, PCI_BASE_REGISTERS + 16, 4 );
  psInfo->u.h0.nBase5	= read_pci_config( nBusNum, nDevNum, nFncNum, PCI_BASE_REGISTERS + 20, 4 );

  psInfo->u.h0.nCISPointer	= read_pci_config( nBusNum, nDevNum, nFncNum, PCI_CARDBUS_CIS, 4 );
  psInfo->u.h0.nSubSysVendorID	= read_pci_config( nBusNum, nDevNum, nFncNum, PCI_SUBSYSTEM_VENDOR_ID, 2 );
  psInfo->u.h0.nSubSysID	= read_pci_config( nBusNum, nDevNum, nFncNum, PCI_SUBSYSTEM_ID, 2 );
  psInfo->u.h0.nExpROMAddr	= read_pci_config( nBusNum, nDevNum, nFncNum, PCI_ROM_BASE, 4 );
  psInfo->u.h0.nCapabilityList	= read_pci_config( nBusNum, nDevNum, nFncNum, PCI_CAPABILITY_LIST, 1 );
  psInfo->u.h0.nInterruptLine	= read_pci_config( nBusNum, nDevNum, nFncNum, PCI_INTERRUPT_LINE, 1 );
  psInfo->u.h0.nInterruptPin	= read_pci_config( nBusNum, nDevNum, nFncNum, PCI_INTERRUPT_PIN, 1 );
  psInfo->u.h0.nMinDMATime	= read_pci_config( nBusNum, nDevNum, nFncNum, PCI_MIN_GRANT, 1 );
  psInfo->u.h0.nMaxDMALatency	= read_pci_config( nBusNum, nDevNum, nFncNum, PCI_MAX_LATENCY, 1 );
/*
  printk( "Sizes for bus: %d dev: %d fnc: %d\n", nBusNum, nDevNum, nFncNum );
  
  for ( i = 0 ; i < 5 ; ++i )
  {
    int	nSize;
    write_pci_config( nBusNum, nDevNum, nFncNum, PCI_BASE_REGISTERS + i * 4, 4, ~0 );
    nSize = read_pci_config( nBusNum, nDevNum, nFncNum, PCI_BASE_REGISTERS + i * 4, 4 );
    printk( "Size %d = %08x\n", i, nSize );
  }
  */
  return( 0 );
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t sys_get_pci_info( PCI_Info_s* psInfo, int nIndex )
{
  if ( NULL == psInfo ) {
    return( -EFAULT );
  }
  if ( nIndex >= 0 && nIndex < g_nNumPCIDevices )
  {
    PCI_Entry_s* psEntry = g_apsPCIDevices[ nIndex ];

    kassertw( NULL != psEntry );
			
    psInfo->nBus		= psEntry->nBus;
    psInfo->nDevice		= psEntry->nDevice;
    psInfo->nFunction		= psEntry->nFunction;

    psInfo->nVendorID 	= psEntry->nVendorID;
    psInfo->nDeviceID 	= psEntry->nDeviceID;
    psInfo->nRevisionID = psEntry->nRevisionID;
	
    psInfo->nCacheLineSize	= psEntry->nCacheLineSize;
    psInfo->nHeaderType		= psEntry->nHeaderType;

	psInfo->nClassApi		= psEntry->nClassApi;
	psInfo->nClassBase		= psEntry->nClassBase;
	psInfo->nClassSub		= psEntry->nClassSub;
	
    psInfo->u.h0.nBase0	= psEntry->u.h0.nBase0;
    psInfo->u.h0.nBase1	= psEntry->u.h0.nBase1;
    psInfo->u.h0.nBase2	= psEntry->u.h0.nBase2;
    psInfo->u.h0.nBase3	= psEntry->u.h0.nBase3;
    psInfo->u.h0.nBase4	= psEntry->u.h0.nBase4;
    psInfo->u.h0.nBase5	= psEntry->u.h0.nBase5;

    psInfo->u.h0.nInterruptLine		= psEntry->u.h0.nInterruptLine;
    psInfo->u.h0.nInterruptPin		= psEntry->u.h0.nInterruptPin;
    psInfo->u.h0.nMinDMATime		= psEntry->u.h0.nMinDMATime;
    psInfo->u.h0.nMaxDMALatency		= psEntry->u.h0.nMaxDMALatency;
		
    return( 0 );
  }
  else
  {
    return( -EINVAL );
  }
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t get_pci_info( PCI_Info_s* psInfo, int nIndex )
{
    return( sys_get_pci_info( psInfo, nIndex ) );
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void	init_pci_module( void )
{
  int	nBusCount = pci_inst_check();

  if ( -1 != nBusCount ) {
    int	nBus;

      //    "0000 0000 000 00000000 00000000 00000000 00000000 00000000 00000000 0    0"
    printk( "Vend Dev  Rev Base0    Base1    Base2    Base3    Base4    Base5  irq-l irq-p\n" );
    for ( nBus = 0 ; nBus < nBusCount ; ++nBus ) {
      int	nDev;
      for ( nDev = 0 ; nDev < 32 ; ++nDev ) {
	int	nHeaderType = 0;
	int	nFnc;
				
	for ( nFnc = 0 ; nFnc < 8 ; ++nFnc ) {
	  uint32	nVendorID = read_pci_config( nBus, nDev, nFnc, PCI_VENDOR_ID, 2 );

	  if ( 0xffff != nVendorID ) {
	    PCI_Entry_s* psInfo;
						
	    if ( 0 == nFnc ) {
	      nHeaderType = read_pci_config( nBus, nDev, nFnc, PCI_HEADER_TYPE, 1 );
	    } else {
	      if ( (nHeaderType & PCI_MULTIFUNCTION) == 0 ) {
		continue;
	      }
	    }
						
	    psInfo = kmalloc( sizeof( PCI_Entry_s ), MEMF_KERNEL | MEMF_CLEAR );

	    if ( NULL != psInfo ) {
	      read_pci_header( psInfo, nBus, nDev, nFnc );

	      if ( g_nNumPCIDevices < MAX_PCI_DEVICES ) {
		g_apsPCIDevices[ g_nNumPCIDevices++ ] = psInfo;

		printk( "%04x %04x %03x %08lx %08lx %08lx %08lx %08lx %08lx %01x    %01x\n",
			psInfo->nVendorID, psInfo->nDeviceID, psInfo->nRevisionID,
			psInfo->u.h0.nBase0, psInfo->u.h0.nBase1, psInfo->u.h0.nBase2,
			psInfo->u.h0.nBase3, psInfo->u.h0.nBase4, psInfo->u.h0.nBase5,
			psInfo->u.h0.nInterruptLine, psInfo->u.h0.nInterruptPin );
//		PCI_PrintInfo( psInfo );
	      } else {
		printk( "WARNING : To many PCI devices!\n" );
	      }
	    }
	  }
	}
      }
    }
    printk( "Found %d PCI devices\n", g_nNumPCIDevices );
  }
  else
  {
    printk( "Unable to read PCI information\n" );
  }
}
