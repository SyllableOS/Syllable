/*  The AtheOS application server
 *  Copyright (C) 1999 - 2001  Kurt Skauen
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */

#ifndef __F_APPSERVER_KEYMAP_H__
#define __F_APPSERVER_KEYMAP_H__

#include <atheos/types.h>

#define CURRENT_KEYMAP_VERSION	2
#define KEYMAP_MAGIC		0x4d7f98c5
#define DEADKEY_ID		0x02

enum
{
    CM_NORMAL,
    CS_SHFT,
    CS_CTRL,
    CS_OPT,
    CS_SHFT_OPT,
    CS_CAPSL,
    CS_SHFT_CAPSL,
    CS_CAPSL_OPT,
    CS_SHFT_CAPSL_OPT
};

enum {
    KLOCK_CAPSLOCK   = 0x0001,
    KLOCK_SCROLLLOCK = 0x0002,
    KLOCK_NUMLOCK    = 0x0004
};

struct keymap_header {
	uint32	m_nMagic;
	uint32	m_nVersion;
	uint32	m_nSize;
};

typedef struct keymap_deadkey {
    uint8  m_nRawKey;
    uint8  m_nQualifier;
    int32  m_nKey;
    int32  m_nValue;
} deadkey;

struct keymap
{
    int16  m_nCapsLock;
    int16  m_nScrollLock;
    int16  m_nNumLock;
    int16  m_nLShift;
    int16  m_nRShift;
    int16  m_nLCommand;
    int16  m_nRCommand;
    int16  m_nLControl;
    int16  m_nRControl;
    int16  m_nLOption;
    int16  m_nROption;
    int16  m_nMenu;
    uint32 m_nLockSetting;
    
    int32 m_anMap[128][9];

    uint32 m_nNumDeadKeys;
    deadkey m_sDeadKey[];
};

#endif // __F_APPSERVER_KEYMAP_H__


