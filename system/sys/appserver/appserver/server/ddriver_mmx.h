/*
 *  The Syllable application server
 *  MMX accelerated colorspace conversion
 *  Copyright (C) MPlayer ( www.mplayerhq.hu )
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
 
#ifndef	INTERFACE_DDRIVER_MMX_HPP
#define	INTERFACE_DDRIVER_MMX_HPP

inline void mmx_rgb32_to_rgb16( uint8* pSrc, uint8* pDst, unsigned nPixels );

#endif	//	INTERFACE_DDRIVER_MMX_HPP



