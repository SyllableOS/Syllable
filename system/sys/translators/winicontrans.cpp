#include <stdio.h>
#include <unistd.h>
#include <setjmp.h>

#include <atheos/types.h>
#include <atheos/kdebug.h>

#include <gui/bitmap.h>		// BitsPerPixel
#include <util/circularbuffer.h>
#include <storage/memfile.h>
#include <translation/translator.h>

#include <vector>
#include <new>
#include <string>

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


class WinIconTrans : public Translator
{
public:
    WinIconTrans( bool bWriteMode = false );
    virtual ~WinIconTrans();

    virtual void     SetConfig( const Message& cConfig ) {}
    virtual status_t AddData( const void* pData, size_t nLen, bool bFinal );
    virtual ssize_t  AvailableDataSize();
    virtual ssize_t  Read( void* pData, size_t nLen );
    virtual void     Abort();
    virtual void     Reset();
  
private:
    bool					m_bWriteMode;
    bool					m_bFirstFrame;
    BitmapHeader			m_sBitmapHeader;
    BitmapFrameHeader		m_sCurrentFrame;
    CircularBuffer			m_cOutBuffer;
    CircularBuffer			m_cInBuffer;
    MemFile					m_cFile;
	WinIconImageDir			m_sIconDir;
	WinIconImageDirEntry*	m_pasEntries;
	BitmapInfoHeader		m_sIconHdr;
	int						m_nFrameIndex;
	int						m_nFilePosition;

    enum {
		STATE_INIT,
		STATE_IMAGEDIRENTRY,
		STATE_SEEKICONHDR,
		STATE_ICONHDR,
		STATE_FRAMEHDR,
		STATE_READING
    } m_eState;
};

WinIconTrans::WinIconTrans( bool bWriteMode )
{
	m_bWriteMode = bWriteMode;
	m_eState = STATE_INIT;
}

WinIconTrans::~WinIconTrans()
{

}

status_t WinIconTrans::AddData( const void* pData, size_t nLen, bool bFinal )
{
	if( m_bWriteMode ) {
		// Not implemented
	} else {
		m_cFile.Write( pData, nLen );
		if( bFinal ) {
			m_cFile.Seek( 0, SEEK_SET );
			m_cFile.Read( &m_sIconDir, sizeof( m_sIconDir ) );
			if( m_sIconDir.nType != 1 )		// Not a .ICO
				return -1;
			m_pasEntries = new WinIconImageDirEntry[ m_sIconDir.nCount ];
			m_cFile.Read( m_pasEntries, ( sizeof( WinIconImageDirEntry ) * m_sIconDir.nCount ) );

			m_sBitmapHeader.bh_header_size = sizeof( m_sBitmapHeader );
			m_sBitmapHeader.bh_data_size = 4 * m_pasEntries[ 0 ].nWidth * m_pasEntries[ 0 ].nHeight;
			m_sBitmapHeader.bh_flags = 0;
			m_sBitmapHeader.bh_bounds.left = 0;
			m_sBitmapHeader.bh_bounds.top = 0;
			m_sBitmapHeader.bh_bounds.right = m_pasEntries[ 0 ].nWidth - 1;
			m_sBitmapHeader.bh_bounds.bottom = m_pasEntries[ 0 ].nHeight - 1;
			m_sBitmapHeader.bh_frame_count = m_sIconDir.nCount;
			m_sBitmapHeader.bh_bytes_per_row = m_pasEntries[ 0 ].nWidth * 4;
			m_sBitmapHeader.bh_color_space = CS_RGBA32;
			m_cOutBuffer.Write( &m_sBitmapHeader, sizeof( m_sBitmapHeader ) );

			for( m_nFrameIndex = 0; m_nFrameIndex < m_sIconDir.nCount; m_nFrameIndex++ )
			{
				m_cFile.Seek( m_pasEntries[m_nFrameIndex].nImageOffset, SEEK_SET );
				m_cFile.Read( &m_sIconHdr, sizeof( m_sIconHdr ) );

				int nWidth = m_pasEntries[ m_nFrameIndex ].nWidth;
				int nHeight = m_pasEntries[ m_nFrameIndex ].nHeight;

				if( nWidth == 0 || nHeight == 0 ) {
						nWidth = m_sIconHdr.nWidth;
						nHeight = m_sIconHdr.nHeight;
				}

				m_sCurrentFrame.bf_header_size = sizeof( m_sCurrentFrame );
				m_sCurrentFrame.bf_data_size = 4 * m_pasEntries[ m_nFrameIndex ].nWidth * m_pasEntries[ m_nFrameIndex ].nHeight;
				m_sCurrentFrame.bf_time_stamp = 0;
				m_sCurrentFrame.bf_color_space = CS_RGBA32;
				m_sCurrentFrame.bf_bytes_per_row = m_pasEntries[ m_nFrameIndex ].nWidth * 4;
				m_sCurrentFrame.bf_frame.left = 0;
				m_sCurrentFrame.bf_frame.top = 0;
				m_sCurrentFrame.bf_frame.right = m_pasEntries[ m_nFrameIndex ].nWidth - 1;
				m_sCurrentFrame.bf_frame.bottom = m_pasEntries[ m_nFrameIndex ].nHeight - 1;
				m_cOutBuffer.Write( &m_sCurrentFrame, sizeof( m_sCurrentFrame ) );

				int nPalSize = 1 << m_sIconHdr.nBitsPerPix;
				int nLineSize = ( nWidth * m_sIconHdr.nBitsPerPix / 8 + 1 ) & ~1;
				int nRasterSize = nHeight * nLineSize;

				if( m_sIconHdr.nBitsPerPix <= 8 )
				{
					Color32_s *pasPallette = new Color32_s[nPalSize];
					uint8 *pBuffer = new uint8[nRasterSize];
					uint8 *pSrc = pBuffer + nRasterSize - nLineSize;
					uint8 pixel[4];
				
					m_cFile.Read( pasPallette, nPalSize * sizeof( Color32_s ) );

					m_cFile.Read( pBuffer, nRasterSize );
				
					if( m_sIconHdr.nBitsPerPix == 8 )
					{
						for( int y = 0; y < nHeight; ++y )
						{
							for( int x = 0; x < nWidth; ++x )
							{
								int nPix = *pSrc++;
				
								if( nPix == 0 )
								{
									memset( pixel, 0xff, 3 );
									pixel[3] = 0x00;
								}
								else
								{
									pixel[0] = pasPallette[nPix].red;
									pixel[1] = pasPallette[nPix].green;
									pixel[2] = pasPallette[nPix].blue;
									pixel[3] = 255;
								}
								m_cOutBuffer.Write( &pixel, 4 );
							}
							if( nWidth & 0x01 )
							{
								pSrc++;
							}
				
							pSrc -= nLineSize * 2;
						}
					}
					else if( m_sIconHdr.nBitsPerPix == 4 )
					{
						for( int y = 0; y < nHeight; ++y )
						{
							for( int x = 0; x < nWidth / 2; ++x )
							{
								int nPix = *pSrc++;
				
								pixel[0] = pasPallette[nPix >> 4].red;
								pixel[1] = pasPallette[nPix >> 4].green;
								pixel[2] = pasPallette[nPix >> 4].blue;
								pixel[3] = 255;
								m_cOutBuffer.Write( &pixel, 4 );
				
								pixel[0] = pasPallette[nPix & 0x0f].red;
								pixel[1] = pasPallette[nPix & 0x0f].green;
								pixel[2] = pasPallette[nPix & 0x0f].blue;
								pixel[3] = 255;
								m_cOutBuffer.Write( &pixel, 4 );
							}
							if( nWidth & 0x01 )
							{
								pSrc++;
							}
							pSrc -= nLineSize * 2;
						}
					}
					delete[]pBuffer;
					delete[]pasPallette;
				} else if( m_sIconHdr.nBitsPerPix <= 24 ) {
					uint8 *pBuffer = new uint8[nRasterSize];
					uint8 *pSrc = pBuffer + nRasterSize - nLineSize;
					uint8 pixel[4];
			
					m_cFile.Read( pBuffer, nRasterSize );

					for( int y = 0; y < nHeight; ++y )
					{
						for( int x = 0; x < nWidth; ++x )
						{
							if( pSrc[0] == 0x00 && pSrc[1] == 0xff && pSrc[2] == 0x00 )
							{
								memset( pixel, 0x00, 4 );
								pSrc += 3;
							}
							else
							{
								for( int i = 0; i < 3; ++i )
								{
									pixel[i] = *pSrc++;
								}
								pixel[3] = 255;
							}
							m_cOutBuffer.Write( &pixel, 4 );
						}
						pSrc -= nLineSize * 2;
					}
					delete[]pBuffer;
				}
			}
			delete []m_pasEntries;
		}
	}
//	if( m_bWriteMode ) {
/*		bool repeat;

		// Data in TRANSLATOR_TYPE_IMAGE format has been added, translate it to icon format

		// Find a BitmapHeader structure:
		if( m_eState == STATE_INIT && m_cInBuffer.Size() > (ssize_t)sizeof( BitmapHeader ) ) {
			m_cInBuffer.Read( &m_sBitmapHeader, sizeof( m_sBitmapHeader ) );
			m_eState = STATE_FRAMEHDR;
			m_sIconDir.nIconMagic = ICON_MAGIC;
			m_sIconDir.nNumImages = m_sBitmapHeader.bh_frame_count;
			m_cOutBuffer.Write( &m_sIconDir, sizeof( m_sIconDir ) );
		}

		// Read frames, one by one:
		do {
			repeat = false;

			if( m_eState == STATE_FRAMEHDR && m_cInBuffer.Size() > (ssize_t)sizeof( BitmapFrameHeader ) ) {
				m_cInBuffer.Read( &m_sCurrentFrame, sizeof( m_sCurrentFrame ) );
				m_sIconHdr.nBitsPerPixel = BitsPerPixel( m_sCurrentFrame.bf_color_space );
				m_sIconHdr.nWidth = m_sCurrentFrame.bf_frame.right - m_sCurrentFrame.bf_frame.left;
				m_sIconHdr.nHeight = m_sCurrentFrame.bf_frame.bottom - m_sCurrentFrame.bf_frame.top;
				m_cOutBuffer.Write( &m_sIconHdr, sizeof( m_sIconHdr ) );
				m_sIconDir.nNumImages--;
				m_eState = STATE_READING;
			}
			
			while( m_eState == STATE_READING && m_cInBuffer.Size() ) {
				uint8	buffer[ 4096 ];
				uint32	nSize = m_sCurrentFrame.bf_data_size > sizeof( buffer ) ? sizeof( buffer ) : m_sCurrentFrame.bf_data_size;

				nSize = nSize > m_cInBuffer.Size() ? m_cInBuffer.Size() : nSize ;
	
				if( nSize ) {
					m_cInBuffer.Read( buffer, nSize );
					m_cOutBuffer.Write( buffer, nSize );
					m_sCurrentFrame.bf_data_size -= nSize;
				} else if( m_sIconDir.nNumImages ) {
					m_eState = STATE_FRAMEHDR;
					repeat = true;
				}
			}
		} while(repeat);*/


/*	} else {
		bool repeat;

		// Data in icon format has been added, translate it to TRANSLATOR_TYPE_IMAGE format

		// Find an IconDir structure:
		if( m_eState == STATE_INIT && m_cInBuffer.Size() > (ssize_t)sizeof( IconDir ) ) {
			m_nFilePosition = m_cInBuffer.Read( &m_sIconDir, sizeof( m_sIconDir ) );
			if( m_sIconDir.nType != 1 )		// Not a .ICO
				return -1;
			m_eState = STATE_IMAGEDIRENTRY;
			m_bFirstFrame = true;
			m_pasEntries = new WinIconImageDirEntry[ m_sIconDir.nCount ];
			m_nFrameIndex = 0;
		}

		// Find ImageDirEntry:		
		if( m_eState == STATE_IMAGEDIRENTRY && m_cInBuffer.Size() > (ssize_t)( sizeof( WinIconImageDirEntry ) * m_sIconDir.nCount ) ) {
			m_nFilePosition += m_cInBuffer.Read( m_pasEntries, ( sizeof( WinIconImageDirEntry ) * m_sIconDir.nCount ) );
			m_eState = STATE_SEEKICONHDR;
		}

		// Read frames, one by one:
		do {
			repeat = false;

			if( m_eState == STATE_SEEKICONHDR ) {
				uint8 buf;
				while( m_nFilePosition < pasEntries[i].nImageOffset && m_cInBuffer.Size() )
					m_nFilePosition += m_cInBuffer.Read( &buf, 1 );
				if( m_nFilePosition == pasEntries[i].nImageOffset )
					m_eState = STATE_ICONHDR;
			}

			if( m_eState == STATE_ICONHDR && m_cInBuffer.Size() > (ssize_t)sizeof( BitmapInfoHeader ) ) {
				m_cInBuffer.Read( &m_sIconHdr, sizeof( m_sIconHdr ) );

				if( m_pasEntries[ m_nFrameIndex ].nWidth == 0 ||
					m_pasEntries[ m_nFrameIndex ].nHeight == 0 ) {
						m_pasEntries[ m_nFrameIndex ].nWidth = m_sIconHdr.nWidth;
						m_pasEntries[ m_nFrameIndex ].nHeight = m_sIconHdr.nHeight;
				}

				if( m_bFirstFrame ) {
					m_sBitmapHeader.bh_header_size = sizeof( m_sBitmapHeader );
					m_sBitmapHeader.bh_data_size = 4 * m_pasEntries[ m_nFrameIndex ].nWidth * m_pasEntries[ m_nFrameIndex ].nHeight;
					m_sBitmapHeader.bh_flags = 0;
					m_sBitmapHeader.bh_bounds.left = 0;
					m_sBitmapHeader.bh_bounds.top = 0;
					m_sBitmapHeader.bh_bounds.right = m_pasEntries[ m_nFrameIndex ].nWidth;
					m_sBitmapHeader.bh_bounds.bottom = m_pasEntries[ m_nFrameIndex ].nHeight;
					m_sBitmapHeader.bh_frame_count = m_sIconDir.nCount;
					m_sBitmapHeader.bh_bytes_per_row = m_pasEntries[ m_nFrameIndex ].nWidth * 4;
					m_sBitmapHeader.bh_color_space = CS_RGBA32;
					m_cOutBuffer.Write( &m_sBitmapHeader, sizeof( m_sBitmapHeader ) );
					m_bFirstFrame = false;
				}

				m_sCurrentFrame.bf_header_size = sizeof( m_sCurrentFrame );
				m_sCurrentFrame.bf_data_size = 4 * m_pasEntries[ m_nFrameIndex ].nWidth * m_pasEntries[ m_nFrameIndex ].nHeight;
				m_sCurrentFrame.bf_time_stamp = 0;
				m_sCurrentFrame.bf_color_space = CS_RGBA32;
				m_sCurrentFrame.bf_bytes_per_row = m_pasEntries[ m_nFrameIndex ].nWidth * 4;
				m_sCurrentFrame.bf_frame.left = 0;
				m_sCurrentFrame.bf_frame.top = 0;
				m_sCurrentFrame.bf_frame.right = m_pasEntries[ m_nFrameIndex ].nWidth;
				m_sCurrentFrame.bf_frame.bottom = m_pasEntries[ m_nFrameIndex ].nHeight;
				m_cOutBuffer.Write( &m_sCurrentFrame, sizeof( m_sCurrentFrame ) );
	
				m_sBitmapHeader.bh_frame_count--;
				m_eState = STATE_READING;
				m_nFrameIndex++;
			}
			
			while( m_eState == STATE_READING && m_cInBuffer.Size() ) {
				uint8	buffer[ 4096 ];
				uint32	nSize = m_sCurrentFrame.bf_data_size > sizeof( buffer ) ? sizeof( buffer ) : m_sCurrentFrame.bf_data_size;

				nSize = nSize > m_cInBuffer.Size() ? m_cInBuffer.Size() : nSize ;
	
				if( nSize ) {
					m_cInBuffer.Read( buffer, nSize );
					m_cOutBuffer.Write( buffer, nSize );
					m_sCurrentFrame.bf_data_size -= nSize;
				} else if( m_sBitmapHeader.bh_frame_count ) {
					m_eState = STATE_ICONHDR;
					repeat = true;
				}
			}
		} while( repeat );
	}*/
	return 0;
}

ssize_t WinIconTrans::AvailableDataSize()
{
    return( m_cOutBuffer.Size() );
}

ssize_t WinIconTrans::Read( void* pData, size_t nLen )
{
    int nCurLen = m_cOutBuffer.Read( pData, nLen );
    if ( nCurLen == 0 /*&& m_bDone == false*/ ) {
	return( ERR_SUSPENDED );
    }
    return( nCurLen );
}

void WinIconTrans::Abort()
{
}

void WinIconTrans::Reset()
{
}

class WinIconReaderNode : public TranslatorNode
{
public:
    virtual int Identify( const String& cSrcType, const String& cDstType, const void* pData, int nLen ) {
	if ( nLen < 4 ) {
	    return( TranslatorFactory::ERR_NOT_ENOUGH_DATA );
	}
	if ( *((int *)pData) == 0x10000 ) {
	    return( 0 );
	}
	return( TranslatorFactory::ERR_UNKNOWN_FORMAT );
    }
    virtual TranslatorInfo GetTranslatorInfo()
    {
	static TranslatorInfo sInfo = { "image/windows-icon", TRANSLATOR_TYPE_IMAGE, 1.0f };
		return( sInfo );
	}
	virtual Translator*	   CreateTranslator() {
		return( new WinIconTrans );
	}
};

/*class IconWriterNode : public TranslatorNode
{
public:
    virtual int Identify( const String& cSrcType, const String& cDstType, const void* pData, int nLen ) {
    	if( cSrcType == TRANSLATOR_TYPE_IMAGE && cDstType == "image/atheos-icon" ) {
		return( 0 );
	}
	return( TranslatorFactory::ERR_UNKNOWN_FORMAT );
    }
    virtual TranslatorInfo GetTranslatorInfo()
    {
	static TranslatorInfo sInfo = { TRANSLATOR_TYPE_IMAGE, "image/atheos-icon", 1.0f };
	return( sInfo );
    }
    virtual Translator*	   CreateTranslator() {
	return( new IconTrans( true ) );
    }
};
*/
extern "C" {
#if 0
}
#endif

int get_translator_count()
{
   // return( 2 );
    return( 1 );
}

TranslatorNode* get_translator_node( int nIndex )
{
	static WinIconReaderNode sRdNode;
//	static IconWriterNode sWrNode;
	switch( nIndex ) {
/*		case 1:
			return( &sWrNode );*/
		default:
			return( &sRdNode );
	}
}
    
};

