#include <atheos/types.h>
#include <posix/errno.h>

#define ISADMA_MODE_VERIFY	0x00
#define ISADMA_MODE_WRITE	0x04
#define ISADMA_MODE_READ	0x08

#define ISADMA_MODE_INC		0x00
#define ISADMA_MODE_DEC		0x20

#define ISADMA_MODE_DEMAND	0x00
#define ISADMA_MODE_SINGLE	0x40
#define ISADMA_MODE_BLOCK	0x80
#define ISADMA_MODE_CASCADE	0xc0

status_t lock_isa_dma_channel( int nChannel );
status_t unlock_isa_dma_channel( int nChannel );

status_t start_isa_dma( int nChannel, void *pAddress, size_t nLength, int nMode );

