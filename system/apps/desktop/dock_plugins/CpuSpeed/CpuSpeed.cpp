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

#define DRAG_THRESHOLD 4

enum
{
	M_SELECTION, 
};


CpuSpeedDrop::CpuSpeedDrop(View* pcParent) : DropdownMenu(Rect(0,0,85,20),"cpuspeed_drop")
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
	m_bCanDrag = m_bDragging = false;
	
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
//	delete( m_pcDragIcon );		/* currently, m_pcIcon == m_pcDragIcon */
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
	os::File* pcFile = new os::File( m_pcPlugin->GetPath() );
	os::Resources cCol( pcFile );
	os::ResStream* pcStream = cCol.GetResourceStream( "icon48x48.png" );
	m_pcIcon = new os::BitmapImage( pcStream );
	delete pcStream;
	delete pcFile;
	
	m_pcDragIcon = m_pcIcon;

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

void CpuSpeed::MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData )
{
	if( nCode != MOUSE_ENTERED && nCode != MOUSE_EXITED )
	{
		/* Create dragging operation */
		if( m_bCanDrag )
		{
			m_bDragging = true;
			os::Message cMsg;
			BeginDrag( &cMsg, os::Point( m_pcDragIcon->GetBounds().Width() / 2,
											m_pcDragIcon->GetBounds().Height() / 2 ), m_pcDragIcon->LockBitmap() );
			m_bCanDrag = false;
		}
	}
	os::View::MouseMove( cNewPos, nCode, nButtons, pcData );
}


void CpuSpeed::MouseUp( const os::Point & cPosition, uint32 nButtons, os::Message * pcData )
{
	// Get the frame of the dock
	// If the plugin is dragged outside of the dock;s frame
	// then remove the plugin
	Rect cRect = ConvertFromScreen( GetWindow()->GetFrame() );

	if( ( m_bDragging && ( cPosition.x < cRect.left ) ) || ( m_bDragging && ( cPosition.x > cRect.right ) ) || ( m_bDragging && ( cPosition.y < cRect.top ) ) || ( m_bDragging && ( cPosition.y > cRect.bottom ) ) ) 
	{
		/* Remove ourself from the dock */
		os::Message cMsg( os::DOCK_REMOVE_PLUGIN );
		cMsg.AddPointer( "plugin", m_pcPlugin );
		m_pcDock->PostMessage( &cMsg, m_pcDock );
		return;
	} /* else if ( nButtons == 1 ) { // left button
		// Check to see if the coordinates passed match when the left mouse button was pressed
		// if so, then it was a single click and not a drag
		if ( abs( (int)(m_cPos.x - cPosition.x) ) < DRAG_THRESHOLD && abs( (int)(m_cPos.y - cPosition.y) ) < DRAG_THRESHOLD )
		{
			// Just eat it for the time being.
		}
	}*/

	m_bDragging = false;
	m_bCanDrag = false;
	os::View::MouseUp( cPosition, nButtons, pcData );
}

void CpuSpeed::MouseDown( const os::Point& cPosition, uint32 nButtons )
{
	if( nButtons == 1 ) /* left button */
	{
		MakeFocus( true );
		m_bCanDrag = true;
		// Store these coordinates for later use in the MouseUp procedure
		m_cPos.x = cPosition.x;
		m_cPos.y = cPosition.y;
	} else if ( nButtons == 2 ) { /* right button */
		MakeFocus( false );
	}

	os::View::MouseDown( cPosition, nButtons );
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

