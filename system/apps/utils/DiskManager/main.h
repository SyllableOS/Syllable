#ifndef __F_DISKMANAGER_MAIN_H__
#define __F_DISKMANAGER_MAIN_H__

#include <atheos/device.h>

#include <string>
#include <vector>
/*
class DiskInfo;

struct PartitionInfo
{
    PartitionInfo() { m_nStart = 0; m_nSize = 0; m_nType = 0; m_psExtended = NULL; };
    ~PartitionInfo();
    off_t	m_nStart;
    off_t	m_nSize;
    int		m_nType;
    uint32	m_nStatus;
    char	m_zName[4];
    DiskInfo*	m_psExtended;
};
*/


struct DiskInfo
{
    std::string		m_cPath;
    int			m_nDiskFD;
    device_geometry	m_sDriveInfo;
//    off_t		m_nOffset;
//    PartitionInfo*	m_psParent;
//    PartitionInfo	m_asPartitions[4];
};




#endif // __F_DISKMANAGER_MAIN_H__
