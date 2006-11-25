/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 2003 The Syllable Team
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

#ifndef	__F_UTIL_DATETIME_H__
#define	__F_UTIL_DATETIME_H__

#include <atheos/types.h>
#include <util/flattenable.h>
#include <util/string.h>
#include <string>

namespace os
{

/** Abstract datatype for date and time
 * \ingroup util
 * \par Description:
 *		DateTime is a storage class for date and time values. It also
 *		facilitates a number of arithmetic operations.
 * \par Examples:
 * \code
 *	DateTime cToday = DateTime::Now();
 *	DateTime cTomorrow = cToday + 24.0;
 *	DateTime cLastYear = cToday - DateTime( 1, 0, 0 );
 *	double vNumberOfDays = ( cToday - DateTime( 2003, 8, 30 ) ).GetDaycount();
 * \endcode
 *
 * \author	Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
class DateTime : public Flattenable{
	public:
	DateTime();
	DateTime( const DateTime& cDateTime );
	DateTime( const String& cTime );
	DateTime( struct tm* psTime );
	DateTime( int nTime );
	DateTime( int nYear, int nMonth, int nDay );
	DateTime( int nHour, int nMin, int nSec, int nUSec );
	~DateTime();
	
	int		GetDay() const;
	int		GetMonth() const;
	int		GetYear() const;
	int		GetHour() const;
	int		GetMin() const;
	int		GetSec() const;
	int		GetUSec() const;

	int		GetDayOfWeek() const;
	double	GetDaycount() const;
	double	GetJulianDate() const;
	time_t	GetEpoch() const;
	String	GetDate() const;
	
	void	SetTime( int nHour, int nMin, int nSec, int nUSec );
	void	SetDate( int nYear, int nMonth, int nDay );
	bool	SetDate( const String& cDate );
	
	bool	IsValid() const;

	static DateTime	Now();
	
	DateTime&	operator=( const DateTime& cTime );
	DateTime	operator+( const DateTime& cTime ) const;
	DateTime	operator+( double vDays ) const;
	DateTime	operator-( const DateTime& cTime ) const;
	DateTime	operator-( double vDays ) const;
	DateTime&	operator+=( const DateTime& cTime );
	DateTime&	operator+=( double vDays );
	DateTime&	operator-=( const DateTime& cTime );
	DateTime&	operator-=( double vDays );
	int			operator==( const DateTime& cTime ) const;
	int			operator!=( const DateTime& cTime ) const;
	int			operator<( const DateTime& cTime ) const;
	int			operator>( const DateTime& cTime ) const;
	inline		operator time_t() const;
	inline		operator std::string() const;
	inline		operator os::String() const;
	
	virtual size_t		GetFlattenedSize( void ) const;
	virtual status_t	Flatten( uint8* pBuffer, size_t nSize ) const;
	virtual status_t	Unflatten( const uint8* pBuffer );
	virtual int			GetType( void )	const;
	
	private:
	class Private;
	Private	*m;
};

DateTime::operator time_t() const
{
	return GetEpoch();
}

DateTime::operator std::string() const
{
	return GetDate().c_str();
}

DateTime::operator os::String() const
{
	return GetDate();
}

};

#endif // __F_UTIL_DATETIME_H__

