#include <stdio.h>

#include "editview.h"
#include "iconview.h"
#include "bitmapscale.h"

#include <gui/bitmap.h>
#include <gui/icon.h>

#include <util/message.h>
#include <util/messenger.h>


bool load_win_icon( const char* pzPath, Bitmap** pcImg1, Bitmap** pcImg2 );

EditView::EditView( const Rect& cFrame ) : View( cFrame, "edit_view" )
{
  m_pcScaleIconView = new IconView( Rect( 0, 0, 255, 255 ) );
  AddChild( m_pcScaleIconView );

  m_cLargeRect = Rect( 0, 0, 31, 31 ) + Point( 268, 20 );
  m_cSmallRect = Rect( 0, 0, 15, 15 ) + Point( 308, 28 );

  m_pcLargeBitmap = NULL;
  m_pcSmallBitmap = NULL;
  m_bLargeSelected = true;
}


void EditView::Save( const char* pzName )
{
  IconDir sDir;
  IconHeader sHeader;

  FILE* hFile = fopen( pzName, "w" );

  if ( hFile == NULL ) {
    printf( "Failed to create file %s\n", pzName );
    return;
  }
  
  sDir.nIconMagic = ICON_MAGIC;
  sDir.nNumImages  = 2;

  if ( fwrite( &sDir, sizeof( sDir ), 1, hFile ) != 1 ) {
    printf( "Failed to write icon header\n" );
  }
  sHeader.nBitsPerPixel = 32;
  sHeader.nWidth = 32;
  sHeader.nHeight = 32;
  if ( fwrite( &sHeader, sizeof( sHeader ), 1, hFile ) != 1 ) {
    printf( "Failed to write icon header\n" );
  }
  fwrite( m_pcLargeBitmap->LockRaster(), 32*32*4, 1, hFile );
  sHeader.nWidth  = 16;
  sHeader.nHeight = 16;
  if ( fwrite( &sHeader, sizeof( sHeader ), 1, hFile ) != 1 ) {
    printf( "Failed to write icon header\n" );
  }
  fwrite( m_pcSmallBitmap->LockRaster(), 16*16*4, 1, hFile );
  fclose( hFile );
}

void EditView::Load( const char* pzName )
{
  IconDir sDir;
  IconHeader sHeader;

  FILE* hFile = fopen( pzName, "r" );

  if ( hFile == NULL ) {
    printf( "Failed to create file %s\n", pzName );
    return;
  }
  
  if ( fread( &sDir, sizeof( sDir ), 1, hFile ) != 1 ) {
    printf( "Failed to read icon dir\n" );
  }
  for ( int i = 0 ; i < sDir.nNumImages ; ++i ) {
    if ( fread( &sHeader, sizeof( sHeader ), 1, hFile ) != 1 ) {
      printf( "Failed to read icon header\n" );
      break;
    }
    if ( sHeader.nWidth == 32 ) {
      delete m_pcLargeBitmap;
      m_pcLargeBitmap = new Bitmap( 32, 32, CS_RGB32, Bitmap::SHARE_FRAMEBUFFER );
      fread( m_pcLargeBitmap->LockRaster(), 32*32*4, 1, hFile );
    } else if ( sHeader.nWidth == 16 ) {
      delete m_pcSmallBitmap;
      m_pcSmallBitmap = new Bitmap( 16, 16, CS_RGB32, Bitmap::SHARE_FRAMEBUFFER );
      fread( m_pcSmallBitmap->LockRaster(), 16*16*4, 1, hFile );
    }
  }
  fclose( hFile );
  if ( m_bLargeSelected ) {
    m_pcScaleIconView->SetBitmap( m_pcLargeBitmap );
  } else {
    m_pcScaleIconView->SetBitmap( m_pcSmallBitmap );
  }
}

void EditView::Paint( const Rect& cUpdateRect )
{
  FillRect( cUpdateRect, get_default_color( COL_NORMAL ) );

  Rect cLargeRect = m_cLargeRect;
  Rect cSmallRect = m_cSmallRect;

  cLargeRect.left   -= 2.0f;
  cLargeRect.top    -= 2.0f;
  cLargeRect.right  += 2.0f;
  cLargeRect.bottom += 2.0f;

  cSmallRect.left   -= 2.0f;
  cSmallRect.top    -= 2.0f;
  cSmallRect.right  += 2.0f;
  cSmallRect.bottom += 2.0f;

  if ( m_bLargeSelected ) {
    DrawFrame( cLargeRect, FRAME_RECESSED | FRAME_TRANSPARENT );
    DrawFrame( cSmallRect, FRAME_RAISED | FRAME_TRANSPARENT );
  } else {
    DrawFrame( cLargeRect, FRAME_RAISED | FRAME_TRANSPARENT );
    DrawFrame( cSmallRect, FRAME_RECESSED | FRAME_TRANSPARENT );
  }
  SetDrawingMode( DM_BLEND );
  if ( m_pcLargeBitmap != NULL ) {
    DrawBitmap( m_pcLargeBitmap, m_pcLargeBitmap->GetBounds(), m_cLargeRect );
  }
  if ( m_pcSmallBitmap != NULL ) {
    DrawBitmap( m_pcSmallBitmap, m_pcSmallBitmap->GetBounds(), m_cSmallRect );
  }
  SetDrawingMode( DM_COPY );
}

void EditView::MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData )
{
  if ( (nButtons & 0x01) == 0 ) {
    return;
  }
  if ( pcData != NULL ) {
    return;
  }
  Message* pcMsg = NULL;
  Point   cOffset;
  Rect	   cFrame;
  if ( m_cLargeRect.DoIntersect( cNewPos ) ) {
    pcMsg = new Message( ID_DRAG_LARGE );
    cFrame = Rect( 0, 0, 31, 31 );
    cOffset = cNewPos - m_cLargeRect.LeftTop();
  } else if ( m_cSmallRect.DoIntersect( cNewPos ) ) {
    pcMsg = new Message( ID_DRAG_SMALL );
    cFrame  = Rect( 0, 0, 15, 15 );
    cOffset = cNewPos - m_cSmallRect.LeftTop();
  }
  if ( pcMsg != NULL ) {
    BeginDrag( pcMsg, cOffset, cFrame );
  }
}
void EditView::MouseDown( const Point& cPosition, uint32 nButtons )
{
  bool bOldSelection = m_bLargeSelected;
  
  if ( m_cLargeRect.DoIntersect( cPosition ) ) {
    m_bLargeSelected = true;
  } else if ( m_cSmallRect.DoIntersect( cPosition ) ) {
    m_bLargeSelected = false;
  }
  if ( m_bLargeSelected != bOldSelection ) {
    if ( m_bLargeSelected ) {
      m_pcScaleIconView->SetBitmap( m_pcLargeBitmap );
    } else {
      m_pcScaleIconView->SetBitmap( m_pcSmallBitmap );
    }
    Paint( GetBounds() );
    Flush();
  }
}

void EditView::MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData )
{
  if ( pcData == NULL ) {
    View::MouseUp( cPosition, nButtons, pcData );
    return;
  }

  if ( (pcData->GetCode() == ID_DRAG_LARGE || pcData->GetCode() == ID_DRAG_SMALL) &&
       pcData->ReturnAddress() == Messenger( this ) ) {
    if ( pcData->GetCode() == ID_DRAG_SMALL && m_cLargeRect.DoIntersect( cPosition ) ) {
      GenerateLarge();
      printf( "Drop L\n" );
    }
    if ( pcData->GetCode() == ID_DRAG_LARGE && m_cSmallRect.DoIntersect( cPosition ) ) {
      GenerateSmall();
      printf( "Drop S\n" );
    }
  } else {
    const char* pzPath;
    if ( pcData->FindString( "file/path", &pzPath ) == 0 ) {
      delete m_pcLargeBitmap;
      m_pcLargeBitmap = NULL;
      load_win_icon( pzPath, &m_pcLargeBitmap, NULL );

      if ( m_pcLargeBitmap != NULL ) {
	m_pcSmallBitmap = new Bitmap( 16, 16, CS_RGB32, Bitmap::SHARE_FRAMEBUFFER );

	Scale( m_pcLargeBitmap, m_pcSmallBitmap, filter_lanczos3, 0.0f );
      }
    }
    if ( m_bLargeSelected ) {
      m_pcScaleIconView->SetBitmap( m_pcLargeBitmap );
    } else {
      m_pcScaleIconView->SetBitmap( m_pcSmallBitmap );
    }
  }
  Paint( GetBounds() );
  Flush();
}

void EditView::GenerateSmall()
{
  if ( m_pcLargeBitmap == NULL ) {
    return;
  }

  delete m_pcSmallBitmap;
  m_pcSmallBitmap = new Bitmap( 16, 16, CS_RGB32, Bitmap::SHARE_FRAMEBUFFER );
  
  Scale( m_pcLargeBitmap, m_pcSmallBitmap, m_nFilterType, 0.0f );
  if ( m_bLargeSelected ) {
    m_pcScaleIconView->SetBitmap( m_pcLargeBitmap );
  } else {
    m_pcScaleIconView->SetBitmap( m_pcSmallBitmap );
  }
  Paint( GetBounds() );
  Flush();
}

void EditView::GenerateLarge()
{
  if ( m_pcSmallBitmap == NULL ) {
    return;
  }

  delete m_pcLargeBitmap;
  m_pcLargeBitmap = new Bitmap( 32, 32, CS_RGB32, Bitmap::SHARE_FRAMEBUFFER );
  
  Scale( m_pcSmallBitmap, m_pcLargeBitmap, m_nFilterType, 0.0f );
//  m_pcLargeIconView->SetBitmap( m_pcLargeBitmap );
  Paint( GetBounds() );
  Flush();
}


void EditView::KeyDown( int nChar, uint32 nQualifiers )
{
}
