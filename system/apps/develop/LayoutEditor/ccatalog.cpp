
#include <util/catalog.h>
#include <util/application.h>
#include <gui/exceptions.h>
#include <map>
#include "ccatalog.h"

using namespace os;

#define CURRENT_CATALOG_VERSION	2
#define CATALOG_MAGIC			0x0CA7A106

struct CCatalog::FileHeader
{
	uint32	m_nMagic;
	uint32	m_nVersion;
	uint32	m_nHeaderSize;
	uint32	m_nNumStrings;
	uint32	m_nNumMnemonics;
	uint32	m_nNumComments;
};

class CCatalog::Private
{
	public:

	Private():m_cLocker("ccatalog-lock") {
	}

	~Private() {
	}

	FileHeader	m_sHdr;
	StringMap	m_cStrings;
	StringMap	m_cMnemonics;
	StringMap	m_cComments;
	mutable Locker m_cLocker;
};

CCatalog::CCatalog()
{
	m = new Private;
}

CCatalog::CCatalog( StreamableIO* pcFile )
{
	m = new Private;

	Load( pcFile );
}

CCatalog::CCatalog( String& cName, Locale *pcLocale )
{
	m = new Private;
}

CCatalog::~CCatalog()
{
	delete m;
}

const String& CCatalog::GetString( uint32 nID ) const
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

void CCatalog::SetString( uint32 nID, const String& cStr ) const
{
	Lock();
	m->m_cStrings[ nID ] = cStr;
	Unlock();
}

const String CCatalog::GetComment( uint32 nID ) const
{
	Lock();

	const_iterator i = m->m_cComments.find( nID );

	String cStr = "";

	if( i != m->m_cComments.end() ) {
		 cStr = m->m_cComments[ nID ];
	}

	Unlock();
	return cStr;
}

void CCatalog::SetComment( uint32 nID, const String& cStr ) const
{
	Lock();
	m->m_cComments[ nID ] = cStr;
	Unlock();
}

const String CCatalog::GetMnemonic( uint32 nID ) const
{
	Lock();

	const_iterator i = m->m_cMnemonics.find( nID );

	String cStr = "";

	if( i != m->m_cMnemonics.end() ) {
		cStr = m->m_cMnemonics[ nID ];
	}

	Unlock();
	return cStr;
}

void CCatalog::SetMnemonic( uint32 nID, const String& cStr ) const
{
	Lock();
	m->m_cMnemonics[ nID ] = cStr;
	Unlock();
}

status_t CCatalog::Load( StreamableIO* pcSource )
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
			SetString( nID, pzBfr );
		}

		for( i = 0; i < m->m_sHdr.m_nNumMnemonics; i++ ) {
			pcSource->Read( &nID, sizeof( nID ) );
			pcSource->Read( &nLen, sizeof( nLen ) );
			if( nLen > nBufSize ) {
				nBufSize = nLen + 100;
				delete pzBfr;
				pzBfr = new char[ nBufSize ];
			}
			pcSource->Read( pzBfr, nLen );
			SetMnemonic( nID, pzBfr );
		}

		for( i = 0; i < m->m_sHdr.m_nNumComments; i++ ) {
			pcSource->Read( &nID, sizeof( nID ) );
			pcSource->Read( &nLen, sizeof( nLen ) );
			if( nLen > nBufSize ) {
				nBufSize = nLen + 100;
				delete pzBfr;
				pzBfr = new char[ nBufSize ];
			}
			pcSource->Read( pzBfr, nLen );
			SetComment( nID, pzBfr );
		}
	
		delete pzBfr;
	} else {
		retval = -1;
	}
	
	Unlock();

	return retval;
}

status_t CCatalog::Save( StreamableIO* pcDest )
{
	Lock();

	m->m_sHdr.m_nMagic = CATALOG_MAGIC;
	m->m_sHdr.m_nVersion = CURRENT_CATALOG_VERSION;
	m->m_sHdr.m_nHeaderSize = sizeof( m->m_sHdr );
	m->m_sHdr.m_nNumStrings = m->m_cStrings.size();
	m->m_sHdr.m_nNumMnemonics = m->m_cMnemonics.size();
	m->m_sHdr.m_nNumComments = m->m_cComments.size();
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

	for(i = m->m_cMnemonics.begin(); i != m->m_cMnemonics.end(); i++) {
		uint32	nID;
		uint32	nLen;
		
		nID = (*i).first;
		nLen = (*i).second.size() + 1;
		
		pcDest->Write( &nID, sizeof( nID ) );
		pcDest->Write( &nLen, sizeof( nLen ) );
		pcDest->Write( (*i).second.c_str(), nLen );
	}

	for(i = m->m_cComments.begin(); i != m->m_cComments.end(); i++) {
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

CCatalog::const_iterator CCatalog::begin() const
{
	return m->m_cStrings.begin();
}

CCatalog::const_iterator CCatalog::end() const
{
	return m->m_cStrings.end();
}

int CCatalog::Lock() const
{
	return m->m_cLocker.Lock();
}

int CCatalog::Unlock() const
{
	return m->m_cLocker.Unlock();
}

/* Copied from catcomp */
bool CCatalog::ParseCatalogDescription( const char *pzFileName )
{
	int i;
	FILE *fp;
	char bfr[ 1024 ], *p;
	char ident[ 256 ], *q;
	char string[ 1024 ];
	char temp[ 256 ];
	int line = 0;
	uint32 nID = 0;
	bool ret = true;
		
	fp = fopen( pzFileName, "r" );
	if( fp ) {
		while( ! feof( fp ) ) {
			p = fgets( bfr, sizeof(bfr), fp );
			if( p ) {
				line++;
				while( *p && isspace(*p) ) p++;
				if( *p == '/' ) {
					// comment
				} else if( *p ) {
					q = ident;
					while( *p && !isspace( *p ) && *p != '=' ) {
						*q++ = *p++;
					}
					*q = 0;
					while( *p && isspace(*p) ) p++;
					if( *p == '=' ) {
						p++;
						while( *p && isspace(*p) ) p++;
						if( *p ) {
							q = temp;
							while( *p && !isspace( *p ) ) {
								*q++ = *p++;
							}
							*q = 0;
							nID = atol( temp );
							while( *p && isspace(*p) ) p++;
						} else {
							printf( "Catalog syntax error: message id missing\n" );
							ret = false;
						}
					}
					if( *p ) {
						q = string;
						while( *p && *p != '\n' && *p != '\r' ) {
							if( *p == '\\' ) {	// Handle escape sequences
								p++;
								switch( *p ) {
									case 'n':
										*q++ = '\n';
										p++;
										break;
									case 't':
										*q++ = '\t';
										p++;
										break;
									case '\n':
										p++;
										while( *p && isspace(*p) ) p++;
										*q++ = ' ';
										break;
									default:
										*q++ = *p++;
								}
							} else {
								*q++ = *p++;
							}
						}
						*q = 0;
						
						// Remove spaces from the end of the string
						for( i = strlen( string ); i>0; i-- ) {
							if( isspace( string[i] ) ) {
								string[i] = '\0';
							} else {
								break;
							}
						}
						
				//		cout << ident << "=" << string << " (" << nID << ")" << endl;
						SetString( nID, string );
						SetMnemonic( nID, ident );
						nID++;
					} else {
						printf( "Catalog syntax error: no text string\n" );						
						ret = false;
					}
				}
			}
		}
		fclose( fp );
	} else {
		printf( "Can't open catalog file %s!\n", pzFileName );
		ret = false;
	}

	return ret;
}



