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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/isa_io.h>
#include "../../../server/bitmap.h"
#include "../../../server/sprite.h"

#include <atheos/pci.h>
// #include <atheos/pci_vendors.h>
#include <gui/bitmap.h>

#include "trident.h"


/*
 *  Trident Methods
 */

/*
 *  I/O Methods
 */
void TridentDriver::outl( uint32 nAddress, uint32 nValue ) const
{
    if ( m_bUseMMIO ) {
      *((vuint32*)(m_pMMIOBase + nAddress)) = nValue;
    } else {
      outl_p( nValue, nAddress );
    }
}

void TridentDriver::outb( uint32 nAddress, uint8 nValue ) const
{
    if ( m_bUseMMIO ) {
      *((vuint8*)(m_pMMIOBase + nAddress)) = nValue;
    } else {
      outb_p( nValue, nAddress );
    }
}

uint32 TridentDriver::inl( uint32 nAddress ) const
{
    if ( m_bUseMMIO ) {
      return( *((vuint32*)(m_pMMIOBase + nAddress)) );
    } else {
      return inl_p( nAddress );
    }
}

uint8  TridentDriver::inb( uint32 nAddress ) const
{
    if ( m_bUseMMIO ) {
      return( *((vuint8*)(m_pMMIOBase + nAddress)) );
    } else {
      return inb_p( nAddress );
    }
}


void TridentDriver::IdentifyChip()
{
    TridentID found = UNKNOWN_CHIP;
    uint8 temp;
    uint8 origVal;
    uint8 newVal;

    // Check first that we have a Trident card.
    outb(0x3C4, 0x0B);
    temp = inb(0x3C5);  // Save old value
    outb(0x3C4, 0x0B);  // Switch to Old Mode
    outb(0x3C5, 0x00);
    inb(0x3C5);         // Now to New Mode
    outb(0x3C4, 0x0E);
    origVal = inb(0x3C5);
    outb(0x3C5, 0x00);
    newVal = inb(0x3C5) & 0x0F;
    outb(0x3C5, (origVal ^ 0x02));

    // If it is not a Trident card, quit.
    if (newVal != 2) {
      outb(0x3C4, 0x0B);  // Restore value of 0x0B
      outb(0x3C5, temp);
      outb(0x3C4, 0x0E);
      outb(0x3C5, origVal);
    } else {
      // At this point, we are sure to have a trident card.
      outb(0x3C4, 0x0B);
      temp = inb(0x3C5);
      switch (temp) {
        // Unsupported chipsets :
        // case 0x01:
        //   found = TVGA8800BR;
        //   break;
        // case 0x02:
        //   found = TVGA8800CS;
        //   break;
        // case 0x03:
        //   found = TVGA8900B;
        //   break;
        // case 0x04:
        // case 0x13:
        //   found = TVGA8900C;
        //   break;
        // case 0x23:
        //   found = TVGA9000;
        //   break;
        // case 0x33:
        //   found = TVGA8900D;
        //   break;
        // case 0x43:
        //   found = TVGA9000I;
        //   break;
        // case 0x53:
        //   found = TVGA9200CXR;
        //   break;
        // case 0x63:
        //   found = TVGA9100B;
        //   break;
        // case 0x83:
        //   found = TVGA8200LX;
        //   break;
        // case 0x93:
        //   found = TGUI9400CXI;
        //   break;
        //
        // List of supported adapters (they provide linear framebuffer.):
        case 0xA3:
          found = CYBER9320;
          break;
        case 0x73:
        case 0xC3:
          found = TGUI9420DGI;
          break;
        case 0xD3:
          found = TGUI9660;
          break;
        case 0xE3:
          found = TGUI9440AGI;
          break;
        case 0xF3:
          found = TGUI9430DGI;
          break;
        default:
          dbprintf("Unrecognised Trident chipset : 0x%x.\n", temp );
          break;
      }
    }
    m_nChip = found;
}

void TridentDriver::DetectVideoMemory()
{
    uint8 videorammask;
    uint8 val;
    uint8 chipID;

    outb(0x3C4, 0x0B);
    chipID = inb(0x3C5);
    
    if (chipID > 0 && chipID < 0xE3) {
      videorammask = 0x07;
    } else {
      videorammask = 0x0F;
    }

    outb(0x3D4, 0x1F);
    val = inb(0x3D5);
    switch (val & videorammask) {
      case 3: m_nVideoMemory = 1024; break;
      case 4: m_nVideoMemory = 4096; break;
        // 8 MB really but only 4 MB are usable with HW cursor (?)
      case 7: m_nVideoMemory = 2048; break;
      case 15: m_nVideoMemory = 4096; break;
        // real 4 MB
      default: m_nVideoMemory = 1024; break;
        // Defaulting to 1 MB when unknown
    }

    dbprintf( "Available Video Memory: %ld\n", m_nVideoMemory );

}

const TridentCardDesc* TridentDriver::GetTridentCardDesc( TridentID chip ) const
{
    for ( int j = 0 ; TridentCardsDesc[j].CardName != NULL ; ++j ) {
      if ( TridentCardsDesc[j].CardID == chip ) {
        return &(TridentCardsDesc[j]);
      }
    }
    return NULL;
}


// This method should be called only for non-PCI cards.
// It detects Framebuffer and MMIO addresses.
// For PCI cards, we use the PCI data to get such addresses.
void TridentDriver::GetAddresses() {
  uint8 val = 0;
  uint32 addr = 0;
  uint8 protect = 0;

  // Unprotect registers
  outb(0x3D4, 0x11);
  protect = inb(0x3D5);
  outb(0x3D5, protect & 0x7F );

  // Ensure we are in new mode
  outb(0x3C4, 0x0B);
  inb(0x3C5);

  outb(0x3C4, 0x0E);
  outb(0x3C5, 0xC0);

  outb(0x3D4, 0x21);
  val = inb(0x3D5);

  addr = (( val & 0xF ) << 20 ) + (( val & 0xC0 ) << 18);

  dbprintf("Linear Register: %x, address: 0x%lx\n", val, addr);

  // Restore protection
  outb(0x3D4, 0x11);
  protect = inb(0x3D5);
  outb(0x3D5, protect | 0x80 );

  m_pFrameBufferBase = (uint8*) addr;

  // Currently I have no information about MMIO...
  m_pMMIOBase = NULL;
}


// This method enables Framebuffer.
// We must call it after each mode initialisation, since BIOS seems
// to disable it.
void TridentDriver::EnableFrameBuffer() {
  uint8 val = 0;
  uint8 protect = 0;

  // Unprotect registers
  outb(0x3D4, 0x11);
  protect = inb(0x3D5);
  outb(0x3D5, protect & 0x7F );

  // Ensure we are in new mode
  outb(0x3C4, 0x0B);
  inb(0x3C5);

  outb(0x3C4, 0x0E);
  outb(0x3C5, 0xC0);

  // Get Framebuffer register value
  outb(0x3D4, 0x21);
  val = inb(0x3D5);

  // Toogle framebuffer
  outb(0x3D4, 0x21);
  outb(0x3D5, val | 0x20);

  // Restore protection
  outb(0x3D4, 0x11);
  protect = inb(0x3D5);
  outb(0x3D5, protect | 0x80 );
}



bool TridentDriver::SetBIOSMode( uint16 num ) {
  struct RMREGS  rm;

  rm.EAX = num;
  realint( 0x10, &rm );
  if ( ! rm.EAX ) {
    dbprintf("Unable to call Vesa INT10 to set mode.\n");
    return false;
  }
  return true;
}


TridentDriver::TridentDriver()
{
    const TridentCardDesc *pCardDesc;
    bool bFound = false;

    m_bUseMMIO = false;
    m_nBusType = NONE;

    m_pFrameBufferBase = NULL;
    m_pMMIOBase = NULL;
    m_hFrameBufferArea = 0;
    m_hMMIOArea = 0;

    dbprintf("Starting probing Trident video cards...\n");

    // First, check PCI bus
    for ( int i = 0 ;  get_pci_info( &m_cCardInfo, i ) == 0 ; ++i )
    {
      dbprintf("PCI %d: Vendor: %x - Device: %x - Rev. %x.\n",
        i, m_cCardInfo.nVendorID, m_cCardInfo.nDeviceID, m_cCardInfo.nRevisionID);
      if ( m_cCardInfo.nVendorID == PCI_VENDOR_ID_TRIDENT )
      {
        for ( int j = 0 ; TridentCardsDesc[j].CardName != NULL ; ++j ) {
          if ( m_cCardInfo.nDeviceID == TridentCardsDesc[j].PciDeviceID ) {
            bFound = true;
            m_nBusType = PCI;
            m_nChip = TridentCardsDesc[j].CardID;
            break;
          }
        }
      }
    }

    if ( bFound ) {
      // A PCI card has been found.
      m_pFrameBufferBase = (uint8*) ( m_cCardInfo.u.h0.nBase0
                           & PCI_ADDRESS_MEMORY_32_MASK );
      m_pMMIOBase = (uint8*) (m_cCardInfo.u.h0.nBase1
                    & PCI_ADDRESS_IO_MASK);
    } else {
      // Try to find an ISA/VLB card
      IdentifyChip();

      if ( m_nChip == UNKNOWN_CHIP ) {
        bFound = false;
      } else {
        bFound = true;
        m_nBusType = ISA;
        // For ISA/VLB cards, we now need to detect the framebuffer base.
        GetAddresses();
      }
    }

    if ( ! bFound ) {
      dbprintf( "No Trident video card found\n" );
      throw exception();
    }

    // From here, we are sure to have at least one Trident Card.

    pCardDesc = GetTridentCardDesc( m_nChip );
    if ( pCardDesc != NULL ) {
      dbprintf("Found device : %s.\n", pCardDesc->CardName);
    } else {
      dbprintf("Internal Driver Logic Error #1\n");
    }

    // We detect the size of the video memory.
    // The value is stored in m_nVideoMemory

    DetectVideoMemory();

    // We scan each mode defined in TridentModes to check
    // if it is compatible with installed amount of video memory.
    // If it is, we add it to the list of available modes : m_cModes

    for ( int i = 0; TridentModes[i].ModeID > 0 ; ++i ) {
      if ( ( TridentModes[i].BPL * TridentModes[i].Height ) / 1024u
           <= m_nVideoMemory ) {
        m_cModes.push_back( ScreenMode( TridentModes[i].Width,
                                        TridentModes[i].Height,
                                        TridentModes[i].BPL,
                                        TridentModes[i].ColorSpace ) );
      }
    }

    // How much memory is required to map the framebuffer and the registers ?
    // We will set the framebuffer the same size as the real video memory.
    // For the registers, I don't know... Anyway, they are not used (yet).

    if ( m_pFrameBufferBase != NULL ) {
      m_hFrameBufferArea = create_area( "trident_fb", NULL,
                                        1024 * m_nVideoMemory,
                                        AREA_FULL_ACCESS, AREA_NO_LOCK );
      if ( m_hFrameBufferArea <= 0 ) {
        dbprintf("Unable to create Frame Buffer area\n");
        throw exception();
      }
      remap_area( m_hFrameBufferArea, (void*) m_pFrameBufferBase );
      dbprintf("Frame Buffer set to 0x%lx.\n", (uint32) m_pFrameBufferBase );
    } else {
      dbprintf("No Frame Buffer Address defined. Must Stop.\n");
      throw exception();
    }

    if ( m_pMMIOBase != NULL ) {
      m_hMMIOArea = create_area( "trident_io", NULL,
                                 1024 * 1024 * 1,
                                 AREA_FULL_ACCESS, AREA_NO_LOCK );
      if ( m_hMMIOArea <= 0 ) {
        dbprintf("Unable to create MMIO area\n");
        throw exception();
      }
      remap_area( m_hMMIOArea, (void*) m_pMMIOBase );
    }

    dbprintf( "Trident video card successfully initialized.\n" );
}

TridentDriver::~TridentDriver()
{
}

area_id TridentDriver::Open( void )
{
    return( m_hFrameBufferArea );
}

void TridentDriver::Close( void )
{
}

int TridentDriver::SetScreenMode( int nWidth, int nHeight, color_space eColorSpc,
				int nPosH, int nPosV, int nSizeH, int nSizeV, float vRefreshRate )
{
  char* clr = "INVALID DEPTH";

  // This long switch is used only to format ColorSpace to string for
  // the next dbprintf().
  switch( eColorSpc ) {
    case CS_CMAP8:
      clr = "CMAP8";
      break;
    case CS_RGB15:
      clr = "RGB15";
      break;
    case CS_RGB16:
      clr = "RGB16";
      break;
    case CS_RGB24:
      clr = "RGB24";
      break;
    case CS_RGB32:
      clr = "RGB32";
      break;
    default:
      break;
  }

  dbprintf("SetScreenMode( %d, %d, %s ) called.\n", nWidth, nHeight, clr);

  m_nCurrentMode = 0;
  
  for ( int i = 0 ; TridentModes[i].ModeID > 0 ; ++i ) {
    if ( TridentModes[i].Width == nWidth
         && TridentModes[i].Height == nHeight
         && TridentModes[i].ColorSpace == eColorSpc ) {
      m_nCurrentMode = TridentModes[i].ModeID;
    }
  }

  if ( m_nCurrentMode != 0 ) {
    SetBIOSMode( m_nCurrentMode );
    EnableFrameBuffer();
    return 0;
  } else {
    dbprintf( "Invalid mode.\n" );
    return -1;
  }
}

int TridentDriver::GetScreenModeCount()
{
  return( m_cModes.size() );
}

bool TridentDriver::GetScreenModeDesc( int nIndex, ScreenMode* psMode )
{
  if ( uint(nIndex) < m_cModes.size() ) {
    *psMode = m_cModes[nIndex];
    return( true );
  } else {
    return( false );
  }
}

int TridentDriver::GetHorizontalRes()
{
  for ( int i = 0 ; TridentModes[i].ModeID > 0 ; ++i ) {
    if ( TridentModes[i].ModeID == m_nCurrentMode ) {
      return TridentModes[i].Width;
    }
  }
  return 0;
}

int TridentDriver::GetVerticalRes()
{
  for ( int i = 0 ; TridentModes[i].ModeID > 0 ; ++i ) {
    if ( TridentModes[i].ModeID == m_nCurrentMode ) {
      return TridentModes[i].Height;
    }
  }
  return 0;
}

int TridentDriver::GetBytesPerLine()
{
  for ( int i = 0 ; TridentModes[i].ModeID > 0 ; ++i ) {
    if ( TridentModes[i].ModeID == m_nCurrentMode ) {
      return TridentModes[i].BPL;
    }
  }
  return 0;
}

color_space TridentDriver::GetColorSpace()
{
  for ( int i = 0 ; TridentModes[i].ModeID > 0 ; ++i ) {
    if ( TridentModes[i].ModeID == m_nCurrentMode ) {
      return TridentModes[i].ColorSpace;
    }
  }
  return CS_RGB16;
}

bool TridentDriver::IntersectWithMouse( const IRect& cRect )
{
    return( false );
}

void TridentDriver::SetColor( int nIndex, const Color32_s& sColor )
{
    outb( 0x3c8, nIndex);
    outb( 0x3c9, sColor.red >> 2);
    outb( 0x3c9, sColor.green >> 2 );
    outb( 0x3c9, sColor.blue >> 2 );
}

bool TridentDriver::DrawLine( SrvBitmap* psBitMap, const IRect& cClipRect,
			    const IPoint& cPnt1, const IPoint& cPnt2, const Color32_s& sColor, int nMode )
{
  return( DisplayDriver::DrawLine(psBitMap, cClipRect, cPnt1, cPnt2, sColor, nMode) );
}

bool TridentDriver::FillRect( SrvBitmap* psBitMap, const IRect& cRect, const Color32_s& sColor )
{
  return( DisplayDriver::FillRect(psBitMap, cRect, sColor) );
}

bool TridentDriver::BltBitmap( SrvBitmap* dstbm, SrvBitmap* srcbm, IRect cSrcRect, IPoint cDstPos, int nMode )
{
  return( DisplayDriver::BltBitmap(dstbm, srcbm, cSrcRect, cDstPos, nMode) );
}


extern "C" DisplayDriver* init_gfx_driver()
{
    dbprintf( "Trident video driver attempts to initialize\n" );

    try {
	DisplayDriver* pcDriver = new TridentDriver();
	return( pcDriver );
    }
    catch( exception&  cExc ) {
	return( NULL );
    }
}



