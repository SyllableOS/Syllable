/* Copyright 1999 VLSI Vision Ltd. Free for use under the Gnu Public License */
#ifndef DAMN_CPIADEVICEDRIVER_CMD_H
#define DAMN_CPIADEVICEDRIVER_CMD_H
/*
	990808 : JH : Fixed all the types, to match the ones Be use.
*/


/* ====================================================================	*/
/* Definitions for Vision Packet Protocol 								*/
/* - similar to 1284.3 CPP (Chain Packet Protocol) 						*/		
/* ====================================================================	*/
#define VPP_PREAMBLE0	0xAA
#define VPP_PREAMBLE1	0x55
#define VPP_PREAMBLE2	0x00
#define VPP_PREAMBLE3	0xFF
#define VPP_PREAMBLE4	0x69
#define VPP_PREAMBLE5	0x96
#define VPP_MANUAL_CMD	0x01
#define	VPP_TERMINATOR	0xFF






/* note these two defines MUST match the info in the packet.h file */
#define CMD_DATA_LEN		4
#define DATA_LEN			8
#define SETUP_LEN			8



/* below are definitions of module identifiers */

#define CPIA_MODULE			0
#define SYSTEM_MODULE		1
#define INT_CTRL_MODULE		2
#define PROCESS_MODULE		3
#define XXX_COM_MODULE		4
#define VP_CTRL_MODULE		5
#define CAPTURE_MODULE		6
#define DEBUG_MODULE		7


#define CPIA_FLAG		(1<<CPIA_MODULE)
#define SYSTEM_FLAG		(1<<SYSTEM_MODULE)
#define INT_CTRL_FLAG	(1<<INT_CTRL_MODULE)
#define PROCESS_FLAG	(1<<PROCESS_MODULE)
#define XXX_COM_FLAG	(1<<XXX_COM_MODULE)
#define VP_CTRL_FLAG	(1<<VP_CTRL_MODULE)
#define CAPTURE_FLAG	(1<<CAPTURE_MODULE)
#define DEBUG_FLAG  	(1<<DEBUG_MODULE)

typedef struct {
	uint8 SystemState;
	uint8 VPCtrlState;
	uint8 StreamState;

	uint8 FatalError;  	/* these three fields all use the XXXX_FLAG 		*/
	uint8 CmdError;		/* #defines to identify each module					*/
	uint8 DebugFlags;	/*													*/

	uint8 VPStatus;
	uint8 ErrorCode;	/* if a module generates an error condition it may 	*/
						/* set this field to a non-zero value to indicate 	*/
						/* what has gone wrong. If this value is already 	*/
						/* non-zero, it should not be altered i.e. the first */
						/* error that was found should stay in here.		*/	
} CAMERA_STATUS;



#define MODULE_ADDR_BITS	0xE0

#define CPIA_ADDR         (CPIA_MODULE<<5)
#define SYSTEM_ADDR       (SYSTEM_MODULE<<5)
#define DEBUG_ADDR        (DEBUG_MODULE<<5)
#define SELFTEST_ADDR     (SELFTEST_MODULE<<5)  
#define XXX_COM_ADDR      (XXX_COM_MODULE<<5)
#define VP_CTRL_ADDR      (VP_CTRL_MODULE<<5) 
#define CAPTURE_ADDR      (CAPTURE_MODULE<<5) 
#define DEBUG_ADDR  	  (DEBUG_MODULE<<5)
                             



#define PROC_ADDR_BITS		0x1F
#define DEFAULT_PROC		0x00


/* ====================================	*/
/* 		CPIA Module Procedures			*/
/*										*/
#define GET_CPIA_VERSION 		0x01
#define GET_PNP_ID				0x02
#define GET_CAMERA_STATUS		0x03

#define GOTO_HI_POWER			0x04
#define GOTO_LO_POWER			0x05
#define CPIA_UNUSED_0			0x06
#define GOTO_SUSPEND			0x07
#define GOTO_PASS_THROUGH		0x08
#define ENABLE_WATCHDOG			0x09	/* not yet implemented */

#define MODIFY_CAMERA_STATUS 	0x0A
/* ====================================	*/


/* ====================================	*/
/* 		SYSTEM Module Procedures 		*/
/*										*/
#define READ_VC_REGS			0x01
#define WRITE_VC_REG			0x02
#define READ_MC_PORTS			0x03
#define WRITE_MC_PORT			0x04
#define SET_BAUD_RATE			0x05
#define SET_ECP_TIMING			0x06
#define READ_IDATA				0x07
#define WRITE_IDATA				0x08
#define GENERIC_CALL			0x09
#define SERIAL_START			0x0A
#define SERIAL_STOP				0x0B
#define SERIAL_WRITE			0x0C
#define SERIAL_READ				0x0D

/* ====================================	*/

/* ====================================	*/
/* 		VP_CTRL Module Procedures 		*/
/*										*/
#define GET_VP_VERSION			0x01
#define RESET_FRAME_COUNTER		0x02
#define SET_COLOUR_PARAMS		0x03

#define SET_EXPOSURE			0x04
#define VP_SPARE_CMD  			0x05	/* used to be SET_GAMMA */	
#define SET_COLOUR_BALANCE		0x06
#define SET_SENSOR_FPS			0x07
#define SET_VP_DEFAULTS			0x08
#define SET_APCOR				0x09
#define FLICKER_CTRL			0x0A
#define SET_VLOFFSET 			0x0B
 
#define GET_COLOUR_PARAMS		0x10
#define GET_COLOUR_BALANCE		0x11
#define GET_EXPOSURE			0x12

#define SET_SENSOR_MATRIX		0x13

#define COLOUR_BARS				0x1D
#define READ_VP_REGS			0x1E
#define WRITE_VP_REG			0x1F
/* ====================================	*/


/* ====================================	*/
/* 		CAPTURE Module Procedures 		*/
/*										*/
#define GRAB_FRAME				0x01
#define UPLOAD_FRAME			0x02
#define SET_GRAB_MODE			0x03

#define INIT_STREAM_CAP			0x04
#define FINI_STREAM_CAP			0x05

#define START_STREAM_CAP		0x06
#define STOP_STREAM_CAP			0x07 

#define SET_FORMAT				0x08
#define SET_ROI					0x09
#define SET_COMPRESSION			0x0A
#define SET_COMPRESSION_TARGET	0x0B
#define SET_YUV_THRESH			0x0C
#define SET_COMPRESSION_PARAMS  0x0D

#define DISCARD_FRAME			0x0E
#define GRAB_RESET				0x0F
/* ====================================	*/


/* ====================================	*/
/* 		DEBUG Module Procedures  		*/
/*										*/
#define OUTPUT_RS232			0x01

#define ABORT_PROCESS			0x04
#define SET_DRAM_PAGE			0x05
#define START_DRAM_UPLOAD		0x06
#define START_DUMMY_STREAM		0x08
#define ABORT_STREAM			0x09
#define DOWNLOAD_DRAM			0x0A
#define NULL_CMD				0x0B

/* ====================================	*/


typedef union {
	uint8 Raw[CMD_DATA_LEN];

	/* ================================================================	*/
	/* 				CPIA Module Command Parameters	 		  		  	*/
	/*																	*/
	struct {
		uint8 StatusByte;												
		uint8 AndMask;	/* mask of bits NOT to clear 	*/
		uint8 OrMask;	/* mask of bits to set 			*/
	} ModifyCameraStatus;												
	/* ==================================================================== */


	/* ====================================================================	*/
	/* 				SYSTEM Module Command Parameters 		  		    	*/
	/*																		*/
	struct { 
		/* READ_VC_REGS */
		uint8 Addr0;
		uint8 Addr1;
		uint8 Addr2;
		uint8 Addr3;
	} ReadVCRegs;
	struct { 
		/* WRITE_VC_REG */
		uint8 Addr;
		uint8 AndMask;
		uint8 OrMask;
	} WriteVCReg;
	struct { 
		/* WRITE_MC_PORT */
		uint8 Port;
		uint8 AndMask;
		uint8 OrMask;
	} WriteMCPort;
	struct {
		/* SET_BAUD_RATE */
 		uint8 BaudRate;		/* divided by 1200 eg 8 = 9600, 16 = 19200		*/
	} SetBaudRate;
	struct {
		/* SET_ECP_TIMING */
 		uint8 SlowTiming;	/* set to TRUE to enable slow ECP timing		*/
	} SetECPTiming;
	struct { 
		/* READ_IDATA */
		uint8 Addr0;
		uint8 Addr1;
		uint8 Addr2;
		uint8 Addr3;
	} ReadIDATA;
	struct { 
		/* WRITE_IDATA */
		uint8 Addr;
		uint8 AndMask;
		uint8 OrMask;
	} WriteIDATA;
	struct {
		/* GENERIC_CALL */
		uint8 AddrLo;
		uint8 AddrHi;
	} GenericCall;
	struct {
		/* SERIAL_WRITE */
		uint8 WriteData;
	} SerialWrite;
	struct {
		/* SERIAL_READ */
		uint8 AckRead;
	} SerialRead;
	/* ====================================================================	*/


	/* ====================================================================	*/
	/* 				VP_CTRL Module Command Parameters	 		  			*/
	/*																		*/
	struct {
		/* SET_COLOUR_PARAMS */
		uint8 Brightness;		/* range 0 -> 128, 64 nominal				*/
		uint8 Contrast;			/* range 0 -> 128, 64 nominal				*/
		uint8 Saturation;		/* range 0 -> 128, 64 nominal				*/
	} SetColourParams;
	struct {
		/* SET_EXPOSURE */
		uint8 GainMode;			/* 0 = Auto, 1 = Manual	 					*/
		uint8 ExpMode;			/* 0 = Auto, 1 = Manual	 					*/
		uint8 CompMode;   		/* 0 = Auto, 1 = Manual			 			*/
		uint8 CentreWeight;		
		/* NOTE - the accompanying DATA_BUFFER struct contains the actual 	*/
		/* values to use if a manual mode is requested.					 	*/ 
	} SetExposure;
	struct {
		/* SET_COLOUR_BALANCE */				
		uint8 BalanceMode;		/* 0 = Auto, 1 = Manual	 					*/
		uint8 RedGain;
		uint8 GreenGain;
		uint8 BlueGain;
	} SetColourBalance;
	struct {
		/* SET_SENSOR_FPS */
		uint8 SensorClkDivisor;	/* legal range 0 -> 2	 					*/
		uint8 SensorBaseRate;	/* 1 = 30fps, 0 = 25 fps 					*/
		uint8 PhaseOverride;		/* if TRUE then apply NewPhase 				*/
		uint8 NewPhase;
	} SetSensorFPS;
 	struct {
 		/* SET_APCOR */
		uint8 gain1;				/* apcor for gain 1							*/
		uint8 gain2;				/* apcor for gain 2							*/
		uint8 gain4;				/* apcor for gain 4							*/
		uint8 gain8;				/* apcor for gain 8							*/
 	} SetApcor;
	struct {
		/* FLICKER_CTRL */
		uint8 FlickerMode;
		uint8 CoarseJump;
		uint8 AllowableOverExposure;
	} FlickerCtrl;
	struct {
		/* SET_VLOFFSET */
		uint8 gain1;				// offset for gain 1
		uint8 gain2;				// offset for gain 2
		uint8 gain4;				// offset for gain 4
		uint8 gain8;				// offset for gain 8
	} SetVLOffset;
	struct {
		/* SET_SENSOR_MATRIX */
		uint8 m00;
		uint8 m01;
		uint8 m02;
		uint8 m10;
	} SetSensorMatrix;
	struct {
		/* COLOUR_BARS */
		uint8 EnableColourBars;
	} ColourBars;
	struct { 
		/* READ_VP_REGS */
		uint8 VPAddr0;
		uint8 VPAddr1;
		uint8 VPAddr2;
		uint8 VPAddr3;
	} ReadVPRegs;
	struct { 
		/* WRITE_VP_REG */
		uint8 VPAddr;
		uint8 AndMask;
		uint8 OrMask;
	} WriteVPReg;
	/* ====================================================================	*/


	/* ====================================================================	*/
	/* 				CAPTURE Module Command Parameters	 		  			*/
	/*																		*/
	struct { 
		/* GRAB_FRAME */
		uint8 Unused;
		uint8 StreamStartLine;	/* line of grab to start the image stream	*/
	} GrabFrame;
	struct { 
		/* UPLOAD_FRAME */
		uint8 ForceUpload;	/* cause the COMP_CTRL.FrameToUpload bit to be 	*/
							/* set before sytream is started, thus causing  */
							/* an immediate start to the upload independant	*/
							/* of the state of the Grab process.			*/
	} UploadFrame;
	struct {
		/* SET_GRAB_MODE */
		uint8 ContinuousGrab;
	} SetGrabMode;
	struct {
		/* INIT_STREAM_CAP */
		uint8 SkipFrames;
		uint8 StreamStartLine;	/* line of grab to start the image stream	*/
	} InitStreamCap;
	struct { 
		/* SET_FORMAT */
		uint8 VideoSize;		/* 0 = QCIF, 1 = CIF 							*/
		uint8 SubSampling;	/* 0 = 4:2:0, 1 = 4:2:2 						*/
		uint8 YUVOrder;		/* 0 = YUYV, 1 = UYVY(only in 4:2:2)			*/
	} SetFormat;
	struct { 
		/* SET_ROI */
		uint8 ColStart;		/* these are the positions divided by 4			*/	
		uint8 ColEnd;		
		uint8 RowStart;
		uint8 RowEnd;
	} SetROI;
	struct { 				
		/* SET_COMPRESSION */
		uint8 CompMode;		/* 0 = Disabled, 1 = Manual, 2 = Auto			*/

		uint8 Decimation;	/* if CompMode = 1, this enables decimation 	*/
							/* 	  CompMode = 2, this allows decimation		*/
	} SetCompression;
	struct { 				
		/* SET_COMPRESSION_TARGET */
		uint8 FRTargetting;		/* 1 = Target FR, 0 = Target Q				*/

		uint8 TargetFR;	/* framerate aimed for, or minimum fr if targetting q */
		uint8 TargetQ;	/* quality aimed for, or minimum q if tagetting FR 	*/
							
	} SetCompressionTarget;
 	struct { 
		/* SET_YUV_THRESH */
		uint8 YThresh;		/* if CompMode above is manual, then set these	*/
		uint8 UVThresh;		/* values.										*/
							/* if CompMode is auto, then these are the max	*/
							/* allowable threshold values. 					*/
	} SetYUVThresh;
	/* ====================================================================	*/


	/* ====================================================================	*/
	/* 				DEBUG Module Command Parameters	 		  				*/
	/*																		*/
	struct { 
		/* SET_DRAM_PAGE */
		uint8 StartPageLo;
		uint8 StartPageHi;
		uint8 EndPageLo;
		uint8 EndPageHi;
	} SetDRAMPage;
	/* ====================================================================	*/

} CMD_DATA;




/*																			*/
/* The union below defines the possible data formats of the 8 BYTE 			*/
/* returned data buffer.											 		*/
/* 																			*/
typedef union {

 	uint8 Raw[DATA_LEN];
			 
	union {	/* Host -> Camera Data */

		/* ================================================================	*/
		/* 				VP_CTRL Module Additional Parameters 		  	   	*/
		/*																   	*/
		struct {
			uint8 Gain;
			uint8 FineExp;		/* msb's of the 9 bit fine value		   	*/
			uint8 CoarseExpLo;
			uint8 CoarseExpHi;
			uint8 RedComp;
			uint8 Green1Comp;
			uint8 Green2Comp;
			uint8 BlueComp;
		} SetExposure;
		struct {
			/* SET_SENSOR_MATRIX */
			uint8 m11;
			uint8 m12;
			uint8 m20;
			uint8 m21;
			uint8 m22;
		} SetSensorMatrix;
		/* ================================================================	*/

		/* ================================================================	*/
		/* 				CAPTURE Module Additional Parameters 		  	   	*/
		/*																   	*/
		struct {
			uint8 Hysteresis;
			uint8 ThreshMax;
			uint8 SmallStep;
			uint8 LargeStep;
			uint8 DecimationHysteresis;
			uint8 FRDiffStepThresh;
			uint8 QDiffStepThresh;
			uint8 DecimationThreshMod;
		} SetCompressionParams;
		/* ================================================================	*/

		/* ================================================================	*/
		/* 				DEBUG Module Additional Parameters		  		   	*/
		/*																   	*/
		struct {
			uint8 Data[DATA_LEN];
		} DownloadDRAM;
		/* ================================================================	*/

	} Out;

	union {	/* Camera -> Host Data */

		/* ================================================================	*/
		/* 				CPIA Module Return Data			 		  		  	*/
		/*																	*/
		CAMERA_STATUS GetCameraStatus;	
		struct {
			uint8 FW_Version;
			uint8 FW_Revision;
			uint8 VC_Version;
			uint8 VC_Revision;
			uint8 spare[4];
		} GetCPIAVersion;
		struct {
			uint8 VendorLSB;
			uint8 VendorMSB;
			uint8 ProductLSB;
			uint8 ProductMSB;
			uint8 DeviceLSB;
			uint8 DeviceMSB;
			uint8 spare[2];
		} GetPnPID;
		/* ================================================================	*/
	
		/* ================================================================	*/
		/* 				SYSTEM Module Return Data		 		  		  	*/
		/*																	*/
		struct {
			uint8 Reg0;
			uint8 Reg1;
			uint8 Reg2;
			uint8 Reg3;
			/* maybe add in another 4 useful registers here ! */
		} ReadVCRegs;
		struct {
			uint8 Port0;
			uint8 Port1;
			uint8 Port2;
			uint8 Port3;
		} ReadMCPorts;
		struct {
			uint8 Byte0;
			uint8 Byte1;
			uint8 Byte2;
			uint8 Byte3;
		} ReadIDATA;
		struct {
			uint8 DataRead;
		} SerialRead;
		/* ================================================================	*/

		/* ================================================================	*/
		/* 				VP_CTRL Module Return Data			 		  	   	*/
		/*																   	*/
		struct {
			uint8 VP_Version;
			uint8 VP_Revision;
			uint8 CamDeviceHi;
			uint8 CamDeviceLo;
		} GetVPVersion;
		struct {
			uint8 Brightness;
			uint8 Contrast;
			uint8 Saturation;
		} GetColourParams;
		struct {
			uint8 RedGain;
			uint8 GreenGain;
			uint8 BlueGain;
		} GetColourBalance;
		struct {
			uint8 Gain;
			uint8 FineExp;		/* msb's of the 9 bit fine value		   	*/
			uint8 CoarseExpLo;
			uint8 CoarseExpHi;
			uint8 RedComp;
			uint8 Green1Comp;
			uint8 Green2Comp;
			uint8 BlueComp;
		} GetExposure;
		struct {
			uint8 VPReg0;
			uint8 VPReg1;
			uint8 VPReg2;
			uint8 VPReg3;	
			/* maybe add in another 4 useful registers here ! */
		} ReadVPRegs;
		/* ================================================================	*/

	} In;

} DATA_BUFFER;



/* ================================================================	*/
/* 	Definition of return data from GetCameraStatus command 		  	*/
/*																	*/
/*																	*/
/* definition of SystemState values */
#define UNINITIALISED_STATE	0
#define PASS_THROUGH_STATE	1
#define LOW_POWER_STATE		2
#define HI_POWER_STATE		3
#define WARM_BOOT_STATE		4


/* definition of VPCtrlState values */
#define VP_STATE_OK						0x00

#define VP_STATE_FAILED_VIDEOINIT	  	0x01
#define VP_STATE_FAILED_AECACBINIT	  	0x02
#define VP_STATE_AEC_MAX				0x04
#define VP_STATE_ACB_BMAX				0x08

#define VP_STATE_ACB_RMIN				0x10
#define VP_STATE_ACB_GMIN				0x20
#define VP_STATE_ACB_RMAX				0x40
#define VP_STATE_ACB_GMAX				0x80



/* definition of GrabState values 	*/
#define GRAB_IDLE			0
#define GRAB_ACTIVE			1
#define GRAB_DONE			2

/* definition of StreamState values */
#define STREAM_NOT_READY	0
#define STREAM_READY		1
#define STREAM_OPEN			2
#define STREAM_PAUSED		3
#define STREAM_FINISHING	4


/* definition of ErrorCode values	*/
#define ERROR_FLICKER_BELOW_MIN_EXP     0x01 /*flicker exposure got below minimum exposure */

/*																	*/
/* ================================================================	*/

#endif
