#ifndef __F_ACHECKBOX_H__
#define __F_ACHECKBOX_H__


#include <qwidget.h>

class QCheckBox : public QWidget
{
    Q_OBJECT
public:
    QCheckBox( QWidget* pcParent );

    virtual void HandleMessage( os::Message* pcMessage );
    
    bool    isChecked() const;
    void    setChecked( bool check );

    virtual QSize sizeHint() const;

signals:
    void clicked();
    void stateChanged(int);

private:
    class Private;
    Private* d;
    
};

#endif // __F_ACHECKBOX_H__
