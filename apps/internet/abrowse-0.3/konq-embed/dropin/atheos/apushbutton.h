#ifndef __F_APUSHBUTTON_H__
#define __F_APUSHBUTTON_H__


#include <qwidget.h>

class QString;

class QPushButton : public QWidget
{
    Q_OBJECT
public:
    QPushButton( QWidget* pcParent );
    QPushButton( const QString &text, QWidget *parent, const char* name=0 );
    virtual ~QPushButton();

    virtual void AllAttached();
    virtual void HandleMessage( os::Message* pcMessage );

    virtual QSize sizeHint() const;
    virtual void setText( const QString &);

signals:
    void clicked();

private:
    class Private;
    Private* d;
};


#endif // __F_APUSHBUTTON_H__
