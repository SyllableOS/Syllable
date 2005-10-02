#ifndef __NVIDIA_H__
#define __NVIDIA_H__

#include <atheos/kernel.h>
#include <atheos/pci.h>

#include "../../../server/ddriver.h"

#include "riva_hw.h"

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


class Riva : public DisplayDriver
{
public:
	Riva( int nFd );
	~Riva();

	virtual area_id	Open();
	virtual void	Close();

        virtual int	GetScreenModeCount();
	virtual bool    GetScreenModeDesc( int nIndex, os::screen_mode* psMode );
	virtual int	SetScreenMode( os::screen_mode );
	virtual os::screen_mode GetCurrentScreenMode(); 
	
	virtual void	LockBitmap( SrvBitmap* pcDstBitmap, SrvBitmap* pcSrcBitmap, os::IRect cSrc, os::IRect cDst );
	
	virtual bool	DrawLine( SrvBitmap* psBitMap, const os::IRect& cClipRect,
				  const os::IPoint& cPnt1, const os::IPoint& cPnt2, const os::Color32_s& sColor, int nMode );
	virtual bool	FillRect( SrvBitmap* psBitMap, const os::IRect& cRect, const os::Color32_s& sColor, int nMode );
	virtual bool	BltBitmap( SrvBitmap* pcDstBitMap, SrvBitmap* pcSrcBitMap, os::IRect cSrcRect, os::IRect cDstRect, int nMode, int nAlpha );

	virtual bool	CreateVideoOverlay( const os::IPoint& cSize, const os::IRect& cDst, os::color_space eFormat, os::Color32_s sColorKey, area_id *pBuffer );
	virtual bool	RecreateVideoOverlay( const os::IPoint& cSize, const os::IRect& cDst, os::color_space eFormat, area_id *pBuffer );
	virtual void	DeleteVideoOverlay( area_id *pBuffer );

	bool		IsInitiated() const { return( m_bIsInitiated ); }
private:

	bool		m_bIsInitiated;
	PCI_Info_s	m_cPCIInfo;
	os::Locker	m_cGELock;
	
	RIVA_HW_INST m_sHW;
	vuint8*		m_pRegisterBase;
	area_id		m_hRegisterArea;

	vuint8*		m_pFrameBufferBase;
	area_id		m_hFrameBufferArea;
	std::vector<os::screen_mode> m_cScreenModeList;
	os::screen_mode m_sCurrentMode;
    
    bool		m_bEngineDirty;
	bool		m_bPaletteEnabled;
	
	uint32		m_nColorKey;
	bool		m_bVideoOverlayUsed;

	void CRTCout(unsigned char index, unsigned char val)
	{
		VGA_WR08(m_sHW.PCIO, 0x3d4, index);
		VGA_WR08(m_sHW.PCIO, 0x3d5, val);
	}
    
	uint8 CRTCin(unsigned char index)
	{
		uint8 res;
		VGA_WR08(m_sHW.PCIO, 0x3d4, index);
		res = VGA_RD08(m_sHW.PCIO, 0x3d5);
		return res;
	}
	
	void GRAout(unsigned char index, unsigned char val)
	{
		VGA_WR08(m_sHW.PVIO, 0x3ce, index);
		VGA_WR08(m_sHW.PVIO, 0x3cf, val);
	}

	uint8 GRAin(unsigned char index)
	{
		uint8 res;
		VGA_WR08(m_sHW.PVIO, 0x3ce, index);
		res = VGA_RD08(m_sHW.PVIO, 0x3cf);
		return res;
	}
	
	void SEQout(unsigned char index, unsigned char val)
	{
		VGA_WR08(m_sHW.PVIO, 0x3c4, index);
		VGA_WR08(m_sHW.PVIO, 0x3c5, val);
	}
	
	uint8 SEQin(unsigned char index)
	{
		uint8 res;
		VGA_WR08(m_sHW.PVIO, 0x3c4, index);
		res = VGA_RD08(m_sHW.PVIO, 0x3c5);
		return res;
	}
	
	void ATTRout(unsigned char index, unsigned char val)
	{
		uint8 tmp;
		tmp = VGA_RD08(m_sHW.PCIO, 0x3da);
		if (m_bPaletteEnabled) {
			index &= ~0x20;
		} else {
			index |= 0x20;
		}
		VGA_WR08(m_sHW.PCIO, 0x3c0, index);
		VGA_WR08(m_sHW.PCIO, 0x3c0, val);
	}
	
	uint8 ATTRin(unsigned char index)
	{
		uint8 tmp, res;
		tmp = VGA_RD08(m_sHW.PCIO, 0x3da);
		if (m_bPaletteEnabled) {
			index &= ~0x20;
		} else {
			index |= 0x20;
		}
		VGA_WR08(m_sHW.PCIO, 0x3c0, index);
		res = VGA_RD08(m_sHW.PCIO, 0x3c1);
		return res;
	}
	
	void MISCout(unsigned char val)
	{
		VGA_WR08(m_sHW.PVIO, 0x3c2, val);
	}
	
	uint8 MISCin()
	{
		uint8 res;
		res = VGA_RD08(m_sHW.PVIO, 0x3cc);
		return res;
	}
	
	void EnablePalette()
	{
		uint8 tmp;
		tmp = VGA_RD08(m_sHW.PCIO, 0x3da);
		VGA_WR08(m_sHW.PCIO, 0x3c0, 0x00);
		m_bPaletteEnabled = true;
	}
	
	void DisablePalette()
	{
		uint8 tmp;
		tmp = VGA_RD08(m_sHW.PCIO, 0x3da);
		VGA_WR08(m_sHW.PCIO, 0x3c0, 0x20);
		m_bPaletteEnabled = false;
	}
	
	void WaitForIdle()
	{
		while (m_sHW.Busy(&m_sHW));
	}
	
	void		LoadState(struct riva_regs *regs);
	void		SetupAccel();
	void		SetupROP();

	void		NVCommonSetup();
	void		NV3Setup();
	void		NV4Setup();
};

#endif // __NVIDIA_H__





