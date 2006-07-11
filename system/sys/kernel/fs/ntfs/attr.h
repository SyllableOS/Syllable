/*
 * attr.h -  Header file for attr.c
 *
 * Copyright (C) 1997 Régis Duchesne
 * Copyright (c) 2001 Anton Altaparmakov (AIA)
 */

#ifndef _NTFS_ATTR_H
#define _NTFS_ATTR_H

uint8 *ntfs_find_attr_in_mft_rec( ntfs_volume_s * vol, uint8 *m, uint32 type, wchar_t *name, uint32 name_len, int ic, uint16 instance );

int ntfs_extend_attr( ntfs_inode_s * ino, ntfs_attribute * attr, const int64 len );

int ntfs_resize_attr( ntfs_inode_s * ino, ntfs_attribute * attr, int64 newsize );

int ntfs_insert_attribute( ntfs_inode_s * ino, unsigned char *attrdata );

int ntfs_read_compressed( ntfs_inode_s * ino, ntfs_attribute * attr, int64 offset, ntfs_io * dest );

int ntfs_write_compressed( ntfs_inode_s * ino, ntfs_attribute * attr, int64 offset, ntfs_io * dest );

int ntfs_create_attr( ntfs_inode_s * ino, int anum, char *aname, void *data, int dsize, ntfs_attribute ** rattr );

int ntfs_read_zero( ntfs_io * dest, int size );

int ntfs_make_attr_nonresident( ntfs_inode_s * ino, ntfs_attribute * attr );

int ntfs_attr_allnonresident( ntfs_inode_s * ino );

int ntfs_new_attr( ntfs_inode_s * ino, int type, void *name, int namelen, void *value, int value_len, int *pos, int *found );

int ntfs_insert_run( ntfs_attribute * attr, int cnum, off_t cluster, int len );

#endif	/* defined _NTFS_ATTR_H */

