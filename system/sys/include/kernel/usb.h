/*
 *  The Syllable kernel
 *	USB busmanager
 *
 *	Ported from the linux USB code:
 *	(C) Copyright Linus Torvalds 1999
 *	(C) Copyright Johannes Erdfelt 1999-2001
 *	(C) Copyright Andreas Gal 1999
 *	(C) Copyright Gregory P. Smith 1999
 *	(C) Copyright Deti Fliegl 1999 (new USB architecture)
 *	(C) Copyright Randy Dunlap 2000
 *	(C) Copyright David Brownell 2000 (kernel hotplug, usb_device_id)
 *	(C) Copyright Yggdrasil Computing, Inc. 2000
 *     (usb_device_id matching changes by Adam J. Richter)
 *  
 *  Copyright (C) 2003 Arno Klenke
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU
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

#ifndef __F_KERNEL_USB_H_
#define __F_KERNEL_USB_H_

#include <kernel/types.h>
#include <kernel/atomic.h>
#include <kernel/semaphore.h>
#include <kernel/spinlock.h>
#include <kernel/list.h>
#include <kernel/usb_defs.h>

/* One USB packet */
struct USB_device_t;
struct USB_packet_t;

typedef void (*usb_complete)( struct USB_packet_t* );


typedef struct
{
	unsigned int nOffset;
	unsigned int nLength;		// expected length
	unsigned int nActualLength;
	unsigned int nStatus;
} USB_ISO_frame_s;

struct USB_packet_t
{
	SpinLock_s		hLock;
	struct list_head lPacketList;
	struct USB_device_t* psDevice;
	void*			pHCPrivate;
	struct USB_packet_t*	psNext;
	unsigned int	nPipe;
	int				nStatus;
	unsigned int	nTransferFlags;
	uint8*			pBuffer;
	int				nBufferLength;
	int				nActualLength;
	int				nBandWidth;
	uint8*			pSetupPacket;
	int				nStartFrame;
	int				nPacketNum;
	int				nInterval;
	int				nErrorCount;
	int				nTimeOut;
	
	usb_complete	pComplete;
	void*			pCompleteData;
	sem_id			hWait;
	bool			bDone;
	USB_ISO_frame_s	sISOFrame[0];
};

typedef struct USB_packet_t USB_packet_s;

/* One USB bus */


struct USB_device_t;

struct USB_bus_driver_t
{
	int			nBusNum;
	atomic_t	nRefCount;
	bool		bDevMap[128];
	struct USB_device_t* psRootHUB;
	
	int (*AddDevice) ( struct USB_device_t* psDevice );
	int (*RemoveDevice) ( struct USB_device_t* psDevice );
	int (*GetFrameNumber) ( struct USB_device_t* psDevice );
	int (*SubmitPacket) ( struct USB_packet_t* psPacket );
	int (*CancelPacket) ( struct USB_packet_t* psPacket );
	
	void*		pHCPrivate;
	int			nBandWidthAllocated;
	int			nBandWidthInt;
	int			nBandWidthISO;
};

typedef struct USB_bus_driver_t USB_bus_driver_s;

struct USB_device_t;

struct USB_libusb_node_t
{
	struct USB_device_t* psDevice;
	int nInterfaceNum;
	int hNode;
	sem_id hLock;
	sem_id hWait;
	sem_id hOpen;
};

typedef struct USB_libusb_node_t USB_libusb_node_s;

/* One USB device */

struct USB_device_t
{
	struct USB_bus_driver_t*	psBus;
	struct USB_device_t*	psParent;
	
	
	int				nDeviceNum;
	int				eSpeed;
	atomic_t		nRefCount;
	sem_id			hLock;
	
	unsigned int	nToggle[2];
	unsigned int	nHalted[2];
	int				nMaxPacketIn[16];
	int				nMaxPacketOut[16];
	USB_desc_device_s sDeviceDesc;
	USB_desc_config_s* psConfig;
	USB_desc_config_s* psActConfig;
	char**			pRawDescs;
	bool			bLangID;
	int				nStringLangID;
	
	void*			pHCPrivate;
	
	int				nMaxChild;
	struct USB_device_t*	psChildren[16];
	
	struct USB_device_t* psTT;
	//void* psTT;
	int nTTPort;
	
	
	int				nHandle;
};

typedef struct USB_device_t USB_device_s;

struct USB_driver_t;

/* One USB driver */
struct USB_driver_t
{
	struct USB_driver_t* psNext;
	char zName[255];
	bool (*AddDevice) ( USB_device_s* psDev, unsigned nInterface, void** pPrivate );
	void (*RemoveDevice) ( USB_device_s* psDev, void* );
	
	sem_id hLock; 
};


typedef struct USB_driver_t USB_driver_s;

/* USB busmanager */

#define USB_BUS_NAME "usb"
#define USB_BUS_VERSION 1

typedef struct 
{
	status_t ( *add_driver )( USB_driver_s* psDriver );
	void ( *add_driver_resistant )( USB_driver_s* psDriver );
	void ( *remove_driver )( USB_driver_s* psDriver );
	
	void ( *connect )( USB_device_s* psDevice );
	void ( *disconnect )( USB_device_s** psDevice );
	int ( *new_device )( USB_device_s* psDevice );
	USB_device_s* ( *alloc_device )( USB_device_s* psParent, USB_bus_driver_s* psBus );
	void ( *free_device )( USB_device_s* psDevice );
	
	int ( *set_idle )( USB_device_s* psDevice, int nIFNum, int nDuration, int nReportID );
	int ( *string )( USB_device_s* psDevice, int nIndex, char* pBuf, size_t nSize );
	
	USB_bus_driver_s* ( *alloc_bus )();
	void ( *free_bus )( USB_bus_driver_s* psBus );
	void ( *add_bus )( USB_bus_driver_s* psBus );
	void ( *remove_bus )( USB_bus_driver_s* psBus );
	int ( *root_hub_string )( int id, int serial, char *type, uint8 *data, int len );
	int ( *check_bandwidth )( USB_device_s* psDevice, USB_packet_s* psPacket );
	void ( *claim_bandwidth )( USB_device_s* psDevice, USB_packet_s* psPacket, int nBusTime, int nISOc );
	void ( *release_bandwidth )( USB_device_s* psDevice, USB_packet_s* psPacket, int nISOc );

	USB_packet_s* ( *alloc_packet )( int nISOPackets );
	void ( *free_packet )( USB_packet_s* psPacket );
	status_t ( *submit_packet )( USB_packet_s* psPacket );
	status_t ( *cancel_packet )( USB_packet_s* psPacket );
	int ( *control_msg )( USB_device_s* psDevice, unsigned int nPipe, uint8 nRequest, 
					uint8 nRequestType, uint16 nValue, uint16 nIndex, void* pData, uint16 nSize,
					bigtime_t nTimeOut );
					
	int ( *set_interface )( USB_device_s* psDevice, int nIFNum, int nAlternate );
	int ( *set_configuration )( USB_device_s* psDevice, int nConfig );
} USB_bus_s;


/* USB pipe stuff */

/*
 * Calling this entity a "pipe" is glorifying it. A USB pipe
 * is something embarrassingly simple: it basically consists
 * of the following information:
 *  - device number (7 bits)
 *  - endpoint number (4 bits)
 *  - current Data0/1 state (1 bit)
 *  - direction (1 bit)
 *  - speed (1 bit)
 *  - max packet size (2 bits: 8, 16, 32 or 64) [Historical; now gone.]
 *  - pipe type (2 bits: control, interrupt, bulk, isochronous)
 *
 * That's 18 bits. Really. Nothing more. And the USB people have
 * documented these eighteen bits as some kind of glorious
 * virtual data structure.
 *
 * Let's not fall in that trap. We'll just encode it as a simple
 * unsigned int. The encoding is:
 *
 *  - max size:		bits 0-1	(00 = 8, 01 = 16, 10 = 32, 11 = 64) [Historical; now gone.]
 *  - direction:	bit 7		(0 = Host-to-Device [Out], 1 = Device-to-Host [In])
 *  - device:		bits 8-14
 *  - endpoint:		bits 15-18
 *  - Data0/1:		bit 19
 *  - speed:		bit 26		(0 = Full, 1 = Low Speed)
 *  - pipe type:	bits 30-31	(00 = isochronous, 01 = interrupt, 10 = control, 11 = bulk)
 *
 * Why? Because it's arbitrary, and whatever encoding we select is really
 * up to us. This one happens to share a lot of bit positions with the UHCI
 * specification, so that much of the uhci driver can just mask the bits
 * appropriately.
 *
 * NOTE:  there's no encoding (yet?) for a "high speed" endpoint; treat them
 * like full speed devices.
 */

#define USB_PIPE_ISOCHRONOUS		0
#define USB_PIPE_INTERRUPT			1
#define USB_PIPE_CONTROL			2
#define USB_PIPE_BULK			3

#define usb_maxpacket(dev, pipe, out)	(out \
				? (dev)->nMaxPacketOut[usb_pipeendpoint(pipe)] \
				: (dev)->nMaxPacketIn [usb_pipeendpoint(pipe)] )
#define usb_packetid(pipe)	(((pipe) & USB_DIR_IN) ? USB_PID_IN : USB_PID_OUT)

#define usb_pipeout(pipe)	((((pipe) >> 7) & 1) ^ 1)
#define usb_pipein(pipe)	(((pipe) >> 7) & 1)
#define usb_pipedevice(pipe)	(((pipe) >> 8) & 0x7f)
#define usb_pipe_endpdev(pipe)	(((pipe) >> 8) & 0x7ff)
#define usb_pipeendpoint(pipe)	(((pipe) >> 15) & 0xf)
#define usb_pipedata(pipe)	(((pipe) >> 19) & 1)
#define usb_pipeslow(pipe)	(((pipe) >> 26) & 1)
#define usb_pipetype(pipe)	(((pipe) >> 30) & 3)
#define usb_pipeisoc(pipe)	(usb_pipetype((pipe)) == USB_PIPE_ISOCHRONOUS)
#define usb_pipeint(pipe)	(usb_pipetype((pipe)) == USB_PIPE_INTERRUPT)
#define usb_pipecontrol(pipe)	(usb_pipetype((pipe)) == USB_PIPE_CONTROL)
#define usb_pipebulk(pipe)	(usb_pipetype((pipe)) == USB_PIPE_BULK)

#define USB_PIPE_DEVEP_MASK		0x0007ff00

/* The D0/D1 toggle bits */
#define usb_gettoggle(dev, ep, out) (((dev)->nToggle[out] >> ep) & 1)
#define	usb_dotoggle(dev, ep, out)  ((dev)->nToggle[out] ^= (1 << ep))
#define usb_settoggle(dev, ep, out, bit) ((dev)->nToggle[out] = ((dev)->nToggle[out] & ~(1 << ep)) | ((bit) << ep))

/* Endpoint halt control/status */
#define usb_endpoint_out(ep_dir)	(((ep_dir >> 7) & 1) ^ 1)
#define usb_endpoint_halt(dev, ep, out) ((dev)->nHalted[out] |= (1 << (ep)))
#define usb_endpoint_running(dev, ep, out) ((dev)->nHalted[out] &= ~(1 << (ep)))
#define usb_endpoint_halted(dev, ep, out) ((dev)->nHalted[out] & (1 << (ep)))

static inline unsigned int __create_pipe( USB_device_s* psDev, unsigned int nEndPoint )
{
	return( ( psDev->nDeviceNum << 8 ) | ( nEndPoint << 15 ) |
		( ( psDev->eSpeed == USB_SPEED_LOW ) << 26 ) );
}

static inline unsigned int __default_pipe( USB_device_s *psDev )
{
	return( ( psDev->eSpeed == USB_SPEED_LOW ) << 26 );
}

/* Create various pipes... */
#define usb_sndctrlpipe(dev,endpoint)	((USB_PIPE_CONTROL << 30) | __create_pipe(dev,endpoint))
#define usb_rcvctrlpipe(dev,endpoint)	((USB_PIPE_CONTROL << 30) | __create_pipe(dev,endpoint) | USB_DIR_IN)
#define usb_sndisocpipe(dev,endpoint)	((USB_PIPE_ISOCHRONOUS << 30) | __create_pipe(dev,endpoint))
#define usb_rcvisocpipe(dev,endpoint)	((USB_PIPE_ISOCHRONOUS << 30) | __create_pipe(dev,endpoint) | USB_DIR_IN)
#define usb_sndbulkpipe(dev,endpoint)	((USB_PIPE_BULK << 30) | __create_pipe(dev,endpoint))
#define usb_rcvbulkpipe(dev,endpoint)	((USB_PIPE_BULK << 30) | __create_pipe(dev,endpoint) | USB_DIR_IN)
#define usb_sndintpipe(dev,endpoint)	((USB_PIPE_INTERRUPT << 30) | __create_pipe(dev,endpoint))
#define usb_rcvintpipe(dev,endpoint)	((USB_PIPE_INTERRUPT << 30) | __create_pipe(dev,endpoint) | USB_DIR_IN)
#define usb_snddefctrl(dev)		((USB_PIPE_CONTROL << 30) | __default_pipe(dev))
#define usb_rcvdefctrl(dev)		((USB_PIPE_CONTROL << 30) | __default_pipe(dev) | USB_DIR_IN)


/* Some packet macros */
#define USB_FILL_CONTROL(URB,DEV,PIPE,SETUP_PACKET,TRANSFER_BUFFER,BUFFER_LENGTH,COMPLETE,CONTEXT) \
    do {\
	(URB)->psDevice=DEV;\
	(URB)->nPipe=PIPE;\
	(URB)->pSetupPacket=SETUP_PACKET;\
	(URB)->pBuffer=TRANSFER_BUFFER;\
	(URB)->nBufferLength=BUFFER_LENGTH;\
	(URB)->pComplete=COMPLETE;\
	(URB)->pCompleteData=CONTEXT; \
    } while (0)
    
#define USB_FILL_BULK(URB,DEV,PIPE,TRANSFER_BUFFER,BUFFER_LENGTH,COMPLETE,CONTEXT) \
    do {\
    (URB)->psDevice=DEV;\
	(URB)->nPipe=PIPE;\
	(URB)->pBuffer=TRANSFER_BUFFER;\
	(URB)->nBufferLength=BUFFER_LENGTH;\
	(URB)->pComplete=COMPLETE;\
	(URB)->pCompleteData=CONTEXT; \
    } while (0)
 
#define USB_FILL_INT(URB,DEV,PIPE,TRANSFER_BUFFER,BUFFER_LENGTH,COMPLETE,CONTEXT,INTERVAL) \
    do {\
	(URB)->psDevice=DEV;\
	(URB)->nPipe=PIPE;\
	(URB)->pBuffer=TRANSFER_BUFFER;\
	(URB)->nBufferLength=BUFFER_LENGTH;\
	(URB)->pComplete=COMPLETE;\
	(URB)->pCompleteData=CONTEXT;\
	(URB)->nInterval=INTERVAL; \
	(URB)->nStartFrame=-1; \
    } while (0)

#define USB_FILL_CONTROL_TO(a,aa,b,c,d,e,f,g,h) \
    do {\
	(a)->psDevice=aa;\
	(a)->nPipe=b;\
	(a)->pSetupPacket=c;\
	(a)->pBuffer=d;\
	(a)->nBufferLength=e;\
	(a)->pComplete=f;\
	(a)->pCompleteData=g;\
	(a)->nTimeout=h;\
    } while (0)

#define USB_FILL_BULK_TO(a,aa,b,c,d,e,f,g) \
    do {\
	(a)->psDevice=aa;\
	(a)->nPipe=b;\
	(a)->pBuffer=c;\
	(a)->nBufferLength=d;\
	(a)->pComplete=e;\
	(a)->pCompleteData=f;\
	(a)->nTimeout=g;\
    } while (0)

#endif	/* __F_KERNEL_USB_H_ */
