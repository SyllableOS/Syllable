/*
 *  ATA/ATAPI driver for Syllable
 *
 *	Copyright (C) 2004 Arno Klenke
 *  Copyright (C) 2003 Kristian Van Der Vliet
 *  Copyright (C) 2002 William Rose
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *	Copyright (C) 2003 Red Hat, Inc.  All rights reserved.
 *	Copyright (C) 2003 Jeff Garzik
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
 
#ifndef __ATA_REGS_H_
#define __ATA_REGS_H_


/* Standard ATA registers */
enum ATA_regs
{
	ATA_REG_DATA, /* 0 */
	ATA_REG_ERROR, /* 1 */
	ATA_REG_COUNT, /* 2 */
	ATA_REG_LBA_LOW, /* 3 */
	ATA_REG_LBA_MID, /* 4 */
	ATA_REG_LBA_HIGH, /* 5 */
	ATA_REG_DEVICE, /* 6 */
	ATA_REG_STATUS, /* 7 */
	ATA_REG_CONTROL, /* 8 */
	ATA_TOTAL_REGS, /* 9 */
	
	ATA_REG_FEATURE = ATA_REG_ERROR, /* 1 */
	ATA_REG_COMMAND = ATA_REG_STATUS, /* 7 */
	ATA_REG_ALTSTATUS = ATA_REG_CONTROL, /* 8 */
};

/* Busmaster DMA registers */
enum ATA_dma_regs
{
	ATA_REG_DMA_CONTROL, /* 0 */
	ATA_REG_DMA_STATUS, /* 1 */
	ATA_REG_DMA_TABLE,  /* 2 */
	ATA_DMA_TOTAL_REGS /* 3 */
};

/* Serial ATA registers */
enum SATA_regs
{
	SATA_REG_STATUS, /* 0 */
	SATA_REG_ERROR, /* 1 */
	SATA_REG_CONTROL, /* 2 */
	SATA_REG_ACTIVE, /* 3 */
	SATA_REG_NOTIFICATION, /* 4 */
	SATA_TOTAL_REGS /* 5 */
};

/* ATA_REG_DEVICE */
#define ATA_DEVICE_DEFAULT		0xA0 /* default mask */
#define ATA_DEVICE_LBA			0x40 /* use lba mode */
#define ATA_DEVICE_SLAVE		0x10 /* select slave device */

/* ATA_REG_STATUS */
#define ATA_STATUS_BUSY			0x80 /* busy */
#define ATA_STATUS_DRDY			0x40 /* data ready */
#define ATA_STATUS_DRQ			0x08 /* data request */
#define ATA_STATUS_ERROR		0x01 /* error */

/* ATA_REG_COMMAND */
#define ATA_CMD_READ_PIO		0x20	/* read data */
#define ATA_CMD_WRITE_PIO		0x30	/* write data */
#define ATA_CMD_READ_PIO_48		0x24	/* read data 48bit */
#define ATA_CMD_WRITE_PIO_48	0x34	/* write data 48bit */
#define ATA_CMD_SEEK			0x70	/* seek cylinder */
#define ATA_CMD_IDENTIFY		0xEC	/* identify drive */
#define ATA_CMD_READ_DMA		0xC8	/* dma read data */
#define ATA_CMD_WRITE_DMA		0xCA	/* dma write data */
#define ATA_CMD_READ_DMA_48		0x25	/* dma read data 48bit */
#define ATA_CMD_WRITE_DMA_48	0x35	/* dma write data 48bit */
#define ATA_CMD_SET_FEATURES	0xEF	/* set features */

/* ATA_REG_CONTROL */
#define ATA_CONTROL_HOB			0x80 /* access 48bit registers */
#define ATA_CONTROL_DEFAULT		0x08 /* default mask */
#define ATA_CONTROL_RESET		0x04 /* reset */
#define ATA_CONTROL_INTDISABLE	0x02 /* disable interrupts */



/* Command: ATA_CMD_SET_FEATURES Register: ATA_REG_FEATURE */
#define ATA_FEATURE_XFER		0x03

/* Command: ATA_CMD_SET_FEATURES Register: ATA_REG_COUNT */
#define ATA_XFER_PIO_3			0x0B
#define ATA_XFER_PIO_4			0x0C
#define ATA_XFER_PIO_5			0x0D
#define ATA_XFER_MWDMA_0		0x20
#define ATA_XFER_MWDMA_1		0x21
#define ATA_XFER_MWDMA_2		0x22
#define ATA_XFER_UDMA_0			0x40
#define ATA_XFER_UDMA_1			0x41
#define ATA_XFER_UDMA_2			0x42
#define ATA_XFER_UDMA_3			0x43
#define ATA_XFER_UDMA_4			0x44
#define ATA_XFER_UDMA_5			0x45
#define ATA_XFER_UDMA_6			0x46
#define ATA_XFER_UDMA_7			0x47

/* ATA_REG_DMA_CONTROL */
#define ATA_DMA_CONTROL_STOP	0x00
#define ATA_DMA_CONTROL_START	0x01
#define ATA_DMA_CONTROL_READ	0x08

/* ATA_REG_DMA_STATUS */
#define ATA_DMA_STATUS_ACTIVE	0x01
#define ATA_DMA_STATUS_ERROR	0x02
#define ATA_DMA_STATUS_IRQ		0x04
#define ATA_DMA_STATUS_0_EN		0x20
#define ATA_DMA_STATUS_1_EN		0x40


#endif








