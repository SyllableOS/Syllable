/* Copyright 1999 VLSI Vision Ltd. Free for use under the Gnu Public License */

/*
 	Version 0.50
	
	pport.c

	Project: Vision DUAL Camera

	interface to parallel port, takes care of reading
	writing data and the operation mode of the port
*/

/*
	990808 : JH : Fixed all the types, to match the ones Be use.
	              Changed DetectPortType() to accept an io address, instead of lpt port number.
*/

#include <atheos/threads.h>
#include <atheos/irq.h>
#include <atheos/semaphore.h>
#include <atheos/time.h>
#include "driver.h"
#include "pport.h"
#include "isa_dma.h"

//#include "vvl_defs.h"

//#define WHILE_OUT(gubbins)	do{ uint32 k; k=Port->Whileout;while((gubbins) && (--k > 0))/**/; if(k==0) goto WhileoutError; }while(0)
#define WHILE_OUT(gubbins)    do{ uint32 k; k=Port->Whileout;while((gubbins) && (--k > 0))/**/; if(k==0) goto WhileoutError; }while(0)

//#define WHILE_OUT_SLEEP1MS(gubbins)	{ uint32 k; k=Port->Timeout_mS;while((gubbins) && (--k > 0)) snooze(1000); if(k==0) goto WhileoutError; }
//#define WHILE_OUT_SLEEP10MS(gubbins)	{ uint32 k; k=Port->Timeout_mS/10;while((gubbins) && (--k > 0)) snooze(10000); if(k==0) goto WhileoutError; }

//#define WHILE_OUT_SLEEP1MS(gubbins)	WHILE_OUT(gubbins)	
//#define WHILE_OUT_SLEEP10MS(gubbins) WHILE_OUT(gubbins)	

/* error code messages */
const char * const ErrorMessagePPort[] = \
{
    "OK",
	"Error in DetectPortType()",
	"Error in InitialisePPort()",
	"Error in ClosePort()",
	"Error in Attempt1284Negotiation",

	"Error in SetTransferMode",
	"Error in EndTransferMode",
	"Error in SetUploadMode",
	"Error in EndUploadMode",

	"Error in Forward2Reverse",
	"Error in Reverse2Forward",
	"Error in ECPWriteAddrByte()",
	"Error in ECPWriteBuffer()",
	"Error in ECPReadBuffer()",

	"Error in SimECPWriteAddrByte()",
	"Error in SimECPWriteBuffer()",
	"Error in SimECPReadBuffer()",

	"Error in NibbleReadBuffer()",

	"Error in ECPUpload",
	"Error in RLE_ECPUpload",

	"Error in BiDirUpload",
	"Error in RLE_BiDirUpload",

	"Error in NibbleUpload",
	"Error in RLE_NibbleUpload",
	"Opps",
	"Opps"
	}; 

//ERRORCHECKPPORT ErrorCheckPPort;


/*--------------------------------------------------------------------------*/
/*	DetectPortType														*/
/*--------------------------------------------------------------------------*/
int DetectPortType( uint16 IOAddr, int DMAChannel, int Interrupt, PPORT *Port )
{
    FUNC(( "DetectPortType: in\n" ));

    Port->DMAChannel		= DMAChannel;
    Port->DMAChannelLocked 	= false;
    Port->Interrupt  		= Interrupt;
    Port->InterruptHandler	= 0;
    Port->BaseAddr 		= IOAddr;
    Port->PrevData 		= _inp( Port->BaseAddr );
    Port->PrevControl		= _inp( Port->BaseAddr+CONTROL_OFFSET );
    Port->PortType 		= TestPortType( Port->BaseAddr );

    FUNC(( "DetectPortType: out(ok)\n" ));
    return( TRUE );
}

int HandleInterrupt( int interrupt, void *data, SysCallRegs_s *regs )
{
    PPORT *Port = (PPORT*)data;

//    printk( "*" );
    
//    printk( "unlock_semaphore\n" );
    unlock_semaphore( Port->InterruptSem );
  
    return 0;//B_INVOKE_SCHEDULER;
}


/*--------------------------------------------------------------------------*/
/*	InitialisePort															*/
/*--------------------------------------------------------------------------*/
int InitialisePPort( PPORT *Port )
{
//    long status;
    uint16 Base;
    double diff;
    bigtime_t start, stop;

    Base = Port->BaseAddr;

    Port->DataAddr = Base;
    Port->StatusAddr = Base + STATUS_OFFSET;
    Port->ControlAddr = Base + CONTROL_OFFSET;
    Port->ECRAddr = Base + ECP_ECR;
    Port->DataFIFOAddr = Base + ECP_D_FIFO;

    if( Port->DMAChannel >= 0 )
    {
	status_t status = lock_isa_dma_channel( Port->DMAChannel );
	if( status < 0 )
	{
	    ERROR(( "InitialisePPort: could not lock dma channel %d\n", Port->DMAChannel ));
	    Port->DMAChannel = -1;
	}
	else
	{
	    Port->DMASize = 4*1024;

	    Port->DMAAddress = alloc_real( Port->DMASize, 0 );

	    if( Port->DMAAddress == NULL )
	    {
		ERROR(( "InitialisePPort: could not allocate dma memory\n" ));
		unlock_isa_dma_channel( Port->DMAChannel );
		Port->DMAChannel = -1;
	    }
	    else
	    {
		Port->DMAChannelLocked = true;
	    }
	}
    }

    if( Port->Interrupt >= 0 )
    {
	Port->InterruptHandler = request_irq( Port->Interrupt, HandleInterrupt, NULL, 0, "cpia_device", Port );
	if( Port->InterruptHandler < 0 )
	{
	    ERROR(( "InitialisePPort: could not install interrupt hander for interrupt %d\n", Port->Interrupt ));
	    Port->Interrupt = -1;
	}
	else
	{
//	    enable_irq( Port->Interrupt );
	    Port->InterruptSem = create_semaphore( "cpiacam interrupt", 0, 0 );
	}
    }

    Port->PrevControl = _inp( Port->ControlAddr );
    if ( !(Port->PrevControl & DirBit) ) { /* ie port was in FORWARD */
	Port->PrevData = _inp( Port->DataAddr );
    }

    if ( Port->PortType & ECP_PORT_TYPE ) {
	Port->PrevECR = _inp( Base + ECP_ECR );
		
	SetECRMode( Port, ECP_STANDARD_MODE );
	if ( GetECRBit( Port, ECR_mode ) != ECP_STANDARD_MODE ) goto InitialiseError; 

	SetECRMode( Port, ECP_CNFG_MODE );
	if ( GetECRBit( Port, ECR_mode ) != ECP_CNFG_MODE ) goto InitialiseError; 

	SetECRMode( Port, ECP_CNFG_MODE );
	Port->PrevCnfgA = _inp( Base + ECP_CNFG_A );
	Port->PrevCnfgB = _inp( Base + ECP_CNFG_B );

	  /* set CnfgA and CnfgB to default values here if required 	*/
	  /* ... probably safer to not touch them for now !!				*/
	SetECRMode( Port, ECP_STANDARD_MODE );
	if ( GetECRBit( Port, ECR_mode ) != ECP_STANDARD_MODE ) goto InitialiseError; 

	SetECRMode( Port, ECP_PS2_MODE );
	if ( GetECRBit( Port, ECR_mode ) !=  ECP_PS2_MODE ) goto InitialiseError; 
    }		

    Port->ControlShadow = (uint8)(Port->PrevControl ^ CONTROL_INVERSION_MASK);

    SetCOMPATMode( Port );
	

    Port->Timeout_mS = DEFAULT_TIMEOUT_MS;

      /* determine port timing for timeout value  */
    Port->Whileout = 100000;					/* calibration value */

    start = get_system_time();
    WHILE_OUT( (_inp( Port->StatusAddr ) | 0xFF) );		/* fastest type of WHILE_OUT useage */

WhileoutError:
    stop = get_system_time();                                        

    if ( start == stop ) goto InitialiseError;	/* Err, something went wrong !!!! */

    diff = (stop - start)/1000;

    Port->Whileout = ((uint32)Port->Timeout_mS * (uint32)100000) / (uint32)diff;
    INFO(( "Port->Whileout: %d\n", (int)Port->Whileout ));

      /* timeout value is the number of port reads done in WHILE_OUTTIME seconds */

    return( TRUE );

InitialiseError:
    Port->ErrorCheckPPort.Code =  INITIALISEPPORT;

    if( Port->DMAChannel>=0 && Port->DMAChannelLocked )
    {
	unlock_isa_dma_channel( Port->DMAChannel );
	free_real( Port->DMAAddress );
	Port->DMAChannelLocked = false;
    }
    
    return( FALSE );
}


/*-----------------------------------------------------------------*/
/* ClosePort - Restores state of the parallel port to that before  */
/*-----------------------------------------------------------------*/
void ClosePPort( PPORT *Port ) {
    uint16 Base;

    Base = Port->BaseAddr;

    if( Port->Interrupt >= 0 )
    {
	release_irq( Port->Interrupt, Port->InterruptHandler );
	delete_semaphore( Port->InterruptSem );
    }

    if ( Port->PortType & ECP_PORT_TYPE ) {
	SetECRMode( Port, ECP_CNFG_MODE );
	_outp( Base + ECP_CNFG_A, Port->PrevCnfgA );
	_outp( Base + ECP_CNFG_B, Port->PrevCnfgB );
	_outp( Port->ECRAddr, Port->PrevECR );
    }
 
    if ( !(Port->PrevControl & DirBit) ) { /* ie port was in FORWARD */
	_outp( Port->DataAddr, Port->PrevData );
    }

    if( Port->DMAChannel>=0 && Port->DMAChannelLocked )
    {
	unlock_isa_dma_channel( Port->DMAChannel );
	free_real( Port->DMAAddress );
	Port->DMAChannelLocked = false;
    }
	
    return;
}


#define VPP_WAIT_TIME	200

/*--------------------------------------------------------------------------*/
/*	SendVPP - Sends a number of data bytes to the  camera without any 		*/
/*				activity on the control lines.								*/
/*--------------------------------------------------------------------------*/
int SendVPP( PPORT *Port, uint8 Bytes, uint8 *Buffer ) {
    int i,j;

    for( i = 0; i < Bytes; i++ ) {
	for( j = 0; j < VPP_WAIT_TIME; j++ ) { /* this loop for delay purposes */
	    PutData( Port, Buffer[ i ] );
	}
    }

    return( TRUE );
}


/*--------------------------------------------------------------------------*/
/*	GetTimeout - Allows the retrieval of the current timeout value			*/
/*--------------------------------------------------------------------------*/
int GetTimeout( PPORT *Port, uint16 *Timeout_mS ) {

    *Timeout_mS = Port->Timeout_mS;

    return( TRUE );
}


/*--------------------------------------------------------------------------*/
/*	SetTimeout - Allows the setting of the timeout value					*/
/*--------------------------------------------------------------------------*/
int SetTimeout(  PPORT *Port, uint16 Timeout_mS ) {
    uint32 tmp;

    if ( Timeout_mS < 10 ) Timeout_mS = 10;
    if ( Timeout_mS > 1000 ) Timeout_mS = 1000;
	
    tmp = ((uint32)Timeout_mS * Port->Whileout) / (uint32)Port->Timeout_mS;

    if ( tmp < 100 ) tmp = 100;

    Port->Timeout_mS = Timeout_mS;
    Port->Whileout = tmp;

    return( TRUE );
}


/*--------------------------------------------------------------------------*/
/*	SetTransferMode - Negotiates from compatibility mode into the requested */
/*					  1284 standard mode.									*/
/*--------------------------------------------------------------------------*/
int SetTransferMode( PPORT *Port, uint8 NewMode )
{
    uint8 request_id;
//	cpu_status cpustatus;

    FUNC(( "SetTransferMode: in\n" ));

//	cpustatus = disable_interrupts();
	
    if ( Port->TransferMode != COMPAT_TRANSFER ) goto ModeNotChanged;
	
    request_id = NewMode & REQUEST_DEVICE_ID;
	
    switch( (NewMode & ~REQUEST_DEVICE_ID) ) {

	case ECP_TRANSFER:
	    if ( !Negotiate2SetupPhase( Port, (EXTEN_ECP_MODE | request_id), 0 ) ) goto ModeNotChanged;
	    if ( !ECPSetupPhase( Port ) ) goto ModeNotChanged;
	    break;

	case NIBBLE_TRANSFER:
	    if ( !Negotiate2SetupPhase( Port, (EXTEN_NIBBLE_MODE | request_id), 0 ) ) goto ModeNotChanged;
	    if ( !NibbleSetupPhase( Port ) ) goto ModeNotChanged;
	    break;

	default:
	    goto ModeNotChanged;
    }
//	restore_interrupts( cpustatus );
    FUNC(( "SetTransferMode: out(ok)\n" ));
    return( TRUE );

ModeNotChanged:
//	restore_interrupts( cpustatus );
    Port->ErrorCheckPPort.Code = SETTRANSFERMODE;
    WARNING(( "SetTransferMode: out(err)\n" ));
    return( FALSE );
}

/*-----------------------------------------------------------------*/
/* EndTransferMode												  */
/*-----------------------------------------------------------------*/
int EndTransferMode( PPORT *Port )
{
//	cpu_status cpustatus;

    FUNC(( "EndTransferMode: in\n" ));

//	cpustatus = disable_interrupts();

    if ( Port->TransferMode == COMPAT_TRANSFER )
    {
	FUNC(( "EndTransferMode: out(ok)\n" ));
	return( TRUE );
    }

    switch( Port->TransferMode )
    {
	case ECP_TRANSFER:
	    if ( Port->Direction == REVERSE_DIR )
	    {
		if ( !Reverse2Forward( Port ) ) goto ModeNotChanged;
	    }
	    if ( !Valid1284Termination( Port ) ) goto DoImmediateTermination;
	    break;

	case NIBBLE_TRANSFER:
	    if ( !Valid1284Termination( Port ) ) goto DoImmediateTermination;
	    break;

	default:
	    goto ModeNotChanged;
    }
//	restore_interrupts( cpustatus );
    FUNC(( "EndTransferMode: out(ok)\n" ));
    return( TRUE );

ModeNotChanged:
//	restore_interrupts( cpustatus );
    Port->ErrorCheckPPort.Code = ENDTRANSFERMODE;
    WARNING(( "EndTransferMode: out(err:ModeNotChanged)\n" ));
    return( FALSE );

DoImmediateTermination:
    Immediate1284Termination( Port );
//	restore_interrupts( cpustatus );
    Port->ErrorCheckPPort.Code = ENDTRANSFERMODE;
    WARNING(( "EndTransferMode: out(err:DoImmediateTermination)\n" ));
    return( FALSE );
}



/*------------------------------------------------------------------*/
/* SetUploadMode														*/
/*------------------------------------------------------------------*/
int SetUploadMode( PPORT *Port, uint8 UploadMode, uint8 ModeFlags )
{
    if ( Port->TransferMode != COMPAT_TRANSFER ) goto ModeNotChanged;

    if ( !Negotiate2SetupPhase( Port, REQUEST_LINK, (UploadMode | ModeFlags) ) )
	goto ModeNotChanged;

    switch( UploadMode ) {
	
	case ECP_UPLOAD:  	/* since this is almost the same as ECP		*/
	      /* it can share the ECP Setup stuff.		*/
	    if ( !ECPSetupPhase( Port ) ) goto ModeNotChanged;
	    if ( !Forward2Reverse( Port ) ) {
		Valid1284Termination( Port );
		goto ModeNotChanged;
	    }
	    break;

	case NIBBLE_UPLOAD:
	    if ( !NibbleSetupPhase( Port ) ) goto ModeNotChanged;
	    break;

	default:
	    goto ModeNotChanged;
    }

    Port->TransferMode = UploadMode;
    return( TRUE );

ModeNotChanged:
    Port->ErrorCheckPPort.Code = SETUPLOADMODE;
    return( FALSE );
}



/*-----------------------------------------------------------------*/
/* EndUploadMode													  */
/*-----------------------------------------------------------------*/
int EndUploadMode( PPORT *Port ) {
    if ( Port->TransferMode == COMPAT_TRANSFER ) return( TRUE );

    switch( (Port->TransferMode & ~RLE_DATA_FLAG) ) {

	case ECP_UPLOAD:
	case BIDIR_UPLOAD:
	    if ( Port->Direction == REVERSE_DIR )
		if ( !Reverse2Forward( Port ) ) goto DoImmediateTermination;
	    if ( !Valid1284Termination( Port ) ) goto DoImmediateTermination;
		 	
	    break;

	case NIBBLE_UPLOAD:
	    if ( !Valid1284Termination( Port ) ) goto DoImmediateTermination;
	    break;

	default:
	    goto ModeNotChanged;
    }
    return( TRUE );

ModeNotChanged:
    Port->ErrorCheckPPort.Code = ENDUPLOADMODE;
    return( FALSE );

DoImmediateTermination:
    Immediate1284Termination( Port );
    Port->ErrorCheckPPort.Code = ENDUPLOADMODE;
    return( FALSE );
}

/*-----------------------------------------------------------------*/
/* Attempt1284Negotiation										  */
/*-----------------------------------------------------------------*/
int Attempt1284Negotiation( PPORT *Port ) {
	
    uint32 tmp_whileout;
	
    PutData( Port, 0 );	/* ie request reverse nibble mode */

    SetControlBit( Port, Active_1284 );
    ClearControlBit( Port, nAutoFd );
	
      /* frig the WHILE_OUT mechanism to give a very short timeout */
    tmp_whileout = Port->Whileout;
    Port->Whileout = 50;

    WHILE_OUT( GetStatusBit( Port, nAck ) );			/* check nAck LO ...	*/
    WHILE_OUT( !((GetStatus( Port ) & (PError|nFault|XFlag))
		 == (PError|nFault|XFlag))  );			/* ... and these lines go HI 	*/

    Port->Whileout = tmp_whileout;
      /* if we got here then the dongle has responded */

    ClearControlBit( Port, nStrobe );
	
    SetControlBit( Port, nInit );	/* do this to ensure that the dongle does reset	*/
    ClearControlBit( Port, Active_1284 );			
   
      /* setting Active_1284 high here will cause the dongle to abort the */
      /* negotiation while it is waiting for event (4) to occur.			*/
      /* it jumps directly to its Immmediate1284Termination routine.		*/

      /* wait for peripheral to see the termination .. */
//	WHILE_OUT( GetStatusBit( Port, XFlag ) );

    SetCOMPATMode( Port );

      /* replace correct WHILE_OUT counts */
    Port->Whileout = tmp_whileout;
      //	OutputDebugStr( "T" );
    return( TRUE );

WhileoutError:
    SetCOMPATMode( Port );

      /* replace correct WHILE_OUT counts */
    Port->Whileout = tmp_whileout;
//	OutputDebugStr( "F" );
    return( FALSE );
}

/*--------------------------------------------------------------------------*/
/*	Forward2Reverse															*/
/*--------------------------------------------------------------------------*/
int Forward2Reverse( PPORT *Port ) {

    SetControlBit( Port, DirBit );		/* tri-state data bits */
    ClearControlBit( Port, HostAck );

    ClearControlBit( Port, nReverseRequest );
    WHILE_OUT( GetStatusBit( Port, nAckReverse ) );

    Port->Direction = REVERSE_DIR;
    return( TRUE );

WhileoutError:
    SetControlBit( Port, nReverseRequest );
    ClearControlBit( Port, DirBit );		/* drive data bits */
    return( FALSE );
}


/*--------------------------------------------------------------------------*/
/*	Reverse2Forward															*/
/*--------------------------------------------------------------------------*/
int Reverse2Forward( PPORT *Port )
{
    FUNC(( "Reverse2Forward: in\n" ));

    SetControlBit( Port, nReverseRequest );
    WHILE_OUT( !GetStatusBit( Port, nAckReverse ) );
    ClearControlBit( Port, DirBit );		/* drive data bits */

    Port->Direction = FORWARD_DIR;
    FUNC(( "Reverse2Forward: out(ok)\n" ));
    return( TRUE );

WhileoutError:
    ClearControlBit( Port, nReverseRequest );
    SetControlBit( Port, DirBit );		/* tri-state data bits */
    WARNING(( "Reverse2Forward: out(err)\n" ));
    return( FALSE );
}




/*--------------------------------------------------------------------------*/
/*	ECPWriteBuffer															*/
/*--------------------------------------------------------------------------*/
int ECPWriteBuffer( PPORT *Port, uint8 *Data, uint32 Bytes ) {
    uint32 i;
    uint16 j;
    uint8 *ptr;

    if ( (!(Port->PortType & ECP_PORT_TYPE)) 
	 || (Port->TransferMode != ECP_TRANSFER) 
	 || (Port->Direction != FORWARD_DIR) ) {
		
	Port->ErrorCheckPPort.Code =  ECPWRITEBUFFER;
	return( FALSE );	
    }

    EmptyFIFO( Port );

    SetControlBit( Port, HostAck );	  				/* NOT what I'd expect to do !!! */
    SetControlBit( Port, HostClk ); 	 		   
    SetECRMode( Port, ECP_ECP_MODE );				

    ptr = Data;

    for( i = 0; i < (Bytes/ECP_FIFO_SIZE); i++ ) {	

	WHILE_OUT( !GetECRBit( Port, ECR_empty ) );  	/* wait for FIFO to empty */
		
	  /* FIFO is now empty so safe to write at least ECP_FIFO_SIZE BYTE s */
	for( j = 0; j < ECP_FIFO_SIZE; j++ ) {
	    _outp( Port->DataFIFOAddr, *ptr++);
	}
    }

    WHILE_OUT( !(GetECRBit( Port, ECR_empty )) ); 	
    for( j = 0; j < (Bytes%ECP_FIFO_SIZE); j++ ) {
	_outp( Port->DataFIFOAddr, *ptr++);
    }

      /* wait for last BYTE to be sent */
    WHILE_OUT( !(GetECRBit( Port, ECR_empty )) ); 	
    SetECRMode( Port, ECP_PS2_MODE );				

    return( TRUE );

WhileoutError:
    SetECRMode( Port, ECP_PS2_MODE );				
    EmptyFIFO( Port );
    Port->ErrorCheckPPort.Code =  ECPWRITEBUFFER;
    return( FALSE );
}

/*--------------------------------------------------------------------------*/
/*	ECPReadBuffer														*/
/*--------------------------------------------------------------------------*/
int ECPReadBuffer( PPORT *Port, uint8 *Data, uint32 Bytes ) 
{
    uint32 i;
    uint16 j;
    uint8 *ptr;
    uint8 *end_ptr;
	
      //	kprintf( "ECPReadBuffer: %d\n", Bytes );
	
    if( (!(Port->PortType & ECP_PORT_TYPE))
	|| ((Port->TransferMode != ECP_TRANSFER) && (Port->TransferMode != ECP_UPLOAD)) 
	|| (Port->Direction != REVERSE_DIR) ) 
    {
	Port->ErrorCheckPPort.Code =  ECPWRITEBUFFER;
	return( FALSE );	
    }

    ptr = Data;
    end_ptr = ptr + Bytes;

    if( Bytes < ECP_FIFO_SIZE*4 )
	goto FinishManual;
	
    ClearControlBit( Port, HostAck );	  				
	
      /* ECP hardware only detects the HI->LO transition on PeriphClk, therefore
	 it will have missed this transition for the first BYTE ( which the dongle
	 sends straight after doing Forward2Reverse() )
	 So here we manually read the first BYTE of data before enabling ECP hardware */ 
    WHILE_OUT( GetStatusBit( Port, PeriphClk ) );
    *ptr++ = GetData( Port );
    Bytes--;
	
      /* acknowledge for this BYTE */
    SetControlBit( Port, HostAck );	  				
    WHILE_OUT( !GetStatusBit( Port, PeriphClk ) ); 
      /* note - switching on ECP hardware will complete handshake for this BYTE by 	*/
      /* taking HostAck LO again.														*/
	
      /* OK we've got the first BYTE ... now let the ECP hardware do its thing */
    if( Port->DMAChannel < 0 )
    {
	SetControlBit( Port, HostClk ); 	 			
	SetECRMode( Port, ECP_ECP_MODE );				
	
	for( i = 0; i < (Bytes/ECP_FIFO_SIZE)-1; i++ ) 
	{
//	    snooze( ECP_FIFO_SIZE*1000000/500000 ); // The camera is capable of about 400-500K/s
	    WHILE_OUT( !(_inp(Port->ECRAddr) & ECR_full) );   /* wait for FIFO to fill */
	
	    for( j = 0; j < ECP_FIFO_SIZE; j++ )
		*ptr++ = _inp( Port->BaseAddr+ECP_D_FIFO );
	}
	  /* switch off automatic filling of the FIFO		 	*/
	ClearControlBit( Port, HostAck );	  				
	  /* read any remaining bytes out the FIFO 			*/
	while( (!(_inp(Port->ECRAddr) & ECR_empty)) && ( ptr != end_ptr ) ) 
	    *ptr++ = _inp( Port->BaseAddr+ECP_D_FIFO );
	
	while( !GetECRBit( Port, ECR_empty ) && ( ptr != end_ptr ) ) 
	{
	    *ptr++ = _inp( Port->DataFIFOAddr );
	}
    }
    else
    {
	long status;
	int dmalen;
		
#if 0
	if( Port->Interrupt >= 0 )
	{
	      // clear the interrupt sem
	    int32 semcnt = get_semaphore_count( Port->InterruptSem );
	    lock_semaphore_ex( Port->InterruptSem, semcnt, 0, 0 );
	}
#endif
	
	SetECRMode( Port, ECP_ECP_MODE );				

	dmalen = end_ptr-ptr;
	if( dmalen > Port->DMASize )
	    dmalen = Port->DMASize;
	dmalen -= ECP_FIFO_SIZE;

	if( Port->Interrupt >= 0 )
	    SetControlBit( Port, IntrEnable );
	
	status = start_isa_dma( Port->DMAChannel, Port->DMAAddress, dmalen, ISADMA_MODE_WRITE|ISADMA_MODE_SINGLE|0x10 );

	  // start DMA
	SetECRMode( Port, ECP_ECP_MODE );
	FrobECRBits( Port, ECR_dmaEn|ECR_serviceIntr, ECR_dmaEn );

	if( Port->Interrupt >= 0 )
	{
//	    bigtime_t semstart,semend,pollstart,pollend;
	    status_t status;
			
//	    semstart = get_system_time();
//	    status = lock_semaphore_x( Port->InterruptSem, 1, 0, 1000000 );
//	    printk( "sleep_on_sem enter\n" );
	    status = sleep_on_sem( Port->InterruptSem, 1000*1000 );
//	    printk( "sleep_on_sem exit\n" );
//	    semend = get_system_time();
	    if( status < 0 )
		ERROR(( "ECPReadBuffer: timeout while waiting for interrupt...\n" ));

	    ClearControlBit( Port, IntrEnable );

	    
//	    pollstart = get_system_time();
	    for( i=Port->Timeout_mS; i>0; i-- )
	    {
		if( _inp(Port->ECRAddr)&ECR_serviceIntr ) break;
		snooze( 1000 );
	    }
	    if( i<=0 ) goto WhileoutError;
//	    pollend = get_system_time();
			
//	    ERROR(( "Time: irq:%Ld poll:%Ld\n", semend-semstart, pollend-pollstart ));
	}
	else
	{
	    snooze( dmalen*1000000/500000 ); // The camera is capable of about 400-500K/s

	    for( i=Port->Timeout_mS*10; i>0; i-- )
	    {
		if( _inp(Port->ECRAddr)&ECR_serviceIntr ) break;
		snooze( 1000/10 );
	    }
	    if( i<=0 ) goto WhileoutError;
	}
		
	  // Stop DMA
	FrobECRBits( Port, ECR_dmaEn, 0 );

	memcpy( ptr, Port->DMAAddress, dmalen );
	ptr += dmalen;

	  /* switch off automatic filling of the FIFO		 	*/
	ClearControlBit( Port, HostAck );	  				
	  /* read any remaining bytes out the FIFO 			*/
	while( (!(_inp(Port->ECRAddr) & ECR_empty)) && ( ptr != end_ptr ) ) 
	    *ptr++ = _inp( Port->BaseAddr+ECP_D_FIFO );
	
	while( !GetECRBit( Port, ECR_empty ) && ( ptr != end_ptr ) ) 
	    *ptr++ = _inp( Port->DataFIFOAddr );
//-----------------------------------------------------------------------------
    }
	
    SetECRMode( Port, ECP_PS2_MODE );				
      //	SetControlBit( Port, HostAck );
	
      /* now manually pull through any remaining bytes 	*/
FinishManual:
    SimECPReadBuffer( Port, ptr, (end_ptr - ptr) );
	
      //	kprintf( ">>>>OK\n" );
	
    return( TRUE );
	
WhileoutError:
    SetECRMode( Port, ECP_PS2_MODE );				
    EmptyFIFO( Port );
    Port->ErrorCheckPPort.Code =  ECPREADBUFFER;
    return( FALSE );
}


/*--------------------------------------------------------------------------*/
/*	SimECPWriteBuffer														*/
/*--------------------------------------------------------------------------*/
int SimECPWriteBuffer( PPORT *Port, uint8 *Data, uint32 Bytes ) {
    uint32 i;
    uint8 *ptr;

    if ( (Port->TransferMode != ECP_TRANSFER)
	 || (Port->Direction != FORWARD_DIR) ) {
	Port->ErrorCheckPPort.Code =  SIMECPWRITEBUFFER;
	return( FALSE );	
    }

    SetControlBit( Port, HostAck );	  	/* this is a data transfer	*/

    ptr = Data;
    for( i = 0; i < Bytes; i++ ) {	
	PutData( Port, *ptr++ );

	ClearControlBit( Port, HostClk );
	WHILE_OUT( !GetStatusBit( Port, PeriphAck ) );  	

	SetControlBit( Port, HostClk );
	WHILE_OUT( GetStatusBit( Port, PeriphAck ) );  	
    }

    return( TRUE );

WhileoutError:
    SetControlBit( Port, HostClk );
    Port->ErrorCheckPPort.Code =  SIMECPWRITEBUFFER;
    return( FALSE );
}

/*--------------------------------------------------------------------------*/
/*	SimECPReadBuffer														*/
/*--------------------------------------------------------------------------*/
int SimECPReadBuffer( PPORT *Port, uint8 *Data, uint32 Bytes ) 
{
    uint32 i;
    uint8 *ptr;

//	kprintf( "SimECPReadBuffer: %d\n", Bytes );

    if ( ( !(Port->PortType & BIDIR_PORT_TYPE))
	 || ((Port->TransferMode != ECP_TRANSFER) && (Port->TransferMode != ECP_UPLOAD))
	 || (Port->Direction != REVERSE_DIR) ) {
	Port->ErrorCheckPPort.Code =  SIMECPREADBUFFER;
	return( FALSE );	
    }

    ptr = Data;
    for( i = 0; i < Bytes; i++ ) {	
	ClearControlBit( Port, HostAck );
	WHILE_OUT( GetStatusBit( Port, PeriphClk ) );
// 		WHILE_OUT_SLEEP1MS( GetStatusBit( Port, PeriphClk ) );
// 		WHILE_OUT_SLEEP10MS( GetStatusBit( Port, PeriphClk ) );

	*ptr++ = GetData( Port );		/* read data on rising edge of PeriphClk */

	SetControlBit( Port, HostAck );
	WHILE_OUT( !GetStatusBit( Port, PeriphClk ) );
//		WHILE_OUT_SLEEP1MS( !GetStatusBit( Port, PeriphClk ) );
//		WHILE_OUT_SLEEP10MS( !GetStatusBit( Port, PeriphClk ) );
    }

//	kprintf( ">>>>OK\n" );
		 
    return( TRUE );

WhileoutError:
    ClearControlBit( Port, HostAck );
    Port->ErrorCheckPPort.Code =  SIMECPREADBUFFER;
    return( FALSE );
}

#define UPPER_NIBBLE	0xF0
#define LOWER_NIBBLE	0x0F


uint8 UnscrambleNibble( uint8 Data ) {

    Data = Data & NIBBLE_DATA_BITS;

    if ( Data & Busy ) {
	return( ((Busy>>1) | (Data & 0x78))>>3 );
    }
    else {
	return( Data>>3);
	  /* busy (bit 7 ) was not set, so no action is required ( bit 6 is already zero ) */
    }
}



/*--------------------------------------------------------------------------*/
/*	NibbleReadBuffer														*/
/*--------------------------------------------------------------------------*/
int NibbleReadBuffer( PPORT *Port, uint8 *Data, uint32 Bytes )
{
    uint32 i;
    uint8 *ptr;
    uint16 tmp;
    uint32 cpustatus;

    FUNC(( "NibbleReadBuffer: in\n" ));

//	kprintf( "NibbleReadBuffer: %d\n", Bytes );

    if ( Port->TransferMode != NIBBLE_TRANSFER ) return( FALSE );

//	cpustatus = disable_interrupts();

    ptr = Data;
    for( i = 0; i < Bytes; i++ ) {
	  //		WHILE_OUT( GetStatusBit( Port, nDataAvail ) );
		
	cpustatus = cli();
	ClearControlBit( Port, HostBusy );
	WHILE_OUT( GetStatusBit( Port, PtrClk ) );
	put_cpu_flags( cpustatus );
	tmp = UnscrambleNibble( GetStatus( Port ) );
		
	  /* now invert and shift BUSY bit */

	cpustatus = cli();
	SetControlBit( Port, HostBusy );
	WHILE_OUT( !GetStatusBit( Port, PtrClk ) );
	put_cpu_flags( cpustatus );

	cpustatus = cli();
	ClearControlBit( Port, HostBusy );
	WHILE_OUT( GetStatusBit( Port, PtrClk ) );
	put_cpu_flags( cpustatus );
	*ptr++ = (uint8)(tmp | (UnscrambleNibble(GetStatus( Port ))<<4) );

	cpustatus = cli();
	SetControlBit( Port, HostBusy );
	WHILE_OUT( !GetStatusBit( Port, PtrClk ) );
	put_cpu_flags( cpustatus );
    }

//	kprintf( ">>>>OK\n" );


//	restore_interrupts( cpustatus );
    FUNC(( "NibbleReadBuffer: out(ok)\n" ));
    return( TRUE );

WhileoutError:

    SetControlBit( Port, HostBusy );
//	restore_interrupts( cpustatus );
    Port->ErrorCheckPPort.Code =  NIBBLEREADBUFFER;
    WARNING(( "NibbleReadBuffer: out(err)\n" ));
    return( FALSE );
}





/****************************************************************************/
/*																			*/
/*	Special PPC Upload Functions	 										*/
/*																			*/
/*																			*/
/****************************************************************************/


/*--------------------------------------------------------------------------*/
/*	ECPUpload																*/
/*--------------------------------------------------------------------------*/
int ECPUpload( PPORT *Port, uint8 *Data, uint32 Bytes ) {

    if ( (!(Port->PortType & ECP_PORT_TYPE) ) 
	 || (Port->TransferMode != ECP_UPLOAD) ) {
	Port->ErrorCheckPPort.Code =  ECPUPLOAD;
	return( FALSE );
    }

    return( ECPReadBuffer( Port, Data, Bytes ) );
}


/*--------------------------------------------------------------------------*/
/*	NibbleUpload															*/
/*--------------------------------------------------------------------------*/
int NibbleUpload( PPORT *Port, uint8 *Data, uint32 Bytes ) {
    int rtn;
    
    rtn = FALSE;

    if ( Port->TransferMode != NIBBLE_UPLOAD ) return( FALSE );

    rtn = LPT_NibbleUpload( Port, Data, Bytes );
    if( rtn == FALSE )
    {
	Port->ErrorCheckPPort.Code =  NIBBLEUPLOAD;
    }

    return( rtn );
}


/****************************************************************************/
/*																			*/
/*	Private functions of PPORT Module below here 							*/
/*																			*/
/****************************************************************************/


void SetCOMPATMode( PPORT *Port ) {

    PutControl( Port, (nStrobe|nAutoFd|nInit) );

    PutData( Port, 0xFF ); 	/* just in case peripheral is still driving lines */

    SetDataDirection( Port, FORWARD_DIR );

    Port->TransferMode = COMPAT_TRANSFER;
    Port->Direction = FORWARD_DIR;

    return;
}




/*--------------------------------------------------------------------------*/
/*	Negotiate2SetupPhase													*/
/*--------------------------------------------------------------------------*/
int Negotiate2SetupPhase( PPORT *Port, uint8 ExtensibilityByte, uint8 LinkMode ) {

    PutData( Port, ExtensibilityByte );

    SetControlBit( Port, Active_1284 );
    ClearControlBit( Port, nAutoFd );

    WHILE_OUT( GetStatusBit( Port, nAck ) );			/* check nAck LO ...	*/
    WHILE_OUT( !((GetStatus( Port ) & (PError|nFault|XFlag))
		 == (PError|nFault|XFlag))  );			/* ... and these lines go HI 	*/

      /* ok we have got a 1284 peripheral attached */

      /* the following LO going pulse on nStrobe is now ignored by the 		*/
      /* periperal which will read the ExtensiblityByte as soon as it sees	*/
      /* Active_1284 going HI.												*/
    ClearControlBit( Port, nStrobe );
    ClearControlBit( Port, nStrobe );
    ClearControlBit( Port, nStrobe );
    ClearControlBit( Port, nStrobe );

    SetControlBit( Port, (nAutoFd | nStrobe) );		   /* min 0.5uS LO pulse required 	*/
  
    WHILE_OUT( !GetStatusBit( Port, nAck ) ); 					//  -- (event 6)		
	
//	ExtensibilityByte &= ~REQUEST_DEVICE_ID;		// clear Device ID Request bit

    if ( ExtensibilityByte == EXTEN_NIBBLE_MODE ) {
	if ( GetStatusBit( Port, XFlag) ) goto ModeNotSupported;
    }
    else {
	if ( !GetStatusBit( Port, XFlag) ) goto ModeNotSupported;
    }

    if ( ExtensibilityByte & REQUEST_LINK ) {
	  /* an Extensiblity Link was requested */

	PutData( Port, LinkMode );								
					
	ClearControlBit( Port, nStrobe);							//  -- (event 51)
	WHILE_OUT( GetStatusBit( Port, nAck ) ); 							

	SetControlBit( Port, (nAutoFd | nStrobe) );		   			//  -- (event 53)
	WHILE_OUT( !GetStatusBit( Port, nAck ) ); 				   		

	if ( !GetStatusBit( Port, XFlag) ) goto ModeNotSupported;
    }
      /* negotiation phase complete */

    return( TRUE );	

WhileoutError:
    SetCOMPATMode( Port );
    return( FALSE );

ModeNotSupported:
    SetCOMPATMode( Port );
    return( FALSE );
}


/*--------------------------------------------------------------------------*/
/*	Valid1284Termination													*/
/*--------------------------------------------------------------------------*/
int Valid1284Termination( PPORT *Port )
{
    FUNC(( "Valid1284Termination: in\n" ));

    SetControlBit( Port, nInit );	/* do this to ensure that the dongle 	*/
      /* doesn't get a reset.					*/
      /* Reset is  nInit = Active_1284 = LO	*/
    SetControlBit( Port, nAutoFd );

    ClearControlBit( Port, Active_1284 );			

    WHILE_OUT( GetStatusBit( Port, (Busy|nFault) ) 	
	       != (Busy|nFault) );			/* 		-- (event 23)	*/

                                                

    WHILE_OUT( GetStatusBit( Port, nAck ) );		/*		-- (event 24)	*/ 
    ClearControlBit( Port, nAutoFd );				/*		-- (event 25)	*/
                                                
                                                
    WHILE_OUT( !GetStatusBit( Port, nAck ) );		/* 		-- (event 27)	*/

    SetControlBit( Port, nAutoFd );

    SetCOMPATMode( Port );

    FUNC(( "Valid1284Termination: out(ok)\n" ));
    return( TRUE );

WhileoutError:
    Immediate1284Termination( Port );
    WARNING(( "Valid1284Termination: out(err)\n" ));
    return( FALSE );
}




/*--------------------------------------------------------------------------*/
/*	Immmediate1284Termination												*/
/*--------------------------------------------------------------------------*/
void Immediate1284Termination( PPORT *Port )
{
    FUNC(( "Immediate1284Termination: in\n" ));
	
    SetControlBit( Port, nInit );	/* make sure dongle will not be reset	*/
    ClearControlBit( Port, Active_1284 );

      /* wait for peripheral to see the termination .. */
    WHILE_OUT( GetStatusBit( Port, XFlag ) );

    SetCOMPATMode( Port );
    FUNC(( "Immediate1284Termination: out(ok)\n" ));
    return;

WhileoutError:
    SetCOMPATMode( Port );
    WARNING(( "Immediate1284Termination: out(err)\n" ));
    return;
}




											
/*--------------------------------------------------------------------------*/
/*	ECPSetupPhase															*/
/*--------------------------------------------------------------------------*/
int  ECPSetupPhase( PPORT *Port ) {
 
      /* Assertion - negotiation phase complete before this functions is 		*/
      /* called ... now do ECP setup phase.									*/

    ClearControlBit( Port, HostAck );

    WHILE_OUT( !GetStatusBit( Port, nAckReverse ) );

    EmptyFIFO( Port );

    Port->TransferMode = ECP_TRANSFER;
    Port->Direction = FORWARD_DIR;
    return( TRUE );

WhileoutError:
    SetCOMPATMode( Port );
    return( FALSE );
}

 

/*--------------------------------------------------------------------------*/
/*	AttemptForward2Reverse	  												*/
/*--------------------------------------------------------------------------*/
int AttemptForward2Reverse( PPORT *Port ) {
    static int WaitingForReverseAck = FALSE;

    if ( Port->Direction != FORWARD_DIR ) {
	WaitingForReverseAck = FALSE;
	return( FALSE );
    }
	
    if ( !WaitingForReverseAck ) {
	SetControlBit( Port, DirBit );		/* tri-state data bits */
	ClearControlBit( Port, HostAck );
	ClearControlBit( Port, nReverseRequest );

	  /* wait a bit here */
	ClearControlBit( Port, nReverseRequest );
	ClearControlBit( Port, nReverseRequest );
	ClearControlBit( Port, nReverseRequest );

	WaitingForReverseAck = TRUE;	
    }

    if ( !GetStatusBit( Port, nAckReverse ) ) { 	/* peripheral has acknowledged */
	Port->Direction = REVERSE_DIR;
	WaitingForReverseAck = FALSE;
	return( TRUE );
    }	
    else {
	return( FALSE );							/* peripheral has not acknowledged */
    }
}





/*--------------------------------------------------------------------------*/
/*	NibbleSetupPhase						 								*/
/*--------------------------------------------------------------------------*/
int  NibbleSetupPhase( PPORT *Port ) {
 
      /* Assertion - negotiation phase complete before this functions is 		*/
      /* called ... now do Nibble setup phase. 								*/

    SetDataDirection( Port, REVERSE_DIR );

//	ClearControlBit( Port, HostBusy );

    Port->TransferMode = NIBBLE_TRANSFER;
    Port->Direction = REVERSE_DIR;

    return( TRUE );
}



/* ====================================================================	*/
/*																		*/
/* General routines										*/
/*																		*/
/* ====================================================================	*/

uint16 TestPortType( uint16 BaseAddr ) {
    uint16 port_type;

    port_type = COMPAT_PORT_TYPE;
	
    if ( TestGenericECP( BaseAddr ) ) 
	port_type |= (ECP_PORT_TYPE | BIDIR_PORT_TYPE);

    return( port_type );
}


int TestGenericECP( uint16 BaseAddr ) {
    uint8 old_ecr, ecr;

    old_ecr = _inp( BaseAddr + ECP_ECR );
    ecr = (old_ecr & 0x1F) | ECP_TEST_MODE;	
    _outp( BaseAddr + ECP_ECR, ecr );
	
      /* check that the mode switch did take */
    if ( _inp( BaseAddr + ECP_ECR ) != ecr ) goto Fail_ECP_Test;
	   
    _outp( BaseAddr + ECP_T_FIFO, 'H' );
    _outp( BaseAddr + ECP_T_FIFO, 'i' );
    if ( _inp( BaseAddr + ECP_T_FIFO ) != 'H' ) goto Fail_ECP_Test;
    if ( _inp( BaseAddr + ECP_T_FIFO ) != 'i' ) goto Fail_ECP_Test;

      /* OK it seems to be an ECP port */
    _outp( BaseAddr + ECP_ECR, old_ecr ); /* restore old ECR */

    return( TRUE );

	
Fail_ECP_Test:
    _outp( BaseAddr + ECP_ECR, old_ecr ); /* restore old ECR */
    return( FALSE );		 
}

 
void SetECRMode( PPORT *Port, uint8 Mode ) {
    _outp( Port->ECRAddr, ( Mode | ECR_nErrIntrEn | ECR_serviceIntr ) );
}

void FrobECRBits( PPORT *Port, uint8 Mask, uint8 Val ) {
    _outp( Port->ECRAddr, ((_inp(Port->ECRAddr)&(~Mask))|Val) );
}

uint8 GetECRBit( PPORT *Port, uint8 BitMask ) {
    uint8 tmp;
    tmp = _inp( Port->BaseAddr + ECP_ECR );
    return( tmp & BitMask );
}

void EmptyFIFO( PPORT *Port ) {
    if ( Port->PortType & ECP_PORT_TYPE ) {
	while( !GetECRBit( Port, ECR_empty ) ) _inp( Port->DataFIFOAddr );
    }
    return;
}



/* ====================================================================	*/
/*																		*/
/* Low level port access routines										*/
/*																		*/
/* ====================================================================	*/
void SetDataDirection( PPORT *Port, uint8 NewDirection ) {

    if ( NewDirection == Port->Direction ) return;	/* requested == current */

    if ( NewDirection == FORWARD_DIR ) {
	ClearControlBit( Port, DirBit );		
	Port->Direction = FORWARD_DIR;
    }
    else {
	SetControlBit( Port, DirBit );
	PutData( Port, 0xFF ); 			/* for pseudo bi-dir ports */
	Port->Direction = REVERSE_DIR;
    }
    return;
}

void PutData( PPORT *Port, uint8 Data ) {
    _outp( Port->DataAddr, Data );
}

uint8 GetData( PPORT *Port ) {
    uint8 tmp;
    tmp = (uint8)_inp( Port->DataAddr );
    return( tmp  );
}

uint8 GetStatus( PPORT *Port ) {
    return( (uint8)(_inp( Port->StatusAddr )^ STATUS_INVERSION_MASK) );
}

uint8 GetControl( PPORT *Port ) {
    return( Port->ControlShadow );
}

void PutControl( PPORT *Port, uint8 Data ) {
    uint8 Data2;
    Data2 = (Data ^ CONTROL_INVERSION_MASK);
    _outp( Port->ControlAddr, Data2 );
    Port->ControlShadow = Data; 
}

uint8 GetStatusBit( PPORT *Port, uint8 BitMask ) {
    return( (uint8)((_inp( Port->StatusAddr )^ STATUS_INVERSION_MASK) & BitMask) );
}

void SetControlBit( PPORT *Port, uint8 BitMask ) {
    uint8 Data;
    Data = ((Port->ControlShadow |= BitMask) ^ CONTROL_INVERSION_MASK);
    _outp( Port->ControlAddr, Data );
}	

void ClearControlBit( PPORT *Port, uint8 BitMask ) {
    uint8 Data;
    Data = ((Port->ControlShadow &= ~BitMask) ^ CONTROL_INVERSION_MASK);
    _outp( Port->ControlAddr, Data );
}






__inline uint8 UnscrambleUpload( uint8 Data ) {

    if ( Data & Busy ) {
	return( (Data&0x38)>>3 );
    }
    else {
	return( ((0x40)|(Data&0x38))>>3 );
	  /* busy (bit 7 ) was not set, so inverted it and return */
    }
}




int LPT_NibbleUpload( PPORT *Port, uint8 *Data, uint32 Bytes ) 
{
    uint32 i;
    uint8 *ptr;
    uint8 tmp_lo, tmp_hi;
	
    if( Port->TransferMode != NIBBLE_UPLOAD )
	return( FALSE );
	
    ptr = Data;
    for( i = 0; i < Bytes; i++ )
    {
	WRITECTRL( Port->ControlAddr, HOSTBUSY_LO );
	WHILE_OUT( (tmp_lo = READSTATUS( Port->StatusAddr )) & PtrClk );
	tmp_lo = UnscrambleUpload( tmp_lo );
	
	WRITECTRL( Port->ControlAddr, HOSTBUSY_HI );
	WHILE_OUT( !(tmp_hi = READSTATUS( Port->StatusAddr )) & PtrClk );
	*ptr++ = (uint8)(tmp_lo | (UnscrambleUpload( tmp_hi )<<4) );
    }
    return( TRUE );              
	
WhileoutError:
    WRITECTRL( Port->ControlAddr, HOSTBUSY_HI );
    return( FALSE );              
}


