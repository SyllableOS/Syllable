#include <atheos/kernel.h>
#include <atheos/mman.h>
#include <atheos/sysbase.h>


typedef	struct	MemHeader MemHeader_s;

struct	MemHeader
{
  MemHeader_s* mc_psNext;
  uint32      mc_nSize;
};

typedef	struct
{
  MemHeader_s* mh_First;
  void*	      mh_pStart;
  void*	      mh_pEnd;
  uint32      mh_Free;
} MemHeap_s;



static void* allocate( MemHeap_s *mh, uint32 RequestSize )
{
  uint32 MemSize;
  MemHeader_s* psHeader;
  MemHeader_s* psPrev;
  char*	 MemAdr = NULL;

  MemSize = ((RequestSize+7L) & ~7L);

  psPrev = (void*) &mh->mh_First;

  for ( psHeader = mh->mh_First ; psHeader != NULL ; psHeader = psHeader->mc_psNext )	/* Scan trough memory chunks	*/
  {
    if ((((uint32)psHeader) < ((uint32)mh->mh_pStart)) || ( ((uint32)psHeader)>((uint32)(mh->mh_pEnd)) ))	/* Check for invalid chunks	*/
    {
      printk("ERROR : Memory list insane\n");
      return( NULL );
    }

    if ( psHeader->mc_nSize == MemSize )	/* bulls eye, remove the chunk and give it away	*/
    {
      psPrev->mc_psNext = psHeader->mc_psNext;
      MemAdr=(void*)	psHeader;
      break;
    }

    if (psHeader->mc_nSize > MemSize)
    {
      psPrev->mc_psNext = (void*) (((uint32)(psPrev->mc_psNext))+MemSize);

      psPrev->mc_psNext->mc_psNext	= psHeader->mc_psNext;
      psPrev->mc_psNext->mc_nSize	= psHeader->mc_nSize - MemSize;
      MemAdr=(void*)psHeader;
      break;
    }
    psPrev=psHeader;
  }
  if ( NULL != MemAdr ) {
    mh->mh_Free -= MemSize;
  }
  return(MemAdr);
}

static void deallocate( MemHeap_s *mh, void *MemBlock, uint32 Size )
{
    MemHeader_s* psPrev;
    MemHeader_s* psHeader;
    uint32	 BlockSize;

    uint32	*Ptr;
    BlockSize = ((Size+7L) & ~7L);

    mh->mh_Free += BlockSize;

    Ptr = MemBlock;

    psHeader=(void*) MemBlock;
    psHeader->mc_nSize=BlockSize;
    psPrev=(void*) &mh->mh_First;

    if ( ((uint32)psHeader) < ((uint32)mh->mh_First) )	/* Before first chunk	*/
    {
	psHeader->mc_psNext=mh->mh_First;
	mh->mh_First = psHeader;

	if ( ((uint32)psHeader)+psHeader->mc_nSize == ((uint32)psHeader->mc_psNext) ) {	/* Join if possible	*/
	    psHeader->mc_nSize += psHeader->mc_psNext->mc_nSize;
	    psHeader->mc_psNext		= psHeader->mc_psNext->mc_psNext;
	}
	return;
    }
    else
    {
	MemHeader_s* psIter;
	for ( psIter = mh->mh_First ; psIter != NULL ; psIter = psIter->mc_psNext ) {
	      // Check for invalid memory chunks
	    if ( ((uint32)psIter) < ((uint32)mh->mh_pStart) || ((uint32)psIter) > ((uint32)mh->mh_pEnd) ) { 
		printk( "Error: deallocate() Invalid memory list!\n");
		return;
	    }

	    if (((uint32)psIter)>((uint32)psHeader))	/* Find area for insertion.	*/
	    {
		if ( ((uint32)psPrev) + psPrev->mc_nSize == ((uint32)psHeader) ) {	/* Join with previous if possible	*/
		    psPrev->mc_nSize += psHeader->mc_nSize;						/* Increase size	*/
		    if ( ((uint32)psPrev)+psPrev->mc_nSize == ((uint32)(psPrev->mc_psNext)) ) {	/* Join with next if possible	*/
			psPrev->mc_nSize += psPrev->mc_psNext->mc_nSize;	/* Increase size.	*/
			psPrev->mc_psNext = psPrev->mc_psNext->mc_psNext;		/* Skip this chunk	*/
		    }
		} else {	/* Not lined up width previous;	*/
		    psHeader->mc_psNext = psPrev->mc_psNext;
		    psPrev->mc_psNext   = psHeader;

		    if (((uint32)psHeader)+psHeader->mc_nSize==((uint32)(psHeader->mc_psNext)))	{ /* Join with next if possible	*/
			psHeader->mc_nSize += psHeader->mc_psNext->mc_nSize;
			psHeader->mc_psNext	  = psHeader->mc_psNext->mc_psNext;
		    }
		}
		return;
	    }
	    psPrev = psIter;
	}
    }
    psPrev->mc_psNext=psHeader;	/* Add node at end of list	*/
    psHeader->mc_psNext=0;

    if ( ((uint32)psPrev)+psPrev->mc_nSize == ((uint32)(psPrev->mc_psNext)) )
    {
	psPrev->mc_nSize += psPrev->mc_psNext->mc_nSize;
	psPrev->mc_psNext = psPrev->mc_psNext->mc_psNext;
    }
}
