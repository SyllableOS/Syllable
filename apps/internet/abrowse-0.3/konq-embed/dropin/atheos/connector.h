#ifndef __F_CONNECTOR_H__
#define __F_CONNECTOR_H__


#include <vector>
#include <map>

namespace os
{
    class Message;
}

class ConnectReceiver;
class ConnectTransmitter;

typedef std::vector< std::pair<ConnectReceiver*,int> >  con_receiver_list_t;
typedef std::map<int,con_receiver_list_t*> con_event_map_t;

class ConnectReceiver
{
public:
    ConnectReceiver();
    virtual ~ConnectReceiver();

    
    virtual void DispatchEvent( ConnectTransmitter* pcSource, int nEvent, os::Message* pcMsg );

    ConnectTransmitter* GetCurrentSender() const;
protected:
    int  AllocIDRange( int nCount = 1000 );
private:
    friend class ConnectTransmitter;
    struct SourceEvent {
	ConnectTransmitter*       m_pcSource;
	int			  m_nEvent;
	int			  m_nID;
    };
    std::vector<SourceEvent> m_cSources;
    class Private;
    Private* d;
};

class ConnectTransmitter
{
public:
    ConnectTransmitter();
    virtual ~ConnectTransmitter();

    void SendEvent( int nEvent, os::Message* pcMsg = NULL );
    void Connect( ConnectReceiver* pcReceiver, int nEvent, int nID = -1, void* pData = NULL );
    void Disconnect( ConnectReceiver* pcReceiver, int nEvent, int nID = -1 );
private:
    friend class ConnectReceiver;
    con_event_map_t m_cEventMap;
};



#endif // __F_CONNECTOR_H__
