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

#include <mntent.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <syllable/filesystem.h>

int syllableFSMountCount;

struct mntent * getmntent (FILE *stream)
{
	static char zPath[PATH_MAX] = "";
	static struct mntent* m = NULL;

	increment:
	if (syllableFSMountCount >= get_mount_point_count())
		return NULL;

	int nOpenFile = get_mount_point(syllableFSMountCount++,zPath,PATH_MAX);
	

	if (strcmp("/dev/pty",zPath) == 0 || strcmp("/dev",zPath) == 0)
	{
		goto increment;
	}

	int nFileDescriptor = open(zPath,O_RDONLY);

	if (nFileDescriptor)
	{
		static fs_info sInfo;
		if (get_fs_info(nFileDescriptor,&sInfo) == 0 && sInfo.fi_flags & FS_IS_BLOCKBASED  )
		{
			if (m == NULL)
				m = (struct mntent*)malloc(sizeof(struct mntent));
	
			if (m != NULL)
			{
				m->mnt_fsname = sInfo.fi_device_path;;
				m->mnt_dir = zPath;
				m->mnt_type =  sInfo.fi_driver_name;
				m->mnt_opts = sInfo.fi_mount_args;
				m->mnt_freq = 0;
				m->mnt_passno = 0;
			}
		}
		close(nFileDescriptor);
	}
	return m;
}


