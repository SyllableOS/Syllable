/*
 *  Trident video driver for Atheos
 *  2002-05-12 Damien Danneels
 *
 *  See README.TXT for general information.
 *
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

#ifndef _TRIDENT_H
#define _TRIDENT_H

#define VERSION "0.1a"

#include <atheos/types.h>
#include <atheos/pci.h>
#include <gui/guidefines.h>

#include "../../../server/ddriver.h"

using namespace os;


#define	B_NO_ERROR  0
#define	B_ERROR    -1


/*
 *  DECLARATIONS
 */

#define PCI_VENDOR_ID_TRIDENT       0x1023
#define PCI_DEVICE_ID_TRIDENT_8400  0x8400
#define PCI_DEVICE_ID_TRIDENT_8420  0x8420
#define PCI_DEVICE_ID_TRIDENT_8500  0x8500
#define PCI_DEVICE_ID_TRIDENT_9320  0x9320
#define PCI_DEVICE_ID_TRIDENT_9388  0x9388
#define PCI_DEVICE_ID_TRIDENT_9397  0x9397
#define PCI_DEVICE_ID_TRIDENT_939A  0x939A
#define PCI_DEVICE_ID_TRIDENT_9420  0x9420
#define PCI_DEVICE_ID_TRIDENT_9440  0x9440
#define PCI_DEVICE_ID_TRIDENT_9520  0x9520
#define PCI_DEVICE_ID_TRIDENT_9525  0x9525
#define PCI_DEVICE_ID_TRIDENT_9540  0x9540
#define PCI_DEVICE_ID_TRIDENT_9660  0x9660
#define PCI_DEVICE_ID_TRIDENT_9750  0x9750
#define PCI_DEVICE_ID_TRIDENT_9850  0x9850
#define PCI_DEVICE_ID_TRIDENT_9880  0x9880

typedef enum {
  NONE,
  ISA,
  PCI
} BusType;

typedef enum {
  TGUI9420DGI,
  TGUI9430DGI,
  TGUI9440AGI,
  TGUI9660,
  TGUI9680,
  PROVIDIA9682,
  PROVIDIA9685,
  CYBER9320,
  CYBER9382,
  CYBER9385,
  CYBER9388,
  CYBER9397,
  CYBER9397DVD,
  CYBER9520,
  CYBER9525DVD,
  CYBER9540,
  IMAGE975,
  IMAGE985,
  BLADE3D,
  CYBERBLADEI7,
  CYBERBLADEI7D,
  CYBERBLADEI1,
  UNKNOWN_CHIP
} TridentID;

typedef struct {
  TridentID  CardID;
  char      *CardName;
  uint16     PciDeviceID;
  uint8      Is3DChip;
} TridentCardDesc;


static TridentCardDesc TridentCardsDesc[] = {
  { CYBER9320, "CYBER 9320", PCI_DEVICE_ID_TRIDENT_9320, 0 },
  { CYBER9388, "CYBER 9388", PCI_DEVICE_ID_TRIDENT_9388, 1 },
  { CYBER9397, "CYBER 9397", PCI_DEVICE_ID_TRIDENT_9397, 1 },
  { CYBER9397DVD, "CYBER 9397DVD", PCI_DEVICE_ID_TRIDENT_939A, 1 },
  { CYBER9520, "CYBER 9520", PCI_DEVICE_ID_TRIDENT_9520, 1 },
  { CYBER9525DVD, "CYBER 9525DVD", PCI_DEVICE_ID_TRIDENT_9525, 1 },
  { CYBER9540, "CYBER 9540", PCI_DEVICE_ID_TRIDENT_9540, 0 },
  { TGUI9420DGI, "TGUI 9420DGi", PCI_DEVICE_ID_TRIDENT_9420, 0 },
  { TGUI9440AGI, "TGUI 9440AGi", PCI_DEVICE_ID_TRIDENT_9440, 0 },
  { TGUI9660, "TGUI 9660", PCI_DEVICE_ID_TRIDENT_9660, 0 },
  { TGUI9680, "TGUI 9680", PCI_DEVICE_ID_TRIDENT_9660, 0 },
  { PROVIDIA9682, "PROVIDIA 9682", PCI_DEVICE_ID_TRIDENT_9660, 0 },
  { PROVIDIA9685, "PROVIDIA 9685", PCI_DEVICE_ID_TRIDENT_9660, 0 },
  { CYBER9382, "CYBER 9382", PCI_DEVICE_ID_TRIDENT_9660, 0 },
  { CYBER9385, "CYBER 9385", PCI_DEVICE_ID_TRIDENT_9660, 0 },
  { IMAGE975, "IMAGE 975", PCI_DEVICE_ID_TRIDENT_9750, 1 },
  { IMAGE985, "IMAGE 985", PCI_DEVICE_ID_TRIDENT_9850, 1 },
  { BLADE3D, "BLADE 3D", PCI_DEVICE_ID_TRIDENT_9880, 1 },
  { CYBERBLADEI7, "CYBERBLADE i7", PCI_DEVICE_ID_TRIDENT_8400, 1 },
  { CYBERBLADEI7D, "CYBERBLADE i7D", PCI_DEVICE_ID_TRIDENT_8420, 1 },
  { CYBERBLADEI1, "CYBERBLADE i1", PCI_DEVICE_ID_TRIDENT_8500, 1 },
  { UNKNOWN_CHIP, NULL, 0, 0 }
};


typedef struct {
  uint16       ModeID;
  uint16       Width;
  uint16       Height;
  color_space  ColorSpace;
  uint16       BPL;        /* Bytes Per Line */
} ModeDesc;


static ModeDesc TridentModes[] = {
// Colorspaces CMAP8 and RGB24 are not handled correctly by AppServer.
//  { 0x5d, 640, 480, CS_CMAP8, 640 },
//  { 0x74, 640, 480, CS_RGB15, 1280 },
  { 0x75, 640, 480, CS_RGB16, 1280 },
//  { 0x6c, 640, 480, CS_RGB24, 1920 },

//  { 0x5e, 800, 600, CS_CMAP8, 800 },
//  { 0x76, 800, 600, CS_RGB15, 1600 },
  { 0x77, 800, 600, CS_RGB16, 1600 },
//  { 0x6d, 800, 600, CS_RGB24, 2400 },

//  { 0x62, 1024, 768, CS_CMAP8, 1024 },
//  { 0x78, 1024, 768, CS_RGB15, 2048 },
  { 0x79, 1024, 768, CS_RGB16, 2048 },

  { 0, 0, 0, CS_CMAP8, 0 }
};


/*
 *  MAIN CLASS
 */

class TridentDriver : public DisplayDriver
{
public:

    TridentDriver( int nFd );
    ~TridentDriver();

    virtual area_id Open();
    virtual void    Close();
    virtual int     GetScreenModeCount();
    virtual bool    GetScreenModeDesc( int nIndex, os::screen_mode* psMode );
    virtual int     SetScreenMode( os::screen_mode sMode );
    virtual os::screen_mode     GetCurrentScreenMode();

private:

    const TridentCardDesc* GetTridentCardDesc( TridentID chip ) const;
    void IdentifyChip();
    void DetectVideoMemory();
    void GetAddresses();
    void EnableFrameBuffer();
    bool SetBIOSMode( uint16 num );

    inline void   outl( uint32 nAddress, uint32 nValue ) const;
    inline void   outb( uint32 nAddress, uint8 nValue ) const;
    inline uint32 inl( uint32 nAddress ) const;
    inline uint8  inb( uint32 nAddress ) const;

    std::vector<os::screen_mode>	 m_cModes;
    os::screen_mode			m_sCurrentMode;
    
    // As a rule, we will only lock and unlock in public methods
    // of this driver because they are topmost and atomic functions
    os::Locker  m_cLock;

    bool        m_bUseMMIO;

    uint32      m_pFrameBufferBase;
    area_id     m_hFrameBufferArea;
    uint32      m_pMMIOBase;
    area_id     m_hMMIOArea;

    PCI_Info_s  m_cCardInfo;

// Technical data about the installed card
    TridentID   m_nChip;
    uint32      m_nVideoMemory;
    BusType     m_nBusType;

};


#endif // _TRIDENT_H
















