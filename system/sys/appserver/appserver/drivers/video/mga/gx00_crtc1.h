#ifndef	CRTC1_H__
#define	CRTC1_H__

#include	"gx00_crtc.h"
#include	"mga.h"

class	SHWParms;
class	SModeParms;

class	CGx00CRTC1 : public CGx00CRTC
{
public:
					CGx00CRTC1( Millenium2 &Gx00Device ) : CGx00CRTC(Gx00Device) {}
	virtual			~CGx00CRTC1() {}

	virtual bool	SetMode( int iXRes, int iYRes, int iBPP, float fRefreshRate );

protected:
	bool			CalcMode( SHWParms *pDest, const SModeParms *pParms );
};

#endif