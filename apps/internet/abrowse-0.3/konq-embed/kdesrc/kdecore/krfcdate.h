/* 
   This file is part of the KDE libraries
   Copyright (c) 2000 Waldo Bastian <bastian@kde.org>
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef _KRFCDATE_H_
#define _KRFCDATE_H_

#include <qstring.h>
#include <time.h>

/**
 * The KDate class contains functions related to the parsing of dates.
 *
 * @author Waldo Bastian <bastian@kde.org>
 */
class KRFCDate
{
public:
   /**
    * This function tries to parse a string containing a date/time in any 
    * of the formats specified by RFC822, RFC850, RFC1036 and RFC1123.
    *
    * If the date/time could not be parsed, 0 is returned.
    * The date/time returned is converted to the current timezone.
    *
    * BUGS: Not all possible formats are covered yet.
    */
   static time_t parseDate(const QString &);
};

#endif
