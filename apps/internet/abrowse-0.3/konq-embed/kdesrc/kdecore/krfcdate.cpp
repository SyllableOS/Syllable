/*
 *
 *  This file is part of the KDE libraries
 *  Copyright (c) 2000 Waldo Bastian <bastian@kde.org>
 *
 * $Id$
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA 02111-1307, USA.
 **/

#include <sys/param.h>

#include <ctype.h>
#include <stdlib.h>

#include <krfcdate.h>


static const char haystack[37]="janfebmaraprmayjunjulaugsepoctnovdec";

time_t 
KRFCDate::parseDate(const QString &_date)
{
     // This parse a date in the form:
     //     Wednesday, 09-Nov-99 23:12:40 GMT
     // or
     //     Sat, 01-Jan-2000 08:00:00 GMT
     // or
     //     Sat, 01 Jan 2000 08:00:00 GMT
     //
     // We always assume GMT, the weekday is ignored
     //
     time_t result = 0;
     char *newPosStr;
     const char *dateString = _date.latin1();
     int day;
     char monthStr[4];
     int month;
     int year;
     int hour;
     int minute;
     int second;
     struct tm tm_s;

     while(*dateString && (*dateString != ' '))
     	dateString++;

     if (!*dateString)
     	return result;  // Invalid expire date
     	
     // ' 09-Nov-99 23:12:40 GMT'
     day = strtol(dateString, &newPosStr, 10);
     dateString = newPosStr;

     if ((day < 1) || (day > 31))
     	return result; // Invalid expire date;
     if (!*dateString)
     	return result;  // Invalid expire date

     if ((*dateString != '-') && (*dateString != ' '))
     	return result;  // Invalid expire date
     dateString++;

     for(int i=0; i < 3;i++)
     {
         if (!*dateString || (*dateString == '-') || (*dateString == ' '))
              return result;  // Invalid expire date
         monthStr[i] = tolower(*dateString++);
     }
     monthStr[3] = '\0';

     newPosStr = (char*)strstr(haystack, monthStr);

     if (!newPosStr)
     	return result;  // Invalid expire date

     month = (newPosStr-haystack)/3; // Jan=00, Feb=01, Mar=02, ..

     if ((month < 0) || (month > 11))
     	return result;  // Invalid expire date

     while(*dateString && (*dateString != '-') && (*dateString != ' '))
     	dateString++;

     if (!*dateString)
     	return result;  // Invalid expire date
     	
     // '-99 23:12:40 GMT'
     if ((*dateString != '-') && (*dateString != ' '))
     	return result;  // Invalid expire date
     dateString++;

     // '99 23:12:40 GMT'
     year = strtol(dateString, &newPosStr, 10);
     dateString = newPosStr;

     // Y2K: Solve 2 digit years
     if ((year >= 0) && (year < 50))
         year += 2000;

     if ((year >= 50) && (year < 100))
         year += 1900;  // Y2K

     if ((year < 1900) || (year > 2500))
     	return result; // Invalid expire date

     if (!*dateString)
     	return result;  // Invalid expire date

     // ' 23:12:40 GMT'
     if (*dateString++ != ' ')
     	return result;  // Invalid expire date

     hour = strtol(dateString, &newPosStr, 10);
     dateString = newPosStr;

     if ((hour < 0) || (hour > 23))
     	return result; // Invalid expire date

     if (!*dateString)
     	return result;  // Invalid expire date
          	
     // ':12:40 GMT'
     if (*dateString++ != ':')
     	return result;  // Invalid expire date

     minute = strtol(dateString, &newPosStr, 10);
     dateString = newPosStr;

     if ((minute < 0) || (minute > 59))
     	return result; // Invalid expire date

     if (!*dateString)
     	return result;  // Invalid expire date

     // ':40 GMT'
     if (*dateString++ != ':')
     	return result;  // Invalid expire date

     second = strtol(dateString, &newPosStr, 10);

     if ((second < 0) || (second > 59))
     	return result; // Invalid expire date

     tm_s.tm_sec = second;
     tm_s.tm_min = minute;
     tm_s.tm_hour = hour;
     tm_s.tm_mday = day;
     tm_s.tm_mon = month;
     tm_s.tm_year = year-1900;
     tm_s.tm_isdst = -1;

#ifndef BSD
     result = mktime( &tm_s)-timezone; // timezone = seconds _west_ of UTC
#else
     result = mktime (&tm_s);
     struct tm *tzone = localtime(&result);
     result += (tzone->tm_gmtoff); // tm_gmtoff = seconds _east_ of UTC
#endif

////////////////
// Debug stuff
//
//   printf("time = %ld sec since epoch, ctime=%s\n", result, ctime(&result));
//
//   time_t now = time(0);
//   printf("now = %ld (expire is in %s)\n", now, result > now ? "future" : "past");
////////////////

     return result;
}
