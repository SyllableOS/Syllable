
/*
**
**	Copyright 1999, Be Incorporated.   All Rights Reserved.
**	This file may be used under the terms of the Be Sample Code License.
**	
**	Copyright 2001, pinc Software.  All Rights Reserved. 
**
**	iso9960/multi-session 
**		2001-03-11: added multi-session support, axeld. 
**	
**	2003 - Ported to Syllable by Ville Kallioniemi (ville.kallioniemi@abo.fi)
**
*/

#include "SUSP.h"
#include "rock.h"
#include "iso.h"

#include <atheos/bcache.h>
#include <atheos/device.h>
#include <atheos/kdebug.h>
#include <atheos/kernel.h>
#include <atheos/nls.h>
#include <macros.h>

// Size of primary volume descriptor for ISO9660
#define ISO_PVD_SIZE 882

// ISO9660 should start with this string
const char *kISO9660IDString = "CD001";

// We don't handle the following formats
const char *kHighSierraIDString = "CDROM";
const char *kCompactDiscInteractiveIDString = "CD-I";

static int GetDeviceBlockSize( int fd );
static int InitVolDate( ISOVolDate * date, char *buf );
static int InitRecDate( ISORecDate * date, char *buf );
static int InitVolDesc( nspace * vol, char *buf );

static int GetDeviceBlockSize( int fd )
{
	struct stat st;
	device_geometry dg;

	kerndbg( KERN_DEBUG_LOW, "GetDeviceBlockSize( int fd = %d )\n", fd );
	if( ioctl( fd, IOCTL_GET_DEVICE_GEOMETRY, &dg ) != -EOK )
	{
		if( fstat( fd, &st ) < 0 || S_ISDIR( st.st_mode ) )
			return 0;
		return 512;	/* just assume it's a plain old file or something */
	}

	return dg.bytes_per_sector;
}

int ISO_HIDDEN ISOMount( const char *path, const int flags, nspace ** newVol, bool allow_rockridge, bool allow_joliet )
{
	// path:                path to device (eg, /dev/disk/scsi/030/raw)
	// partition:   partition number on device ????
	// flags:               currently unused

	nspace *vol = NULL;
	int result = -EOK;

	kerndbg( KERN_DEBUG, "ISOMount - ENTER\n" );

	vol = ( nspace * ) calloc( sizeof( nspace ), 1 );
	if( vol == NULL )
	{
		result = -ENOMEM;
		kerndbg( KERN_WARNING, "ISOMount - mem error \n" );
	}
	else
	{
		/* open and lock the device */
		vol->fd = open( path, O_RDONLY );

		if( vol->fd >= 0 )
		{
			//int deviceBlockSize, logicalBlockSize, multiplier;
			int deviceBlockSize, multiplier;

			deviceBlockSize = GetDeviceBlockSize( vol->fd );
			if( deviceBlockSize == 0 )
			{
				kerndbg( KERN_WARNING, "ISO9660 ERROR - device block size is 0\n" );
				if( vol->fd >= 0 )
					close( vol->fd );
				result = -EINVAL;
				free( vol );
			}
			else
			{
				kerndbg( KERN_DEBUG_LOW, "ISOMount: block size is %d\n", deviceBlockSize );

				// determine if it is an ISO volume.
				char buf[ISO_PVD_SIZE], svd_buf[ISO_PVD_SIZE];
				bool done = false;
				bool is_iso = false, has_joliet = false;
				uint8 joliet_level = 0;
				off_t offset = 0x8000;
				ssize_t retval;

				/* Always assume that RockRidge is available when the root inode is read by InitVolDesc().
				   We'll switch it off later if required. */
				vol->rockRidge = true;
				vol->joliet_level = 0;

				while( ( !done ) && ( offset < 0x10000 ) )
				{
					retval = read_pos( vol->fd, offset, ( void * )buf, ISO_PVD_SIZE );
					if( retval == ISO_PVD_SIZE )
					{
						if( !strncmp( buf + 1, kISO9660IDString, 5 ) )
						{
							if( ( *buf == 0x01 ) && ( !is_iso ) )
							{
								/* ISO_VD_PRIMARY */
								kerndbg( KERN_DEBUG_LOW, "ISOMount: Is an ISO9660 volume, initializing rec\n" );

								InitVolDesc( vol, buf );
								strncpy( vol->devicePath, path, 127 );
								vol->id = vol->rootDirRec.id;

								multiplier = deviceBlockSize / vol->logicalBlkSize[FS_DATA_FORMAT];
								kerndbg( KERN_DEBUG_LOW, "ISOMount: block size multiplier is %d\n", multiplier );

								is_iso = true;
							}
							else if( ( *buf == 0x02 ) && is_iso )
							{
								/* ISO_VD_SUPPLEMENTARY */
								kerndbg( KERN_DEBUG_LOW, "ISOMount: found ISO_VD_SUPPLEMENTARY\n" );

								/* Joliet SVD */
								if( buf[88] == 0x25 && buf[89] == 0x2f )
								{
									has_joliet = true;

									switch ( buf[90] )
									{
									case 0x40:
										joliet_level = 1;
										break;
									case 0x43:
										joliet_level = 2;
										break;
									case 0x45:
										joliet_level = 3;
										break;
									}

									/* We'll need a copy of the SVD later if we decide to use Joliet */
									memcpy( svd_buf, buf, ISO_PVD_SIZE );

									kerndbg( KERN_DEBUG_LOW, "ISOMount: Microsoft Joliet level %d\n", joliet_level );
								}
							}
							else if( *( unsigned char * )buf == 0xff )
							{
								/* ISO_VD_END */

								kerndbg( KERN_DEBUG_LOW, "ISOMount: found ISO_VD_END\n" );
								done = true;
							}
						}
						offset += 0x800;
					}
					else
					{
						kerndbg( KERN_DEBUG_LOW, "ISOMount: reading of PVD failed\n" );
						done = true;
						is_iso = false;
					}
				}

				if( is_iso )
				{
					/* If the CD has both Joliet & RockRidge, we want to select RockRidge over
					   Joliet. We'll have to read the "." entry from the root directory of the volume
					   to find out if any RockRidge SUSP extensions are in use. This is necasary because
					   the "root" directory record contained within the PVD can not contain any System Use
					   entries, so they have to be stored on the entry for "." instead.

					   Note: according to the SUSP spec, a System Use entry "ER" should appear in the
					   entry to indicate that SUSP is in use, but on the CDs I've tried there was none.
					   This is possibly because the ER entry is contained within the CE (Continuation Area),
					   which this driver does not support. Instead we use the SP field, which is mandatory
					   for all SUSP nodes, to inidicate if RockRidge extensions are in use).
					*/

					off_t block = vol->rootBlock;
					size_t blockSize = vol->logicalBlkSize[FS_DATA_FORMAT];

					char root_buf[blockSize];
					read_pos( vol->fd, block * blockSize, (void *)root_buf, blockSize );

					char fileIDString[2] = {0};
					vnode *root = calloc( 1, sizeof( vnode ) );
					root->fileIDString = fileIDString;
					InitNode( vol, root, root_buf, NULL, 0 );

					kerndbg( KERN_DEBUG, "has_rockridge=%s, allow_rockridge=%s, has_joliet=%s (joilet_level=%d), allow_joliet=%s\n",
						(root->has_rockridge?"true":"false"),
						(allow_rockridge?"true":"false"),
						(has_joliet?"true":"false"),
						joliet_level,
						(allow_joliet?"true":"false")
					);

					/* If the disc uses RockRidge, ignore Joliet */

					if( root->has_rockridge && allow_rockridge )
					{
						vol->joliet_level = 0;
						vol->rockRidge = true;
					}
					else if( has_joliet && allow_joliet )
					{
						vol->joliet_level = joliet_level;
						vol->rockRidge = false;

						/* Re-read the root inode from the SVD */
						InitNode( vol, &(vol->rootDirRec), &svd_buf[156], NULL, 0 );
					}
					else
					{
						vol->joliet_level = 0;
						vol->rockRidge = false;
					}

					free( root );
				}
				else
				{
					kerndbg( KERN_DEBUG_LOW, "ISOMount: This is not iso volume\n" );
					if( vol->fd >= 0 )
						close( vol->fd );
					free( vol );
					vol = NULL;
					result = -EINVAL;
				}
			}
		}
		else
		{
			kerndbg( KERN_DEBUG_LOW, "ISOMount: Unable to open device\n" );
			free( vol );
			vol = NULL;
			result = -EINVAL;
		}
	}

	*newVol = vol;
	return result;
}

/* ISOReadDirEnt */

/* Reads in a single directory entry and fills in the values in the
	dirent struct. Uses the cookie to keep track of the current block
	and position withing the block. Also uses the cookie to determine when
	it has reached the end of the directory file.
	
	NOTE: If your file sytem seems to work ok from the command line, but
	the tracker doesn't seem to like it, check what you do here closely;
	in particular, if the d_ino in the stat struct isn't correct, the tracker
	will not display the entry.
*/

// SYLLABLE dirent -> kernel_dirent
int ISO_HIDDEN ISOReadDirEnt( nspace * volume, dircookie *cookie, struct kernel_dirent *buf, size_t bufsize )
{
	off_t totalRead;
	off_t cacheBlock;
	char *blockData;
	int result = -EOK;
	int bytesRead = 0;
	bool blockOut = false;

	kerndbg( KERN_DEBUG, "ISOReadDirEnt - ENTER\n" );

	totalRead = cookie->pos + ( ( cookie->block - cookie->startBlock ) * volume->logicalBlkSize[FS_DATA_FORMAT] );

	kerndbg( KERN_DEBUG_LOW, "ISOReadDirEnt - total read set to %Ld", totalRead );
	// If we're at the end of the data in a block, move to the next block.  
	while( 1 )
	{
		//kerndbg( KERN_DEBUG_LOW, "ISOReadDirEnt - read a block\n" );
		blockData = ( char * )get_cache_block( volume->fd, cookie->block, volume->logicalBlkSize[FS_DATA_FORMAT] );
		blockOut = true;

		if( blockData != NULL && *( blockData + cookie->pos ) == 0 )
		{
			//NULL data, move to next block.
			//kerndbg( KERN_DEBUG_LOW, "ISOReadDirEnt - at the end of a block");
			release_cache_block( volume->fd, cookie->block );
			blockOut = false;
			totalRead += volume->logicalBlkSize[FS_DATA_FORMAT] - cookie->pos;
			cookie->pos = 0;
			cookie->block++;
		}
		else
			break;
		if( totalRead >= cookie->totalSize )
			break;
	}

	cacheBlock = cookie->block;
	if( blockData != NULL && totalRead < cookie->totalSize )
	{
		vnode node;
		char name[ISO_MAX_FILENAME_LENGTH];
		char symbolicLinkName[ISO_MAX_FILENAME_LENGTH];

		node.fileIDString = name;
		node.attr.slName = symbolicLinkName;

		if( ( ( ( result = InitNode( volume, &node, blockData + cookie->pos, &bytesRead, volume->joliet_level ) ) ) == -EOK ) )
		{
			int nameBufSize = ( bufsize - ( 2 * sizeof( dev_t ) + 2 * sizeof( ino_t ) + sizeof( unsigned short ) ) );

			buf->d_ino = ( cookie->block << 30 ) + ( cookie->pos & 0x3FFFFFFF );
			buf->d_namlen = node.fileIDLen;
			if( node.fileIDLen <= nameBufSize )
			{
				// need to do some size checking here.
				strncpy( buf->d_name, node.fileIDString, node.fileIDLen + 1 );
				kerndbg( KERN_DEBUG_LOW, "ISOReadDirEnt  - success, name is %s\n", buf->d_name );
			}
			else
			{
				kerndbg( KERN_WARNING, "ISOReadDirEnt - ERROR, name %s does not fit in buffer of size %d\n", node.fileIDString, nameBufSize );
				result = -EINVAL;
			}
			cookie->pos += bytesRead;
		}
		//release_cache_block(volume->fd, cacheBlock);
	}
	else
	{
		if( totalRead >= cookie->totalSize )
			result = -ENOENT;
		else
			result = -ENOMEM;
	}

	if( blockOut )
		release_cache_block( volume->fd, cacheBlock );

	kerndbg( KERN_DEBUG, "ISOReadDirEnt - EXIT (%d), vnid is %Lu\n", result, buf->d_ino );
	return result;
}

// InitVolDesc handles only Primary Volume Descriptors.

int ISO_HIDDEN InitVolDesc( nspace * vol, char *buf )
{
	vnode *node;

	kerndbg( KERN_DEBUG, "InitVolDesc - ENTER\n" );

	vol->volDescType = *( uint8 * )buf++;

	vol->stdIDString[5] = '\0';
	strncpy( vol->stdIDString, buf, 5 );
	buf += 5;

	vol->volDescVersion = *( uint8 * )buf;
	buf += 2;		// 8th byte unused

	vol->systemIDString[32] = '\0';
	strncpy( vol->systemIDString, buf, 32 );
	buf += 32;
	kerndbg( KERN_DEBUG_LOW, "InitVolDesc - system id string is %s\n", vol->systemIDString );

	vol->volIDString[32] = '\0';
	strncpy( vol->volIDString, buf, 32 );
	buf += ( 32 + 80 - 73 + 1 );	// bytes 80-73 unused
	kerndbg( KERN_DEBUG_LOW, "InitVolDesc - volume id string is %s\n", vol->volIDString );

	vol->volSpaceSize[LSB_DATA] = *( uint32 * )buf;
	buf += 4;
	vol->volSpaceSize[MSB_DATA] = *( uint32 * )buf;
	buf += ( 4 + 120 - 89 + 1 );	// bytes 120-89 unused

	vol->volSetSize[LSB_DATA] = *( uint16 * )buf;
	buf += 2;
	vol->volSetSize[MSB_DATA] = *( uint16 * )buf;
	buf += 2;

	vol->volSeqNum[LSB_DATA] = *( uint16 * )buf;
	buf += 2;
	vol->volSeqNum[MSB_DATA] = *( uint16 * )buf;
	buf += 2;

	vol->logicalBlkSize[LSB_DATA] = *( uint16 * )buf;
	buf += 2;
	vol->logicalBlkSize[MSB_DATA] = *( uint16 * )buf;
	buf += 2;

	vol->pathTblSize[LSB_DATA] = *( uint32 * )buf;
	buf += 4;
	vol->pathTblSize[MSB_DATA] = *( uint32 * )buf;
	buf += 4;

	vol->lPathTblLoc[LSB_DATA] = *( uint16 * )buf;
	buf += 2;
	vol->lPathTblLoc[MSB_DATA] = *( uint16 * )buf;
	buf += 2;

	vol->optLPathTblLoc[LSB_DATA] = *( uint16 * )buf;
	buf += 2;
	vol->optLPathTblLoc[MSB_DATA] = *( uint16 * )buf;
	buf += 2;

	vol->mPathTblLoc[LSB_DATA] = *( uint16 * )buf;
	buf += 2;
	vol->mPathTblLoc[MSB_DATA] = *( uint16 * )buf;
	buf += 2;

	vol->optMPathTblLoc[LSB_DATA] = *( uint16 * )buf;
	buf += 2;
	vol->optMPathTblLoc[MSB_DATA] = *( uint16 * )buf;
	buf += 2;

	// Read the volume root node
	node = &( vol->rootDirRec );
	node->fileIDString = calloc( ISO_MAX_FILENAME_LENGTH, 1 );
	node->attr.slName = calloc( ISO_MAX_FILENAME_LENGTH, 1 );
	InitNode( vol, &( vol->rootDirRec ), buf, NULL, 0 );

	// error checking...FIXME!!!

	vol->rootDirRec.id = BLOCK_TO_INO( vol->rootDirRec.startLBN[FS_DATA_FORMAT], 0 );
	vol->rootBlock = vol->rootDirRec.startLBN[FS_DATA_FORMAT];

	//vol->rootDirRec.parID = ???
	buf += 34;

	vol->volSetIDString[128] = '\0';
	strncpy( vol->volSetIDString, buf, 128 );
	buf += 128;
	kerndbg( KERN_DEBUG_LOW, "InitVolDesc - volume set id string is %s\n", vol->volSetIDString );

	vol->pubIDString[128] = '\0';
	strncpy( vol->pubIDString, buf, 128 );
	buf += 128;
	kerndbg( KERN_DEBUG_LOW, "InitVolDesc - volume publisher id string is %s\n", vol->pubIDString );

	vol->dataPreparer[128] = '\0';
	strncpy( vol->dataPreparer, buf, 128 );
	buf += 128;
	kerndbg( KERN_DEBUG_LOW, "InitVolDesc - volume dataPreparer string is %s\n", vol->dataPreparer );

	vol->appIDString[128] = '\0';
	strncpy( vol->appIDString, buf, 128 );
	buf += 128;
	kerndbg( KERN_DEBUG_LOW, "InitVolDesc - volume application id string is %s\n", vol->appIDString );

	vol->copyright[37] = '\0';
	strncpy( vol->copyright, buf, 37 );
	buf += 37;
	kerndbg( KERN_DEBUG_LOW, "InitVolDesc - copyright is %s\n", vol->copyright );

	vol->abstractFName[37] = '\0';
	strncpy( vol->abstractFName, buf, 37 );
	buf += 37;
	kerndbg( KERN_DEBUG_LOW, "InitVolDesc - abstract file id string is %s\n", vol->abstractFName );

	vol->biblioFName[37] = '\0';
	strncpy( vol->biblioFName, buf, 37 );
	buf += 37;
	kerndbg( KERN_DEBUG_LOW, "InitVolDesc - bibliographic file identifier: %s\n", vol->biblioFName );

	InitVolDate( &( vol->createDate ), buf );
	buf += 17;

	InitVolDate( &( vol->modDate ), buf );
	buf += 17;

	InitVolDate( &( vol->expireDate ), buf );
	buf += 17;

	InitVolDate( &( vol->effectiveDate ), buf );
	buf += 17;

	vol->fileStructVers = *( uint8 * )buf;
	return 0;
}

int ISO_HIDDEN InitNode( nspace * volume, vnode *rec, char *buf, int *bytesRead, uint8 joliet_level )
{
	int result = -EOK;
	uint8 recLen = *( uint8 * )buf++;
	bool no_rock_ridge_stat_struct = true;

	if( bytesRead != NULL )
		*bytesRead = recLen;

	kerndbg( KERN_DEBUG, "InitNode - ENTER\n" );

	/* Assume the node does not have RockRidge by default */
	rec->has_rockridge = false;

	if( recLen > 0 )
	{
		rec->extAttrRecLen = *( uint8 * )( buf++ );
		rec->startLBN[LSB_DATA] = *( uint32 * )( buf );
		buf += 4;
		rec->startLBN[MSB_DATA] = *( uint32 * )( buf );
		buf += 4;
		kerndbg( KERN_DEBUG_LOW, "InitNode - data start LBN is %d\n", rec->startLBN[FS_DATA_FORMAT] );
		rec->dataLen[LSB_DATA] = *( uint32 * )( buf );
		buf += 4;
		rec->dataLen[MSB_DATA] = *( uint32 * )( buf );
		buf += 4;
		kerndbg( KERN_DEBUG_LOW, "InitNode - data length is %d\n", rec->dataLen[FS_DATA_FORMAT] );
		InitRecDate( &( rec->recordDate ), buf );
		buf += 7;
		rec->flags = *( uint8 * )( buf );
		buf++;
		kerndbg( KERN_DEBUG_LOW, "InitNode - flags are %d\n", rec->flags );
		rec->fileUnitSize = *( uint8 * )( buf );
		buf++;
		kerndbg( KERN_DEBUG_LOW, "InitNode - fileUnitSize is %d\n", rec->fileUnitSize );
		rec->interleaveGapSize = *( uint8 * )( buf );
		buf++;
		kerndbg( KERN_DEBUG_LOW, "InitNode - interleave gap size = %d\n", rec->interleaveGapSize );
		rec->volSeqNum = *( uint32 * )( buf );
		buf += 4;
		kerndbg( KERN_DEBUG_LOW, "InitNode - volume seq num is %d\n", rec->volSeqNum );
		rec->fileIDLen = *( uint8 * )( buf );
		buf++;
		kerndbg( KERN_DEBUG_LOW, "InitNode - file id length is %d\n", rec->fileIDLen );
		if( rec->fileIDLen > 0 )
		{
			if( rec->fileIDLen == 1 )
			{
				// Take care of "." and "..", the first two dirents are
				// these in iso.
				if( buf[0] == 0 )
				{
					kerndbg( KERN_DEBUG_LOW, "InitNode - dir: . \n" );
					strncpy( rec->fileIDString, ".", 2 );
					rec->fileIDLen = 1;
					rec->fileIDString[rec->fileIDLen] = '\0';
				}
				else if( buf[0] == 1 )
				{
					kerndbg( KERN_DEBUG_LOW, "InitNode - dir: .. \n" );
					strncpy( rec->fileIDString, "..", 3 );
					rec->fileIDLen = 2;
					rec->fileIDString[rec->fileIDLen] = '\0';
				}
			}
			else
			{
				memset( rec->fileIDString, '\0', ISO_MAX_FILENAME_LENGTH );
				// JOLIET extension: convert Unicode 16 to UTF8
				if( joliet_level > 0 )
				{
					int err;

					err = nls_conv_cp_to_utf8( NLS_UTF16_BE, buf, rec->fileIDString, rec->fileIDLen, ISO_MAX_FILENAME_LENGTH );
					if( err < 0 )
					{
						printk( "iso9660: unable to convert unicode to utf8\n" );
						result = -EIO;
					}
					else
					{
						rec->fileIDString[err] = '\0';
						rec->fileIDLen = err;
					}
				}
				else
				{
					strncpy( rec->fileIDString, buf, rec->fileIDLen );
					rec->fileIDString[rec->fileIDLen] = '\0';
				}
			}

			//Get rid of semicolons, which are used to delineate file versions.q
			{
				char *semi = NULL;

				while( ( semi = strchr( rec->fileIDString, ';' ) ) != NULL )
				{
					semi[0] = '\0';
				}
			}

			kerndbg( KERN_DEBUG_LOW, "DirRec ID String is: %s\n", rec->fileIDString );
			kerndbg( KERN_DEBUG_LOW, "DirRec ID String length is: %d\n", rec->fileIDLen );

			memset( &( rec->attr.stat ), 0, 2 * sizeof( struct stat ) );
			// Set defaults, in case there is no RR stuff.
			rec->attr.stat[FS_DATA_FORMAT].st_mode = ( S_IRUSR | S_IRGRP | S_IROTH );

			if( volume->rockRidge == false )
			{
				kerndbg( KERN_DEBUG_LOW, "ROCK RIDGE extensions DISABLED\n" );
			}
			else
			{
				kerndbg( KERN_DEBUG_LOW, "ROCK RIDGE extensions ENABLED\n" );
				//if( result == EOK )
				{
					buf += rec->fileIDLen;
					if( !( rec->fileIDLen % 2 ) )
						buf++;

					// Now we're at the start of the rock ridge stuff
					{
						char altName[ISO_MAX_FILENAME_LENGTH];
						char slName[ISO_MAX_FILENAME_LENGTH];
						uint16 altNameSize = 0;
						uint16 slNameSize = 0;
						uint8 slFlags = 0;
						uint8 length = 0;
						bool done = false;

						kerndbg( KERN_DEBUG_LOW, "RR: Start of extensions, but at %p\n", buf );

						while( !done )
						{
							buf += length;
							length = *( uint8 * )( buf + 2 );

							kerndbg( KERN_DEBUG_LOW, "%c %c\n", buf[0], buf[1] );

							switch ( SUSP_TAG( buf[0], buf[1] ) )
							{
								// POSIX file attributes
							case SUSP_TAG( 'P', 'X' ):
								{
									uint8 bytePos = 3;

									kerndbg( KERN_DEBUG_LOW, "RR: found PX, length %u\n", length );

									rec->attr.pxVer = *( uint8 * )( buf + bytePos++ );
									no_rock_ridge_stat_struct = false;
									memset( &( rec->attr.stat ), 0, 2 * sizeof( struct stat ) );
									// st_mode
									rec->attr.stat[LSB_DATA].st_mode = *( mode_t * )( buf + bytePos );
									bytePos += 4;
									rec->attr.stat[MSB_DATA].st_mode = *( mode_t * )( buf + bytePos );
									bytePos += 4;
									// st_nlink
									rec->attr.stat[LSB_DATA].st_nlink = *( nlink_t * )( buf + bytePos );
									bytePos += 4;
									rec->attr.stat[MSB_DATA].st_nlink = *( nlink_t * )( buf + bytePos );
									bytePos += 4;
									// st_uid
									rec->attr.stat[LSB_DATA].st_uid = *( uid_t * )( buf + bytePos );
									bytePos += 4;
									rec->attr.stat[MSB_DATA].st_uid = *( uid_t * )( buf + bytePos );
									bytePos += 4;
									// st_gid
									rec->attr.stat[LSB_DATA].st_gid = *( gid_t * )( buf + bytePos );
									bytePos += 4;
									rec->attr.stat[MSB_DATA].st_gid = *( gid_t * )( buf + bytePos );
									bytePos += 4;
									break;
								}
								// POSIX Device Number
							case SUSP_TAG( 'P', 'N' ):
								kerndbg( KERN_DEBUG, "RR: found PN, length %u - NOT IMPLEMENTED\n", length );
								break;
								// Symbolic link
							case SUSP_TAG( 'S', 'L' ):
								{
									uint8 bytePos = 3;
									uint8 lastCompFlag = 0;
									uint8 addPos = 0;
									bool slDone = false;
									bool useSeparator = true;

									kerndbg( KERN_DEBUG_LOW, "RockRidge: found SL, length %u\n", length );
									rec->attr.slVer = *( uint8 * )( buf + bytePos++ );
									slFlags = *( uint8 * )( buf + bytePos++ );
									while( !slDone && bytePos < length )
									{
										uint8 compFlag = *( uint8 * )( buf + bytePos++ );
										uint8 compLen = *( uint8 * )( buf + bytePos++ );

										if( slNameSize == 0 )
											useSeparator = false;
										addPos = slNameSize;
										kerndbg( KERN_DEBUG_LOW, "RockRidge: addPos= %d\n", addPos );
										kerndbg( KERN_DEBUG_LOW, "Current name size is %u\n", slNameSize );
										switch ( compFlag )
										{
										case SLCP_CONTINUE:
											kerndbg( KERN_DEBUG_LOW, "SLCP_CONTINUE\n" );
											useSeparator = false;
										default:
											// Add the component to the total path.
											kerndbg( KERN_DEBUG_LOW, "default\n" );
											slNameSize += compLen;
											if( useSeparator )
											{
												// KV: Don't add the seperator to the begining
												// of a link, as this incorrectly forces all symlinks
												// to become relative to /, which is certainly not
												// what we want!
												kerndbg( KERN_DEBUG_LOW, "Adding separator\n" );
												slName[addPos++] = '/';
												slNameSize++;
											}
											memcpy( ( slName + addPos ), ( buf + bytePos ), compLen );
											addPos += compLen;
											useSeparator = true;
											break;
										case SLCP_CURRENT:
											kerndbg( KERN_DEBUG_LOW, "RockRidge - found link to current directory\n" );
											slNameSize += 2;
											memcpy( slName + addPos, "./", 2 );
											useSeparator = false;
											break;
										case SLCP_PARENT:
											kerndbg( KERN_DEBUG_LOW, "RockRidge - found link to parent\n" );
											slNameSize += 3;
											memcpy( slName + addPos, "../", 3 );
											useSeparator = false;
											break;
										case SLCP_ROOT:
											kerndbg( KERN_DEBUG_LOW, "RockRidge -  found link to root directory\n" );
											slNameSize += 1;
											memcpy( slName + addPos, "/", 1 );
											useSeparator = false;
											break;
										case SLCP_VOLROOT:
											kerndbg( KERN_DEBUG_LOW, "RockRidge - SLCP_VOLROOT\n" );
											slDone = true;
											break;
										case SLCP_HOST:
											kerndbg( KERN_DEBUG_LOW, "RockRidge - SLCP_HOST\n" );
											slDone = true;
											break;
										}
										slName[slNameSize] = '\0';
										lastCompFlag = compFlag;
										bytePos += compLen;
									}
								}
								// copy the name to the node structure
								strncpy( rec->attr.slName, slName, slNameSize );
								kerndbg( KERN_DEBUG_LOW, "RockRidge - symlink name is \'%s\'\n", slName );
								kerndbg( KERN_DEBUG_LOW, "InitNode - name length is %d\n", slNameSize );
								break;
								// Alternate name
							case SUSP_TAG( 'N', 'M' ):
								{
									uint8 bytePos = 3;
									uint8 flags = 0;
									uint16 oldEnd = altNameSize;

									altNameSize += length - 5;
									kerndbg( KERN_DEBUG_LOW, "RR: found NM, length %u\n", length );
									// Read flag and version.
									rec->attr.nmVer = *( uint8 * )( buf + bytePos++ );
									flags = *( uint8 * )( buf + bytePos++ );
									// Build the file name.
									memcpy( altName + oldEnd, buf + bytePos, length - 5 );
									altName[altNameSize] = '\0';
									kerndbg( KERN_DEBUG_LOW, "RR: alternate name is %s\n", altName );
									// If the name is not continued in another record, update
									// the record name.
									if( !( flags & NM_CONTINUE ) )
									{
										// Get rid of the ISO name, replace with RR name.
										memcpy( rec->fileIDString, altName, ISO_MAX_FILENAME_LENGTH );
										rec->fileIDLen = altNameSize;
									}
								}
								break;
								// Deep directory record masquerading as a file.
								// Child Link
							case SUSP_TAG( 'C', 'L' ):
								kerndbg( KERN_DEBUG_LOW, "RR: found CL, length %u\n", length );
								rec->flags |= ISO_ISDIR;
								rec->startLBN[LSB_DATA] = *( uint32 * )( buf + 4 );
								rec->startLBN[MSB_DATA] = *( uint32 * )( buf + 8 );
								break;
								// Parent Link
							case SUSP_TAG( 'P', 'L' ):
								kerndbg( KERN_DEBUG, "RR: found PL, length %u - NOT IMPLEMENTED\n", length );
								break;
								// Relocated directory, we should skip.
							case SUSP_TAG( 'R', 'E' ):
								result = -EINVAL;
								kerndbg( KERN_DEBUG, "RR: found RE, length %u - NOT IMPLEMENTED\n", length );
								break;
								// Time Stamp
							case SUSP_TAG( 'T', 'F' ):
								kerndbg( KERN_DEBUG, "RR: found TF, length %u - NOT IMPLEMENTED\n", length );
								break;
								// File data in sparse file format
							case SUSP_TAG( 'S', 'F' ):
								result = -EINVAL;
								kerndbg( KERN_DEBUG, "RR: found SF, length %u - NOT IMPLEMENTED\n", length );
								break;
								// Extension Reference - for the purpose of identifying discs with RR
							case SUSP_TAG( 'E', 'R' ):
								kerndbg( KERN_DEBUG, "RR: found ER, length %u - NOT IMPLEMENTED\n", length );
								break;
								// Cant find this in the spec (v.1.12)
							case SUSP_TAG( 'R', 'R' ):
								kerndbg( KERN_DEBUG, "RR: found RR, length %u - WHAT'S THIS??\n", length );
								break;
							case SUSP_TAG( 'S', 'P' ):
							{
								kerndbg( KERN_DEBUG, "RR: found SP, length %u - NOT IMPLEMENTED\n", length );

								/* XXXKV: An SP entry is mandatory, so we can abuse it for the purposes of knowing
								   if any SUSP is in use (Which is mearly part of a much larger hack: see the stuff
								   in InitVolDesc() for more information) */
								rec->has_rockridge = true;
								break;
							}
								// Continuation Area
							case SUSP_TAG( 'C', 'E' ):
								kerndbg( KERN_DEBUG, "RR: found CE, length %u - NOT IMPLEMENTED\n", length );
								break;

							default:
								kerndbg( KERN_DEBUG_LOW, "RockRidge: End of extensions.\n" );
								done = true;
								break;
							}
						}
					}
				}
			}	// rockRidge == true
		}
		else
		{
			kerndbg( KERN_DEBUG_LOW, "InitNode - File ID String is 0 length\n" );
			result = -ENOENT;
		}
	}
	else
	{
		result = -ENOENT;
	}
	kerndbg( KERN_DEBUG, "InitNode - EXIT (%d),  name is \'%s\'\n", result, rec->fileIDString );

	// "A "PX" System Use Entry shall be recorded in each Directory Record -> move this to RR DISABLED
	if( result == EOK && no_rock_ridge_stat_struct )
	{
		// Set defaults, in case there is no RR stuff.
		memset( &( rec->attr.stat ), 0, 2 * sizeof( struct stat ) );
		rec->attr.stat[FS_DATA_FORMAT].st_mode = ( S_IRUSR | S_IRGRP | S_IROTH );
		if( rec->flags & ISO_ISDIR )
			rec->attr.stat[FS_DATA_FORMAT].st_mode |= ( S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH );
		else
			rec->attr.stat[FS_DATA_FORMAT].st_mode |= ( S_IFREG );
	}

	return result;
}

static int InitVolDate( ISOVolDate * date, char *buf )
{
	memcpy( date, buf, 17 );
	return 0;
}

static int InitRecDate( ISORecDate * date, char *buf )
{
	memcpy( date, buf, 7 );
	return 0;
}

int ISO_HIDDEN ConvertRecDate( ISORecDate * inDate, time_t *outDate )
{
	time_t time;
	int days, i, year, tz;

	year = inDate->year - 70;
	tz = inDate->offsetGMT;

	if( year < 0 )
	{
		time = 0;
	}
	else
	{
		int monlen[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
		days = ( year * 365 );
		if( year > 2 )
		{
			days += ( year + 1 ) / 4;
		}
		for( i = 1; ( i < inDate->month ) && ( i < 12 ); i++ )
		{
			days += monlen[i - 1];
		}
		if( ( ( year + 2 ) % 4 ) == 0 && inDate->month > 2 )
		{
			days++;
		}
		days += inDate->date - 1;
		time = ( ( ( ( days * 24 ) + inDate->hour ) * 60 + inDate->minute ) * 60 ) + inDate->second;
		if( tz & 0x80 )
		{
			tz |= ( -1 << 8 );
		}
		if( -48 <= tz && tz <= 52 )
			time += tz * 15 * 60;
	}
	*outDate = time;
	return 0;
}
