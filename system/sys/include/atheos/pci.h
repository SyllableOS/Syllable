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

#ifndef _ATHEOS_PCI_H_
#define _ATHEOS_PCI_H_

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

#include <atheos/pci_vendors.h>
#include <atheos/device.h>

typedef struct
{
    int		nBus;
    int		nDevice;
    int		nFunction;
	
    uint16	nVendorID;
    uint16	nDeviceID;
    uint16	nCommand;
    uint16	nStatus;
    uint8	nRevisionID;
	uint8 	nClassApi;
	uint8	nClassBase;
	uint8	nClassSub;
    uint8	nCacheLineSize;
    uint8	nLatencyTimer;
    uint8	nHeaderType;
    uint8	nSelfTestResult;

    union
    {
	struct
	{
	    uint32	nBase0;
	    uint32	nBase1;
	    uint32	nBase2;
	    uint32	nBase3;
	    uint32	nBase4;
	    uint32	nBase5;
	    uint32	nCISPointer;
	    uint16	nSubSysVendorID;
	    uint16	nSubSysID;
	    uint32	nExpROMAddr;
	    uint8	nCapabilityList;
	    uint8	nInterruptLine;
	    uint8	nInterruptPin;
	    uint8	nMinDMATime;
	    uint8	nMaxDMALatency;
	} h0;
    } u;
    int nHandle;
} PCI_Entry_s;

typedef struct
{
  int		nBus;
  int		nDevice;
  int		nFunction;
	
  uint16	nVendorID;
  uint16	nDeviceID;
  uint8		nRevisionID;
	uint8 	nClassApi;
	uint8	nClassBase;
	uint8	nClassSub;
   uint8		nCacheLineSize;
  uint8		nHeaderType;

  union
  {
    struct
    {
      uint32	nBase0;
      uint32	nBase1;
      uint32	nBase2;
      uint32	nBase3;
      uint32	nBase4;
      uint32	nBase5;
      uint8	nInterruptLine;
      uint8	nInterruptPin;
      uint8	nMinDMATime;
      uint8	nMaxDMALatency;
    } h0;
  } u;
  int		nHandle;
} PCI_Info_s;

/* PCI bus */

#define PCI_BUS_NAME "pci"
#define PCI_BUS_VERSION 1

typedef struct
{
	status_t (*get_pci_info) ( PCI_Info_s* psInfo, int nIndex );
	uint32 (*read_pci_config)( int nBusNum, int nDevNum, int nFncNum, int nOffset, int nSize );
	status_t (*write_pci_config)( int nBusNum, int nDevNum, int nFncNum, int nOffset, 
									int nSize, uint32 nValue );
	void (*enable_pci_master)( int nBusNum, int nDevNum, int nFncNum );
	void (*set_pci_latency)( int nBusNum, int nDevNum, int nFncNum, uint8 nLatency );
	
} PCI_bus_s;

#define PCI_VENDOR_ID	0x00		/* (2 byte) vendor id */
#define PCI_DEVICE_ID	0x02		/* (2 byte) device id */
#define PCI_COMMAND	0x04		/* (2 byte) command */
#define PCI_STATUS	0x06		/* (2 byte) status */
#define PCI_REVISION	0x08		/* (1 byte) revision id */
#define PCI_CLASS_API	0x09		/* (1 byte) specific register interface type */
#define PCI_CLASS_SUB	0x0a		/* (1 byte) specific device function */
#define PCI_CLASS_BASE	0x0b		/* (1 byte) device type (display vs network, etc) */
#define PCI_LINE_SIZE	0x0c		/* (1 byte) cache line size in 32 bit words */
#define PCI_LATENCY	0x0d		/* (1 byte) latency timer */
#define PCI_HEADER_TYPE	0x0e		/* (1 byte) header type */
#define PCI_BIST	0x0f		/* (1 byte) built-in self-test */


	/***	 offsets in PCI configuration space to the elements of header type 0x00	***/

#define PCI_BASE_REGISTERS	0x10	/* base registers (size varies) */
#define PCI_CARDBUS_CIS		0x28	/* (4 bytes) CardBus CIS (Card Information Structure) pointer (see PCMCIA v2.10 Spc) */

	/*** NOTE : The next two registers are swapped in ralf-brown's interrupt list!!! Dont know what is right	***/
	
#define PCI_SUBSYSTEM_ID				0x2c	/* (2 bytes) subsystem (add-in card) id */
#define PCI_SUBSYSTEM_VENDOR_ID	0x2e	/* (2 bytes) subsystem (add-in card) vendor id */


	
#define PCI_ROM_BASE			0x30	/* (4 bytes) expansion rom base address */

#define	PCI_CAPABILITY_LIST		0x34	/* (1 byte) Offset of capability list within configuration space (R/O)
						   (Only valid if bit 4 of status register is set) */
	
#define PCI_INTERRUPT_LINE		0x3c	/* (1 byte) interrupt line */
#define PCI_INTERRUPT_PIN		0x3d	/* (1 byte) interrupt pin */
#define PCI_MIN_GRANT			0x3e	/* (1 byte) burst period @ 33 Mhz */
#define PCI_MAX_LATENCY			0x3f	/* (1 byte) how often PCI access needed */


	/*** values for the class_base field in the common header ***/

#define PCI_EARLY			0x00	/* built before class codes defined */
#define PCI_MASS_STORAGE		0x01	/* mass storage_controller */
#define PCI_NETWORK			0x02	/* network controller */
#define PCI_DISPLAY			0x03	/* display controller */
#define PCI_MULTIMEDIA			0x04	/* multimedia device */
#define PCI_MEMORY			0x05	/* memory controller */
#define PCI_BRIDGE			0x06	/* bridge controller */
#define PCI_SIMPLE_COMMUNICATIONS	0x07	/* simple communications controller */
#define PCI_BASE_PERIPHERAL		0x08	/* base system peripherals */
#define PCI_INPUT			0x09	/* input devices */
#define PCI_DOCKING_STATION		0x0a	/* docking stations */
#define PCI_PROCESSOR			0x0b	/* processors */
#define PCI_SERIAL_BUS			0x0c	/* serial_bus_controller */

#define PCI_UNDEFINED							0xFF	/* not in any defined class */


	/* values for the class_sub field for class_base = 0x00 (built before
	 * class codes were defined)
	 */

#define PCI_EARLY_NOT_VGA	0x00			/* all except vga */
#define PCI_EARLY_VGA		0x01			/* vga devices */


	/***values for the class_sub field for class_base = 0x01 (mass storage) ***/

#define PCI_SCSI		0x00			/* SCSI controller */
#define PCI_IDE			0x01			/* IDE controller */
#define PCI_FLOPPY		0x02			/* floppy disk controller */
#define PCI_IPI			0x03			/* IPI bus controller */
#define PCI_RAID		0x03			/* RAID controller */
#define PCI_MASS_STORAGE_OTHER	0x80			/* other mass storage controller */


	/*** values for the class_sub field for class_base = 0x02 (network) ***/

#define PCI_ETHERNET		0x00			/* Ethernet controller */
#define PCI_TOKEN_RING		0x01			/* Token Ring controller */
#define PCI_FDDI		0x02			/* FDDI controller */
#define PCI_ATM			0x03			/* ATM controller */
#define PCI_NETWORK_OTHER	0x80			/* other network controller */


	/*** values for the class_sub field for class_base = 0x03 (display) ***/
	
#define PCI_VGA			0x00			/* VGA controller */
#define PCI_XGA			0x01			/* XGA controller */
#define PCI_DISPLAY_OTHER	0x80			/* other display controller */


/*** values for the class_sub field for class_base = 0x04 (multimedia device) ***/

#define PCI_VIDEO		0x00			/* video */
#define PCI_AUDIO		0x01			/* audio */
#define PCI_MULTIMEDIA_OTHER	0x80			/* other multimedia device */


	/*** values for the class_sub field for class_base = 0x05 (memory) ***/

#define PCI_RAM			0x00			/* RAM */
#define PCI_FLASH		0x01			/* flash */
#define PCI_MEMORY_OTHER	0x80			/* other memory controller */


	/*** values for the class_sub field for class_base = 0x06 (bridge) ***/

#define PCI_HOST		0x00			/* host bridge */
#define PCI_ISA			0x01			/* ISA bridge */
#define PCI_EISA		0x02			/* EISA bridge */
#define PCI_MICROCHANNEL	0x03			/* MicroChannel bridge */
#define PCI_PCI			0x04			/* PCI-to-PCI bridge */
#define PCI_PCMCIA		0x05			/* PCMCIA bridge */
#define PCI_NUBUS		0x06			/* NuBus bridge */
#define PCI_CARDBUS		0x07			/* CardBus bridge */
#define PCI_BRIDGE_OTHER	0x80			/* other bridge device */


	/* values for the class_sub field for class_base = 0x07 (simple
	 *communications controllers)
	 */

#define PCI_SERIAL			0x00	/* serial port controller */
#define PCI_PARALLEL			0x01	/* parallel port */
#define PCI_SIMPLE_COMMUNICATIONS_OTHER	0x80	/* other communications device */

	/* 
	 * values of the class_api field for
	 * class_base	= 0x07 (simple communications), and
	 * class_sub	= 0x00 (serial port controller)
	 */

#define PCI_SERIAL_XT		0x00			/* XT-compatible serial controller */
#define PCI_SERIAL_16450	0x01			/* 16450-compatible serial controller */
#define PCI_SERIAL_16550	0x02			/* 16550-compatible serial controller */


	/* values of the class_api field for
	 * class_base	= 0x07 (simple communications), and
	 * class_sub	= 0x01 (parallel port)
	 */

#define PCI_PARALLEL_SIMPLE		0x00	/* simple (output-only) parallel port */
#define PCI_PARALLEL_BIDIRECTIONAL	0x01	/* bidirectional parallel port */
#define PCI_PARALLEL_ECP		0x02	/* ECP 1.x compliant parallel port */


	/* values for the class_sub field for class_base = 0x08 (generic
	 * system peripherals)
	 */

#define PCI_PIC				0x00	/* periperal interrupt controller */
#define PCI_DMA				0x01	/* dma controller */
#define PCI_TIMER			0x02	/* timers */
#define PCI_RTC				0x03	/* real time clock */
#define PCI_SYSTEM_PERIPHERAL_OTHER	0x80	/* other generic system peripheral */

/* ---
	 values of the class_api field for
	 class_base	= 0x08 (generic system peripherals)
	 class_sub	= 0x00 (peripheral interrupt controller)
	 --- */

#define PCI_PIC_8259			0x00	/* generic 8259 */
#define PCI_PIC_ISA			0x01	/* ISA pic */
#define PCI_PIC_EISA			0x02	/* EISA pic */

/* ---
	 values of the class_api field for
	 class_base	= 0x08 (generic system peripherals)
	 class_sub	= 0x01 (dma controller)
	 --- */

#define PCI_DMA_8237			0x00	/* generic 8237 */
#define PCI_DMA_ISA			0x01	/* ISA dma */
#define PCI_DMA_EISA			0x02	/* EISA dma */

/*	values of the class_api field for
 *		class_base	= 0x08 (generic system peripherals)
 *		class_sub	= 0x02 (timer)
 */

#define PCI_TIMER_8254	0x00	/* generic 8254 */
#define PCI_TIMER_ISA	0x01	/* ISA timer */
#define PCI_TIMER_EISA	0x02	/* EISA timers (2 timers) */


	/*
	 *	values of the class_api field for
	 *		class_base	= 0x08 (generic system peripherals)
	 *		class_sub		= 0x03 (real time clock
	 */

#define PCI_RTC_GENERIC	0x00	/* generic real time clock */
#define PCI_RTC_ISA	0x01	/* ISA real time clock */


	/***	values for the class_sub field for class_base = 0x09 (input devices) ***/

#define PCI_KEYBOARD	0x00	/* keyboard controller */
#define PCI_PEN		0x01	/* pen */
#define PCI_MOUSE	0x02	/* mouse controller */
#define PCI_INPUT_OTHER	0x80	/* other input controller */


	/***	values for the class_sub field for class_base = 0x0a (docking stations) ***/

#define PCI_DOCKING_GENERIC		0x00	/* generic docking station */

	/***	values for the class_sub field for class_base = 0x0b (processor) ***/

#define PCI_386		0x00	/* 386 */
#define PCI_486		0x01	/* 486 */
#define PCI_PENTIUM	0x02	/* Pentium */
#define PCI_ALPHA	0x10	/* Alpha */
#define PCI_POWERPC	0x20	/* PowerPC */
#define PCI_COPROCESSOR	0x40	/* co-processor */

	/***	values for the class_sub field for class_base = 0x0c (serial bus controller) ***/

#define PCI_FIREWIRE	0x00	/* FireWire (IEEE 1394) */
#define PCI_ACCESS	0x01	/* ACCESS bus */
#define PCI_SSA		0x02	/* SSA */
#define PCI_USB		0x03	/* Universal Serial Bus */
#define PCI_FIBRE_CHANNEL		0x04	/* Fibre channel */


	/*** masks for command register bits ***/

#define PCI_COMMAND_IO			0x001		/* 1/0 i/o space en/disabled */
#define PCI_COMMAND_MEMORY		0x002		/* 1/0 memory space en/disabled */
#define PCI_COMMAND_MASTER		0x004		/* 1/0 pci master en/disabled */
#define PCI_COMMAND_SPECIAL		0x008		/* 1/0 pci special cycles en/disabled */
#define PCI_COMMAND_MWI			0x010		/* 1/0 memory write & invalidate en/disabled */
#define PCI_COMMAND_VGA_SNOOP		0x020		/* 1/0 vga pallette snoop en/disabled */
#define PCI_COMMAND_PARITY		0x040		/* 1/0 parity check en/disabled */
#define PCI_COMMAND_ADDRESS_STEP	0x080		/* 1/0 address stepping en/disabled */
#define PCI_COMMAND_SERR		0x100		/* 1/0 SERR# en/disabled */
#define PCI_COMMAND_FASTBACK		0x200		/* 1/0 fast back-to-back en/disabled */

/***	masks for status register bits ***/

#define PCI_STATUS_66_MHZ_CAPABLE		0x0020	/* 66 Mhz capable */
#define PCI_STATUS_UDF_SUPPORTED		0x0040	/* user-definable-features (udf) supported */
#define PCI_STATUS_FASTBACK			0x0080	/* fast back-to-back capable */
#define PCI_STATUS_PARITY_SIGNALLED		0x0100	/* parity error signalled */
#define PCI_STATUS_DEVSEL			0x0600	/* devsel timing (see below) */
#define PCI_STATUS_TARGET_ABORT_SIGNALLED	0x0800	/* signaled a target abort */
#define PCI_STATUS_TARGET_ABORT_RECEIVED	0x1000	/* received a target abort */
#define PCI_STATUS_MASTER_ABORT_RECEIVED	0x2000	/* received a master abort */
#define PCI_STATUS_SERR_SIGNALLED		0x4000	/* signalled SERR# */
#define PCI_STATUS_PARITY_ERROR_DETECTED	0x8000	/* parity error detected */


/***	masks for devsel field in status register ***/

#define PCI_STATUS_DEVSEL_FAST		0x0000		/* fast */
#define PCI_STATUS_DEVSEL_MEDIUM	0x0200		/* medium */
#define PCI_STATUS_DEVSEL_SLOW		0x0400		/* slow */


/***	masks for header type register ***/

#define PCI_HEADER_TYPE_MASK	0x7F		/* header type field */
#define PCI_HEADER_BRIDGE	0x01		/* pci bridge */
#define PCI_MULTIFUNCTION	0x80		/* multifunction device flag */

/***   masks for pci bridges ( header type 1 ) ***/
#define PCI_BUS_PRIMARY		0x18
#define PCI_BUS_SECONDARY	0x19
#define PCI_BUS_SUBORDINATE 0x1a


/***	masks for built in self test (bist) register bits ***/

#define PCI_BIST_CODE		0x0F		/* self-test completion code, 0 = success */
#define PCI_BIST_START		0x40		/* 1 = start self-test */
#define PCI_BIST_CAPABLE	0x80		/* 1 = self-test capable */


/***	masks for flags in the various base address registers ***/

#define PCI_ADDRESS_SPACE		0x01		/* 0 = memory space, 1 = i/o space */


/***	masks for flags in memory space base address registers ***/

#define PCI_ADDRESS_TYPE		0x06	/* type (see below) */
#define PCI_ADDRESS_PREFETCHABLE	0x08	/* 1 if prefetchable (see PCI spec) */

#define PCI_ADDRESS_TYPE_32		0x00	/* locate anywhere in 32 bit space */
#define PCI_ADDRESS_TYPE_32_LOW		0x01	/* locate below 1 Meg */
#define PCI_ADDRESS_TYPE_64		0x02	/* locate anywhere in 64 bit space */

#define PCI_ADDRESS_MEMORY_32_MASK	0xFFFFFFF0	/* mask to get 32bit memory space base address */


/***	masks for flags in i/o space base address registers ***/

#define PCI_ADDRESS_IO_MASK	0xFFFFFFFC	/* mask to get i/o space base address */


/***	masks for flags in expansion rom base address registers ***/

#define PCI_ROM_ENABLE		0x00000001	/* 1 = expansion rom decode enabled */
#define PCI_ROM_ADDRESS_MASK	0xFFFFF800	/* mask to get expansion rom addr */

#ifndef __KERNEL__

status_t get_pci_info( PCI_Info_s* psInfo, int nIndex );

uint32 	 read_pci_config( int nBusNum, int nDevNum, int nFncNum, int nOffset, int nSize );
status_t write_pci_config( int nBusNum, int nDevNum, int nFncNum, int nOffset, int nSize, uint32 nValue );

status_t raw_read_pci_config( int nBusNum, int nDevFnc, int nOffset, int nSize, uint32 *pnRes );
status_t raw_write_pci_config( int nBusNum, int nDevFnc, int nOffset, int nSize, uint32 nValue );

#endif

#ifdef __cplusplus
}
#endif

#endif /* _ATHEOS_PCI_H_ */
