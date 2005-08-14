
/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 2001  Kurt Skauen
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

#include <atheos/kernel.h>
#include <atheos/image.h>

#include <memory.h>

#include <translation/translator.h>
#include <util/message.h>
#include <util/messenger.h>
#include <storage/directory.h>

#include <vector>
#include <new>

using namespace os;


static TranslatorFactory *g_pcDefaultFactory = NULL;


struct Plugin
{
	int m_nLibrary;
	  std::vector < TranslatorNode * >m_cNodes;
};

struct TranslatorFactory::Internal
{
	std::vector < Plugin * >m_cPlugins;
	std::vector < TranslatorNode * >m_cNodes;
};

TranslatorFactory::TranslatorFactory()
{
	m = new Internal;
}

TranslatorFactory::~TranslatorFactory()
{
	delete m;
}

typedef int get_translator_count();
typedef TranslatorNode *get_translator_node( int nIndex );

void TranslatorFactory::LoadAll()
{
	try
	{
		Directory cDir( "/system/extensions/translators" );

		String cName;

		while( cDir.GetNextEntry( &cName ) == 1 )
		{
			if( cName == "." || cName == ".." )
			{
				continue;
			}
			String cPath( "/system/extensions/translators/" );

			cPath += cName;

			int nLib = load_library( cPath.c_str(), 0 );

			if( nLib < 0 )
			{
				dbprintf( "failed to load translator %s\n", cPath.c_str() );
				continue;
			}
			get_translator_count *pGetTransCount;
			get_translator_node *pGetTrans;

			int nError;

			nError = get_symbol_address( nLib, "get_translator_count", -1, ( void ** )&pGetTransCount );
			if( nError < 0 )
			{
				dbprintf( "Translator %s does not export entry point get_translator_count()\n", cName.c_str() );
				unload_library( nLib );
				continue;
			}
			nError = get_symbol_address( nLib, "get_translator_node", -1, ( void ** )&pGetTrans );
			if( nError < 0 )
			{
				dbprintf( "Translator %s does not export entry point get_translator_node()\n", cName.c_str() );
				unload_library( nLib );
				continue;
			}
			int nTransCount = pGetTransCount();

			if( nTransCount == 0 )
			{
				unload_library( nLib );
				continue;
			}
			Plugin *pcPlugin = new Plugin;

			pcPlugin->m_nLibrary = nLib;
			m->m_cPlugins.push_back( pcPlugin );

			for( int i = 0; i < nTransCount; ++i )
			{
				TranslatorNode *pcNode = pGetTrans( i );

				if( pcNode != NULL )
				{
					pcPlugin->m_cNodes.push_back( pcNode );
					m->m_cNodes.push_back( pcNode );
				}
			}
		}
	}
	catch( ... )
	{
	}
}

status_t TranslatorFactory::FindTranslator( const String &cSrcType, const String &cDstType, const void *pData, size_t nLen, Translator ** ppcTrans )
{
	float vBestQuality = -1000.0f;
	TranslatorNode *pcBestTrans = NULL;
	int nError = ERR_UNKNOWN_FORMAT;

	for( uint i = 0; i < m->m_cNodes.size(); ++i )
	{
		TranslatorNode *pcNode = m->m_cNodes[i];
		TranslatorInfo sInfo = pcNode->GetTranslatorInfo();

		if( !( sInfo.dest_type == cDstType ) )
		{
			continue;
		}

		int nResult = pcNode->Identify( cSrcType, cDstType, pData, nLen );

		if( nResult == 0 )
		{
			if( sInfo.quality > vBestQuality )
			{
				vBestQuality = sInfo.quality;
				pcBestTrans = pcNode;
			}
		}
		else if( nResult == ERR_NOT_ENOUGH_DATA )
		{
			nError = ERR_NOT_ENOUGH_DATA;
		}
	}
	if( pcBestTrans != NULL )
	{
		*ppcTrans = pcBestTrans->CreateTranslator();
		if( *ppcTrans != NULL )
		{
			nError = 0;
		}
		else
		{
			nError = ERR_NO_MEM;
		}
	}
	return ( nError );
}

int TranslatorFactory::GetTranslatorCount()
{
	return ( m->m_cNodes.size() );
}

TranslatorNode *TranslatorFactory::GetTranslatorNode( int nIndex )
{
	return ( m->m_cNodes[nIndex] );
}

TranslatorInfo TranslatorFactory::GetTranslatorInfo( int nIndex )
{
	return ( m->m_cNodes[nIndex]->GetTranslatorInfo() );
}

Translator *TranslatorFactory::CreateTranslator( int nIndex )
{
	return ( m->m_cNodes[nIndex]->CreateTranslator() );
}




TranslatorFactory *TranslatorFactory::GetDefaultFactory()
{
	if( g_pcDefaultFactory == NULL )
	{
		g_pcDefaultFactory = new TranslatorFactory();
		g_pcDefaultFactory->LoadAll();
	}
	return ( g_pcDefaultFactory );
}















struct Translator::Internal
{
	Internal()
	{
		m_pcDataTarget = NULL;
		m_nBufSize = 0;
		m_pnBuffer = NULL;
	}
	~Internal()
	{
		if( m_pnBuffer )
			delete[]m_pnBuffer;
	}
	void ResizeBuffer( uint32 nNewSize )
	{
		uint8 *tmp = new uint8[nNewSize];

		memcpy( tmp, m_pnBuffer, m_nBufSize );
		m_nBufSize = nNewSize;
		delete[]tmp;
		m_pnBuffer = tmp;
	}
	Messenger m_cMsgTarget;
	Message m_cMessage;
	DataReceiver *m_pcDataTarget;
	bool m_bSendData;

//    std::vector<uint8> m_cBuffer;
	uint8 *m_pnBuffer;
	uint32 m_nBufSize;
};




Translator::Translator()
{
	m = new Internal;
}

Translator::~Translator()
{
	delete m;
}

void Translator::SetMessage( const Message & cMsg )
{
	m->m_cMessage = cMsg;
}

void Translator::SetTarget( const Messenger & cTarget, bool bSendData )
{
	m->m_cMsgTarget = cTarget;
	m->m_bSendData = bSendData;
}

void Translator::SetTarget( DataReceiver * pcTarget )
{
	m->m_pcDataTarget = pcTarget;
}

status_t Translator::AddData( const void *pData, size_t nLen, bool bFinal )
{
//    size_t nOldSize = m->m_cBuffer.size();
	size_t nOldSize = m->m_nBufSize;

	m->ResizeBuffer( nOldSize + nLen );
//    m->m_cBuffer.resize( nOldSize + nLen );
//    memcpy( (void*)(m->m_cBuffer.begin() + nOldSize), pData, nLen );
	memcpy( m->m_pnBuffer + nOldSize, pData, nLen );

	return ( DataAdded( m->m_pnBuffer, m->m_nBufSize, bFinal ) );
//    return( DataAdded( m->m_cBuffer.c_str(), m->m_cBuffer.size(), bFinal ) );
}

status_t Translator::DataAdded( void *pData, size_t nLen, bool bFinal )
{
	return ( 0 );
}

void Translator::Invoke( void *pData, size_t nSize, bool bFinal )
{
	if( m->m_cMsgTarget.IsValid() )
	{
		Message cMsg( m->m_cMessage );

		cMsg.AddInt32( "size", nSize );
		cMsg.AddBool( "final", bFinal );

		if( m->m_bSendData )
		{
			cMsg.AddData( "data", T_ANY_TYPE, pData, nSize );
		}
		m->m_cMsgTarget.SendMessage( &cMsg );
	}
	if( m->m_pcDataTarget != NULL )
	{
		m->m_pcDataTarget->DataReady( pData, nSize, bFinal );
	}
}


TranslatorNode::TranslatorNode()
{
	m = NULL;
}

TranslatorNode::~TranslatorNode()
{
}

