#ifndef __F_ARADIOBUTTON_H__
#define __F_ARADIOBUTTON_H__


#include <qwidget.h>

class QRadioButton : public QWidget
{
    Q_OBJECT
public:
    QRadioButton( QWidget* pcParent );
    ~QRadioButton();
    
    virtual void HandleMessage( os::Message* pcMessage );
    
    bool    isChecked() const;
    void    setChecked( bool check );
    
    virtual QSize sizeHint() const;

signals:
    void clicked();
    
private:
    class Private;
    Private* d;
    
};


#endif // __F_ARADIOBUTTON_H__
