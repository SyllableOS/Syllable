#ifndef __NVIDIA_H__
#define __NVIDIA_H__

#include <atheos/kernel.h>
#include <atheos/pci.h>

#include "../../../server/ddriver.h"

#include "riva_hw.h"
#include "nvreg.h"
#include "nv_type.h"

#define MAX_CURS		32
#define CURS_WHITE		0xffff
#define CURS_BLACK		0x8000
#define CURS_TRANSPARENT	0x0000

#define NUM_SEQ_REGS		0x05
#define NUM_CRT_REGS		0x41
#define NUM_GRC_REGS		0x09
#define NUM_ATC_REGS		0x15

struct chip_info
{
	uint32 nDeviceId;
	uint32 nArchRev;
	char *pzName;
};

struct riva_regs
{
	uint8 attr[NUM_ATC_REGS];
        uint8 crtc[NUM_CRT_REGS];
	uint8 gra[NUM_GRC_REGS];
	uint8 seq[NUM_SEQ_REGS];
	uint8 misc_output;
	RIVA_HW_STATE ext;
};

struct video_mode
{
	int nXres, nYres;
	int nBpp; // bytes per pixel
	os::color_space eColorSpace;
};

class NVidia : public DisplayDriver
{
public:
	NVidia();
	~NVidia();

	virtual area_id	Open();
	virtual void	Close();

        virtual int	GetScreenModeCount();
	virtual bool    GetScreenModeDesc( int nIndex, ScreenMode* psMode );

	virtual int	SetScreenMode( int nWidth, int nHeight, os::color_space eColorSpc,
				       int nPosH, int nPosV, int nSizeH, int nSizeV, float vRefreshRate );
	virtual int	GetHorizontalRes();
	virtual int	GetVerticalRes();
	virtual int	GetBytesPerLine();
	virtual os::color_space GetColorSpace();  
	virtual void	SetColor(int nIndex, const Color32_s& sColor);

	virtual void	SetCursorBitmap( os::mouse_ptr_mode eMode, const os::IPoint& cHotSpot, const void* pRaster, int nWidth, int nHeight );
	virtual void	MouseOn( void );
	virtual void    MouseOff( void );
	virtual void    SetMousePos( os::IPoint cNewPos );	
	
	virtual bool	IntersectWithMouse(const IRect& cRect);

	virtual bool	DrawLine( SrvBitmap* psBitMap, const os::IRect& cClipRect,
				  const os::IPoint& cPnt1, const os::IPoint& cPnt2, const os::Color32_s& sColor, int nMode );
	virtual bool	FillRect( SrvBitmap* psBitMap, const os::IRect& cRect, const os::Color32_s& sColor );
	virtual bool	BltBitmap( SrvBitmap* pcDstBitMap, SrvBitmap* pcSrcBitMap, os::IRect cSrcRect, os::IPoint cDstPos, int nMode );

	bool		IsInitiated() const { return( m_bIsInitiated ); }
private:

	bool		m_bIsInitiated;
	PCI_Info_s	m_cPCIInfo;
	os::Locker	m_cGELock;
	
	NVRec		m_sRiva;
	vuint8*		m_pRegisterBase;
	area_id		m_hRegisterArea;

	vuint8*		m_pFrameBufferBase;
	area_id		m_hFrameBufferArea;
	std::vector<ScreenMode> m_cScreenModeList;
    
	bool		m_bPaletteEnabled;
	
	bool		m_bUsingHWCursor;
	bool		m_bCursorIsOn;
	os::IPoint	m_cCursorPos;
	os::IPoint	m_cCursorHotSpot;
	uint16		m_anCursorShape[MAX_CURS*MAX_CURS];
	
	void CRTCout(unsigned char index, unsigned char val)
	{
		VGA_WR08(m_sRiva.riva.PCIO, 0x3d4, index);
		VGA_WR08(m_sRiva.riva.PCIO, 0x3d5, val);
	}
    
	uint8 CRTCin(unsigned char index)
	{
		uint8 res;
		VGA_WR08(m_sRiva.riva.PCIO, 0x3d4, index);
		res = VGA_RD08(m_sRiva.riva.PCIO, 0x3d5);
		return res;
	}
	
	void GRAout(unsigned char index, unsigned char val)
	{
		VGA_WR08(m_sRiva.riva.PVIO, 0x3ce, index);
		VGA_WR08(m_sRiva.riva.PVIO, 0x3cf, val);
	}

	uint8 GRAin(unsigned char index)
	{
		uint8 res;
		VGA_WR08(m_sRiva.riva.PVIO, 0x3ce, index);
		res = VGA_RD08(m_sRiva.riva.PVIO, 0x3cf);
		return res;
	}
	
	void SEQout(unsigned char index, unsigned char val)
	{
		VGA_WR08(m_sRiva.riva.PVIO, 0x3c4, index);
		VGA_WR08(m_sRiva.riva.PVIO, 0x3c5, val);
	}
	
	uint8 SEQin(unsigned char index)
	{
		uint8 res;
		VGA_WR08(m_sRiva.riva.PVIO, 0x3c4, index);
		res = VGA_RD08(m_sRiva.riva.PVIO, 0x3c5);
		return res;
	}
	
	void ATTRout(unsigned char index, unsigned char val)
	{
		uint8 tmp;
		tmp = VGA_RD08(m_sRiva.riva.PCIO, 0x3da);
		if (m_bPaletteEnabled) {
			index &= ~0x20;
		} else {
			index |= 0x20;
		}
		VGA_WR08(m_sRiva.riva.PCIO, 0x3c0, index);
		VGA_WR08(m_sRiva.riva.PCIO, 0x3c0, val);
	}
	
	uint8 ATTRin(unsigned char index)
	{
		uint8 tmp, res;
		tmp = VGA_RD08(m_sRiva.riva.PCIO, 0x3da);
		if (m_bPaletteEnabled) {
			index &= ~0x20;
		} else {
			index |= 0x20;
		}
		VGA_WR08(m_sRiva.riva.PCIO, 0x3c0, index);
		res = VGA_RD08(m_sRiva.riva.PCIO, 0x3c1);
		return res;
	}
	
	void MISCout(unsigned char val)
	{
		VGA_WR08(m_sRiva.riva.PVIO, 0x3c2, val);
	}
	
	uint8 MISCin()
	{
		uint8 res;
		res = VGA_RD08(m_sRiva.riva.PVIO, 0x3cc);
		return res;
	}
	
	void EnablePalette()
	{
		uint8 tmp;
		tmp = VGA_RD08(m_sRiva.riva.PCIO, 0x3da);
		VGA_WR08(m_sRiva.riva.PCIO, 0x3c0, 0x00);
		m_bPaletteEnabled = true;
	}
	
	void DisablePalette()
	{
		uint8 tmp;
		tmp = VGA_RD08(m_sRiva.riva.PCIO, 0x3da);
		VGA_WR08(m_sRiva.riva.PCIO, 0x3c0, 0x20);
		m_bPaletteEnabled = false;
	}
	
	void WaitForIdle()
	{
		while (m_sRiva.riva.Busy(&m_sRiva.riva));
	}
	
	void		LoadState(struct riva_regs *regs);
	void		SetupAccel();
	void		SetupROP();

	// Stuff from "nv_setup.c"
	Bool		NVIsConnected(Bool second);
	void		NVOverrideCRTC();
	void		NVIsSecond();
	void		NVCommonSetup();
	void		NV1Setup();
	void		NV3Setup();
	void		NV4Setup();
	void		NV10Setup();
	void		NV20Setup();

};

#endif // __NVIDIA_H__




