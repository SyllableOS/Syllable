/* This file is part of the KDE libraries
    Copyright (C) 1997 Matthias Kalle Dalheimer (kalle@kde.org)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

// stripped down for konq-embed by Simon Hausmann <simon@kde.org>

// $Id$

// Include our header without NDEBUG defined to avoid having the kDebugInfo
// functions inlined to noops (which would then conflict with their definition
// here).

#ifdef NDEBUG
#define NODEBUG
#undef NDEBUG
#endif
#include "kdebug.h"
#ifdef NODEBUG
#define NDEBUG
#undef NODEBUG
#endif

#include <qfile.h>
#include <qlist.h>
#include <qstring.h>
#include <qtextstream.h>
//#include <qwidget.h>

#include <stdlib.h>     // abort
#include <stdarg.h>     // vararg stuff
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <config.h>

enum DebugLevels {
    KDEBUG_INFO=    0,
    KDEBUG_WARN=    1,
    KDEBUG_ERROR=   2,
    KDEBUG_FATAL=   3
};

static void kDebugBackend( unsigned short , unsigned short , const char *data)
{
    qDebug( "debug: %s", data );
}

static void kDebugBackend2( unsigned short nLevel, unsigned short nArea, const char* fmt, va_list arguments )
{
    char buf[4096] = "";
    int nSize = vsnprintf( buf, 4096, fmt, arguments );
    if( nSize > 4094 ) nSize = 4094;
    buf[nSize] = '\n';
    buf[nSize+1] = '\0';
    kDebugBackend( nLevel, nArea, buf);
}

void kDebugInfo( const char* fmt, ... )
{
    va_list arguments;
    va_start( arguments, fmt );
    kDebugBackend2( KDEBUG_INFO, 0, fmt, arguments );
    va_end( arguments );
}

void kDebugInfo( unsigned short area, const char* fmt, ... )
{
    va_list arguments;
    va_start( arguments, fmt );
    kDebugBackend2( KDEBUG_INFO, area, fmt, arguments  );
    va_end( arguments );
}

void kDebugInfo( bool cond, unsigned short area, const char* fmt, ... )
{
  if(cond)
    {
      va_list arguments;
      va_start( arguments, fmt );
      kDebugBackend2( KDEBUG_INFO, area, fmt, arguments );
      va_end( arguments );
    }
}

void kDebugWarning( const char* fmt, ... )
{
    va_list arguments;
    va_start( arguments, fmt );
    kDebugBackend2( KDEBUG_WARN, 0, fmt, arguments );
    va_end( arguments );
}

void kDebugWarning( unsigned short area, const char* fmt, ... )
{
    va_list arguments;
    va_start( arguments, fmt );
    kDebugBackend2( KDEBUG_WARN, area, fmt, arguments );
    va_end( arguments );
}

void kDebugWarning( bool cond, unsigned short area, const char* fmt, ... )
{
  if(cond)
    {
      va_list arguments;
      va_start( arguments, fmt );
      kDebugBackend2( KDEBUG_INFO, area, fmt, arguments );
      va_end( arguments );
    }
}

void kDebugError( const char* fmt, ... )
{
    va_list arguments;
    va_start( arguments, fmt );
    kDebugBackend2( KDEBUG_ERROR, 0, fmt, arguments );
    va_end( arguments );
}

void kDebugError( unsigned short area, const char* fmt, ... )
{
    va_list arguments;
    va_start( arguments, fmt );
    kDebugBackend2( KDEBUG_ERROR, area, fmt, arguments );
    va_end( arguments );
}

void kDebugError( bool cond, unsigned short area, const char* fmt, ... )
{
  if(cond)
    {
      va_list arguments;
      va_start( arguments, fmt );
      kDebugBackend2( KDEBUG_INFO, area, fmt, arguments );
      va_end( arguments );
    }
}

void kDebugFatal(const char* fmt, ... )
{
    va_list arguments;
    va_start( arguments, fmt );
    kDebugBackend2( KDEBUG_FATAL, 0, fmt, arguments );
    va_end( arguments );
}

void kDebugFatal(unsigned short area, const char* fmt, ... )
{
    va_list arguments;
    va_start( arguments, fmt );
    kDebugBackend2( KDEBUG_FATAL, area, fmt, arguments );
    va_end( arguments );
}

void kDebugFatal( bool cond, unsigned short area, const char* fmt, ... )
{
  if(cond)
    {
      va_list arguments;
      va_start( arguments, fmt );
      kDebugBackend2( KDEBUG_INFO, area, fmt, arguments );
      va_end( arguments );
    }
}

void kDebugPError( const char* fmt, ... )
{
    char buf[4096];
    va_list arguments;
    va_start( arguments, fmt );
    vsprintf( buf, fmt, arguments );
    kDebugError( "%s: %s", buf, strerror(errno) );
}

void kDebugPError( unsigned short area, const char* fmt, ... )
{
    char buf[4096];
    va_list arguments;
    va_start( arguments, fmt );
    vsprintf( buf, fmt, arguments );
    kDebugError( area, "%s: %s", buf, strerror(errno) );
}

kdbgstream &perror( kdbgstream &s) { return s << " " << strerror(errno); }
kdbgstream kdDebug(int area) { return kdbgstream(area, KDEBUG_INFO); }
kdbgstream kdDebug(bool cond, int area) { if (cond) return kdbgstream(area, KDEBUG_INFO); else return kdbgstream(0, 0, false); }

kdbgstream kdError(int area) { return kdbgstream("ERROR: ", area, KDEBUG_ERROR); }
kdbgstream kdError(bool cond, int area) { if (cond) return kdbgstream("ERROR: ", area, KDEBUG_ERROR); else return kdbgstream(0,0,false); }
kdbgstream kdWarning(int area) { return kdbgstream("WARNING: ", area, KDEBUG_WARN); }
kdbgstream kdWarning(bool cond, int area) { if (cond) return kdbgstream("WARNING: ", area, KDEBUG_WARN); else return kdbgstream(0,0,false); }
kdbgstream kdFatal(int area) { return kdbgstream("FATAL: ", area, KDEBUG_FATAL); }
kdbgstream kdFatal(bool cond, int area) { if (cond) return kdbgstream("FATAL: ", area, KDEBUG_FATAL); else return kdbgstream(0,0,false); }

void kdbgstream::flush() {
    if (output.isEmpty() || !print)
        return;
    kDebugBackend( level, area, output.local8Bit().data() );
    output = QString::null;
}

kdbgstream &kdbgstream::form(const char *format, ...)
{
    char buf[4096];
    va_list arguments;
    va_start( arguments, format );
    vsprintf( buf, format, arguments );
    va_end(arguments);
    *this << buf;
    return *this;
}

kdbgstream::~kdbgstream() {
    if (!output.isEmpty()) {
        fprintf(stderr, "ASSERT: debug output not ended with \\n\n");
        *this << "\n";
    }
}
#ifndef __ATHEOS__
kdbgstream& kdbgstream::operator << (QWidget* widget)
{
  QString string, temp;
  // -----
  if(widget==0)
    {
      string=(QString)"[Null pointer]";
    } else {
      temp.setNum((ulong)widget, 16);
      string=(QString)"["+widget->className()+" pointer "
        + "(0x" + temp + ")";
      if(widget->name(0)==0)
        {
          string += " to unnamed widget, ";
        } else {
          string += (QString)" to widget " + widget->name() + ", ";
        }
      string += "geometry="
        + QString().setNum(widget->width())
        + "x"+QString().setNum(widget->height())
        + "+"+QString().setNum(widget->x())
        + "+"+QString().setNum(widget->y())
        + "]";
    }
  if (!print)
    {
      return *this;
    }
  output += string;
  if (output.at(output.length() -1 ) == '\n')
    {
      flush();
    }
  return *this;
}
#endif
