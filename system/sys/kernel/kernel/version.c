/*
 *  The AtheOS kernel
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

#include "version.h"

#define MAJOR	0LL
#define MINOR	4LL
#define SUB	2LL

const int64   g_nKernelVersion	= (MAJOR << 32) | (MINOR << 16) | (SUB);
const char*   g_pzKernelName	= "kernel.so";
const char*   g_pzBuildData	= __DATE__;
const char*   g_pzBuildTime	= __TIME__;
