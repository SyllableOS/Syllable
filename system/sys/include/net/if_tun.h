/*
 *  
 *  Tun Driver for Syllable
 *  Copyright (C) 2003 William Rose
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
 * 
 */

#ifndef __F_NET_IF_TUN_H_
#define __F_NET_IF_TUN_H_

/**
 * ioctl commands:
 *   TUN_IOC_CREATE_IF:         Creates the associated interface.
 *                              Expects a pointer to a name
 *                              (null-terminated, IFNAMSIZ bytes total).
 *                              The name must comprise characters from a-z only,
 *                              except that a name ending with + will have an
 *                              index inserted in place of the + to avoid name
 *                              clashes.  Otherwise the name must be unique.
 *   
 *   TUN_IOC_DELETE_IF:         Deletes the associated interface.
 *                              This not necessary -- calling close() will do
 *                              it automatically.  No arguments needed.
 */
#define TUN_IOC_CREATE_IF       0x89370000 /* Intentionally == SIOC_ETH_START */
#define TUN_IOC_DELETE_IF       0x89370001

#endif	/* __F_NET_IF_TUN_H_ */
