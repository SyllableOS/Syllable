
#include <qobject.h>
#ifndef __ATHEOS__
#include <qiconset.h>
#endif
#include <qstring.h>

class QString;

class QAction : public QObject
{
public:
    QAction() {}


    void setEnabled( bool ) {}
    bool isEnabled() const { return( false ); }
    void setText( const QString& ) {}
};
