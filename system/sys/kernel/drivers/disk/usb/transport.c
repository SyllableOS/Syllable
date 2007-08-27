
/* Driver for USB Mass Storage compliant devices
 *
 * $Id$
 *
 * Current development and maintenance by:
 *   (c) 1999, 2000 Matthew Dharm (mdharm-usb@one-eyed-alien.net)
 *
 * Developed with the assistance of:
 *   (c) 2000 David L. Brown, Jr. (usb-storage@davidb.org)
 *   (c) 2000 Stephen J. Gowdy (SGowdy@lbl.gov)
 *
 * Initial work by:
 *   (c) 1999 Michael Gee (michael@linuxspecific.com)
 *
 * This driver is based on the 'USB Mass Storage Class' document. This
 * describes in detail the protocol used to communicate with such
 * devices.  Clearly, the designers had SCSI and ATAPI commands in
 * mind when they created this document.  The commands are all very
 * similar to commands in the SCSI-II and ATAPI specifications.
 *
 * It is important to note that in a number of cases this class
 * exhibits class-specific exemptions from the USB specification.
 * Notably the usage of NAK, STALL and ACK differs from the norm, in
 * that they are used to communicate wait, failed and OK on commands.
 *
 * Also, for certain devices, the interrupt endpoint is used to convey
 * status of a command.
 *
 * Please see http://www.one-eyed-alien.net/~mdharm/linux-usb for more
 * information about this driver.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <atheos/types.h>
#include <posix/errno.h>
#include "transport.h"
#include "protocol.h"
#include "usb_disk.h"

extern USB_bus_s *g_psBus;

/***********************************************************************
 * Helper routines
 ***********************************************************************/

/* Calculate the length of the data transfer (not the command) for any
 * given SCSI command
 */
unsigned int usb_stor_transfer_length( SCSI_cmd_s * srb )
{
	int doDefault = 0;
	unsigned int len = 0;

	/* This table tells us:
	   X = command not supported
	   L = return length in cmnd[4] (8 bits).
	   M = return length in cmnd[8] (8 bits).
	   G = return length in cmnd[3] and cmnd[4] (16 bits)
	   H = return length in cmnd[7] and cmnd[8] (16 bits)
	   I = return length in cmnd[8] and cmnd[9] (16 bits)
	   C = return length in cmnd[2] to cmnd[5] (32 bits)
	   D = return length in cmnd[6] to cmnd[9] (32 bits)
	   B = return length in blocksize so we use buff_len
	   R = return length in cmnd[2] to cmnd[4] (24 bits)
	   S = return length in cmnd[3] to cmnd[5] (24 bits)
	   T = return length in cmnd[6] to cmnd[8] (24 bits)
	   U = return length in cmnd[7] to cmnd[9] (24 bits)
	   0-9 = fixed return length
	   V = 20 bytes
	   W = 24 bytes
	   Z = return length is mode dependant or not in command, use buff_len
	 */

	static char *lengths =
		/* 0123456789ABCDEF   0123456789ABCDEF */
		"00XLZ6XZBXBBXXXB" "00LBBLG0R0L0GG0X"	/* 00-1F */
		"XXXXT8XXB4B0BBBB" "ZZZ0B00HCSSZTBHH"	/* 20-3F */
		"M0HHB0X000H0HH0X" "XHH0HHXX0TH0H0XX"	/* 40-5F */
		"XXXXXXXXXXXXXXXX" "XXXXXXXXXXXXXXXX"	/* 60-7F */
		"XXXXXXXXXXXXXXXX" "XXXXXXXXXXXXXXXX"	/* 80-9F */
		"X0XXX00XB0BXBXBB" "ZZZ0XUIDU000XHBX"	/* A0-BF */
		"XXXXXXXXXXXXXXXX" "XXXXXXXXXXXXXXXX"	/* C0-DF */
		"XDXXXXXXXXXXXXXX" "XXW00HXXXXXXXXXX";	/* E0-FF */

	/* Commands checked in table:

	   CHANGE_DEFINITION 40
	   COMPARE 39
	   COPY 18
	   COPY_AND_VERIFY 3a
	   ERASE 19
	   ERASE_10 2c
	   ERASE_12 ac
	   EXCHANGE_MEDIUM a6
	   FORMAT_UNIT 04
	   GET_DATA_BUFFER_STATUS 34
	   GET_MESSAGE_10 28
	   GET_MESSAGE_12 a8
	   GET_WINDOW 25   !!! Has more data than READ_CAPACITY, need to fix table
	   INITIALIZE_ELEMENT_STATUS 07 !!! REASSIGN_BLOCKS luckily uses buff_len
	   INQUIRY 12
	   LOAD_UNLOAD 1b
	   LOCATE 2b
	   LOCK_UNLOCK_CACHE 36
	   LOG_SELECT 4c
	   LOG_SENSE 4d
	   MEDIUM_SCAN 38     !!! This was M
	   MODE_SELECT6 15
	   MODE_SELECT_10 55
	   MODE_SENSE_6 1a
	   MODE_SENSE_10 5a
	   MOVE_MEDIUM a5
	   OBJECT_POSITION 31  !!! Same as SEARCH_DATA_EQUAL
	   PAUSE_RESUME 4b
	   PLAY_AUDIO_10 45
	   PLAY_AUDIO_12 a5
	   PLAY_AUDIO_MSF 47
	   PLAY_AUDIO_TRACK_INDEX 48
	   PLAY_AUDIO_TRACK_RELATIVE_10 49
	   PLAY_AUDIO_TRACK_RELATIVE_12 a9
	   POSITION_TO_ELEMENT 2b
	   PRE-FETCH 34
	   PREVENT_ALLOW_MEDIUM_REMOVAL 1e
	   PRINT 0a             !!! Same as WRITE_6 but is always in bytes
	   READ_6 08
	   READ_10 28
	   READ_12 a8
	   READ_BLOCK_LIMITS 05
	   READ_BUFFER 3c
	   READ_CAPACITY 25
	   READ_CDROM_CAPACITY 25
	   READ_DEFECT_DATA 37
	   READ_DEFECT_DATA_12 b7
	   READ_ELEMENT_STATUS b8 !!! Think this is in bytes
	   READ_GENERATION 29 !!! Could also be M?
	   READ_HEADER 44     !!! This was L
	   READ_LONG 3e
	   READ_POSITION 34   !!! This should be V but conflicts with PRE-FETCH
	   READ_REVERSE 0f
	   READ_SUB-CHANNEL 42 !!! Is this in bytes?
	   READ_TOC 43         !!! Is this in bytes?
	   READ_UPDATED_BLOCK 2d
	   REASSIGN_BLOCKS 07
	   RECEIVE 08        !!! Same as READ_6 probably in bytes though
	   RECEIVE_DIAGNOSTIC_RESULTS 1c
	   RECOVER_BUFFERED_DATA 14 !!! For PRINTERs this is bytes
	   RELEASE_UNIT 17
	   REQUEST_SENSE 03
	   REQUEST_VOLUME_ELEMENT_ADDRESS b5 !!! Think this is in bytes
	   RESERVE_UNIT 16
	   REWIND 01
	   REZERO_UNIT 01
	   SCAN 1b          !!! Conflicts with various commands, should be L
	   SEARCH_DATA_EQUAL 31
	   SEARCH_DATA_EQUAL_12 b1
	   SEARCH_DATA_LOW 30
	   SEARCH_DATA_LOW_12 b0
	   SEARCH_DATA_HIGH 32
	   SEARCH_DATA_HIGH_12 b2
	   SEEK_6 0b         !!! Conflicts with SLEW_AND_PRINT
	   SEEK_10 2b
	   SEND 0a           !!! Same as WRITE_6, probably in bytes though
	   SEND 2a           !!! Similar to WRITE_10 but for scanners
	   SEND_DIAGNOSTIC 1d
	   SEND_MESSAGE_6 0a   !!! Same as WRITE_6 - is in bytes
	   SEND_MESSAGE_10 2a  !!! Same as WRITE_10 - is in bytes
	   SEND_MESSAGE_12 aa  !!! Same as WRITE_12 - is in bytes
	   SEND_OPC 54
	   SEND_VOLUME_TAG b6 !!! Think this is in bytes
	   SET_LIMITS 33
	   SET_LIMITS_12 b3
	   SET_WINDOW 24
	   SLEW_AND_PRINT 0b !!! Conflicts with SEEK_6
	   SPACE 11
	   START_STOP_UNIT 1b
	   STOP_PRINT 1b
	   SYNCHRONIZE_BUFFER 10
	   SYNCHRONIZE_CACHE 35
	   TEST_UNIT_READY 00
	   UPDATE_BLOCK 3d
	   VERIFY 13
	   VERIFY 2f
	   VERIFY_12 af
	   WRITE_6 0a
	   WRITE_10 2a
	   WRITE_12 aa
	   WRITE_AND_VERIFY 2e
	   WRITE_AND_VERIFY_12 ae
	   WRITE_BUFFER 3b
	   WRITE_FILEMARKS 10
	   WRITE_LONG 3f
	   WRITE_SAME 41
	 */

	if ( srb->nDirection == SCSI_DATA_WRITE )
	{
		doDefault = 1;
	}
	else
		switch ( lengths[srb->nCmd[0]] )
		{
		case 'L':
			len = srb->nCmd[4];
			break;

		case 'M':
			len = srb->nCmd[8];
			break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			len = lengths[srb->nCmd[0]] - '0';
			break;

		case 'G':
			len = ( ( ( unsigned int )srb->nCmd[3] ) << 8 ) | srb->nCmd[4];
			break;

		case 'H':
			len = ( ( ( unsigned int )srb->nCmd[7] ) << 8 ) | srb->nCmd[8];
			break;

		case 'I':
			len = ( ( ( unsigned int )srb->nCmd[8] ) << 8 ) | srb->nCmd[9];
			break;

		case 'R':
			len = ( ( ( unsigned int )srb->nCmd[2] ) << 16 ) | ( ( ( unsigned int )srb->nCmd[3] ) << 8 ) | srb->nCmd[4];
			break;

		case 'S':
			len = ( ( ( unsigned int )srb->nCmd[3] ) << 16 ) | ( ( ( unsigned int )srb->nCmd[4] ) << 8 ) | srb->nCmd[5];
			break;

		case 'T':
			len = ( ( ( unsigned int )srb->nCmd[6] ) << 16 ) | ( ( ( unsigned int )srb->nCmd[7] ) << 8 ) | srb->nCmd[8];
			break;

		case 'U':
			len = ( ( ( unsigned int )srb->nCmd[7] ) << 16 ) | ( ( ( unsigned int )srb->nCmd[8] ) << 8 ) | srb->nCmd[9];
			break;

		case 'C':
			len = ( ( ( unsigned int )srb->nCmd[2] ) << 24 ) | ( ( ( unsigned int )srb->nCmd[3] ) << 16 ) | ( ( ( unsigned int )srb->nCmd[4] ) << 8 ) | srb->nCmd[5];
			break;

		case 'D':
			len = ( ( ( unsigned int )srb->nCmd[6] ) << 24 ) | ( ( ( unsigned int )srb->nCmd[7] ) << 16 ) | ( ( ( unsigned int )srb->nCmd[8] ) << 8 ) | srb->nCmd[9];
			break;

		case 'V':
			len = 20;
			break;

		case 'W':
			len = 24;
			break;

		case 'B':
			/* Use buffer size due to different block sizes */
			doDefault = 1;
			break;

		case 'X':
			printk( "Error: UNSUPPORTED COMMAND %02X\n", srb->nCmd[0] );
			doDefault = 1;
			break;

		case 'Z':
			/* Use buffer size due to mode dependence */
			doDefault = 1;
			break;

		default:
			printk( "Error: COMMAND %02X out of range or table inconsistent (%c).\n", srb->nCmd[0], lengths[srb->nCmd[0]] );
			doDefault = 1;
		}

	if ( doDefault == 1 )
	{

		/* Just return the length of the buffer */
		len = srb->nRequestSize;
	}

	return len;
}

/* This is a version of usb_clear_halt() that doesn't read the status from
 * the device -- this is because some devices crash their internal firmware
 * when the status is requested after a halt
 */
int usb_stor_clear_halt( USB_device_s * dev, int pipe )
{
	int result;
	int endp = usb_pipeendpoint( pipe ) | ( usb_pipein( pipe ) << 7 );

	result = g_psBus->control_msg( dev, usb_sndctrlpipe( dev, 0 ), USB_REQ_CLEAR_FEATURE, USB_RECIP_ENDPOINT, 0, endp, NULL, 0, 100 * 3 * 1000 );

	/* this is a failure case */
	if ( result < 0 )
		return result;

	/* reset the toggles and endpoint flags */
	usb_endpoint_running( dev, usb_pipeendpoint( pipe ), usb_pipeout( pipe ) );
	usb_settoggle( dev, usb_pipeendpoint( pipe ), usb_pipeout( pipe ), 0 );

	return 0;
}

/***********************************************************************
 * Data transfer routines
 ***********************************************************************/

/* This is the completion handler which will wake us up when an URB
 * completes.
 */
static void usb_stor_blocking_completion( USB_packet_s * psPacket )
{
	psPacket->bDone = true;
	UNLOCK( psPacket->hWait );
}

/* This is our function to emulate usb_control_msg() but give us enough
 * access to make aborts/resets work
 */
int usb_stor_control_msg( USB_disk_s * psDisk, unsigned int pipe, uint8 request, uint8 requesttype, uint16 value, uint16 index, void *data, uint16 size )
{
	int status;
	USB_request_s *dr;

	/* allocate the device request structure */
	dr = kmalloc( sizeof( USB_request_s ), MEMF_KERNEL );
	if ( !dr )
		return -ENOMEM;

	/* fill in the structure */
	dr->nRequestType = requesttype;
	dr->nRequest = request;
	dr->nValue = value;
	dr->nIndex = index;
	dr->nLength = size;


	/* lock the URB */
	LOCK( psDisk->current_urb_sem );

	/* fill the URB */
	USB_FILL_CONTROL( psDisk->current_urb, psDisk->pusb_dev, pipe, ( unsigned char * )dr, data, size, usb_stor_blocking_completion, NULL );
	psDisk->current_urb->nActualLength = 0;
	psDisk->current_urb->nErrorCount = 0;
	psDisk->current_urb->nTransferFlags = USB_ASYNC_UNLINK;

	psDisk->current_urb->bDone = false;
	psDisk->current_urb->hWait = create_semaphore( "usb_disk_control_packet", 0, 0 );

	/* submit the URB */
	status = g_psBus->submit_packet( psDisk->current_urb );
	if ( status )
	{
		/* something went wrong */
		UNLOCK( psDisk->current_urb_sem );
		kfree( dr );
		return status;
	}

	/* wait for the completion of the URB */
	//UNLOCK(psDisk->current_urb_sem);
	LOCK( psDisk->current_urb->hWait );
	//LOCK(psDisk->current_urb_sem);

	delete_semaphore( psDisk->current_urb->hWait );

	/* return the actual length of the data transferred if no error */
	status = psDisk->current_urb->nStatus;
	if ( status >= 0 )
		status = psDisk->current_urb->nActualLength;

	/* release the lock and return status */
	UNLOCK( psDisk->current_urb_sem );
	kfree( dr );
	return status;
}

/* This is our function to emulate usb_bulk_msg() but give us enough
 * access to make aborts/resets work
 */
int usb_stor_bulk_msg( USB_disk_s * psDisk, void *data, int pipe, unsigned int len, unsigned int *act_len )
{
	int status;

	/* lock the URB */
	LOCK( psDisk->current_urb_sem );

	/* fill the URB */
	USB_FILL_BULK( psDisk->current_urb, psDisk->pusb_dev, pipe, data, len, usb_stor_blocking_completion, NULL );
	psDisk->current_urb->nActualLength = 0;
	psDisk->current_urb->nErrorCount = 0;
	psDisk->current_urb->nTransferFlags = USB_ASYNC_UNLINK;

	psDisk->current_urb->bDone = false;
	psDisk->current_urb->hWait = create_semaphore( "usb_disk_bulk_packet", 0, 0 );

	/* submit the URB */
	status = g_psBus->submit_packet( psDisk->current_urb );
	if ( status )
	{
		/* something went wrong */
		UNLOCK( psDisk->current_urb_sem );
		return status;
	}

	LOCK( psDisk->current_urb->hWait );
	//LOCK(psDisk->current_urb_sem);

	delete_semaphore( psDisk->current_urb->hWait );

	/* return the actual length of the data transferred */
	*act_len = psDisk->current_urb->nActualLength;

	/* release the lock and return status */
	UNLOCK( psDisk->current_urb_sem );
	return psDisk->current_urb->nStatus;
}

/*
 * Transfer one SCSI scatter-gather buffer via bulk transfer
 *
 * Note that this function is necessary because we want the ability to
 * use scatter-gather memory.  Good performance is achieved by a combination
 * of scatter-gather and clustering (which makes each chunk bigger).
 *
 * Note that the lower layer will always retry when a NAK occurs, up to the
 * timeout limit.  Thus we don't have to worry about it for individual
 * packets.
 */
int usb_stor_transfer_partial( USB_disk_s * psDisk, char *buf, int length )
{
	int result;
	int partial;
	int pipe;

	/* calculate the appropriate pipe information */
	if ( psDisk->srb->nDirection == SCSI_DATA_READ )
		pipe = usb_rcvbulkpipe( psDisk->pusb_dev, psDisk->ep_in );
	else
		pipe = usb_sndbulkpipe( psDisk->pusb_dev, psDisk->ep_out );

	/* transfer the data */
	//printk("usb_stor_transfer_partial(): xfer %d bytes\n", length);
	result = usb_stor_bulk_msg( psDisk, buf, pipe, length, &partial );
	//printk("usb_stor_bulk_msg() returned %d xferred %d/%d\n",
	//  result, partial, length);

	/* if we stall, we need to clear it before we go on */
	if ( result == -EPIPE )
	{
		printk( "clearing endpoint halt for pipe 0x%x\n", pipe );
		usb_stor_clear_halt( psDisk->pusb_dev, pipe );
	}

	/* did we send all the data? */
	if ( partial == length )
	{
		//printk("usb_stor_transfer_partial(): transfer complete\n");
		return US_BULK_TRANSFER_GOOD;
	}

	/* uh oh... we have an error code, so something went wrong. */
	if ( result )
	{
		/* NAK - that means we've retried a few times already */
		if ( result == -ETIMEDOUT )
		{
			printk( "usb_stor_transfer_partial(): device NAKed\n" );
			return US_BULK_TRANSFER_FAILED;
		}

		/* -ENOENT -- we canceled this transfer */
		if ( result == -ENOENT )
		{
			printk( "usb_stor_transfer_partial(): transfer aborted\n" );
			return US_BULK_TRANSFER_ABORTED;
		}

		/* the catch-all case */
		printk( "usb_stor_transfer_partial(): unknown error\n" );
		return US_BULK_TRANSFER_FAILED;
	}

	/* no error code, so we must have transferred some data, 
	 * just not all of it */
	return US_BULK_TRANSFER_SHORT;
}

/*
 * Transfer an entire SCSI command's worth of data payload over the bulk
 * pipe.
 *
 * Note that this uses usb_stor_transfer_partial to achieve it's goals -- this
 * function simply determines if we're going to use scatter-gather or not,
 * and acts appropriately.  For now, it also re-interprets the error codes.
 */
void usb_stor_transfer( SCSI_cmd_s * srb, USB_disk_s * psDisk )
{
	int result = -1;
	unsigned int transfer_amount;

	/* calculate how much we want to transfer */
	transfer_amount = usb_stor_transfer_length( srb );

	/* was someone foolish enough to request more data than available
	 * buffer space? */
	if ( transfer_amount > srb->nRequestSize )
		transfer_amount = srb->nRequestSize;

	/* no scatter-gather, just make the request */
	result = usb_stor_transfer_partial( psDisk, srb->pRequestBuffer, transfer_amount );

	/* return the result in the data structure itself */
	srb->nResult = result;
}

/***********************************************************************
 * Transport routines
 ***********************************************************************/

/* Invoke the transport and basic error-handling/recovery methods
 *
 * This is used by the protocol layers to actually send the message to
 * the device and recieve the response.
 */
void usb_stor_invoke_transport( SCSI_cmd_s * srb, USB_disk_s * psDisk )
{
	int need_auto_sense;
	int result;

	/* send the command to the transport layer */
	result = psDisk->transport( srb, psDisk );

	/* if the command gets aborted by the higher layers, we need to
	 * short-circuit all other processing
	 */
	if ( result == USB_STOR_TRANSPORT_ABORTED )
	{
		printk( "-- transport indicates command was aborted\n" );
		srb->nResult = SCSI_ABORT << 16;
		return;
	}

	/* Determine if we need to auto-sense
	 *
	 * I normally don't use a flag like this, but it's almost impossible
	 * to understand what's going on here if I don't.
	 */
	need_auto_sense = 0;

	/*
	 * If we're running the CB transport, which is incapable
	 * of determining status on it's own, we need to auto-sense almost
	 * every time.
	 */
	if ( psDisk->protocol == US_PR_CB || psDisk->protocol == US_PR_DPCM_USB )
	{
		//printk("-- CB transport device requiring auto-sense\n");
		need_auto_sense = 1;

		/* There are some exceptions to this.  Notably, if this is
		 * a UFI device and the command is REQUEST_SENSE or INQUIRY,
		 * then it is impossible to truly determine status.
		 */
		if ( psDisk->subclass == US_SC_UFI && ( ( srb->nCmd[0] == SCSI_REQUEST_SENSE ) || ( srb->nCmd[0] == SCSI_INQUIRY ) ) )
		{
			//printk("** no auto-sense for a special command\n");
			need_auto_sense = 0;
		}
	}

	/*
	 * If we have an error, we're going to do a REQUEST_SENSE 
	 * automatically.  Note that we differentiate between a command
	 * "failure" and an "error" in the transport mechanism.
	 */
	if ( result == USB_STOR_TRANSPORT_FAILED )
	{
		//printk("-- transport indicates command failure\n");
		need_auto_sense = 1;
	}
	if ( result == USB_STOR_TRANSPORT_ERROR )
	{
		psDisk->transport_reset( psDisk );
		//printk("-- transport indicates transport failure\n");
		need_auto_sense = 0;
		srb->nResult = SCSI_ERROR << 16;
		return;
	}

	/*
	 * Also, if we have a short transfer on a command that can't have
	 * a short transfer, we're going to do this.
	 */
	if ( ( srb->nResult == US_BULK_TRANSFER_SHORT ) && !( ( srb->nCmd[0] == SCSI_REQUEST_SENSE ) || ( srb->nCmd[0] == SCSI_INQUIRY ) || ( srb->nCmd[0] == SCSI_MODE_SENSE ) || ( srb->nCmd[0] == SCSI_LOG_SENSE ) || ( srb->nCmd[0] == SCSI_MODE_SENSE_10 ) ) )
	{
		//printk("-- unexpectedly short transfer\n");
		need_auto_sense = 1;
	}

	/* Now, if we need to do the auto-sense, let's do it */
	if ( need_auto_sense )
	{
		int temp_result;
		uint8 *old_request_buffer;
		unsigned old_request_bufflen;
		unsigned char old_sc_data_direction;
		unsigned char old_cmnd[SCSI_CMD_SIZE];

		//printk("Issuing auto-REQUEST_SENSE\n");

		/* save the old command */
		memcpy( old_cmnd, srb->nCmd, SCSI_CMD_SIZE );

		/* set the command and the LUN */
		srb->nCmd[0] = SCSI_REQUEST_SENSE;
		srb->nCmd[1] = srb->nCmd[1] & 0xE0;
		srb->nCmd[2] = 0;
		srb->nCmd[3] = 0;
		srb->nCmd[4] = 18;
		srb->nCmd[5] = 0;

		/* set the transfer direction */
		old_sc_data_direction = srb->nDirection;
		srb->nDirection = SCSI_DATA_READ;

		/* use the new buffer we have */
		old_request_buffer = srb->pRequestBuffer;
		srb->pRequestBuffer = srb->s.nSense;

		/* set the buffer length for transfer */
		old_request_bufflen = srb->nRequestSize;
		srb->nRequestSize = 18;


		/* issue the auto-sense command */
		temp_result = psDisk->transport( psDisk->srb, psDisk );
		if ( temp_result != USB_STOR_TRANSPORT_GOOD )
		{
			//printk("-- auto-sense failure\n");

			/* we skip the reset if this happens to be a
			 * multi-target device, since failure of an
			 * auto-sense is perfectly valid
			 */
			if ( !( psDisk->flags & US_FL_SCM_MULT_TARG ) )
			{
				psDisk->transport_reset( psDisk );
			}
			srb->nResult = SCSI_ERROR << 16;
			return;
		}

		/*printk("-- Result from auto-sense is %d\n", temp_result);
		   printk("-- code: 0x%x, key: 0x%x, ASC: 0x%x, ASCQ: 0x%x\n",
		   srb->s.nSense[0],
		   srb->s.nSense[2] & 0xf,
		   srb->s.nSense[12], 
		   srb->s.nSense[13]); */


		/* set the result so the higher layers expect this data */
		srb->nResult = SCSI_CHECK_CONDITION << 1;

		/* we're done here, let's clean up */
		srb->pRequestBuffer = old_request_buffer;
		srb->nRequestSize = old_request_bufflen;
		srb->nDirection = old_sc_data_direction;
		memcpy( srb->nCmd, old_cmnd, SCSI_CMD_SIZE );

		/* If things are really okay, then let's show that */
		if ( ( srb->s.nSense[2] & 0xf ) == 0x0 )
			srb->nResult = SCSI_GOOD << 1;
	}
	else			/* if (need_auto_sense) */
		srb->nResult = SCSI_GOOD << 1;

	/* Regardless of auto-sense, if we _know_ we have an error
	 * condition, show that in the result code
	 */
	if ( result == USB_STOR_TRANSPORT_FAILED )
		srb->nResult = SCSI_CHECK_CONDITION << 1;

	/* If we think we're good, then make sure the sense data shows it.
	 * This is necessary because the auto-sense for some devices always
	 * sets byte 0 == 0x70, even if there is no error
	 */
	if ( ( psDisk->protocol == US_PR_CB || psDisk->protocol == US_PR_DPCM_USB ) && ( result == USB_STOR_TRANSPORT_GOOD ) && ( ( srb->s.nSense[2] & 0xf ) == 0x0 ) )
		srb->s.nSense[0] = 0x0;
}

/*
 * Control/Bulk/Interrupt transport
 */

/* The interrupt handler for CBI devices */
void usb_stor_CBI_irq( USB_packet_s * urb )
{
	USB_disk_s *psDisk = urb->pCompleteData;

	/*printk("USB IRQ recieved for device\n");
	   printk("-- IRQ data length is %d\n", urb->nActualLength);
	   printk("-- IRQ state is %d\n", urb->nStatus);
	   printk("-- Interrupt Status (0x%x, 0x%x)\n",
	   psDisk->irqbuf[0], psDisk->irqbuf[1]); */

	/* reject improper IRQs */
	if ( urb->nActualLength != 2 )
	{
		//printk("-- IRQ too short\n");
		return;
	}

	/* is the device removed? */
	if ( urb->nStatus == -ENOENT )
	{
		//printk("-- device has been removed\n");
		return;
	}

	/* was this a command-completion interrupt? */
	if ( psDisk->irqbuf[0] && ( psDisk->subclass != US_SC_UFI ) )
	{
		//printk("-- not a command-completion IRQ\n");
		return;
	}

	/* was this a wanted interrupt? */
	if ( !atomic_read( &psDisk->ip_wanted ) )
	{
		printk( "ERROR: Unwanted interrupt received!\n" );
		return;
	}

	/* adjust the flag */
	atomic_set( &psDisk->ip_wanted, 0 );

	/* copy the valid data */
	psDisk->irqdata[0] = psDisk->irqbuf[0];
	psDisk->irqdata[1] = psDisk->irqbuf[1];

	/* wake up the command thread */
	wakeup_sem( psDisk->ip_waitq, true );
}

int usb_stor_CBI_transport( SCSI_cmd_s * srb, USB_disk_s * psDisk )
{
	int result;

	/* Set up for status notification */
	atomic_set( &psDisk->ip_wanted, 1 );


	/* COMMAND STAGE */
	/* let's send the command via the control pipe */
	result = usb_stor_control_msg( psDisk, usb_sndctrlpipe( psDisk->pusb_dev, 0 ), US_CBI_ADSC, USB_TYPE_CLASS | USB_RECIP_INTERFACE, 0, psDisk->ifnum, srb->nCmd, srb->nCmdLen );

	/* check the return code for the command */
	//printk("Call to usb_stor_control_msg() returned %d\n", result);
	if ( result < 0 )
	{
		/* Reset flag for status notification */
		atomic_set( &psDisk->ip_wanted, 0 );

		/* if the command was aborted, indicate that */
		if ( result == -ENOENT )
			return USB_STOR_TRANSPORT_ABORTED;

		/* STALL must be cleared when they are detected */
		if ( result == -EPIPE )
		{
			printk( "-- Stall on control pipe. Clearing\n" );
			result = usb_stor_clear_halt( psDisk->pusb_dev, usb_sndctrlpipe( psDisk->pusb_dev, 0 ) );
			printk( "-- usb_stor_clear_halt() returns %d\n", result );
			return USB_STOR_TRANSPORT_FAILED;
		}

		/* Uh oh... serious problem here */
		return USB_STOR_TRANSPORT_ERROR;
	}

	/* DATA STAGE */
	/* transfer the data payload for this command, if one exists */
	if ( usb_stor_transfer_length( srb ) )
	{
		usb_stor_transfer( srb, psDisk );
		//printk("CBI data stage result is 0x%x\n", srb->nResult);

		/* if it was aborted, we need to indicate that */
		if ( srb->nResult == USB_STOR_TRANSPORT_ABORTED )
		{
			return USB_STOR_TRANSPORT_ABORTED;
		}
	}

	/* STATUS STAGE */

	/* go to sleep until we get this interrupt */
	sleep_on_sem( psDisk->ip_waitq, INFINITE_TIMEOUT );

	/* if we were woken up by an abort instead of the actual interrupt */
	if ( atomic_read( &psDisk->ip_wanted ) )
	{
		printk( "Did not get interrupt on CBI\n" );
		atomic_set( &psDisk->ip_wanted, 0 );
		return USB_STOR_TRANSPORT_ABORTED;
	}

	//printk("Got interrupt data (0x%x, 0x%x)\n", 
	//      psDisk->irqdata[0], psDisk->irqdata[1]);

	/* UFI gives us ASC and ASCQ, like a request sense
	 *
	 * REQUEST_SENSE and INQUIRY don't affect the sense data on UFI
	 * devices, so we ignore the information for those commands.  Note
	 * that this means we could be ignoring a real error on these
	 * commands, but that can't be helped.
	 */
	if ( psDisk->subclass == US_SC_UFI )
	{
		if ( srb->nCmd[0] == SCSI_REQUEST_SENSE || srb->nCmd[0] == SCSI_INQUIRY )
			return USB_STOR_TRANSPORT_GOOD;
		else if ( ( ( unsigned char * )psDisk->irq_urb->pBuffer )[0] )
			return USB_STOR_TRANSPORT_FAILED;
		else
			return USB_STOR_TRANSPORT_GOOD;
	}

	/* If not UFI, we interpret the data as a result code 
	 * The first byte should always be a 0x0
	 * The second byte & 0x0F should be 0x0 for good, otherwise error 
	 */
	if ( psDisk->irqdata[0] )
	{
		printk( "CBI IRQ data showed reserved bType %d\n", psDisk->irqdata[0] );
		return USB_STOR_TRANSPORT_ERROR;
	}

	switch ( psDisk->irqdata[1] & 0x0F )
	{
	case 0x00:
		return USB_STOR_TRANSPORT_GOOD;
	case 0x01:
		return USB_STOR_TRANSPORT_FAILED;
	default:
		return USB_STOR_TRANSPORT_ERROR;
	}

	/* we should never get here, but if we do, we're in trouble */
	return USB_STOR_TRANSPORT_ERROR;
}

/*
 * Control/Bulk transport
 */
int usb_stor_CB_transport( SCSI_cmd_s * srb, USB_disk_s * psDisk )
{
	int result;

	/* COMMAND STAGE */
	/* let's send the command via the control pipe */
	result = usb_stor_control_msg( psDisk, usb_sndctrlpipe( psDisk->pusb_dev, 0 ), US_CBI_ADSC, USB_TYPE_CLASS | USB_RECIP_INTERFACE, 0, psDisk->ifnum, srb->nCmd, srb->nCmdLen );

	/* check the return code for the command */
	//printk("Call to usb_stor_control_msg() returned %d\n", result);
	if ( result < 0 )
	{
		/* if the command was aborted, indicate that */
		if ( result == -ENOENT )
			return USB_STOR_TRANSPORT_ABORTED;

		/* a stall is a fatal condition from the device */
		if ( result == -EPIPE )
		{
			printk( "-- Stall on control pipe. Clearing\n" );
			result = usb_stor_clear_halt( psDisk->pusb_dev, usb_sndctrlpipe( psDisk->pusb_dev, 0 ) );
			printk( "-- usb_stor_clear_halt() returns %d\n", result );
			return USB_STOR_TRANSPORT_FAILED;
		}

		/* Uh oh... serious problem here */
		return USB_STOR_TRANSPORT_ERROR;
	}

	/* DATA STAGE */
	/* transfer the data payload for this command, if one exists */
	if ( usb_stor_transfer_length( srb ) )
	{
		usb_stor_transfer( srb, psDisk );
		printk( "CB data stage result is 0x%x\n", srb->nResult );

		/* if it was aborted, we need to indicate that */
		if ( srb->nResult == USB_STOR_TRANSPORT_ABORTED )
			return USB_STOR_TRANSPORT_ABORTED;
	}

	/* STATUS STAGE */
	/* NOTE: CB does not have a status stage.  Silly, I know.  So
	 * we have to catch this at a higher level.
	 */
	return USB_STOR_TRANSPORT_GOOD;
}

/*
 * Bulk only transport
 */

/* Determine what the maximum LUN supported is */
int usb_stor_Bulk_max_lun( USB_disk_s * psDisk )
{
	unsigned char data;
	int result;
	int pipe;

	/* issue the command */
	pipe = usb_rcvctrlpipe( psDisk->pusb_dev, 0 );
	result = g_psBus->control_msg( psDisk->pusb_dev, pipe, US_BULK_GET_MAX_LUN, USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 0, psDisk->ifnum, &data, sizeof( data ), 1000 * 1000 );

	//printk("GetMaxLUN command result is %d, data is %d\n", 
	//  result, data);

	/* if we have a successful request, return the result */
	if ( result == 1 )
		return data;

	/* if we get a STALL, clear the stall */
	if ( result == -EPIPE )
	{
		printk( "clearing endpoint halt for pipe 0x%x\n", pipe );
		usb_stor_clear_halt( psDisk->pusb_dev, pipe );
	}

	/* return the default -- no LUNs */
	return 0;
}

int usb_stor_Bulk_reset( USB_disk_s * psDisk );

int usb_stor_Bulk_transport( SCSI_cmd_s * srb, USB_disk_s * psDisk )
{
	struct bulk_cb_wrap bcb;
	struct bulk_cs_wrap bcs;
	int result;
	int pipe;
	int partial;

	/* if the device was removed, then we're already reset */
	if ( !psDisk->pusb_dev )
		return SCSI_SUCCESS;

	/* set up the command wrapper */
	bcb.Signature = US_BULK_CB_SIGN;
	bcb.DataTransferLength = usb_stor_transfer_length( srb );
	bcb.Flags = srb->nDirection == SCSI_DATA_READ ? 1 << 7 : 0;
	bcb.Tag = 0;		// ???
	bcb.Lun = srb->nCmd[1] >> 5;
	if ( psDisk->flags & US_FL_SCM_MULT_TARG )
		bcb.Lun |= srb->nDevice << 4;
	bcb.Length = srb->nCmdLen;

	/* construct the pipe handle */
	pipe = usb_sndbulkpipe( psDisk->pusb_dev, psDisk->ep_out );

	/* copy the command payload */
	memset( bcb.CDB, 0, sizeof( bcb.CDB ) );
	memcpy( bcb.CDB, srb->nCmd, bcb.Length );

	/* send it to out endpoint */
	/*printk("Bulk command S 0x%x T 0x%x Trg %d LUN %d L %d F %d CL %d\n",
	   bcb.Signature, bcb.Tag,
	   (bcb.Lun >> 4), (bcb.Lun & 0x0F), 
	   bcb.DataTransferLength, bcb.Flags, bcb.Length); */
	result = usb_stor_bulk_msg( psDisk, &bcb, pipe, US_BULK_CB_WRAP_LEN, &partial );
	//printk("Bulk command transfer result=%d\n", result);

	/* if the command was aborted, indicate that */
	if ( result == -ENOENT )
		return USB_STOR_TRANSPORT_ABORTED;

	/* if we stall, we need to clear it before we go on */
	if ( result == -EPIPE )
	{
		printk( "clearing endpoint halt for pipe 0x%x\n", pipe );
		usb_stor_clear_halt( psDisk->pusb_dev, pipe );
	}
	else if ( result )
	{
		/* unknown error -- we've got a problem */
		return USB_STOR_TRANSPORT_ERROR;
	}

	/* if the command transfered well, then we go to the data stage */
	if ( result == 0 )
	{
		/* send/receive data payload, if there is any */
		if ( bcb.DataTransferLength )
		{
			usb_stor_transfer( srb, psDisk );
			//printk("Bulk data transfer result 0x%x\n", 
			// srb->nResult);

			/* if it was aborted, we need to indicate that */
			if ( srb->nResult == USB_STOR_TRANSPORT_ABORTED )
				return USB_STOR_TRANSPORT_ABORTED;
		}
	}

	/* See flow chart on pg 15 of the Bulk Only Transport spec for
	 * an explanation of how this code works.
	 */

	/* construct the pipe handle */
	pipe = usb_rcvbulkpipe( psDisk->pusb_dev, psDisk->ep_in );

	/* get CSW for device status */
	//printk("Attempting to get CSW...\n");
	result = usb_stor_bulk_msg( psDisk, &bcs, pipe, US_BULK_CS_WRAP_LEN, &partial );

	/* if the command was aborted, indicate that */
	if ( result == -ENOENT )
		return USB_STOR_TRANSPORT_ABORTED;

	/* did the attempt to read the CSW fail? */
	if ( result == -EPIPE )
	{
		//printk("clearing endpoint halt for pipe 0x%x\n", pipe);
		usb_stor_clear_halt( psDisk->pusb_dev, pipe );

		/* get the status again */
		printk( "Attempting to get CSW (2nd try)...\n" );
		result = usb_stor_bulk_msg( psDisk, &bcs, pipe, US_BULK_CS_WRAP_LEN, &partial );

		/* if the command was aborted, indicate that */
		if ( result == -ENOENT )
			return USB_STOR_TRANSPORT_ABORTED;

		/* if it fails again, we need a reset and return an error */
		if ( result == -EPIPE )
		{
			printk( "clearing halt for pipe 0x%x\n", pipe );
			usb_stor_clear_halt( psDisk->pusb_dev, pipe );
			return USB_STOR_TRANSPORT_ERROR;
		}
	}

	/* if we still have a failure at this point, we're in trouble */
	//printk("Bulk status result = %d\n", result);
	if ( result )
	{
		return USB_STOR_TRANSPORT_ERROR;
	}

	/* check bulk status */
	/*printk("Bulk status Sig 0x%x T 0x%x R %d Stat 0x%x\n",
	   bcs.Signature, bcs.Tag, 
	   bcs.Residue, bcs.Status); */
	if ( bcs.Signature != US_BULK_CS_SIGN || bcs.Tag != bcb.Tag || bcs.Status > US_BULK_STAT_PHASE || partial != 13 )
	{
		printk( "Bulk logical error\n" );
		return USB_STOR_TRANSPORT_ERROR;
	}

	/* based on the status code, we report good or bad */
	switch ( bcs.Status )
	{
	case US_BULK_STAT_OK:
		/* command good -- note that data could be short */
		return USB_STOR_TRANSPORT_GOOD;

	case US_BULK_STAT_FAIL:
		/* command failed */
		return USB_STOR_TRANSPORT_FAILED;

	case US_BULK_STAT_PHASE:
		/* phase error -- note that a transport reset will be
		 * invoked by the invoke_transport() function
		 */
		return USB_STOR_TRANSPORT_ERROR;
	}

	/* we should never get here, but if we do, we're in trouble */
	return USB_STOR_TRANSPORT_ERROR;
}

/***********************************************************************
 * Reset routines
 ***********************************************************************/

/* This issues a CB[I] Reset to the device in question
 */
int usb_stor_CB_reset( USB_disk_s * psDisk )
{
	unsigned char cmd[12];
	int result;

	printk( "CB_reset() called\n" );

	/* if the device was removed, then we're already reset */
	if ( !psDisk->pusb_dev )
		return SCSI_SUCCESS;

	memset( cmd, 0xFF, sizeof( cmd ) );
	cmd[0] = SCSI_SEND_DIAGNOSTIC;
	cmd[1] = 4;
	result = g_psBus->control_msg( psDisk->pusb_dev, usb_sndctrlpipe( psDisk->pusb_dev, 0 ), US_CBI_ADSC, USB_TYPE_CLASS | USB_RECIP_INTERFACE, 0, psDisk->ifnum, cmd, sizeof( cmd ), 1000 * 1000 * 5 );

	if ( result < 0 )
	{
		printk( "CB[I] soft reset failed %d\n", result );
		return SCSI_FAILED;
	}

	/* long wait for reset */
	snooze( 6 * 1000 );

	printk( "CB_reset: clearing endpoint halt\n" );
	usb_stor_clear_halt( psDisk->pusb_dev, usb_rcvbulkpipe( psDisk->pusb_dev, psDisk->ep_in ) );
	usb_stor_clear_halt( psDisk->pusb_dev, usb_rcvbulkpipe( psDisk->pusb_dev, psDisk->ep_out ) );

	printk( "CB_reset done\n" );
	/* return a result code based on the result of the control message */
	return SCSI_SUCCESS;
}

/* This issues a Bulk-only Reset to the device in question, including
 * clearing the subsequent endpoint halts that may occur.
 */
int usb_stor_Bulk_reset( USB_disk_s * psDisk )
{
	int result;

	printk( "Bulk reset requested\n" );

	/* if the device was removed, then we're already reset */
	if ( !psDisk->pusb_dev )
		return SCSI_SUCCESS;

	result = g_psBus->control_msg( psDisk->pusb_dev, usb_sndctrlpipe( psDisk->pusb_dev, 0 ), US_BULK_RESET_REQUEST, USB_TYPE_CLASS | USB_RECIP_INTERFACE, 0, psDisk->ifnum, NULL, 0, 1000 * 1000 * 5 );

	if ( result < 0 )
	{
		printk( "Bulk soft reset failed %d\n", result );
		return SCSI_FAILED;
	}

	/* long wait for reset */
	snooze( 6 * 1000 );

	usb_stor_clear_halt( psDisk->pusb_dev, usb_rcvbulkpipe( psDisk->pusb_dev, psDisk->ep_in ) );
	usb_stor_clear_halt( psDisk->pusb_dev, usb_sndbulkpipe( psDisk->pusb_dev, psDisk->ep_out ) );
	printk( "Bulk soft reset completed\n" );
	return SCSI_SUCCESS;
}

