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
 
#ifndef __ATAPI_REGS_H_
#define __ATAPI_REGS_H_


/* ATAPI registers */

enum ATAPI_regs
{
	ATAPI_REG_IRR = 2, /* 2 = ATA_REG_COUNT */
	ATAPI_REG_SAMTAG, /* 3 = ATA_REG_LBA_LOW */
	ATAPI_REG_COUNT_LOW, /* 4 = ATA_REG_LBA_MID */
	ATAPI_REG_COUNT_HIGH, /* 5 = ATA_REG_LBA_HIGH */
	ATAPI_TOTAL_REGS
};

/* ATA_REG_FEATURE */
#define ATAPI_FEATURE_DMA	0x01

/* ATA_REG_STATUS */
#define ATAPI_STATUS_BUSY	0x80
#define ATAPI_STATUS_DRQ	0x08
#define ATAPI_STATUS_CHECK	0x01

/* ATA_REG_COMMAND */
#define ATAPI_CMD_RESET		0x08
#define ATAPI_CMD_PACKET	0xA0
#define ATAPI_CMD_IDENTIFY	0xA1

/* Sense data */
typedef struct
{
	uint8 error_code	: 7;
	uint8 valid			: 1;
	uint8 segment_number;
	uint8 sense_key		: 4;
	uint8 reserved2		: 1;
	uint8 ili			: 1;
	uint8 reserved1		: 2;
	uint8 information[4];
	uint8 add_sense_len;
	uint8 command_info[4];
	uint8 asc;
	uint8 ascq;
	uint8 fruc;
	uint8 sks[3];
	uint8 asb[46];
} ATAPI_sense_s;

#endif






