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

#include <util/locale.h>
#include <util/resources.h>
#include <util/settings.h>
#include <storage/file.h>
#include <storage/path.h>
#include <locale.h>
#include <util/catalog.h>

#include <vector>

using namespace os;

class Locale::Private
{
	public:

	Private() {
		m_pcCol = NULL;
		m_pcSystemResource = NULL;

		Init();
	}
	
	void Init() {
		try {
			char* pzHome;
			pzHome = getenv( "HOME" );

			File* pcFile = new File( String( pzHome ) + String( "/Settings/System/Locale" ) );
			Settings cSettings( pcFile );
			cSettings.Load( );
			
			int i;
			int nCount = 0, nType;
			cSettings.GetNameInfo( "LANG", &nType, &nCount );
			for( i = 0; i < nCount; i++ ) {
				String cString;
				if( cSettings.FindString( "LANG", &cString, i ) == 0 ) {
					m_cLang.push_back( cString );
					dbprintf( "Preferred Language: %s (prio %d)\n", cString.c_str(), i );
				}
			}
		} catch( ... ) {
		}
	}

	~Private() {
		Clear();
	}
	
	void Clear() {
		delete m_pcCol;
		delete m_pcSystemResource;
	}
	
	Resources* GetResources() {
		if( !m_pcCol ) {
			m_pcCol = new Resources( 0 );	// Get resources from the main executable, wich is always ID = 0
		}
		return m_pcCol;
	}

	Resources* GetSystemResources() {
		if( !m_pcCol ) {
//			m_pcCol = new Resources( get_image_id() );
// get_image_id() does not work as intended. Workaround follows...
			void* x;
			int i;
			int id = 0;
			image_info psInfo;
			x = __builtin_return_address( 0 );
			for( i = 0; i < 300; i++ ) {
				if( get_image_info( i, 0, &psInfo ) != -1 ) {
					if( (uint32)(x) >= (uint32)(psInfo.ii_text_addr) && (uint32)(x) <= (uint32)(psInfo.ii_text_addr) + psInfo.ii_text_size ) {
						id = psInfo.ii_image_id;
						break;
					}
				} else {
					break;
				}
			}
			m_pcCol = new Resources( id );
		}
		return m_pcCol;
	}

	std::vector<String> m_cLang;
	String		m_cName;
	Resources*	m_pcCol;
	Resources*	m_pcSystemResource;
};

Locale::Locale( )
{
	m = new Private;
	
	char *lang = getenv( "LANG" );
	
	if( lang ) {
		char *p = lang;
		char bfr[128], *q;
		uint i;
		for( i = 0, q = bfr; *p && *p != '_' && i < sizeof(bfr); p++, i++, q++ ) {
			*q = *p;
		}
		*q = '\0';
		SetName( bfr );
	} else {
		SetName( "C" );
	}
}

Locale::Locale( int nCategory )
{
	m = new Private;
	
	SetName( setlocale( nCategory, NULL ) );
}

Locale::Locale( const Locale& cLocale )
{
	m = new Private;
	SetName( cLocale.GetName() );
}

const Locale& Locale::operator=( const Locale& cLocale )
{
	m->Clear();
	m->Init();
	SetName( cLocale.GetName() );
}

Locale::~Locale()
{
	delete m;
}

const String& Locale::GetName() const
{
	return m->m_cName;
}

void Locale::SetName( const String& cName )
{
	m->m_cName = cName;
}

StreamableIO* Locale::GetResourceStream( const String& cName )
{
	Resources* pcCol = NULL;
	StreamableIO* pcStream = NULL;

	try {
		pcCol = m->GetResources();
	} catch( ... ) {
		// eat "could not find resource section" exceptions
	}

	if( pcCol ) {
		pcStream = pcCol->GetResourceStream( cName );
	}

	if( !pcStream ) {
		try {
			pcStream = new File( String("^/resources/") + cName );
		} catch( ... ) {
			if( pcStream ) delete pcStream;
			pcStream = NULL;
		}
	}

	return pcStream;
}

StreamableIO* Locale::GetSystemResourceStream( const String& cName )
{
	Resources* pcCol = NULL;
	StreamableIO* pcStream = NULL;

	try {
		pcCol = m->GetSystemResources();
	} catch( ... ) {
		// eat "could not find resource section" exceptions
	}

	if( pcCol ) {
		pcStream = pcCol->GetResourceStream( cName );
	}

	if( !pcStream ) {
		try {
			pcStream = new File( String("/system/resources/") + cName );
		} catch( ... ) {
			if( pcStream ) delete pcStream;
			pcStream = NULL;
		}
	}

	return pcStream;
}

StreamableIO* Locale::GetLocalizedResourceStream( const String& cName )
{
	StreamableIO* pcStream;

	pcStream = GetResourceStream( m->m_cName + String("/") + cName );
	
	if( pcStream == NULL ) {
		pcStream = GetResourceStream( cName );
	}

	return pcStream;
}

StreamableIO* Locale::GetLocalizedSystemResourceStream( const String& cName )
{
	StreamableIO* pcStream;

	pcStream = GetResourceStream( m->m_cName + String("/") + cName );
	
	if( pcStream == NULL ) {
		pcStream = GetResourceStream( cName );
	}

	return pcStream;
}

Catalog* Locale::GetLocalizedCatalog( const String& cName )
{
	StreamableIO* pcStream;
	Catalog* pcCatalog = new Catalog;
	bool bDef = false, bLoc = false;

	pcStream = GetResourceStream( cName );
	if( pcStream ) {
		pcCatalog->Load( pcStream );
		delete pcStream;
		bDef = true;
	}

	for( size_t i = 0; i < m->m_cLang.size(); i++ ) {
		pcStream = GetResourceStream( m->m_cLang[i] + String("/") + cName );
		if( pcStream ) {
			pcCatalog->Load( pcStream );
			delete pcStream;
			bLoc = true;
			break;
		}
	}

	if( bLoc || bDef ) {
		return pcCatalog;
	} else {
		delete pcCatalog;
		return NULL;
	}
}

Catalog* Locale::GetLocalizedSystemCatalog( const String& cName )
{
	StreamableIO* pcStream;
	Catalog* pcCatalog = new Catalog;
	bool bDef = false, bLoc = false;

	pcStream = GetSystemResourceStream( os::String( "catalogs/" ) + cName );
	if( pcStream ) {
		pcCatalog->Load( pcStream );
		delete pcStream;
		bDef = true;
	}

	for( size_t i = 0; i < m->m_cLang.size(); i++ ) {
		pcStream = GetSystemResourceStream( String("catalogs/") + m->m_cLang[i] + String("/") + cName );
		if( pcStream ) {
			pcCatalog->Load( pcStream );
			delete pcStream;
			bLoc = true;
			break;
		}
	}

	if( bLoc || bDef ) {
		return pcCatalog;
	} else {
		delete pcCatalog;
		return NULL;
	}
}

