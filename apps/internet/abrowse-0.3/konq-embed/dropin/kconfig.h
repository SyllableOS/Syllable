#ifndef __kconfig_h__
#define __kconfig_h__

#include <qstring.h>
#include <qstringlist.h>
#include <qmap.h>
#include <qcolor.h>

class KConfig
{
public:
    KConfig( const QString &fileName, bool = false, bool = false );
    ~KConfig();

    QString group();
    void setGroup( const QString &group );

    void writeEntry( const QString &key, const QString &value );
    void writeEntry( const QString &key, int value );
    void writeEntry( const QString &key, const QStringList &value, QChar sep = ',' );

    bool hasGroup( const QString &group );
    bool hasKey( const QString &key );

    QString readEntry( const QString &key, const QString &defaultValue = QString::null );
    int readNumEntry( const QString &key, int defaultValue = 0 );
    QStringList readListEntry( const QString &key, QChar sep = ',' );
    bool readBoolEntry( const QString &key, bool defaultValue = false );

    QColor readColorEntry( const QString &key, const QColor *defaultVal );

    QMap<QString,QString> entryMap( const QString &group );

    void reparseConfiguration();

    void sync();

private:
    QString m_fileName;

    // maps field -> value
    typedef QMap<QString,QString> EntryMap;
    // maps group -> entry-map
    typedef QMap<QString,EntryMap> GroupMap;

    GroupMap m_data;
    GroupMap::Iterator m_current;
};

#endif
