#ifndef __dcopobject_h__
#define __dcopobject_h__

#include <qcstring.h>
#include <qlist.h>
#include <qobject.h>

#ifndef __ATHEOS__

class QSocketNotifier;

namespace KIO
{
    class Connection;
};

class DCOPClientTransaction;

class DCOPObject : public QObject
{
    Q_OBJECT
public:
    DCOPObject();
    virtual ~DCOPObject();

    // hehe ;-)
    static DCOPObject *self() { return s_self; }

    // yes, this is suddenly handled in here :)
    DCOPClientTransaction *beginTransaction();

    void endTransaction( DCOPClientTransaction *transaction, QCString &replyType, QByteArray &replyData );

    virtual bool process( const QCString &fun, const QByteArray &data,
                          QCString &replyType, QByteArray &replyData );

    // extension
    void addClient( KIO::Connection *connection, QObject *owner );

private slots:
    void slotDisconnectClient();
    void slotDispatch( int sockfd );

private:
    struct Client
    {
        KIO::Connection *m_connection;
        QObject *m_owner;
        QSocketNotifier *m_notifier;
        int m_infd;
    };
    QList<Client> m_clients;

    void disconnectClient( const QObject *owner );
    void dispatchClient( Client *client );

    struct InternalDCOPClientTransaction
    {
        Client *m_client;
    };

    QList<InternalDCOPClientTransaction> m_transactions;

    bool m_transactionAdded;

    static Client *s_currentClient;

    static DCOPObject *s_self;
};
#endif // __ATHEOS__

#endif
