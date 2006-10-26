
/** Resamples audio data.
 * \author	Arno Klenke
 *****************************************************************************/

static uint32 Resample( os::MediaFormat_s sSrcFormat, os::MediaFormat_s sDstFormat, uint16* pDst, int nRingPos, int nRingSize, uint16* pSrc, uint32 nLength )
{
	uint32 nSkipBefore = 0;
	uint32 nSkipBehind = 0;
	uint32 nSampleMultiply = 1;
	uint32 nWriteSamples = 0;
	uint32 nSkipBetween = 1;
	
	/* Calculate skipping ( I just guess what is right here ) */
	if( sSrcFormat.nChannels == sDstFormat.nChannels )
	{
		nWriteSamples = sSrcFormat.nChannels;
	} else if( sSrcFormat.nChannels < sDstFormat.nChannels ) {
		if( sDstFormat.nChannels % sSrcFormat.nChannels != 0 )
		{
			printf( "Error: Cannot resample this stream (src: %i dst:%i )!\n", sSrcFormat.nChannels, sDstFormat.nChannels );
			return( 0 );
		}
		nSampleMultiply = sDstFormat.nChannels / sSrcFormat.nChannels;
		nWriteSamples = sSrcFormat.nChannels;
	} else if( sSrcFormat.nChannels == 3 && sDstFormat.nChannels == 2 ) {
		nSkipBefore = 0;
		nSkipBehind = 1;
		nWriteSamples = 2;
	} else if( sSrcFormat.nChannels == 4 && sDstFormat.nChannels == 2 ) {
		nSkipBefore = 0;
		nSkipBehind = 2;
		nWriteSamples = 2;
	} else if( sSrcFormat.nChannels == 5 && sDstFormat.nChannels == 2 ) {
		nSkipBefore = 0;
		nSkipBetween = 3;
		nSkipBehind = 0;
		nWriteSamples = 2;
	} else if( sSrcFormat.nChannels == 6 && sDstFormat.nChannels == 2 ) {
		nSkipBefore = 0;
		nSkipBetween = 4;
		nSkipBehind = 1;
		nWriteSamples = 2;
	} else if( sSrcFormat.nChannels > sDstFormat.nChannels ) {
		nSkipBehind = sSrcFormat.nChannels - sDstFormat.nChannels;
		nWriteSamples = sDstFormat.nChannels;
	} else {
		printf( "Error: Cannot resample this stream (src: %i dst:%i )!\n", sSrcFormat.nChannels, sDstFormat.nChannels );
		return( 0 );
	}
	
	/* Calculate samplerate factors */
	float vFactor = (float)sDstFormat.nSampleRate / (float)sSrcFormat.nSampleRate;
	float vSrcActiveSample = 0;
	float vSrcFactor = 0;
	uint32 nSamples = 0;
	
	vSrcFactor = 1 / vFactor;
	float vSamples = (float)nLength * vFactor / (float)sSrcFormat.nChannels / 2;
	nSamples = (uint32)vSamples;
	
	uint32 nSrcPitch;
	
	nSrcPitch = nSkipBefore + nWriteSamples + ( nSkipBetween - 1 ) + nSkipBehind;
	
	uint32 nBytes = 0;
	
	assert( ( nRingPos % 2 ) == 0 );
	uint16* pDstWrite = &pDst[nRingPos / 2];
	
	if( nSampleMultiply > 1 )
	{
		for( uint32 i = 0; i < nSamples; i++ ) 
		{
			uint16* pSrcWrite = &pSrc[nSrcPitch * (int)vSrcActiveSample];
			/* Skip */
			pSrcWrite += nSkipBefore;
		
			for( uint32 j = 0; j < nWriteSamples; j++ )
			{
				for( uint32 k = 0; k < nSampleMultiply; k++ )
				{
				
					*pDstWrite++ = *pSrcWrite;
					nRingPos += 2;
					if( nRingPos >= nRingSize )
					{
						nRingPos = 0;
						pDstWrite = pDst;
					}
				}
				pSrcWrite += nSkipBetween;
			}
			
			vSrcActiveSample += vSrcFactor;
			
		}
		nBytes = nSamples * nWriteSamples * nSampleMultiply * 2;
	}
	else
	{
		for( uint32 i = 0; i < nSamples; i++ ) 
		{
			uint16* pSrcWrite = &pSrc[nSrcPitch * (int)vSrcActiveSample];
			/* Skip */
			pSrcWrite += nSkipBefore;
		
			for( uint32 j = 0; j < nWriteSamples; j++ )
			{
				
				*pDstWrite++ = *pSrcWrite;
				nRingPos += 2;
				if( nRingPos >= nRingSize )
				{
					nRingPos = 0;
					pDstWrite = pDst;
				}
				pSrcWrite += nSkipBetween;
			}
			vSrcActiveSample += vSrcFactor;
		}
		nBytes = nSamples * nWriteSamples * 2;
	}
	
	//printf("Ring pos %i %i %i\n", nRingPos, nBytes, nLength );
	
	return( nBytes );
}
















