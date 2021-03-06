#include "mach64.h"


unsigned short cursor_lookup[256] = {
	0x0000, 0x0001, 0x0004, 0x0005, 0x0010, 0x0011, 0x0014, 0x0015, 
	0x0040, 0x0041, 0x0044, 0x0045, 0x0050, 0x0051, 0x0054, 0x0055, 
	0x0100, 0x0101, 0x0104, 0x0105, 0x0110, 0x0111, 0x0114, 0x0115, 
	0x0140, 0x0141, 0x0144, 0x0145, 0x0150, 0x0151, 0x0154, 0x0155, 
	0x0400, 0x0401, 0x0404, 0x0405, 0x0410, 0x0411, 0x0414, 0x0415, 
	0x0440, 0x0441, 0x0444, 0x0445, 0x0450, 0x0451, 0x0454, 0x0455, 
	0x0500, 0x0501, 0x0504, 0x0505, 0x0510, 0x0511, 0x0514, 0x0515, 
	0x0540, 0x0541, 0x0544, 0x0545, 0x0550, 0x0551, 0x0554, 0x0555, 
	0x1000, 0x1001, 0x1004, 0x1005, 0x1010, 0x1011, 0x1014, 0x1015, 
	0x1040, 0x1041, 0x1044, 0x1045, 0x1050, 0x1051, 0x1054, 0x1055, 
	0x1100, 0x1101, 0x1104, 0x1105, 0x1110, 0x1111, 0x1114, 0x1115, 
	0x1140, 0x1141, 0x1144, 0x1145, 0x1150, 0x1151, 0x1154, 0x1155, 
	0x1400, 0x1401, 0x1404, 0x1405, 0x1410, 0x1411, 0x1414, 0x1415, 
	0x1440, 0x1441, 0x1444, 0x1445, 0x1450, 0x1451, 0x1454, 0x1455, 
	0x1500, 0x1501, 0x1504, 0x1505, 0x1510, 0x1511, 0x1514, 0x1515, 
	0x1540, 0x1541, 0x1544, 0x1545, 0x1550, 0x1551, 0x1554, 0x1555, 
	0x4000, 0x4001, 0x4004, 0x4005, 0x4010, 0x4011, 0x4014, 0x4015, 
	0x4040, 0x4041, 0x4044, 0x4045, 0x4050, 0x4051, 0x4054, 0x4055, 
	0x4100, 0x4101, 0x4104, 0x4105, 0x4110, 0x4111, 0x4114, 0x4115, 
	0x4140, 0x4141, 0x4144, 0x4145, 0x4150, 0x4151, 0x4154, 0x4155, 
	0x4400, 0x4401, 0x4404, 0x4405, 0x4410, 0x4411, 0x4414, 0x4415, 
	0x4440, 0x4441, 0x4444, 0x4445, 0x4450, 0x4451, 0x4454, 0x4455, 
	0x4500, 0x4501, 0x4504, 0x4505, 0x4510, 0x4511, 0x4514, 0x4515, 
	0x4540, 0x4541, 0x4544, 0x4545, 0x4550, 0x4551, 0x4554, 0x4555, 
	0x5000, 0x5001, 0x5004, 0x5005, 0x5010, 0x5011, 0x5014, 0x5015, 
	0x5040, 0x5041, 0x5044, 0x5045, 0x5050, 0x5051, 0x5054, 0x5055, 
	0x5100, 0x5101, 0x5104, 0x5105, 0x5110, 0x5111, 0x5114, 0x5115, 
	0x5140, 0x5141, 0x5144, 0x5145, 0x5150, 0x5151, 0x5154, 0x5155, 
	0x5400, 0x5401, 0x5404, 0x5405, 0x5410, 0x5411, 0x5414, 0x5415, 
	0x5440, 0x5441, 0x5444, 0x5445, 0x5450, 0x5451, 0x5454, 0x5455, 
	0x5500, 0x5501, 0x5504, 0x5505, 0x5510, 0x5511, 0x5514, 0x5515, 
	0x5540, 0x5541, 0x5544, 0x5545, 0x5550, 0x5551, 0x5554, 0x5555
};


unsigned short cursor_mask[256] = {
	0xaaaa, 0xaaa8, 0xaaa2, 0xaaa0, 0xaa8a, 0xaa88, 0xaa82, 0xaa80, 
	0xaa2a, 0xaa28, 0xaa22, 0xaa20, 0xaa0a, 0xaa08, 0xaa02, 0xaa00, 
	0xa8aa, 0xa8a8, 0xa8a2, 0xa8a0, 0xa88a, 0xa888, 0xa882, 0xa880, 
	0xa82a, 0xa828, 0xa822, 0xa820, 0xa80a, 0xa808, 0xa802, 0xa800, 
	0xa2aa, 0xa2a8, 0xa2a2, 0xa2a0, 0xa28a, 0xa288, 0xa282, 0xa280, 
	0xa22a, 0xa228, 0xa222, 0xa220, 0xa20a, 0xa208, 0xa202, 0xa200, 
	0xa0aa, 0xa0a8, 0xa0a2, 0xa0a0, 0xa08a, 0xa088, 0xa082, 0xa080, 
	0xa02a, 0xa028, 0xa022, 0xa020, 0xa00a, 0xa008, 0xa002, 0xa000, 
	0x8aaa, 0x8aa8, 0x8aa2, 0x8aa0, 0x8a8a, 0x8a88, 0x8a82, 0x8a80, 
	0x8a2a, 0x8a28, 0x8a22, 0x8a20, 0x8a0a, 0x8a08, 0x8a02, 0x8a00, 
	0x88aa, 0x88a8, 0x88a2, 0x88a0, 0x888a, 0x8888, 0x8882, 0x8880, 
	0x882a, 0x8828, 0x8822, 0x8820, 0x880a, 0x8808, 0x8802, 0x8800, 
	0x82aa, 0x82a8, 0x82a2, 0x82a0, 0x828a, 0x8288, 0x8282, 0x8280, 
	0x822a, 0x8228, 0x8222, 0x8220, 0x820a, 0x8208, 0x8202, 0x8200, 
	0x80aa, 0x80a8, 0x80a2, 0x80a0, 0x808a, 0x8088, 0x8082, 0x8080, 
	0x802a, 0x8028, 0x8022, 0x8020, 0x800a, 0x8008, 0x8002, 0x8000, 
	0x2aaa, 0x2aa8, 0x2aa2, 0x2aa0, 0x2a8a, 0x2a88, 0x2a82, 0x2a80, 
	0x2a2a, 0x2a28, 0x2a22, 0x2a20, 0x2a0a, 0x2a08, 0x2a02, 0x2a00, 
	0x28aa, 0x28a8, 0x28a2, 0x28a0, 0x288a, 0x2888, 0x2882, 0x2880, 
	0x282a, 0x2828, 0x2822, 0x2820, 0x280a, 0x2808, 0x2802, 0x2800, 
	0x22aa, 0x22a8, 0x22a2, 0x22a0, 0x228a, 0x2288, 0x2282, 0x2280, 
	0x222a, 0x2228, 0x2222, 0x2220, 0x220a, 0x2208, 0x2202, 0x2200, 
	0x20aa, 0x20a8, 0x20a2, 0x20a0, 0x208a, 0x2088, 0x2082, 0x2080, 
	0x202a, 0x2028, 0x2022, 0x2020, 0x200a, 0x2008, 0x2002, 0x2000, 
	0x0aaa, 0x0aa8, 0x0aa2, 0x0aa0, 0x0a8a, 0x0a88, 0x0a82, 0x0a80, 
	0x0a2a, 0x0a28, 0x0a22, 0x0a20, 0x0a0a, 0x0a08, 0x0a02, 0x0a00, 
	0x08aa, 0x08a8, 0x08a2, 0x08a0, 0x088a, 0x0888, 0x0882, 0x0880, 
	0x082a, 0x0828, 0x0822, 0x0820, 0x080a, 0x0808, 0x0802, 0x0800, 
	0x02aa, 0x02a8, 0x02a2, 0x02a0, 0x028a, 0x0288, 0x0282, 0x0280, 
	0x022a, 0x0228, 0x0222, 0x0220, 0x020a, 0x0208, 0x0202, 0x0200, 
	0x00aa, 0x00a8, 0x00a2, 0x00a0, 0x008a, 0x0088, 0x0082, 0x0080, 
	0x002a, 0x0028, 0x0022, 0x0020, 0x000a, 0x0008, 0x0002, 0x0000, 
};


void ATImach64::SetCursorBitmap( mouse_ptr_mode eMode, const IPoint& cHotSpot, const void* pRaster, int nWidth, int nHeight )
{
	if( ( eMode != MPTR_MONO || nWidth > 64 || nHeight > 64 ) && HWcursor == true ) {
		MouseOff();
		HWcursor = false;
	}
	
	if( HWcursor == true ) {
		const uint8* bits = static_cast<const uint8*>(pRaster);
		uint16 *ram;
		int i, j, p;
		int image;
		uint8 mask;
		uint8 pixel;
		info.cursor->size.x = nWidth;
		info.cursor->size.y = nHeight;
		info.cursor->hot.x = cHotSpot.x;
		info.cursor->hot.y = cHotSpot.y;
		MouseOff();
		ram = (uint16 *)info.cursor->ram;
		image = 0;
		for(i = 0; i < info.cursor->size.y; i++) {
			for(j = 0; j < info.cursor->size.x / 8 + 1; j++) {
	 			pixel = 0x00;
	 			mask = 0x00;
	 			for(p = 0; p < 8; p++) {
					pixel>>=1;
	 				mask>>=1;
					if(j * 8 + p < info.cursor->size.x) {
	 					if(bits[image] == 0x03) { 
	 						pixel+=128; 
	 						mask+=128;
	 					}
						if(bits[image] == 0x02) { 
	 						mask+=128;
	 					}
	 					image++;
					} 
				}
				*ram++ = cursor_mask[mask] | cursor_lookup[pixel & mask];	
			}
      		for(; j < 8; j++)
	  			*ram++ = 0xaaaa;
		}
  		memset(ram, 0xaa, (64 - i) * 2 * 8);
  		MouseOn();
  	} else 
		return DisplayDriver::SetCursorBitmap(eMode, cHotSpot, pRaster, nWidth, nHeight);
}
//----------------------------------------------------------------
bool ATImach64::IntersectWithMouse(const IRect &cRect) { 
	return false;
}
// -------------------------------------------------------------
void ATImach64::MouseOn( void )
{
	if( HWcursor == true ) {
		uint16 xoff, yoff;
		int x, y;
		
		x = info.cursor->pos.x - info.cursor->hot.x - info.par.crtc.xoffset;
		if (x < 0) {
			xoff = -x;
			x = 0;
		} else {
			xoff = 0;
		}

		y = info.cursor->pos.y - info.cursor->hot.y - info.par.crtc.yoffset;
		if (y < 0) {
			yoff = -y;
			y = 0;
		} else {
			yoff = 0;
		}

		wait_for_fifo(4);
		aty_st_le32(CUR_OFFSET, (info.cursor->offset >> 3) + (yoff << 1));
		aty_st_le32(CUR_HORZ_VERT_OFF,
		    ((uint32)(64 - info.cursor->size.y + yoff) << 16) | xoff);
		aty_st_le32(CUR_HORZ_VERT_POSN, ((uint32)y << 16) | x);
		aty_st_le32(GEN_TEST_CNTL, aty_ld_le32(GEN_TEST_CNTL) | HWCURSOR_ENABLE);
		wait_for_idle();
	} else 
		return DisplayDriver::MouseOn();
}
//--------------------------------------------------------------------------
void ATImach64::MouseOff( void )
{
	if( HWcursor == true ) {
		wait_for_fifo(1);
		aty_st_le32(GEN_TEST_CNTL, aty_ld_le32(GEN_TEST_CNTL) & ~HWCURSOR_ENABLE);
		wait_for_idle();
	} else
		return DisplayDriver::MouseOn();
}
// --------------------------------------------------------------------------
void ATImach64::SetMousePos( IPoint cNewPos )
{
	if( HWcursor == true ) {
		info.cursor->pos.x = cNewPos.x;
		info.cursor->pos.y = cNewPos.y;
		MouseOn();	
	} else
		return DisplayDriver::SetMousePos(cNewPos);
}

// --------------------------------------------------------------------------
void ATImach64::aty_hw_cursor_init()
{  
	info.cursor = (struct ati_cursor *)malloc(sizeof(struct ati_cursor));
	if (!info.cursor)
		return;
	memset(info.cursor, 0, sizeof(ati_cursor));

	info.total_vram -= PAGE_SIZE;
	info.cursor->offset = info.total_vram;
	info.cursor->ram = (uint8 *)info.frame_buffer + info.cursor->offset;

	if (!info.cursor->ram) {
		free(info.cursor);
		return;
	}
	
	info.cursor->hot.x = 0;
	info.cursor->hot.y = 0;
	info.cursor->size.x = 0;
	info.cursor->size.y = 0;
	
	/* cursor color */
	
	wait_for_fifo(2);
	aty_st_le32(CUR_CLR0, 0);
	aty_st_le32(CUR_CLR1, ((uint32)255<<24)|((uint32)255<<16)|((uint32)255<<8));
	
	HWcursor = true; 
	dprintf("Mach64:: Hardware cursor enabled\n");
	
}


