/*
 *  S3 Chrome driver for Syllable
 *
 *
 *  Copyright 2007 Arno Klenke
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#ifndef	__S3_CHROME_H__
#define	__S3_CHROME_H__

#include <atheos/kernel.h>
#include <atheos/vesa_gfx.h>
#include <atheos/pci.h>

#include "../../../server/ddriver.h"

struct ChromeMode : public os::screen_mode
{
	ChromeMode( int w, int h, int bbl, os::color_space cs, float rf, int mode ) : os::screen_mode( w,h,bbl,cs, rf ) { m_nVesaMode = mode; }
	int	   m_nVesaMode;
};

class Chrome : public DisplayDriver
{
public:

    Chrome( int nFd );
    ~Chrome();
    
    area_id	Open();
    void	Close();

    virtual int	 GetScreenModeCount();
    virtual bool GetScreenModeDesc( int nIndex, os::screen_mode* psMode );
  
    int		SetScreenMode( os::screen_mode sMode );
    os::screen_mode GetCurrentScreenMode();
   
    virtual int		    GetFramebufferOffset() { return( 0 ); }

	bool		IsInitiated() const { return( m_bIsInitiated ); }
private:
    bool		InitModes();
    
    bool		m_bIsInitiated;
    PCI_Info_s	m_cPCIInfo;
	
	vuint8*		m_pFrameBufferBase;
	area_id		m_hFrameBufferArea;

	uint32		m_nActiveDevices;

    int	   m_nCurrentMode;
    std::vector<ChromeMode> m_cModeList;
    
  
};

#endif







