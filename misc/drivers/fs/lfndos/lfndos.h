#ifndef	_LFNDOS_H_
#define	_LFNDOS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "inodeext.h"

//#define LFD_CACHE_SIZE (1024*1024*2)
#define	CACHE_BLOCK_SIZE 32768
/*#define	NUM_BUFFERS	 512*/
//#define	NUM_BUFFERS	((LFD_CACHE_SIZE) / (CACHE_BLOCK_SIZE))
#define	LFD_NUM_DOS_HANDLES	11
#define	LFD_MAX_DOS_PATH	300

typedef struct	FileEntry   FileEntry_s;
typedef	struct	CacheBuffer CacheBuffer_s;
typedef	struct  FileNode    FileNode_s;


struct	CacheBuffer
{
  CacheBuffer_s* cb_psNext;
  CacheBuffer_s* cb_psPrev;
  FileEntry_s*	 cb_psFileEntry;
  int		 cb_nOffset;
  int		 cb_nBytesUsed;
  bool		 cb_bDirty;
  int		 cb_nMagic1;
  uint8		 cb_aData[ CACHE_BLOCK_SIZE ];
  int		 cb_nMagic2;
};

#define	FE_DIRTY	    0x0001 /* File table has changed	*/
#define	FE_FILE_INFO_LOADED 0x0002 /* File info is loaded from disk	*/
#define	FE_FILE_SIZE_VALID  0x0004
#define	FE_FILE_EXISTS	    0x0008 /* File is physically created on the underlying FS */
#define FE_INODE_LOADED	    0x0010 /* Inode is loaded by the kernel */

struct	FileEntry
{
  FileEntry_s*	 fe_psNext;
  FileEntry_s*	 fe_psParent;

  union {
    size_t	 nSize;
    FileEntry_s* psFirstChild;
  } fe_u;
  int		 fe_nFlags;

  int		 fe_nMode;
  uid_t		 fe_nUID;
  gid_t	 	 fe_nGID;
  time_t	 fe_nMTime;
  time_t	 fe_nATime;
  time_t	 fe_nCTime;
  char		 fe_zShortName[16];
  char		 fe_zLongName[LFD_MAX_NAME_LEN];
};

typedef struct
{
  int	nFlags;
} LfdCooki_s;


struct FileNode
{
  FileNode_s*	fn_psNext;		/*	List of all cached file entries	*/
  FileEntry_s*	fn_psFileEntry;
  int		fn_nInoNum;
  int		fn_nRefCount;
  int		fn_nLinkCount;
};




int		lfd_LockFileEntries( void );
void		lfd_UnlockFileEntries( void );
bool 		lfd_FileExists( const char* pzPath );
int		lfd_AssertValidBuffer( CacheBuffer_s* psBuffer );
void		lfd_RemoveBuffer( CacheBuffer_s* psBuffer );
void		lfd_AppendBuffer( CacheBuffer_s* psBuffer );
//CacheBuffer_s*	lfd_FindBuffer( FileEntry_s* psEntry, int nOffset );
void		lfd_FlushNodeBuffers( FileEntry_s* psEntry, bool bRelease, bool bDelete );
//int		lfd_FlushAllBuffers( void );
bool 		lfd_flush_some_buffers();
int		lfd_ConstructFileSize( FileEntry_s* psEntry );
int		lfd_FlushBuffer( CacheBuffer_s* psBuffer );
//int		lfd_ReadBuffer( CacheBuffer_s* psBuffer );
void		lfd_TouchBuffer( CacheBuffer_s* psBuffer );
CacheBuffer_s*	lfd_AllocBuffer( FileEntry_s* psEntry );
int		lfd_GetFullPath( FileEntry_s* psNode, char* pzBuffer );
int		lfd_ReadDirectoryInfo( FileEntry_s* psParent );
int		lfd_LookupFile( FileEntry_s* psParent, const char* pzName, int nNameLen, FileEntry_s** ppsRes );
//void		lfd_ReleaseFile( FileEntry_s* psEntry );
int		lfd_CachedRead( FileEntry_s* psEntry, void* pBuffer, off_t nPos, size_t nLen );
int		lfd_CachedWrite(  FileEntry_s* psEntry, const void* pBuffer, off_t nPos, size_t nLen  );
void		lfd_InitCache( int nCacheSize );

//int		lfd_GetEntryFileHandle( FileEntry_s* psEntry );
void		lfd_close_dos_handle( FileEntry_s* psEntry );
int		lfd_create_file_entry( FileEntry_s* psParent, const char* pzName, int nNameLen, int nMode, FileEntry_s** ppsRes );
//void		lfd_UnlinkFileEntry( FileEntry_s* psNode );
int		lfd_DeleteFileEntry( FileEntry_s* psEntry );
int		lfd_TruncFile( FileEntry_s* psEntry );


int	lfd_CreateShortName( FileEntry_s* psParent, const char *name, int len, char *pzDst );
int	lfd_write_directory_info( FileEntry_s* psParent );
int	lfd_rename_entry( FileEntry_s* psOldEntry, FileEntry_s* psDestDir, const char* pzNewName, int nNewNameLen );

#ifdef __cplusplus
}
#endif

#endif
