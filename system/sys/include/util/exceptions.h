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

#ifndef __UTIL_EXCEPTIONS_H__
#define __UTIL_EXCEPTIONS_H__

#include <string.h>
#include <errno.h>
#include <util/string.h>

#include <exception>

namespace os {
#if 0
} // Fool Emacs autoindent
#endif

class errno_exception : public std::exception
{
public:
    errno_exception( const String& cMessage, int nErrorCode ) : m_cMessage(cMessage) {
	m_nErrorCode = nErrorCode;
    }
    errno_exception( const String& cMessage ) : m_cMessage(cMessage) {
	m_nErrorCode = errno;
    }
    virtual ~errno_exception() throw() {
		//
    }

    virtual const char* what () const throw() { return( m_cMessage.c_str() ); }
    char* error_str() const { return( strerror( m_nErrorCode ) ); }
    int error() const { return( m_nErrorCode ); }
private:
    String m_cMessage;
    int		m_nErrorCode;
};


} // end of namespace

#endif // __UTIL_EXCEPTIONS_H__
