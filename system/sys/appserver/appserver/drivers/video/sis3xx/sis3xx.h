#ifndef __SIS3XX_H__
#define __SIS3XX_H__

#include <atheos/kernel.h>
#include <atheos/pci.h>

#include "../../../server/ddriver.h"

extern "C" {
	#include "init.h"
};


struct chip_info
{
	uint16 nDeviceId;
};

class SiS3xx : public DisplayDriver
{
public:
	SiS3xx( int nFd );
	~SiS3xx();

	virtual area_id	Open();
	virtual void	Close();

	virtual int	GetScreenModeCount();
	virtual bool    GetScreenModeDesc( int nIndex, os::screen_mode* psMode );
	virtual int	SetScreenMode( os::screen_mode sMode );
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
	area_id SISStartVideo(short src_x, short src_y,
		short drw_x, short drw_y,
		short src_w, short src_h,
		short drw_w, short drw_h,
		int id, short width, short height, uint32 color_key );

	void SISStopVideo();
	void SISSetupImageVideo();

	bool			m_bIsInitiated;
	bool			m_bUsingHWCursor;
	PCI_Info_s		m_cPCIInfo;
	os::Locker		m_cGELock;
	bool			m_bEngineDirty;
	shared_info*	m_pHw;	
	
	vuint8*			m_pRomBase;
	area_id			m_hRomArea;
	
	vuint8*			m_pRegisterBase;
	area_id			m_hRegisterArea;

	vuint8*			m_pFrameBufferBase;
	area_id			m_hFrameBufferArea;
	std::vector<os::screen_mode> m_cScreenModeList;
	os::screen_mode m_sCurrentMode;
    

	uint32			m_nColorKey;
	bool			m_bVideoOverlayUsed;
	
};


#endif // __SIS3XX_H__




