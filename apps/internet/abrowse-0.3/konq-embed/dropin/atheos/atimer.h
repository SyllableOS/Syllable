#ifndef __F_ATIMER_H__
#define __F_ATIMER_H__






#include <qobject.h>
#include <util/handler.h>





class QTimer : public QObject, public os::Handler
{
    Q_OBJECT
public:
    QTimer( QObject *parent=0, const char *name=0 );
   ~QTimer();

    virtual void TimerTick( int nID );
    
    bool	isActive() const;

    int		start( int msec, bool sshot = FALSE );
    void	changeInterval( int msec );
    void	stop();

    static void singleShot( int msec, QObject *receiver, const char *member );
//    static void singleShot( int msec, QObject *receiver, int nEvent );

signals:
    void	timeout();

private:
    bool	m_bSingleShot;
    bool	m_bIsActive;
    bool	m_bDeleteWhenTrigged;

private:	// Disabled copy constructor and operator=
    QTimer( const QTimer & );
    QTimer &operator=( const QTimer & );
};


#endif // __F_ATIMER_H__
