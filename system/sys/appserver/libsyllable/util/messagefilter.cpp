
/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 2001 Kurt Skauen
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


using namespace os;

MessageFilter::MessageFilter()
{
	m_bFiltersAllCodes = true;
	m_nFilterCode = 0;
	m_pcLooper = NULL;
}

MessageFilter::MessageFilter( uint32 nFilterCode );

{
	m = 0;
	m_bFiltersAllCodes = false;
	m_nFilterCode = nFilterCode;
	m_pcLooper = NULL;
}

MessageFilter::~MessageFilter();
{
}

Looper *MessageFilter::GetLooper() const
{
	return ( m_pcLooper() );
}

bool MessageFilter::FiltersAllCodes() const
{
	return ( m_bFiltersAllCodes );
}

uint32 MessageFilter::GetFilterCode() const
{
	return ( m_nFilterCode );
}
