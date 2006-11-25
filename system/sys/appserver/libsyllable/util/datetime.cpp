#include <time.h>
#include <string.h>
#include <stdio.h>
#include <iostream>

#include <util/datetime.h>

using namespace os;

const static int days_elapsed[13] = {	0, 306, 337, 0, 31, 61, 92, 122, 153, 184, 214, 245, 275 };
const static int days_in_month[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };


/*this is a private struct for flattening DateTime*/
/*Please do not touch the order of this struct*/
struct FlattenStruct
{
	FlattenStruct(bool bJulian, double vTimeVal, long nYear, long nMonth, long nDay, double vSec)
	{
		m_bJulian = bJulian;
		m_vTimeVal = vTimeVal;
		m_nYear = nYear;
		m_nMonth = nMonth;
		m_nDay = nDay;
		m_vSec = vSec;
	}
	
	bool		m_bJulian;
	double		m_vTimeVal;
	long		m_nYear;
	long		m_nMonth;
	long		m_nDay;
	double		m_vSec;
};

// -----------------------------------------------------------------------------
// Private members
// -----------------------------------------------------------------------------

class DateTime::Private {

	public:
	bool			m_bJulian;
	double			m_vTimeVal;
	long			m_nYear;
	long			m_nMonth;
	long			m_nDay;
	double			m_vSec;
	
	 
	Private() {
		m_bJulian = false;
	}
	
	void Copy( Private *m )
 	{
 		m_bJulian = m->m_bJulian;
		m_vTimeVal = m->m_vTimeVal;
		m_nYear = m->m_nYear;
		m_nMonth = m->m_nMonth;
		m_nDay = m->m_nDay;
		m_vSec = m->m_vSec;
	}

	bool IsDateValid()
	{
		if( m_nMonth < 1 || m_nMonth > 12 ) return false;
		if( ( ( m_nYear % 4 == 0 && m_nYear % 100 != 0 ) || m_nYear % 400 == 0 ) && m_nMonth == 2 )
		{
			if( m_nDay < 1 || m_nDay > 29 ) return false;
		} else {
  			if( m_nDay < 1 || m_nDay > days_in_month[m_nMonth] ) return false;
		}
		return true;
	}
	
	double GetJulianDate() const
	{
 		if( m_bJulian ) return m_vTimeVal;

		double retval = (long)( ( 1461 * ( m_nYear + 4800 + ( m_nMonth - 14 ) / 12 ) ) / 4 +
 			( 367 * ( m_nMonth - 2 - 12 * ( ( m_nMonth - 14 ) / 12 ) ) ) / 12 -
	  		( 3 * ( ( m_nYear + 4900 + ( m_nMonth - 14 ) / 12 ) / 100 ) ) / 4 +
	  		m_nDay - 32075 );

	  	retval += m_vSec / 86400;
	  	
	  	return retval;
	}
	
	void ToJulian()
 	{
// 		if( m_bJulian ) return;

 		m_vTimeVal = GetJulianDate();
   
   /*(long)( ( 1461 * ( m_nYear + 4800 + ( m_nMonth - 14 ) / 12 ) ) / 4 +
 			( 367 * ( m_nMonth - 2 - 12 * ( ( m_nMonth - 14 ) / 12 ) ) ) / 12 -
	  		( 3 * ( ( m_nYear + 4900 + ( m_nMonth - 14 ) / 12 ) / 100 ) ) / 4 +
	  		m_nDay - 32075 );

	  	m_vTimeVal += m_vSec / 86400;*/

	    m_bJulian = true;
	}

	void ToGregorian() {
 		if( !m_bJulian ) return;

 		long l = ((long)m_vTimeVal) + 68569;
 		long n = ( 4 * l ) / 146097;
 		l = l - ( 146097 * n + 3 ) / 4;
 		long i = ( 4000 * ( l + 1 ) ) / 1461001;
 		l = l - ( 1461 * i ) / 4 + 31;
 		long j = ( 80 * l ) / 2447;
 		m_nDay = l - ( 2447 * j ) / 80;
 		l = j / 11;
 		m_nMonth = j + 2 - ( 12 * l );
 		m_nYear = 100 * ( n - 49 ) + i + l;

 		m_vSec = ( m_vTimeVal - ( (long)m_vTimeVal ) ) * 86400;

		m_bJulian = false;
	}
	
	struct FlattenStruct GetFlattenedStruct()
	{
		FlattenStruct m_sFlattenable(m_bJulian,m_vTimeVal,m_nYear,m_nMonth,m_nDay,m_vSec);
		return m_sFlattenable;
	}
	
	void SetFromFlattenStruct(struct FlattenStruct* m_sFlattenedStruct)
	{
		m_bJulian = m_sFlattenedStruct->m_bJulian;
		m_vTimeVal = m_sFlattenedStruct->m_vTimeVal;
		m_nYear = m_sFlattenedStruct->m_nYear;
		m_nMonth = m_sFlattenedStruct->m_nMonth;
		m_nDay = m_sFlattenedStruct->m_nDay;
		m_vSec = m_sFlattenedStruct->m_vSec;
	}

};

// -----------------------------------------------------------------------------
// Constructors
// -----------------------------------------------------------------------------

/** Default constructor
 * \par		Description:
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
DateTime::DateTime()
{
	m = new Private;
}

/** Copy constructor
 * \par		Description:
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
DateTime::DateTime( const DateTime& cDateTime)
{
	m = new Private;
	m->Copy( cDateTime.m );
}

/** Constructor: string
 * \par		Description:
 *			Creates a DateTime object from a string.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
DateTime::DateTime( const String& cDate )
{
	m = new Private;
	SetDate( cDate );
}

/** Constructor: struct tm
 * \par		Description:
 *			Creates a DateTime object from a tm struct.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
DateTime::DateTime( struct tm* psTime )
{
	m = new Private;
	m->m_nYear = psTime->tm_year;
	m->m_nMonth = psTime->tm_mon;
	m->m_nDay = psTime->tm_mday;
	m->m_vSec = psTime->tm_hour * 3600 + psTime->tm_min * 60 + psTime->tm_sec;
	m->m_bJulian = false;
}

/** Constructor: Date
 * \par		Description:
 *			Creates a DateTime object from year, month and day.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
DateTime::DateTime( int nYear, int nMonth, int nDay )
{
	m = new Private;
	m->m_nYear = nYear;
	m->m_nMonth = nMonth;
	m->m_nDay = nDay;
	m->m_vSec = 0;
	m->m_bJulian = false;
}

/** Constructor: Time
 * \par		Description:
 *			Creates a DateTime object from hour, minute, second and
 *			microsecond.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
DateTime::DateTime( int nHour, int nMin, int nSec, int nUSec )
{
	m = new Private;
	m->m_nYear = 0;
	m->m_nMonth = 0;
	m->m_nDay = 0;
	m->m_vSec = nHour * 3600 + nMin * 60 + nSec + nUSec / 1000000;
	m->m_bJulian = false;
}

/** Constructor: Unix Epoch
 * \par		Description:
 *			Creates a DateTime object from a Unix Epoch number (ie. the number
 *			of seconds since 1970-01-01 00:00:00).
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
DateTime::DateTime( int nTime )
{
	m = new Private;
	m->m_bJulian = false;
	m->m_nYear = 1970;
	m->m_nMonth = 1;
	m->m_nDay = 1;	
	m->m_vSec = nTime;
	m->ToJulian();
}

DateTime::~DateTime()
{
	delete m;
}

/** Get current date and time
 * \par		Description:
 *			Returns a DateTime object containing the current date and time.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
DateTime DateTime::Now()
{
	return DateTime( time( NULL ) );
}

// -----------------------------------------------------------------------------
// Operators
// -----------------------------------------------------------------------------

/** Assign a DateTime value
 * \par		Description:
 *			Makes an exact copy of the source object.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
DateTime& DateTime::operator=( const DateTime& cTime )
{
	m->Copy( cTime.m );
	return *this;
}

/** Add a DateTime value
 * \par		Description:
 *			Add one DateTime value to another.
 *			The result will be a valid Gregorian calendar date (months and
 *			years will be adjusted automatically), even if the operands are not
 *			valied dates.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
DateTime DateTime::operator+( const DateTime& cTime ) const
{
	DateTime cResult = *this;
	cResult += cTime;
	return cResult;
}

/** Add a number of days to date
 * \par		Description:
 *			Add a number of days, or a fraction of a day, to the DateTime
 *			value. The result is a valid Gregorian calendar date (months and
 *			years will be adjusted automatically).
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
DateTime DateTime::operator+( double vDays ) const
{
	DateTime cResult = *this;
	cResult += vDays;
	return cResult;
}

/** Subtract a DateTime value
 * \par		Description:
 *			Subtract one DateTime value from another.
 *			The result will be a valid Gregorian calendar date (months and
 *			value. The result is a valid Gregorian calendar date (months and
 *			years will be adjusted automatically), even if the operands are not
 *			valied dates.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
DateTime DateTime::operator-( const DateTime& cTime ) const
{
	DateTime cResult = *this;
	cResult -= cTime;
	return cResult;
}

/** Subtract a number of days from date
 * \par		Description:
 *			Subtract a number of days, or a fraction of a day, from the DateTime
 *			value. The result is a valid Gregorian calendar date (months and
 *			years will be adjusted automatically). 
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
DateTime DateTime::operator-( double vDays ) const
{
	DateTime cResult = *this;
	cResult -= vDays;
	return cResult;
}

/** Add a DateTime value
 * \par		Description:
 *			Add one DateTime value to another.
 *			The result will be a valid Gregorian calendar date (months and
 *			years will be adjusted automatically), even if the operands are not
 *			valied dates.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
DateTime& DateTime::operator+=( const DateTime& cTime )
{
	m->ToGregorian();
	cTime.m->ToGregorian();
	m->m_nYear += cTime.m->m_nYear;
	m->m_nMonth += cTime.m->m_nMonth;
	m->m_nDay += cTime.m->m_nDay;
	m->ToJulian();
	return *this;
}

/** Add a number of days to date
 * \par		Description:
 *			Add a number of days, or a fraction of a day, to the DateTime
 *			value. The result is a valid Gregorian calendar date (months and
 *			years will be adjusted automatically).
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
DateTime& DateTime::operator+=( double vDays )
{
	m->ToJulian();
	m->m_vTimeVal += vDays;
	return *this;
}

/** Subtract a DateTime value
 * \par		Description:
 *			Subtract one DateTime value from another.
 *			The result will be a valid Gregorian calendar date (months and
 *			value. The result is a valid Gregorian calendar date (months and
 *			years will be adjusted automatically), even if the operands are not
 *			valied dates.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
DateTime& DateTime::operator-=( const DateTime& cTime )
{
	m->ToGregorian();
	cTime.m->ToGregorian();
	m->m_nYear -= cTime.m->m_nYear;
	m->m_nMonth -= cTime.m->m_nMonth;
	m->m_nDay -= cTime.m->m_nDay;
	m->ToJulian();
	return *this;
}

/** Subtract a number of days from date
 * \par		Description:
 *			Subtract a number of days, or a fraction of a day, from the DateTime
 *			value. The result is a valid Gregorian calendar date (months and
 *			years will be adjusted automatically). 
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
DateTime& DateTime::operator-=( double vDays )
{
	m->ToJulian();
	m->m_vTimeVal -= vDays;
	return *this;
}

/** Compare dates
 * \par		Description:
 *			Dates are compared according to their Julian date equivalents. This
 *			means that 2003-08-30 13:39 is considered equal to 2003-08-29 37:39.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
int DateTime::operator==( const DateTime& cTime ) const
{
	return ( m->GetJulianDate() == cTime.m->GetJulianDate() );
}

/** Compare dates
 * \par		Description:
 *			Dates are compared according to their Julian date equivalents. This
 *			means that 2003-08-30 13:39 is considered equal to 2003-08-29 37:39.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
int DateTime::operator!=( const DateTime& cTime ) const
{
	return ( m->GetJulianDate() != cTime.m->GetJulianDate() );
}

/** Compare dates
 * \par		Description:
 *			Dates are compared according to their Julian date equivalents. This
 *			means that 2003-08-30 13:39 is considered equal to 2003-08-29 37:39.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
int DateTime::operator<( const DateTime& cTime ) const
{
	return ( m->GetJulianDate() < cTime.m->GetJulianDate() );
}

/** Compare dates
 * \par		Description:
 *			Dates are compared according to their Julian date equivalents. This
 *			means that 2003-08-30 13:39 is considered equal to 2003-08-29 37:39.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
int DateTime::operator>( const DateTime& cTime ) const
{
	return ( m->GetJulianDate() > cTime.m->GetJulianDate() );
}

// -----------------------------------------------------------------------------
// Access methods
// -----------------------------------------------------------------------------

/** Get number of days
 * \par		Description:
 *			Returns the number of days since year 0. Use this method to get
 *			the difference between two dates as a number of days.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
double DateTime::GetDaycount() const
{
	return m->GetJulianDate()-1.72106e6+32;
}

/** Get Julian date
 * \par		Description:
 *			Returns the number of days since 4712 B.C.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
double DateTime::GetJulianDate() const
{
	return m->GetJulianDate();
}

/** Get Unix Epoch
 * \par		Description:
 *			Returns the number of seconds since 1970-01-01 00:00:00.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
time_t DateTime::GetEpoch() const
{
	return (time_t)( ( m->GetJulianDate()-1.72106e6+32-719560)*24*3600+0.5 );
}

/** Check if date is valid
 * \par		Description:
 *			Returns true if the object contains a valid date. Examples of
 *			invalid dates include "2003-13-01", "2003-00-01", "2003-02-30" etc.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
bool DateTime::IsValid() const
{
	return m->IsDateValid();
}

/** Get day of month
 * \par		Description:
 *			Returns the day of month (1 - 31), or the number of days, if this
 *			DateTime object is used to represent a duration.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
int DateTime::GetDay() const
{
	m->ToGregorian();
	return m->m_nDay;
}

/** Get month of year
 * \par		Description:
 *			Returns the month of year (1 - 12), or the number of months, if this
 *			DateTime object is used to represent a duration.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
int DateTime::GetMonth() const
{
	m->ToGregorian();
	return m->m_nMonth;
}

/** Get year
 * \par		Description:
 *			Returns the year, or the number of years, if this
 *			DateTime object is used to represent a duration.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
int DateTime::GetYear() const
{
	m->ToGregorian();
	return m->m_nYear;
}

/** Get hours
 * \par		Description:
 *			Returns hours since midnight, or the number of hours if this
 *			DateTime object is used to represent a duration.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
int DateTime::GetHour() const
{
	m->ToGregorian();
	return ( (long)m->m_vSec / 3600 );
}

/** Get minutes
 * \par		Description:
 *			Returns minutes since the hour (0-59).
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
int DateTime::GetMin() const
{
	m->ToGregorian();
	return ( ( (long)m->m_vSec % 3600 ) / 60);
}

/** Get seconds
 * \par		Description:
 *			Returns seconds (0-59).
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
int DateTime::GetSec() const
{
	m->ToGregorian();
	return ( (long)m->m_vSec % 60 );
}

/** Set time value
 * \par		Description:
 *			Set the time value of a DateTime object. The date value is left
 *			unchanged.
 *			No validity checking is performed.
 * \param	nHour Hours since midnight
 * \param	nMin Minutes past the hour
 * \param	nSec Seconds
 * \param	nUSec Microseconds
 * \sa SetDate()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void DateTime::SetTime( int nHour, int nMin, int nSec, int nUSec )
{
	m->ToGregorian();
	m->m_vSec = nHour * 3600 + nMin * 60 + nSec + nUSec / 1000000;
}

/** Set date value
 * \par		Description:
 *			Set the date value of a DateTime object. The time value is left
 *			unchanged. No validity checking is performed, this method will
 *			happily accept a date like 2003-13-32. (This would be interpreted as
 *			1 Feb 2004.)
 * \param	nYear Year
 * \param	nMonth Month
 * \param	nDay Year
 * \sa SetTime()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void DateTime::SetDate( int nYear, int nMonth, int nDay )
{
	m->ToGregorian();
	m->m_nYear = nYear;
	m->m_nMonth = nMonth;
	m->m_nDay = nDay;
}

bool DateTime::SetDate( const String& cDate )
{
	int y, m, d;
	int h, mi, s;
	int p;

	y = m = d = 0;
	h = mi = s = 0;
	
	// First, try parsing as an ISO 8601 date
	p = sscanf( cDate.c_str(), "%d-%d-%d %d:%d:%d", &y, &m, &d, &h, &mi, &s );
	if( p > 2 ) {
		if( y < 100 ) {
			y = y + ( ( y > 50 ) ? 1900 : 2000 );	// This is the Y2k050 bug :o)
		}
		SetDate( y, m, d );
		if( p < 4 ) h = 0;
		if( p < 5 ) mi = 0;
		if( p < 6 ) s = 0;
		SetTime( h, mi, s, 0 );
		return true;
	} else {
		// That didn't work, let's see if it's time only
		p = sscanf( cDate.c_str(), "%d:%d:%d", &h, &mi, &s );
		if( p > 1 ) {
			if( p < 3 ) s = 0;
			SetTime( h, mi, s, 0 );
			return true;
		} else {
			// TODO: try American format and IETF format
		}
	}

//	struct tm t;

/*struct tm {
  int 	tm_sec;
  int 	tm_min;
  int 	tm_hour;
  int 	tm_mday;
  int 	tm_mon;
  int 	tm_year;
  int 	tm_wday;
  int 	tm_yday;
  int 	tm_isdst;
  char* tm_zone;
  int 	tm_gmtoff;
};*/
	
	/*
		2003-08-24
		2003-08-24 19:26
		19:26:30.123
	*/
	
/*	if( strptime( pzDate, "%F", &t ) != NULL ) {	// ISO 8601 dates
		m->m_nMonth = t.tm_mon;
		m->m_nYear = t.tm_year;
		m->m_nDay = t.tm_mday;
		m->m_bJulian = false;
		return true;
	}*/
	return false;
}

String DateTime::GetDate() const
{
	char bfr[256];
	struct tm *pstm;
	time_t nTime = GetEpoch();

	pstm = localtime( &nTime );

	strftime( bfr, sizeof( bfr ), "%c", pstm );

	return (String)bfr;
}

int DateTime::GetDayOfWeek() const
{
	m->ToGregorian();

	int a = (14-m->m_nMonth) / 12;
	int y = m->m_nYear - a + 2000;

	return (m->m_nDay + y + y/4 - y/100 + y/400 + (31*(m->m_nMonth+12*a-2))/12) % 7;
}


size_t DateTime::GetFlattenedSize( void ) const
{
	return sizeof(m->GetFlattenedStruct());
}

status_t DateTime::Flatten( uint8* pBuffer, size_t nSize ) const
{
	FlattenStruct sProps = m->GetFlattenedStruct();
	if( nSize >= sizeof(sProps) ) 
	{
		memcpy( pBuffer, &sProps, sizeof(sProps ) );
		return 0;
    } 
	else 
	{
		return -1;
	}	
}

status_t DateTime::Unflatten( const uint8* pBuffer )
{
	FlattenStruct* psProps;
	memcpy(psProps,pBuffer,GetFlattenedSize());
	m->SetFromFlattenStruct(psProps);
	return 0;
}

int DateTime::GetType( void ) const
{
	return T_DATETIME;
}




