/*
 *  The AtheOS application server
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
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

#ifndef	__F_VESADRV_H__
#define	__F_VESADRV_H__

#include "../../../server/ddriver.h"

struct VesaMode : public os::screen_mode
 {
    VesaMode( int w, int h, int bbl, os::color_space cs, float rf, int mode, uint32 fb ) : os::screen_mode( w,h,bbl,cs, rf ) { m_nVesaMode = mode; m_nFrameBuffer = fb; }
    int	   m_nVesaMode;
    uint32 m_nFrameBuffer;
};

class M64VesaDriver : public DisplayDriver
{
public:

    M64VesaDriver( int nFd );
    ~M64VesaDriver();
    
    bool InitModes();

    area_id	Open();
    void	Close();

    virtual int	 GetScreenModeCount();
    virtual bool GetScreenModeDesc( int nIndex, os::screen_mode* psMode );
  
    int		SetScreenMode( os::screen_mode sMode );
    os::screen_mode GetCurrentScreenMode();
   
    bool		IntersectWithMouse( const os::IRect& cRect );

private:
    bool		SetVesaMode( uint32 nMode );

    int	   m_nCurrentMode;
    std::vector<VesaMode> m_cModeList;
};

#endif // __F_VESADRV_H__









