/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999  Kurt Skauen
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

#ifndef __F_GUI_EXCEPTIONS_H__
#define __F_GUI_EXCEPTIONS_H__

#include <exception>

class GeneralFailure : public std::exception
{
public:
    GeneralFailure( const char* pzMessage, int nErrorCode ) {
	strncpy( m_zMessage, pzMessage, 256 ); m_zMessage[255] = '\0'; m_nErrorCode = nErrorCode;
    }
    virtual const char* what () const throw() { return( m_zMessage ); }
    int code() const { return( m_nErrorCode ); }
private:
    char	m_zMessage[256];
    int	m_nErrorCode;
};

#endif // __F_GUI_EXCEPTIONS_H__


