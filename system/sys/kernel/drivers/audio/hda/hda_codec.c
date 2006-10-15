/*
 * Universal Interface for Intel High Definition Audio Codec
 *
 *	Copyright(c) 2006 Arno Klenke
 *
 * Copyright (c) 2004 Takashi Iwai <tiwai@suse.de>
 *
 *
 *  This driver is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This driver is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */
#include <posix/errno.h>

#include <atheos/kernel.h>
#include <atheos/device.h>
#include <atheos/pci.h>
#include <atheos/irq.h>
#include <atheos/semaphore.h>
#include <posix/signal.h>
#include <macros.h>

#include "hda.h"

/* Node types */
static char* g_azTypes[] =
{
	"Audio Out",
	"Audio In",
	"Mixer",
	"Selector",
	"Pin",
	"Power",
	"Volume knob",
	"Beep"
};

/* Config types */
static char* g_azConfigType[] =
{
	"Line out",
	"Speaker",
	"Headphone",
	"CD",
	"SPDIF out",
	"Other digital out",
	"Modem line",
	"Modem hand",
	"Line in",
	"Aux",
	"Mic in",
	"Telephony",
	"SPDIF in",
	"Other digital in"
};


/* Send one command to the codec and return the response */
static uint32 hda_codec_read( HDAAudioDriver_s* psDriver, int nCodec, int nNode, int nDirect,
						int nVerb, int nPara )
{
	if( hda_send_cmd( psDriver, nCodec, nNode, nDirect, nVerb, nPara ) != 0 )
		return( 0 );
	return( hda_get_response( psDriver ) );
}

/* Return the number of subnodes of one node */
static int hda_get_subnodes( HDAAudioDriver_s* psDriver, int nCodec, int nNode, int* pnStart )
{
	uint32 nVal = hda_codec_read( psDriver, nCodec, nNode, 0, AC_VERB_PARAMETERS, AC_PAR_NODE_COUNT );
	*pnStart = ( nVal >> 16 ) & 0x7fff;
	return( nVal & 0x7fff );
}

/* Unmute output */
static int hda_unmute_output( HDAAudioDriver_s* psDriver, int nCodec, int nNode )
{
	HDANode_s* psNode = &psDriver->pasNodes[nNode-psDriver->nNodesStart];
	uint32 nVal = ( psNode->nAmpOutCaps & AC_AMPCAP_NUM_STEPS) >> AC_AMPCAP_NUM_STEPS_SHIFT;
	uint32 nOfs = ( psNode->nAmpOutCaps & AC_AMPCAP_OFFSET) >> AC_AMPCAP_OFFSET_SHIFT;
	//printk( "Unmute output of node %i (%i, %i)\n", nNode, nVal, nOfs );
	//if( nVal >= nOfs )
	//	nVal -= nOfs;
	nVal /= 2;
	nVal |= AC_AMP_SET_LEFT | AC_AMP_SET_RIGHT | AC_AMP_SET_OUTPUT;
	
	hda_send_cmd( psDriver, nCodec, nNode, 0, AC_VERB_SET_AMP_GAIN_MUTE, nVal );
	//printk( "%x\n", hda_codec_read( psDriver, nCodec, nNode, 0, AC_VERB_GET_AMP_GAIN_MUTE, AC_AMP_GET_LEFT | AC_AMP_GET_RIGHT | AC_AMP_GET_OUTPUT ) );
	return( 0 );	
}


/* Unmute input */
static int hda_unmute_input( HDAAudioDriver_s* psDriver, int nCodec, int nNode, int nInput )
{
	HDANode_s* psNode = &psDriver->pasNodes[nNode-psDriver->nNodesStart];
	uint32 nVal = ( psNode->nAmpInCaps & AC_AMPCAP_NUM_STEPS) >> AC_AMPCAP_NUM_STEPS_SHIFT;
	uint32 nOfs = ( psNode->nAmpInCaps & AC_AMPCAP_OFFSET) >> AC_AMPCAP_OFFSET_SHIFT;
	//printk( "Unmute input %i of node %i (%i, %i)\n", nInput, nNode, nVal, nOfs );	
	//if( nVal >= nOfs )
		//nVal -= nOfs;
	nVal /= 2;
	nVal |= AC_AMP_SET_LEFT | AC_AMP_SET_RIGHT | AC_AMP_SET_INPUT /*| ( nInput << AC_AMP_SET_INDEX_SHIFT )*/;
	hda_send_cmd( psDriver, nCodec, nNode, 0, AC_VERB_SET_AMP_GAIN_MUTE, nVal );
	return( 0 );	
}


/* Select input */
static int hda_select_input( HDAAudioDriver_s* psDriver, int nCodec, int nNode, int nInput )
{
	//printk( "Select input %i of node %i\n", nInput, nNode );	
	hda_send_cmd( psDriver, nCodec, nNode, 0, AC_VERB_SET_CONNECT_SEL, nInput );
	return( 0 );
}

/* Fill connection entries */
static void hda_parse_connections( HDAAudioDriver_s* psDriver, int nCodec, int nNode )
{
	HDANode_s* psNode = &psDriver->pasNodes[nNode-psDriver->nNodesStart];
	int i, n;
	
	
	//if( !( psNode->nWidCaps & AC_WCAP_CONN_LIST ) )
//		return;
	uint32 nVal = hda_codec_read( psDriver, nCodec, nNode, 0, AC_VERB_PARAMETERS, AC_PAR_CONNLIST_LEN );
	
	if( nVal & AC_CLIST_LONG )
	{
		printk( "Error: Long connection lists not supported!\n" );
		return;
	}
	int nConnLen = nVal & AC_CLIST_LENGTH;
	int nMask = ( 1 << 7 ) - 1;
	if( nConnLen == 0 )
		return;
	if( nConnLen == 1 )
	{
		nVal = hda_codec_read( psDriver, nCodec, nNode, 0, AC_VERB_GET_CONNECT_LIST, 0 );
		nVal &= nMask;
		psNode->pasConns[psNode->nConns++] = &psDriver->pasNodes[nVal-psDriver->nNodesStart];
		printk( "    -> %i\n", nVal & nMask );
		return;
	}
	int nPrevNode = 0;
	for( i = 0; i < nConnLen; i++ )
	{
		if( ( i % 4 ) == 0 )
		{

			nVal = hda_codec_read( psDriver, nCodec, nNode, 0, AC_VERB_GET_CONNECT_LIST, i );
		}
		if( nVal & ( 1 << 7 ) )
		{
			/* Range of values */
			for( n = nPrevNode + 1; n <= ( nVal & nMask ); n++ )
			{
				psNode->pasConns[psNode->nConns++] = &psDriver->pasNodes[n-psDriver->nNodesStart];
				printk( "   R-> %i\n", nVal & nMask );
			}
		}
		else
		{
			psNode->pasConns[psNode->nConns++] = &psDriver->pasNodes[(nVal & nMask)-psDriver->nNodesStart];
			printk( "    -> %i\n", nVal & nMask );
		}
		nPrevNode = nVal & nMask;
		nVal >>= 8;
	}
}

/* Parse nodes */
static void hda_parse_nodes( HDAAudioDriver_s* psDriver, int nCodec )
{
	/* Parse nodes */
	int nNodes = 0;
	int nStart = 0;
	uint32 nAmpOutCaps = hda_codec_read( psDriver, nCodec, psDriver->nFg, 0, AC_VERB_PARAMETERS, AC_PAR_AMP_OUT_CAP );
	uint32 nAmpInCaps = hda_codec_read( psDriver, nCodec, psDriver->nFg, 0, AC_VERB_PARAMETERS, AC_PAR_AMP_IN_CAP );
	int i = 0;
	nNodes = hda_get_subnodes( psDriver, nCodec, psDriver->nFg, &nStart );
	psDriver->nNodes = nNodes;
	psDriver->nNodesStart = nStart;
	printk( "%i nodes starting at %i\n", nNodes, nStart );
	psDriver->pasNodes = kmalloc( sizeof( HDANode_s ) * nNodes, MEMF_KERNEL | MEMF_CLEAR );
	for( i = nStart; i < nStart + nNodes; i++ )
	{
		/* Read settings */
		HDANode_s* psNode = &psDriver->pasNodes[i-nStart];
		psNode->nNode = i;
		psNode->nWidCaps = hda_codec_read( psDriver, nCodec, i, 0, AC_VERB_PARAMETERS, AC_PAR_AUDIO_WIDGET_CAP );
		psNode->nType = ( psNode->nWidCaps & AC_WCAP_TYPE) >> AC_WCAP_TYPE_SHIFT;
		

		if( psNode->nType == AC_WID_PIN )
		{
			psNode->nPinCaps = hda_codec_read( psDriver, nCodec, i, 0, AC_VERB_PARAMETERS, AC_PAR_PIN_CAP );
			psNode->nPinCtl = hda_codec_read( psDriver, nCodec, i, 0, AC_VERB_GET_PIN_WIDGET_CONTROL, 0 );
			psNode->nDefCfg = hda_codec_read( psDriver, nCodec, i, 0, AC_VERB_GET_CONFIG_DEFAULT, 0 );
			//printk( "%x %x %x\n", psNode->nPinCaps, psNode->nPinCtl, psNode->nDefCfg );
		}
		if( psNode->nWidCaps & AC_WCAP_OUT_AMP )
		{
			if( psNode->nWidCaps & AC_WCAP_AMP_OVRD )
				psNode->nAmpOutCaps = hda_codec_read( psDriver, nCodec, i, 0, AC_VERB_PARAMETERS, AC_PAR_AMP_OUT_CAP );
			else
				psNode->nAmpOutCaps = nAmpOutCaps;
			
		}
		if( psNode->nWidCaps & AC_WCAP_IN_AMP )
		{
			if( psNode->nWidCaps & AC_WCAP_AMP_OVRD )
				psNode->nAmpInCaps = hda_codec_read( psDriver, nCodec, i, 0, AC_VERB_PARAMETERS, AC_PAR_AMP_IN_CAP );
			else
				psNode->nAmpInCaps = nAmpInCaps;
		}
	}
	/* Parse connections */
	for( i = 0; i < nNodes; i++ )
	{
		HDANode_s* psNode = &psDriver->pasNodes[i];
		if( psNode->nType < 8 )
			printk( "Node %i %s (widcaps %x)\n", psNode->nNode, g_azTypes[psNode->nType], psNode->nWidCaps );
		else
			printk( "Node %i vendor (widcaps %x)\n", psNode->nNode, psNode->nWidCaps );
		if( psNode->nWidCaps & AC_WCAP_OUT_AMP )
			printk( "AMP out flags %x\n", psNode->nAmpOutCaps );
		if( psNode->nWidCaps & AC_WCAP_IN_AMP )
			printk( "AMP in flags %x\n", psNode->nAmpInCaps );
		if( psNode->nType == AC_WID_PIN ) {
			printk( "Pin flags %x %x %x\n", psNode->nPinCaps, psNode->nPinCtl, psNode->nDefCfg );
			if( ( ( psNode->nDefCfg & AC_DEFCFG_DEVICE ) >> AC_DEFCFG_DEVICE_SHIFT )  < 0xff )
				printk( "-> %s\n",  g_azConfigType[( ( psNode->nDefCfg & AC_DEFCFG_DEVICE ) >> AC_DEFCFG_DEVICE_SHIFT )] );
		}
		hda_parse_connections( psDriver, nCodec, psDriver->nNodesStart + i );
	}
	
	
}

/* Clear search flag */
static status_t hda_clear_searched( HDAAudioDriver_s* psDriver, int nCodec )
{
	int i = 0;
	for( i = 0; i < psDriver->nNodes; i++ )
	{
		psDriver->pasNodes[i].bSearched = false;
	}
	return( 0 );
}

static status_t hda_find_output_path_node( HDAAudioDriver_s* psDriver, HDAOutputPath_s* psPath, int nCodec, HDANode_s* psNode )
{
	int i = 0;
	if( psNode->bSearched || psNode->bConfigured )
		return -1;
	psNode->bSearched = true;
	
	if( psNode->nType == AC_WID_AUD_OUT )
	{
		/* Audio output found */
		if( ( psNode->nWidCaps & AC_WCAP_DIGITAL ) )
			return( -1 );
		
		printk( "%i\n", psNode->nNode );
		if( psNode->nWidCaps & AC_WCAP_OUT_AMP )
		{
			psPath->nVolNode = psNode->nNode;
			psPath->nVolIndex = 0;
		}
		psPath->nDacNode = psNode->nNode;
		hda_unmute_output( psDriver, nCodec, psNode->nNode );
		psNode->bConfigured = true;
		return( 0 );
	}
	/* Try all nodes in the connection list */
	for( i = 0; i < psNode->nConns; i++ )
	{
		HDANode_s* psNextNode = psNode->pasConns[i];
		if( hda_find_output_path_node( psDriver, psPath, nCodec, psNextNode ) == 0 )
		{
			psNode->bConfigured = true;
			if( psNode->nConns > 1 )
				hda_select_input( psDriver, nCodec, psNode->nNode, i );
			hda_unmute_input( psDriver, nCodec, psNode->nNode, i );
			hda_unmute_output( psDriver, nCodec, psNode->nNode );
			printk( " ->%i\n", psNode->nNode );
			/* Save volume node */
			if( psPath->nVolNode == -1 )
			{
				if( psNode->nWidCaps & AC_WCAP_OUT_AMP )
				{
					psPath->nVolNode = psNode->nNode;
					psPath->nVolIndex = 0;
				}
				else if( psNode->nWidCaps & AC_WCAP_IN_AMP )
				{
					psPath->nVolNode = psNode->nNode;
					psPath->nVolIndex = i;
				}
			}
			return( 0 );
		}
	}
	/* Try to connect to a configured node */
	for( i = 0; i < psNode->nConns; i++ )
	{
		HDANode_s* psNextNode = psNode->pasConns[i];
		if( psNextNode->bConfigured )
		{
			/* Found */
			if( psNode->nConns > 1 )
				hda_select_input( psDriver, nCodec, psNode->nNode, i );
			printk( " ->%i (already configured)\n", psNextNode->nNode );
			return( 0 );
		}
	}
	return( -1 );
}

/* Find the available output paths for one output type */
static status_t hda_find_output_paths( HDAAudioDriver_s* psDriver, int nCodec, int nType )
{
	int nNode = -1;
	int nCnt = 0;
	do
	{
		/* Try to find the first unconfigured node with the lowest sequence number */
		int nSeq = 0xff;
		int i;
		nNode = -1;
		HDANode_s* psNode = NULL;
		for( i = 0; i < psDriver->nNodes; i++ )
		{
			psNode = &psDriver->pasNodes[i];
			if( !psNode->bConfigured && psNode->nType == AC_WID_PIN )
			{
				/* Check if this is a supported input */
				if( !( psNode->nPinCaps & AC_PINCAP_OUT ) )
					continue;
				/* Check type */
				int nDevice = ( psNode->nDefCfg & AC_DEFCFG_DEVICE ) >> AC_DEFCFG_DEVICE_SHIFT;
				if( !( nDevice == nType ) )
					continue;
				/* Read sequence number and compare it to the current one */
				int nNodeSeq = ( psNode->nDefCfg & AC_DEFCFG_SEQUENCE );
				if( nNodeSeq < nSeq )
				{
					nSeq = nNodeSeq;
					nNode = i;
				}
			}
		}
		
		if( nNode != -1 )
		{
			psNode = &psDriver->pasNodes[nNode];
			printk( "Try pin node %i\n", psNode->nNode );
			
			
			/* Clear path */
			HDAOutputPath_s* psPath = &psDriver->pasOutputPaths[psDriver->nOutputPaths];
			psPath->nDacNode = psPath->nVolNode = -1;
			
			/* Clear seached flag */
			hda_clear_searched( psDriver, nCodec );
			
			
			/* Try to find a path to an audio output */
			if( hda_find_output_path_node( psDriver, psPath, nCodec, psNode ) == 0 )
			{
				hda_unmute_output( psDriver, nCodec, psNode->nNode );
				hda_send_cmd( psDriver, nCodec, psNode->nNode, 0, AC_VERB_SET_PIN_WIDGET_CONTROL,
					AC_PINCTL_OUT_EN | AC_PINCTL_HP_EN );
				psPath->nPinNode = psNode->nNode;
				
				
				/* Get supported formats */
				if( psPath->nDacNode != -1 )
				{
					uint32 nFormats;
					if( psDriver->pasNodes[psPath->nDacNode-psDriver->nNodesStart].nWidCaps & AC_WCAP_FORMAT_OVRD )
						nFormats = hda_codec_read( psDriver, nCodec, psPath->nDacNode, 0, AC_VERB_PARAMETERS, AC_PAR_PCM );
					else
						nFormats = hda_codec_read( psDriver, nCodec, psDriver->nFg, 0, AC_VERB_PARAMETERS, AC_PAR_PCM );
					if( !( nFormats & AC_SUPPCM_BITS_16 ) )
					{
						printk( "Output does not support 16bit samples!\n" );
						continue;
					}
					psPath->nFormats = nFormats;
				}
				/* Save path */
				psDriver->nOutputPaths++;
				
				printk( "Output path found (DAC %i Volume %i:%i Pin %i)!\n", psPath->nDacNode,
				psPath->nVolNode, psPath->nVolIndex, psPath->nPinNode );
			}
			else
			{
				printk( "No output path found!\n" );
			}
			
			psNode->bConfigured = true;
		}
	} while( nNode != -1 && nCnt++ < 10 );
	return( 0 );
}

/* Configure the available output paths */
static status_t hda_configure_output_paths( HDAAudioDriver_s* psDriver, int nCodec )
{
	/* First Line out, then speaker and then headphone outputs */
	hda_find_output_paths( psDriver, nCodec, AC_JACK_LINE_OUT );
	hda_find_output_paths( psDriver, nCodec, AC_JACK_SPEAKER );
	hda_find_output_paths( psDriver, nCodec, AC_JACK_HP_OUT );
	return( 0 );
}

/* Powerup nodes */
static status_t hda_powerup_nodes( HDAAudioDriver_s* psDriver, int nCodec )
{
	int i = 0;
	for( i = 0; i < psDriver->nNodes; i++ )
	{
		if( psDriver->pasNodes[i].nWidCaps & AC_WCAP_POWER )
		{
			printk( "Powerup node %i\n", psDriver->pasNodes[i].nNode );
			hda_send_cmd( psDriver, nCodec, psDriver->pasNodes[i].nNode, 0, AC_VERB_SET_POWER_STATE, AC_PWRST_D0 );
		}
	}
	snooze( 10000 );
	return( 0 );
}

status_t hda_initialize_codec( HDAAudioDriver_s* psDriver, int nCodec )
{
	/* Read vendor ID */
	int nFgNodes = 0;
	int nFgStart = 0;
	int i = 0;
	
	uint32 nVendorID = hda_codec_read( psDriver, nCodec, AC_NODE_ROOT, 0, AC_VERB_PARAMETERS, AC_PAR_VENDOR_ID );
	printk( "Codec VendorID: %x\n", nVendorID );
	
	/* Read function types */
	nFgNodes = hda_get_subnodes( psDriver, nCodec, AC_NODE_ROOT, &nFgStart );
	printk( "%i function groups starting at node %i\n", nFgNodes, nFgStart );
	
	/* Find audio function type */
	psDriver->nFg = -1;
	for( i = nFgStart; i < nFgStart + nFgNodes; i++ )
	{
		uint32 nType = hda_codec_read( psDriver, nCodec, i, 0, AC_VERB_PARAMETERS, AC_PAR_FUNCTION_TYPE );
		if( ( nType & AC_FGT_TYPE ) == AC_GRP_AUDIO_FUNCTION )
		{
			psDriver->nFg = i;
			break;
		}
	}
	if( psDriver->nFg == -1 )
	{
		printk( "Could not find audio function group\n" );
		return( -1 );
	}
	printk( "Audio function group @ node %i\n", psDriver->nFg );
	
	/* Parse nodes */
	hda_parse_nodes( psDriver, nCodec );
	
	/* Configure output paths */
	hda_configure_output_paths( psDriver, nCodec );
	
	/* Powerup nodes */
	hda_powerup_nodes( psDriver, nCodec );
	
	return( 0 );
}




















































