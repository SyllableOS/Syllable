/*
 *  ATA/ATAPI driver for Syllable
 *
 *  Timing calculation helpers
 *
 *  Copyright (C) 2004 Arno Klenke
 *  Copyright (c) 1999-2001 Vojtech Pavlik
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
 
#ifndef __ATA_TIMING_H_
#define __ATA_TIMING_H_

typedef struct 
{
	int nMode;
	uint16 nSetup;
	uint16 nAct8b;
	uint16 nRec8b;
	uint16 nCyc8b;
	uint16 nAct;
	uint16 nRec;
	uint16 nCyc;
	uint16 nUDMA;
} ATA_timing_s;


static ATA_timing_s sTimings[] = {

	{ ATA_SPEED_UDMA_6,     0,   0,   0,   0,   0,   0,   0,  15 },
	{ ATA_SPEED_UDMA_5,     0,   0,   0,   0,   0,   0,   0,  20 },
	{ ATA_SPEED_UDMA_4,     0,   0,   0,   0,   0,   0,   0,  30 },
	{ ATA_SPEED_UDMA_3,     0,   0,   0,   0,   0,   0,   0,  45 },

	{ ATA_SPEED_UDMA_2,     0,   0,   0,   0,   0,   0,   0,  60 },
	{ ATA_SPEED_UDMA_1,     0,   0,   0,   0,   0,   0,   0,  80 },
	{ ATA_SPEED_UDMA_0,     0,   0,   0,   0,   0,   0,   0, 120 },
                                     
	{ ATA_SPEED_MWDMA_2,  25,   0,   0,   0,  70,  25, 120,   0 },
	{ ATA_SPEED_MWDMA_1,  45,   0,   0,   0,  80,  50, 150,   0 },
	{ ATA_SPEED_MWDMA_0,  60,   0,   0,   0, 215, 215, 480,   0 },
                                          
	{ ATA_SPEED_PIO_5,     20,  50,  30, 100,  50,  30, 100,   0 },
	{ ATA_SPEED_PIO_4,     25,  70,  25, 120,  70,  25, 120,   0 },
	{ ATA_SPEED_PIO_3,     30,  80,  70, 180,  80,  70, 180,   0 },
	
	{ ATA_SPEED_PIO,     30, 290,  40, 330, 100,  90, 240,   0 },

	{ -1 }
};

#define ATA_TIMING_SETUP	0x01
#define ATA_TIMING_ACT8B	0x02
#define ATA_TIMING_REC8B	0x04
#define ATA_TIMING_CYC8B	0x08
#define ATA_TIMING_8BIT		0x0e
#define ATA_TIMING_ACTIVE	0x10
#define ATA_TIMING_RECOVER	0x20
#define ATA_TIMING_CYCLE	0x40
#define ATA_TIMING_UDMA		0x80
#define ATA_TIMING_ALL		0xff

#define MIN(a,b)	((a)<(b)?(a):(b))
#define MAX(a,b)	((a)>(b)?(a):(b))
#define FIT(v,min,max)	MAX(MIN(v,max),min)
#define ENOUGH(v,unit)	(((v)-1)/(unit)+1)
#define EZ(v,unit)	((v)?ENOUGH(v,unit):0)

static void ata_timing_quantize( ATA_timing_s *psTiming, ATA_timing_s *psResult, int T, int UT )
{
	psResult->nSetup   = EZ(psTiming->nSetup   * 1000,  T);
	psResult->nAct8b   = EZ(psTiming->nAct8b   * 1000,  T);
	psResult->nRec8b   = EZ(psTiming->nRec8b   * 1000,  T);
	psResult->nCyc8b   = EZ(psTiming->nCyc8b   * 1000,  T);
	psResult->nAct     = EZ(psTiming->nAct     * 1000,  T);
	psResult->nRec     = EZ(psTiming->nRec     * 1000,  T);
	psResult->nCyc     = EZ(psTiming->nCyc     * 1000,  T);
	psResult->nUDMA    = EZ(psTiming->nUDMA    * 1000, UT);
}


static void ata_timing_merge( ATA_timing_s *psFirst,  ATA_timing_s *psSecond, ATA_timing_s *psResult, unsigned int what )
{
	if (what & ATA_TIMING_SETUP  ) psResult->nSetup   = MAX(psFirst->nSetup,   psSecond->nSetup);
	if (what & ATA_TIMING_ACT8B  ) psResult->nAct8b   = MAX(psFirst->nAct8b,   psSecond->nAct8b);
	if (what & ATA_TIMING_REC8B  ) psResult->nRec8b   = MAX(psFirst->nRec8b,   psSecond->nRec8b);
	if (what & ATA_TIMING_CYC8B  ) psResult->nCyc8b   = MAX(psFirst->nCyc8b,   psSecond->nCyc8b);
	if (what & ATA_TIMING_ACTIVE ) psResult->nAct     = MAX(psFirst->nAct,  psSecond->nAct);
	if (what & ATA_TIMING_RECOVER) psResult->nRec     = MAX(psFirst->nRec, psSecond->nRec);
	if (what & ATA_TIMING_CYCLE  ) psResult->nCyc     = MAX(psFirst->nCyc,   psSecond->nCyc);
	if (what & ATA_TIMING_UDMA   ) psResult->nUDMA    = MAX(psFirst->nUDMA,    psSecond->nUDMA);
}

static ATA_timing_s* ata_timing_find_mode(short speed)
{
	ATA_timing_s *t;

	for (t = sTimings; t->nMode != speed; t++)
		if (t->nMode < 0)
			return NULL;
	return t; 
}

static int ata_timing_compute( ATA_port_s* psPort, int nSpeed, ATA_timing_s* psResult, int T, int UT)
{
	ATA_timing_s *s, p;
	int i;
	int nPIOMode = -1;
	
	if (!(s = ata_timing_find_mode(nSpeed)))
		return -EINVAL;

/*
 * If the drive is an EIDE drive, it can tell us it needs extended
 * PIO/MWDMA cycle timing.
 */

	if ( psPort->sID.valid & 2 ) {	/* EIDE drive */

		memset(&p, 0, sizeof(p));
		
		if( nSpeed == ATA_SPEED_PIO )
			p.nCyc = p.nCyc8b = psPort->sID.eide_pio;
		else if( nSpeed <= ATA_SPEED_PIO_5 )
			p.nCyc = p.nCyc8b = psPort->sID.eide_pio_iordy;
		else if( nSpeed <= ATA_SPEED_MWDMA_2 )
			p.nCyc = psPort->sID.eide_dma_min;

		
		ata_timing_merge( &p, psResult, psResult, ATA_TIMING_CYCLE | ATA_TIMING_CYC8B);
	}

/*
 * Convert the timing to bus clock counts.
 */

	ata_timing_quantize( s, psResult, T, UT );
	
/*
 * Find best pio mode
 */
	if( nSpeed > ATA_SPEED_PIO_5 )
	{
		for( i = ATA_SPEED_PIO_5; i >= 0; i-- )
		{
			if( ( psPort->nSupportedPortSpeed & ( 1 << i ) ) &&
				( psPort->nSupportedDriveSpeed & ( 1 << i ) ) )
			{
				nPIOMode = i;
				break;
			}
		}
	
		if( nPIOMode < 0 )
		{
			printk( "Could not find a supported PIO mode\n" );
			return( -EINVAL );
		}
		ata_timing_compute( psPort, nPIOMode, &p, T, UT );
		ata_timing_merge( &p, psResult, psResult, ATA_TIMING_ALL );
	}
	

/*
 * Lenghten active & recovery time so that cycle time is correct.
 */

	if( psResult->nAct8b + psResult->nRec8b < psResult->nCyc8b) {
		psResult->nAct8b += (psResult->nCyc8b - (psResult->nAct8b + psResult->nRec8b)) / 2;
		psResult->nRec8b = psResult->nCyc8b - psResult->nAct8b;
	}

	if (psResult->nAct + psResult->nRec < psResult->nCyc) {
		psResult->nAct += (psResult->nCyc - (psResult->nAct + psResult->nRec)) / 2;
		psResult->nRec = psResult->nCyc - psResult->nAct;
	}
	
	return( 0 );
}

#endif







