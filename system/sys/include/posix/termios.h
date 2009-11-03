/*
 *  The Syllable kernel
 *  Copyright (C) 2009 Kristian Van Der Vliet
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

#ifndef __F_POSIX_TERMIOS_H_
#define __F_POSIX_TERMIOS_H_

#include <posix/termbits.h>
#include <posix/ioctls.h>

#ifdef __cplusplus
extern "C" {
#endif

struct winsize {
	unsigned short ws_row;
	unsigned short ws_col;
	unsigned short ws_xpixel;
	unsigned short ws_ypixel;
};

#define NCC 8
struct termio {
	unsigned short c_iflag;		/* input mode flags */
	unsigned short c_oflag;		/* output mode flags */
	unsigned short c_cflag;		/* control mode flags */
	unsigned short c_lflag;		/* local mode flags */
	unsigned char c_line;		/* line discipline */
	unsigned char c_cc[NCC];	/* control characters */
};

/* modem lines */
#define TIOCM_LE	0x001
#define TIOCM_DTR	0x002
#define TIOCM_RTS	0x004
#define TIOCM_ST	0x008
#define TIOCM_SR	0x010
#define TIOCM_CTS	0x020
#define TIOCM_CAR	0x040
#define TIOCM_RNG	0x080
#define TIOCM_DSR	0x100
#define TIOCM_CD	TIOCM_CAR
#define TIOCM_RI	TIOCM_RNG

/* ioctl (fd, TIOCSERGETLSR, &result) where result may be as below */

/* line disciplines */
#define N_TTY		0
#define N_SLIP		1
#define N_MOUSE		2
#define N_PPP		3
#define N_STRIP		4

speed_t	cfgetispeed(const struct termios *_termios_p);
speed_t	cfgetospeed(const struct termios *_termios_p);
int	cfsetispeed(struct termios *_termios_p, speed_t _speed);
int	cfsetospeed(struct termios *_termios_p, speed_t _speed);
int	tcdrain(int _fildes);
int	tcflow(int _fildes, int _action);
int	tcflush(int _fildes, int _queue_selector);
int	tcgetattr(int _fildes, struct termios *_termios_p);
int	tcsendbreak(int _fildes, int _duration);
int	tcsetattr(int _fildes, int _optional_actions, const struct termios *_termios_p);

#ifdef __cplusplus
}
#endif

#endif	/* __F_POSIX_TERMIOS_H_ */
