#ifndef __klocale_h__
#define __klocale_h__

#include <qobject.h>
#include <qstringlist.h>
#include <qdatetime.h>

// ### compat to make html_formimpl.cpp compile
#include <kglobal.h>

#define i18n QObject::tr
#define I18N_NOOP( x ) x

// dummy
class KLocale
{
public:
    KLocale() {}
    ~KLocale() {}

    // ###
    QStringList languageList() const
        {
            QStringList res;
            res << QString::fromLatin1( "C" );
            return res;
        }

    // ###
    QString language() const { return QString::fromLatin1( "C" ); }

    // ###
    QString languages() { return language(); }

    // ###
    QString charset() const { return QString::fromLatin1( "iso-8859-1" ); }

    static void setMainCatalogue( const char * ) {}

    QString formatDate( const QDate &date ) const { return date.toString(); }

    QString formatTime( const QTime &time ) const { return time.toString(); }

    QString formatDateTime( const QDateTime &dt, bool = false, bool = false ) const
        { return formatDate( dt.date() ) + ' ' + formatTime( dt.time() ); }

    // ###
    QString formatNumber( double num, int precision = 6 ) const
        { return QString::number( num, 'g', precision ); }
};

#endif
