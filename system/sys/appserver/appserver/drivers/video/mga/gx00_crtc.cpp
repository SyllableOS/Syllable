#include	"gx00_crtc.h"
#include	<stdio.h>

#include <atheos/isa_io.h>
#define outp(a,b) outb(b,a)
#define inp(a) inb(a)

#define	SIZE	(8192<<10)

void	*	CGx00CRTC::s_pMemory;

CGx00CRTC::CGx00CRTC( Millenium2 &Gx00Device ) : m_Device(Gx00Device)
{
}

CGx00CRTC::~CGx00CRTC()
{
}


/*
long	CGx00CRTC::ReadRegisterDWORD( UWORD iReg )
{
	m_Device.WriteConfigDWORD( 0x44, iReg );
	return m_Device.ReadConfigDWORD( 0x48 );
}
*/

void	CGx00CRTC::SetMGAMode()
{
	outp( 0x3DE, 3 );
	outp( 0x3DF, inp(0x3DF)|0x80 );
}

void	CGx00CRTC::CRTC_WriteEnable()
{
	outp( 0x3D4, 0x11 );
	outp( 0x3D5, inp(0x3D5)&0x7F );
}

void	CGx00CRTC::GCTL_Write( int iReg, UBYTE iValue )
{
	outp( 0x3CE, iReg );
	outp( 0x3CF, iValue );
}

void	CGx00CRTC::CRTC_Write( int iReg, UBYTE iValue )
{
	outp( 0x3D4, iReg );
	outp( 0x3D5, iValue );
}

void	CGx00CRTC::CRTCEXT_Write( int iReg, UBYTE iValue )
{
	outp( 0x3DE, iReg );
	outp( 0x3DF, iValue );
}

void	CGx00CRTC::SEQ_Write( int iReg, UBYTE iValue )
{
	outp( 0x3C4, iReg );
	outp( 0x3C5, iValue );
}

void	CGx00CRTC::X_Write( int iReg, UBYTE iValue )
{
	Register_WriteBYTE( 0x3C00, UBYTE(iReg) );
	Register_WriteBYTE( 0x3C0A, iValue );
}

void	CGx00CRTC::Register_WriteBYTE( int iReg, UBYTE iValue )
{
	m_Device.WriteConfigDWORD( 0x44, iReg&0xFFFC );
	m_Device.WriteConfigBYTE( UBYTE(0x48+(iReg&3)), iValue );
}

void	CGx00CRTC::WaitVBlank()
{
	while( (inp(0x3DA)&0x08)==0 ) {}
}

void	CGx00CRTC::Display_UnBlank()
{
	outp( 0x3C4, 0x01 );
	outp( 0x3C5, inp(0x3C5)&~0x20 );
}

void	CGx00CRTC::Display_Blank()
{
	outp( 0x3C4, 0x01 );
	outp( 0x3C5, inp(0x3C5)|0x20 );
}