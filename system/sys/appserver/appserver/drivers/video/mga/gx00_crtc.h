#ifndef	CRTC_H__
#define	CRTC_H__

#include	"mga.h"

typedef uint8 UBYTE;
typedef uint16 UWORD;
typedef uint32 ULONG;

class	CGx00CRTC
{
public:
							CGx00CRTC( Millenium2 &Gx00Device );
	virtual					~CGx00CRTC();

	virtual bool			SetMode( int iXRes, int iYRes, int iBPP, float fRefreshRate )=0;

protected:
	void			SetMGAMode();
	void			CRTC_WriteEnable();
	void			GCTL_Write( int iReg, UBYTE iValue );
	void			CRTC_Write( int iReg, UBYTE iValue );
	void			CRTCEXT_Write( int iReg, UBYTE iValue );
	void			SEQ_Write( int iReg, UBYTE iValue );
	void			X_Write( int iReg, UBYTE iValue );
	void			Register_WriteBYTE( int iReg, UBYTE iValue );

	void			WaitVBlank();
	void			Display_UnBlank();
	void			Display_Blank();

//	long			ReadRegisterDWORD( UWORD iReg );
	void			WriteRegisterDWORD( UWORD iReg, ULONG iValue );

	Millenium2	&	m_Device;
	static void	*	s_pMemory;
};

#endif