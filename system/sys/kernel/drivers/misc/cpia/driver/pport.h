/* Copyright 1999 VLSI Vision Ltd. Free for use under the Gnu Public License */
#ifndef CPIAPPCDRIVER_PP_H
#define CPIAPPCDRIVER_PP_H
/*
	Version 0.30
			pport.c


	Project: Vivitar PP Camera

	interface to parallel port, takes care of reading
	writing data and the operation mode of the port
	
	B.Stewart 8/1/96 
	B.Stewart + R. Monteith 5/2/96 
	                                                
	Alterations
	----------- 
	BS	21/02/96	added support for temporal difference data upload
	REM	02/04/96	Rewrite to support full 1284 stuff and nibble mode
*/

/*
	990808 : JH : Fixed all the types, to match the ones Be use.
*/

#include <atheos/kernel.h>

/****************************************************************************/
/*																			*/
/*	Public definitions of PPORT Module								 		*/
/*																			*/
/****************************************************************************/

/* The below represent the hardware capabilities of the parallel port.		*/ 
#define NULL_PORT_TYPE		0x00	
#define COMPAT_PORT_TYPE 	0x01		
#define BIDIR_PORT_TYPE		0x02
#define ECP_PORT_TYPE		0x04
#define EPP_PORT_TYPE		0x08


/* The below represent the current hardware mode the port is in.			*/
#define COMPAT_HW_MODE		0x01	/* std uni-directional mode				*/
#define BIDIR_HW_MODE		0x02	/* bidirectional mode					*/
#define ECP_HW_MODE			0x03	/* ECP mode 							*/
#define EPP_HW_MODE			0x04	/* 					- NOT YET SUPPORTED	*/


/* The below represent the std 1284 data transfer mode currently in use.	*/
#define COMPAT_TRANSFER		0x00	/* 					- NOT YET SUPPORTED	*/
#define NIBBLE_TRANSFER		0x01	/* 1284 mode				 			*/	
#define BIDIR_TRANSFER		0x02  	/* NOT USED 							*/
#define ECP_TRANSFER		0x03	/* 1284 mode				 			*/


/* Special PPC modes.These are invoked by using the 1284 Extensibility Link */
/* Flag during negotiation.													*/
#define UPLOAD_FLAG			0x08
#define NIBBLE_UPLOAD 		(UPLOAD_FLAG|NIBBLE_TRANSFER)	
#define BIDIR_UPLOAD		(UPLOAD_FLAG|BIDIR_TRANSFER)	
#define ECP_UPLOAD			(UPLOAD_FLAG|ECP_TRANSFER)


/* The flag below can be added to UPLOAD modes defined above to specify that*/
/* the difference data should be Run Length Encoded.						*/
#define NORMAL_DATA_FLAG  	0x00	
#define RLE_DATA_FLAG		0x40



#define FORWARD_DIR			0x00
#define REVERSE_DIR			0x01

/* status register */                 
#define S3	0x08   /* pin 15 */
#define S4	0x10   /* pin 13 */
#define S5	0x20   /* pin 12 */
#define	S6	0x40   /* pin 10 */
#define	S7	0x80   /* pin 11   inverted */

/* control register */
#define	C0	0x01   /* pin 1    inverted */
#define	C1	0x02   /* pin 14   inverted */
#define	C2	0x04   /* pin 16 */
#define	C3	0x08   /* pin 17   inverted */
#define C4	0x10   /* IRQ enable, if HI then S6 is connected to IRQ7 */
#define C5	0x20   /* 0 = Fwd, 1 = Reverse */


#define STATUS_INVERSION_MASK	(S7) 		/* for the inversion on some bits */
#define CONTROL_INVERSION_MASK	(C0|C1|C3) 	/* these can be used to correct   */


/* IEEE 1284 Compatiblity Mode signal names 	*/
#define nStrobe		C0	/* inverted */
#define nAutoFd		C1	/* inverted */
#define nInit		C2
#define nSelectIn	C3
#define IntrEnable	C4	/* normally zero for no IRQ */
#define	DirBit		C5	/* 0 = Forward, 1 = Reverse	*/

#define nFault		S3
#define Select		S4
#define PError		S5
#define nAck		S6
#define Busy		S7	/* inverted */	

typedef	enum { 
    ZILCHERROR,
    DETECTPORTTYPE,
    INITIALISEPPORT,
    CLOSEPPORT,
    ATTEMPT1284NEGOTIATION,

    SETTRANSFERMODE,
    ENDTRANSFERMODE,
    SETUPLOADMODE,
    ENDUPLOADMODE,

    FORWARD2REVERSE,
    REVERSE2FORWARD,
    ECPWRITEADDRBYTE ,
    ECPWRITEBUFFER,
    ECPREADBUFFER,
	
    SIMECPWRITEADDRBYTE ,
    SIMECPWRITEBUFFER,
    SIMECPREADBUFFER,
	
    NIBBLEREADBUFFER,

    ECPUPLOAD,
    RLE_ECPUPLOAD,

    BIDIRUPLOAD,
    RLE_BIDIRUPLOAD,

    NIBBLEUPLOAD,
    RLE_NIBBLEUPLOAD,

    LAST_PPORT_ERROR

} ERRORCODEPPORT;

typedef struct
{
	ERRORCODEPPORT Code;
} ERRORCHECKPPORT; 

typedef struct {
      /* Public Read Only Data */
    uint16 PortType;
    uint16 BaseAddr;

      /* Private Data */
    uint16 DataAddr;
    uint16 StatusAddr;
    uint16 ControlAddr;
    uint16 ECRAddr;
    uint16 DataFIFOAddr;

    uint8 TransferMode;
    uint8 Direction;
    uint8 ControlShadow; 
	
    uint16 Timeout_mS;
    uint32 Whileout;

      /* Data below is used to restore the port to the state it was 	*/
      /* in before started using it.									*/

    uint16 PreviousMode;	   /* which mode ie BIDIR, ECP, EPP			*/
    uint8 PrevData;
    uint8 PrevControl;

      /* state of the ECP interface */
    uint8 PrevECR;
    uint8 PrevCnfgA;
    uint8 PrevCnfgB;
	
    ERRORCHECKPPORT	ErrorCheckPPort;

    int			DMAChannel;
    bool		DMAChannelLocked;
    int			Interrupt;
    int			InterruptHandler;
    sem_id		InterruptSem;
    int			DMASize;
    void		*DMAAddress;
} PPORT;

/****************************************************************************/
/*																			*/
/*	Private definitions of PPORT Module																			*/
/*																			*/
/****************************************************************************/

/* ECP Signal names */
#define HostClk			nStrobe
#define HostAck			nAutoFd
#define nReverseRequest	nInit
#define Active_1284		nSelectIn
#define nPeriphRequest	nFault
#define XFlag			Select
#define nAckReverse		PError
#define PeriphClk		nAck
#define PeriphAck		Busy
#define HOSTACK_LO	( (DirBit | Active_1284 | HostClk)^CONTROL_INVERSION_MASK )
#define HOSTACK_HI	( (DirBit | Active_1284 | HostClk | HostAck)^CONTROL_INVERSION_MASK )


#define LPT1_BASE_ADDR	0x378
#define LPT2_BASE_ADDR	0x278
#define LPT3_BASE_ADDR	0x3BC

/* Offsets from the base address for the various registers */
#define STATUS_OFFSET		0x01
#define CONTROL_OFFSET		0x02

/* define ECP register offsets */
#define ECP_DATA			0x00	/* Data Register			*/
#define ECP_A_FIFO			0x00	/* ECP Address FIFO			*/
#define ECP_DSR				0x01	/* Device Status Register	*/
#define ECP_DCR				0x02	/* Device Control Register	*/
#define ECP_C_FIFO			0x400	/* Compatiblity mode FIFO 	*/
#define ECP_D_FIFO			0x400	/* ECP Data FIFO			*/
#define ECP_T_FIFO			0x400	/* ECP Test FIFO			*/
#define ECP_CNFG_A			0x400	/* Config Register A		*/
#define ECP_CNFG_B			0x401	/* Config Register B		*/
#define ECP_ECR				0x402	/* Extended Control Reg.	*/

#define ECP_FIFO_SIZE	16 //16

#define WRITECTRL(portaddress,val)		_outp((portaddress),(uint8)(val))
#define READDATA(portaddress)			(uint8)_inp((portaddress))                           
#define READSTATUS(portaddress)			(uint8 )_inp((portaddress))                           


/* the following define the bits within the ECR register */
#define ECR_mode			0xE0
#define ECR_nErrIntrEn		0x10
#define ECR_dmaEn			0x08
#define ECR_serviceIntr		0x04
#define ECR_full			0x02
#define ECR_empty			0x01

#define ECP_STANDARD_MODE	0x00
#define ECP_PS2_MODE		0x20
#define ECP_FIFO_MODE		0x40
#define ECP_ECP_MODE		0x60
#define RESERVED_1			0x80
#define RESERVED_2			0xA0
#define ECP_TEST_MODE		0xC0
#define ECP_CNFG_MODE		0xE0




/* Nibble Mode specific names			*/
#define HostClk			nStrobe		
#define HostBusy		nAutoFd

#define nDataAvail		nFault
#define AckDataReq		PError
#define PtrClk			nAck
#define PtrBusy			Busy

#define NIBBLE_DATA_BITS	(nFault|Select|PError|Busy)
#define HOSTBUSY_LO	( (DirBit | Active_1284 | nInit | HostClk)^CONTROL_INVERSION_MASK )
#define HOSTBUSY_HI	( (DirBit | Active_1284 | nInit | HostClk | HostBusy)^CONTROL_INVERSION_MASK )




/* Extensiblity Request Bits 	*/
#define REQUEST_LINK			0x80
#define REQUEST_EPP				0x40
#define REQUEST_ECP_RLE			0x20
#define REQUEST_ECP				0x10
#define REQUEST_DEVICE_ID 		0x04
#define REQUEST_REVERSE_BYTE 	0x01
#define REQUEST_NIBBLE			0x00


/* Valid Extensiblity Request Values */
#define EXTEN_DEVICE_ID			(REQUEST_DEVICE_ID)
#define EXTEN_ECP_MODE 			(REQUEST_ECP)
#define EXTEN_NIBBLE_MODE		(REQUEST_NIBBLE)



#define DEFAULT_TIMEOUT_MS	250	/* default timeout = 250 mS */





/****************************************************************************/
/*																			*/
/*	Public functions of PPORT Module																			*/
/*																			*/
/****************************************************************************/

/*
	General Setup Functions
*/
int DetectPortType( uint16 IOAddr, int DMAChannel, int Interrupt, PPORT *Port );
int InitialisePPort( PPORT *Port );   /* sets port to COMPAT_TRANSFER 		*/
void ClosePPort( PPORT *Port );
int Attempt1284Negotiation( PPORT *Port );

int SendVPP( PPORT *Port, uint8 Bytes, uint8 *Buffer );			

int GetTimeout( PPORT *Port, uint16 *Timeout_mS );
int SetTimeout( PPORT *Port, uint16 Timeout_mS );


/* The functions below select the standard 1284 transfer modes				*/
int SetTransferMode( PPORT *Port, uint8 TransferMode );
int EndTransferMode( PPORT *Port );

/* The functions below select the special PPC Fast Transfer modes. 			*/
int SetUploadMode( PPORT *Port, uint8 UploadMode, uint8 ModeFlags );
int EndUploadMode( PPORT *Port );

/*
	ECP Mode Public Functions 
*/
int Forward2Reverse( PPORT *Port );
int Reverse2Forward( PPORT *Port );

int ECPWriteBuffer( PPORT *Port, uint8 *Data, uint32 Bytes );
int ECPReadBuffer( PPORT *Port, uint8 *Data, uint32 Bytes );

int SimECPWriteBuffer( PPORT *Port, uint8 *Data, uint32 Bytes );
int SimECPReadBuffer( PPORT *Port, uint8 *Data, uint32 Bytes );

/* 
	Nibble Mode Public Functions 
*/
int NibbleReadBuffer( PPORT *Port, uint8 *Data, uint32 Bytes );

/* 
	Special PPC Upload Modes (use undefined 1284 Extensibility Link modes )
*/
int ECPUpload( PPORT *Port, uint8 *Data, uint32 Bytes );

int NibbleUpload( PPORT *Port, uint8 *Data, uint32 Bytes );







/****************************************************************************/
/*																			*/
/*	Private functions of PPORT Module																			*/
/*																			*/
/****************************************************************************/

/* 
	Functions to do general IEEE 1284 stuff
*/
void SetCOMPATMode( PPORT *Port );
int Negotiate2SetupPhase( PPORT *Port, uint8 ExtensibilityByte, uint8 LinkMode );
int Valid1284Termination( PPORT *Port );
void Immediate1284Termination( PPORT *Port );

/* 
	ECP Mode
*/
int ECPSetupPhase( PPORT *Port );
int AttemptForward2Reverse( PPORT *Port );
int TerminateECPMode( PPORT *Port );

void SetECRMode( PPORT *Port, uint8 Mode );
void FrobECRBits( PPORT *Port, uint8 Mask, uint8 Val );
uint8 GetECRBit( PPORT *Port, uint8 BitMask );
void EmptyFIFO( PPORT *Port );

/*
	Nibble Mode 
*/
int NibbleSetupPhase( PPORT *Port );
int TerminateNibbleMode( PPORT *Port );


/* 
	General functions
*/
uint16 TestPortType( uint16 BaseAddr );
int TestGenericECP( uint16 BaseAddr );

/*
	Low Level port access routines
*/
void SetDataDirection( PPORT *Port, uint8 Direction ); 

void PutData( PPORT *Port, uint8 Data );
uint8 GetData( PPORT *Port );

uint8 GetStatus( PPORT *Port );
uint8 GetControl( PPORT *Port );
void PutControl( PPORT *Port, uint8 Data );

uint8 GetStatusBit( PPORT *Port, uint8 BitMask );
void SetControlBit( PPORT *Port, uint8 BitMask );
void ClearControlBit( PPORT *Port, uint8 BitMask );

/*
	Hard coded upload functions
*/

uint8 UnscrambleNibble( uint8 data );

int LPT_NibbleUpload( PPORT *Port, uint8 *Data, uint32 Bytes );

#endif

