#ifndef __khtml_find_h__
#define __khtml_find_h__

#include <qwidget.h>
#include <qobject.h>
class KHTMLPart;

class KHTMLFind : public QObject
{
public:
    KHTMLFind( KHTMLPart *part, QWidget *parent, const char *name )
        : QObject( parent, name ), m_part( part ) {}

    KHTMLPart *part() const { return m_part; }

    void setNewSearch() {}
    void setText( const QString & ) {}
    void setCaseSensitive( bool ) {}
    void setDirection( bool ) {}

    void show() {}

    QString getText() const { return QString::null; }
    bool case_sensitive() const { return false; }
    bool get_direction() const { return true; }

private:
    KHTMLPart *m_part;
};

#endif
