#ifndef __F_FISKMANAGER_H__
#define __F_FISKMANAGER_H__

#include <sys/types.h>
#include <gui/layoutview.h>

class PartitionEntry
{
public:
    PartitionEntry( off_t nStart, off_t nEnd ) { m_nStart = nStart; m_nEnd = nEnd; }
    off_t	m_nStart;
    off_t	m_nEnd;
};

class PartitionView : public os::View
{
public:
    PartitionView( const std::string& cName, const std::string& cDevicePath );
    virtual os::Point	GetPreferredSize( bool bLargest ) const;

    virtual void	Paint( const os::Rect& cUpdateRect );
    virtual void	Activated( bool bIsActive );
    virtual void	MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData );
    virtual void	MouseDown( const os::Point& cPosition, uint32 nButtons );
    virtual void	MouseUp( const os::Point& cPosition, uint32 nButtons, os::Message* pcData );

private:
    std::string 	       m_cDevicePath;
    off_t		       m_nDiskSize;
    std::vector<PartitionEntry> m_cPartitionList;
    std::vector<os::Rect>	m_cPartitionFrames;
    int				m_nSelected;
};


class DiskManagerView : public os::LayoutView
{
public:
    DiskManagerView( const std::string& cName );
    ~DiskManagerView();
private:
    std::vector<PartitionView*>	m_cDisks;
};

#endif // __F_FISKMANAGER_H__
