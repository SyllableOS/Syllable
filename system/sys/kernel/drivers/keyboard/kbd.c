/*
 *  The AtheOS kernel
 *  Copyright (C) 1999  Kurt Skauen
 *  Copyright (C) 2002  Kristian Van Der Vliet (vanders@users.sourceforge.net)
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
#include <posix/stat.h>
#include <posix/fcntl.h>
#include <posix/dirent.h>

#include <atheos/types.h>
#include <atheos/isa_io.h>

#include <atheos/kernel.h>
#include <atheos/device.h>
#include <atheos/irq.h>
#include <atheos/smp.h>

#include <macros.h>

static const uint8 g_anRawKeyTab[] =
{
    0x00,	/*	NO KEY	*/
    0x01,	/* 1   ESC	*/
    0x12,	/* 2   1	*/
    0x13,	/* 3   2	*/
    0x14,	/* 4   3	*/
    0x15,	/* 5   4	*/
    0x16,	/* 6   5	*/
    0x17,	/* 7   6	*/
    0x18,	/* 8   7	*/
    0x19,	/* 9   8	*/
    0x1a,	/* 10   9	*/
    0x1b,	/* 11   0	*/
    0x1c,	/* 12   -	*/
    0x1d,	/* 13   =	*/
    0x1e,	/* 14   BACKSPACE	*/
    0x26,	/* 15   TAB	*/
    0x27,	/* 16   Q	*/
    0x28,	/* 17   W	*/
    0x29,	/* 18   E	*/
    0x2a,	/* 19   R	*/
    0x2b,	/* 20   T	*/
    0x2c,	/* 21   Y	*/
    0x2d,	/* 22   U	*/
    0x2e,	/* 23   I	*/
    0x2f,	/* 24   O	*/
    0x30,	/* 25   P	*/
    0x31,	/* 26   [ {	*/
    0x32,	/* 27   ] }	*/
    0x47,	/* 28   ENTER (RETURN)	*/
    0x5c,	/* 29   LEFT CONTROL	*/
    0x3c,	/* 30   A	*/
    0x3d,	/* 31   S	*/
    0x3e,	/* 32   D	*/
    0x3f,	/* 33   F	*/
    0x40,	/* 34   G	*/
    0x41,	/* 35   H	*/
    0x42,	/* 36   J	*/
    0x43,	/* 37   K	*/
    0x44,	/* 38   L	*/
    0x45,	/* 39   ; :	*/
    0x46,	/* 40   ' "	*/
    0x11,	/* 41   ` ~	*/
    0x4b,	/* 42   LEFT SHIFT	*/
    0x33,	/* 43 	NOTE : This key code was not defined in the original table! (' *)	*/
    0x4c,	/* 44   Z	*/
    0x4d,	/* 45   X	*/
    0x4e,	/* 46   C	*/
    0x4f,	/* 47   V	*/
    0x50,	/* 48   B	*/
    0x51,	/* 49   N	*/
    0x52,	/* 50   M	*/
    0x53,	/* 51   , <	*/
    0x54,	/* 52   . >	*/
    0x55,	/* 53   / ?	*/
    0x56,	/* 54   RIGHT SHIFT	*/
    0x24,	/* 55   *            (KEYPAD)	*/
    0x5d,	/* 56   LEFT ALT	*/
    0x5e,	/* 57   SPACEBAR	*/
    0x3b,	/* 58   CAPSLOCK	*/
    0x02,	/* 59   F1	*/
    0x03,	/* 60   F2	*/
    0x04,	/* 61   F3	*/
    0x05,	/* 62   F4	*/
    0x06,	/* 63   F5	*/
    0x07,	/* 64   F6	*/
    0x08,	/* 65   F7	*/
    0x09,	/* 66   F8	*/
    0x0a,	/* 67   F9	*/
    0x0b,	/* 68   F10	*/
    0x22,	/* 69   NUMLOCK      (KEYPAD)	*/
    0x0f,	/* 70   SCROLL LOCK	*/
    0x37,	/* 71   7 HOME       (KEYPAD)	*/
    0x38,	/* 72   8 UP         (KEYPAD)	*/
    0x39,	/* 73   9 PGUP       (KEYPAD)	*/
    0x25,	/* 74   -            (KEYPAD)	*/
    0x48,	/* 75   4 LEFT       (KEYPAD)	*/
    0x49,	/* 76   5            (KEYPAD)	*/
    0x4a,	/* 77   6 RIGHT      (KEYPAD)	*/
    0x3a,	/* 78   +            (KEYPAD)	*/
    0x58,	/* 79   1 END        (KEYPAD)	*/
    0x59,	/* 80   2 DOWN       (KEYPAD)	*/
    0x5a,	/* 81   3 PGDN       (KEYPAD)	*/
    0x64,	/* 82   0 INSERT     (KEYPAD)	*/
    0x65,	/* 83   . DEL        (KEYPAD)	*/
    0x7e,	/* 84		SYSRQ	*/
    0x00,	/* 85	*/
    0x69,	/* 86 	NOTE : This key code was not defined in the original table! (< >)	*/
    0x0c,	/* 87   F11	*/
    0x0d	/* 88   F12	*/
};

static const uint8 g_anExtRawKeyTab[] =
{
    0x00,	/*	0	*/
    0x00,	/*	1	*/
    0x00,	/*		2	*/
    0x00,	/*		3	*/
    0x00,	/*		4	*/
    0x00,	/*		5	*/
    0x00,	/*		6	*/
    0x00,	/*		7	*/
    0x00,	/*		8	*/
    0x00,	/*		9	*/
    0x00,	/*		10	*/
    0x00,	/*		11	*/
    0x00,	/*		12	*/
    0x00,	/*		13	*/
    0x00,	/*		14	*/
    0x00,	/*		15	*/
    0x00,	/*		16	*/
    0x00,	/*		17	*/
    0x00,	/*		18	*/
    0x00,	/*		19	*/
    0x00,	/*		20	*/
    0x00,	/*		21	*/
    0x00,	/*		22	*/
    0x00,	/*		23	*/
    0x00,	/*		24	*/
    0x00,	/*		25	*/
    0x00,	/*		26	*/
    0x00,	/*		27	*/
    0x5b,	/*		28   ENTER        (KEYPAD)	*/
    0x60,	/*		29   RIGHT CONTROL	*/
    0x00,	/*		30	*/
    0x00,	/*		31	*/
    0x00,	/*		32	*/
    0x00,	/*		33	*/
    0x00,	/*		34	*/
    0x00,	/*		35	*/
    0x00,	/*		36	*/
    0x00,	/*		37	*/
    0x00,	/*		38	*/
    0x00,	/*		39	*/
    0x00,	/*		40	*/
    0x00,	/*		41	*/
    0x00,	/*		42   PRINT SCREEN (First code)	*/
    0x00,	/*		43	*/
    0x00,	/*		44	*/
    0x00,	/*		45	*/
    0x00,	/*		46	*/
    0x00,	/*		47	*/
    0x00,	/*		48	*/
    0x00,	/*		49	*/
    0x00,	/*		50	*/
    0x00,	/*		51	*/
    0x00,	/*		52	*/
    0x23,	/*		53   /            (KEYPAD)	*/
    0x00,	/*		54	*/
    0x0e,	/*		55   PRINT SCREEN (Second code)	*/
    0x5f,	/*		56   RIGHT ALT	*/
    0x00,	/*		57	*/
    0x00,	/*		58	*/
    0x00,	/*		59	*/
    0x00,	/*		60	*/
    0x00,	/*		61	*/
    0x00,	/*		62	*/
    0x00,	/*		63	*/
    0x00,	/*		64	*/
    0x00,	/*		65	*/
    0x00,	/*		66	*/
    0x00,	/*		67	*/
    0x00,	/*		68	*/
    0x00,	/*		69	*/
    0x7f,	/*		70	 BREAK	*/
    0x20,	/*		71   HOME         (NOT KEYPAD)	*/
    0x57,	/*		72   UP           (NOT KEYPAD)	*/
    0x21,	/*		73   PAGE UP      (NOT KEYPAD)	*/
    0x00,	/*		74	*/
    0x61,	/*		75   LEFT         (NOT KEYPAD)	*/
    0x00,	/*		76	*/
    0x63,	/*		77   RIGHT        (NOT KEYPAD)	*/
    0x00,	/*		78	*/
    0x35,	/*		79   END          (NOT KEYPAD)	*/
    0x62,	/*		80   DOWN         (NOT KEYPAD)	*/
    0x36,	/*		81   PAGE DOWN    (NOT KEYPAD)	*/
    0x1f,	/*		82   INSERT       (NOT KEYPAD)	*/
    0x34,	/*		83   DELETE       (NOT KEYPAD)	*/
    0x00,	/*		84	*/
    0x00,	/*		85	*/
    0x00,	/*		86	*/
    0x00,	/*		87	*/
    0x00,	/*		88	*/
    0x00,	/*		89	*/
    0x00,	/*		90	*/
    0x00,	/*		91	*/
    0x00,	/*		92	*/
    0x00,	/*		93	*/
    0x00,	/*		94	*/
    0x00,	/*		95	*/
    0x00,	/*		96	*/
    0x00,	/*		97	*/
    0x00,	/*		98	*/
    0x00,	/*		99	*/
    0x00,	/*		100	*/
    0x00,	/*		101	*/
    0x00,	/*		102	*/
    0x00,	/*		103	*/
    0x00,	/*		104	*/
    0x00,	/*		105	*/
    0x00,	/*		106	*/
    0x00,	/*		107	*/
    0x00,	/*		108	*/
    0x00,	/*		109	*/
    0x00,	/*		110	*/
    0x00,	/*		111   MACRO	*/
    0x00,	/*		112	*/
    0x00,	/*		113	*/
    0x00,	/*		114	*/
    0x00,	/*		115	*/
    0x00,	/*		116	*/
    0x00,	/*		117	*/
    0x00,	/*		118	*/
    0x00,	/*		119	*/
    0x00,	/*		120	*/
    0x00,	/*		121	*/
    0x00,	/*		122	*/
    0x00,	/*		123	*/
    0x00,	/*		124	*/
    0x00,	/*		125	*/
    0x00,	/*		126	*/
    0x00,	/*		127	*/
    0x00,	/*		128	*/
    0x00,	/*		129	*/

    0x00,	/*		130	*/
    0x00,	/*		131	*/
    0x00,	/*		132	*/
    0x00,	/*		133	*/
    0x00,	/*		134	*/
    0x00,	/*		135	*/
    0x00,	/*		136	*/
    0x00,	/*		137	*/
    0x00,	/*		138	*/
    0x00,	/*		139	*/
    0x00,	/*		130	*/

    0x00,	/*		1	*/
    0x00,	/*		2	*/
    0x00,	/*		3	*/
    0x00,	/*		4	*/
    0x00,	/*		5	*/
    0x00,	/*		6	*/
    0x00,	/*		7	*/
    0x00,	/*		8	*/
    0x00,	/*		9	*/

    0x00,	/*		150	*/
    0x00,	/*		151	*/
    0x00,	/*		152	*/
    0x00,	/*		153	*/
    0x00,	/*		154	*/
    0x00,	/*		155	*/
    0x00,	/*		156	*/
    0x00,	/*		157	*/
    0x00,	/*		158	*/
    0x00,	/*		159	*/

    0x00,	/*		0	*/
    0x00,	/*		1	*/
    0x00,	/*		2	*/
    0x00,	/*		3	*/
    0x00,	/*		4	*/
    0x00,	/*		5	*/
    0x00,	/*		6	*/
    0x00,	/*		7	*/
    0x00,	/*		8	*/
    0x00,	/*		9	*/

    0x00,	/*		170	*/
    0x00,	/*		171	*/
    0x00,	/*		172	*/
    0x00,	/*		173	*/
    0x00,	/*		174	*/
    0x00,	/*		175	*/
    0x00,	/*		176	*/
    0x00,	/*		177	*/
    0x00,	/*		178	*/
    0x00,	/*		179	*/

    0x00,	/*		0	*/
    0x00,	/*		1	*/
    0x00,	/*		2	*/
    0x00,	/*		3	*/
    0x00,	/*		4	*/
    0x00,	/*		5	*/
    0x00,	/*		6	*/
    0x00,	/*		7	*/
    0x00,	/*		8	*/
    0x00,	/*		9	*/

    0x00,	/*		190	*/
    0x00,	/*		191	*/
    0x00,	/*		192	*/
    0x00,	/*		193	*/
    0x00,	/*		194	*/
    0x00,	/*		195	*/
    0x00,	/*		196	*/
    0x00,	/*		197	*/
    0x00,	/*		198	*/
    0x00,	/*		199	*/

    0x00,	/*		0	*/
    0x00,	/*		1	*/
    0x00,	/*		2	*/
    0x00,	/*		3	*/
    0x00,	/*		4	*/
    0x00,	/*		5	*/
    0x00,	/*		6	*/
    0x00,	/*		7	*/
    0x00,	/*		8	*/
    0x00,	/*		9	*/

    0x00,	/*		210	*/
    0x00,	/*		211	*/
    0x00,	/*		212	*/
    0x00,	/*		213	*/
    0x00,	/*		214	*/
    0x00,	/*		215	*/
    0x00,	/*		216	*/
    0x00,	/*		217	*/
    0x00,	/*		218	*/
    0x00,	/*		219	*/

    0x00,	/*		0	*/
    0x00,	/*		1	*/
    0x00,	/*		2	*/
    0x00,	/*		3	*/
    0x00,	/*		4	*/
    0x00,	/*		5	*/
    0x00,	/*		6	*/
    0x00,	/*		7	*/
    0x00,	/*		8	*/
    0x00,	/*		9	*/

    0x00,	/*		230	*/
    0x00,	/*		231	*/
    0x00,	/*		232	*/
    0x00,	/*		233	*/
    0x00,	/*		234	*/
    0x00,	/*		235	*/
    0x00,	/*		236	*/
    0x00,	/*		237	*/
    0x00,	/*		238	*/
    0x00,	/*		239	*/

    0x00,	/*		0	*/
    0x00,	/*		1	*/
    0x00,	/*		2	*/
    0x00,	/*		3	*/
    0x00,	/*		4	*/
    0x00,	/*		5	*/
    0x00,	/*		6	*/
    0x00,	/*		7	*/
    0x00,	/*		8	*/
    0x00,	/*		9	*/

    0x00,	/*		250	*/
    0x00,	/*		251	*/
    0x00,	/*		252	*/
    0x00,	/*		253	*/
    0x00,	/*		254	*/
    0x00,	/*		255	*/
    0x00,	/*		256	*/
};



typedef struct
{
    uint32	nFlags;
} KbdFileCookie_s;

typedef struct
{
    thread_id hWaitThread;
    char	    zBuffer[ 256 ];
    atomic_t	    nOutPos;
    atomic_t	    nInPos;
    atomic_t	    nBytesReceived;
    atomic_t	    nOpenCount;
    int	    nDevNum;
    int	    nIrqHandle;
} KbdVolume_s;

static KbdVolume_s	g_sVolume;
static unsigned char g_nKbdLedStatus;

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int kbd_open( void* pNode, uint32 nFlags, void **ppCookie )
{
    int	nError;
	
    if ( 0 != atomic_read( &g_sVolume.nOpenCount ) )
    {
	nError = -EBUSY;
	printk( "ERROR : Attempt to reopen keyboard device\n" );
    }
    else
    {
	KbdFileCookie_s* psCookie = kmalloc( sizeof( KbdFileCookie_s ), MEMF_KERNEL | MEMF_CLEAR );
		
	if ( NULL != psCookie )
	{
	    *ppCookie = psCookie;
	    psCookie->nFlags = nFlags;
		
	    atomic_inc( &g_sVolume.nOpenCount );
	    nError = 0;
	}
	else
	{
	    nError = -ENOMEM;
	}
    }
    return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int kbd_close( void* pNode, void* pCookie )
{
    KbdFileCookie_s* 	psCookie = pCookie;
	
    kassertw( NULL != psCookie );
	
    atomic_dec( &g_sVolume.nOpenCount );

    kfree( psCookie );
	
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static status_t kbd_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
	int nFlags;

	nFlags = cli();

	switch( nCommand )
	{
		case IOCTL_KBD_LEDRST:
			g_nKbdLedStatus=0;
			break;

		case IOCTL_KBD_SCRLOC:
			g_nKbdLedStatus ^= 0x01;
			break;

		case IOCTL_KBD_NUMLOC:
			g_nKbdLedStatus ^= 0x02;
			break;

		case IOCTL_KBD_CAPLOC:
			g_nKbdLedStatus ^= 0x04;
			break;

		default:
			printk( "Keyboard device: Unknown IOCTL %x\n",(int)nCommand );
			break;
	}

	while(inb(0x64) & 0x02);			/* Wait for the command buffer to empty	*/

	outb_p(0xed, 0x60);				/* Write the command and wait for it to	*/
	while(inb(0x64) & 0x02);			/* appear in the command buffer.			*/

	outb_p(g_nKbdLedStatus, 0x60);	/* Write the LED status to the keyboard	*/

	put_cpu_flags( nFlags );

	return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int kbd_read( void* pNode, void* pCookie, off_t nPos, void* pBuf, size_t nLen )
{
    KbdVolume_s* 	psVolume = &g_sVolume;
    int		nError;

    if ( 0 == nLen ) {
	return( 0 );
    }
	
    for ( ;; )
    {
	if ( atomic_read( &psVolume->nBytesReceived ) > 0 )
	{
	    int	nSize = min( nLen, atomic_read( &psVolume->nBytesReceived ) );
	    int	i;
	    char*	pzBuf = pBuf;

	    for ( i = 0 ; i < nSize ; ++i ) {
		pzBuf[ i ] = psVolume->zBuffer[ atomic_inc_and_read( &psVolume->nOutPos ) & 0xff ];
	    }
	    atomic_sub( &g_sVolume.nBytesReceived, nSize );
			
	    nError = nSize;
	}
	else
	{
	    int	nEFlg = cli();

	    if ( -1 != psVolume->hWaitThread )
	    {
		nError = -EBUSY;
		printk( "ERROR : two threads attempted to read from keyboard device!\n" );
	    }
	    else
	    {
		if ( 1 )
		{
		    nError = -EWOULDBLOCK;
		}
		else
		{
		    psVolume->hWaitThread = get_thread_id(NULL);
		    nError = suspend();
		    psVolume->hWaitThread = -1;
		}
	    }
	    put_cpu_flags( nEFlg );
	}
	if ( 0 != nError ) {
	    break;
	}
    }
    return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

DeviceOperations_s g_sOperations = {
    kbd_open,
    kbd_close,
    kbd_ioctl,
    kbd_read,
    NULL		/* write */
};

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static uint ConvertKeyCode( uint8 nCode )
{
    uint	     nKey;
    int	     nFlg;
    static int nLastCode      = 0;
    static int nLastKey       = 0;
    static int nPauseKeyCount = 0;

	
    if ( nPauseKeyCount > 0 )
    {
	nPauseKeyCount++;

	if ( nPauseKeyCount == 6 ) {
	    nPauseKeyCount	=	0;
	    return( 0x10 );		/* PAUSE	*/
	} else {
	    return( 0 );
	}
    }

    if ( 0xe1 == nCode ) {
	nPauseKeyCount = 1;
	return( 0 );
    }

    nFlg = nCode & 0x80;

    if ( 0xe0 == nLastCode ) {
	nKey = g_anExtRawKeyTab[ nCode & ~0x80 ];
    } else {
	if ( 0xe0 != nCode ) {
	    nKey = g_anRawKeyTab[ nCode & ~0x80 ];
	} else {
	    nKey = 0;
	}
    }

    nLastCode = nCode;

    if ( (nKey | nFlg) == nLastKey || 0 == nKey ) {
	return( 0 );
    }
    nLastKey = nKey | nFlg;

    return( nKey | nFlg );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int kbd_irq( int nIrqNum, void* pData, SysCallRegs_s* psRegs )
{
    int	nCode;
    int	nEFlg;
    int n;
	
    nCode = inb_p( 0x60 );

    nEFlg = cli();
    n = inb_p( 0x61 );
    outb_p( n | 0x80, 0x61 );
    outb_p( n & ~0x80, 0x61 );

    nCode = ConvertKeyCode( nCode );

    if ( 0 != nCode )
    {
	g_sVolume.zBuffer[ atomic_inc_and_read( &g_sVolume.nInPos ) & 0xff ] = nCode;
    
	atomic_inc( &g_sVolume.nBytesReceived );

	if ( -1 != g_sVolume.hWaitThread ) {
	    wakeup_thread( g_sVolume.hWaitThread, false );
	}
    }
    put_cpu_flags( nEFlg );
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t device_init( int nDeviceID )
{
    int nError;
    int nHandle;
    printk( "Keyboard device: device_init() called\n" );

    g_sVolume.hWaitThread    = -1;
    atomic_set( &g_sVolume.nInPos, 0 );
    atomic_set( &g_sVolume.nOutPos, 0 );
    atomic_set( &g_sVolume.nBytesReceived, 0 );
    atomic_set( &g_sVolume.nOpenCount, 0 );
    g_sVolume.nIrqHandle	   = request_irq( 1, kbd_irq, NULL, 0, "keyboard_device", NULL );

    g_nKbdLedStatus = 0;
  
    if ( g_sVolume.nIrqHandle < 0 ) {
	printk( "ERROR : Failed to initiate keyboard interrupt handler\n" );
	return( g_sVolume.nIrqHandle );
    }
    
	nHandle = register_device( "", "system" );
	claim_device( nDeviceID, nHandle , "Keyboard", DEVICE_INPUT );
    nError = create_device_node( nDeviceID, nHandle, "keybd", &g_sOperations, NULL );
    
    return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t device_uninit( int nDeviceID )
{
    release_irq( 1, g_sVolume.nIrqHandle );
    printk( "Test device: device_uninit() called\n" );
    return( 0 );
}

