
/*
 *  The AtheOS application server
 *  Copyright (C) 1999  Kurt Skauen
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

#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <cstdlib>
#include "ddriver.h"
#include "bitmap.h"
#include "layer.h"
#include "server.h"
#include <ctime>

extern "C"
{
#include <png.h>
}

/*static variables, so that you can keep track of the number of screenshots per minute*/
static int nScreenShotNum = -1;
static long nMinCounter;

#if 1
static void write_png( const char *file_name, SrvBitmap * pcBitmap )
{
	FILE *fp;
	png_structp png_ptr;
	png_infop info_ptr;

//   png_color_8 sig_bit;

	/* open the file */
	fp = fopen( file_name, "wb" );
	if( fp == NULL )
		return;

	/* Create and initialize the png_struct with the desired error handler
	 * functions.  If you want to use the default stderr and longjump method,
	 * you can supply NULL for the last three parameters.  We also check that
	 * the library version is compatible with the one used at compile time,
	 * in case we are using dynamically linked libraries.  REQUIRED.
	 */
	png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
//      png_voidp user_error_ptr, user_error_fn, user_warning_fn);

	if( png_ptr == NULL )
	{
		fclose( fp );
		return;
	}

	/* Allocate/initialize the image information data.  REQUIRED */
	info_ptr = png_create_info_struct( png_ptr );
	if( info_ptr == NULL )
	{
		fclose( fp );
		png_destroy_write_struct( &png_ptr, ( png_infopp ) NULL );
		return;
	}

	/* Set error handling.  REQUIRED if you aren't supplying your own
	 * error hadnling functions in the png_create_write_struct() call.
	 */
	if( setjmp( png_ptr->jmpbuf ) )
	{
		/* If we get here, we had a problem reading the file */
		fclose( fp );
		png_destroy_write_struct( &png_ptr, ( png_infopp ) NULL );
		return;
	}

	png_init_io( png_ptr, fp );

	/* Set the image information here.  Width and height are up to 2^31,
	 * bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
	 * the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
	 * PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
	 * or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
	 * PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
	 * currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED
	 */
	printf( "Init info header\n" );
	png_set_IHDR( png_ptr, info_ptr, pcBitmap->m_nWidth, pcBitmap->m_nHeight, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE );

	/* set the palette if there is one.  REQUIRED for indexed-color images */
//   palette = (png_colorp)png_malloc(png_ptr, 256 * sizeof (png_color));
	/* ... set palette colors ... */
//   png_set_PLTE(png_ptr, info_ptr, palette, 256);

#if 0
	/* optional significant bit chunk */
	/* if we are dealing with a grayscale image then */
	sig_bit.gray = 0;
	/* otherwise, if we are dealing with a color image then */
	sig_bit.red = 8;
	sig_bit.green = 8;
	sig_bit.blue = 8;
	/* if the image has an alpha channel then */
	sig_bit.alpha = 0;
	png_set_sBIT( png_ptr, info_ptr, &sig_bit );
#endif


	/* Optional gamma chunk is strongly suggested if you have any guess
	 * as to the correct gamma of the image.
	 */
//   png_set_gAMA(png_ptr, info_ptr, gamma);

	/* Optionally write comments into the image */

/*   
     text_ptr[0].key = "Title";
     text_ptr[0].text = "Mona Lisa";
     text_ptr[0].compression = PNG_TEXT_COMPRESSION_NONE;
     text_ptr[1].key = "Author";
     text_ptr[1].text = "Leonardo DaVinci";
     text_ptr[1].compression = PNG_TEXT_COMPRESSION_NONE;
     text_ptr[2].key = "Description";
     text_ptr[2].text = "<long text>";
     text_ptr[2].compression = PNG_TEXT_COMPRESSION_zTXt;
     png_set_text(png_ptr, info_ptr, text_ptr, 3);
     */

	/* other optional chunks like cHRM, bKGD, tRNS, tIME, oFFs, pHYs, */
	/* note that if sRGB is present the cHRM chunk must be ignored
	 * on read and must be written in accordance with the sRGB profile */

	/* Write the file header information.  REQUIRED */
	printf( "Write info header\n" );
	png_write_info( png_ptr, info_ptr );

	/* Once we write out the header, the compression type on the text
	 * chunks gets changed to PNG_TEXT_COMPRESSION_NONE_WR or
	 * PNG_TEXT_COMPRESSION_zTXt_WR, so it doesn't get written out again
	 * at the end.
	 */

	/* set up the transformations you want.  Note that these are
	 * all optional.  Only call them if you want them.
	 */

	/* invert monocrome pixels */
//   png_set_invert_mono(png_ptr);

	/* Shift the pixels up to a legal bit depth and fill in
	 * as appropriate to correctly scale the image.
	 */
//   png_set_shift(png_ptr, &sig_bit);

	/* pack pixels into bytes */
//   png_set_packing(png_ptr);

	/* swap location of alpha bytes from ARGB to RGBA */
//   png_set_swap_alpha(png_ptr);

	/* Get rid of filler (OR ALPHA) bytes, pack XRGB/RGBX/ARGB/RGBA into
	 * RGB (4 channels -> 3 channels). The second parameter is not used.
	 */
//   png_set_filler(png_ptr, 0, PNG_FILLER_BEFORE);

	/* flip BGR pixels to RGB */
//   png_set_bgr(png_ptr);

	/* swap bytes of 16-bit files to most significant byte first */
//   png_set_swap(png_ptr);

	/* swap bits of 1, 2, 4 bit packed pixel formats */
//   png_set_packswap(png_ptr);

	/* turn on interlace handling if you are not using png_write_image() */
//   if (interlacing)
//      number_passes = png_set_interlace_handling(png_ptr);
//   else
//      number_passes = 1;

	/* The easiest way to write the image (you may have a different memory
	 * layout, however, so choose what fits your needs best).  You need to
	 * use the first method if you aren't handling interlacing yourself.
	 */
	uint8 *pRaster = ( uint8 * )pcBitmap->m_pRaster;
	uint8 *pRaster8 = ( uint8 * )pRaster;
	uint16 *pRaster16 = ( uint16 * )pRaster;
	uint32 *pRaster32 = ( uint32 * )pRaster;

	os::color_space eColorSpace = pcBitmap->m_eColorSpc;
	switch ( eColorSpace )
	{
	case CS_RGB32:
	case CS_RGBA32:
		eColorSpace = CS_RGB32;
		break;
	case CS_RGB24:
		break;
	default:
	case CS_RGB16:
		break;
	case CS_RGB15:
	case CS_RGBA15:
		eColorSpace = CS_RGB15;
		break;
	}
	png_byte *pRowBuffer = new png_byte[pcBitmap->m_nWidth * 3];

	for( int y = 0; y < pcBitmap->m_nHeight; ++y )
	{
		int x;
		png_byte *pRowPtr = pRowBuffer;

		switch ( eColorSpace )
		{
		case CS_RGB32:
			for( x = 0; x < pcBitmap->m_nWidth; ++x )
			{
				Color32_s sColor = RGB32_TO_COL( *pRaster32++ );

				*pRowPtr++ = sColor.red;
				*pRowPtr++ = sColor.green;
				*pRowPtr++ = sColor.blue;
			}
			break;
		case CS_RGB24:
			for( x = 0; x < pcBitmap->m_nWidth; ++x )
			{
				*pRowPtr++ = *pRaster8++;
				*pRowPtr++ = *pRaster8++;
				*pRowPtr++ = *pRaster8++;
			}
			break;
		case CS_RGB16:
			for( x = 0; x < pcBitmap->m_nWidth; ++x )
			{
				Color32_s sColor = RGB16_TO_COL( *pRaster16++ );

				*pRowPtr++ = sColor.red;
				*pRowPtr++ = sColor.green;
				*pRowPtr++ = sColor.blue;
			}
			break;
		case CS_RGB15:
			for( x = 0; x < pcBitmap->m_nWidth; ++x )
			{
				Color32_s sColor = RGB15_TO_COL( *pRaster16++ );

				*pRowPtr++ = sColor.red;
				*pRowPtr++ = sColor.green;
				*pRowPtr++ = sColor.blue;
			}
			break;
		default:
			break;
		}
		png_write_rows( png_ptr, &pRowBuffer, 1 );
		pRaster += pcBitmap->m_nBytesPerLine;
		pRaster8 = ( uint8 * )pRaster;
		pRaster16 = ( uint16 * )pRaster;
		pRaster32 = ( uint32 * )pRaster;
	}

#if 0
	png_uint_32 k, height, width;
	png_byte image[height][width];
	png_bytep row_pointers[height];

	for( k = 0; k < height; k++ )
		row_pointers[k] = image + k * width;

	/* One of the following output methods is REQUIRED */
#ifdef entire			/* write out the entire image data in one call */
	png_write_image( png_ptr, row_pointers );

	/* the other way to write the image - deal with interlacing */

#else	/* no_entire */	/* write out the image data by one or more scanlines */
	/* The number of passes is either 1 for non-interlaced images,
	 * or 7 for interlaced images.
	 */
	for( pass = 0; pass < number_passes; pass++ )
	{
		/* Write a few rows at a time. */
		png_write_rows( png_ptr, &row_pointers[first_row], number_of_rows );

		/* If you are only writing one row at a time, this works */
		for( y = 0; y < height; y++ )
		{
			png_write_rows( png_ptr, &row_pointers[y], 1 );
		}
	}
#endif	/* no_entire */	 /* use only one output method */
#endif
	/* You can write optional chunks like tEXt, zTXt, and tIME at the end
	 * as well.
	 */

	/* It is REQUIRED to call this to finish writing the rest of the file */
	png_write_end( png_ptr, info_ptr );

	/* if you malloced the palette, free it here */
	free( info_ptr->palette );

	/* if you allocated any text comments, free them here */

	/* clean up after the write, and free any memory allocated */
	png_destroy_write_struct( &png_ptr, ( png_infopp ) NULL );

	/* close the file */
	fclose( fp );

	/* that's it */
	return;
}

#endif
void ScreenShot()
{
#if 1

	char zTemp[1024];	//this variable holds the name of the screenhot

	g_cLayerGate.Close();
	if( g_pcTopView != NULL && g_pcTopView->GetBitmap() != NULL )
	{

		/*This is probably the best way to trap time. I saw it in Lancher's Clock code and I think it is a good way to do it */
		long nCurSysTime = get_real_time() / 1000000;
		struct tm *psTime = gmtime( &nCurSysTime );
		long anTimes[] = {
			psTime->tm_mday,
			psTime->tm_hour,
			( psTime->tm_hour > 12 ) ? psTime->tm_hour - 12 : psTime->tm_hour,
			psTime->tm_mon + 1,
			psTime->tm_min,
			psTime->tm_sec,
			psTime->tm_year - 100
		};

		/*if the minute counter is not the same as the current time we must reset the screenshot number */
		if( nMinCounter != anTimes[4] )
			nScreenShotNum = -1;

		/*make sure that the minute variable equals the current min.  In theory this could cause a little problem, but very unlikely */
		nMinCounter = anTimes[4];

		/*make sure you increment the screenshot counter */
		nScreenShotNum++;

		/*must create the filename */
		sprintf( zTemp, "/tmp/Screenshot-200%ld.%ld.%ld-%ld:%ld.%d.png", anTimes[6], anTimes[3], anTimes[0], anTimes[2], anTimes[4], nScreenShotNum );

		/*write the png already */
		write_png( zTemp, g_pcTopView->GetBitmap() );

	}
	g_cLayerGate.Open();
#endif
}
