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

#include <atheos/bcache.h>
#include <atheos/device.h>
#include <atheos/kernel.h>
#include <macros.h>

#include "rock.h"
#include "iso.h"

// Size of primary volume descriptor for ISO9660
#define ISO_PVD_SIZE 882

// ISO9660 should start with this string
const char* 	kISO9660IDString = "CD001";
// We don't handle the following formats
const char*		kHighSierraIDString = "CDROM";
const char*		kCompactDiscInteractiveIDString = "CD-I";

#if 0
static int   	GetLogicalBlockSize(int fd);
static off_t 	GetNumDeviceBlocks(int fd, int block_size);
#endif

static int   	GetDeviceBlockSize(int fd);

static int		InitVolDate(ISOVolDate* date, char* buf);
static int		InitRecDate(ISORecDate* date, char* buf);
static int		InitVolDesc(nspace* vol, char* buf);

// the joliet extensions are still missing
// Just needed here (for Joliet)
#if 0
static status_t unicode_to_utf8(const char        *src, int32     *srcLen, char *dst, int32 *dstLen); 
#endif

#if 0 // currently unused
static int
GetLogicalBlockSize(int fd)
{
// grepping trough the source, can't find where this would be used.

    partition_info  p_info;

	dprintf( "GetLogicalBlockSize - ENTER \n" );
    if (ioctl(fd, B_GET_PARTITION_INFO, &p_info) == B_NO_ERROR)
    {
    	dprintf("GetLogicalBlockSize: ioctl succeed\n");
		return p_info.logical_block_size;
	}
    else 
    dprintf("GetLogicalBlockSize = ioctl returned error\n")

	
	dprintf( "GetLogicalBlockSize - EXIT - NOT IMPLEMENTED\n" );
	return 0;
}

static int
GetDeviceBlockSize(int fd)
{
    struct stat     st;
    device_geometry dg;
	
	dprintf( "GetDeviceBlockSize - ENTER\n" );
    if (ioctl(fd, IOCTL_GET_DEVICE_GEOMETRY, &dg) < 0) 
    {
		if (fstat(fd, &st) < 0 || S_ISDIR(st.st_mode) )
	    	return 0;
		return 512;   // just assume it's a plain old file or something
    }
    dprintf( "GetDeviceBlockSize - block size: %d\n", dg.bytes_per_sector );
    dprintf( "GetDeviceBlockSize - EXIT\n" );
    return dg.bytes_per_sector;
}
#endif

#if 0 
// This is the old version...
static off_t
GetNumDeviceBlocks(int fd, int block_size)
{
    struct stat			st;
    device_geometry		dg;

    if (ioctl(fd, IOCTL_GET_DEVICE_GEOMETRY, &dg) > 0) 
    {
		return (off_t)dg.cylinder_count *
	       (off_t)dg.sectors_per_track *
	       (off_t)dg.head_count;
    }

    /* if the ioctl fails, try just stat'ing in case it's a regular file */
    if (fstat(fd, &st) < 0)
	return 0;

    return st.st_size / block_size;
}
#endif


static int 
GetDeviceBlockSize(int fd) 
{ 
	struct stat     st; 
	device_geometry dg; 

	dprintf( "GetDeviceBlockSize( int fd = %d )\n", fd );
	if ( ioctl(fd, IOCTL_GET_DEVICE_GEOMETRY, &dg) != -EOK )  
	{ 
		if (fstat(fd, &st) < 0 || S_ISDIR(st.st_mode)) 
			return 0; 
		return 512;   /* just assume it's a plain old file or something */ 
	} 

	return dg.bytes_per_sector; 
} 


// We don't support MS Joliet extensions yet.
// From EncodingComversions.cpp 

// Pierre's (modified) Uber Macro 

// NOTE: iso9660 seems to store the unicode text in big-endian form 
#if 0
#define u_to_utf8(str, uni_str)\ 
{\ 
        if ((B_BENDIAN_TO_HOST_INT16(uni_str[0])&0xff80) == 0)\ 
                *str++ = B_BENDIAN_TO_HOST_INT16(*uni_str++);\ 
        else if ((B_BENDIAN_TO_HOST_INT16(uni_str[0])&0xf800) == 0) {\ 
                str[0] = 0xc0|(B_BENDIAN_TO_HOST_INT16(uni_str[0])>>6);\ 
                str[1] = 0x80|(B_BENDIAN_TO_HOST_INT16(*uni_str++)&0x3f);\ 
                str += 2;\ 
        } else if ((B_BENDIAN_TO_HOST_INT16(uni_str[0])&0xfc00) != 0xd800) {\ 
                str[0] = 0xe0|(B_BENDIAN_TO_HOST_INT16(uni_str[0])>>12);\ 
                str[1] = 0x80|((B_BENDIAN_TO_HOST_INT16(uni_str[0])>>6)&0x3f);\ 
                str[2] = 0x80|(B_BENDIAN_TO_HOST_INT16(*uni_str++)&0x3f);\ 
                str += 3;\ 
        } else {\ 
                int   val;\ 
                val = ((B_BENDIAN_TO_HOST_INT16(uni_str[0])-0xd7c0)<<10) | (B_BENDIAN_TO_HOST_INT16(uni_str[1])&0x3ff);\ 
                str[0] = 0xf0 | (val>>18);\ 
                str[1] = 0x80 | ((val>>12)&0x3f);\ 
                str[2] = 0x80 | ((val>>6)&0x3f);\ 
                str[3] = 0x80 | (val&0x3f);\ 
                uni_str += 2; str += 4;\ 
        }\ 
}       

static status_t 
unicode_to_utf8(const char        *src, int32     *srcLen, char *dst, int32 *dstLen) 
{ 
        int32 srcLimit = *srcLen; 
        int32 dstLimit = *dstLen; 
        int32 srcCount = 0; 
        int32 dstCount = 0; 
        
        for (srcCount = 0; srcCount < srcLimit; srcCount += 2) { 
                uint16  *UNICODE = (uint16 *)&src[srcCount]; 
                uchar   utf8[4]; 
                uchar   *UTF8 = utf8; 
                int32 utf8Len; 
                int32 j; 

                u_to_utf8(UTF8, UNICODE); 

                utf8Len = UTF8 - utf8; 
                if ((dstCount + utf8Len) > dstLimit) 
                        break; 

                for (j = 0; j < utf8Len; j++) 
                        dst[dstCount + j] = utf8[j]; 
                dstCount += utf8Len; 
        } 

        *srcLen = srcCount; 
        *dstLen = dstCount; 
        
        return ((dstCount > 0) ? B_NO_ERROR : B_ERROR); 
} 

#endif


int
ISOMount(const char *path, const int flags, nspace** newVol)
{
	// path: 		path to device (eg, /dev/disk/scsi/030/raw)
	// partition:	partition number on device ????
	// flags:		currently unused

	nspace*		vol = NULL;
	int 		result = -EOK;
	
	dprintf("ISOMount - ENTER\n");
	
	vol = (nspace*)calloc(sizeof(nspace), 1);
	if (vol == NULL)
	{
		/// error.
		result = -ENOMEM;
		dprintf("ISOMount - mem error \n");
	}
	else
	{		
		/* open and lock the device */
		vol->fd = open(path, O_RDONLY);
		if (vol->fd >= 0)
		{
			//int deviceBlockSize, logicalBlockSize, multiplier;
			int deviceBlockSize, multiplier;
			
			deviceBlockSize = GetDeviceBlockSize(vol->fd);
			if (deviceBlockSize == 0) 
			{
				// Error
				dprintf("ISO9660 ERROR - device block size is 0\n");
				if (vol->fd >= 0) close(vol->fd);
				result = -EINVAL;
				free(vol);
			}
			else
			{
				// determine if it is an ISO volume.
				char buf[ISO_PVD_SIZE];
//				uint32	sizeInSectors = 0;
				// ecma:"The System Area shall occupy the Logical... Sector Numbers 0-15"
				// It seems that the standard allows the logical sector size to vary (>2048),
				// bu does it ever?
				
				//off_t dataAreaOffset = 16 * deviceBlockSize;
				uint8 volumeDescriptorType = 0;
				char	standardIdentifier[6];
				
				read_pos (vol->fd, 0x8000, (void*)buf, ISO_PVD_SIZE);
				
				dprintf("deviceBlockSize = %d\n", deviceBlockSize );
				dprintf("0x8000 = %d\n", 0x8000 );
				dprintf("ISOMount - Data Area starts at %Ld: %d\n", dataAreaOffset, (uint8)*buf );
				
				// Check which kind of a volume descriptor we have:
				
				volumeDescriptorType = (uint8)(*buf);
				dprintf( "ISOMount - The volume descriptor is of type: \n" );
				switch ( volumeDescriptorType )
				{
					case 0:
						dprintf( "Boot Record\n" );
						break;
					case 1:
						dprintf( "Primary Volume Descriptor\n" );
						break;
					case 2:
						dprintf( "Supplementary Volume Descriptor\n" );
						break;
					case 3:
						dprintf( "Volume Partition Descriptor\n" );
						break;
					case 255:
						dprintf( "Volume Descriptor Set Terminator\n" );
						break;
					default:
						if ( volumeDescriptorType >=4 && volumeDescriptorType <=254 )
							dprintf( " %d, reserved for future standardization\n", volumeDescriptorType );
						else
							dprintf( "error\n" );	
				}
					
				memcpy( standardIdentifier, buf+1, 5 );
				standardIdentifier[5] = '\0';
				dprintf( "\nStandard Identifier: %s\n", standardIdentifier );
				
				// if it is an ISO volume, initialize the volume descriptor
				if (!strncmp(buf+1, kISO9660IDString, 5))
				{
					dprintf("ISOMount: Is an ISO9660 volume, initting rec\n");
					InitVolDesc( vol, buf );
					strncpy(vol->devicePath,path,127);
					vol->id = ISO_ROOTNODE_ID;
					dprintf("ISO9660: vol->blockSize = %d\n",vol->logicalBlkSize[FS_DATA_FORMAT]); 
					multiplier = deviceBlockSize / vol->logicalBlkSize[FS_DATA_FORMAT];
					dprintf("ISOMount: block size multiplier is %d\n", multiplier);
				}
				else
				{
					// It isn't an ISO disk.
					dprintf( "ISOMount - tried to mount a cd of type:\n" );
					if ( !strncmp(buf+1, kCompactDiscInteractiveIDString, 3) )
						dprintf( "Compact Disc Interactive\n" );
					else 
						if ( !strncmp( buf+1, kHighSierraIDString, 5 ) )
							dprintf( "High Sierra" );
						else
						 	dprintf( "UNKNOWN type\n" );
						 	
					if (vol->fd >= 0) close(vol->fd);
					free(vol);
					vol = NULL;
					result = -EINVAL;
					dprintf("ISOMount: Not an ISO9660 volume!\n");
				}
			}
		}
		else
		{
			dprintf("ISO9660 ERROR - Unable to open <%s>\n", path);
			free(vol);
			vol = NULL;
			result = -EINVAL;
		}
	}
	
	dprintf("ISOMount - EXIT, result %d, returning %p\n", result/*strerror(result)*/, vol);
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
int
ISOReadDirEnt(nspace* volume, dircookie* cookie, struct kernel_dirent* buf, size_t bufsize)
{
	off_t		totalRead;
	off_t		cacheBlock;
	char*			blockData;
	int				result = -EOK;
	int 			bytesRead = 0;
	bool			blockOut = false;

	dprintf("ISOReadDirEnt - ENTER\n");
	
	totalRead = cookie->pos + ( (cookie->block - cookie->startBlock) *
				volume->logicalBlkSize[FS_DATA_FORMAT] );
	
	dprintf("ISOReadDirEnt - total read set to %Ld", totalRead );
	// If we're at the end of the data in a block, move to the next block.	
	while (1)
	{
		dprintf( "ISOReadDirEnt - read a block\n" );
		blockData = (char*)get_cache_block(volume->fd, cookie->block,
							volume->logicalBlkSize[FS_DATA_FORMAT]);
		blockOut = true;
		
		if (blockData != NULL && *(blockData + cookie->pos) == 0)
		{
			//NULL data, move to next block.
			dprintf("ISOReadDirEnt - at the end of a block");
			release_cache_block(volume->fd, cookie->block);
			blockOut = false;
			totalRead += volume->logicalBlkSize[FS_DATA_FORMAT] - cookie->pos;
			cookie->pos = 0;
			cookie->block++;
		} 
		else
			break;
		if (totalRead >= cookie->totalSize)
			break;
	}
	
	cacheBlock = cookie->block;
	if (blockData != NULL && totalRead < cookie->totalSize)
	{
		vnode node;
		char name[ISO_MAX_FILENAME_LENGTH];
		char symbolicLinkName[ISO_MAX_FILENAME_LENGTH];
		node.fileIDString = name;
		node.attr.slName = symbolicLinkName;
							
		if ((((result = InitNode( volume, &node, blockData + cookie->pos, &bytesRead))) == 
							-EOK))
		{
			int nameBufSize = (bufsize - (2 * sizeof(dev_t) + 2* sizeof(ino_t) +
					sizeof(unsigned short)));

			// 0x3FFFFFFF was 0xFFFFFFFF
			buf->d_ino = (cookie->block << 30) + (cookie->pos & 0x3FFFFFFF);
			buf->d_namlen = node.fileIDLen;
			if (node.fileIDLen <= nameBufSize)
			{
				// need to do some size checking here.
				strncpy(buf->d_name, node.fileIDString, node.fileIDLen +1);
				dprintf("ISOReadDirEnt  - success, name is %s\n", buf->d_name);
			}
			else
			{
				dprintf("ISOReadDirEnt - ERROR, name %s does not fit in buffer of size %d\n", node.fileIDString, nameBufSize);
				result = -EINVAL;
			}
			cookie->pos += bytesRead;
		}
		//release_cache_block(volume->fd, cacheBlock);
	}
	else 
	{
		if (totalRead >= cookie->totalSize) 
			result = -ENOENT;
		else 
			result = -ENOMEM;
	}
	
	if ( blockOut )
		release_cache_block(volume->fd, cacheBlock);
	
	dprintf("ISOReadDirEnt - EXIT, result is %d, vnid is %Lu\n", result,buf->d_ino);
	return result;
}

// InitVolDesc handles only Primary Volume Descriptors.

int
InitVolDesc(nspace* _vol, char* buf)
{	
	nspace * vol = (nspace*)_vol;
//	char * fileName;
	vnode * node;
	
	dprintf("InitVolDesc - ENTER\n");

	
	vol->volDescType = *(uint8*)buf++;
	
	vol->stdIDString[5] = '\0';
	strncpy(vol->stdIDString, buf, 5);
	buf += 5;
	
	vol->volDescVersion = *(uint8*)buf;
	buf += 2; // 8th byte unused
	
	vol->systemIDString[32] = '\0';
	strncpy(vol->systemIDString, buf, 32);
	buf += 32;
	dprintf("InitVolDesc - system id string is %s\n", vol->systemIDString);
	
	vol->volIDString[32] = '\0';
	strncpy(vol->volIDString, buf, 32);
	buf += (32 + 80-73 + 1);	// bytes 80-73 unused
	dprintf("InitVolDesc - volume id string is %s\n", vol->volIDString);
	
	vol->volSpaceSize[LSB_DATA] = *(uint32*)buf;
	buf += 4;
	vol->volSpaceSize[MSB_DATA] = *(uint32*)buf;
	buf+= (4 + 120-89 + 1); 		// bytes 120-89 unused
	
	vol->volSetSize[LSB_DATA] = *(uint16*)buf;
	buf += 2;
	vol->volSetSize[MSB_DATA] = *(uint16*)buf;
	buf += 2;
	
	vol->volSeqNum[LSB_DATA] = *(uint16*)buf;
	buf += 2;
	vol->volSeqNum[MSB_DATA] = *(uint16*)buf;
	buf += 2;
	
	vol->logicalBlkSize[LSB_DATA] = *(uint16*)buf;
	buf += 2;
	vol->logicalBlkSize[MSB_DATA] = *(uint16*)buf;
	buf += 2;
	
	vol->pathTblSize[LSB_DATA] = *(uint32*)buf;
	buf += 4;
	vol->pathTblSize[MSB_DATA] = *(uint32*)buf;
	buf += 4;
	
	vol->lPathTblLoc[LSB_DATA] = *(uint16*)buf;
	buf += 2;
	vol->lPathTblLoc[MSB_DATA] = *(uint16*)buf;
	buf += 2;
	
	vol->optLPathTblLoc[LSB_DATA] = *(uint16*)buf;
	buf += 2;
	vol->optLPathTblLoc[MSB_DATA] = *(uint16*)buf;
	buf += 2;
	
	vol->mPathTblLoc[LSB_DATA] = *(uint16*)buf;
	buf += 2;
	vol->mPathTblLoc[MSB_DATA] = *(uint16*)buf;
	buf += 2;
	
	vol->optMPathTblLoc[LSB_DATA] = *(uint16*)buf;
	buf += 2;
	vol->optMPathTblLoc[MSB_DATA] = *(uint16*)buf;
	buf += 2;
	
	dprintf( "Filling in the directory record...\n" );
	// Fill in directory record.
	node = &(vol->rootDirRec);
	node->fileIDString = calloc( ISO_MAX_FILENAME_LENGTH, 1 );
	node->attr.slName = calloc( ISO_MAX_FILENAME_LENGTH, 1);
	InitNode(_vol,&(vol->rootDirRec), buf, NULL);

	// error checking...FIXME!!!

	vol->rootDirRec.id = ISO_ROOTNODE_ID;
	//vol->rootDirRec.parID = ISO_ROOTNODE_ID;// Not sure about this ??? FIXME!!!
	buf += 34;
	
	vol->volSetIDString[128] = '\0';
	strncpy(vol->volSetIDString, buf, 128);
	buf += 128;
	dprintf("InitVolDesc - volume set id string is %s\n", vol->volSetIDString);
	
	vol->pubIDString[128] = '\0';
	strncpy(vol->pubIDString, buf, 128);
	buf +=128;
	dprintf("InitVolDesc - volume publisher id string is %s\n", vol->pubIDString);
	
	vol->dataPreparer[128] = '\0';
	strncpy(vol->dataPreparer, buf, 128);
	buf += 128;
	dprintf("InitVolDesc - volume dataPreparer string is %s\n", vol->dataPreparer);
	
	vol->appIDString[128] = '\0';
	strncpy(vol->appIDString, buf, 128);
	buf += 128;
	dprintf("InitVolDesc - volume application id string is %s\n", vol->appIDString);
	
	vol->copyright[38] = '\0';
	strncpy(vol->copyright, buf, 38);
	buf += 38;
	dprintf("InitVolDesc - copyright is %s\n", vol->copyright);
	
	vol->abstractFName[38] = '\0';
	strncpy(vol->abstractFName, buf, 38);
	buf += 38;
	dprintf("InitVolDesc - abstract file id string is %s\n", vol->abstractFName );
	
	vol->biblioFName[38] = '\0';
	strncpy(vol->biblioFName, buf, 38);
	buf += 38;
	dprintf("InitVolDesc - bibliographic file identifier: %s\n", vol->biblioFName );
	
	InitVolDate(&(vol->createDate), buf);
	buf += 17;
	
	InitVolDate(&(vol->modDate), buf);
	buf += 17;
	
	InitVolDate(&(vol->expireDate), buf);
	buf += 17;
	
	InitVolDate(&(vol->effectiveDate), buf);
	buf += 17;
	
	vol->fileStructVers = *(uint8*)buf;
	return 0;
}

int
InitNode(nspace * volume, vnode* rec, char* buf, int* bytesRead)
{
	int 	result = -EOK;
	uint8 	recLen = *(uint8*)buf++;
	bool    no_rock_ridge_stat_struct = true;
	
	if (bytesRead != NULL) 
		*bytesRead = recLen;

	dprintf("InitNode - ENTER, bufstart is %p, record length is %d bytes\n", buf, recLen);

	if (recLen > 0)
	{
		rec->extAttrRecLen = *(uint8*)(buf++);
		
		rec->startLBN[LSB_DATA] = *(uint32*)(buf);
		buf += 4;
		rec->startLBN[MSB_DATA] = *(uint32*)(buf);
		buf += 4;
		dprintf("InitNode - data start LBN is %ld\n", rec->startLBN[FS_DATA_FORMAT]);
		
		rec->dataLen[LSB_DATA] = *(uint32*)(buf);
		buf += 4;
		rec->dataLen[MSB_DATA] = *(uint32*)(buf);
		buf += 4;
		dprintf("InitNode - data length is %ld\n", rec->dataLen[FS_DATA_FORMAT]);
		
		InitRecDate(&(rec->recordDate), buf);
		buf += 7;
		
		rec->flags = *(uint8*) (buf );
		buf++;
		dprintf("InitNode - flags are %d\n", rec->flags);
		
		rec->fileUnitSize = *(uint8*)(buf);
		buf++;
		dprintf("InitNode - fileUnitSize is %d\n", rec->fileUnitSize);
		
		rec->interleaveGapSize = *(uint8*)(buf);
		buf ++;
		dprintf("InitNode - interleave gap size = %d\n", rec->interleaveGapSize);

		rec->volSeqNum = *(uint32*)(buf);
		buf += 4;
		dprintf("InitNode - volume seq num is %ld\n", rec->volSeqNum);
			
		rec->fileIDLen = *(uint8*)(buf);
		buf ++;
		dprintf("InitNode - file id length is %d\n", rec->fileIDLen);
		
		if (rec->fileIDLen > 0)
		{
			// Take care of "." and "..", the first two dirents are
			// these in iso.
			if (buf[0] == 0)
			{
				dprintf( "InitNode - dir: . \n" );
				strncpy( rec->fileIDString , ".", 2 );
				rec->fileIDLen = 1;
				rec->fileIDString[rec->fileIDLen] = '\0';
				
			}
			else if (buf[0] == 1)
			{
				dprintf( "InitNode - dir: .. \n" );

				strncpy( rec->fileIDString , "..", 3 );
				rec->fileIDLen = 2;
				rec->fileIDString[rec->fileIDLen] = '\0';
			}
			else
			{	

					memset( rec->fileIDString, '\0', ISO_MAX_FILENAME_LENGTH );
					strncpy(rec->fileIDString, buf, rec->fileIDLen );
					rec->fileIDString[rec->fileIDLen] = '\0';

			}
			
			//Get rid of semicolons, which are used to delineate file versions.q
			{
				char* semi = NULL;
				while ( (semi = strchr(rec->fileIDString, ';')) != NULL)
				{
					semi[0] = '\0';
				}
			
			}
			dprintf("DirRec ID String is: %s\n", rec->fileIDString);
			dprintf("DirRec ID String length is: %d\n", rec->fileIDLen );


			memset(&(rec->attr.stat), 0, 2*sizeof(struct stat));			
			// Set defaults, in case there is no RR stuff.
			rec->attr.stat[FS_DATA_FORMAT].st_mode = (S_IRUSR | S_IRGRP | S_IROTH);

			dprintf( "checking if we support rock ridge\n" );
			if ( volume->rockRidge == false )
			{
				dprintf( "ROCK RIDGE extensions DISABLED\n" );
			}
			else
			{
				dprintf( "ROCK RIDGE extensions ENABLED\n" );
			if (result == -EOK)
			{
				buf += rec->fileIDLen;
				if (!(rec->fileIDLen % 2)) 
					buf++;
	
				// Now we're at the start of the rock ridge stuff
				{
					char	altName[ISO_MAX_FILENAME_LENGTH];
					char	slName[ISO_MAX_FILENAME_LENGTH]; 
					uint16	altNameSize = 0; 
					uint16	slNameSize = 0; 
					uint8	slFlags = 0;
					uint8	length = 0;
					bool 	done = false;

					dprintf("RR: Start of extensions, but at %p\n", buf);
					
					while (!done)
					{
						buf+= length;
						length = *(uint8*)(buf + 2);
						switch (0x100 * buf[0] + buf[1])
						{
							// Posix attributes
							case 'PX':
							{
								uint8 bytePos = 3;
								dprintf("RR: found PX, length %u\n", length);
								rec->attr.pxVer = *(uint8*)(buf+bytePos++);
                                no_rock_ridge_stat_struct = false;
								
								// st_mode
								rec->attr.stat[LSB_DATA].st_mode = *(mode_t*)(buf + bytePos);
								bytePos += 4;
								rec->attr.stat[MSB_DATA].st_mode = *(mode_t*)(buf + bytePos);
								bytePos += 4;
								
								// st_nlink
								rec->attr.stat[LSB_DATA].st_nlink = *(nlink_t*)(buf+bytePos);
								bytePos += 4;
								rec->attr.stat[MSB_DATA].st_nlink = *(nlink_t*)(buf + bytePos);
								bytePos += 4;
								
								// st_uid
								rec->attr.stat[LSB_DATA].st_uid = *(uid_t*)(buf+bytePos);
								bytePos += 4;
								rec->attr.stat[MSB_DATA].st_uid = *(uid_t*)(buf+bytePos);
								bytePos += 4;
								
								// st_gid
								rec->attr.stat[LSB_DATA].st_gid = *(gid_t*)(buf+bytePos);
								bytePos += 4;
								rec->attr.stat[MSB_DATA].st_gid = *(gid_t*)(buf+bytePos);
								bytePos += 4;
							}	
							break;
							
							// ???
							case 'PN':
								dprintf("RR: found PN, length %u\n", length);
							break;
							
							// Symbolic link info
							case 'SL':
							{
								uint8	bytePos = 3;
								uint8	lastCompFlag = 0;
								uint8	addPos = 0;
								bool	slDone = false;
								bool	useSeparator = true;
								
								dprintf("RR: found SL, length %u\n", length);
								dprintf("Buffer is at %p\n", buf);
								dprintf("Current length is %u\n", slNameSize);
	
								rec->attr.slVer = *(uint8*)(buf + bytePos++);
								slFlags = *(uint8*)(buf + bytePos++);
								
								dprintf("sl flags are %u\n", slFlags);
								while (!slDone && bytePos < length)
								{
									uint8 compFlag = *(uint8*)(buf + bytePos++);
									uint8 compLen = *(uint8*)(buf + bytePos++);
									
									if (slName == NULL) useSeparator = false;
									
									addPos = slNameSize;
									
									dprintf("sl comp flags are %u, length is %u\n", compFlag, compLen);
									dprintf("Current name size is %u\n", slNameSize);
									switch (compFlag)
									{
										case SLCP_CONTINUE:
											useSeparator = false;
										default:
											// Add the component to the total path.
											slNameSize += compLen;
											if ( useSeparator )
												slNameSize++;											
											if (useSeparator) 
											{
												dprintf("Adding separator\n");
												slName[addPos++] = '/';
											}	
											
											dprintf("doing memcopy of %u bytes at offset %d\n", compLen, addPos );
											memcpy((slName + addPos), (buf + bytePos), compLen);
											
											addPos += compLen;
											useSeparator = true;
										break;
										
										case SLCP_CURRENT:
											dprintf("InitNode - found link to current directory\n");
											slNameSize += 2;
											memcpy(slName + addPos, "./", 2);
											useSeparator = false;
										break;
										
										case SLCP_PARENT:
											slNameSize += 3;
											memcpy(slName + addPos, "../", 3);
											useSeparator = false;
										break;
										
										case SLCP_ROOT:
											dprintf("InitNode - found link to root directory\n");
											slNameSize += 1;
											memcpy(slName + addPos, "/", 1);
											useSeparator = false;
										break;
										
										case SLCP_VOLROOT:
											slDone = true;
										break;
										
										case SLCP_HOST:
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
							dprintf("InitNode - symlink name is \'%s\'\n", slName);
							dprintf("InitNode - name length is %d\n", slNameSize );
							break;
							// Altername name
							case 'NM':
							{
								uint8	bytePos = 3;
								uint8	flags = 0;
								uint16	oldEnd = altNameSize;
								
								altNameSize += length - 5;
								dprintf("RR: found NM, length %u\n", length);
								// Read flag and version.
								rec->attr.nmVer = *(uint8*)(buf + bytePos++);
								flags = *(uint8*)(buf + bytePos++);
							
								dprintf("RR: nm buf is %s, start at %p\n", (buf + bytePos), buf+bytePos);
	
								// Build the file name.
								memcpy(altName + oldEnd, buf + bytePos, length - 5);
								altName[altNameSize] = '\0';
								dprintf("RR: alt name is %s\n", altName);
								
								// If the name is not continued in another record, update
								// the record name.
								if (! (flags & NM_CONTINUE))
								{
									// Get rid of the ISO name, replace with RR name.
//									if (rec->fileIDString != NULL) free(rec->fileIDString);
									memcpy( rec->fileIDString, altName, ISO_MAX_FILENAME_LENGTH );
									rec->fileIDLen = altNameSize;
								}
							}
							break;
							
							// Deep directory record masquerading as a file.
							case 'CL':
							
								dprintf("RR: found CL, length %u\n", length);
								rec->flags |= ISO_ISDIR;
								rec->startLBN[LSB_DATA] = *(uint32*)(buf+4);
								rec->startLBN[MSB_DATA] = *(uint32*)(buf+8);
								
								break;
							
							case 'PL':
								dprintf("RR: found PL, length %u\n", length);
								break;
							
							// Relocated directory, we should skip.
							case 'RE':
								result = -EINVAL;
								dprintf("RR: found RE, length %u\n", length);
								break;
							
							case 'TF':
								dprintf("RR: found TF, length %u\n", length);
								break;
							
							case 'RR':
								dprintf("RR: found RR, length %u\n", length);
								break;
							
							default:
								dprintf("RR: End of extensions.\n");
								done = true;
								break;
						}
					}
				}
			}
			} // rockRidge == true
		}
		else
		{
			dprintf("InitNode - File ID String is 0 length\n");
			result = -ENOENT;
		}
	}
	else result = -ENOENT;
	dprintf("InitNode - EXIT, result is %d name is \'%s\'\n", result /*strerror(result)*/,rec->fileIDString );
	
	
	if (no_rock_ridge_stat_struct) {
		if (rec->flags & ISO_ISDIR)
			rec->attr.stat[FS_DATA_FORMAT].st_mode |= (S_IFDIR|S_IXUSR|S_IXGRP|S_IXOTH);
		else
			rec->attr.stat[FS_DATA_FORMAT].st_mode |= (S_IFREG);
	}

	return result;
}

static int
InitVolDate(ISOVolDate* date, char* buf)
{
	memcpy(date, buf, 17);
	return 0;
}

static int
InitRecDate(ISORecDate* date, char* buf)
{
	memcpy(date, buf, 7 );
	return 0;
}

int
ConvertRecDate(ISORecDate* inDate, time_t* outDate)
{
	time_t	time;
	int		days, i, year, tz;
	
	year = inDate->year -70;
	tz = inDate->offsetGMT;
	
	if (year < 0)
	{
		time = 0;
	}
	else
	{
		int monlen[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
		days = (year * 365);
		if (year > 2)
		{
			days += (year + 1)/ 4;
		}
		for (i = 1; (i < inDate->month) && (i < 12); i++)
		{
			days += monlen[i-1];
		}
		if (((year + 2) % 4) == 0 && inDate->month > 2)
		{
			days++;
		}
		days += inDate->date - 1;
		time = ((((days*24) + inDate->hour) * 60 + inDate->minute) * 60)
					+ inDate->second;
		if (tz & 0x80)
		{
			tz |= (-1 << 8);	
		}
		if (-48 <= tz && tz <= 52)
			time += tz *15 * 60;
	}
	*outDate = time;
	return 0;
}








































