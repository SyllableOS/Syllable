#ifndef __kjavaappletwidget_h__
#define __kjavaappletwidget_h__

#include <qwidget.h>

class KJavaAppletContext;

class KJavaAppletWidget : public QWidget
{
public:
    KJavaAppletWidget( KJavaAppletContext *, QWidget *parent, const char *name = 0 )
        : QWidget( parent, name )
    {}

    void create() {};

    void setParameter( const QString &, const QString & ) {}

    void showApplet() {}

    void setBaseURL( const QString & ) {}

    void setAppletClass( const QString & ) {}

    void setCodeBase( const QString & ) {}

    void setAppletName( const QString & ) {}

    void setJARFile( const QString & ) {}
};

#endif
