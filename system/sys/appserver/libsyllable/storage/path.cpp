/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 - 2004 Syllable Team
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

#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <storage/path.h>

using namespace os;

class Path::Private
{
	public:
	Private() {
		m_pzPath = NULL;
		m_nMaxSize = 0;
	}
	~Private() {
		delete[]m_pzPath;
		m_pzPath = NULL;
	}
//    String m_cBuffer;
    char*	m_pzPath;
    uint	m_nMaxSize;
};

Path::Path()
{
	m = new Private;
}

Path::Path( const Path & cPath )
{
	m = new Private;
	SetTo( cPath.m->m_pzPath );
}

Path::Path( const String& cPath )
{
	m = new Private;
	SetTo( cPath );
}

Path::~Path()
{
	delete m;
}

void Path::operator =( const Path & cPath )
{
	SetTo( cPath.m->m_pzPath );
}

void Path::operator =( const String& cPath )
{
	SetTo( cPath );
}

bool Path::operator==( const Path & cPath ) const
{
	return ( strcmp( m->m_pzPath, cPath.m->m_pzPath ) == 0 );
}

void Path::SetTo( const String& cPath )
{
	_SetTo( cPath.c_str(), cPath.size() );
}

void Path::_SetTo( const char *pzPath, int nLen )
{
	char zCWD[4096];

	uint nMaxSize = nLen;

	if( pzPath[0] != '/' )
	{
		getcwd( zCWD, 4095 );
		strcat( zCWD, "/" );
		nMaxSize += strlen( zCWD );
	}
	else
	{
		zCWD[0] = '/';
		zCWD[1] = '\0';
		nMaxSize++;
	}
	nMaxSize++;		// Zero terminator

	if( nMaxSize > m->m_nMaxSize )
	{
		delete[]m->m_pzPath;
		m->m_pzPath = new char[nMaxSize];

		m->m_nMaxSize = nMaxSize;
	}
	strcpy( m->m_pzPath, zCWD );
	memcpy( m->m_pzPath + strlen( zCWD ), pzPath, nLen );
	( m->m_pzPath + strlen( zCWD ) + nLen )[0] = '\0';

	assert( strlen( m->m_pzPath ) < m->m_nMaxSize );
	_Normalize();
	assert( strlen( m->m_pzPath ) < m->m_nMaxSize );
}

void Path::Append( const String& cPath )
{
	const char *pzPath = cPath.c_str();
	uint nNewLen = strlen( m->m_pzPath ) + cPath.size() + 2;

	if( nNewLen > m->m_nMaxSize )
	{
		char *pzNewBuf = new char[nNewLen];

		strcpy( pzNewBuf, m->m_pzPath );
		delete[]m->m_pzPath;
		m->m_pzPath = pzNewBuf;
		m->m_nMaxSize = nNewLen;
	}
	strcat( m->m_pzPath, "/" );
	strcat( m->m_pzPath, pzPath );
	assert( strlen( m->m_pzPath ) < m->m_nMaxSize );
	_Normalize();
	assert( strlen( m->m_pzPath ) < m->m_nMaxSize );
}

void Path::Append( const Path & cPath )
{
	Append( cPath.m->m_pzPath );
}

String Path::GetLeaf() const
{
	char *pzLeaf = strrchr( m->m_pzPath, '/' );

	if( pzLeaf == NULL )
	{
		pzLeaf = m->m_pzPath;
	}
	return ( pzLeaf + 1 );
}

Path Path::GetDir() const
{
	char *pzLeaf = strrchr( m->m_pzPath, '/' );

	if( pzLeaf == NULL )
	{
		return ( Path( "." ) );
	}
	else if( pzLeaf == m->m_pzPath )
	{
		return ( Path( "/" ) );
	}
	else
	{
		return ( Path( String( m->m_pzPath, pzLeaf - m->m_pzPath ) ) );
	}
}

String Path::GetPath() const
{
	return ( m->m_pzPath );
}

void Path::_Normalize()
{
	int i;
	char *pzPath = m->m_pzPath;
	char *pzDst = m->m_pzPath;
	char *pzCurName;
	char c;
	int nLen;

	if( '/' == pzPath[0] )
	{
		pzDst++;
		pzPath++;
	}

	pzCurName = pzPath;

	for( i = 0, nLen = 0; ( c = pzPath[i] ); ++i )
	{
		if( '/' == c )
		{
			if( 0 != nLen || ( 1 == nLen && c == '.' ) )
			{
				if( nLen == 2 && pzCurName[0] == '.' && pzCurName[1] == '.' )
				{
					while( pzDst != pzPath )
					{
						pzDst--;
						if( '/' == pzDst[-1] )
						{
							break;
						}
					}
				}
				else
				{
					memcpy( pzDst, pzCurName, nLen );
					pzDst[nLen] = '/';
					pzDst += nLen + 1;
				}

				nLen = 0;
			}
			pzCurName = &pzPath[i + 1];
		}
		else
		{
			nLen++;
		}
	}

	if( nLen == 2 && pzCurName[0] == '.' && pzCurName[1] == '.' )
	{
		while( pzDst != pzPath )
		{
			pzDst--;
			if( '/' == pzDst[-1] )
			{
				break;
			}
		}
	}
	else
	{
		memcpy( pzDst, pzCurName, nLen );
		pzDst += nLen;
	}
	pzDst[0] = 0;

	nLen = strlen( pzPath ) - 1;

	while( '/' == pzPath[nLen] && nLen > 0 )
	{
		pzPath[nLen] = 0;
		nLen--;
	}
}

Path::operator  String() const
{
	if( m->m_pzPath == NULL )
	{
		return ( "" );
	}
	else
	{
		return ( m->m_pzPath );
	}
}
