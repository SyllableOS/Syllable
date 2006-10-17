/*
 *  The Syllable kernel
 *  MPC table definitions
 *  Copyright (C) 2004 The Syllable Team
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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
 
#ifndef _MPC_H_
#define _MPC_H_

static const uint32_t SMP_MAGIC_IDENT = (('_'<<24)|('P'<<16)|('M'<<8)|'_');
#define MPC_SIGNATURE	"PCMP"

typedef struct
{
	char mpc_anSignature[4];
	uint16 mpc_nSize;	/* Size of table */
	char mpc_nVersion;	/* Table version (0x01 or 0x04) */
	char mpc_checksum;
	char mpc_anOEMString[8];
	char mpc_anProductID[12];
	void *mpc_pOEMPointer;	/* 0 if not present */
	uint16 mpc_nOEMSize;	/* 0 if not present */
	uint16 mpc_nOEMCount;
	uint32 mpc_nAPICAddress;	/* APIC address */
	uint32 mpc_reserved;
} MpConfigTable_s;

typedef struct
{
	char mpf_anSignature[4];	/* "_MP_"                               */
	MpConfigTable_s *mpf_psConfigTable;	/* Configuration table address          */
	uint8 mpf_nLength;	/* Length of struct in paragraphs       */
	uint8 mpf_nVersion;	/* Specification version                */
	uint8 mpf_nChecksum;	/* Checksum (makes sum 0)               */
	uint8 mpf_nFeature1;	/* Standard or configuration ?          */
	uint8 mpf_nFeature2;	/* Bit7 set for IMCR|PIC                */
	uint8 mpf_unused[3];	/* Unused (0)                           */
} MpFloatingPointer_s;

/* Followed by entries */

#define	MP_PROCESSOR	0
#define	MP_BUS		1
#define	MP_IOAPIC	2
#define	MP_INTSRC	3
#define	MP_LINTSRC	4

/* CPU flags */
static const uint8_t CPU_ENABLED	= 1;	/* Processor is available */
static const uint8_t CPU_BOOTPROCESSOR	= 2;	/* Processor is the BP */

/* CPU features */
static const uint32_t CPU_STEPPING_MASK = 0x0F;
static const uint32_t CPU_MODEL_MASK	 = 0xF0;
static const uint32_t CPU_FAMILY_MASK	 = 0xF00;

typedef struct
{
	uint8 mpc_nType;
	uint8 mpc_nAPICID;		/* Local APIC number */
	uint8 mpc_nAPICVer;		/* Its versions */
	uint8 mpc_cpuflag;
	uint32 mpc_cpufeature;
	uint32 mpc_featureflag;	/* CPUID feature value */
	uint32 mpc_reserved[2];
} MpConfigProcessor_s;

typedef struct
{
	uint8 mpc_nType;
	uint8 mpc_busid;
	uint8 mpc_bustype[6];
} MpConfigBus_s;

#define BUSTYPE_EISA	"EISA"
#define BUSTYPE_ISA	"ISA"
#define BUSTYPE_INTERN	"INTERN"	/* Internal BUS */
#define BUSTYPE_MCA	"MCA"
#define BUSTYPE_VL	"VL"			/* Local bus */
#define BUSTYPE_PCI	"PCI"
#define BUSTYPE_PCMCIA	"PCMCIA"

/* We don't understand the others */

static const uint8_t MPC_APIC_USABLE	= 0x01;

typedef struct
{
	uint8 mpc_nType;
	uint8 mpc_nAPICID;
	uint8 mpc_nAPICVer;
	uint8 mpc_nFlags;
	uint32 mpc_nAPICAddress;
} MpConfigIOAPIC_s;

typedef struct
{
	uint8 mpc_nType;
	uint8 mpc_nIRQType;
	uint16 mpc_nIRQFlags;
	uint8 mpc_nSrcBusID;
	uint8 mpc_nSrcBusIRQ;
	uint8 mpc_nDstAPIC;
	uint8 mpc_nDstIRQ;
} MpConfigIntSrc_s;

typedef struct
{
	uint8 mpc_nType;
	uint8 mpc_nIRQType;
	uint16 mpc_nIRQFlags;
	uint8 mpc_nSrcBusID;
	uint8 mpc_nSrcBusIRQ;
	uint8 mpc_nDstAPIC;
	uint8 mpc_nDstAPICLocalInt;
} MpConfigIntLocal_s;


/*
 *	Default configurations
 *
 *	1	2 CPU ISA 82489DX
 *	2	2 CPU EISA 82489DX no IRQ 8 or timer chaining
 *	3	2 CPU EISA 82489DX
 *	4	2 CPU MCA 82489DX
 *	5	2 CPU ISA+PCI
 *	6	2 CPU EISA+PCI
 *	7	2 CPU MCA+PCI
 */

#endif	/* _MPC_H_ */

