#include <stdio.h>
#include <assert.h>

#include <util/message.h>
#include "connector.h"


#include <algorithm>

class ConnectReceiver::Private
{
public:
    int			m_nCurIDStart;
    ConnectTransmitter* m_pcCurSender;

};

ConnectReceiver::ConnectReceiver()
{
    d = new Private;
    d->m_nCurIDStart = 1000000;
    d->m_pcCurSender = NULL;
}

int ConnectReceiver::AllocIDRange( int nCount )
{
    int nStart = d->m_nCurIDStart;
    d->m_nCurIDStart += nCount;
    return( nStart );
}

ConnectReceiver::~ConnectReceiver()
{
    for ( uint i = 0 ; i < m_cSources.size() ; ++i ) {
	con_event_map_t::iterator j = m_cSources[i].m_pcSource->m_cEventMap.find( m_cSources[i].m_nEvent );
	if ( j == m_cSources[i].m_pcSource->m_cEventMap.end() ) {
	    printf( "Warning: ConnectReceiver::~ConnectReceiver() Invalid entry in source list. Event list not found\n" );
	    continue;
	}
	con_receiver_list_t* pcList = (*j).second;
	for ( con_receiver_list_t::iterator k = pcList->begin() ; k != pcList->end() ; ) {
	    if ( (*k).first == this ) {
		k = pcList->erase( k );
		if ( pcList->empty() ) {
		    delete pcList;
		    m_cSources[i].m_pcSource->m_cEventMap.erase( j );
		    break;
		}
	    } else {
		++k;
	    }
	}
    }
    delete d;
}

ConnectTransmitter* ConnectReceiver::GetCurrentSender() const
{
    return( d->m_pcCurSender );
}

void ConnectReceiver::DispatchEvent( ConnectTransmitter* /*pcSource*/, int /*nEvent*/, os::Message* /*pcMsg*/ )
{
}


ConnectTransmitter::ConnectTransmitter()
{
}

ConnectTransmitter::~ConnectTransmitter()
{
    for ( con_event_map_t::iterator i = m_cEventMap.begin() ; i != m_cEventMap.end() ; ++i ) {
	con_receiver_list_t* pcList = (*i).second;
	for ( uint j = 0 ; j < pcList->size() ; ++j ) {
	    ConnectReceiver* pcReceiver = (*pcList)[j].first;
	    for ( uint k = 0 ; k < pcReceiver->m_cSources.size() ; ++k ) {
		if ( pcReceiver->m_cSources[k].m_pcSource == this && pcReceiver->m_cSources[k].m_nEvent == (*i).first ) {
		    pcReceiver->m_cSources.erase( pcReceiver->m_cSources.begin() + k );
		    break;
		}
	    }
	}
	delete pcList;
    }
}


void ConnectTransmitter::SendEvent( int nEvent, os::Message* pcMsg )
{
    fflush( stdout );

    con_event_map_t::iterator i = m_cEventMap.find( nEvent );
    if ( i == m_cEventMap.end() ) {
	return;
    }
    con_receiver_list_t cList = *(*i).second;
    for ( uint i = 0 ; i < cList.size() ; ++i ) {
	ConnectReceiver* pcRec = cList[i].first;
	pcRec->d->m_pcCurSender = this;
	pcRec->DispatchEvent( this, cList[i].second, pcMsg );
	pcRec->d->m_pcCurSender = NULL;
    }
}

void ConnectTransmitter::Connect( ConnectReceiver* pcReceiver, int nEvent, int nID, void* /*pData*/ )
{
    con_receiver_list_t* pcList;

    if ( nID == -1 ) {
	nID = nEvent;
    }
    
    con_event_map_t::iterator i = m_cEventMap.find( nEvent );
    if ( i == m_cEventMap.end() ) {
	pcList = new con_receiver_list_t;
	m_cEventMap.insert( std::make_pair( nEvent, pcList ) );
    } else {
	pcList = (*i).second;
    }
    assert( std::find( pcList->begin(), pcList->end(), std::make_pair( pcReceiver, nID ) ) == pcList->end() );
    pcList->push_back( std::make_pair( pcReceiver, nID ) );
    ConnectReceiver::SourceEvent sEntry;
    sEntry.m_pcSource = this;
    sEntry.m_nEvent   = nEvent;
    sEntry.m_nID      = nID;
    pcReceiver->m_cSources.push_back( sEntry );
}

void ConnectTransmitter::Disconnect( ConnectReceiver* pcReceiver, int nEvent, int nID )
{
    con_event_map_t::iterator i = m_cEventMap.find( nEvent );

    if ( nID == -1 ) {
	nID = nEvent;
    }
    
    if ( i == m_cEventMap.end() ) {
	printf( "ERROR: ConnectTransmitter::Disconnect() unknown event %d\n", nEvent );
	return;
    }
    con_receiver_list_t* pcList = (*i).second;
    con_receiver_list_t::iterator j = std::find( pcList->begin(), pcList->end(), std::make_pair( pcReceiver, nID ) );
    if ( j == pcList->end() ) {
	printf( "ERROR: ConnectTransmitter::Disconnect() unknown receiver %p for event %d\n", pcReceiver, nEvent );
	return;
    }
    for ( uint k = 0 ; k < pcReceiver->m_cSources.size() ; ++k ) {
	if ( pcReceiver->m_cSources[k].m_pcSource == this && pcReceiver->m_cSources[k].m_nEvent == nEvent && pcReceiver->m_cSources[k].m_nID == nID ) {
	    pcReceiver->m_cSources.erase( pcReceiver->m_cSources.begin() + k );
	    break;
	}
    }
    pcList->erase(j);
    if ( pcList->empty() ) {
	delete pcList;
	m_cEventMap.erase( i );
    }
}

