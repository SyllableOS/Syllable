#ifndef __ATHEOS_DOS_H__
#define __ATHEOS_DOS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <posix/dirent.h>


typedef void* DOS_DIR;

typedef struct	fileinfo
{
  char	srcattribs1	__attribute__((packed));
  char	drive		__attribute__((packed));
  char	srcname[11]	__attribute__((packed));
  char	srcattribs2	__attribute__((packed));
  short	direntrynum	__attribute__((packed));
  short	firstcluster1	__attribute__((packed));
  char	a		__attribute__((packed));
  short	firstcluster2	__attribute__((packed));
  char	fileattribs	__attribute__((packed));
  short	time		__attribute__((packed));
  short	date		__attribute__((packed));
  long	size		__attribute__((packed));
  char	name[14]	__attribute__((packed));
} DosFileInfo_s;

int dos_move_dir( char* pzOldPath, char* pzNewPath );
int dos_rename( const char *pzOldName, const char* pzNewName );
int dos_create( const char *path );
int dos_open( const char *path, int access, ... );
int dos_close( int fn );
int	dos_setftime( int nFile, time_t nTime );
int dos_read( int fn, void *buffer, unsigned size );
int dos_write( int fn, const void *buffer, unsigned size );
long	dos_lseek( int fn, long disp, int mode );
int dos_stat( const char *pth, struct stat *buf );
int dos_find_first( const char* pth, DosFileInfo_s* psBuffer );
DOS_DIR	*dos_opendir( const char *pth );
int	dos_closedir( DOS_DIR *dir );
struct kernel_dirent *dos_readdir( DOS_DIR *pDir );
int	dos_mkdir( const char *path, int ehhh );
int	dos_rmdir( const char *path );
int	dos_unlink( const char *path );


#ifdef __cplusplus
}
#endif

#endif	/* __ATHEOS_DOS_H__ */
