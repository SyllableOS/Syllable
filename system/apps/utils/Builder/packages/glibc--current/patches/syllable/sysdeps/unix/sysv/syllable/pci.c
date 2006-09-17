/* Copyright (C) 1996, 1997, 1998, 1999, 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <errno.h>
#include <sysdep.h>
#include <sys/syscall.h>

#include <atheos/types.h>
#include <atheos/pci.h>

uint32 __read_pci_config( int bus, int dev, int func, int offset, int size )
{
  uint32 val;

  INLINE_SYSCALL(raw_read_pci_config, 5, bus, (dev << 16 ) | func, offset, size, &val);
  return val;
}
weak_alias(__read_pci_config,read_pci_config)

status_t __write_pci_config( int bus, int dev, int func, int offset, int size, uint32 val )
{
  int ret;
  ret = INLINE_SYSCALL(raw_write_pci_config, 5, bus, (dev << 16) | func, offset, size, val);
  return ret;
}
weak_alias(__write_pci_config,write_pci_config)

