#ifndef __ATHEOS_POSIX_UNISTD_H_
#define __ATHEOS_POSIX_UNISTD_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ASSEMBLER	
#include <atheos/types.h> /* NOT POSIX but you can't include just unistd.h without it */
#endif
	
#include <atheos/syscall.h>

#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2

#ifndef NULL	
#define NULL 0
#endif
	
#define R_OK	0x04
#define W_OK	0x02
#define X_OK	0x01
#define F_OK	0x00

  
#define STDIN_FILENO		0
#define STDOUT_FILENO		1
#define STDERR_FILENO		2

#define _SC_OPEN_MAX 4

#ifndef ASSEMBLER	
void		__exit(int _status) __attribute__((noreturn));
void		_exit(int _status) __attribute__((noreturn));
int		access(const char *_path, int _amode);
#ifndef alarm
unsigned int	alarm(unsigned int _seconds);
#endif

int		chdir(const char *_path);
int		chown(const char *_path, uid_t _owner, gid_t _group);
int		close(int _fildes);
char*		ctermid(char *_s);
int		dup(int _fildes);
int		dup2(int _fildes, int _fildes2);
int		execl(const char *_path, const char *_arg, ...);
int		execle(const char *_path, const char *_arg, ...);
int		execlp(const char *_file, const char *_arg, ...);
int		execv(const char *_path, char *const _argv[]);
int		execve(const char *_path, char *const _argv[], char *const _envp[]);
int		execvp(const char *_file, char *const _argv[]);
pid_t		fork(void);
long		fpathconf(int _fildes, int _name);
char*		getcwd(char *_buf, size_t _size);
gid_t		getegid(void);
uid_t		geteuid(void);
gid_t		getgid(void);
int		getgroups(int _gidsetsize, gid_t *_grouplist);
char*		getlogin(void);
pid_t		getpgrp(void);
pid_t		getpgid( pid_t hPid );
pid_t		getpid(void);
pid_t		getppid(void);
uid_t		getuid(void);
int		isatty(int _fildes);
int		link(const char *_existing, const char *_new);
off_t		lseek(int _fildes, off_t _offset, int _whence);
long		pathconf(const char *_path, int _name);
int		pause(void);
int		pipe(int _fildes[2]);
int		rmdir(const char *_path);
int		setgid(gid_t _gid);
int		setpgid(pid_t _pid, pid_t _pgid);
pid_t		setsid(void);
int		setuid(uid_t uid);
unsigned int	sleep(unsigned int _seconds);
long		sysconf(int _name);
pid_t		tcgetpgrp(int _fildes);
int		tcsetpgrp(int _fildes, pid_t _pgrp_id);
char*		ttyname(int _fildes);
int		unlink(const char *_path);

ssize_t	read( int nFile, void* pBuffer, size_t nLength );
ssize_t	write( int nFile, const void* pBuffer, size_t nLength );


/* additional access() checks */
/*#define D_OK	0x10*/

int		brk(void *_heaptop);
int		__file_exists(const char *_fn);
int		fsync(int _fd);
int		ftruncate(int, off_t);
int		getdtablesize(void);
int		gethostname(char *buf, size_t size);
int		getpagesize(void);
char*		getwd(char *__buffer);
int		nice(int _increment);
void*		sbrk(int _delta);
int		symlink (const char *, const char *);
int		sync(void);
int		truncate(const char*, off_t);
unsigned int	usleep(unsigned int _useconds);
pid_t		vfork(void);
int 		readlink( const char* pzPath, char* pzBuffer, size_t nBufSize );



#endif	/** #ifndef ASSEMBLER **/

#ifdef __cplusplus
}
#endif

#endif /* __ATHEOS_POSIX_UNISTD_H_ */
