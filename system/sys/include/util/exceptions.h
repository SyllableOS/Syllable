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

#ifndef __UTIL_EXCEPTIONS_H__
#define __UTIL_EXCEPTIONS_H__

#include <string.h>
#include <errno.h>

#include <exception>

namespace os {
#if 0
} // Fool Emacs autoindent
#endif

class errno_exception : public exception
{
public:
    errno_exception( const std::string& cMessage, int nErrorCode ) : m_cMessage(cMessage) {
	m_nErrorCode = nErrorCode;
    }
    errno_exception( const std::string& cMessage ) : m_cMessage(cMessage) {
	m_nErrorCode = errno;
    }
    virtual const char* what () const { return( m_cMessage.c_str() ); }
    char* error_str() const { return( strerror( m_nErrorCode ) ); }
    int error() const { return( m_nErrorCode ); }
private:
    std::string m_cMessage;
    int		m_nErrorCode;
};


} // end of namespace

#endif // __UTIL_EXCEPTIONS_H__
