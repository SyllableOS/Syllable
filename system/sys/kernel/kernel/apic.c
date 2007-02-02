/*
 *  The Syllable kernel
 *  Copyright (C) 2006 Kristian Van Der Vliet
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

#include <inc/apic.h>
#include <inc/scheduler.h>
#include <inc/smp.h>
#include <inc/mpc.h>
#include <inc/acpi.h>

#include <atheos/kernel.h>
#include <atheos/kdebug.h>
#include <atheos/irq.h>

/* Ensure get_processor_id() works on non-smp machines.  Will be set to the virtual address of local APIC later
   if this is a SMP machine. */
static uint32 g_nFakeCPUID = 0;
vuint32 g_nApicAddr = ( (uint32)&g_nFakeCPUID ) - APIC_ID;

static vuint32 g_nApicBase = APIC_DEFAULT_PHYS_BASE;
bool g_bAPICPresent = false;

extern vint g_anLogicToRealID[MAX_CPU_COUNT];

/**
 * Checksum an MP configuration block.
 * \internal
 * \ingroup CPU
 * \author Kurt Skuen
 */
static int mpf_checksum( uint8 *mp, int len )
{
	int sum = 0;

	while ( len-- )
		sum += *mp++;

	return ( sum & 0xFF );
}

/**
 * Parses the ACPI RSDT table to find the ACPI MADT table. This table contains
 * information about the installed processors and is similiar to the MPC table
 * on SMP systems.
 * \param nRsdt - Physical address of the rsdt.
 * \internal
 * \ingroup CPU
 * \author Arno Klenke (arno_klenke@yahoo.de)
 */
static int smp_read_acpi_rsdt( uint32 nRsdt )
{
	uint32 nTableEnt;
	int nApics = 0;
	int nCPUCount = 0;
	uint32 i;
	area_id hRsdtArea;
	AcpiRsdt_s* psRsdt = NULL;
	
	/* Late compile-time check... */
	if( sizeof( *psRsdt ) > PAGE_SIZE )
		kerndbg( KERN_FATAL, "smp_read_acpi_rsdt() wrong RSDT size %d\n", sizeof( *psRsdt ) );
	
	/* Map the rsdt because it is outside the accessible memory */
	hRsdtArea = create_area( "acpi_rsdt", ( void ** )&psRsdt, PAGE_SIZE, PAGE_SIZE, AREA_ANY_ADDRESS | AREA_KERNEL, AREA_FULL_LOCK );
	if( hRsdtArea < 0 )
	{
		kerndbg( KERN_FATAL, "smp_read_acpi_rsdt() failed to map the acpi rsdt table\n" );
		return 1;
	}
	remap_area( hRsdtArea, (void*)( nRsdt & PAGE_MASK ) );
	psRsdt = (AcpiRsdt_s*)( (uint32)psRsdt + ( nRsdt - ( nRsdt & PAGE_MASK ) ) );
	
	/* Check signature and checksum */
	if( strncmp( psRsdt->ar_sHeader.ath_anSignature, ACPI_RSDT_SIGNATURE, 4 ) )
	{
		kerndbg( KERN_WARNING, "Bad signature [%c%c%c%c].\n", psRsdt->ar_sHeader.ath_anSignature[0], psRsdt->ar_sHeader.ath_anSignature[1], psRsdt->ar_sHeader.ath_anSignature[2], psRsdt->ar_sHeader.ath_anSignature[3] );
		delete_area( hRsdtArea );
		return 0;
	}
	
	if( mpf_checksum( ( unsigned char * )psRsdt, psRsdt->ar_sHeader.ath_nLength ) != 0 )
	{
		kerndbg( KERN_WARNING, "MP checksum error.\n" );
		delete_area( hRsdtArea );
		return 0;
	}

	printk( "OEM ID: %.6s Product ID: %.8s\n", psRsdt->ar_sHeader.ath_anOemId, psRsdt->ar_sHeader.ath_anOemTableId );

	/* Calculate number of table entries */
	nTableEnt = ( psRsdt->ar_sHeader.ath_nLength - sizeof( AcpiTableHeader_s ) ) >> 2;
	if( nTableEnt > 8 )
		nTableEnt = 8;
	
	for( i = 0; i < nTableEnt; i++ )
	{
		/* Map entries */
		area_id hEntry;
		AcpiTableHeader_s* psHeader = NULL;

		/* Map the rsdt entries. We map only one page (+1 for alignment) because we only read the table header
		   or the madt table and both are smaller than one page */
		hEntry = create_area( "acpi_rsdt_entry", ( void ** )&psHeader, 2 * PAGE_SIZE, 2 * PAGE_SIZE, AREA_ANY_ADDRESS | AREA_KERNEL, AREA_FULL_LOCK );
		if( hEntry < 0 )
		{
			kerndbg( KERN_FATAL, "smp_read_acpi_rsdt() failed to map a rsdt entry\n" );
			return 0;
		}
		
		remap_area( hEntry, (void*)( psRsdt->ar_nEntry[i] & PAGE_MASK ) );
		psHeader = (AcpiTableHeader_s*)( (uint32)psHeader + ( psRsdt->ar_nEntry[i] - ( psRsdt->ar_nEntry[i] & PAGE_MASK ) ) );

		kerndbg( KERN_DEBUG, "%.4s %.6s %.8s\n", psHeader->ath_anSignature, psHeader->ath_anOemId, psHeader->ath_anOemTableId );

		if( !strncmp( psHeader->ath_anSignature, ACPI_MADT_SIGNATURE, 4 ) )
		{
			/* Found the madt */
			area_id hMadt;
			AcpiMadt_s* psMadt = NULL;
			AcpiMadtEntry_s* psEntry = NULL;
			uint32 nMadtSize = PAGE_ALIGN( psHeader->ath_nLength ) + PAGE_SIZE;
			uint32 nMadtEnd;

			/* Map the madt and its entries */
			hMadt = create_area( "acpi_madt", ( void ** )&psMadt, nMadtSize, nMadtSize, AREA_ANY_ADDRESS | AREA_KERNEL, AREA_FULL_LOCK );
			if( hMadt < 0 )
			{
				kerndbg( KERN_FATAL, "smp_read_acpi_rsdt() failed to map the madt table\n" );
				return 0;
			}
			remap_area( hMadt, (void*)( psRsdt->ar_nEntry[i] & PAGE_MASK ) );
			psMadt = (AcpiMadt_s*)( (uint32)psMadt + ( psRsdt->ar_nEntry[i] - ( psRsdt->ar_nEntry[i] & PAGE_MASK ) ) );
			
			/* Set the local APIC address */
			g_bAPICPresent = true;
			g_nApicBase = (vuint32)psMadt->am_nApicAddr;

			kerndbg( KERN_DEBUG, "APIC at: 0x%X\n", psMadt->am_nApicAddr );

			nMadtEnd = (uint32)psMadt + psHeader->ath_nLength;
			psEntry = (AcpiMadtEntry_s*)( (uint32)psMadt + sizeof( AcpiMadt_s ) );
			
			/* Parse the madt entries */
			while( ( (uint32)psEntry ) < nMadtEnd )
			{
				switch( psEntry->ame_nType )
				{
					case ACPI_MADT_PROCESSOR:
					{
						AcpiMadtProcessor_s *psP = ( AcpiMadtProcessor_s * )psEntry;

						if( psP->amp_nFlags & ACPI_MADT_CPU_ENABLED )
						{
							nCPUCount++;
							printk( "Processor #%d found\n", psP->amp_nApicId );

							g_asProcessorDescs[psP->amp_nApicId].pi_bIsPresent = true;
							g_asProcessorDescs[psP->amp_nApicId].pi_nAPICVersion = 0x10;
							g_asProcessorDescs[psP->amp_nApicId].pi_nAcpiId = psP->amp_nAcpiId;
						}
						break;
					}
		
					case ACPI_MADT_IOAPIC:
					{
						AcpiMadtIoApic_s *psP = ( AcpiMadtIoApic_s * )psEntry;
						nApics++;
						printk( "I/O APIC %d at 0x%08X.\n", psP->ami_nId, psP->ami_nAddr );
						break;
					}
					case ACPI_MADT_LAPIC_OVR:
					{
						AcpiMadtLapicOvr_s *psP = ( AcpiMadtLapicOvr_s * )psEntry;
						printk( "WARNING: FOUND LAPIC OVERRIDE!!! to 0x%x\n", (uint)psP->aml_nAddress );
						break;
					}
				}
				psEntry = ( AcpiMadtEntry_s* )( (uint32)psEntry + psEntry->ame_nLength );
			}
			delete_area( hMadt );
		}
		delete_area( hEntry );	
	}

	delete_area( hRsdtArea );

	if( nApics > 1 )
		kerndbg( KERN_WARNING, "Multiple APICs are not supported.\n" );

	return nCPUCount;
}

/**
 * Tries to find the ACPI RSDP table in the system memory.
 * \param pBase - First base address pointer.
 * \param nSize - Size of the memory region.
 * \internal
 * \ingroup CPU
 * \author Arno Klenke (arno_klenke@yahoo.de)
 */
static bool smp_scan_acpi_config( void *pBase, uint nSize )
{
	AcpiRsdp_s *psRsdp = NULL;
	int nCPUCount = 0;

	for( psRsdp = ( AcpiRsdp_s * ) pBase; nSize > 0; psRsdp = ( AcpiRsdp_s * )( ((uint8_t*)psRsdp) + 16 ), nSize -= 16 )
	{
		if ( strncmp( ( char* )psRsdp->ar_anSignature, ACPI_RSDP_SIGNATURE, 8 ) )
			continue;
		
		if ( mpf_checksum( ( uint8 * )psRsdp, sizeof( AcpiRsdp_s ) ) != 0 )
			continue;
		
		if ( psRsdp->ar_nRevision >= 2 )
			continue;

		nCPUCount = smp_read_acpi_rsdt( psRsdp->ar_nRsdt );
	}

	if( nCPUCount > 0 )
	{
		printk( "%d processors found.\n", nCPUCount );
		return true;
	}
	else
		return false;
}

static void smp_init_default_config( int nConfig )
{
	/* We need to know what the local APIC id of the boot CPU is! */

	/* NOTE: The MMU is still disabled. */
	g_nBootCPU = GET_APIC_LOGICAL_ID( g_nApicBase + APIC_ID );
	g_nFakeCPUID = SET_APIC_LOGICAL_ID( g_nBootCPU );

	/* 2 CPUs, numbered 0 & 1. */
	g_asProcessorDescs[0].pi_bIsPresent = true;
	g_asProcessorDescs[1].pi_bIsPresent = true;

	kerndbg( KERN_DEBUG, "I/O APIC at 0xFEC00000.\n" );

	switch( nConfig )
	{
		case 1:
		{
			printk( "ISA - external APIC\n" );
			g_asProcessorDescs[0].pi_nAPICVersion = 0x00;
			g_asProcessorDescs[1].pi_nAPICVersion = 0x00;
			break;
		}
		case 2:
		{
			printk( "EISA with no IRQ8 chaining - external APIC\n" );
			g_asProcessorDescs[0].pi_nAPICVersion = 0x00;
			g_asProcessorDescs[1].pi_nAPICVersion = 0x00;
			break;
		}
		case 3:
		{
			printk( "EISA - external APIC\n" );
			g_asProcessorDescs[0].pi_nAPICVersion = 0x00;
			g_asProcessorDescs[1].pi_nAPICVersion = 0x00;
			break;
		}
		case 4:
		{
			printk( "MCA - external APIC\n" );
			g_asProcessorDescs[0].pi_nAPICVersion = 0x00;
			g_asProcessorDescs[1].pi_nAPICVersion = 0x00;
			break;
		}
		case 5:
		{
			printk( "ISA/PCI - internal APIC\n" );
			g_asProcessorDescs[0].pi_nAPICVersion = 0x10;
			g_asProcessorDescs[1].pi_nAPICVersion = 0x10;
			break;
		}
		case 6:
		{
			printk( "EISA/PCI - internal APIC\n" );
			g_asProcessorDescs[0].pi_nAPICVersion = 0x10;
			g_asProcessorDescs[1].pi_nAPICVersion = 0x10;
			break;
		}
		case 7:
		{
			printk( "MCA/PCI - internal APIC\n" );
			g_asProcessorDescs[0].pi_nAPICVersion = 0x10;
			g_asProcessorDescs[1].pi_nAPICVersion = 0x10;
			break;
		}
		default:
			printk( "Unknown default SMP configuration %d\n", nConfig );
	}
}

/**
 * Processor encoding in an MP configuration block.
 * \internal
 * \ingroup CPU
 * \author Kurt Skuen
 */
static char *mpc_family( int family, int model )
{
	static char n[32];
	static char *model_defs[] = {
		"80486DX", "80486DX",
		"80486SX", "80486DX/2 or 80487",
		"80486SL", "Intel5X2(tm)",
		"Unknown", "80486DX/2-WB",
		"80486DX/4", "80486DX/4-WB"
	};

	switch( family )
	{
		case 0x04:
		{
			if( model < 10 )
				return model_defs[model];
			break;
		}

		case 0x05:
			return "Pentium(tm)";

		case 0x06:
			return "Pentium(tm) Pro";

		case 0x0f:
		{
			if( model == 0x00 || model == 0x01 )
				return "Pentium 4(tm)";

			if( model == 0x02 )
				return "Pentium 4(tm) XEON(tm)";

			if( model == 0x0f )
				return "Special controller";
		}
	}
	sprintf( n, "Unknown CPU [%d:%d]", family, model );
	return n;
}

/**
 * Read the MP configuration tables.
 * \internal
 * \ingroup CPU
 * \author Kurt Skuen
 */
static int smp_read_mpc( MpConfigTable_s * mpc )
{
	char str[16];
	int count = sizeof( *mpc );
	int apics = 0;
	unsigned char *mpt = ( ( unsigned char * )mpc ) + count;
	int nProcessorCount = 0;

	if( memcmp( mpc->mpc_anSignature, MPC_SIGNATURE, 4 ) )
	{
		kerndbg( KERN_WARNING, "Bad MPC signature [%c%c%c%c].\n", mpc->mpc_anSignature[0], mpc->mpc_anSignature[1], mpc->mpc_anSignature[2], mpc->mpc_anSignature[3] );
		return 1;
	}

	if( mpf_checksum( ( unsigned char * )mpc, mpc->mpc_nSize ) != 0 )
	{
		kerndbg( KERN_WARNING, "MPC checksum error.\n" );
		return 1;
	}

	if( mpc->mpc_nVersion != 0x01 && mpc->mpc_nVersion != 0x04 )
	{
		kerndbg( KERN_WARNING, "Bad MPC table version (%d)!!\n", mpc->mpc_nVersion );
		return 1;
	}

	printk( "OEM ID: %.8s Product ID: %.12s\n", mpc->mpc_anOEMString, mpc->mpc_anProductID );
	kerndbg( KERN_DEBUG, "APIC at: 0x%X\n", mpc->mpc_nAPICAddress );

	/* Set the local APIC address */
	g_nApicBase = (vuint32)mpc->mpc_nAPICAddress;

	/* Now parse the configuration blocks. */
	while( count < mpc->mpc_nSize )
	{
		switch ( *mpt )
		{
			case MP_PROCESSOR:
			{
				MpConfigProcessor_s *m = (MpConfigProcessor_s *) mpt;

				if( m->mpc_cpuflag & CPU_ENABLED )
				{
					nProcessorCount++;
					printk( "Processor #%d %s APIC version %d\n", m->mpc_nAPICID, mpc_family( ( m->mpc_cpufeature & CPU_FAMILY_MASK ) >> 8, ( m->mpc_cpufeature & CPU_MODEL_MASK ) >> 4 ), m->mpc_nAPICVer );

					if( m->mpc_featureflag & ( 1 << 0 ) )
						printk( "    Floating point unit present.\n" );

					if( m->mpc_featureflag & ( 1 << 7 ) )
						printk( "    Machine Exception supported.\n" );

					if( m->mpc_featureflag & ( 1 << 8 ) )
						printk( "    64 bit compare & exchange supported.\n" );

					if( m->mpc_featureflag & ( 1 << 9 ) )
						printk( "    Internal APIC present.\n" );

					if( m->mpc_cpuflag & CPU_BOOTPROCESSOR )
						printk( "    Bootup CPU\n" );

					if( m->mpc_nAPICID > MAX_CPU_COUNT )
						printk( "Processor #%d unused. (Max %d processors).\n", m->mpc_nAPICID, MAX_CPU_COUNT );
					else
					{
						g_asProcessorDescs[m->mpc_nAPICID].pi_bIsPresent = true;
						g_asProcessorDescs[m->mpc_nAPICID].pi_nAPICVersion = m->mpc_nAPICVer;
					}
				}
				mpt += sizeof( *m );
				count += sizeof( *m );
				break;
			}

			case MP_BUS:
			{
				MpConfigBus_s *m = (MpConfigBus_s *) mpt;

				memcpy( str, m->mpc_bustype, 6 );
				str[6] = 0;
				printk( "Bus %d is %s\n", m->mpc_busid, str );
				mpt += sizeof( *m );
				count += sizeof( *m );
				break;
			}

			case MP_IOAPIC:
			{
				MpConfigIOAPIC_s *m = (MpConfigIOAPIC_s *) mpt;

				if( m->mpc_nFlags & MPC_APIC_USABLE )
				{
					apics++;
					printk( "I/O APIC %d Version %d at 0x%08X.\n", m->mpc_nAPICID, m->mpc_nAPICVer, m->mpc_nAPICAddress );
				}
				mpt += sizeof( *m );
				count += sizeof( *m );
				break;
			}

			case MP_INTSRC:
			{
				MpConfigIOAPIC_s *m = (MpConfigIOAPIC_s *) mpt;

				mpt += sizeof( *m );
				count += sizeof( *m );
				break;
			}

			case MP_LINTSRC:
			{
				MpConfigIntLocal_s *m = (MpConfigIntLocal_s *) mpt;

				mpt += sizeof( *m );
				count += sizeof( *m );
				break;
			}
		}
	}

	if( apics > 1 )
		kerndbg( KERN_WARNING, "Multiple APICs are not supported.\n" );

	return nProcessorCount;
}

/**
 * Tries to find the MPC table in the system memory.
 * \param pBase - First base address pointer.
 * \param nSize - Size of the memory region.
 * \internal
 * \ingroup CPU
 * \author Kurt Skuen
 */
static bool smp_scan_mpc_config( void *pBase, uint nSize )
{
	MpFloatingPointer_s *mpf;
	int nProcessorCount = 0;

	/* Late compile-time check... */
	if( sizeof( *mpf ) != 16 )
		kerndbg( KERN_FATAL, "smp_scan_config() Wrong MPF size %d\n", sizeof( *mpf ) );

	for( mpf = ( MpFloatingPointer_s * ) pBase; nSize > 0; mpf++, nSize -= 16 )
	{
		if ( *( (uint32 *)mpf->mpf_anSignature ) != SMP_MAGIC_IDENT )
			continue;

		if( mpf->mpf_nLength != 1 )
			continue;

		if( mpf_checksum( (uint8 *)mpf, 16 ) != 0 )
			continue;

		if( mpf->mpf_nVersion != 1 && mpf->mpf_nVersion != 4 )
			continue;

		g_bAPICPresent = true;

		printk( "Intel SMP v1.%d\n", mpf->mpf_nVersion );
		if ( mpf->mpf_nFeature2 & ( 1 << 7 ) )
			printk( "    IMCR and PIC compatibility mode.\n" );
		else
			printk( "    Virtual Wire compatibility mode.\n" );

		/* Check if we got one of the standard configurations */
		if( mpf->mpf_nFeature1 != 0 )
		{
			smp_init_default_config( mpf->mpf_nFeature1 );
			nProcessorCount = 2;
		}
		else if( mpf->mpf_psConfigTable != NULL )
			nProcessorCount = smp_read_mpc( mpf->mpf_psConfigTable );

		printk( "%d processors found.\n", nProcessorCount );

		/* Only use the first configuration found. */
		return true;
	}

	return false;
}

static void init_apic_ldr( int nCPU )
{
	uint32 nVal;

	nVal = apic_read( APIC_LDR ) & ~APIC_LDR_MASK;
	nVal |= SET_APIC_LOGICAL_ID( 1UL << nCPU );
	apic_write( APIC_LDR, nVal );
}

void setup_local_apic( void )
{
	uint32 nVal, nVer;
	int nCPU = get_processor_id();

	apic_write( APIC_ESR, 0 );
	apic_write( APIC_ESR, 0 );
	apic_write( APIC_ESR, 0 );
	apic_write( APIC_ESR, 0 );

	nVal = apic_read( APIC_LVR );
	nVer = GET_APIC_VERSION( nVal );

	init_apic_ldr( nCPU );

	/* Set Task Priority to "accept all" */
	nVal = ( apic_read( APIC_TASKPRI ) & ~APIC_TPRI_MASK );
	apic_write( APIC_TASKPRI, nVal );

	/* Enable the APIC */
	nVal = ( apic_read( APIC_SPIV ) & ~APIC_VECTOR_MASK );
	nVal |= APIC_SPIV_APIC_ENABLED;
	nVal &= ~APIC_SPIV_FOCUS_DISABLED;

	/* Set spurious IRQ vector */
	nVal |= INT_SPURIOUS;
	apic_write( APIC_SPIV, nVal );

	/* Setup LVT0 & LVT1 */
	nVal = apic_read( APIC_LVT0 ) & APIC_LVT_MASKED;
	if( nCPU == g_nBootCPU )
		nVal = APIC_DM_EXTINT;
	else
		nVal = APIC_DM_EXTINT | APIC_LVT_MASKED;
	apic_write( APIC_LVT0, nVal );

	if( nCPU == g_nBootCPU )
		nVal = APIC_DM_NMI;
	else
		nVal = APIC_DM_NMI | APIC_LVT_MASKED;
	apic_write( APIC_LVT1, nVal );
}

bool test_local_apic( void )
{
	uint32 nReg0, nReg1;

	/* Verify that we're looking at a real local APIC.  Check these against your board if the CPUs aren't
	   getting started for no apparent reason. */

	nReg0 = apic_read( APIC_LVR );
	kerndbg( KERN_DEBUG_LOW, "Getting VERSION: %x\n", nReg0 );
	apic_write( APIC_LVR, nReg0 ^ APIC_LVR_MASK );
	nReg1 = apic_read( APIC_LVR );
	kerndbg( KERN_DEBUG_LOW, "Getting VERSION: %x\n", nReg1 );

	/*  The two version reads above should print the same NON-ZERO!!! numbers.  If not, there is a problem
	    with the APIC write/read definitions. */

	if( ( nReg0 == 0 || nReg1 == 0 ) || nReg0 != nReg1 )
		return false;

	/* Check to see if the version looks reasonable */
	nReg1 = GET_APIC_VERSION( nReg0 );
	if( nReg1 == 0x00 || nReg1 == 0xff )
		return false;

	/* The ID register is read/write in a real APIC */
	nReg0 = apic_read( APIC_ID );
	kerndbg( KERN_DEBUG_LOW, "Getting ID: %x\n", nReg0 );
	
	/*  The next two are just to see if we have sane values.  They're only really relevant if we're in Virtual Wire
        compatibility mode, but most boxes are. */
	nReg0 = apic_read( APIC_LVT0 );
	kerndbg( KERN_DEBUG_LOW, "Getting LVT0: %x\n", nReg0 );
	nReg1 = apic_read( APIC_LVT1 );
	kerndbg( KERN_DEBUG_LOW, "Getting LVT1: %x\n", nReg1 );

	return true;
}

/* Send an IPI */
void apic_send_ipi( int nTarget, uint32 nDeliverMode, int nIntNum )
{
	uint32 nFlags, nVal;

	nFlags = cli();

	/* Wait for ready */
	apic_wait_icr_idle();

	/* Set target */
	nVal = apic_read( APIC_ICR2 );
	nVal &= 0x00FFFFFF;
	apic_write( APIC_ICR2, nVal | SET_APIC_DEST_FIELD( nTarget ) );

	/* Send the IPI */
	nVal = apic_read( APIC_ICR );
	nVal &= ~0xFDFFF;	/* Clear bits */
	nVal |= ( nDeliverMode | nIntNum | nTarget | APIC_DEST_LOGICAL );

	apic_write( APIC_ICR, nVal );

	put_cpu_flags( nFlags );
}

int init_apic( bool bScanACPI )
{
	int i;
	bool bFound = false;

	kerndbg( KERN_DEBUG_LOW, "init_apic( %s )\n", bScanACPI ? "true" : "false" );

	/* If enabled, scan for ACPI tables */
	if( bScanACPI )
	{
		uint32 anAcpiConfigAreas[] = {
			0x0, 0x400,				/* Bottom 1k of base memory */
			0xe0000, 0x20000,		/* 64k of BIOS */
			0, 0
		};

		printk( "Scan memory for ACPI SMP config tables...\n" );

		for( i = 0; bFound == false && anAcpiConfigAreas[i + 1] > 0; i += 2 )
		{
			bFound = smp_scan_acpi_config( (void *)anAcpiConfigAreas[i], anAcpiConfigAreas[i + 1] );
			if( bFound )
				break;
		}
	}

	/* If we didn't find an ACPI table, scan for MPC tables */
	if( !bFound )
	{
		uint32 anMpcConfigAreas[] = {
			0x0, 0x400,				/* Bottom 1k of base memory */
			639 * 0x400, 0x400,		/* Top 1k of base memory (FIXME: Should first verify that we have 640kb!!!) */
			0xF0000, 0x10000,		/* 64k of BIOS */
			0, 0
		};

		printk( "Scan memory for MPC SMP config tables...\n" );

		for( i = 0; bFound == false && anMpcConfigAreas[i + 1] > 0; i += 2 )
		{
			bFound = smp_scan_mpc_config( (void *)anMpcConfigAreas[i], anMpcConfigAreas[i + 1] );
			if( bFound )
				break;
		}	
	}

	/* Is this a non-SMP machine with an APIC? */
	if( bFound == false && ( g_asProcessorDescs[g_nBootCPU].pi_nFeatures & CPU_FEATURE_APIC ) )
	{
		g_asProcessorDescs[g_nBootCPU].pi_bIsPresent = true;
		g_asProcessorDescs[g_nBootCPU].pi_bIsRunning = true;

		bFound = true;
		
	}

	if( bFound )
	{
		/* Map the APIC registers */
		g_nApicAddr = 0;
		area_id hApicArea = create_area( "apic_registers", (void**)&g_nApicAddr, PAGE_SIZE, PAGE_SIZE, AREA_ANY_ADDRESS | AREA_KERNEL, AREA_FULL_LOCK );
		if( hApicArea < 0 )
		{
			g_nApicAddr = ( (uint32)&g_nFakeCPUID ) - APIC_ID;

			kerndbg( KERN_WARNING, "Failed to create APIC area!\n" );
			return false;
		}

		if( remap_area( hApicArea, (void*)g_nApicBase ) < 0 )
		{
			g_nApicAddr = ( (uint32)&g_nFakeCPUID ) - APIC_ID;

			kerndbg( KERN_WARNING, "Failed to map APIC registers!\n" );
			delete_area( hApicArea );
			return false;
		}

		bFound = test_local_apic();
		if( bFound == false )
		{
			g_nApicAddr = ( (uint32)&g_nFakeCPUID ) - APIC_ID;
			delete_area( hApicArea );

			kerndbg( KERN_WARNING, "Local APIC test failed!\n" );
		}
		else
		{
			/* At this point the AP processors havn't been started, so we must be running on the
			   boot CPU.  By default it is assumed the boot CPU is #0, but it may not be. */

			int nCPU = get_processor_id();
			if( g_nBootCPU != nCPU )
			{
				g_nBootCPU = nCPU;	/* g_asProcessorDescs is the same for every processor at this point: see init_cpuid() */
				g_anLogicToRealID[0] = nCPU;
			}
		}

		/* g_nBootCPU is now valid no matter which CPU we booted on, so we can set the task register */
		SetTR( ( 8 + g_nBootCPU ) << 3 );

		/* This may be useful information */
		printk( "Boot from CPU #%d.\n", g_nBootCPU );
	}

	return bFound;
}

