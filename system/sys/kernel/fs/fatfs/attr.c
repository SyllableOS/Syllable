/* attr.c
 * handles mime type information for dosfs
 * gets/sets mime information in vnode
 */

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

typedef long long dr9_off_t;

#include <atheos/stdlib.h>
#include <atheos/string.h>
#include <atheos/kernel.h>
#include <atheos/device.h>

#include <posix/stat.h>
#include <posix/errno.h>
#include <posix/ioctl.h>

#include <atheos/time.h>

#include <atheos/filesystem.h>
#include <atheos/bcache.h>

#include <macros.h>

#include "dosfs.h"
#include "attr.h"

#define DPRINTF(a,b) if (debug_dir > (a)) printk b

struct ext_mime {
	char *extension;
	char *mime;
};

// some of this stolen from servers/roster/Sniff.cpp

static struct ext_mime mimes[] = {
	{ "gz", "application/x-gzip" },
	{ "hqx", "application/x-binhex40" },
	{ "lha", "application/x-lharc" },
	{ "pcl", "application/x-pcl" },
	{ "pdf", "application/pdf" },
	{ "ps", "application/postscript" },
	{ "sit", "application/x-stuff-it" },
	{ "tar", "application/x-tar" },
	{ "tgz", "application/x-gzip" },
	{ "uue", "application/x-uuencode" },
	{ "z", "application/x-compress" },
	{ "zip", "application/zip" },
	{ "zoo", "application/x-zoo" },

	{ "aif", "audio/x-aiff" },
	{ "aiff", "audio/x-aiff" },
	{ "au", "audio/basic" },
	{ "mid", "audio/x-midi" },
	{ "midi", "audio/x-midi" },
	{ "mod", "audio/mod" },
	{ "ra", "audio/x-real-audio" },
	{ "wav", "audio/x-wav" },

	{ "bmp", "image/x-bmp" },
	{ "fax", "image/g3fax" },
	{ "gif", "image/gif" },
	{ "iff", "image/x-iff" },
	{ "jpg", "image/jpeg" },
	{ "jpeg", "image/jpeg" },
	{ "pbm", "image/x-portable-bitmap" },
	{ "pcx", "image/x-pcx" },
	{ "pgm", "image/x-portable-graymap" },
	{ "png", "image/png" },
	{ "ppm", "image/x-portable-pixmap" },
	{ "rgb", "image/x-rgb" },
	{ "tga", "image/x-targa" },
	{ "tif", "image/tiff" },
	{ "tiff", "image/tiff" },
	{ "xbm", "image/x-xbitmap" },

	{ "txt", "text/plain" },
	{ "doc", "text/plain" },
	{ "htm", "text/html" },
	{ "html", "text/html" },
	{ "rtf", "text/rtf" },
	{ "c", "text/x-source-code" },
	{ "cc", "text/x-source-code" },
	{ "c++", "text/x-source-code" },
	{ "h", "text/x-source-code" },
	{ "hh", "text/x-source-code" },
	{ "cxx", "text/x-source-code" },
	{ "cpp", "text/x-source-code" },
	{ "S", "text/x-source-code" },
	{ "java", "text/x-source-code" },

	{ "avi", "video/x-msvideo" },
	{ "mov", "video/quicktime" },
	{ "mpg", "video/mpeg" },
	{ "mpeg", "video/mpeg" },

	{ 0, 0 }
};

status_t set_mime_type(vnode *node, const char *filename)
{
	struct ext_mime *p;
	int32 namelen, ext_len;

	DPRINTF(0, ("get_mime_type of (%s)\n", filename));

	node->mime = NULL;

	namelen = strlen(filename);

	for (p=mimes;p->extension;p++) {
		ext_len = strlen(p->extension);

		if (namelen <= ext_len)
			continue;

		if (filename[namelen-ext_len-1] != '.')
			continue;
		
		if (!strcmp(filename + namelen - ext_len, p->extension))
			break;
	}

	node->mime = p->mime;

	return B_OK;
}

int dosfs_open_attrdir(void *_vol, void *_node, void **_cookie)
{
	nspace *vol = (nspace *)_vol;

	TOUCH(_node);

	DPRINTF(0, ("dosfs_open_attrdir called\n"));

	LOCK_VOL(vol);

	if (check_nspace_magic(vol, "dosfs_open_attrdir")) {
		UNLOCK_VOL(vol);
		return -EINVAL;
	}

	if ((*_cookie = malloc(sizeof(uint32))) == NULL) {
		UNLOCK_VOL(vol);
		return -ENOMEM;
	}
	*(int32 *)(*_cookie) = 0;
	
	UNLOCK_VOL(vol);
	
	return 0;
}

int dosfs_close_attrdir(void *_vol, void *_node, void *_cookie)
{
	nspace *vol = (nspace *)_vol;

	TOUCH(_node);

	DPRINTF(0, ("dosfs_close_attrdir called\n"));

	LOCK_VOL(vol);

	if (check_nspace_magic(vol, "dosfs_open_attrdir")) {
		UNLOCK_VOL(vol);
		return -EINVAL;
	}

	*(int32 *)_cookie = 1;
	
	kfree( _cookie );
	
	UNLOCK_VOL(vol);
	
	return 0;
}

int dosfs_free_attrcookie(void *_vol, void *_node, void *_cookie)
{
	TOUCH(_vol); TOUCH(_node);

	DPRINTF(0, ("dosfs_free_attrcookie called\n"));

	if (_cookie == NULL) {
		printk("error: dosfs_free_attrcookie called with null cookie\n");
		return -EINVAL;
	}

	*(int32 *)_cookie = 0x87654321;
	free(_cookie);

	return 0;
}

int dosfs_rewind_attrdir(void *_vol, void *_node, void *_cookie)
{
	TOUCH(_vol); TOUCH(_node);

	DPRINTF(0, ("dosfs_rewind_attrdir called\n"));

	if (_cookie == NULL) {
		printk("error: dosfs_rewind_attrcookie called with null cookie\n");
		return -EINVAL;
	}

	*(uint32 *)_cookie = 0;
	return 0;
}

int dosfs_read_attrdir(void *_vol, void *_node, void *_cookie, struct kernel_dirent *entry, size_t bufsize)
{
	nspace *vol = (nspace *)_vol;
	vnode *node = (vnode *)_node;
	int32 *cookie = (int32 *)_cookie;
	int num = 0;

	TOUCH(bufsize);

	DPRINTF(0, ("dosfs_read_attrdir called\n"));
	
	LOCK_VOL(vol);

	if (check_nspace_magic(vol, "dosfs_read_attrdir") ||
		check_vnode_magic(node, "dosfs_read_attrdir")) {
		UNLOCK_VOL(vol);
		return -EINVAL;
	}

	if ((*cookie == 0) && (node->mime)) {
		num = 1;
		
		entry->d_ino = node->vnid;
		entry->d_namlen = 14;
		strcpy(entry->d_name, "os::MimeType");
	}

	*cookie = 1;

	UNLOCK_VOL(vol);
	
	return num;
}

int dosfs_stat_attr(void *_vol, void *_node, const char *name, int namelen, struct attr_info *buf)
{
	nspace *vol = (nspace *)_vol;
	vnode *node = (vnode *)_node;

	DPRINTF(0, ("dosfs_stat_attr (%s)\n", name));

	if (strncmp(name, "os::MimeType", min( strlen( "os::MimeType" ), namelen)))
		return -ENOENT;

	LOCK_VOL(vol);

	if (check_nspace_magic(vol, "dosfs_read_attr") ||
		check_vnode_magic(node, "dosfs_read_attr")) {
		UNLOCK_VOL(vol);
		return -EINVAL;
	}

	if (node->mime == NULL) {
		UNLOCK_VOL(vol);
		return -ENOENT;
	}
	
	buf->ai_type = ATTR_TYPE_STRING;
	buf->ai_size = strlen(node->mime) + 1;

	UNLOCK_VOL(vol);
	return 0;
}

int dosfs_read_attr(void *_vol, void *_node, const char *name, int namelen, int type, void *buf, 
	off_t pos, size_t len )
{
	nspace *vol = (nspace *)_vol;
	vnode *node = (vnode *)_node;
	int attrib_len = node->mime ? strlen( node->mime ) : 0;

	DPRINTF(0, ("dosfs_read_attr (%s)\n", name));

	if (strncmp(name, "os::MimeType", min( strlen( "os::MimeType" ), namelen )))
		return -ENOENT;
		
	if (type != ATTR_TYPE_STRING)
		return -ENOENT;

	LOCK_VOL(vol);

	if (check_nspace_magic(vol, "dosfs_read_attr") ||
		check_vnode_magic(node, "dosfs_read_attr")) {
		UNLOCK_VOL(vol);
		return -EINVAL;
	}

	if( node->mime == NULL) {
		UNLOCK_VOL(vol);
		return -ENOENT;
	}
	
	if( pos + len > attrib_len )
	{
		len = ( attrib_len > pos ) ? ( attrib_len - pos ) : 0;
	}
	if( len > 0 )
	{
		memcpy( buf, node->mime + pos, len );
	}

	UNLOCK_VOL(vol);
	return len;
}
#ifndef __SYLLABLE__
// suck up application attempts to set mime types; this hides an unsightly
// error message printed out by zip
int dosfs_write_attr(void *_vol, void *_node, const char *name, int namelen, int type,
	const void *buf, off_t pos, size_t len )
{
	TOUCH(_vol); TOUCH(_node); TOUCH(name); TOUCH(type); TOUCH(buf);
	TOUCH(len); TOUCH(pos);

	DPRINTF(0, ("dosfs_write_attr (%s)\n", name));

	*len = 0;

	if (strcmp(name, "BEOS:TYPE"))
		return -ENOSYS;
		
	if (type != MIME_STRING_TYPE)
		return -ENOSYS;

	return 0;
}
#endif
