/*
 *  Users and Passwords Manager for AtheOS
 *  Copyright (C) 2002 William Rose
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
#include <util/application.h>

#include "userswindow.h"
#include "main.h"

using namespace os;

int main( int argc, char **argv ) {
  Application *pcApp = new Application( "application/x-vnd.WDR-user_prefs" );
  UsersWindow *pcMain = new UsersWindow( Rect( 0, 0, 450 - 1, 400 - 1 ) +
                                         Point( 100, 50 ) );
  
  pcMain->Show();
  pcMain->MakeFocus( true );

  pcApp->Run();
}

#include <crypt.h>

bool compare_pwd_to_pwd( const char *pzCmp, const char *pzPwd ) {
  return strcmp( pzCmp, pzPwd );
}

bool compare_pwd_to_txt( const char *pzCmp, const char *pzTxt ) {
  return strcmp( pzCmp, crypt( pzTxt, "$1$" ) ) == 0;
}

#include <atheos/filesystem.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>
#include <png.h>

Bitmap *LoadPNG( const char *pzFile ) {
  FILE *fp;
  png_structp png_ptr = NULL;
  png_infop   info_ptr = NULL, end_info = NULL;
  Bitmap *pcBitmap = NULL;
  int fd, dir;
  
  if( (dir = open_image_file( IMGFILE_BIN_DIR )) < 0 ) {
    return NULL;
  }
  
  if( (fd = based_open( dir, pzFile, O_RDONLY )) < 0 ) {
    fp = fopen( pzFile, "r" );
  } else {
    fp = fdopen( fd, "r" );
  }

  close( dir );

  if( fp == NULL )
    goto error;

  // Check header looks valid
  uint8 acHeader[8];
  size_t nBytes;
  
  nBytes = fread( acHeader, 1, 8, fp );

  if( nBytes < 1 )
    goto error;

  if( png_sig_cmp( acHeader, 0, nBytes ) )
    goto error;
  
  // Allocate memory
  png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
  if( png_ptr == NULL )
    goto error;
  
  info_ptr = png_create_info_struct( png_ptr );
  if( info_ptr == NULL )
    goto error;

  end_info = png_create_info_struct( png_ptr );
  if( end_info == NULL )
    goto error;

  // Exception catcher
  if( setjmp( png_ptr->jmpbuf ) )
    goto error;
  
  // Rewind and read PNG
  rewind( fp );

  png_init_io( png_ptr, fp );

  png_read_info( png_ptr, info_ptr );

  uint32 nHeight, nWidth;
  int nDepth, nType;

  png_get_IHDR( png_ptr, info_ptr, &nWidth, &nHeight, &nDepth, &nType,
                NULL, NULL, NULL );
  
  pcBitmap = new Bitmap( nWidth, nHeight, CS_RGB32 );

  png_bytep pacBits;
  pacBits = static_cast<png_bytep>(pcBitmap->LockRaster());
  
  // Allocate the row pointers using alloca.
  png_bytep *apbRows;

  apbRows = reinterpret_cast<png_bytep *>
            (alloca( sizeof( png_bytep ) * nHeight ));

  for( int row = 0; row < nHeight; ++row )
    apbRows[ row ] = pacBits + row * (nWidth * 4);
  
  png_read_image( png_ptr, apbRows );

  int pixel;
  pixel = 0;
  for( int row = 0; row < nHeight; ++row ) {
    for( int col = 0; col < nWidth; ++col, pixel += 4 ) {
      pacBits[ pixel + 2 ] = pacBits[ pixel + 2 ] ^ pacBits[ pixel ];
      pacBits[ pixel     ] = pacBits[ pixel + 2 ] ^ pacBits[ pixel ];
      pacBits[ pixel + 2 ] = pacBits[ pixel + 2 ] ^ pacBits[ pixel ];
    }
  }
      
error:
  if( pcBitmap )
    pcBitmap->UnlockRaster();
  
  if( png_ptr ) 
    png_destroy_read_struct( &png_ptr,
                             (info_ptr ? &info_ptr : NULL),
                             (end_info ? &end_info : NULL) );
  
  if( fp )
    fclose( fp );
  
  return pcBitmap;
}

#include <gui/requesters.h>

void errno_alert( const char *pzTitle, const char *pzAction ) {
  char acMsg[256];
  
  snprintf( acMsg, 256, "An error occurred while %s (%d: %s).",
            pzAction, errno, strerror( errno ) );
  
  Alert *pcAlert = new Alert( pzTitle, acMsg, 0, "OK", NULL );

  pcAlert->Go();
}
