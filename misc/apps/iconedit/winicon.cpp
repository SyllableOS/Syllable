#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <atheos/types.h>
#include <atheos/kdebug.h>

#include <gui/bitmap.h>
#include <gui/exceptions.h>

using namespace os;

struct WinIconImageDirEntry {
  uint8 nWidth;         /* width in pixels. */
  uint8 nHeight;        /* height in pixels. */
  uint8 nColorCount;    /* number of colors. */
  uint8 nReserved;      /* must be 0. */
  union {
    int16 wPlanes;    /* color planes. */
    int16 xHotSpot;   /* x-coodinate of the hotspot. */
  };
  union {
    int16 wBitCount;  /* bits per pixel. */
    int16 yHotSpot;   /* y-coodinate of the hotspot. */
  };
  int32 nBytesInRes;  /* size of the image data. */
  int32 nImageOffset; /* offset to the image data. */
};



struct WinIconImageDir {
  int16  nReserved; /* must be 0. */
  uint16 nType;     /* must be 1 for icons. */
  uint16 nCount;    /* number of images. */
//  WinIconImageDirEntry	asEntries[1];
} __attribute__((packed));


struct BitmapFileHeader
{
  uint16	nType;
  uint32	nSize;
  uint16	nReserved1;
  uint16	nReserved2;
  uint32	nRasterOffset;
} __attribute__((packed));


struct BitmapInfoHeader
{
  uint32 nSize;
  uint32 nWidth;
  uint32 nHeight;
  uint16 nPlanes;
  uint16 nBitsPerPix;
  uint32 nCompression;
  uint32 nImageSize;
  int32	 nPelsPerMeterX;
  int32	 nPelsPerMeterY;
  uint32 nColorsUsed;
  uint32 nImportantColors;
};

struct WinIconImageData {
  BitmapInfoHeader sHeader;
//  Color32_s	asColors[];
//  RGBQUAD idColors[1];
//  BYTE idXOR[1];      /* DIB bits for XOR mask. */
//  BYTE idAND[1];      /* DIB bits for AND mask. */
};

Bitmap* load_bmp( FILE* hFile, int nRasterOffset, int nWidth, int nHeight )
{
  BitmapInfoHeader sBitmapHeader;
  
  fread( &sBitmapHeader, 1, sizeof( sBitmapHeader ), hFile );
  dbprintf( "Width  = %ld\n", sBitmapHeader.nWidth );
  dbprintf( "Height = %ld\n", sBitmapHeader.nHeight );
  dbprintf( "Planes = %d\n", sBitmapHeader.nPlanes );
  dbprintf( "Depth  = %d\n", sBitmapHeader.nBitsPerPix );

  if ( nWidth == 0 || nHeight == 0 ) {
    nWidth  = sBitmapHeader.nWidth;
    nHeight = sBitmapHeader.nHeight;
  }
  dbprintf( "Create bitmap\n" );
  Bitmap* pcBitmap;
  try {
    pcBitmap = new Bitmap( nWidth, nHeight, CS_RGB32, Bitmap::SHARE_FRAMEBUFFER );
  }
  catch( GeneralFailure& cExc ) {
    printf( "Failed to create bitmap: %s\n", cExc.what() );
    return( NULL );
  }
  catch (...) {
    printf( "Failed to create bitmap: *unknown caught exception*\n" );
    return( NULL );
  }
  uint8* pRaster = pcBitmap->LockRaster();

  int nPalSize = 1 << sBitmapHeader.nBitsPerPix;
  int nLineSize   = (nWidth * sBitmapHeader.nBitsPerPix / 8 + 1) & ~1;
  int nRasterSize = nHeight * nLineSize;
  
  if ( sBitmapHeader.nBitsPerPix <= 8 )
  {
    Color32_s* pasPallette = new Color32_s[ nPalSize ];
    uint8*     pBuffer = new uint8[ nRasterSize ];
    uint8*     pSrc = pBuffer + nRasterSize - nLineSize;
    
    fread( pasPallette, nPalSize, sizeof( Color32_s ), hFile );

    if ( nRasterOffset != 0 ) {
      fseek( hFile, nRasterOffset, SEEK_SET );
    }
    fread( pBuffer, nRasterSize, 1, hFile );

    if ( sBitmapHeader.nBitsPerPix == 8 ) {
      for ( int y = 0 ; y < nHeight ; ++y )
      {
	for ( int x = 0 ; x < nWidth ; ++x )
	{
	  int nPix = *pSrc++;

	  if ( nPix == 0 ) {
	    memset( pRaster, 0xff, 3 );
	    pRaster[3] = 0x00;
	    pRaster += 4;
	  } else {
	  *pRaster++ = pasPallette[ nPix ].red;
	  *pRaster++ = pasPallette[ nPix ].green;
	  *pRaster++ = pasPallette[ nPix ].blue;
	  *pRaster++ = 255;
	  }
	}
	if ( nWidth & 0x01 ) {
	  pSrc++;
	}
	
	pSrc -= nLineSize * 2;
      }
    } else if ( sBitmapHeader.nBitsPerPix == 4 ) {
      for ( int y = 0 ; y < nHeight ; ++y )
      {
	for ( int x = 0 ; x < nWidth / 2 ; ++x )
	{
	  int nPix = *pSrc++;
	  *pRaster++ = pasPallette[ nPix >> 4 ].red;
	  *pRaster++ = pasPallette[ nPix >> 4 ].green;
	  *pRaster++ = pasPallette[ nPix >> 4 ].blue;
	  *pRaster++ = 255;

	  *pRaster++ = pasPallette[ nPix & 0x0f ].red;
	  *pRaster++ = pasPallette[ nPix & 0x0f ].green;
	  *pRaster++ = pasPallette[ nPix & 0x0f ].blue;
	  *pRaster++ = 255;
	}
	if ( nWidth & 0x01 ) {
	  pSrc++;
	}
	pSrc -= nLineSize * 2;
      }
    }
    delete[] pBuffer;
    delete[] pasPallette;
  } else if ( sBitmapHeader.nBitsPerPix == 24 ) {
    uint8*     pBuffer = new uint8[ nRasterSize ];
    uint8*     pSrc = pBuffer + nRasterSize - nLineSize;
    
    if ( nRasterOffset != 0 ) {
      fseek( hFile, nRasterOffset, SEEK_SET );
    }
    fread( pBuffer, nRasterSize, 1, hFile );
    for ( int y = 0 ; y < nHeight ; ++y )
    {
      for ( int x = 0 ; x < nWidth ; ++x )
      {
	if ( pSrc[0] == 0x00 && pSrc[1] == 0xff && pSrc[2] == 0x00 ) {
	  memset( pRaster, 0x00, 4 );
	  pRaster += 4;
	  pSrc += 3;
	} else {
	  for ( int i = 0 ; i < 3 ; ++i ) {
	    *pRaster++ = *pSrc++;
	  }
	  *pRaster++ = 255;
	}
      }
      pSrc -= nLineSize * 2;
    }
    delete[] pBuffer;
  }
  return( pcBitmap );
}

bool load_win_bmp( const char* pzPath, Bitmap** ppcImg )
{
  BitmapFileHeader sFileHeader;
  
  FILE* hFile = fopen( pzPath, "r" );

  if ( hFile == NULL ) {
    printf( "Failed to open %s\n", pzPath );
    return( false );
  }
  dbprintf( "load_win_bmp() file opened\n" );
  fread( &sFileHeader, 1, sizeof( sFileHeader ), hFile );
  dbprintf( "Type = %d\n", sFileHeader.nType );
  dbprintf( "Size = %ld\n", sFileHeader.nSize );
  dbprintf( "Offs = %ld\n", sFileHeader.nRasterOffset );
  *ppcImg = load_bmp( hFile, sFileHeader.nRasterOffset, 0, 0 );
  return( true );
}

bool load_win_icon( const char* pzPath, Bitmap** ppcImg1, Bitmap** ppcImg2 )
{
  FILE* hFile;

  hFile = fopen( pzPath, "r" );
  if ( hFile == NULL ) {
    printf( "Failed to open %s\n", pzPath );
    return( false );
  }

  WinIconImageDir sDir;

  dbprintf( "Read icon dir\n" );
  
  if ( fread( &sDir, sizeof( sDir ), 1, hFile ) != 1 ) {
    printf( "Failed to read image directory\n" );
    fclose( hFile );
    return( false );
  }
  if ( sDir.nType != 1 ) {
    fclose( hFile );
    dbprintf( "Not an icon, attempts to load as BMP\n" );
    return( load_win_bmp( pzPath, ppcImg1 ) );
  }
  dbprintf( "Icon containes %d images (%d)\n", sDir.nCount, sDir.nType );

  WinIconImageDirEntry* pasEntries = new WinIconImageDirEntry[sDir.nCount];
  
  if ( fread( pasEntries, sizeof( WinIconImageDirEntry ), sDir.nCount, hFile ) != sDir.nCount ) {
    printf( "Failed to read image directory entry\n" );
    fclose( hFile );
    return( false );
  }
  
  for ( int i = 0 ; i < sDir.nCount ; ++i ) {
    dbprintf( "%d, %d (%ld)\n", pasEntries[i].nWidth, pasEntries[i].nHeight, pasEntries[i].nImageOffset );
  }
  dbprintf( "Load bitmap\n" );
  fseek( hFile, pasEntries[0].nImageOffset, SEEK_SET );
  *ppcImg1 = load_bmp( hFile, 0, pasEntries[0].nWidth, pasEntries[0].nHeight );
/*  
  for ( int i = 0 ; i < sDir.nCount ; ++i ) {
//    BitmapInfoHeader sHeader;
    fseek( hFile, pasEntries[i].nImageOffset, SEEK_SET );
//    fread( &sHeader, sizeof( sHeader ), 1, hFile );
//    printf( "%ldx%ldx%d size=%ld\n", sHeader.nWidth, sHeader.nHeight, sHeader.nBitsPerPix, sHeader.nImageSize );
  }
  */  
  fclose( hFile );
  return( true );
}


