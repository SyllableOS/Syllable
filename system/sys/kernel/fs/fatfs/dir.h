/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _DOSFS_DIR_H_
#define _DOSFS_DIR_H_

status_t	check_dir_empty(nspace *vol, vnode *dir);
status_t	findfile(nspace *vol, vnode *dir, const char *file,
			 ino_t *vnid, vnode **node);

status_t	erase_dir_entry(nspace *vol, vnode *node);
status_t	compact_directory(nspace *vol, vnode *dir);
status_t	create_volume_label(nspace *vol, const char name[11], uint32 *index);
status_t	create_dir_entry(nspace *vol, vnode *dir, vnode *node, 
				const char *name, uint32 *ns, uint32 *ne);

int dosfs_read_vnode(void *_vol, ino_t vnid/*, char r*/, void **node);
int fatfs_locate_inode( void* pVolume, void* pParentNode, const char* pzName, int nNameLen, ino_t* pnInodeNum );
//int			dosfs_walk(void *_vol, void *_dir, const char *file, char **newpath, ino_t *vnid);
int dosfs_access(void *_vol, void *_node, int mode);
int dosfs_readlink(void *_vol, void *_node, char *buf, size_t *bufsize);
int dosfs_opendir(void *_vol, void *_node, void **cookie);
int dosfs_readdir( void *_vol, void *_dir, void *_cookie, int nPos, struct kernel_dirent *entry, size_t bufsize );
int dosfs_rewinddir(void *_vol, void *_node, void *cookie);
int dosfs_closedir(void *_vol, void *_node, void *cookie);
int dosfs_free_dircookie(void *_vol, void *_node, void *cookie);

#endif
