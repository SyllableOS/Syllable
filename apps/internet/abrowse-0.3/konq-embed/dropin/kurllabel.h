#ifndef __kurllabel_h__
#define __kurllabel_h__

#include <qlabel.h>

class KURLLabel : public QLabel
{
public:
    KURLLabel( const QString &, const QString &text, QWidget *parent = 0, const char *name = 0 )
        : QLabel( text, parent, name ) {}
};

#endif
