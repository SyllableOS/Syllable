
#include <qobject.h>
#include <qshared.h>

class QGuardedPtrPrivate : public  ConnectReceiver, public QShared
{
//    Q_OBJECT
public:
    QGuardedPtrPrivate( QObject*);
    ~QGuardedPtrPrivate();

    virtual void DispatchEvent( ConnectTransmitter* pcSource, int nEvent, os::Message* pcMsg );
    
    QObject* object() const;
private:
    QObject* obj;
};


template <class T> class QGuardedPtr
{
public:
    QGuardedPtr()
    {
	priv = new QGuardedPtrPrivate( 0 );
    }
    QGuardedPtr( T* o)
    {
#if defined(Q_TEMPLATE_NEEDS_EXPLICIT_CONVERSION)
	priv = new QGuardedPtrPrivate( (QObject*)o );
#else
	priv = new QGuardedPtrPrivate( o );
#endif
    }
    QGuardedPtr(const QGuardedPtr<T> &p)
    {
	priv = p.priv;
	ref();
    }
    ~QGuardedPtr()
    {
	deref();
    }

    QGuardedPtr<T> &operator=(const QGuardedPtr<T> &p)
    {
	if ( priv != p.priv ) {
	    deref();
	    priv = p.priv;
	    ref();
	}
	return *this;
    }

    QGuardedPtr<T> &operator=(T* o)
    {
	deref();
#if defined(Q_TEMPLATE_NEEDS_EXPLICIT_CONVERSION)
	priv = new QGuardedPtrPrivate( (QObject*)o );
#else
	priv = new QGuardedPtrPrivate( o );
#endif
	return *this;
    }

    bool operator==( const QGuardedPtr<T> &p ) const
    {
	return priv->object() == p.priv->object();
    }

    bool operator!= ( const QGuardedPtr<T>& p ) const
    {
	return !( *this == p );
    }

    bool isNull() const
    {
	return !priv->object();
    }

    T* operator->() const
    {
	return (T*) priv->object();
    }

    T& operator*() const
    {
	return *( (T*)priv->object() );
    }

    operator T*() const
    {
	return (T*) priv->object();
    }


private:
    void ref()
    {
	priv->ref();
    }
    void deref()
    {
	if ( priv->deref() )
	    delete priv;
    }
    QGuardedPtrPrivate* priv;
};




inline QObject* QGuardedPtrPrivate::object() const
{
    return obj;
}
