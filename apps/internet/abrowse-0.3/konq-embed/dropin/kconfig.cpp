/*  This file is part of the KDE project
    Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "kconfig.h"
#ifndef __ATHEOS__
#include "kstddirs.h"
#endif
#include "kdebug.h"

#include <qfile.h>
#include <qtextstream.h>

KConfig::KConfig( const QString &fileName, bool, bool )
{
    // ### locateLocal!
#ifdef __ATHEOS__
    printf( "Warning: KConfig::KConfig() not using locate() to find \"config\"\n" );
    m_fileName = "";
#else    
    m_fileName = locate( "config", fileName );
#endif    
    if ( m_fileName.isEmpty() )
        kdDebug() << "Can't find " << fileName << " for kconfig" << endl;
    m_current = m_data.end();
    reparseConfiguration();
}

KConfig::~KConfig()
{
    sync();
}

QString KConfig::group()
{
    if ( m_current == m_data.end() )
        return QString::null;
    return m_current.key();
}

void KConfig::setGroup( const QString &group )
{
    QString grp = group;
    if ( grp.isEmpty() )
        grp = "<default>";

    m_current = m_data.find( grp );
    if ( m_current == m_data.end() )
        m_current = m_data.insert( grp, EntryMap() );
}

void KConfig::writeEntry( const QString &key, const QString &value )
{
    if ( m_current != m_data.end() )
        m_current.data().replace( key, value );
    else
        kdDebug() << "writeEntry without previous setGroup!" << endl;
}

void KConfig::writeEntry( const QString &key, int value )
{
    writeEntry( key, QString::number( value ) );
}

void KConfig::writeEntry( const QString &key, const QStringList &value, QChar sep )
{
    QString val;

    QStringList::ConstIterator it = value.begin();
    QStringList::ConstIterator end = value.end();
    while ( it != end )
    {
        val += *it;
        ++it;
        if ( it != end )
            val += sep;
    }

    writeEntry( key, val );
}

bool KConfig::hasGroup( const QString &group )
{
    return m_data.contains( group );
}

bool KConfig::hasKey( const QString &key )
{
    if ( m_current == m_data.end() )
        return false;

    return m_current.data().contains( key );
}

QString KConfig::readEntry( const QString &key, const QString &defaultValue )
{
    if ( m_current == m_data.end() )
        return defaultValue;

    EntryMap::ConstIterator it = m_current.data().find( key );

    if ( it == m_current.data().end() )
        return defaultValue;

    return *it;
}

int KConfig::readNumEntry( const QString &key, int defaultValue )
{
    QString val = readEntry( key );

    if ( val.isEmpty() )
        return defaultValue;

    return val.toInt();
}

QStringList KConfig::readListEntry( const QString &key, QChar sep )
{
    QString val = readEntry( key );

    if ( val.isEmpty() )
        return QStringList();

    return QStringList::split( sep, val );
}

bool KConfig::readBoolEntry( const QString &key, bool defaultValue )
{
    QString val = readEntry( key );

    if ( val.isEmpty() )
        return defaultValue;

    val = val.lower();

    if ( val == "true" )
        return true;

    if ( val == "on" )
        return true;

    if ( val == "off" )
        return false;

    if ( val == "false" )
        return false;

    return static_cast<bool>( val.toInt() );
}

QColor KConfig::readColorEntry( const QString &key, const QColor *defaultVal )
{
    QString val = readEntry( key );

    QColor res = *defaultVal;

    if ( !val.isEmpty() && val.at( 0 ) == '#' )
        res.setNamedColor( val );

    return res;
}

QMap<QString,QString> KConfig::entryMap( const QString &group )
{
    QString currentKey = m_current.key();

    GroupMap::ConstIterator grp = m_data.find( group );
    if ( grp == m_data.end() )
    {
        grp = m_data.insert( group, EntryMap() );
        m_current = m_data.find( currentKey );
    }

    return grp.data();
}

void KConfig::reparseConfiguration()
{
    m_data.clear();
    m_current = m_data.end();

    if ( m_fileName.isEmpty() )
        return;

    QFile f( m_fileName );
    if ( !f.open( IO_ReadOnly ) )
        return;

    QTextStream stream( &f );

    while ( !stream.eof() )
    {
        QString line = stream.readLine();

        QString stripped = line.stripWhiteSpace();

        // comment? skip
        if ( stripped[ 0 ] == '#' )
            continue;

        // group?
        if ( stripped[ 0 ] == '[' )
        {
            // find end of group
            int pos = stripped.find( ']' );

            if ( pos == -1 )
                continue;

            QString group = stripped.mid( 1, pos - 1 );

            // create new group only if it doesn't exist, yet
            if ( m_data.contains( group ) )
                continue;

            m_current = m_data.insert( group, EntryMap() );
        }
        else if ( stripped.find( '=' ) != -1 )
        {
            QStringList lst = QStringList::split( '=', stripped );

            if ( lst.count() != 2 )
                continue;

            if ( m_current == m_data.end() )
            {
                // default group
                m_current = m_data.find( "<default>" );
                if ( m_current == m_data.end() )
                    m_current = m_data.insert( "<default>", EntryMap() );
            }

            m_current.data().insert( lst[ 0 ], lst[ 1 ] );
        }
    }
}

void KConfig::sync()
{
    if ( m_fileName.isEmpty() )
        return;

    QFile f( m_fileName );
    if ( !f.open( IO_WriteOnly ) )
        return;

    QTextStream s( &f );

    GroupMap::ConstIterator gIt = m_data.begin();
    GroupMap::ConstIterator gEnd = m_data.end();
    for (; gIt != gEnd; ++gIt )
    {
        s << '[' << gIt.key() << ']' << endl;
        EntryMap m = gIt.data();
        EntryMap::ConstIterator eIt = m.begin();
        EntryMap::ConstIterator eEnd = m.end();
        for (; eIt != eEnd; ++eIt )
            s << eIt.key() << '=' << eIt.data() << endl;
    }

    f.close();
}
