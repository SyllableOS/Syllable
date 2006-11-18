/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 2004 Syllable Team
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

#include <util/catalog.h>
#include <util/application.h>
#include <gui/exceptions.h>
#include <map>

using namespace os;

#define CURRENT_CATALOG_VERSION	2
#define CATALOG_MAGIC			0x0CA7A106

struct Catalog::FileHeader
{
	uint32	m_nMagic;
	uint32	m_nVersion;
	uint32	m_nHeaderSize;
	uint32	m_nNumStrings;
	uint32	m_nNumMnemonics;
	uint32	m_nNumComments;
};

class Catalog::Private
{
	public:

	Private():m_cLocker("catalog-lock") {
	}

	~Private() {
	}

	FileHeader	m_sHdr;
	StringMap	m_cStrings;
	mutable Locker m_cLocker;
};

Catalog::Catalog()
{
	m = new Private;
}

Catalog::Catalog( StreamableIO* pcFile )
{
	m = new Private;

	Load( pcFile );
}

Catalog::Catalog( String& cName, Locale *pcLocale )
{
	m = new Private;
}

Catalog::~Catalog()
{
	delete m;
}

const String& Catalog::GetString( uint32 nID ) const
{
	Lock();

	const_iterator i = m->m_cStrings.find( nID );

	if( i == m->m_cStrings.end() ) {
		m->m_cStrings[ nID ] = "(?)";
	}

	const String& cStr = m->m_cStrings[ nID ];
	Unlock();
	return cStr;
}

const String& Catalog::GetString( uint32 nID, const String& cDefault ) const
{
	Lock();

	const_iterator i = m->m_cStrings.find( nID );

	if( i == m->m_cStrings.end() ) {
		m->m_cStrings[ nID ] = cDefault;
	}

	const String& cStr = m->m_cStrings[ nID ];
	Unlock();
	return cStr;
}

void Catalog::SetString( uint32 nID, const String& cStr ) const
{
	Lock();
	m->m_cStrings[ nID ] = cStr;
	Unlock();
}

status_t Catalog::Load( StreamableIO* pcSource )
{
	status_t retval = 0;

	Lock();

	pcSource->Read( &m->m_sHdr, 3 * sizeof( uint32 ) );
	pcSource->Read( &m->m_sHdr.m_nNumStrings, std::min( (uint32)(sizeof( m->m_sHdr ) ), m->m_sHdr.m_nHeaderSize ) - 3 * sizeof( uint32 ) );
	
	if(	m->m_sHdr.m_nMagic == CATALOG_MAGIC &&
		m->m_sHdr.m_nVersion <= CURRENT_CATALOG_VERSION )
	{
		uint32	i;
		uint32	nID;
		uint32	nLen;
		char*	pzBfr = new char[ 256 ];
		uint32	nBufSize = 256;

		if( m->m_sHdr.m_nVersion == 1 ) {
			m->m_sHdr.m_nNumMnemonics = 0;
			m->m_sHdr.m_nNumComments = 0;
		}

		for( i = 0; i < m->m_sHdr.m_nNumStrings; i++ ) {
			pcSource->Read( &nID, sizeof( nID ) );
			pcSource->Read( &nLen, sizeof( nLen ) );
			if( nLen > nBufSize ) {
				nBufSize = nLen + 100;
				delete pzBfr;
				pzBfr = new char[ nBufSize ];
			}
			pcSource->Read( pzBfr, nLen );
			m->m_cStrings[ nID ] = os::String( pzBfr );
		}
	
		delete pzBfr;
	} else {
		retval = -1;
	}
	
	Unlock();

	return retval;
}

status_t Catalog::Save( StreamableIO* pcDest )
{
	Lock();

	m->m_sHdr.m_nMagic = CATALOG_MAGIC;
	m->m_sHdr.m_nVersion = CURRENT_CATALOG_VERSION;
	m->m_sHdr.m_nHeaderSize = sizeof( m->m_sHdr );
	m->m_sHdr.m_nNumStrings = m->m_cStrings.size();
	m->m_sHdr.m_nNumMnemonics = 0;
	m->m_sHdr.m_nNumComments = 0;
	pcDest->Write( &m->m_sHdr, sizeof( m->m_sHdr ) );

	StringMap::iterator i;
	
	for(i = m->m_cStrings.begin(); i != m->m_cStrings.end(); i++) {
		uint32	nID;
		uint32	nLen;
		
		nID = (*i).first;
		nLen = (*i).second.size() + 1;
		
		pcDest->Write( &nID, sizeof( nID ) );
		pcDest->Write( &nLen, sizeof( nLen ) );
		pcDest->Write( (*i).second.c_str(), nLen );
	}
	
	Unlock();
	
	return 0;
}

Catalog::const_iterator Catalog::begin() const
{
	return m->m_cStrings.begin();
}

Catalog::const_iterator Catalog::end() const
{
	return m->m_cStrings.end();
}

int Catalog::Lock() const
{
	return m->m_cLocker.Lock();
}

int Catalog::Unlock() const
{
	return m->m_cLocker.Unlock();
}


LString::LString( uint32 nID )
{
	const Catalog* pcCat = Application::GetInstance()->GetCatalog();

	if( pcCat ) {
		String::operator=( pcCat->GetString( nID ) );
	} else {
		throw GeneralFailure( "Catalog not loaded!", 0 );
	}
}


