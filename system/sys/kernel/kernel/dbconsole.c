
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

#include <atheos/kernel.h>
#include <atheos/kdebug.h>

static int g_nCsrX = 0;
static int g_nCsrY = 0;
static int g_nWidth = 80;
static int g_nHeight = 25;
static char *g_pFrameBuffer = ( char * )0xb8000;


static void dbcon_scroll( void )
{
	int i;

	memmove( g_pFrameBuffer, g_pFrameBuffer + g_nWidth * 2, g_nWidth * g_nHeight * 2 - g_nWidth * 2 );
	for ( i = 0; i < g_nWidth; ++i )
	{
		g_pFrameBuffer[g_nWidth * ( g_nHeight - 1 ) * 2 + i * 2] = 0x20;
	}
	g_nCsrY--;
}

void dbcon_set_color( int Color, int R, int G, int B )
{
	isa_writeb( 0x3c8, Color );
	isa_writeb( 0x3c9, R >> 2 );
	isa_writeb( 0x3c9, G >> 2 );
	isa_writeb( 0x3c9, B >> 2 );
}

void dbcon_write( const char *pData, int nSize )
{
	int i;

	for ( i = 0; i < nSize; ++i )
	{
		if ( pData[i] == '\n' )
		{
			g_nCsrY++;
			g_nCsrX = 0;
			if ( g_nCsrY >= g_nHeight )
			{
				dbcon_scroll();
			}
			continue;
		}
		if ( pData[i] == '\r' )
		{
			continue;
		}
		if ( g_nCsrX >= g_nWidth )
		{
			g_nCsrX = 0;
			g_nCsrY++;
			if ( g_nCsrY >= g_nHeight )
			{
				dbcon_scroll();
			}
		}
		g_pFrameBuffer[( g_nCsrY * g_nWidth + g_nCsrX ) * 2] = pData[i];
		g_nCsrX++;
	}
}

void dbcon_clear( void )
{
	int i;

	for ( i = 0; i < g_nWidth * g_nHeight; ++i )
	{
		( ( uint16 * )g_pFrameBuffer )[i] = 0x1720;
	}
}
