#include "LoadBitmap.h"
#include <gui/font.h>
#include <stdio.h>

/*Bitmap *LoadBitmapFromFile(const char *name)
{
    os::File file(name);
 
    return LoadBitmapFromStream(&file);
}
 
Bitmap *LoadIconFromFile(const char* name)
{
    os::File file(name);
    return LoadIconFromStream(&file);
}*/

Bitmap *LoadBitmapFromResource(const char *name)
{
    os::Resources cRes(get_image_id());

    os::ResStream* pcStream = cRes.GetResourceStream(name);
    if(pcStream == NULL)
    {
        throw(os::errno_exception(""));
    }
    else
    {
        return LoadBitmapFromStream(pcStream);
    }
}

Bitmap *LoadBitmapFromStream(StreamableIO* pcStream)
{
    os::Translator* pcTrans = NULL;

    os::TranslatorFactory* pcFactory = new os::TranslatorFactory;
    pcFactory->LoadAll();

    os::Bitmap* pcBitmap = NULL;

    try
    {
        uint8  anBuffer[8192];
        uint   nFrameSize = 0;
        int    x = 0;
        int    y = 0;

        os::BitmapFrameHeader sFrameHeader;
        for (;;)
        {
            int nBytesRead = pcStream->Read( anBuffer, sizeof(anBuffer) );

            if (nBytesRead == 0 )
            {
                break;
            }

            if ( pcTrans == NULL )
            {
                int nError = pcFactory->FindTranslator( "", os::TRANSLATOR_TYPE_IMAGE, anBuffer, nBytesRead, &pcTrans );
                if ( nError < 0 )
                {
                    return( NULL );
                }
            }

            pcTrans->AddData( anBuffer, nBytesRead, nBytesRead != sizeof(anBuffer) );

            while( true )
            {
                if ( pcBitmap == NULL )
                {
                    os::BitmapHeader sBmHeader;
                    if ( pcTrans->Read( &sBmHeader, sizeof(sBmHeader) ) != sizeof( sBmHeader ) )
                    {
                        break;
                    }
                    pcBitmap = new os::Bitmap( sBmHeader.bh_bounds.Width() + 1, sBmHeader.bh_bounds.Height() + 1, os::CS_RGB32 );
                }
                if ( nFrameSize == 0 )
                {
                    if ( pcTrans->Read( &sFrameHeader, sizeof(sFrameHeader) ) != sizeof( sFrameHeader ) )
                    {
                        break;
                    }
                    x = sFrameHeader.bf_frame.left * 4;
                    y = sFrameHeader.bf_frame.top;
                    nFrameSize = sFrameHeader.bf_data_size;
                }
                uint8 pFrameBuffer[8192];
                nBytesRead = pcTrans->Read( pFrameBuffer, std::min( nFrameSize, sizeof(pFrameBuffer) ) );
                if ( nBytesRead <= 0 )
                {
                    break;
                }
                nFrameSize -= nBytesRead;
                uint8* pDstRaster = pcBitmap->LockRaster();
                int nSrcOffset = 0;
                for ( ; y <= sFrameHeader.bf_frame.bottom && nBytesRead > 0 ; )
                {
                    int nLen = std::min( nBytesRead, sFrameHeader.bf_bytes_per_row - x );
                    memcpy( pDstRaster + y * pcBitmap->GetBytesPerRow() + x, pFrameBuffer + nSrcOffset, nLen );
                    if ( x + nLen == sFrameHeader.bf_bytes_per_row )
                    {
                        x = 0;
                        y++;
                    }
                    else
                    {
                        x += nLen;
                    }
                    nBytesRead -= nLen;
                    nSrcOffset += nLen;
                }
            }
        }
        return( pcBitmap );
    }
    catch( ... )
    {
        return( NULL );
    }
}

/*Bitmap *LoadIconFromStream(StreamableIO* pcStream)
{
    os::Translator* pcTrans = NULL;
 
    os::TranslatorFactory* pcFactory = new os::TranslatorFactory;
    pcFactory->LoadAll();
 
    os::Bitmap* pcBitmap = NULL;
 
    try
    {
        uint8  anBuffer[8192];
        uint   nFrameSize = 0;
        int    x = 0;
        int    y = 0;
 
        os::BitmapFrameHeader sFrameHeader;
        for (;;)
        {
            int nBytesRead = pcStream->Read( anBuffer, sizeof(anBuffer) );
 
            if (nBytesRead == 0 )
            {
                break;
            }
 
            if ( pcTrans == NULL )
            {
                int nError = pcFactory->FindTranslator( "", os::TRANSLATOR_TYPE_IMAGE, anBuffer, nBytesRead, &pcTrans );
                if ( nError < 0 )
                {
                    return( NULL );
                }
            }
 
            pcTrans->AddData( anBuffer, nBytesRead, nBytesRead != sizeof(anBuffer) );
 
            while( true )
            {
                if ( pcBitmap == NULL )
                {
                    os::BitmapHeader sBmHeader;
                    if ( pcTrans->Read( &sBmHeader, sizeof(sBmHeader) ) != sizeof( sBmHeader ) )
                    {
                        break;
                    }
                    pcBitmap = new os::Bitmap( sBmHeader.bh_bounds.Width() + 1, sBmHeader.bh_bounds.Height() + 1, os::CS_RGBA32 );
                }
                if ( nFrameSize == 0 )
                {
                    if ( pcTrans->Read( &sFrameHeader, sizeof(sFrameHeader) ) != sizeof( sFrameHeader ) )
                    {
                        break;
                    }
                    x = sFrameHeader.bf_frame.left * 4;
                    y = sFrameHeader.bf_frame.top;
                    nFrameSize = sFrameHeader.bf_data_size;
                }
                uint8 pFrameBuffer[8192];
                nBytesRead = pcTrans->Read( pFrameBuffer, std::min( nFrameSize, sizeof(pFrameBuffer) ) );
                if ( nBytesRead <= 0 )
                {
                    break;
                }
                nFrameSize -= nBytesRead;
                uint8* pDstRaster = pcBitmap->LockRaster();
                int nSrcOffset = 0;
                for ( ; y <= sFrameHeader.bf_frame.bottom && nBytesRead > 0 ; )
                {
                    int nLen = std::min( nBytesRead, sFrameHeader.bf_bytes_per_row - x );
                    memcpy( pDstRaster + y * pcBitmap->GetBytesPerRow() + x, pFrameBuffer + nSrcOffset, nLen );
                    if ( x + nLen == sFrameHeader.bf_bytes_per_row )
                    {
                        x = 0;
                        y++;
                    }
                    else
                    {
                        x += nLen;
                    }
                    nBytesRead -= nLen;
                    nSrcOffset += nLen;
                }
            }
        }
        return( pcBitmap );
    }
    catch( ... )
    {
        return( NULL );
    }
}*/






