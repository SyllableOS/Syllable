/*
 * A dock plugin to select the frequency of the (PowerNow enabled) CPU
 * from a list.
 *
 * (c) 2007 Tim ter Laak, based on the Address dock plugin by Rick Caudill.
*/

#include "CpuSpeed.h"

#include <util/looper.h>
#include <gui/requesters.h>
#include <storage/directory.h>

#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <atheos/acpi.h>

#define PLUGIN_NAME       "CpuSpeed"
#define PLUGIN_VERSION    "1.0"
#define PLUGIN_DESC       "A plugin that lets the user select a processor frequency from all available frequencies."
#define PLUGIN_AUTHOR     "Tim ter Laak (based on code by Rick Caudill)"

enum
{
	M_SELECTION, 
};


CpuSpeedDrop::CpuSpeedDrop(View* pcParent) : DropdownMenu(Rect(0,3,85,20),"cpuspeed_drop")
{
	pcParentView = pcParent; 
	this->SetReadOnly(true);
	this->SetTarget(pcParentView, NULL);
	
	os::Message * pcSelectionMessage = new os::Message(M_SELECTION);
	this->SetSelectionMessage(pcSelectionMessage);
}


CpuSpeed::CpuSpeed(os::DockPlugin* pcPlugin, os::Looper* pcDock) : os::View( os::Rect(), "cpuspeed" )
{
	m_pcDock = pcDock;
	m_pcPlugin = pcPlugin;
	m_pcIcon = NULL;
	
	m_cDeviceFileName = "";	
	m_nFd = -1;
	m_nLastIndex = -1;
	
	pcCpuSpeedDrop = new CpuSpeedDrop(this);
	AddChild( pcCpuSpeedDrop );
}

CpuSpeed::~CpuSpeed()
{
	if( m_nFd > -1 )
		close( m_nFd );
}

void CpuSpeed::DetachedFromWindow()
{	
	delete( m_pcIcon );	
}

void CpuSpeed::LoadCpuSpeeds()
{
	String cName;
	os::Directory cDir;
	if( cDir.SetTo( "/dev/cpu" ) != 0 )
		return;
	
	if (m_cDeviceFileName == "")
	{
		next:
		while (cDir.GetNextEntry(&cName))
		{
			if (cName == "." || cName == "..")
				continue;
			else
			{
				m_cDeviceFileName = String( "/dev/cpu/" ) + cName;
				m_nFd = open( m_cDeviceFileName.c_str(), O_RDONLY );
				if( m_nFd < 0 )
					continue;
				int nNumStates = 0;
				while( 1 )
				{
					CPUPerformanceState_s sState;
					sState.cps_nIndex = nNumStates;
					if( ioctl( m_nFd, CPU_GET_PSTATE, &sState ) != 0 )
					{
						if( nNumStates == 0 )
						{
							close( m_nFd );
							m_nFd = -1;
							goto next;
						}
						return;
					}
					String cUnits;
					double vNormalized;
		
					vNormalized = sState.cps_nFrequency;
					if (vNormalized > 1000)
					{
						vNormalized /= 1000;
						cUnits.Format("%.2f GHz", vNormalized);	
					}
					else
					{
						cUnits.Format("%.0f MHz", vNormalized);
					}
		
					m_cFreqs.push_back( std::pair<int, String>(sState.cps_nFrequency, cUnits) );
					pcCpuSpeedDrop->AppendItem(cUnits);
					nNumStates++;
				}
				return;
			}
		}
	}
}

void CpuSpeed::AttachedToWindow()
{
	
#if 0
	os::File* pcFile = new os::File( m_pcPlugin->GetPath() );
	os::Resources cCol( pcFile );
	os::ResStream* pcStream = cCol.GetResourceStream( "icon48x48.png" );
	m_pcIcon = new os::BitmapImage( pcStream );
	delete pcStream;
	delete pcFile;
#endif
	LoadCpuSpeeds();
	GetSpeed();

	pcCpuSpeedDrop->SetTarget(this);
}

Point CpuSpeed::GetPreferredSize( bool bLargest ) const
{
	return Point(pcCpuSpeedDrop->GetBounds().Width(),pcCpuSpeedDrop->GetBounds().Height());
}


void CpuSpeed::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_SELECTION:
			{
				bool bFinal;
				if (pcMessage->FindBool("final", &bFinal) != 0)
				{
					break;
				}
				if (!bFinal)
				{
					break;
				}
				
				int32 nSelection;
				if (pcMessage->FindInt32("selection", &nSelection) != 0)
				{
					break;
				}
				SetSpeed(nSelection);
				break;
			}
			
			default:
				break;
	}
}

void CpuSpeed::SetSpeed(int nIndex)
{
	if ( (nIndex < 0) || (nIndex >= m_cFreqs.size()) )
		return;
	
	if ( m_nFd < 0 )
		return;
		
	if( m_nLastIndex == nIndex )
		return;
		
	CPUPerformanceState_s sState;
	sState.cps_nIndex = nIndex;
	ioctl( m_nFd, CPU_SET_PSTATE, &sState );
	
	GetSpeed();	
	
	return;
}

int CpuSpeed::GetSpeed()
{
	int nFreq;
	
	if ( m_nFd < 0 )
		return -1;
		
	CPUPerformanceState_s sCurrentState;
	if( ioctl( m_nFd, CPU_GET_CURRENT_PSTATE, &sCurrentState ) != 0 )
	{
		return( 0 );
	}		
	nFreq = sCurrentState.cps_nFrequency;
		
	int nDelta;
	int nBest = 0;
	for (int i=0; i<m_cFreqs.size(); i++)
	{
		if( abs( nFreq - m_cFreqs[i].first ) < abs( nFreq - m_cFreqs[nBest].first ) )
			nBest = i;
	}
	m_nLastIndex = nBest;
	pcCpuSpeedDrop->SetSelection(nBest, false);	
	return nFreq;	
}

//*************************************************************************************

class CpuSpeedPlugin : public os::DockPlugin
{
public:
	CpuSpeedPlugin()
	{
		m_pcView = NULL;
	}
	~CpuSpeedPlugin()
	{
	}
	
	status_t Initialize()
	{
		m_pcView = new CpuSpeed( this, GetApp() );
		AddView( m_pcView );
		return( 0 );
	}
	void Delete()
	{
		RemoveView( m_pcView );
	}
	
	os::String GetIdentifier()
	{
		return( PLUGIN_NAME );
	}
private:
	CpuSpeed* m_pcView;
};

extern "C"
{
DockPlugin* init_dock_plugin( os::Path cPluginFile, os::Looper* pcDock )
{
	return( new CpuSpeedPlugin() );
}
} // of extern "C"














