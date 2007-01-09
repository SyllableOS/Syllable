
/*
 *  The Syllable kernel
 *  ACPI table definitions
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
 
#ifndef _ACPI_H_
#define _ACPI_H_
 
#define ACPI_RSDP_SIGNATURE "RSD PTR "
#define ACPI_RSDT_SIGNATURE "RSDT"
#define ACPI_MADT_SIGNATURE "APIC"

typedef struct
{
	char	ath_anSignature[4];
	uint32	ath_nLength;
	uint8	ath_nRevision;
	uint8	ath_nChecksum;
	char	ath_anOemId[6];
	char	ath_anOemTableId[8];
	uint32	ath_nOemRevision;
	char	ath_AslCompilerId[4];
	uint32	ath_AslCompilerRevision;
} __attribute__ ((packed)) AcpiTableHeader_s;

typedef struct 
{
	AcpiTableHeader_s ar_sHeader;
	uint32 ar_nEntry[8];
} __attribute__ ((packed)) AcpiRsdt_s;
 
typedef struct
{
	char	ar_anSignature[8]; /* "RSD PTR " */
	uint8	ar_nChecksum; /* Checksum (makes sum 0) */
	char	ar_anOemId[6]; /* OEM identification */
	uint8	ar_nRevision; /* Must be 0 for 1.0, 2 for 2.0 */
	uint32	ar_nRsdt; /* 32-bit physical address of RSDT */
} __attribute__ ((packed)) AcpiRsdp_s;

typedef struct
{
	AcpiTableHeader_s am_sHeader;
	uint32	am_nApicAddr;
	uint32	am_nReserved;
} AcpiMadt_s;

#define ACPI_MADT_PROCESSOR	0
#define ACPI_MADT_IOAPIC	1
#define ACPI_MADT_LAPIC_OVR	5

typedef struct
{
	uint8 ame_nType;
	uint8 ame_nLength;
} __attribute__ ((packed)) AcpiMadtEntry_s;

static const uint32_t ACPI_MADT_CPU_ENABLED = 1;

typedef struct
{
	AcpiMadtEntry_s amp_sHeader;
	uint8 amp_nAcpiId;
	uint8 amp_nApicId;
	uint32 amp_nFlags;
} __attribute__ ((packed)) AcpiMadtProcessor_s;


typedef struct
{
	AcpiMadtEntry_s ami_sHeader;
	uint8 ami_nId;
	uint8 ami_nReserved;
	uint32 ami_nAddr;
	uint32 ami_nReserved2;
} __attribute__ ((packed)) AcpiMadtIoApic_s;

typedef struct
{
	AcpiMadtEntry_s ami_sHeader;
	uint16 aml_nReserved;
	uint64 aml_nAddress;
} __attribute__ ((packed)) AcpiMadtLapicOvr_s;

#endif



