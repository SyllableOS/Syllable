//-----------------------------------------------------------------------------
#include <atheos/kernel.h>
#include <atheos/device.h>
#include <posix/errno.h>

#include "w8378x_driver.h"

//-----------------------------------------------------------------------------

status_t w8378x_open( void* pNode, uint32 nFlags, void **pCookie );
status_t w8378x_close( void* pNode, void* pCookie );
status_t w8378x_control( void* pNode, void* pCookie, uint32 op, void* arg, bool bFromKernel );

int w8378x_type;

#define inp  isa_readb
#define outp isa_writeb
#define kprintf printk
#define B_NO_ERROR 0

//-----------------------------------------------------------------------------


int w8378x_port = 0x290;
#define W8378x_ADDR_REG_OFFSET 5
#define W8378x_DATA_REG_OFFSET 6

#define W8378x_REG_TEMP				0x27
#define W8378x_REG_TEMP_OVER		0x39
#define W8378x_REG_TEMP_HYST		0x3A
#define W8378x_REG_TEMP_CONFIG		0x52
#define W8378x_REG_SCFG2			0x59

#define W8378x_REG_TEMP2_MSB		0x0150
#define W8378x_REG_TEMP2_LSB		0x0151
#define W8378x_REG_TEMP2_CONFIG		0x0152
#define W8378x_REG_TEMP2_HYST		0x0153
#define W8378x_REG_TEMP2_OVER		0x0155

#define W8378x_REG_TEMP3_MSB		0x0250
#define W8378x_REG_TEMP3_LSB		0x0251
#define W8378x_REG_TEMP3_CONFIG		0x0252
#define W8378x_REG_TEMP3_HYST		0x0253
#define W8378x_REG_TEMP3_OVER		0x0255

#define W8378x_REG_FAN1				0x0028
#define W8378x_REG_FAN2				0x0029
#define W8378x_REG_FAN3				0x002a

#define W8378x_REG_VID_FANDIV		0x47
#define W8378x_REG_PIN				0x4B
#define W8378x_REG_VBAT				0x5d


#define W8378x_REG_CONFIG		0x40
#define W8378x_REG_BANK			0x4E
#define W8378x_REG_CHIPMAN		0x4F
#define W8378x_REG_WCHIPID		0x58

#define W8378x_TYPEID_W83781D	0x10
#define W8378x_TYPEID_W83782D	0x30

uint16 w8378x_readvalue( uint16 reg )
{
	bool readword;
	uint16 retval;
	
	// register 50,53 and 55 in bank 1 and 2 should be read as 16 bit:
	readword = (((reg&0xff00)==0x0100)||((reg&0xff00)==0x0200)) &&
		(((reg&0x00ff)==0x0050)||((reg&0x00ff)==0x0053)||((reg&0x00ff)==0x0055));

	// set bank
	if( reg&0xff00 )
	{
		outp( w8378x_port+W8378x_ADDR_REG_OFFSET, W8378x_REG_BANK );
		outp( w8378x_port+W8378x_DATA_REG_OFFSET, reg >> 8);
	}

	// read msb
	outp( w8378x_port+W8378x_ADDR_REG_OFFSET, reg&0x00ff );
	retval = inp( w8378x_port+W8378x_DATA_REG_OFFSET );
//	dprintf( "in1:%d\n", (int)retval );

	if( readword )
	{
		// read lsb
		outp( w8378x_port+W8378x_ADDR_REG_OFFSET, (reg&0x00ff)+1 );
		retval <<= 8;
		retval |= inp( w8378x_port+W8378x_DATA_REG_OFFSET );
//		dprintf( "in2:%d\n", (int)retval&0xff );
	}

	// reset bank
	if( reg&0xff00 )
	{
		outp( w8378x_port+W8378x_ADDR_REG_OFFSET, W8378x_REG_BANK );
		outp( w8378x_port+W8378x_DATA_REG_OFFSET, 0);
	}

	return retval;
}

void w8378x_writevalue( uint16 reg, uint16 value )
{
	bool writeword;

	// register 50,53 and 55 in bank 1 and 2 should be written as 16 bit:
	writeword = (((reg&0xff00)==0x0100)||((reg&0xff00)==0x0200)) &&
		(((reg&0x00ff)==0x0050)||((reg&0x00ff)==0x0053)||((reg&0x00ff)==0x0055));

	// set bank
	if( reg&0xff00 )
	{
		outp( w8378x_port+W8378x_ADDR_REG_OFFSET, W8378x_REG_BANK );
		outp( w8378x_port+W8378x_DATA_REG_OFFSET, reg >> 8);
	}

	outp( w8378x_port+W8378x_ADDR_REG_OFFSET, reg&0x00ff );
	if( writeword )
	{
		// write lsb
		outp( w8378x_port+W8378x_DATA_REG_OFFSET, value>>8 );
		outp( w8378x_port+W8378x_ADDR_REG_OFFSET, (reg&0x00ff)+1 );
	}

	// write msb
	outp( w8378x_port+W8378x_DATA_REG_OFFSET, value&0x00ff );

	// reset bank
	if( reg&0xff00 )
	{
		outp( w8378x_port+W8378x_ADDR_REG_OFFSET, W8378x_REG_BANK );
		outp( w8378x_port+W8378x_DATA_REG_OFFSET, 0);
	}
}

//-----------------------------------------------------------------------------

status_t init_hardware()
{
  return 0;
}

static DeviceOperations_s g_sOperations = {
  w8378x_open,
  w8378x_close,
  w8378x_control,
  NULL, /* read */
  NULL	/* write */
};

status_t device_init( int nDeviceID )
{
	int i, j;
	int nHandle;

	kprintf( "w8378x>init_driver()\n" );

	// HW detection (not found in the manual, but the Linux driver):
	i = inp( w8378x_port+1 );
	if( inp(w8378x_port+2)!=i || inp(w8378x_port+3)!=i || inp(w8378x_port+7)!=i )
		goto error1;

	// Let's just hope nothing breaks here
	i = inp( w8378x_port+5) & 0x7f;
	outp( w8378x_port+5, (~i)&0x7f );
	if( (inp(w8378x_port+5)&0x7f) != ((~i)&0x7f) )
	{
		outp( w8378x_port+5, i );
		goto error1;
	}

	if( w8378x_readvalue(W8378x_REG_CONFIG) & 0x80 )
		goto error1;
	i = w8378x_readvalue( W8378x_REG_BANK );
	j = w8378x_readvalue( W8378x_REG_CHIPMAN );
	if( !(i&0x07) && ((!(i&0x80) && (j!=0xa3)) || ((i&0x80) && (j!=0x5c))) )
		goto error1;

	// We have either had a force parameter, or we have already detected the
	// Winbond. Put it now into bank 0
	w8378x_writevalue( W8378x_REG_BANK,w8378x_readvalue(W8378x_REG_BANK)&0xf8 );

	// Detecting winbond version
	i = w8378x_readvalue( W8378x_REG_WCHIPID);
	if( i==0x10 || i==0x11 )
	{
		// My W83781D manual says that the chipid is 0x10, but the
		// W83781D on my P2B-DS returns 0x11??!?
		w8378x_type = W8378x_TYPEID_W83781D;
		kprintf( "w8378x>init_driver(): found W83781D\n" );
	}
	else if( i==0x30 )
	{
		w8378x_type = W8378x_TYPEID_W83782D;
		kprintf( "w8378x>init_driver(): found W83782D\n" );
	}
	else
	{
		kprintf( "w8378x>init_driver(): found unknown winbond chip, id:%02X\n", i );
		goto error1;
	}
#if 0
	i = w8378x_readvalue( W8378x_REG_SCFG2 );
	kprintf( "%02X\n", i );
	i = 0;//&= ~0x20;
	kprintf( "%02X\n", i );
	w8378x_writevalue( W8378x_REG_SCFG2, i );
#endif
	nHandle = register_device( "", "system" );
	claim_device( nDeviceID, nHandle, "W8378x", DEVICE_SYSTEM );
	return( create_device_node( nDeviceID, nHandle, "misc/w8378x", &g_sOperations, NULL ) );
	
	return B_NO_ERROR;

error1:
	kprintf( "w8378x>init_driver(): could not find hardware\n" );
	return( -ENOENT );
}

int device_uninit( int nDeviceID )
{
  kprintf( "w83782d>uninit_driver()\n" );
  return( 0 );
}

//-----------------------------------------------------------------------------

status_t w8378x_open( void* pNode, uint32 nFlags, void **pCookie )
{
  return( 0 );
}

status_t w8378x_close( void* pNode, void* pCookie )
{
  return( 0 );
}

status_t w8378x_control( void* pNode, void* pCookie, uint32 op, void* arg, bool bFromKernel )
{
	switch( op )
	{
		case W8378x_READ_TEMP1:
			*((int*)arg) = ((int)(int8)w8378x_readvalue(W8378x_REG_TEMP))*256;
			return B_NO_ERROR;

		case W8378x_READ_TEMP2:
//			*((int*)arg) = 
//				((((int)((int8)w8378x_readvalue(W8378x_REG_TEMP2_MSB)))<<1) |
//				((w8378x_readvalue(W8378x_REG_TEMP2_LSB)>>7)&1))*128;
			*((int*)arg) = ((int)(int16)w8378x_readvalue(W8378x_REG_TEMP2_MSB));
			return B_NO_ERROR;

		case W8378x_READ_TEMP3:
//			*((int*)arg) = 
//				((((int)((int8)w8378x_readvalue(W8378x_REG_TEMP3_MSB)))<<1) |
//				((w8378x_readvalue(W8378x_REG_TEMP3_LSB)>>7)&1))*128;
			*((int*)arg) = ((int)(int16)w8378x_readvalue(W8378x_REG_TEMP3_MSB));
				return B_NO_ERROR;
			return B_NO_ERROR;

		case W8378x_READ_FAN1:
		{
			int fandiv, rpm;
			fandiv = (w8378x_readvalue(W8378x_REG_VID_FANDIV)>>4)&0x03;
			if( w8378x_type==W8378x_TYPEID_W83782D )
				fandiv |= ((w8378x_readvalue(W8378x_REG_VBAT)>>5)&1)<<2;
			fandiv = 1<<fandiv;
			rpm = w8378x_readvalue( W8378x_REG_FAN1 );
			if( rpm==0 || rpm==255 )
				return -EINVAL;
			rpm = 1350000/(rpm*fandiv);
			*((int*)arg) = rpm;
			return B_NO_ERROR;
		}

		case W8378x_READ_FAN2:
		{
			int fandiv, rpm;
			fandiv = (w8378x_readvalue(W8378x_REG_VID_FANDIV)>>6)&0x03;
			if( w8378x_type==W8378x_TYPEID_W83782D )
				fandiv |= ((w8378x_readvalue(W8378x_REG_VBAT)>>6)&1)<<2;
			fandiv = 1<<fandiv;
			rpm = w8378x_readvalue( W8378x_REG_FAN2 );
			if( rpm==0 || rpm==255 )
				return -EINVAL;
			rpm = 1350000/(rpm*fandiv);
			*((int*)arg) = rpm;
			return B_NO_ERROR;
		}

		case W8378x_READ_FAN3:
		{
			int fandiv, rpm;
			fandiv = (w8378x_readvalue(W8378x_REG_PIN)>>6)&0x03;
			if( w8378x_type==W8378x_TYPEID_W83782D )
				fandiv |= ((w8378x_readvalue(W8378x_REG_VBAT)>>7)&1)<<2;
			fandiv = 1<<fandiv;
			rpm = w8378x_readvalue( W8378x_REG_FAN3 );
			if( rpm==0 || rpm==255 )
				return -EINVAL;
			rpm = 1350000/(rpm*fandiv);
			*((int*)arg) = rpm;
			return B_NO_ERROR;
		}
	}

	return -EINVAL;
}

