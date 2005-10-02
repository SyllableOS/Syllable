#include "mach64.h"


// Video Overlay support ported from :
/*
   mach64_vid - VIDIX based video driver for Mach64 and 3DRage chips
   Copyrights 2002 Nick Kurshev. This file is based on sources from
   GATOS (gatos.sf.net) and X11 (www.xfree86.org)
   Licence: GPL
   WARNING: THIS DRIVER IS IN BETTA STAGE
*/

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool ATImach64::CreateVideoOverlay( const os::IPoint& cSize, const os::IRect& cDst, 
								os::color_space eFormat, os::Color32_s sColorKey, area_id *pBuffer )
{
	if( ( ( eFormat == CS_YUV12 && m_bSupportsYUV ) ||
		  eFormat == CS_RGB32 ) && !m_bVideoOverlayUsed ) {
		
		
		/* Stop video */
		wait_for_fifo(14);
	    aty_st_le32(OVERLAY_SCALE_CNTL, 0x80000000);
    	aty_st_le32(OVERLAY_EXCLUSIVE_HORZ, 0);
    	aty_st_le32(OVERLAY_EXCLUSIVE_VERT, 0);
    	aty_st_le32(SCALER_H_COEFF0, 0x00002000);
    	aty_st_le32(SCALER_H_COEFF1, 0x0D06200D);
    	aty_st_le32(SCALER_H_COEFF2, 0x0D0A1C0D);
    	aty_st_le32(SCALER_H_COEFF3, 0x0C0E1A0C);
    	aty_st_le32(SCALER_H_COEFF4, 0x0C14140C);
	    aty_st_le32(VIDEO_FORMAT, 0xB000B);
	    aty_st_le32(0x0430, 0x0);
	    aty_st_le32(VBI_START_END, 0x0 );
	    aty_st_le32(OVERLAY_EXCLUSIVE_HORZ, 0x0 );
	    aty_st_le32(VIDEO_SYNC_TEST, 0x0 );
    	aty_st_le32(SCALER_TEST, 0x0);
    	 	
    	/* Calculate offset */
    	uint32 pitch = 0; 
    	uint32 totalSize = 0;
    	if( eFormat == CS_YUV12 ) {
    		pitch = (cSize.x + 8) & ~8;
    		totalSize = (pitch * cSize.y * 3) >> 1;
    	} else {
    		pitch = (cSize.x + 8) & ~8;
    		totalSize = (pitch * cSize.y * 4);
    	}
    	uint32 offset = PAGE_ALIGN( info.total_vram - totalSize - PAGE_SIZE );
		
		*pBuffer = create_area( "mach64_overlay", NULL, PAGE_ALIGN( totalSize ), AREA_FULL_ACCESS, AREA_NO_LOCK );
		remap_area( *pBuffer, (void*)(( info.frame_buffer_phys ) + offset) );
		
		
		/* Start video */
		uint32 key_msk = 0;
		
		switch( BitsPerPixel( m_sCurrentMode.m_eColorSpace ) )
		{
			case 16:
				key_msk = 0xFFFF;
				m_nColorKey = COL_TO_RGB16( sColorKey );
			break;
			case 32:
				key_msk = 0xFFFFFF;
				m_nColorKey = COL_TO_RGB32( sColorKey );
			break;
			
			default:
				delete_area( *pBuffer );
				return( false );
		}
		wait_for_fifo(5);
		aty_st_le32(SCALER_COLOUR_CNTL,0x00101000);
		aty_st_le32(OVERLAY_GRAPHICS_KEY_MSK, key_msk);
		aty_st_le32(OVERLAY_GRAPHICS_KEY_CLR, m_nColorKey);
		aty_st_le32(OVERLAY_KEY_CNTL,VIDEO_KEY_FN_TRUE|GRAPHIC_KEY_FN_EQ|CMP_MIX_AND);
		wait_for_fifo(14);
		aty_st_le32(OVERLAY_Y_X_START, (cDst.left << 16 )|cDst.top);
		aty_st_le32(OVERLAY_Y_X_END, (cDst.right << 16 )|cDst.bottom);
		uint32 ecp = ( aty_ld_pll( PLL_VCLK_CNTL ) & 0x30 ) >> 4;
		uint32 hinc = ( cSize.x << ( 12 + ecp ) ) / cDst.Width();
		uint32 vinc = cSize.y * ( 1 << 16 );
		vinc>>=4;
		vinc /= cDst.Height();
		aty_st_le32(OVERLAY_SCALE_INC, ( hinc << 16 ) | vinc);
		/*if( eFormat == CS_RGB32 ) {
			pitch >>= 2;
		}*/
		aty_st_le32(SCALE_BUF_PITCH, pitch );
		aty_st_le32(SCALER_HEIGHT_WIDTH, ( cSize.x << 16 ) | cSize.y );
		aty_st_le32(SCALER_BUF0_OFFSET, offset );
		aty_st_le32(SCALER_BUF0_OFFSET_U, offset + (( pitch * cSize.y + 15 )&~15) );
		aty_st_le32(SCALER_BUF0_OFFSET_V, offset + (( pitch * cSize.y * 5 / 4 + 15 )&~15) );
		aty_st_le32(SCALER_BUF1_OFFSET, offset );
		aty_st_le32(SCALER_BUF1_OFFSET_U, offset + (( pitch * cSize.y + 15 )&~15) );
		aty_st_le32(SCALER_BUF0_OFFSET_V, offset + (( pitch * cSize.y * 5 / 4 + 15 )&~15)  );
		wait_for_vblank();
		wait_for_fifo(4);
		aty_st_le32(OVERLAY_SCALE_CNTL, 0xC4000003);
		wait_for_idle();
		(void)aty_ld_le32( VIDEO_FORMAT );
		if( eFormat == CS_YUV12 ) {
    		aty_st_le32( VIDEO_FORMAT, 0x000A0000 );
    	} else {
    		aty_st_le32( VIDEO_FORMAT, 0x00060000 );
    	}
		
		
   	 	
		m_bVideoOverlayUsed = true;
		return( true );
	}
	return( false );
}


//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool ATImach64::RecreateVideoOverlay( const os::IPoint& cSize, const os::IRect& cDst, 
								os::color_space eFormat, area_id *pBuffer )
{
	
	if( ( eFormat == CS_YUV12 && m_bSupportsYUV ) || eFormat == CS_RGB32 ) {
		delete_area( *pBuffer );
		/* Calculate offset */
    	uint32 pitch = 0; 
    	uint32 totalSize = 0;
    	if( eFormat == CS_YUV12 ) {
    		pitch = (cSize.x + 8) & ~8;
    		totalSize = (pitch * cSize.y * 3) >> 1;
    	} else {
    		pitch = (cSize.x + 8) & ~8;
    		totalSize = (pitch * cSize.y * 4);
    	}
    	uint32 offset = PAGE_ALIGN( info.total_vram - totalSize - PAGE_SIZE );
		
		*pBuffer = create_area( "mach64_overlay", NULL, PAGE_ALIGN( totalSize ), AREA_FULL_ACCESS, AREA_NO_LOCK );
		remap_area( *pBuffer, (void*)(( info.frame_buffer_phys ) + offset) );
		
		
		/* Start video */
		wait_for_fifo(5);
		aty_st_le32(SCALER_COLOUR_CNTL,0x00101000);
		wait_for_fifo(14);
		aty_st_le32(OVERLAY_Y_X_START, (cDst.left << 16 )|cDst.top);
		aty_st_le32(OVERLAY_Y_X_END, (cDst.right << 16 )|cDst.bottom);
		uint32 ecp = ( aty_ld_pll( PLL_VCLK_CNTL ) & 0x30 ) >> 4;
		uint32 hinc = ( cSize.x << ( 12 + ecp ) ) / cDst.Width();
		uint32 vinc = cSize.y * ( 1 << 16 );
		vinc>>=4;
		vinc /= cDst.Height();
		aty_st_le32(OVERLAY_SCALE_INC, ( hinc << 16 ) | vinc);
		/*if( eFormat == CS_RGB32 ) {
			pitch >>= 2;
		}*/
		aty_st_le32(SCALE_BUF_PITCH, pitch );
		aty_st_le32(SCALER_HEIGHT_WIDTH, ( cSize.x << 16 ) | cSize.y );
		aty_st_le32(SCALER_BUF0_OFFSET, offset );
		aty_st_le32(SCALER_BUF0_OFFSET_U, offset + (( pitch * cSize.y + 15 )&~15) );
		aty_st_le32(SCALER_BUF0_OFFSET_V, offset + (( pitch * cSize.y * 5 / 4 + 15 )&~15) );
		aty_st_le32(SCALER_BUF1_OFFSET, offset );
		aty_st_le32(SCALER_BUF1_OFFSET_U, offset + (( pitch * cSize.y + 15 )&~15) );
		aty_st_le32(SCALER_BUF0_OFFSET_V, offset + (( pitch * cSize.y * 5 / 4 + 15 )&~15)  );
		wait_for_vblank();
		wait_for_fifo(4);
		aty_st_le32(OVERLAY_SCALE_CNTL, 0xC4000003);
		wait_for_idle();
		(void)aty_ld_le32( VIDEO_FORMAT );
		if( eFormat == CS_YUV12 ) {
    		aty_st_le32( VIDEO_FORMAT, 0x000A0000 );
    	} else {
    		aty_st_le32( VIDEO_FORMAT, 0x00060000 );
    	}
		
		m_bVideoOverlayUsed = true;
		return( true );
	}
	return( false );
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

void ATImach64::DeleteVideoOverlay( area_id *pBuffer )
{
	if( m_bVideoOverlayUsed ) {
		delete_area( *pBuffer );
		wait_for_fifo(14);
	    aty_st_le32(OVERLAY_SCALE_CNTL, 0x80000000);
    	aty_st_le32(OVERLAY_EXCLUSIVE_HORZ, 0);
    	aty_st_le32(OVERLAY_EXCLUSIVE_VERT, 0);
    	aty_st_le32(SCALER_H_COEFF0, 0x00002000);
    	aty_st_le32(SCALER_H_COEFF1, 0x0D06200D);
    	aty_st_le32(SCALER_H_COEFF2, 0x0D0A1C0D);
    	aty_st_le32(SCALER_H_COEFF3, 0x0C0E1A0C);
    	aty_st_le32(SCALER_H_COEFF4, 0x0C14140C);
	    aty_st_le32(VIDEO_FORMAT, 0xB000B);
    	aty_st_le32(SCALER_TEST, 0x0);
	}
	m_bVideoOverlayUsed = false;
}



























