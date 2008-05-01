
/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */

#include <malloc.h>
#include <util/application.h>
#include <gui/bitmap.h>
#include <gui/window.h>
#include <gui/exceptions.h>

#include <atheos/areas.h>

using namespace os;

/**
 * Bitmap constructor.
 * \par Description:
 * This is the constructor used to create a standard bitmap.
 * \param nWidth - Width of the bitmap.
 * \param nHeight - Height of the bitmap.
 * \param eColorSpc - ColorSpace of the bitmap
 * \param nFlags - See description of the class.
 * \par Note:
 * An exception will be thrown if the bitmap could not be created. 	
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
Bitmap::Bitmap( int nWidth, int nHeight, color_space eColorSpc, uint32 nFlags )
{
	area_id hArea;

	m_cBounds = Rect( 0, 0, nWidth - 1, nHeight - 1 );

	m_hHandle = -1;
	m_pRaster = NULL;
	m_eColorSpace = eColorSpc;

	int nError = Application::GetInstance()->CreateBitmap( nWidth, nHeight, eColorSpc, nFlags, &m_hHandle, &hArea );

	if( nError < 0 )
	{
		throw( GeneralFailure( "Application server failed to create bitmap", -nError ) );
	}

	if( nFlags & SHARE_FRAMEBUFFER )
	{
		m_hArea = clone_area( "bm_clone", ( void ** )&m_pRaster, AREA_FULL_ACCESS, AREA_NO_LOCK, hArea );
	}
	else
	{
		m_hArea = -1;
	}
	if( nFlags & ACCEPT_VIEWS )
	{
		m_pcWindow = new Window( this );
		m_pcWindow->Unlock();
	}
	else
	{
		m_pcWindow = NULL;
	}
}

/**
 * Create a bitmap from a handle.
 * \par Description:
 * This constructor will create a handle from the appserver bitmap handle.
 * This is only useful in special cases, e.g. to share bitmaps between two
 * applications.
 * \param hHandle - The appserver handle of the bitmap.
 * \par Note:
 * An exception will be thrown if the bitmap could not be created. 	
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
Bitmap::Bitmap( int hHandle )
{
	area_id hArea;
	int nWidth;
	int nHeight;
	uint nFlags;
	color_space eColorSpc;
	int hNewHandle;
	
	int nError = Application::GetInstance()->CloneBitmap( hHandle, &hNewHandle, &hArea, &nFlags, &nWidth, &nHeight, &eColorSpc );

	if( nError < 0 )
	{
		throw( GeneralFailure( "Application server failed to create bitmap", -nError ) );
	}


	m_cBounds = Rect( 0, 0, nWidth - 1, nHeight - 1 );

	m_hHandle = hNewHandle;
	m_pRaster = NULL;
	m_eColorSpace = eColorSpc;
	m_pcWindow = NULL;

	
	if( nFlags & SHARE_FRAMEBUFFER )
	{
		m_hArea = clone_area( "bm_clone", ( void ** )&m_pRaster, AREA_FULL_ACCESS, AREA_NO_LOCK, hArea );
	}
	else
	{
		throw( GeneralFailure( "Tried to clone bitmap without SHARE_FRAMEBUFFER flags", -nError ) );
	}
	
//	printf("Remapped bitmap to %i\n", hNewHandle );
}

Bitmap::~Bitmap()
{
	if( m_hArea != -1 )
	{
		delete_area( m_hArea );
	}
	delete m_pcWindow;

	Application::GetInstance()->DeleteBitmap( m_hHandle );
}



/**
 * Returns the appserver handle
 * \par Description:
 * Returns the appserver handle. This method is only useful if you want to
 * share a bitmap with another application.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
int Bitmap::GetHandle() const
{
	return ( m_hHandle );
}

/**
 * Returns the colorspace of the bitmap.
 * \par Description:
 * Returns the colorspace of the bitmap.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
color_space Bitmap::GetColorSpace() const
{
	return ( m_eColorSpace );
}

/**
 * Adds a view to the bitmap.
 * \par Description:
 * This method adds a view to the bitmap. This is only valid if the bitmap
 * has been created with the ACCEPT_VIEWS flag.
 * \param pcView - The view.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
void Bitmap::AddChild( View * pcView )
{
	if( NULL != m_pcWindow )
	{
		m_pcWindow->AddChild( pcView );
	}
}

/**
 * Removes a view from the bitmap.
 * \par Description:
 * This method removes a view from the bitmap. This is only valid if the bitmap
 * has been created with the ACCEPT_VIEWS flag.
 * \param pcView - The view.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
bool Bitmap::RemoveChild( View * pcView )
{
	if( NULL != m_pcWindow )
	{
		m_pcWindow->RemoveChild( pcView );
		return ( true );
	}
	else
	{
		return ( false );
	}
}

/**
 * Returns the view with the given name.
 * \par Description:
 * This method returns the (previously added) view with the given name. 
 * This is only valid if the bitmap has been created with the ACCEPT_VIEWS flag.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
View *Bitmap::FindView( const char *pzName ) const
{
	if( NULL != m_pcWindow )
	{
		return ( m_pcWindow->FindView( pzName ) );
	}
	else
	{
		return ( NULL );
	}
}

/**
 * Flush rendering operations.
 * \par Description:
 * This method flushes all pending rendering commands of the views added to
 * this bitmap. This is only valid if the bitmap has been created with the ACCEPT_VIEWS flag.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
void Bitmap::Flush( void )
{
	if( NULL != m_pcWindow )
	{
		m_pcWindow->Flush();
	}
}

/**
 * Flush rendering operations and wait until they have been finished.
 * \par Description:
 * This method works like Flush() but waits until all operations have been
 * finished. This is only valid if the bitmap has been created with the ACCEPT_VIEWS flag.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
void Bitmap::Sync( void )
{
	if( NULL != m_pcWindow )
	{
		m_pcWindow->Sync();
	}
}

/**
 * Returns the bounds of the bitmap.
 * \par Description:
 * Returns the bounds of the bitmap.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
Rect Bitmap::GetBounds( void ) const
{
	return ( m_cBounds );
}

/**
 * Returns the numer of bytes in one bitmap row.
 * \par Description:
 * Returns the numer of bytes in one bitmap row.
 * \par Note:
 * This value might differ from (Width of the bitmap) * (Number of bytes per pixel)!
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
int Bitmap::GetBytesPerRow() const
{
	int nBitsPerPix = BitsPerPixel( m_eColorSpace );

	if( nBitsPerPix == 15 )
	{
		nBitsPerPix = 16;
	}
	return ( int ( m_cBounds.right - m_cBounds.left + 1.0f ) * nBitsPerPix / 8 );
}




const uint8 os::__5_to_8_bit_table[] = {
	0x00, 0x08, 0x10, 0x18, 0x20, 0x29, 0x31, 0x39,
	0x41, 0x4a, 0x52, 0x5a, 0x62, 0x6a, 0x73, 0x7b,
	0x83, 0x8b, 0x94, 0x9c, 0xa4, 0xac, 0xb4, 0xbd,
	0xc5, 0xcd, 0xd5, 0xde, 0xe6, 0xee, 0xf6, 0xff
};

const uint8 os::__6_to_8_bit_table[] = {
	0x00, 0x04, 0x08, 0x0c, 0x10, 0x14, 0x18, 0x1c,
	0x20, 0x24, 0x28, 0x2c, 0x30, 0x34, 0x38, 0x3c,
	0x40, 0x44, 0x48, 0x4c, 0x50, 0x55, 0x59, 0x5d,
	0x61, 0x65, 0x69, 0x6d, 0x71, 0x75, 0x79, 0x7d,
	0x81, 0x85, 0x89, 0x8d, 0x91, 0x95, 0x99, 0x9d,
	0xa1, 0xa5, 0xaa, 0xae, 0xb2, 0xb6, 0xba, 0xbe,
	0xc2, 0xc6, 0xca, 0xce, 0xd2, 0xd6, 0xda, 0xde,
	0xe2, 0xe6, 0xea, 0xee, 0xf2, 0xf6, 0xfa, 0xff
};
