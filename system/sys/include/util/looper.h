/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 - 2004 Syllable Team
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */

#ifndef	__F_UTIL_LOOPER_H__
#define	__F_UTIL_LOOPER_H__

#include <atheos/types.h>
#include <atheos/threads.h>
#include <atheos/msgport.h>
#include <util/handler.h>
#include <util/messagequeue.h>
#include <util/locker.h>
#include <util/string.h>

#include <map>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent


/** 
 * \ingroup util
 * \par Description:
 *
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class	Looper : public Handler
{
public:
    typedef std::map<int,Handler*> handler_map;
  
    Looper( const String& cName, int nPriority = NORMAL_PRIORITY, int nPortSize = DEFAULT_PORT_SIZE );
    virtual ~Looper();

    void	SetName( const String& cName );
    String GetName() const;

	bool IsPublic() const;
	void SetPublic( bool bPublic );

    port_id	GetMsgPort() const;
    thread_id	GetThread() const;
    proc_id	GetProcess() const;

    status_t	Lock();
    status_t	Lock( bigtime_t nTimeout );
    status_t	SafeLock();
    status_t	Unlock();

    void	SetMutex( Locker* pcMutex );
    Locker*	GetMutex() const;

    int		GetLockCount() const;
    thread_id	GetLockingThread() const;

    bool	IsLocked() const;

    virtual thread_id	Run();
    int			Wait() const;
    
    status_t	PostMessage( uint32 nCode );
    status_t	PostMessage( Message *pcMsg );
    status_t	PostMessage( uint32 cCode, Handler* pcHandler, Handler* pcReplyTo = NULL );
    status_t	PostMessage( Message *pcMsg, Handler *pcHandler, Handler* pcReplyTo = NULL );

    void		SpoolMessages();
    Message*		GetCurrentMessage() const;
    Message*		DetachCurrentMessage();
    virtual void	DispatchMessage( Message* pcMessage, Handler* pcHandler );
    virtual void	Started();
    virtual bool	Idle();
    
    MessageQueue*	GetMessageQueue() const;



    virtual bool	OkToQuit();
    virtual void	Quit();

    void			Terminate();
private:
    virtual void	__LO_reserved1__();
    virtual void	__LO_reserved2__();
    virtual void	__LO_reserved3__();
    virtual void	__LO_reserved4__();
    virtual void	__LO_reserved5__();
    virtual void	__LO_reserved6__();
    virtual void	__LO_reserved7__();
    virtual void	__LO_reserved8__();
public:
    void		AddTimer( Handler* pcTarget, int nID, bigtime_t nTimeout, bool bOneShot = true );
    bool		RemoveTimer( Handler* pcTarget, int nID );

    
    const handler_map&	GetHandlerMap() const;
    void		AddHandler( Handler* pcHandler );
    bool		RemoveHandler( Handler* pcHandler );
    Handler*		FindHandler( const String& cName ) const;
    int			GetHandlerCount() const;

    void		SetDefaultHandler( Handler* pcHandler );
    Handler*		GetDefaultHandler() const;

    void		AddCommonFilter( MessageFilter* pcFilter );
    void		RemoveCommonFilter( MessageFilter* pcFilter );
    
    const MsgFilterList& GetCommonFilterList() const;
    static Looper*		GetLooperForThread( thread_id hThread ) { return( _GetLooperForThread( hThread ) ); }

protected:
	bool FilterMessage( Message* pcMsg, Handler** ppcTarget, std::list<MessageFilter*>* pcFilterList );

private:
    friend class Application;
    friend class Messenger;
    friend class NodeMonitor;
    friend class Window;
    
    struct TimerNode
    {
	TimerNode( Handler* pcHandler, int nID, bigtime_t nPeriode, bool bOneShot );
    
	TimerNode* m_pcNext;
	bigtime_t  m_nTimeout;
    
	Handler*   m_pcHandler;
	int	       m_nID;
	bigtime_t  m_nPeriode;
	bool       m_bOnShot;
    };
  

    void	  _DecodeMessage( uint32 nCode, const void* pData );
    Handler*	  _FindHandler( int nToken ) const;
    void	  _Loop( void );
    static uint32 _Entry( void* pData );
    port_id	  _GetPort() const;
    void	  _InsertTimer( TimerNode* pcTimer );
    void	  _SetThread( thread_id hThread );

    class Private;
    Private* m;
  
    static void			_AddLooper( Looper* pcLooper );
    static void			_RemoveLooper( Looper* pcLooper );
    static Looper*		_GetLooperForThread( thread_id hThread );
    static Looper*		_GetLooperForPort( port_id hPort );
    static bool			_IsLooperValid( Looper* pcLooper );

//    static Looper*	s_pcFirstLooper;
//    static Locker	s_cLopperListMutex;
};

}

#endif	// __F_UTIL_LOOPER_H__

