/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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

#ifndef	GUI_ARRAY_HPP
#define	GUI_ARRAY_HPP

#include <atheos/types.h>
#include <sys/types.h>

template<class T> class	Array
{
public:
    Array();
    ~Array();

    int	Insert( T* pcObject );
    void	Remove( int nIndex );

    T*	GetObj( uint nID ) const	{ return( (*this)[ nID ] ); }
    T*	operator[] ( uint nID ) const;

    int	GetTabCount( void )	const	{ return( m_nTabCount );	}
    int	GetMaxCount( void )	const	{ return( m_nMaxCount );	}
    int	GetAvgCount( void )	const	{ return( m_nAvgCount );	}

private:
    void	IncCount( void );
    void	DecCount( void );

//	enum	{ PRIM_SIZE = 512, SEC_SIZE = 512 };

    int	m_nLastID;

    int	m_nTabCount;
    int	m_nMaxCount;
    int	m_nAvgCount;

    T*			m_pcMagic[ 256 ];
    T***		m_pArray[ 256 ];
};

template<class T>
void Array<T>::IncCount()
{
    m_nTabCount++;

    m_nAvgCount = (m_nAvgCount + m_nTabCount) / 2;

    if ( m_nTabCount > m_nMaxCount ) {
	m_nMaxCount = m_nTabCount;
    }
}

template<class T>
void Array<T>::DecCount()
{
    m_nTabCount--;

    m_nAvgCount = (m_nAvgCount + m_nTabCount) / 2;
}


template<class T>
Array<T>::Array()
{
    m_nLastID	= 0;

    m_nTabCount	=	0;
    m_nMaxCount	=	0;
    m_nAvgCount	=	0;

    for ( int i = 0 ; i < 256 ; ++i )	{
	m_pArray[ i ] = (T***) m_pcMagic;
	m_pcMagic[ i ] = (T*) m_pcMagic;
    }
}

template<class T>
Array<T>::~Array()
{
}

template<class T>
int	Array<T>::Insert( T* pcObject )
{
/*	
	kassertw( pcObject != (T*) m_pcMagic );
	kassertw( NULL != pcObject );
	*/
    for(;;)
    {
	int	nIndex	=	(m_nLastID++) & 0xffffff;
	T**	pcObj = m_pArray[ (nIndex >> 16) & 0xff ][ (nIndex >> 8) & 0xff ];

	if ( pcObj == m_pcMagic )
	{
	    if ( m_pArray[ (nIndex >> 16) & 0xff ] == (T***) m_pcMagic )
	    {
		int	i;

		m_pArray[ (nIndex >> 16) & 0xff ] = new T**[ 256 ];
		IncCount();

		for ( i = 0 ; i < 256 ; ++i ) {
		    m_pArray[ (nIndex >> 16) & 0xff ][i] = m_pcMagic;
		}
		m_pArray[ (nIndex >> 16) & 0xff ][(nIndex >> 8) & 0xff] = new T*[ 256 ];
		IncCount();

		for ( i = 0 ; i < 256 ; ++i ) {
		    m_pArray[ (nIndex >> 16) & 0xff ][ (nIndex >> 8) & 0xff ][i] = (T*) m_pcMagic;
		}
	    }
	    else
	    {
		int	i;

		m_pArray[ (nIndex >> 16) & 0xff ][(nIndex >> 8) & 0xff] = new T*[ 256 ];
		IncCount();

		for ( i = 0 ; i < 256 ; ++i ) {
		    m_pArray[ (nIndex >> 16) & 0xff ][ (nIndex >> 8) & 0xff ][i] = (T*) m_pcMagic;
		}
	    }
	    m_pArray[ (nIndex >> 16) & 0xff ][(nIndex >> 8) & 0xff][ nIndex & 0xff ] = pcObject;
	    return( nIndex );
	}
	else
	{
	    if ( pcObj[ nIndex & 0xff ] != (T*) m_pcMagic ) {
/*				dbprintf( "WARNING : Array::Insert() index already used, skipping %lx\n", nIndex ); */
		continue;
	    }

	    pcObj[ nIndex & 0xff ] = pcObject;
	    return( nIndex );
	}
    }
}

template<class T>
void	Array<T>::Remove( int nIndex )
{
    int	i;

    if ( m_pArray[ (nIndex >> 16) & 0xff ][ (nIndex >> 8) & 0xff ][ nIndex & 0xff ] == (T*) m_pcMagic )
    {
/*		dbprintf( "ERROR : Array::Remove() Attempt to release empty entry %lx\n", nIndex ); */
	return;
    }

    m_pArray[ (nIndex >> 16) & 0xff ][ (nIndex >> 8) & 0xff ][ nIndex & 0xff ] = (T*) m_pcMagic;

    for ( i = 0 ; i < 256 ; ++i )
    {
	if ( m_pArray[ (nIndex >> 16) & 0xff ][ (nIndex >> 8) & 0xff ][i] != (T*)m_pcMagic ) {
	    return;
	}
    }

    delete[] m_pArray[ (nIndex >> 16) & 0xff ][ (nIndex >> 8) & 0xff ];
    DecCount();



    m_pArray[ (nIndex >> 16) & 0xff ][ (nIndex >> 8) & 0xff ] = m_pcMagic;


    for ( i = 0 ; i < 256 ; ++i )
    {
	if ( m_pArray[ (nIndex >> 16) & 0xff ][i] != m_pcMagic ) {
	    return;
	}
    }

    delete[] m_pArray[ (nIndex >> 16) & 0xff ];
    DecCount();


    m_pArray[ (nIndex >> 16) & 0xff ] = (T***)m_pcMagic;
}


template<class T>
T*	Array<T>::operator[] ( uint nIndex ) const
{
    T*	pcObj = m_pArray[ (nIndex >> 16) & 0xff ][ (nIndex >> 8) & 0xff ][ nIndex & 0xff ];

    if ( pcObj == (T*) m_pcMagic ) {
	return( NULL );
    }
    return( pcObj );
}

#endif	//	GUI_ARRAY_HPP
