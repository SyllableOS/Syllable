#ifndef _LINUXCOMP_H_
#define _LINUXCOMP_H_

#include <atheos/linux_compat.h>

inline uint32 pci_size(uint32 base, uint32 mask)
{
	uint32 size = base & mask;
	return size & ~(size-1);
}

inline uint32 get_pci_memory_size(PCI_Info_s *pcPCIInfo, int nResource)
{
	int nBus = pcPCIInfo->nBus;
	int nDev = pcPCIInfo->nDevice;
	int nFnc = pcPCIInfo->nFunction;
	int nOffset = PCI_BASE_REGISTERS + nResource * 4;
	uint32 nBase = g_psBus->read_pci_config(nBus, nDev, nFnc, nOffset, 4);
	
	g_psBus->write_pci_config(nBus, nDev, nFnc, nOffset, 4, ~0);
	uint32 nSize = g_psBus->read_pci_config(nBus, nDev, nFnc, nOffset, 4);
	g_psBus->write_pci_config(nBus, nDev, nFnc, nOffset, 4, nBase);
	if (!nSize || nSize == 0xffffffff) return 0;
	if (nBase == 0xffffffff) nBase = 0;
	if (!(nSize & PCI_ADDRESS_SPACE)) {
		return pci_size(nSize, PCI_ADDRESS_MEMORY_32_MASK);
	} else {
		return pci_size(nSize, PCI_ADDRESS_IO_MASK & 0xffff);
	}
}

#endif
