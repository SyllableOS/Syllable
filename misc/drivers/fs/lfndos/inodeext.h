#ifndef	_INODEEXT_H_
#define	_INODEEXT_H_

#include <atheos/types.h>

#define	LFD_OLD_INODE_FILE_NAME	"ALTINTAB.AFE"
#define	LFD_INODE_FILE_NAME	"DOS_FAT.INO"

#define LFD_INODE_WARNING	"WARNING: Don't delete this file! It contains important information used by the AtheOS FAT filesystem\n"
#define LFD_MAX_WARNING_SIZE	128

#define LFD_SYMLINK_MARKER	"L:F:DS@Y@M@L@N@K"
#define LFD_SYMLINK_MARKER_SIZE 16

typedef	struct	OldFileNodeInfo	OldFileNodeInfo_s;
typedef	struct	FileNodeInfo	FileNodeInfo_s;

#define	LFD_MAX_NAME_LEN	64

#define DOS_S_IFMT   070000
#define DOS_S_IFREG  000000
#define DOS_S_IFDIR  030000
#define DOS_S_IFLNK  050000

#define DOS_S_ISUID  004000
#define DOS_S_ISGID  002000

#define DOS_S_ISLNK(m)	(((m) & DOS_S_IFMT) == DOS_S_IFLNK)
#define DOS_S_ISREG(m)	(((m) & DOS_S_IFMT) == DOS_S_IFREG)
#define DOS_S_ISDIR(m)	(((m) & DOS_S_IFMT) == DOS_S_IFDIR)


struct	OldFileNodeInfo
{
  char		zShortName[16];
  char		zLongName[LFD_MAX_NAME_LEN];

  int		nSize;
  int		nMode;
  time_t	nMTime;
  time_t	nATime;
  time_t	nCTime;
};

#define LFD_CUR_INODE_FILE_VERSION	1

typedef	struct
{
  char	zWarning[LFD_MAX_WARNING_SIZE];
  int	nVersion;
  int	nNumEntries;
} FileInfoHeader_s;

struct	FileNodeInfo
{
  int		nSize;
  int		nMode;
  int		nUID;
  int		nGID;
  time_t	nMTime;
  time_t	nATime;
  time_t	nCTime;
  char		zShortName[16];
  char		zLongName[LFD_MAX_NAME_LEN];
};

#endif	_INODEEXT_H_
