#include <posix/errno.h>
#include <atheos/kernel.h>
#include <atheos/device.h>

#include "ac97audio.h"

#define ac97_read( driver, codec, reg ) driver->pfRead( driver->pDriverData, codec, reg )
#define ac97_write( driver, codec, reg, val ) driver->pfWrite( driver->pDriverData, codec, reg, val )
#define ac97_wait( driver, codec ) driver->pfWait( driver->pDriverData, codec )
#define ac97_add_mixer( driver, mixer, stereo, invert, codec, reg, scale ) \
	mixer.bStereo = stereo; \
	mixer.bInvert = invert; \
	mixer.nCodec = codec; \
	mixer.nReg = reg; \
	mixer.nScale = scale; \
	mixer.nValue = 0; \
	driver->asMixer[driver->nNumMixer++] = mixer;
	


/** Returns whether the codec supports variable samplingrates-
 * \par Description:
 * Returns whether the codec supports variable samplingrates-
 * \author	Arno Klenke
 *****************************************************************************/
bool ac97_supports_vra( AC97AudioDriver_s* psDriver, int nCodec )
{
	return( psDriver->nExtID[nCodec] & AC97_EI_VRA );
}
/** Returns whether the codec supports spdif.
 * \par Description:
 * Returns whether the codec supports spdif.
 * \author	Arno Klenke
 *****************************************************************************/
int ac97_supports_spdif( AC97AudioDriver_s* psDriver, int nCodec )
{
	return( psDriver->nExtID[nCodec] & AC97_EI_SPDIF );
}
/** Returns the maximal numbers of channels that all codecs together support.
 * \par Description:
 * Returns the maximal numbers of channels that all codecs together support.
 * \author	Arno Klenke
 *****************************************************************************/	
int ac97_get_max_channels( AC97AudioDriver_s* psDriver )
{
	return( psDriver->nMaxChannels );
}
/** Returns the maximal numbers of channels of one codec.
 * \par Description:
 * Returns the maximal numbers of channels of one codec.
 * \param nCodec - Codec.
 * \author	Arno Klenke
 *****************************************************************************/	
int ac97_get_channels( AC97AudioDriver_s* psDriver, int nCodec )
{
	return( psDriver->nChannels[nCodec] );
}

/** Writes a volume to a stereo mixer.
 * \par Description:
 * Writes a volume to a stereo mixer.
 * \param nCodec - Codec.
 * \param nReg - Register.
 * \param nLeft - Left volume. Value between 0 and 100.
 * \param nRight - Right volume. Value between 0 and 100.
 * \author	Arno Klenke
 *****************************************************************************/
static void ac97_write_stereo_mixer( AC97AudioDriver_s* psDriver, int nCodec, uint8 nReg, int nLeft, int nRight )
{
	/* Find mixer */
	int nScale = 32;
	AC97Mixer_s* psMixer = NULL;
	uint i;
	for( i = 0; i < psDriver->nNumMixer; i++ )
	{
		if( ( psDriver->asMixer[i].nCodec == nCodec ) && ( psDriver->asMixer[i].nReg == nReg ) )
		{
			nScale = ( 1 << psDriver->asMixer[i].nScale );
			psMixer = &psDriver->asMixer[i];
			break;
		}
	}
	
	if( psMixer == NULL ) {
		printk("Error: Could not find mixer %i:%x\n", nCodec, (uint)nReg );
		return;
	}
	
	if( nLeft == 0 && nRight == 0 )
		psMixer->nValue = AC97_MUTE;
	else
	{
		
		/* Calculate value */
		nRight = ( ( 100 - nRight ) * nScale ) / 100;
		nLeft = ( ( 100 - nLeft ) * nScale ) / 100;
		if( nRight >= nScale )
			nRight = nScale - 1;
		if( nLeft >= nScale )
			nLeft = nScale - 1;
		psMixer->nValue = ( nLeft << 8 ) | nRight;
	}
	//printf( "Write mixer %x::%x\n", (uint)nReg, (uint)pcMixer->m_nValue );
	ac97_write( psDriver, nCodec, nReg, psMixer->nValue );
}

/** Writes a volume to a mono mixer.
 * \par Description:
 * Writes a volume to a mono mixer.
 * \param nCodec - Codec.
 * \param nReg - Register.
 * \param nVolume - Volume. Value between 0 and 100.
 * \author	Arno Klenke
 *****************************************************************************/
static void ac97_write_mono_mixer( AC97AudioDriver_s* psDriver, int nCodec, uint8 nReg, int nVolume )
{
	/* Find mixer */
	int nScale = 32;
	AC97Mixer_s* psMixer = NULL;
	uint i;
	for( i = 0; i < psDriver->nNumMixer; i++ )
	{
		if( ( psDriver->asMixer[i].nCodec == nCodec ) && ( psDriver->asMixer[i].nReg == nReg ) )
		{
			nScale = ( 1 << psDriver->asMixer[i].nScale );
			psMixer = &psDriver->asMixer[i];
			break;
		}
	}
	
	if( psMixer == NULL ) {
		printk("Error: Could not find mixer %i:%x\n", nCodec, (uint)nReg );
		return;
	}
	
	if( nVolume == 0 )
		psMixer->nValue = AC97_MUTE;
	else
	{
		
		/* Calculate value */
		nVolume = ( ( 100 - nVolume ) * nScale ) / 100;
		if( nVolume >= nScale )
			nVolume = nScale - 1;
		psMixer->nValue = nVolume;
	}
	//printf( "Write mixer %x::%x\n", (uint)nReg, (uint)pcMixer->m_nValue );
	ac97_write( psDriver, nCodec, nReg, psMixer->nValue );
}

/** Writes a volume to the microphone mixer.
 * \par Description:
 * Writes a volume to the microphone mixer.
 * \param nCodec - Codec.
 * \param nValue - Volume. Value between 0 and 100.
 * \author	Arno Klenke
 *****************************************************************************/
static void ac97_write_mic_mixer( AC97AudioDriver_s* psDriver, int nCodec, int nValue )
{
	/* Find mixer */
	int nScale = 32;
	AC97Mixer_s* psMixer = NULL;
	uint i;
	for( i = 0; i < psDriver->nNumMixer; i++ )
	{
		if( ( psDriver->asMixer[i].nCodec == nCodec ) && ( psDriver->asMixer[i].nReg == AC97_MIC_VOL ) )
		{
			nScale = ( 1 << psDriver->asMixer[i].nScale );
			psMixer = &psDriver->asMixer[i];
			break;
		}
	}
	
	if( psMixer == NULL ) {
		printk("Error: Could not find mixer %i:%x\n", nCodec, AC97_MIC_VOL );
		return;
	}
	
	if( nValue == 0 )
		psMixer->nValue = AC97_MUTE;
	else
	{
		/* Calculate value */
		psMixer->nValue = psMixer->nValue & ~0x801f;
		nValue = ( ( 100 - nValue ) * nScale ) / 100;
		if( nValue >= nScale )
			nValue = nScale - 1;
		psMixer->nValue |= nValue;
	}
	//printk( "Write mixer %x::%x\n", AC97_MIC_VOL, (uint)psMixer->nValue );
	ac97_write( psDriver, nCodec, AC97_MIC_VOL, psMixer->nValue );
}


/** Enables or disables the spdif output.
 * \par Description:
 * Enables or disables the spdif output.
 * \par Note:
 * Use the AC97SupportsSPDIF() method first.
 * \param nCodec - Codec.
 * \param bEnable - Enable or disable.
 * \author	Arno Klenke
 *****************************************************************************/
status_t ac97_enable_spdif( AC97AudioDriver_s* psDriver, int nCodec, bool bEnable )
{
	if( !ac97_supports_spdif( psDriver, nCodec ) )
		return( -EINVAL );
	
	uint16 nVal = ac97_read( psDriver, nCodec, AC97_EXTENDED_STATUS );
	nVal &= ~AC97_EA_SPDIF;
	if( bEnable )
		nVal |= AC97_EA_SPDIF;
	ac97_write( psDriver, nCodec, AC97_EXTENDED_STATUS, nVal );
	printk( "EXT %x\n", (uint)nVal );
	
	return( 0 );
}

/** Sets the sampling rate.
 * \par Description:
 * Sets the sampling rate.
 * \param nCodec - Codec.
 * \param nReg - Register.
 * \param nRate - Sampling rate.
 * \author	Arno Klenke
 *****************************************************************************/
status_t ac97_set_rate( AC97AudioDriver_s* psDriver, int nCodec, uint8 nReg, uint nRate )
{
	/* Check double-rate rates */
	printk("Set rate %i\n", nRate );
	if( ( psDriver->nExtID[nCodec] & AC97_EI_DRA ) && nReg == AC97_PCM_FRONT_DAC_RATE )
	{
		uint16 nVal = ac97_read( psDriver, nCodec, AC97_EXTENDED_STATUS );
		nVal &= ~AC97_EA_DRA;
		if( nRate > 48000 )
			nVal |= AC97_EA_DRA;
		ac97_write( psDriver, nCodec, AC97_EXTENDED_STATUS, nVal );
	}
	
	if( nReg == AC97_SPDIF_CONTROL )
	{
		/* TODO: Check if this is right */
		uint16 nVal = AC97_SC_COPY;
		switch( nRate )
		{
			case 44100:
				nVal |= AC97_SC_SPSR_44K;
			break;
			case 48000:
				nVal |= AC97_SC_SPSR_48K;
			break;
			case 32000:
				nVal |= AC97_SC_SPSR_32K;
			break;
			default:
				ac97_enable_spdif( psDriver, nCodec, false );
				return( -EINVAL );
			break;
		}
		
		/* Disable SPDIF */
		ac97_enable_spdif( psDriver, nCodec, false );
		/* Write value */
		ac97_write( psDriver, nCodec, nReg, nVal );
		/* Enable spdif */
		ac97_enable_spdif( psDriver, nCodec, true );
		printk( "SPDIF programmed\n" );
		return( 0 );
	}
	
	ac97_write( psDriver, nCodec, nReg, nRate );
	uint16 nNewRate = ac97_read( psDriver, nCodec, nReg );
	
	if( nRate != nNewRate )
	{
		printk( "AC97SetRate(): Wanted %i, Got %i\n", (int)nRate, (int)nNewRate );
		return( -EINVAL );
	}
	
	return( 0 );
}

status_t ac97_resume_codec( AC97AudioDriver_s* psDriver, int nCodec )
{
	printk( "Resume AC97 codec #%i\n", nCodec );
	int i;
	ac97_wait( psDriver, nCodec );
	
	/* Powerup codec */
	ac97_write( psDriver, nCodec, AC97_POWER_CONTROL, 0 );
	ac97_write( psDriver, nCodec, AC97_RESET, 0 );
	snooze( 100 );
	ac97_write( psDriver, nCodec, AC97_POWER_CONTROL, 0 );
	ac97_write( psDriver, nCodec, AC97_GENERAL_PURPOSE, 0 );
	
	/* Restore status */
	ac97_write( psDriver, nCodec, AC97_EXTENDED_ID, psDriver->nExtID[nCodec] );
	ac97_write( psDriver, nCodec, AC97_EXTENDED_STATUS,psDriver->nExtStat[nCodec] );
	
	/* Restore mixer */
	for( i = 0; i < psDriver->nNumMixer; i++ )
	{
		AC97Mixer_s* psMixer = &psDriver->asMixer[i];
		ac97_write( psDriver, psMixer->nCodec, psMixer->nReg, psMixer->nValue );
	}
	return( 0 );
}

/** Resumes one card.
 * \par Description:
 * Resumes one card.
 * \param psDriver - AC97 driver structure.
 * \author	Arno Klenke
 *****************************************************************************/
status_t ac97_resume( AC97AudioDriver_s* psDriver )
{
	int i;
	for( i = 0; i < psDriver->nCodecs; i++ )
	{
		if( ac97_resume_codec( psDriver, i ) != 0 )
			return( -EIO );
	}
	return( 0 );
}


/**Suspends one card.
 * \par Description:
 * Suspends one card.
 * \param psDriver - AC97 driver structure.
 * \author	Arno Klenke
 *****************************************************************************/
status_t ac97_suspend( AC97AudioDriver_s* psDriver )
{
	int nCodec;
	for( nCodec = 0; nCodec < psDriver->nCodecs; nCodec++ )
	{
		ac97_write( psDriver, nCodec, AC97_POWER_CONTROL, AC97_PWR_D3 );
	}
	return( 0 );
}
/** Test the resolution of one volume control.
 * \par Description:
 * Test the resolution of one volume control.
 * \param nCodec - Codec.
 * \param nReg - Register.
 * \internal
 * \author	Arno Klenke
 *****************************************************************************/
static int ac97_test_resolution( AC97AudioDriver_s* psDriver, int nCodec, uint8 nReg )
{
	int nRes = 0;
	if( ac97_write( psDriver, nCodec, nReg, 0x0008 ) == 0 && ac97_read( psDriver, nCodec, nReg ) == 0x0008 )
		nRes = 4;
	if( ac97_write( psDriver, nCodec, nReg, 0x0010 ) == 0 && ac97_read( psDriver, nCodec, nReg ) == 0x0010 )
		nRes = 5;
	if( ac97_write( psDriver, nCodec, nReg, 0x0020 ) == 0 && ac97_read( psDriver, nCodec, nReg ) == 0x0020 )
		nRes = 6;
	//printk( "Register %x resolution %i\n", (uint)nReg, nRes );
	return( nRes );
}

/** Initializes one codec
 * \par Description:
 * Initializes one codec
 * \param nCodec - Codec.
 * \internal
 * \author	Arno Klenke
 *****************************************************************************/
status_t ac97_initialize_codec( AC97AudioDriver_s* psDriver, int nCodec )
{
	AC97Mixer_s sMixer;
	
	printk( "Initialize AC97 codec #%i\n", nCodec );
	ac97_wait( psDriver, nCodec );
	
	psDriver->nID[nCodec] = ac97_read( psDriver, nCodec, AC97_VENDOR_ID1 ) << 16;
	psDriver->nID[nCodec] |= ac97_read( psDriver, nCodec, AC97_VENDOR_ID2 );
	
	printk( "AC97 codec id: 0x%x\n", psDriver->nID[nCodec] );
	
	/* Check if this is a modem codec */
	uint16 nExtM = ac97_read( psDriver, nCodec, AC97_EXTENDED_MODEM_ID );
	if( nExtM & 1 )
	{
		printk( "Found AC97 modem codec\n" );
		return( -ENODEV );
	}
	
	/* Read caps */
	psDriver->nBasicID[nCodec] = ac97_read( psDriver, nCodec, AC97_RESET );
	psDriver->nExtID[nCodec] = ac97_read( psDriver, nCodec, AC97_EXTENDED_ID );
	
	/* Powerup codec */
	ac97_write( psDriver, nCodec, AC97_POWER_CONTROL, 0 );
	ac97_write( psDriver, nCodec, AC97_RESET, 0 );
	snooze( 100 );
	ac97_write( psDriver, nCodec, AC97_POWER_CONTROL, 0 );
	ac97_write( psDriver, nCodec, AC97_GENERAL_PURPOSE, 0 );
	
	/* Show version */
	if( psDriver->nExtID[nCodec] & AC97_EI_REV_23 )
		printk( "AC97 2.2 codec\n" );
	else if( psDriver->nExtID[nCodec] & AC97_EI_REV_22 )
		printk( "AC97 2.2 codec\n" );
	else
		printk( "AC97 2.0 codec\n" );
	
	/* Check for VRA */
	uint16 nExtStatus = ac97_read( psDriver, nCodec, AC97_EXTENDED_STATUS );
	if( psDriver->nExtID[nCodec] & ( AC97_EI_VRA | AC97_EI_VRM ) )
	{
		printk( "Variable bitrates supported\n" );
		nExtStatus |= psDriver->nExtID[nCodec] & ( AC97_EI_VRA | AC97_EI_VRM );
	}
	
	/* Check for Surround DAC */
	if( psDriver->nExtID[nCodec] & AC97_EI_SDAC )
	{
		printk( "Found surround DAC\n" );
		nExtStatus |= AC97_EA_SDAC;
		nExtStatus &= ~AC97_EA_PRJ;
	}
	
	/* Check for Center/LFE DAC */
	if( ( psDriver->nExtID[nCodec] & ( AC97_EI_CDAC | AC97_EI_LDAC ) ) == ( AC97_EI_CDAC | AC97_EI_LDAC ) )
	{
		printk( "Found center/LFE DAC\n" );
		nExtStatus |= psDriver->nExtID[nCodec] & ( AC97_EI_CDAC | AC97_EI_LDAC );
		nExtStatus &= ~( AC97_EA_PRI| AC97_EA_PRK );
	}
	
	bool bSurroundPossible = false;
	
	/* Configure AC97 2.2 codecs to emulate AMAP behaviour */
	if( psDriver->nExtID[nCodec] & AC97_EI_REV_22 )
	{
		psDriver->nExtID[nCodec] &= ~AC97_EI_DACS_SLOT_MASK;
		switch( nCodec )
		{
			case 1:
			case 2:
				psDriver->nExtID[nCodec] |= ( 1 << AC97_EI_DACS_SLOT_SHIFT );
				break;
			case 3:
				psDriver->nExtID[nCodec]|= ( 2 << AC97_EI_DACS_SLOT_SHIFT );
				break;
		}
		ac97_write( psDriver, nCodec, AC97_EXTENDED_ID, psDriver->nExtID[nCodec] );
		bSurroundPossible = true;
	}
	/* Check for AMAP support */
	else if( psDriver->nExtID[nCodec] & AC97_EI_AMAP ) {
		printk( "Codec supports AMAP\n" );
		bSurroundPossible = true;
	}
	
	/* We would need codec specific code to configure this */
	if( nCodec != 0 && !bSurroundPossible )
	{
		printk( "Cannot use this codec\n" );
		return( -ENODEV );
	}
	
	psDriver->nChannels[nCodec] = 2;
	
	/* Add surround channels */
	if( bSurroundPossible )
	{
		if( psDriver->nExtID[nCodec] & AC97_EI_SDAC ) {
			psDriver->nMaxChannels += 2;
			psDriver->nChannels[nCodec] += 2;
		}
		if( ( psDriver->nExtID[nCodec] & ( AC97_EI_CDAC | AC97_EI_LDAC ) ) == ( AC97_EI_CDAC | AC97_EI_LDAC ) ) {
			psDriver->nMaxChannels += 2;
			psDriver->nChannels[nCodec] += 2;
		}
	} else {
		psDriver->nExtID[nCodec] &= ~( AC97_EI_SDAC | AC97_EI_CDAC | AC97_EI_LDAC );
	}
	
	/* Check for SPDIF */
	if( psDriver->nExtID[nCodec] & AC97_EI_SPDIF )
		printk( "SPDIF output supported\n" );
	
	psDriver->nExtStat[nCodec] = nExtStatus;
	ac97_write( psDriver, nCodec, AC97_EXTENDED_STATUS, nExtStatus );
	
	/* Master */
	psDriver->nNumMixer = 0;
	ac97_add_mixer( psDriver, sMixer, true, true, nCodec, AC97_MASTER_VOL_STEREO, ac97_test_resolution( psDriver, nCodec, AC97_MASTER_VOL_STEREO ) );
	ac97_write_stereo_mixer( psDriver, nCodec, AC97_MASTER_VOL_STEREO, 0x43, 0x43 );

	
	/* Headphone */
	if( psDriver->nBasicID[nCodec] & AC97_BC_HEADPHONE )
	{
		ac97_add_mixer( psDriver, sMixer, true, true, nCodec, AC97_HEADPHONE_VOL, ac97_test_resolution( psDriver, nCodec, AC97_HEADPHONE_VOL ) );
		ac97_write_stereo_mixer( psDriver, nCodec, AC97_HEADPHONE_VOL, 0x43, 0x43 );

	}
	
	/* Sourround */
	if( psDriver->nExtID[nCodec] & AC97_EI_SDAC )
	{
		ac97_add_mixer( psDriver, sMixer, true, true, nCodec, AC97_SURROUND_MASTER, ac97_test_resolution( psDriver, nCodec, AC97_SURROUND_MASTER ) );
		ac97_write_stereo_mixer( psDriver, nCodec, AC97_SURROUND_MASTER, 0x43, 0x43 );
	}
	
	/* Center/LFE */
	if( ( psDriver->nExtID[nCodec] & ( AC97_EI_CDAC | AC97_EI_LDAC ) ) == ( AC97_EI_CDAC | AC97_EI_LDAC ) )
	{
		ac97_add_mixer( psDriver, sMixer, true, true, nCodec, AC97_CENTER_LFE_MASTER, ac97_test_resolution( psDriver, nCodec, AC97_CENTER_LFE_MASTER ) );
		ac97_write_stereo_mixer( psDriver, nCodec, AC97_CENTER_LFE_MASTER, 0x43, 0x43 );
	}
	
	/* PCM */
	ac97_add_mixer( psDriver, sMixer, true, true, nCodec, AC97_PCMOUT_VOL, ac97_test_resolution( psDriver, nCodec, AC97_PCMOUT_VOL ) );
	ac97_write_stereo_mixer( psDriver, nCodec, AC97_PCMOUT_VOL, 0x43, 0x43 );
	
	/* LineIn */
	ac97_add_mixer( psDriver, sMixer, true, true, nCodec, AC97_LINEIN_VOL, ac97_test_resolution( psDriver, nCodec, AC97_LINEIN_VOL ) );
	ac97_write_stereo_mixer( psDriver, nCodec, AC97_LINEIN_VOL, 0x43, 0x43 );
	
	/* CD */
	ac97_add_mixer( psDriver, sMixer, true, true, nCodec, AC97_CD_VOL, ac97_test_resolution( psDriver, nCodec, AC97_CD_VOL ) );
	ac97_write_stereo_mixer( psDriver, nCodec, AC97_CD_VOL, 0x43, 0x43 );
	
	/* AUX */
	ac97_add_mixer( psDriver, sMixer, true, true, nCodec, AC97_AUX_VOL, ac97_test_resolution( psDriver, nCodec, AC97_AUX_VOL ) );
	ac97_write_stereo_mixer( psDriver, nCodec, AC97_AUX_VOL, 0x43, 0x43 );
	
	/* Mic */
	ac97_add_mixer( psDriver, sMixer, false, true, nCodec, AC97_MIC_VOL, ac97_test_resolution( psDriver, nCodec, AC97_MIC_VOL ) );
	ac97_write_mic_mixer( psDriver, nCodec, 0x00 );
	
	/* Record Gain */
	ac97_add_mixer( psDriver, sMixer, true, false, nCodec, AC97_RECORD_GAIN, ac97_test_resolution( psDriver, nCodec, AC97_RECORD_GAIN ) );
	ac97_write_stereo_mixer( psDriver, nCodec, AC97_RECORD_GAIN, 0x43, 0x43 );
	
	/* Record Gain Mic */
	ac97_add_mixer( psDriver, sMixer, false, false, nCodec, AC97_RECORD_GAIN_MIC, ac97_test_resolution( psDriver, nCodec, AC97_RECORD_GAIN_MIC ) );
	ac97_write_mono_mixer( psDriver, nCodec, AC97_RECORD_GAIN_MIC, 0x43 );

	
	return( 0 );
}


/** Handle ac97 driver ioctl.
 * \par Description:
 * Handle ac97 driver ioctl.
 * \param psDriver - AC97 driver structure.
 * \param nCommand - Command.
 * \param pArgs - Args.
 * \param bFromKernel - Ioctl from kernel.
 * \author	Arno Klenke
 *****************************************************************************/
status_t ac97_ioctl( AC97AudioDriver_s* psDriver, uint32 nCommand, void *pArgs, bool bFromKernel )
{
	switch( nCommand )
	{
		case AC97_GET_CODEC_INFO:
			memcpy_to_user( pArgs, psDriver, sizeof( AC97AudioDriver_s ) );
			break;
		case AC97_WAIT:
		{
			AC97RegOp_s sOp;
			memcpy_from_user( &sOp, pArgs, sizeof( AC97RegOp_s ) );
			psDriver->pfWait( psDriver->pDriverData, sOp.nCodec );
		}
		break;
		case AC97_READ:
		{
			AC97RegOp_s sOp;
			memcpy_from_user( &sOp, pArgs, sizeof( AC97RegOp_s ) );
			sOp.nVal = ac97_read( psDriver, sOp.nCodec, sOp.nReg );
			memcpy_to_user( pArgs, &sOp, sizeof( AC97RegOp_s ) );
		}
		break;
		case AC97_WRITE:
		{
			AC97RegOp_s sOp;
			memcpy_from_user( &sOp, pArgs, sizeof( AC97RegOp_s ) );
			ac97_write( psDriver, sOp.nCodec, sOp.nReg, sOp.nVal );
		}
		break;
		default:
			return( -EINVAL );
	}
	return( 0 );
}

/** Initializes the ac97 driver.
 * \par Description:
 * Initializes the ac97 driver.
 * \param psDriver - AC97 driver structure.
 * \param pzID - A unique id string.
 * \param nNumCodecs - Number of codecs of the card.
 * \author	Arno Klenke
 *****************************************************************************/
status_t ac97_initialize( AC97AudioDriver_s* psDriver, char* pzID, int nNumCodecs )
{
	printk( "Initialize AC97 codecs\n" );
	int i;
	

	strncpy( psDriver->zID, pzID, AC97_NAME_LENGTH );
	psDriver->nCodecs = nNumCodecs;
	psDriver->nMaxChannels = 2;
	
	/* Intialize codecs */
	for( i = 0; i < nNumCodecs; ++i )
	{
		if( ac97_initialize_codec( psDriver, i ) == -EIO )
			return( -EIO );
	}
	printk( "Found %i channels\n", psDriver->nMaxChannels );
	
	return( 0 );
}


















