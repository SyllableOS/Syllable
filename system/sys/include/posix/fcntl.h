/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __F_POSIX_FCNTL_H__
#define __F_POSIX_FCNTL_H__

#ifdef __cplusplus
extern "C"{
#endif

  
#ifdef __KERNEL__    
#include <atheos/types.h>
    
int	open(const char *_path, int _oflag, ...);
int	close( int nFile );
int	creat(const char *_path, mode_t _mode);
int	fcntl(int _fildes, int _cmd, ...);
#endif /* __KERNEL__  */

#define O_ACCMODE	  0003
#define O_RWMASK	  O_ACCMODE
#define O_RDONLY	    00
#define O_WRONLY	    01
#define O_RDWR		    02
#define O_CREAT		  0100	/* not fcntl */
#define O_EXCL		  0200	/* Dont allow O_CREAT to overwrite existing file	*/
#define O_NOCTTY	  0400	/* not fcntl */
#define O_TRUNC		 01000	/* not fcntl */
#define O_APPEND	 02000
#define O_NONBLOCK	 04000
#define O_NDELAY	O_NONBLOCK
#define O_SYNC		010000
#define O_FSYNC		O_SYNC
#define O_ASYNC		020000
#define FASYNC		O_ASYNC	/* fcntl, for BSD compatibility */
#define O_DIRECTORY	040000	/* Only open if it's a directory			*/
#define O_NOTRAVERSE   0100000	/* Don't resolv symlinks but open the link itself	*/
#define O_NOFOLLOW      O_NOTRAVERSE

#define F_DUPFD		0	/* dup						*/
#define F_GETFD		1	/* get f_flags					*/
#define F_SETFD		2	/* set f_flags					*/
#define F_GETFL		3	/* more flags (cloexec)				*/
#define F_SETFL		4	/* set file status flags			*/
#define F_GETLK		5	/* get record locking info			*/
#define F_SETLK		6	/* set record locking info (non-blocking)	*/
#define F_SETLKW	7	/* set record locking info (blocking)		*/

#define F_SETOWN	8	/* for sockets					*/
#define F_GETOWN	9	/* for sockets					*/

# define F_SETSIG	10	/* set signal number to be sent			*/
# define F_GETSIG	11	/* get signal number to be sent			*/

#define F_COPYFD	12	/* reopen a file and return an independent FD	*/
/* for F_[GET|SET]FL */
#define FD_CLOEXEC	1	/* If set the file will be closed during execve() */

/* for posix fcntl() and lockf() */
#define F_RDLCK		0
#define F_WRLCK		1
#define F_UNLCK		2

/* for old implementation of bsd flock () */
#define F_EXLCK		4	/* or 3 */
#define F_SHLCK		8	/* or 4 */

/* operations for bsd flock(), also used by the kernel implementation */
#define LOCK_SH		1	/* shared lock */
#define LOCK_EX		2	/* exclusive lock */
#define LOCK_NB		4	/* or'd with one of the above to prevent
				   blocking */
#define LOCK_UN		8	/* remove lock */

struct flock {
    short l_type;
    short l_whence;
    off_t l_start;
    off_t l_len;
    pid_t l_pid;
};

#ifdef __cplusplus
}
#endif

#endif /* __F_POSIX_FCNTL_H__ */
