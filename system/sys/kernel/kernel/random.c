/*
 *  The Syllable kernel
 *  Copyright (C) 2002 Kristian Van Der Vliet
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

#include <atheos/random.h>
#include <atheos/types.h>
#include <atheos/time.h>

/* kseed can be changed at any time by a call to seed() */
static int64 kseed = 0x12345678;

/* a is the lower half, b is the upper half */
static int32 *a = (int32 *) &kseed;
static int32 *b = (int32 *) &kseed + 1; 

void seed(int32 seed_val)
{
	kseed ^= seed_val;

	*b ^= *a;

	kseed *= kseed;

	return;
}

uint32 rand(void)
{
	/* a condition where 'a' topmost two bits are set */
	if(!(*a & 0xC000000))
		seed(get_system_time()>>8); /* Reinitialize */

	/* To avoid zero value after the shift also to give the strengthen no1 */
	*a ^= *b;

	/* Re-confuse ... the strengthen no2 */
	kseed *= *a;

	/* Shift ... the strengthen no3 */
	kseed <<= ((*a % 3) + 4);

	/* Avoid zero value */
	if(*b == 0) 
		seed(get_system_time()>>8); /* Reinitialize */

	return(*b);
}

