
#include <atheos/device.h>
#include "usb_disk.h"
#include "transport.h"
#include "protocol.h"
#include "jumpshot.h"

USB_driver_s *g_pcDriver;
USB_bus_s *g_psBus = NULL;
SCSI_bus_s* g_psSCSIBus = NULL;
int g_nDeviceID;

USB_disk_s *us_list;
sem_id us_list_semaphore;

static int my_host_number;

/* Virtual SCSI controller */

int usbdisk_scsi_get_device_id()
{
	return ( g_nDeviceID );
}


char *usbdisk_scsi_get_name()
{
	return ( "USB disk" );
}

int usbdisk_scsi_get_max_cd()
{
	return ( 0 );
}

int usbdisk_scsi_command( SCSI_cmd * psCommand )
{
	USB_disk_s *psDisk = psCommand->psHost->pPrivate;
	
	psDisk->srb = psCommand;

	if ( psCommand->nLun > psDisk->max_lun )
		return ( SCSI_ERROR );

	psDisk->proto_handler( psDisk->srb, psDisk );
	return ( 0 );
}


/* Set up the IRQ pipe and handler
 * Note that this function assumes that all the data in the us_data
 * strucuture is current.  This includes the ep_int field, which gives us
 * the endpoint for the interrupt.
 * Returns non-zero on failure, zero on success
 */
int usbdisk_allocate_irq( USB_disk_s * psDisk )
{
	unsigned int pipe;
	int maxp;
	int result;

	printk( "Allocating IRQ for CBI transport\n" );

	/* lock access to the data structure */
	LOCK( psDisk->irq_urb_sem );

	/* allocate the URB */
	psDisk->irq_urb = g_psBus->alloc_packet( 0 );
	if ( !psDisk->irq_urb )
	{
		UNLOCK( psDisk->irq_urb_sem );
		printk( "couldn't allocate interrupt URB\n" );
		return 1;
	}

	/* calculate the pipe and max packet size */
	pipe = usb_rcvintpipe( psDisk->pusb_dev, psDisk->ep_int->nEndpointAddress & USB_ENDPOINT_NUMBER_MASK );
	maxp = usb_maxpacket( psDisk->pusb_dev, pipe, usb_pipeout( pipe ) );
	if ( maxp > sizeof( psDisk->irqbuf ) )
		maxp = sizeof( psDisk->irqbuf );

	/* fill in the URB with our data */
	USB_FILL_INT( psDisk->irq_urb, psDisk->pusb_dev, pipe, psDisk->irqbuf, maxp, usb_stor_CBI_irq, psDisk, psDisk->ep_int->nInterval );

	/* submit the URB for processing */
	result = g_psBus->submit_packet( psDisk->irq_urb );
	if ( result )
	{
		g_psBus->free_packet( psDisk->irq_urb );
		UNLOCK( psDisk->irq_urb_sem );
		return 2;
	}

	/* unlock the data structure and return success */
	UNLOCK( psDisk->irq_urb_sem );
	return 0;
}

bool usbdisk_add( USB_device_s * psDevice, unsigned int nIfnum, void **pPrivate )
{
	USB_interface_s *psIface;
	USB_desc_interface_s *psInterface = NULL;
	USB_desc_endpoint_s *psEndpointIn = NULL;
	USB_desc_endpoint_s *psEndpointOut = NULL;
	USB_desc_endpoint_s *psEndpointInt = NULL;
	char mf[USB_STOR_STRING_LEN];	/* manufacturer */
	char prod[USB_STOR_STRING_LEN];	/* product */
	char serial[USB_STOR_STRING_LEN];	/* serial number */
	uint8 subclass = 0;
	uint8 protocol = 0;
	unsigned int flags;
	bool bFound = false;
	USB_disk_s *psDisk = NULL;
	int i;


	psIface = psDevice->psActConfig->psInterface + nIfnum;
	
	if( psDevice->sDeviceDesc.nVendorID == 0x05dc )
	{
		printk( "Lexar Jumpshot USB CF Reader detected\n" );
		psInterface = &psIface->psSetting[psIface->nActSetting];
		subclass = US_SC_SCSI;
		protocol = US_PR_JUMPSHOT;
		bFound = true;
		
	}
	
	if( !bFound )
	{
	for ( i = 0; i < psIface->nNumSetting; i++ )
	{
		psIface->nActSetting = i;
		psInterface = &psIface->psSetting[psIface->nActSetting];

		if ( psInterface->nInterfaceClass == USB_CLASS_MASS_STORAGE && ( psInterface->nInterfaceProtocol == US_PR_CBI || psInterface->nInterfaceProtocol == US_PR_CB || psInterface->nInterfaceProtocol == US_PR_BULK ) )
		{
			bFound = true;
			break;
		}
	}
	}
	if ( !bFound )
		return ( false );

	/* clear the temporary strings */
	memset( mf, 0, sizeof( mf ) );
	memset( prod, 0, sizeof( prod ) );
	memset( serial, 0, sizeof( serial ) );

	/* Determine subclass and protocol, or copy from the interface */
	if( subclass == 0 )
		subclass = psInterface->nInterfaceSubClass;
	if( protocol == 0 )
		protocol = psInterface->nInterfaceProtocol;
	flags = 0;

	printk( "USB disk connected\n" );

	/*
	 * Find the endpoints we need
	 * We are expecting a minimum of 2 endpoints - in and out (bulk).
	 * An optional interrupt is OK (necessary for CBI protocol).
	 * We will ignore any others.
	 */
	for ( i = 0; i < psInterface->nNumEndpoints; i++ )
	{
		/* is it an BULK endpoint? */
		if ( ( psInterface->psEndpoint[i].nMAttributes & USB_ENDPOINT_XFERTYPE_MASK ) == USB_ENDPOINT_XFER_BULK )
		{
			/* BULK in or out? */
			if ( psInterface->psEndpoint[i].nEndpointAddress & USB_DIR_IN )
				psEndpointIn = &psInterface->psEndpoint[i];
			else
				psEndpointOut = &psInterface->psEndpoint[i];
		}

		/* is it an interrupt endpoint? */
		if ( ( psInterface->psEndpoint[i].nMAttributes & USB_ENDPOINT_XFERTYPE_MASK ) == USB_ENDPOINT_XFER_INT )
		{
			psEndpointInt = &psInterface->psEndpoint[i];
		}
	}

	/* Do some basic sanity checks, and bail if we find a problem */
	if ( !psEndpointIn || !psEndpointOut || ( protocol == US_PR_CBI && !psEndpointInt ) )
	{
		printk( "Invalid endpoint configuration\n" );
		return ( false );
	}

	atomic_inc( &psDevice->nRefCount );

	/* Print information */
	if ( psDevice->sDeviceDesc.nManufacturer )
		g_psBus->string( psDevice, psDevice->sDeviceDesc.nManufacturer, mf, sizeof( mf ) );

	if ( psDevice->sDeviceDesc.nProduct )
		g_psBus->string( psDevice, psDevice->sDeviceDesc.nProduct, prod, sizeof( prod ) );

	if ( psDevice->sDeviceDesc.nSerialNumber )
		g_psBus->string( psDevice, psDevice->sDeviceDesc.nSerialNumber, serial, sizeof( serial ) );

	printk( "%s %s detected\n", mf, prod );

	/* Create device */
	psDisk = ( USB_disk_s * ) kmalloc( sizeof( USB_disk_s ), MEMF_KERNEL );
	if ( !psDisk )
	{
		atomic_dec( &psDevice->nRefCount );
		printk( "Out of memory\n" );
		return ( false );
	}
	memset( psDisk, 0, sizeof( USB_disk_s ) );

	/* Allocate packet */
	psDisk->current_urb = g_psBus->alloc_packet( 0 );
	if ( !psDisk->current_urb )
	{
		kfree( psDisk );
		atomic_dec( &psDevice->nRefCount );
		printk( "Out of memory\n" );
		return ( false );
	}

	/* Initialize semaphores */
	psDisk->ip_waitq = create_semaphore( "ip_waitq", 0, 0 );
	psDisk->queue_exclusion = create_semaphore( "queue_exclusion", 1, 0 );
	psDisk->irq_urb_sem = create_semaphore( "irq_urb_sem", 1, 0 );
	psDisk->current_urb_sem = create_semaphore( "current_urb_sem", 1, 0 );
	psDisk->dev_semaphore = create_semaphore( "dev_semaphore", 1, 0 );

	/* copy over the subclass and protocol data */
	psDisk->subclass = subclass;
	psDisk->protocol = protocol;
	psDisk->flags = flags;


	/* copy over the endpoint data */
	if ( psEndpointIn )
		psDisk->ep_in = psEndpointIn->nEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
	if ( psEndpointOut )
		psDisk->ep_out = psEndpointOut->nEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
	psDisk->ep_int = psEndpointInt;

	/* establish the connection to the new device */
	psDisk->ifnum = nIfnum;
	psDisk->pusb_dev = psDevice;

	/* copy over the identifiying strings */
	strncpy( psDisk->vendor, mf, USB_STOR_STRING_LEN );
	strncpy( psDisk->product, prod, USB_STOR_STRING_LEN );
	strncpy( psDisk->serial, serial, USB_STOR_STRING_LEN );
	if ( strlen( psDisk->vendor ) == 0 )
	{
		strncpy( psDisk->vendor, "Unknown", USB_STOR_STRING_LEN );
	}
	if ( strlen( psDisk->product ) == 0 )
	{
		strncpy( psDisk->product, "Unknown", USB_STOR_STRING_LEN );
	}
	if ( strlen( psDisk->serial ) == 0 )
		strncpy( psDisk->serial, "None", USB_STOR_STRING_LEN );

	/* 
	 * Set the handler pointers based on the protocol
	 * Again, this data is persistant across reattachments
	 */
	switch ( psDisk->protocol )
	{
	case US_PR_CB:
		psDisk->transport_name = "Control/Bulk";
		psDisk->transport = usb_stor_CB_transport;
		psDisk->transport_reset = usb_stor_CB_reset;
		psDisk->max_lun = 7;
		break;

	case US_PR_CBI:
		psDisk->transport_name = "Control/Bulk/Interrupt";
		psDisk->transport = usb_stor_CBI_transport;
		psDisk->transport_reset = usb_stor_CB_reset;
		psDisk->max_lun = 7;
		break;

	case US_PR_BULK:
		psDisk->transport_name = "Bulk";
		psDisk->transport = usb_stor_Bulk_transport;
		psDisk->transport_reset = usb_stor_Bulk_reset;
		psDisk->max_lun = usb_stor_Bulk_max_lun( psDisk );
		break;
	case US_PR_JUMPSHOT:
		psDisk->transport_name = "Lexar Jumpshot Control/Bulk";
		psDisk->transport = jumpshot_transport;
		psDisk->transport_reset = usb_stor_Bulk_reset;
		psDisk->max_lun = 1;
		break;
	default:
		psDisk->transport_name = "Unknown";
		kfree( psDisk->current_urb );
		kfree( psDisk );
		atomic_dec( &psDevice->nRefCount );
		return ( false );
		break;
	}
	printk( "Transport: %s\n", psDisk->transport_name );

	switch ( psDisk->subclass )
	{
	case US_SC_RBC:
		psDisk->protocol_name = "Reduced Block Commands (RBC)";
		psDisk->proto_handler = usb_stor_transparent_scsi_command;
		break;

	case US_SC_8020:
		psDisk->protocol_name = "8020i";
		psDisk->proto_handler = usb_stor_ATAPI_command;
		psDisk->max_lun = 0;
		break;

	case US_SC_QIC:
		psDisk->protocol_name = "QIC-157";
		psDisk->proto_handler = usb_stor_qic157_command;
		psDisk->max_lun = 0;
		break;

	case US_SC_8070:
		psDisk->protocol_name = "8070i";
		psDisk->proto_handler = usb_stor_ATAPI_command;
		psDisk->max_lun = 0;
		break;

	case US_SC_SCSI:
		psDisk->protocol_name = "SCSI";
		psDisk->proto_handler = usb_stor_transparent_scsi_command;
		break;

	case US_SC_UFI:
		psDisk->protocol_name = "Uniform Floppy Interface (UFI)";
		psDisk->proto_handler = usb_stor_ufi_command;
		break;

	default:
		kfree( psDisk->current_urb );
		kfree( psDisk );
		atomic_dec( &psDevice->nRefCount );
		return ( false );
		break;
	}
	printk( "Protocol: %s\n", psDisk->protocol_name );

	/* allocate an IRQ callback if one is needed */
	if ( ( psDisk->protocol == US_PR_CBI ) && usbdisk_allocate_irq( psDisk ) )
	{
		atomic_dec( &psDevice->nRefCount );
		return ( false );
	}

	/* Create SCSI host */
	psDisk->host = kmalloc( sizeof( SCSI_host_s ), MEMF_KERNEL );
	memset( psDisk->host, 0, sizeof( SCSI_host_s ) );
	psDisk->host->get_device_id = usbdisk_scsi_get_device_id;
	psDisk->host->get_name = usbdisk_scsi_get_name;
	psDisk->host->get_max_channel = psDisk->host->get_max_device = usbdisk_scsi_get_max_cd;
	psDisk->host->queue_command = usbdisk_scsi_command;
	psDisk->host->pPrivate = psDisk;

	g_psSCSIBus->scan_host( psDisk->host );


	/* Grab the next host number */
	psDisk->host_number = my_host_number++;


	/* lock access to the data structures */
	LOCK( us_list_semaphore );

	/* put us in the list */
	psDisk->next = us_list;
	us_list = psDisk;

	/* release the data structure lock */
	UNLOCK( us_list_semaphore );

	{
		char buf[128];

		sprintf( buf, "%s %s", mf, prod );
		claim_device( g_nDeviceID, psDevice->nHandle, buf, DEVICE_DRIVE );
	}

	*pPrivate = psDisk;

	return ( true );
}



void usbdisk_remove( USB_device_s * psDevice, void *pPrivate )
{
	USB_disk_s *psDisk = pPrivate;

	g_psSCSIBus->remove_host( psDisk->host );
	release_device( psDevice->nHandle );
	atomic_dec( &psDevice->nRefCount );
}

status_t device_init( int nDeviceID )
{
	/* Get USB bus */
	g_psBus = get_busmanager( USB_BUS_NAME, USB_BUS_VERSION );
	if ( g_psBus == NULL )
	{
		return ( -1 );
	}
	
	/* Get SCSI bus */
	g_psSCSIBus = get_busmanager( SCSI_BUS_NAME, SCSI_BUS_VERSION );
	if ( g_psSCSIBus == NULL )
	{
		return ( -1 );
	}

	g_nDeviceID = nDeviceID;

	us_list = NULL;
	us_list_semaphore = create_semaphore( "usb_disk_list", 1, 0 );
	my_host_number = 0;

	/* Register */
	g_pcDriver = ( USB_driver_s * ) kmalloc( sizeof( USB_driver_s ), MEMF_KERNEL | MEMF_OKTOFAIL );

	strcpy( g_pcDriver->zName, "USB disk" );
	g_pcDriver->AddDevice = usbdisk_add;
	g_pcDriver->RemoveDevice = usbdisk_remove;

	g_psBus->add_driver_resistant( g_pcDriver );

	printk( "USB disk driver loaded\n" );
	return ( 0 );
}

status_t device_uninit( int nDeviceID )
{
	if ( g_psBus )
		g_psBus->remove_driver( g_pcDriver );
	return ( 0 );
}







