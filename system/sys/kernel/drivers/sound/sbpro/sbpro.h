/*
 *  SoundBlaster Pro Driver for the AtheOS kernel
 *  Copyright (C) 2000  Joel Smith
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
                 
#ifndef __ATHEOS_SBPRO_H_
#define __ATHEOS_SBPRO_H_

#define SB_DMA_SIZE        	(64*1024)
#define SB_PORT_BASE    	0x220
#define SB_TIMEOUT		64000
#define SB_IRQ			7

#define SB_DMA			1
#define SB_DMA_PLAY		0x58
#define SB_DMA_REC		0x54

#define SB_MIN_SPEED		4000
#define SB_MAX_SPEED		44100
#define SB_DEFAULT_SPEED        22050

/* I/O ports for SBPro. */
#define SB_DSP_RESET		(SB_PORT_BASE + 0x06)
#define SB_DSP_READ		(SB_PORT_BASE + 0x0A)
#define SB_DSP_WRITE		(SB_PORT_BASE + 0x0C)
#define SB_DSP_COMMAND		(SB_PORT_BASE + 0x0C)
#define SB_DSP_STATUS		(SB_PORT_BASE + 0x0C)
#define SB_DSP_DATA_AVL		(SB_PORT_BASE + 0x0E)
#define SB_DSP_DATA_AVL16	(SB_PORT_BASE + 0x0F)

#define SB_MIXER_ADDR		(SB_PORT_BASE + 0x04)
#define SB_MIXER_DATA		(SB_PORT_BASE + 0x05)

/* DSP Commands */
#define SB_DSP_CMD_SPKON	0xD1
#define SB_DSP_CMD_SPKOFF	0xD3

#define SB_DSP_CMD_DMAHALT	0xD0
#define SB_DSP_CMD_TCONST	0x40
#define SB_DSP_CMD_HSSIZE	0x48

#define SB_DSP_CMD_HSDAC	0x91
#define SB_DSP_CMD_HSADC	0x99
#define SB_DSP_CMD_DAC8		0x14
#define SB_DSP_CMD_ADC8		0x24

#define SB_DSP_CMD_GETVER	0xE1
#define SB_DSP_CMD_GETID	0xE7

#endif /* __ATHEOS_SBPRO_H_ */
