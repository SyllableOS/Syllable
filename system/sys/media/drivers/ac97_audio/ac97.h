/*  Syllable Audio Interface (AC97 Codec)
 *  Copyright (C) 2006 Arno Klenke
 *
 *  Copyright 2000 Silicon Integrated System Corporation
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */

#ifndef _F_AC97_AUDIODRIVER_H_
#define _F_AC97_AUDIODRIVER_H_

#include <vector>
#include <media/format.h>
#include <media/input.h>
#include <media/output.h>
#include <media/addon.h>
#include "ac97audio.h"

namespace os
{

class AC97AudioDriver;

class AC97AudioOutputStream : public os::MediaOutput
{
public:
	AC97AudioOutputStream( AC97AudioDriver* pcDriver, os::String zDevice, bool bSPDIF );
	~AC97AudioOutputStream();
	
	/* reimplemented from MediaOutput */
	bool			FileNameRequired();
	os::String		GetIdentifier();
	uint32			GetPhysicalType();
	os::View*		GetConfigurationView();
	uint32			GetOutputFormatCount();
	uint64			GetDelay( bool bNonSharedOnly );
	uint64			GetBufferSize( bool bNonSharedOnly );
	os::MediaFormat_s	GetOutputFormat( uint32 nIndex );
	status_t		Open( os::String zFileName );
	uint32			GetSupportedStreamCount();
	status_t		AddStream( os::String zName, os::MediaFormat_s sFormat );
	status_t		WritePacket( uint32 nIndex, os::MediaPacket_s* psPacket );
	void			Clear();
	void			Close();
	
	uint64			ConvertToTime( uint32 nBytes );
	
	AC97AudioDriver* m_pcDriver;
	os::String m_zDevice;
	int m_nFd;
	bool				m_bSPDIF;
	uint64				m_nFactor;
	int					m_nBufferSize;
	os::MediaFormat_s	m_sSrcFormat;
	os::MediaFormat_s	m_sDstFormat;
	bool				m_bResample;
	
	
	
	std::vector<os::MediaFormat_s> m_asFormats;
};

class AC97AudioInputStream : public os::MediaInput
{
public:
	AC97AudioInputStream( AC97AudioDriver* pcDriver, os::String zDevice );
	~AC97AudioInputStream();
	
	/* reimplemented from MediaOutput */
	bool				FileNameRequired();
	os::String		GetIdentifier();
	uint32			GetPhysicalType();	
	os::View*		GetConfigurationView();
	uint32			GetTrackCount();
	virtual bool		PacketBased();
	virtual bool		StreamBased();
	status_t		Open( os::String zFileName );
	void			Clear();
	void			Close();
	virtual uint32		GetStreamCount();
	virtual MediaFormat_s	GetStreamFormat( uint32 nIndex );
	virtual status_t		ReadPacket( MediaPacket_s* psPacket );
	virtual void			FreePacket( MediaPacket_s* psPacket );
	
	AC97AudioDriver* m_pcDriver;
	os::String m_zDevice;
	int m_nFd;
	
	
};



class AC97AudioDriver : public os::MediaAddon
{
public:
	
	AC97AudioDriver( os::String zDevice );
	virtual ~AC97AudioDriver();
	
	
	/* reimplemented from MediaAddon */
	status_t Initialize();
	os::String GetIdentifier();
	uint32 GetOutputCount();
	os::MediaOutput* GetOutput( uint32 nIndex );
	uint32 GetInputCount();
	os::MediaInput* GetInput( uint32 nIndex );
	
	void AC97WriteStereoMixer( int nFd, int nCodec, uint8 nReg, int nLeft, int nRight );
	void AC97ReadStereoMixer( int nFd, int nCodec, uint8 nReg, int *pnLeft, int *pnRight );
	void AC97WriteMonoMixer( int nFd, int nCodec, uint8 nReg, int nVolume );
	void AC97ReadMonoMixer( int nFd, int nCodec, uint8 nReg, int *pnVolume );
	void AC97WriteMicMixer( int nFd, int nCodec, int nValue );
	void AC97ReadMicMixer( int nFd, int nCodec, int *pnValue );	
	status_t AC97EnableSPDIF( int nFd, int nCodec, bool bEnable );
	os::View* GetConfigurationView();

/** Returns whether the codec supports variable samplingrates-
 * \par Description:
 * Returns whether the codec supports variable samplingrates-
 * \author	Arno Klenke
 *****************************************************************************/
	bool AC97SupportsVRA( int nCodec )
	{
		return( m_nExtID[nCodec] & AC97_EI_VRA );
	}
/** Returns whether the codec supports spdif.
 * \par Description:
 * Returns whether the codec supports spdif.
 * \author	Arno Klenke
 *****************************************************************************/
	bool AC97SupportsSPDIF( int nCodec )
	{
		return( m_nExtID[nCodec] & AC97_EI_SPDIF );
	}
/** Returns the maximal numbers of channels that all codecs together support.
 * \par Description:
 * Returns the maximal numbers of channels that all codecs together support.
 * \author	Arno Klenke
 *****************************************************************************/	
	int AC97GetMaxChannels()
	{
		return( m_nMaxChannels );
	}
/** Returns the maximum numbers of channels of one codec.
 * \par Description:
 * Returns the maximum numbers of channels of one codec.
 * \param nCodec - Codec.
 * \author	Arno Klenke
 *****************************************************************************/	
	int AC97GetChannels( int nCodec )
	{
		return( m_nChannels[nCodec] );
	}
	
	status_t AC97Wait( int nFd, int nCodec );

	status_t AC97Write( int nFd, int nCodec, uint8 nReg, uint16 nVal );
	uint16 AC97Read( int nFd, int nCodec, uint8 nReg );
/** Returns the maximum numbers of channels that the card supports.
 * \par Description:
 * Returns the maximum numbers of channels that the card supports.
 * \author	Arno Klenke
 *****************************************************************************/	
	int GetCardMaxChannels()
	{
		return( m_nCardMaxChannels );
	}
/** Returns the maximum samplerate that the card supports.
 * \par Description:
 * Returns the maximum samplerate that the card supports.
 * \author	Arno Klenke
 *****************************************************************************/	
	int GetCardMaxSampleRate()
	{
		return( m_nCardMaxSampleRate );
	}
private:
	friend class AC97Channel;
	os::String AC97GetSettingsName( int nCodec, uint8 nReg );
	//status_t AC97LoadMixerSettings();
	//void AC97SaveMixerSettings();
	void AC97WriteMixer( int nFd, int nCodec, uint8 nReg, uint16 nValue );
	
	os::String m_zDevice;
	os::String m_zID;
	os::String m_zName;
	int m_nDevHandle;
	int m_nCodecs;
	uint32 m_nID[4];
	uint16 m_nBasicID[4];
	uint16 m_nExtID[4];
	int m_nChannels[4];
	bool m_bSPDIF;
	bool m_bRecord;
	int m_nMaxChannels;
	int m_nCardMaxChannels;
	int m_nCardMaxSampleRate;
	class Mixer
	{
		public:
		Mixer()
		{
		}
		Mixer( bool bStereo, bool bInvert, os::String cName, int nCodec, uint8 nReg, uint32 nScale )
		{
			m_bStereo = bStereo;
			m_bInvert = bInvert;
			m_cName = cName;
			m_nCodec = nCodec,
			m_nReg = nReg;
			m_nScale = nScale;
		}
		bool m_bStereo;
		bool m_bInvert;
		os::String m_cName;
		int m_nCodec;
		uint8 m_nReg;
		uint32 m_nScale;
		uint16 m_nValue;
	};
	std::vector<Mixer> m_cMixers;
};

};

#endif




























