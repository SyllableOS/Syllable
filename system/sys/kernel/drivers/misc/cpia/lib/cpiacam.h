// Copyright 1999 Jesper Hansen. Free for use under the Gnu Public License
#ifndef DAMNREDNEX_CPIACAM_H
#define DAMNREDNEX_CPIACAM_H
//-----------------------------------------------------------------------------
#include <limits.h>

//-------------------------------------
#include <atheos/types.h>
typedef unsigned int uint;
#include <gui/bitmap.h>
#include <gui/locker.h>
//-------------------------------------
typedef struct cpiacmd cpiacmd;
//-----------------------------------------------------------------------------

//#define SHOWSTATUS

class CPiACam : public os::Locker
{
public:
    enum videosize_t { RES_QCIF=0, RES_CIF=1 }; // QCIF=176, CIF=352

    enum framerate_t { FRATE_25=0, FRATE_30=1 };
    enum framedivide_t { FDIV_1=0, FDIV_2=1, FDIV_4=2, FDIV_8=3 };

    enum subsampling_t { SUBSAMPLING_YUV420=0, SUBSAMPLING_YUV422=1 };

    enum compression_t { COMPRESSION_DISABLED=0, COMPRESSION_AUTO=1, COMPRESSION_MANUAL=2 };
    enum decimation_t { DECIMATION_OFF=0, DECIMATION_ON=1 };
    enum comprressiontarget_t { COMPTARGET_QUALITY=0, COMPTARGET_FRAMERATE=1 };

    CPiACam( const char *device );
    ~CPiACam();

    status_t		InitCheck() const;
	
    status_t		Reset();
	
    int 		GetFirmwareVersion();
    int 		GetFirmwareRevision();
    int 		GetVideoCompressorVersion();
    int			GetVideoCompressorRevision();
	
    status_t		SetVideoSize( videosize_t videosize,
				      uint roileft=0, uint roitop=0, uint roiright=INT_MAX, uint roibottom=INT_MAX );
	
    status_t		EnableAutoExposure( bool autoexp );
    status_t		SetExposure( float exposure, int gain );
    status_t		GetColorBalance( float *red, float *green, float *blue );
    status_t		GetExposure( float *exposure, int *gain );

    status_t		EnableAutoColorBalance( bool autocolorbalance );
    status_t		SetColorBalance( float red, float green, float blue );

    status_t		SetColorParms( float brightness, float contrast, float saturation );
	
    status_t		SetFrameRate( framerate_t rate, framedivide_t div );
    status_t		SetFrameRate( float vFramerate );
	
    status_t		EnableCompression( compression_t compression, decimation_t decimation );
    status_t		SetCompressionTarget( comprressiontarget_t target, int framerate, float quality );
    status_t		SetYUVTreshold( float ythreshold, float uvthreshold );

    status_t		SetApcor( uint correct1, uint intensity1, uint correct2, uint intensity2, uint correct3, uint intensity3, uint correct4, uint intensity4 );
    status_t		SetVLOffset( int offset1, int offset2, int offset3, int offset4 );
	
    status_t		CaptureFrame( os::Bitmap *bitmap, bool delta=false );
    status_t		CaptureFrame( void *data, int bytesperline, color_space colorspace=CS_RGB32, bool delta=false );
	
private:
    CPiACam( const CPiACam &other );
    CPiACam		&operator=( const CPiACam &other );

    status_t		Init();
	
//	void		AllocateBitmap();

    void		PrintCameraStatus( const char *string );

    status_t		ProcessFrame_YUV422_RGB32_NoComp( uint8 *src, size_t srcsize, uint8 *dst, size_t bpl, uint roileft, uint roiright, uint roitop, uint roibottom, uint decimation );
    status_t		ProcessFrame_YUV422_RGB32_Comp( uint8 *src, size_t srcsize, uint8 *dst, size_t bpl, uint roileft, uint roiright, uint roitop, uint roibottom, uint decimation );
    status_t		ProcessFrame_YUV422_YUV422_NoComp( uint8 *src, size_t srcsize, uint8 *dst, size_t bpl, uint roileft, uint roiright, uint roitop, uint roibottom, uint decimation );
    status_t		ProcessFrame_YUV422_YUV422_Comp( uint8 *src, size_t srcsize, uint8 *dst, size_t bpl, uint roileft, uint roiright, uint roitop, uint roibottom, uint decimation );

    status_t		UploadStreamData( size_t maxlen, void *buffer, size_t *actualread );
    status_t		DoIOCTL( cpiacmd *cmd );

//--------MIDLEVEL IO--------
    status_t		MidSetSensorFPS();
    status_t		MidSetExposure();
    status_t		MidSetColorBalance();
    status_t		MidSetColorParms();
    status_t		MidSetFormatAndROI();
    status_t		MidSetCompressionTarget();
    status_t		MidSetYUVThresh();
    status_t		MidSetApcor();
    status_t		MidSetVLOffset();

//--------LOWLEVEL IO--------
      // Misc
    status_t		IOResetCam(); // PPC
    status_t		IOWaitForFrame(); // PPC
    status_t		IOStopDataStream(); // USB
    status_t		IOStartDataStream(); // USB
      // CPIA
    status_t		IOGetVersion( uint8 *fwver, uint8 *fwrev, uint8 *vcver, uint8 *vcrev );
    status_t		IOGetPnpId( uint16 *vendor, uint16 *product, uint16 *device );
    status_t		IOGetCameraStatus( uint8 *systemstate, uint8 *vpctrlstate, uint8 *streamstate, uint8 *fatalerror, uint8 *cmderror, uint8 *debugflags, uint8 *vpstatus, uint8 *errorcode );
    status_t		IOGotoHiPower();
    status_t		IOGotoLoPower();
      // System
    status_t		IOReadVCRegs( uint8 addr0, uint8 addr1, uint8 addr2, uint8 addr3, uint8 *reg0, uint8 *reg1, uint8 *reg2, uint8 *reg3 );
    status_t		IOWriteVCReg( uint8 addr, uint8 andmask, uint8 ormask );
    status_t		IOReadMCPorts( uint8 *port0, uint8 *port1, uint8 *port2, uint8 *port3 );
    status_t		IOWriteMCPort( uint8 port, uint andmask, uint8 ormask );
    status_t		IOSetECPTiming(  uint8 slowtiming );
    status_t		IOReadIDATA( uint8 addr0, uint8 addr1, uint8 addr2, uint8 addr3, uint8 *data0, uint8 *data1, uint8 *data2, uint8 *data3 );
    status_t		IOWriteIDATA( uint8 addr, uint8 andmask, uint8 ormask );
      // VP
    status_t		IOSetColorParms( uint8 brightness, uint8 contrast, uint8 saturation );
    status_t		IOSetExposure( uint8 gainmode, uint8 expmode, uint8 compmode, uint8 centreweight, uint8 gain,
				       uint8 fineexp, uint8 coarseexplo, uint8 coarseexphi,  int8 redcomp, int8 green1comp, int8 green2comp, int8 bluecomp );
    status_t		IOSetColorBalance( uint8 balancemode, uint8 redgain, uint8 greengain, uint8 bluegain );
    status_t		IOSetSensorFPS( uint8 clrdiv, uint8 rate );
    status_t		IOSetVPDefaults();
    status_t		IOSetApcor( uint8 gain1, uint8 gain2, uint8 gain4, uint8 gain8 );
    status_t		IOSetVLOffset( uint8 gain1, uint8 gain2, uint8 gain4, uint8 gain8 );
    status_t		IOGetColorBalance( uint8 *red, uint8 *gree, uint8 *blue );
    status_t		IOGetExposure( uint8 *gain, uint8 *fineexp, uint8 *coarseexplo, uint8 *coarseexphi, int8 *redcomp, int8 *green1comp, int8 *green2comp, int8 *bluecomp );
    status_t		IOReadVPRegs( uint8 addr0, uint8 addr1, uint8 addr2, uint8 addr3, uint8 *reg0, uint8 *reg1, uint8 *reg2, uint8 *reg3 );
    status_t		IOWriteVPReg( uint8 addr, uint8 andmask, uint8 ormask );
      // Capture
    status_t		IOGrabFrame( uint8 startline );
    status_t		IOSetGrabMode( uint8 continuous );
    status_t		IOInitStreamCap( uint8 skipframes, uint8 startline );
    status_t		IOFiniStreamCap();
    status_t		IOStartStreamCap();
    status_t		IOStopStreamCap();
    status_t		IOSetFormat( uint8 videosize, uint8 subsampling, uint8 yuvorder );
    status_t		IOSetROI( uint8 colstart, uint8 colend, uint rowstart, uint rowend );
    status_t		IOSetCompression( uint8 compressmode, uint8 decimation );
    status_t		IOSetCompressionTarget( uint8 targetting, uint8 framerate, uint8 quality );
    status_t		IOSetYUVThresh( uint8 ythresh, uint8 uvthresh );
    status_t		IODiscardFrame();
    status_t		IOGrabReset();
      // Debug

    char			fDevicePath[PATH_MAX];
    int 			fFileHandle;
    uint8			fFirmwareVersion;
    uint8			fFirmwareRevision;
    uint8			fVideoCompressorVersion;
    uint8			fVideoCompressorRevision;
    uint8			*fCameraData;
	
    videosize_t			fVideoSize;
    uint			fRoiLeft;
    uint			fRoiRight;
    uint			fRoiTop;
    uint			fRoiBottom;

    bool			fAutoExposure;
    float			fExposure;
    int				fGain;

    bool			fAutoColorBalance;
    float			fRedColorBalance;
    float			fGreenColorBalance;
    float			fBlueColorBalance;
	
    float			fBrightness;
    float			fContrast;
    float			fSaturation;
	
    framerate_t			fFrameRate;
    framedivide_t		fFrameDiv;
	
    comprressiontarget_t	fCompressTarget;
    int				fCompressRate;
    float			fCompressQuality;
    float			fYThreshold;
    float			fUVThreshold;

    uint			fApcorCorrect1,fApcorCorrect2,fApcorCorrect3,fApcorCorrect4;
    uint			fApcorIntensity1,fApcorIntensity2,fApcorIntensity3,fApcorIntensity4;
    int				fVLOffset1,fVLOffset2,fVLOffset3,fVLOffset4;
	
    compression_t		fCompression;
    compression_t		fCurrentSetCompression;
    decimation_t		fDecimation;
    decimation_t		fCurrentSetDecimation;
};

//-----------------------------------------------------------------------------

#endif
