#ifndef __F_ALINEEDIT_H__
#define __F_ALINEEDIT_H__


#include <qwidget.h>



class QLineEdit : public QWidget
{
    Q_OBJECT
public:
    enum	EchoMode { Normal, NoEcho, Password };
    QLineEdit( QWidget* pcParent );
    ~QLineEdit();

    virtual void AllAttached();
    virtual void HandleMessage( os::Message* pcMessage );
    
    void   SetMinPreferredSize( int nWidthChars );
    virtual QSize sizeHint() const;

    virtual void setMaxLength( int );
    
    void 	setText( const QString &);
    bool	frame() const;

    void	selectAll();
    
    void setReadOnly( bool );
    bool isReadOnly() const;

    virtual void setEchoMode( EchoMode );

signals:
    void returnPressed();
    void textChanged( const QString & );
    
private:
    class Private;
    Private* d;
    
};

#endif // __F_ALINEEDIT_H__
