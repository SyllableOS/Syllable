/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 Syllable Team
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

#ifndef __F_TRANSLATION_TRANSLATOR_H__
#define __F_TRANSLATION_TRANSLATOR_H__

#include <sys/types.h>

#include <gui/rect.h>
#include <gui/guidefines.h>

#include <util/string.h>

namespace os
{
#if 0
}
#endif

class Messenger;
class Message;

enum
{
    BMF_MULTIPASS = 0x0001
};


static const char* TRANSLATOR_TYPE_IMAGE = "image/x-atheos_trans";

struct BitmapHeader
{
    size_t	bh_header_size;
    size_t	bh_data_size;
    uint32	bh_flags;
    IRect	bh_bounds;
    int		bh_frame_count;
    int		bh_bytes_per_row;
    color_space bh_color_space;
};

struct BitmapFrameHeader
{
    size_t 	bf_header_size;
    size_t 	bf_data_size;
    uint32  	bf_time_stamp;
    color_space bf_color_space;
    int		bf_bytes_per_row;
    IRect	bf_frame;
};


class DataReceiver
{
public:
    DataReceiver();
    virtual ~DataReceiver();

    virtual void DataReady( void* pData, size_t nLen, bool bFinal ) = 0;
private:
    void* m;
};

class Translator
{
public:
    enum { ERR_OK = 0, ERR_INVALID_DATA = -1, ERR_SUSPENDED = -2 };
public:
    Translator();
    virtual ~Translator();

    
    void    SetMessage( const Message& cMsg );
    void    SetTarget( const Messenger& cTarget, bool bSendData );
    void    SetTarget( DataReceiver* pcTarget );

    virtual void     SetConfig( const Message& cConfig ) = 0;
    virtual status_t AddData( const void* pData, size_t nLen, bool bFinal );
    virtual ssize_t  AvailableDataSize() = 0;
    virtual ssize_t  Read( void* pData, size_t nLen ) = 0;
    virtual void     Abort() = 0;
    virtual void     Reset() = 0;
protected:
    virtual status_t DataAdded( void* pData, size_t nLen, bool bFinal );
    void Invoke( void* pData, size_t nSize, bool bFinal );
private:
    struct Internal;

    Internal* m;
};

struct TranslatorInfo
{
    String source_type;
    String dest_type;
    float	quality;
};

class TranslatorNode
{
public:
    TranslatorNode();
    virtual ~TranslatorNode();

    virtual int Identify( const String& cSrcType, const String& cDstType, const void* pData, int nLen ) = 0;
    virtual TranslatorInfo GetTranslatorInfo() = 0;
    virtual Translator*	   CreateTranslator() = 0;
private:
    void* m;
};



class TranslatorFactory
{
public:
    enum { ERR_NOT_ENOUGH_DATA = -1, ERR_UNKNOWN_FORMAT = -2, ERR_NO_MEM = -3 };
public:
    TranslatorFactory();
    ~TranslatorFactory();

    static TranslatorFactory* GetDefaultFactory();
    
    void LoadAll();

    status_t FindTranslator( const String& cSrcType, const String& cDstType,
			     const void* pData, size_t nLen, Translator** ppcTrans );

    int             GetTranslatorCount();
    TranslatorNode* GetTranslatorNode( int nIndex );
    TranslatorInfo  GetTranslatorInfo( int nIndex );
    Translator*     CreateTranslator( int nIndex );

private:
    struct Internal;
    Internal* m;
};

    
} // end of namespace

#endif // __F_TRANSLATION_TRANSLATOR_H__
