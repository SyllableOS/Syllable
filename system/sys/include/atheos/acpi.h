/*
 *  The Syllable kernel
 *	ACPI busmanager
 *
 *  Copyright (C) 2001, 2002 Andy Grover <andrew.grover@intel.com>
 *  Copyright (C) 2001, 2002 Paul Diefenbaugh <paul.s.diefenbaugh@intel.com>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at
 *  your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#ifndef __F_ATHEOS_ACPI_H__
#define __F_ATHEOS_ACPI_H__

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

#include <atheos/types.h>

/* Battery status */
enum
{
	BATTERY_GET_STATUS
};

typedef struct
{
	int		bs_nDataSize;
	int		bs_nPercentage;
	bool	bs_bChargingOrFull;
} BatteryStatus_s;

/* CPU powermanagement */
enum
{
	CPU_GET_CURRENT_PSTATE,
	CPU_GET_PSTATE,
	CPU_SET_PSTATE,
	CPU_GET_ISTATE = 100
};

typedef struct
{
	int		cps_nCpuId;
	int		cps_nIndex;
	int		cps_nFrequency;
	int		cps_nPower;
} CPUPerformanceState_s;

typedef struct
{
	int		cis_nCpuId;
	int		cis_nIndex;
	int		cis_nType;
	uint32	cis_nUsage;
} CPUIdleState_s;

#ifdef __cplusplus
}
#endif

#endif	/*__F_ATHEOS_ACPI_H__*/
