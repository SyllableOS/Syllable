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

#ifndef	_GUI_RESOURCE_HPP
#define	_GUI_RESOURCE_HPP

/** 
 * \ingroup util
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class	Resource
{
public:
    Resource()	{ m_nRefCount = 1; }

    void	AddRef( void )		{ m_nRefCount++;	}
    void	Release( void )		{ if ( --m_nRefCount == 0 ) delete this; }
    int		GetRefCount( void ) const 	{ return( m_nRefCount ); }

protected:
    virtual	~Resource()	{}
private:
    int	m_nRefCount;
};


#endif	//	_GUI_RESOURCE_HPP
