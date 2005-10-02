#include "mach64.h"

int MixFromMode( int nMode )
{
	switch( nMode )
	{
		case DM_COPY:
			return( FRGD_MIX_S );
			break;
		case DM_INVERT:
			return( FRGD_MIX_NOT_D );
			break;
		default:
			return( FRGD_MIX_S );
			break;
	}
	return( FRGD_MIX_S );
}


//-----------------------------------------------------------------------------
bool ATImach64::FillRect(SrvBitmap *pcBitmap, const IRect &cRect, 
		      const Color32_s &sColor, int nMode) {
  
	if( pcBitmap->m_bVideoMem == false || nMode != DM_COPY ) {
		return( DisplayDriver::FillRect(pcBitmap, cRect, sColor, nMode) );
	}

	int dstx = cRect.left;
	unsigned int width = cRect.Width();

	uint32 nColor;
	switch( pcBitmap->m_eColorSpc ) {
		case CS_RGB16:
			nColor = COL_TO_RGB16 (sColor);
			break;
		case CS_RGB32:
		case CS_RGBA32:
			nColor = COL_TO_RGB32 (sColor);
			break;
		default:
			return( DisplayDriver::FillRect(pcBitmap, cRect, sColor, nMode) );
	}

	m_cLock.Lock();

	wait_for_fifo(4);
	aty_st_le32(DP_SRC, BKGD_SRC_BKGD_CLR | FRGD_SRC_FRGD_CLR | MONO_SRC_ONE);
	aty_st_le32(DST_CNTL, DST_LAST_PEL | DST_Y_TOP_TO_BOTTOM | DST_X_LEFT_TO_RIGHT);
	aty_st_le32(DP_MIX, FRGD_MIX_S);
	aty_st_le32(DP_FRGD_CLR, nColor);

	wait_for_fifo(2);
	aty_st_le32(DST_Y_X, (dstx << 16) | cRect.top);
	aty_st_le32(DST_HEIGHT_WIDTH, (width+1 << 16) | cRect.Height()+1);
	wait_for_idle();
	m_cLock.Unlock();
	return( true );
}
//-----------------------------------------------------------------------------         
bool ATImach64::DrawLine(SrvBitmap *psBitMap, const IRect &cClipRect, 
                          const IPoint &cPnt1, const IPoint &cPnt2, 
                          const Color32_s &sColor, int nMode) { 
      
	/* based upon mach64seg.c */
	if( psBitMap->m_bVideoMem == false || ( nMode != DM_COPY && nMode != DM_INVERT ) ) {
		return( DisplayDriver::DrawLine(psBitMap, cClipRect, cPnt1, cPnt2, sColor, nMode) );
	}

	uint32 nColor;
	switch( psBitMap->m_eColorSpc ) {
		case CS_RGB16:
			nColor = COL_TO_RGB16 (sColor);
			break;
		case CS_RGB32:
		case CS_RGBA32:
			nColor = COL_TO_RGB32 (sColor);
			break;
		default:
		return( DisplayDriver::DrawLine(psBitMap, cClipRect, cPnt1, cPnt2, sColor, nMode) );
	}

	int x1 = cPnt1.x;
	int y1 = cPnt1.y;
	int x2 = cPnt2.x;
	int y2 = cPnt2.y;
	register int dx, dy;
	register int minDelta, maxDelta;
	register int x_dir, y_dir, y_major;
	if( DisplayDriver::ClipLine( cClipRect, &x1, &y1, &x2, &y2 ) == false ) {
		return( false );
	}

	/* determine x & y deltas and x & y direction bits */
	if( x1 < x2 )
	{
		dx = x2 - x1;
		x_dir = DST_X_LEFT_TO_RIGHT;
	}
	else
	{
		dx = x1 - x2;
		x_dir = 0;
	}
	if( y1 < y2 )
	{
		dy = y2 - y1;
		y_dir = DST_Y_TOP_TO_BOTTOM;
	}
	else
	{
		dy = y1 - y2;
		y_dir = 0;
	}

	/* determine x & y min and max values; also determine y major bit */
	if( dx < dy )
	{
		minDelta = dx;
		maxDelta = dy;
		y_major = DST_Y_MAJOR;
	}
	else
	{
		minDelta = dy;
		maxDelta = dx;
		y_major = 0;
	}

	m_cLock.Lock();
	wait_for_fifo(6);
	aty_st_le32(DP_SRC, BKGD_SRC_BKGD_CLR | FRGD_SRC_FRGD_CLR | MONO_SRC_ONE);
	aty_st_le32(DP_MIX, MixFromMode( nMode ));
	aty_st_le32(DP_FRGD_CLR, nColor);

	wait_for_fifo(2);
	aty_st_le32(DST_CNTL, (y_major | y_dir | x_dir));
	aty_st_le32(DST_Y_X, (x1 << 16) | (y1 & 0x0000ffff));
	aty_st_le32(DST_BRES_ERR, ((minDelta << 1) - maxDelta));
	aty_st_le32(DST_BRES_INC, (minDelta << 1));
	aty_st_le32(DST_BRES_DEC, (0x3ffff - ((maxDelta - minDelta) <<1)));
	aty_st_le32(DST_BRES_LNTH, (maxDelta + 2));
	wait_for_idle();
	m_cLock.Unlock();
	return( true );
}
//-----------------------------------------------------------------------------         
bool ATImach64::BltBitmap(SrvBitmap *pcDstBitMap, SrvBitmap *pcSrcBitMap, 
                           IRect cSrcRect, IRect cDstRect, 
                           int nMode, int nAlpha) { 
 
	if( pcDstBitMap->m_bVideoMem == false ||
		pcSrcBitMap->m_bVideoMem == false || ( nMode != DM_COPY && nMode != DM_INVERT ) || ( cSrcRect.Size() != cDstRect.Size() ) ) {
		return( DisplayDriver::BltBitmap( pcDstBitMap, pcSrcBitMap,
                                         cSrcRect, cDstRect, nMode, nAlpha ) );
	}
    
    IPoint cDstPos = cDstRect.LeftTop();
	int srcx = cSrcRect.left;
	int srcy = cSrcRect.top;
	int dstx = cDstPos.x;
	int dsty = cDstPos.y;
	u_int width = cSrcRect.Width();
	u_int height = cSrcRect.Height();


	uint32 direction = 0;

	if( srcy < dsty ) {
		dsty += height;
		srcy += height;
	} else
		direction |= DST_Y_TOP_TO_BOTTOM;

	if (srcx < dstx) {
		dstx += width;
		srcx += width;
	} else
		direction |= DST_X_LEFT_TO_RIGHT;


	m_cLock.Lock();
	aty_st_le32(DST_CNTL, direction);
	wait_for_fifo(3);

	aty_st_le32(DP_SRC, FRGD_SRC_BLIT);
	aty_st_le32(DP_MIX, MixFromMode( nMode ));
	wait_for_fifo(4);
	aty_st_le32(SRC_Y_X, (srcx << 16) | srcy );
	aty_st_le32(SRC_HEIGHT1_WIDTH1, (width + 1 << 16) | height + 1);
	aty_st_le32(DST_Y_X, (dstx << 16) | dsty);
	aty_st_le32(DST_HEIGHT_WIDTH, (width + 1 << 16) | height + 1);
	wait_for_idle();
	m_cLock.Unlock();
	return( true );
}
