/*  Syllable Audio Interface (HDA Codec)
 *  Copyright (C) 2006 Arno Klenke
 *
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

#ifndef _F_HDA_AUDIODRIVER_H_
#define _F_HDA_AUDIODRIVER_H_

#include <vector>
#include <media/format.h>
#include <media/input.h>
#include <media/output.h>
#include <media/addon.h>
#include "hda_codec.h"

namespace os
{

class HDAAudioDriver;

class HDAAudioOutputStream : public os::MediaOutput
{
public:
	HDAAudioOutputStream( HDAAudioDriver* pcDriver, os::String zDevice, bool bSPDIF );
	~HDAAudioOutputStream();
	
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
	
	HDAAudioDriver* m_pcDriver;
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
#if 0
class HDAAudioInputStream : public os::MediaInput
{
public:
	HDAAudioInputStream( HDAAudioDriver* pcDriver, os::String zDevice );
	~HDAAudioInputStream();
	
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
	
	HDAAudioDriver* m_pcDriver;
	os::String m_zDevice;
	int m_nFd;
	
	
};
#endif


class HDAAudioDriver : public os::MediaAddon
{
public:
	
	HDAAudioDriver( os::String zDevice );
	virtual ~HDAAudioDriver();
	
	
	/* reimplemented from MediaAddon */
	status_t Initialize();
	os::String GetIdentifier();
	uint32 GetOutputCount();
	os::MediaOutput* GetOutput( uint32 nIndex );
	uint32 GetInputCount();
	os::MediaInput* GetInput( uint32 nIndex );
	
	os::View* GetConfigurationView();
	HDANode_s* GetNode( int nNode )
	{
		for( uint i = 0; i < m_asNodes.size(); i++ )
		{
			if( m_asNodes[i].nNode == nNode )
				return( &m_asNodes[i] );
		}
		return( NULL );
	}
	#if 0
	void HDAWriteStereoMixer( int nFd, int nCodec, uint8 nReg, int nLeft, int nRight );
	void HDAReadStereoMixer( int nFd, int nCodec, uint8 nReg, int *pnLeft, int *pnRight );
	void HDAWriteMonoMixer( int nFd, int nCodec, uint8 nReg, int nVolume );
	void HDAReadMonoMixer( int nFd, int nCodec, uint8 nReg, int *pnVolume );
	void HDAWriteMicMixer( int nFd, int nCodec, int nValue );
	void HDAReadMicMixer( int nFd, int nCodec, int *pnValue );	
	status_t HDAEnableSPDIF( int nFd, int nCodec, bool bEnable );
	

/** Returns whether the codec supports variable samplingrates-
 * \par Description:
 * Returns whether the codec supports variable samplingrates-
 * \author	Arno Klenke
 *****************************************************************************/
	bool HDASupportsVRA( int nCodec )
	{
		return( m_nExtID[nCodec] & HDA_EI_VRA );
	}
/** Returns whether the codec supports spdif.
 * \par Description:
 * Returns whether the codec supports spdif.
 * \author	Arno Klenke
 *****************************************************************************/
	bool HDASupportsSPDIF( int nCodec )
	{
		return( m_nExtID[nCodec] & HDA_EI_SPDIF );
	}
	
	status_t HDAWait( int nFd, int nCodec );

	status_t HDAWrite( int nFd, int nCodec, uint8 nReg, uint16 nVal );
	uint16 HDARead( int nFd, int nCodec, uint8 nReg );
	#endif
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
	uint32 GetCardFormats()
	{
		return( m_nCardFormats );
	}
private:
	friend class HDAChannel;
	#if 0
	os::String HDAGetSettingsName( int nCodec, uint8 nReg );
	//status_t HDALoadMixerSettings();
	//void HDASaveMixerSettings();
	void HDAWriteMixer( int nFd, int nCodec, uint8 nReg, uint16 nValue );
	#endif
	os::String m_zDevice;
	
	os::String m_zID;
	os::String m_zName;
	int m_nCardMaxChannels;
	int m_nCardMaxSampleRate;
	uint32 m_nCardFormats;
	bool m_bSPDIF;
	bool m_bRecord;
	
	int m_nDevHandle;
	std::vector<HDANode_s> m_asNodes;
	std::vector<HDAOutputPath_s> m_asOutputPaths;
	#if 0
	int m_nCodecs;
	uint32 m_nID[4];
	uint16 m_nBasicID[4];
	uint16 m_nExtID[4];
	int m_nChannels[4];
	
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
	#endif
};

};

#endif










































