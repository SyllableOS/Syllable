/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _DOSFS_FILE_H_
#define _DOSFS_FILE_H_

status_t write_vnode_entry(nspace *vol, vnode *node);

int	dosfs_write_vnode(void *_vol, void *_node );
int	dosfs_rstat(void *_vol, void *_node, struct stat *st);
int	dosfs_open(void *_vol, void *_node, int omode, void **cookie);
int	dosfs_read(void *_vol, void *_node, void *cookie, off_t pos,
					void *buf, size_t len);
int	dosfs_free_cookie(void *vol, void *node, void *cookie);
int	dosfs_close(void *vol, void *node, void *cookie);

int	dosfs_remove_vnode(void *vol, void *node, char r);
int	dosfs_create(void *_vol, void *_dir, const char* pzName, int nNameLen, int omode, int perms, ino_t *vnid, void **_cookie );
int	dosfs_mkdir(void *vol, void *dir, const char *name, int nNameLen, int perms);
int	dosfs_rename( void *_vol, void *_odir, const char* pzOldName, int nOldNameLen,
		      void *_ndir, const char* pzNewName, int nNewNameLen, bool bMustBeDir );
int	dosfs_unlink(void *vol, void *dir, const char *name, int nNameLen );
int	dosfs_rmdir(void *vol, void *dir, const char *name, int nNameLen );
int	dosfs_wstat(void *vol, void *node, const struct stat *st, uint32 mask);
int	dosfs_write(void *vol, void *node, void *cookie, off_t pos,
						const void *buf, size_t len);

#endif
