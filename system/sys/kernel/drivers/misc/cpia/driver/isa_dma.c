#include <atheos/isa_io.h>

#include "isa_dma.h"

//-----------------------------------------------------------------------------

static int fChannelAlloc = 0;

typedef struct
{
    bool	b16Bit;
    int		nAddress;
    int		nCount;
    int		nPage;
    int		nMask;
    int		nMode;
    int		nFlipFlop;
} DMAChannel;

const static DMAChannel DMAChannels[8] =
{
    { false, 0x00, 0x01, 0x87, 0x0a, 0x0b, 0x0c }, // channel 0, 8bit
    { false, 0x02, 0x03, 0x83, 0x0a, 0x0b, 0x0c }, // channel 1, 8bit
    { false, 0x04, 0x05, 0x81, 0x0a, 0x0b, 0x0c }, // channel 2, 8bit (used by motherboard)
    { false, 0x06, 0x07, 0x82, 0x0a, 0x0b, 0x0c }, // channel 3, 8bit
    { true,  0x00, 0x00, 0x00, 0xd4, 0xd6, 0xd8 }, // channel 4, 16bit (used by motherboard)
    { true,  0xc4, 0xc6, 0x8b, 0xd4, 0xd6, 0xd8 }, // channel 5, 16bit
    { true,  0xc8, 0xca, 0x89, 0xd4, 0xd6, 0xd8 }, // channel 6, 16bit
    { true,  0xcc, 0xce, 0x8a, 0xd4, 0xd6, 0xd8 }, // channel 7, 16bit
};

//-----------------------------------------------------------------------------

// FIXME: Well, not quite atomic yet ;)
int atomic_or( int *pnAddr, int nValue )
{
    int nOld = *pnAddr;
    *pnAddr |= nValue;
    return nOld;
}
int atomic_and( int *pnAddr, int nValue )
{
    int nOld = *pnAddr;
    *pnAddr &= nValue;
    return nOld;
}

//-----------------------------------------------------------------------------

status_t lock_isa_dma_channel( int nChannel )
{
    if( nChannel<0 || nChannel>7 )
	return -EINVAL;
    
    if( atomic_or(&fChannelAlloc,1<<nChannel)&(1<<nChannel) )
	return -EBUSY;

    return 0;
}

status_t unlock_isa_dma_channel( int nChannel )
{
    if( nChannel<0 || nChannel>7 )
	return -EINVAL;

    if( (atomic_and(&fChannelAlloc,~(1<<nChannel))&(1<<nChannel)) == 0 )
	return -EINVAL; // Hey! the dma channel was not locket!

    return 0;
}

status_t start_isa_dma( int nChannel, void *pAddress, size_t nLength, int nMode )
{
    uint8 nPage;
    uint16 nOffset;
    const DMAChannel *pcDMA = NULL;
    
    if( nChannel<0 || nChannel>7 )
	return -EINVAL;

    pcDMA = &DMAChannels[nChannel];
    
      // FIXME:: add spinlock
    if( pcDMA->b16Bit )
    {
	pAddress = (void*)(((int)pAddress)>>1);
	nLength = nLength>>1;
    }

    nPage = ((int)(pAddress))>>16;
    nOffset = ((int)(pAddress))&0xffff;

      // Stop DMA
    outb( (nChannel&0x3)|4, pcDMA->nMask );

    outb( 0, pcDMA->nFlipFlop );
    outb( (nChannel&0x3)|nMode, pcDMA->nMode );
    outb( (nLength-1)&0xff, pcDMA->nCount );
    outb( (nLength-1)>>8, pcDMA->nCount );

      // Program DMA
    outb( nPage, pcDMA->nPage );
    outb( nOffset&0xff, pcDMA->nAddress );
    outb( nOffset>>8, pcDMA->nAddress );

      // Start DMA
    outb( nChannel&0x3, pcDMA->nMask );

    return 0;
}

//-----------------------------------------------------------------------------
