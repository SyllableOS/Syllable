#define PCI_VENDOR_ID_ESS			0x125D

#define PCI_DEVICE_ID_ESS_ESS1968	0x1968		/* Maestro 2	*/
#define PCI_DEVICE_ID_ESS_ESS1978   0x1978		/* Maestro 2E	*/

#define PCI_VENDOR_ESS_OLD			0x1285		/* Platform Tech, the people the maestro  was bought from */
#define PCI_DEVICE_ID_ESS_ESS0100	0x0100		/* Maestro 1 */

/* NEC Versas ? */
#define NEC_VERSA_SUBID1			0x80581033
#define NEC_VERSA_SUBID2			0x803c1033

#define	PCI_CLASS_MULTIMEDIA_AUDIO	0x0401

/* changed so that I could actually find all the
	references and fix them up.  it's a little more readable now. */
#define ESS_FMT_STEREO	0x01
#define ESS_FMT_16BIT	0x02
#define ESS_FMT_MASK	0x03
#define ESS_DAC_SHIFT	0   
#define ESS_ADC_SHIFT	4

#define DAC_RUNNING		1
#define ADC_RUNNING		2

#define ESS_STATE_MAGIC		0x125D1968
#define ESS_CARD_MAGIC		0x19283746

#define ESS_CHAN_HARD		0x100

/*
 *	Registers for the ESS PCI cards
 */
 
/*
 *	Memory access
 */
 
#define ESS_MEM_DATA		0x00
#define	ESS_MEM_INDEX		0x02

/*
 *	AC-97 Codec port. Delay 1uS after each write. This is used to
 *	talk AC-97 (see intel.com). Write data then register.
 */
 
#define ESS_AC97_INDEX		0x30		/* byte wide */
#define ESS_AC97_DATA		0x32

/* 
 *	Reading is a bit different. You write register|0x80 to ubdex
 *	delay 1uS poll the low bit of index, when it clears read the
 *	data value.
 */

/*
 *	Control port. Not yet fully understood
 *	The value 0xC090 gets loaded to it then 0x0000 and 0x2800
 *	to the data port. Then after 4uS the value 0x300 is written
 */
 
#define RING_BUS_CTRL_L		0x34
#define RING_BUS_CTRL_H		0x36

/*
 *	This is also used during setup. The value 0x17 is written to it
 */
 
#define ESS_SETUP_18		0x18

/*
 *	And this one gets 0x000b
 */
 
#define ESS_SETUP_A2		0xA2

/*
 *	And this 0x0000
 */
 
#define ESS_SETUP_A4		0xA4
#define ESS_SETUP_A6		0xA6

/*
 *	Stuff to do with Harpo - the wave stuff
 */
 
#define ESS_WAVETABLE_SIZE	0x14
#define 	ESS_WAVETABLE_2M	0xA180

