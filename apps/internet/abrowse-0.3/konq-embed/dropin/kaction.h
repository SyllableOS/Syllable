#ifndef __kaction_h__
#define __kaction_h__

#ifndef __ATHEOS__

#include <qaction.h>
#include <qmap.h>

class KAction;

class KActionCollection : public QObject
{
public:
    KActionCollection( QObject *parent = 0, const char *name = 0 )
        : QObject( parent, name ) {}

    KAction *action( const char *name );
};

class KAction : public QAction
{
public:
    KAction( const QString &text, int accel, const QObject *receiver, const char *slot,
             QObject *parent, const char *name );

    KAction( const QString &text, const QString &icon, int accel, const QObject *receiver, const char *slot,
             QObject *parent, const char *name );

    void setIcon( const QString &name );

    // assigns an icon name (used with setIcon) a specified iconset
#ifndef __ATHEOS__
    void assignIconSet( const QString &name, const QIconSet &iconSet );
#endif
    
private:
#ifndef __ATHEOS__
    QMap<QString,QIconSet> m_icons;
#endif
};

class KSelectAction : public QAction // ### K?
{
public:
    KSelectAction( const QString &text, int accel, const QObject *receiver, const char *slot,
                   QObject *parent, const char *name );

    // ###
    void setItems( const QStringList & ) {}
    void setCurrentItem( int ) {}

    int currentItem() { return 0; }
    QString currentText() { return QString::null; }
};
#endif // __ATHEOS__

#endif
