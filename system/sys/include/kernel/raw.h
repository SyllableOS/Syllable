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

#ifndef __F_KERNEL_RAW_H__
#define __F_KERNEL_RAW_H__

struct _RawEndPoint {
	RawEndPoint_s *re_psNext;
	RawPort_s *re_psPort;

	ipaddr_t re_anRemoteAddr;
	ipaddr_t re_anLocalAddr;

	bool re_bNonBlocking;
	bool re_bRoute;
	int re_nCreateHeader;

	NetQueue_s re_sPackets;

	int re_nProtocol;

	SelectRequest_s *re_psFirstReadSelReq;
	SelectRequest_s *re_psFirstWriteSelReq;
	SelectRequest_s *re_psFirstExceptSelReq;
};

struct _RawPort {
	RawPort_s *rp_psNext;
	RawEndPoint_s *rp_psFirstEndPoint;
	int rp_nProtocol;
};

int init_raw(void);

int raw_open(Socket_s * psSocket);

#endif	/* __F_KERNEL_RAW_H__ */
