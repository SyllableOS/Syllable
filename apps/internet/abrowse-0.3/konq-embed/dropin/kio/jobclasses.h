#ifndef __kio_job_h__
#define __kio_job_h__

class QWidget;

#include <qobject.h>
#include <qcstring.h>

#include <kurl.h>

#include <kio/global.h>


namespace KIO
{
    class Slave;
    
#ifdef __ATHEOS__

#include <qstring.h>
    
    class Job : public QObject
    {
	Q_OBJECT
    public:
        QString errorString() { QString s; return( s ); }
	Job() { m_nError = 0; }
        int error() const { return m_nError; }

	virtual void putOnHold() = 0;
	void setError( int nError ) { m_nError = nError; }
        void showErrorDialog( QWidget */*parent*/ = 0 ) {}
	
    signals:
        void result( KIO::Job *job );

        void infoMessage( KIO::Job *, const QString &msg );

        void connect( KIO::Job * );

        void percent( KIO::Job *job, unsigned long percent );

        void totalSize( KIO::Job *, unsigned long size );

        void processedSize( KIO::Job *, unsigned long size );

        void speed( KIO::Job *, unsigned long bytesPerSecond );
    private:
	int m_nError;
	
    };

    class SimpleJob : public KIO::Job
    {
        Q_OBJECT
    public:
	virtual KURL url() = 0;
	virtual void kill() = 0;
    };

class TransferJob : public KIO::SimpleJob
{
    Q_OBJECT
public:
    TransferJob();
    virtual ~TransferJob();

    virtual void Start();
//    void setError( int nError );
//    int error() const;
//    void showErrorDialog( QWidget *parent = 0 );
    
    virtual void    addMetaData( const QString &key, const QString &value );
    virtual void    addMetaData( const QMap<QString,QString> &values );
    virtual QString queryMetaData( const QString &key );

signals:
    void redirection( KIO::Job *, const KURL & );

    void data( KIO::Job *job, const QByteArray &data );

    void mimetype( KIO::Job *, const QString &);
private:
};

TransferJob* get( const KURL &url, const QByteArray &data, bool showProgressInfo );
//TransferJob* get_http_get( const KURL &url, const QByteArray &data, bool showProgressInfo );
TransferJob* http_post( const KURL &url, const QByteArray &data, bool showProgressInfo );
    
#else // __ATHEOS__
/*    
    class Job : public QObject
    {
        Q_OBJECT
    protected:
        Job( bool showProgressInfo );
    public:
        virtual ~Job();

        virtual void kill();

        void setWindow( QWidget *window ) { m_widget = window; }
        QWidget *window() const { return m_widget; }

        int error() const { return m_error; }

        QString errorText() { return m_errorText; }

        QString errorString();

        void showErrorDialog( QWidget *parent = 0 );

        void putOnHold() {} // ### check whether this belongs into another job class

    signals:
        void result( KIO::Job *job );

        void infoMessage( KIO::Job *, const QString &msg );

        void connect( KIO::Job * );

        void percent( KIO::Job *job, unsigned long percent );

        void totalSize( KIO::Job *, unsigned long size );

        void processedSize( KIO::Job *, unsigned long size );

        void speed( KIO::Job *, unsigned long bytesPerSecond );

    protected:
        int m_error;
        QString m_errorText;

    private:
        QWidget *m_widget;
        bool m_showProgressInfo;
    };

    class SimpleJob : public KIO::Job
    {
        Q_OBJECT
    public:
        SimpleJob( const KURL &url, int command,
                   const QByteArray &packedArgs,
                   bool showProgressInfo );
        virtual ~SimpleJob();

        KURL url() const { return m_url; }

        virtual void start( Slave *slave );

        virtual void kill();

     protected slots:
        virtual void receiveData( const QByteArray &dat );

        virtual void slaveFinished();

        virtual void dataReq();

        virtual void slaveRedirection( const KURL &url );

        virtual void slotInfoMessage( const QString &msg );

        virtual void slotError( int id, const QString &text );

    protected:
        Slave *m_slave;

        KURL m_url;
        int m_command;
        QByteArray m_packedArgs;
        unsigned long m_totalSize;
    };

    class TransferJob : public KIO::SimpleJob
    {
        Q_OBJECT
    public:
        TransferJob( const KURL &, int command,
                     const QByteArray &packedArgs,
                     const QByteArray &_staticData,
                     bool showProgressInfo );

        void setMetaData( const KIO::MetaData &data );

        void addMetaData( const QString &key, const QString &value );

        void addMetaData( const QMap<QString,QString> &values );

        MetaData metaData();

        QString queryMetaData( const QString &key );

        bool isErrorPage() { return false; }

        virtual void start( Slave *slave );

     protected slots:
        void setIncomingMetaData( const KIO::MetaData &dat );

        virtual void dataReq();

        virtual void slaveRedirection( const KURL &url );

        virtual void slaveFinished();

        virtual void receiveData( const QByteArray &dat );

    signals:
        void redirection( KIO::Job *, const KURL & );

        void data( KIO::Job *job, const QByteArray &data );

    private slots:
        void slotRedirectDelayed();

    private:
        MetaData m_outgoingMetaData;
        MetaData m_incomingMetaData;
        QByteArray m_staticData;
        KURL::List m_redirectionList;
        KURL m_redirectionURL;
    };*/
#endif // __ATHEOS__
};

#endif
