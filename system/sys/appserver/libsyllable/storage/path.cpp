
/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999 - 2001  Kurt Skauen
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

Path::Path()
{
	m_pzPath = NULL;
	m_nMaxSize = 0;
}

Path::Path( const Path & cPath )
{
	m_pzPath = NULL;
	m_nMaxSize = 0;
	SetTo( cPath.m_pzPath );
}

Path::Path( const char *pzPath )
{
	m_pzPath = NULL;
	m_nMaxSize = 0;
	SetTo( pzPath );
}

Path::Path( const char *pzPath, int nLen )
{
	m_pzPath = NULL;
	m_nMaxSize = 0;
	SetTo( pzPath, nLen );
}

Path::~Path()
{
	delete[]m_pzPath;
	m_pzPath = NULL;
}

void Path::operator =( const Path & cPath )
{
	SetTo( cPath.m_pzPath );
}

void Path::operator =( const char *pzPath )
{
	SetTo( pzPath );
}

bool Path::operator==( const Path & cPath ) const
{
	return ( strcmp( m_pzPath, cPath.m_pzPath ) == 0 );
}

void Path::SetTo( const char *pzPath )
{
	SetTo( pzPath, strlen( pzPath ) );
}

void Path::SetTo( const char *pzPath, int nLen )
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

	if( nMaxSize > m_nMaxSize )
	{
		delete[]m_pzPath;
		m_pzPath = new char[nMaxSize];

		m_nMaxSize = nMaxSize;
	}
	strcpy( m_pzPath, zCWD );
	memcpy( m_pzPath + strlen( zCWD ), pzPath, nLen );
	( m_pzPath + strlen( zCWD ) + nLen )[0] = '\0';

	assert( strlen( m_pzPath ) < m_nMaxSize );
	_Normalize();
	assert( strlen( m_pzPath ) < m_nMaxSize );
}

void Path::Append( const char *pzPath )
{
	if( pzPath == NULL )
	{
		return;
	}
	uint nNewLen = strlen( m_pzPath ) + strlen( pzPath ) + 2;

	if( nNewLen > m_nMaxSize )
	{
		char *pzNewBuf = new char[nNewLen];

		strcpy( pzNewBuf, m_pzPath );
		delete[]m_pzPath;
		m_pzPath = pzNewBuf;
		m_nMaxSize = nNewLen;
	}
	strcat( m_pzPath, "/" );
	strcat( m_pzPath, pzPath );
	assert( strlen( m_pzPath ) < m_nMaxSize );
	_Normalize();
	assert( strlen( m_pzPath ) < m_nMaxSize );
}

void Path::Append( const Path & cPath )
{
	Append( cPath.m_pzPath );
}

const char *Path::GetLeaf() const
{
	char *pzLeaf = strrchr( m_pzPath, '/' );

	if( pzLeaf == NULL )
	{
		pzLeaf = m_pzPath;
	}
	return ( pzLeaf + 1 );
}

Path Path::GetDir() const
{
	char *pzLeaf = strrchr( m_pzPath, '/' );

	if( pzLeaf == NULL )
	{
		return ( Path( "." ) );
	}
	else if( pzLeaf == m_pzPath )
	{
		return ( "/" );
	}
	else
	{
		return ( Path( m_pzPath, pzLeaf - m_pzPath ) );
	}
}

const char *Path::GetPath() const
{
	return ( m_pzPath );
}

void Path::_Normalize()
{
	int i;
	char *pzPath = m_pzPath;
	char *pzDst = m_pzPath;
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

Path::operator  std::string() const
{
	if( m_pzPath == NULL )
	{
		return ( "" );
	}
	else
	{
		return ( m_pzPath );
	}
}
