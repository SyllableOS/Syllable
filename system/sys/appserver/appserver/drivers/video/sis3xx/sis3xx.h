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

struct video_mode
{
	int nXres, nYres;
	int nBpp; // bytes per pixel
	os::color_space eColorSpace;
};

class SiS3xx : public DisplayDriver
{
public:
	SiS3xx();
	~SiS3xx();

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

	bool			m_bIsInitiated;
	bool			m_bUsingHWCursor;
	PCI_Info_s		m_cPCIInfo;
	os::Locker		m_cGELock;
	shared_info*	m_pHw;	
	
	vuint8*			m_pRomBase;
	area_id			m_hRomArea;
	
	vuint8*			m_pRegisterBase;
	area_id			m_hRegisterArea;

	vuint8*			m_pFrameBufferBase;
	area_id			m_hFrameBufferArea;
	std::vector<ScreenMode> m_cScreenModeList;
    
	bool			m_bCursorIsOn;
	os::IPoint		m_cCursorPos;
	os::IPoint		m_cCursorHotSpot;
	uint8			m_anCursorShape[64*64*4];
	
};


#endif // __SIS3XX_H__




