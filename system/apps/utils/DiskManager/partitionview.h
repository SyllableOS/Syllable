#ifndef __F_PARTITIONVIEW_H__
#define __F_PARTITIONVIEW_H__

#include <atheos/device.h>

#include <gui/window.h>
#include <gui/frameview.h>

#include <stack>

#include "main.h"

namespace os {
    class Spinner;
    class CheckBox;
    class DropdownMenu;
    class Button;
    class StringView;
};

class DiskInfo;
class PartitionView;
class PartitionEdit;


class PartitionEntry
{
public:
    off_t	GetByteSize( const device_geometry& sDiskGeom ) const;
    bool	IsModified() const { return( m_nStart != m_nOrigStart || m_nEnd != m_nOrigEnd ); }
    
    off_t m_nStart;
    off_t m_nEnd;

    off_t m_nOrigStart;
    off_t m_nOrigEnd;
    int	  m_nOrigType;
    int   m_nOrigStatus;
    
    off_t m_nMin;
    off_t m_nMax;
    
    int	  m_nType;
    int	  m_nStatus;

    std::string	    m_cVolumeName;
    std::string	    m_cPartitionPath;
    bool	    m_bIsMounted;
    
};


typedef std::vector<PartitionEntry*> PartitionList_t;



class PartitionBar : public os::View
{
public:
    PartitionBar( const os::Rect& cFrame, PartitionView* pcParent, const std::string& cName, float vStart, float vEnd );
    virtual void	AttachedToWindow();

    void		SetStart( float vValue );
    void		SetEnd( float vValue );
    void		SetColor( uint32 nColor );

    virtual os::Point	GetPreferredSize( bool bLargest ) const;
    virtual void	Paint( const os::Rect& cUpdateRect );
  
    virtual void	MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData );
    virtual void	MouseDown( const os::Point& cPosition, uint32 nButtons );
    virtual void	MouseUp( const os::Point& cPosition, uint32 nButtons, os::Message* pcData );
private:
    PartitionView* m_pcParent;
    float m_vStart;
    float m_vEnd;
    float m_vDragOffset;
    bool  m_bDragPointer;
    bool  m_bDragStart;
    bool  m_bDragEnd;
	uint32 m_nColor;
};


class PartitionView : public os::FrameView
{
public:
    PartitionView( PartitionEdit* pcParent, const std::string& cName, off_t nDiskSize, int nCylSize, int nSectorSize );

    void	SetPInfo( PartitionEntry* pcPInfo );
    

    int		GetType() const { return( (m_pcPInfo != NULL) ? m_pcPInfo->m_nType : 0 ); }
    int		GetStatus() const { return( (m_pcPInfo != NULL) ? m_pcPInfo->m_nStatus : 0 ); }
    off_t	GetStart() const { return( (m_pcPInfo != NULL) ? m_pcPInfo->m_nStart : 0 ); }
    off_t	GetEnd()  const { return( (m_pcPInfo != NULL) ? m_pcPInfo->m_nEnd : 0 ); }

    off_t	GetByteStart() const;
    off_t	GetByteSize() const;
    
    bool	IsModified() const {
	return( (m_pcPInfo != NULL) ? (m_pcPInfo->m_nStart != m_pcPInfo->m_nOrigStart || m_pcPInfo->m_nEnd != m_pcPInfo->m_nOrigEnd) : false );
    }
    bool	IsValidExtended() const;
    bool	IsMounted() const { return( (m_pcPInfo != NULL) ? m_pcPInfo->m_bIsMounted : false ); }

    void	EnableTypeMenu( bool bEnable );

    void	SetPath( const std::string& cPath );
    void	SetPartitionType( int nType, bool bPromptDelete = true );
    void	SetStart( off_t nStart );
    void	SetEnd( off_t nEnd );

    void	 SliderStartChanged( float* pvStart );
    void	 SliderEndChanged( float* pvEnd );

    virtual void AttachedToWindow();
    virtual void HandleMessage( os::Message* pcMessage );
    
private:
    bool FindFreeSpace();
    void SetSpinnerLimits();
    void SetSizeString();

    uint GetIndex() const;
    
    PartitionEdit*    m_pcParent;
    
    PartitionBar*     m_pcBarView;
    os::CheckBox*     m_pcActiveCB;
    os::Spinner*      m_pcStartSpinner;
    os::Spinner*      m_pcEndSpinner;
    os::DropdownMenu* m_pcTypeMenu;
    os::StringView*   m_pcVolumeName;
    os::StringView*   m_pcPartitionSize;
//    bool	      m_bIsMounted;
    
    off_t	      m_nDiskSize;
    int		      m_nCylinderSize;
    int		      m_nSectorSize;
    PartitionEntry*   m_pcPInfo;
};


struct BootBlock_s
{
    char   p_anLoader[446];
    struct {
	uint8  p_nStatus;
	uint8  p_nStartDH;	// Start sector in INT-13 call format
	uint8  p_nStartCL;
	uint8  p_nStartCH;
	uint8  p_nType;
	uint8  p_nEndDH;	// End sector in INT-13 call format
	uint8  p_nEndCL;
	uint8  p_nEndCH;
	uint32 p_nStart;
	uint32 p_nSize;
    } bb_asPartitions[4];
    uint16 p_nSignature;
} __attribute__((packed));


class PartitionTable
{
public:
    BootBlock_s	    m_sBootBlock;
    off_t	    m_nPosition;
    off_t	    m_nSize;
    std::string	    m_cDevicePath;
    int		    m_nNestingLevel;
    PartitionList_t m_cLogicalPartitions;
    std::string	    m_acVolumeNames[4];
    std::string	    m_acPartitionPaths[4];
    bool	    m_abMountedFlags[4];
};


class PartitionEdit : public os::LayoutView
{
public:
    PartitionEdit( const os::Rect& cFrame, const std::string& cName, DiskInfo cDisk, PartitionList_t* pcPList, off_t nDiskSize, bool bIsExtended );

    virtual void AllAttached();
    virtual void HandleMessage( os::Message* pcMessage );
    virtual void MouseDown( const os::Point& cPosition, uint32 nButtons );
    virtual void KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );

    bool	IsExtended() const { return( m_bIsExtended ); }
    void	PartitionTypeChanged();
    void	UpdateFreeSpace();

    void	UpdateLogicalDeviceNames();

    bool	FindFreeSpace( off_t* pnStart, off_t* pnEnd );
    
    void	DeletePartition( uint nIndex );
    void	CreatePartition( int nType );
    
    bool	m_bLayoutChanged;
    bool	m_bTypeChanged;
    bool	m_bStatusChanged;
    
private:
    void	SetFirstVisible( uint nFirst );
    void	EnableNavigationButtons();

    friend class PartitionView;
    bool		m_bIsExtended;
//    off_t		m_nPartitionOffset;
    off_t		m_nPartitionSize;
    device_geometry	m_sDiskGeom;
    std::string		m_cDevicePath;
    PartitionList_t*	m_pcPList;
    os::StringView*     m_pcDiskSizeLabel;
    os::StringView*     m_pcUnitSizeLabel;
    os::StringView*	m_pcUnusedSizeLabel;
    std::vector<PartitionView*>	m_cPartitionViews;
    uint		m_nFirstVisible;
    os::Button*		m_pcOkBut;
    os::Button*		m_pcPartitionBut;
    os::Button*		m_pcCancelBut;
    os::Button*		m_pcClearBut;
    os::Button*		m_pcSortBut;
    os::Button*		m_pcAddBut;
    os::Button*		m_pcPrevBut;
    os::Button*		m_pcNextBut;
};



class PartitionEditReq : public os::Window
{
public:
    PartitionEditReq( const DiskInfo& cDiskInfo );
    virtual void HandleMessage( os::Message* pcMessage );
private:
    void 	WritePartitionTable();
    bool	m_bLayoutChanged;
    bool	m_bTypeChanged;
    bool	m_bStatusChanged;
    int		m_nExtendedPos;
    bool	m_bIsPrimaryChanged;
    bool	m_bIsExtendedChanged;
    
    std::stack<PartitionEdit*> m_cEditViews;
    PartitionTable*	       m_pcFirstPTable;
    PartitionList_t	       m_cPrimary;
//    PartitionList_t	       m_cLogical;
    DiskInfo		       m_cDiskInfo;
};







#endif // __F_PARTITIONVIEW_H__
