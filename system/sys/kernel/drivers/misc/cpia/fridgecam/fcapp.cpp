//-----------------------------------------------------------------------------
#include <stdio.h>
#include <time.h>
#include <unistd.h>
//-------------------------------------
#include <atheos/time.h>
#include <atheos/msgport.h>
#include <atheos/kernel.h>
#include <gui/font.h>
#include <gui/message.h>
#include <gui/view.h>
//-------------------------------------
extern "C" {
#include <jpeglib.h>
};
#include "cpiacam.h"
//-------------------------------------
#include "fcapp.h"
#include "fcwin.h"
//-----------------------------------------------------------------------------

FCApp::FCApp() :
	os::Application( "application/x-vnd.DamnRednex-FridgeCam" )
{
    fCam = NULL;
    fWorkQuit = false;
    fWorkThread = -1;

    fWindow = NULL;

    fCam = new CPiACam( "/dev/video/cpia/0" );
    if( (fCam==NULL) || (fCam->InitCheck()<0) )
    {
	fprintf( stderr, "Could not open camera device\n" );
	if( fCam )
	{
	    fCam->Lock();
	    delete fCam;
	    fCam = NULL;
	}
//	exit( 1 );
    }
    else
    {
	fCam->Lock();
	printf( "CPiA; Firmware version:%d.%d VideoCompressor version:%d.%d\n", 
		fCam->GetFirmwareVersion(), fCam->GetFirmwareRevision(),
		fCam->GetVideoCompressorVersion(), fCam->GetVideoCompressorRevision() );

	fCam->SetVideoSize( CPiACam::RES_CIF );
#if 0
	fCam->EnableCompression( CPiACam::COMPRESSION_MANUAL, CPiACam::DECIMATION_OFF );
	fCam->SetYUVTreshold( 0.6f, 0.6f );
#else
	fCam->EnableCompression( CPiACam::COMPRESSION_AUTO, CPiACam::DECIMATION_OFF );
// 	fCam->SetCompressionTarget( CPiACam::COMPTARGET_FRAMERATE, 30, 0.95f );
	fCam->SetCompressionTarget( CPiACam::COMPTARGET_QUALITY, 30, 0.0f );
#endif
	fCam->Unlock();
    }

    fCaptureBitmap = new os::Bitmap( 352,288, CS_RGB32, os::Bitmap::SHARE_FRAMEBUFFER );
    fBitmap = new os::Bitmap( 352,288, CS_RGB32, os::Bitmap::ACCEPT_VIEWS|os::Bitmap::SHARE_FRAMEBUFFER );
    fBitmapView = new os::View( os::Rect(0,0,352-1,288-1), "" );
    fBitmap->AddChild( fBitmapView );

    fWindow = new FCWin( os::Rect(100,100,100+352,100+288), "FridgeCam" );

    fFrameReadyPort = create_port( "fridgecam frame request", 1024 );
    FILE *hf = fopen( "/tmp/fridgecam.port", "wt" );
    if( hf != NULL )
    {
	fprintf( hf, "%d", fFrameReadyPort );
	fclose( hf );
    }

    fWorkQuit = false;
    fWorkThread = spawn_thread( "Video Capture", _WorkThread, NORMAL_PRIORITY, 128*1024, this );
    resume_thread( fWorkThread );
}

FCApp::~FCApp()
{
    if( fWorkThread >= 0 )
    {
	fWorkQuit = true;
	printf( "Waiting for worker thread to quit...\n" );
	wait_for_thread( fWorkThread );
	printf( "Whee, the thread actually stopped ;)\n" );
    }

    fWindow->Quit();
    delete fBitmap;
    delete fCaptureBitmap;
    
    if( fCam && fCam->Lock() >= 0 )
    {
	delete_port( fFrameReadyPort );
	delete fCam;
    }
}

//-----------------------------------------------------------------------------

bool FCApp::OkToQuit()
{
    return true;
}

void FCApp::Quit()
{
    printf( "QUIT!\n" );
    os::Application::Quit();
}

//-----------------------------------------------------------------------------

void FCApp::HandleMessage( os::Message *message )
{
    int v = message->GetCode();
    
    switch( v )
    {
	case MSG_SETCAMERAPARMS:
	      // this is for the camera thread
	    if( fWorkMessageQueue.Lock() )
	    {
		fWorkMessageQueue.AddMessage( new os::Message(*message) );
		fWorkMessageQueue.Unlock();
	    }
	    break;
    }
}

//-----------------------------------------------------------------------------

long FCApp::_WorkThread( void *data )
{
    FCApp *app = (FCApp*)data;
    app->WorkThread();
    return 0;
}

void FCApp::WorkThread()
{
    bool delta = false;

    bigtime_t starttime = get_system_time();
    int framecnt = 0;

    while( !fWorkQuit )
    {
	if( fCam )
	    fCam->Lock();

	if( !fWorkMessageQueue.Lock() )
	{
	    if( fCam )
		fCam->Unlock();
	    return; // something must have gone wrong, get out of here!
	}
	    
	  // process our messages:
	os::Message *msg;
	bool set_exp_gain = false;
	float exposure;
	int32 gain;
	while( (msg=fWorkMessageQueue.NextMessage()) != NULL )
	{
	    if( fCam )
	    {

		int32 framerate;
		if( msg->FindInt32("framerate",&framerate) >= 0 )
		    fCam->SetFrameRate( framerate );

		bool autoexposure;
		if( msg->FindBool("exp:auto",&autoexposure) >= 0 )
		    fCam->EnableAutoExposure( autoexposure );
	    
		if( msg->FindFloat("exp:exposure",&exposure)>=0 &&
		    msg->FindInt32("exp:gain",&gain)>=0 )
		    set_exp_gain = true;

		int32 cmode;
		int32 cdecimation;
		if( msg->FindInt32("comp:mode",&cmode)>=0 &&
		    msg->FindInt32("comp:decimation",&cdecimation)>=0 )
		{
		    fCam->EnableCompression(
			(CPiACam::compression_t)cmode,
			cdecimation? CPiACam::DECIMATION_ON : CPiACam::DECIMATION_OFF );
		}

		bool catargetquality;
		int32 caframerate;
		float caquality;
		if( msg->FindBool("comp:auto:targetquality",&catargetquality) >= 0 &&
		    msg->FindInt32("comp:auto:framerate",&caframerate) >= 0 &&
		    msg->FindFloat("comp:auto:quality",&caquality) >= 0 )
		{
		    fCam->SetCompressionTarget(
			catargetquality ? CPiACam::COMPTARGET_FRAMERATE : CPiACam::COMPTARGET_QUALITY,
			caframerate, caquality );
		}

		float cmy;
		float cmuv;
		if( msg->FindFloat("comp:manual:y",&cmy) >= 0 &&
		    msg->FindFloat("comp:manual:uv",&cmuv) >= 0 )
		{
		    fCam->SetYUVTreshold( cmy, cmuv );
		}

		float colbright;
		float colcon;
		float colsat;
		if( msg->FindFloat("color:brightness",&colbright) >= 0 &&
		    msg->FindFloat("color:contrast",&colcon) >= 0 &&
		    msg->FindFloat("color:saturation",&colsat) >= 0 )
		{
		    fCam->SetColorParms( colbright, colcon, colsat );
		}

		bool cbmode;
		if( msg->FindBool("colorbalance:auto",&cbmode) >= 0 )
		{
		    fCam->EnableAutoColorBalance( cbmode );
		}

		float cbred, cbgreen, cbblue;
		if( msg->FindFloat("colorbalance:red",&cbred) >= 0 &&
		    msg->FindFloat("colorbalance:green",&cbgreen) >= 0 &&
		    msg->FindFloat("colorbalance:blue",&cbblue) >= 0 )
		{
		    fCam->SetColorBalance( cbred, cbgreen, cbblue );
		}
	    }

	    delete msg;
	}
	if( set_exp_gain ) fCam->SetExposure( exposure, gain );

	fWorkMessageQueue.Unlock();

	float hwexposure;
	int hwgain;
	float hwred, hwgreen, hwblue;
	bool captureerr = false;
	if( fCam )
	{
	    int nStatus = fCam->CaptureFrame( fCaptureBitmap, delta );
	    if( nStatus < 0 )
	    {
		fCam->Reset();
		fCam->Unlock();
		delta = false;
		captureerr = true;
	    }
	    else
	    {
		fCam->GetExposure( &hwexposure, &hwgain );
		fCam->GetColorBalance( &hwred, &hwgreen, &hwblue );
		fCam->Unlock();
		delta = true;

		uint8 *sdata = (uint8*)fCaptureBitmap->LockRaster();
		uint8 *ddata = (uint8*)fBitmap->LockRaster();
		for( int i=0; i<288; i++ )
		    memcpy( ddata+i*fBitmap->GetBytesPerRow(),
			    sdata+i*fCaptureBitmap->GetBytesPerRow(),
			    fCaptureBitmap->GetBytesPerRow() );
		fBitmap->UnlockRaster();
		fCaptureBitmap->UnlockRaster();
		
		TimestampBitmap( fBitmap );
	    }
	}
	if( fCam==NULL || captureerr )
	{
	    uint8 rnddata[352+64];
	    for( uint i=0; i<sizeof(rnddata); i++ )
		rnddata[i] = rand();
	    
	    snooze( 1000000/10 );
	    void *data = fBitmap->LockRaster();
	    for( int iy=0; iy<288; iy++ )
	    {
		uint32 *line = (uint32*)(((uint8*)data)+iy*fBitmap->GetBytesPerRow());
		uint8 *rnddata2 = rnddata+(rand()%(sizeof(rnddata)-352));
		for( int ix=0; ix<352; ix++ )
		{
		    uint8 col = *rnddata2++;
		    line[ix] = col<<16 | col<<8 | col;
		}
	    }
	    fBitmap->UnlockRaster();

	      // testing;
	    hwexposure = 0;
	    hwgain = 0;
	    hwred = hwgreen = hwblue = 0.0f;

	    TimestampBitmap( fBitmap, "Somebody stole the camera!" );
	}

	if( fWindow->Lock() >= 0 )
	{
	    fWindow->DrawBitmap( fBitmap );
	    fWindow->SetExposure( hwexposure, hwgain );
	    fWindow->SetColorBalance( hwred, hwgreen, hwblue );
	    fWindow->Unlock();
	}

	  // anybody want's this frame?
#if 1
	bool saved = false;
	while( 1 )
	{
	    uint32 code;
	    port_id notifyport;
	    if( get_msg_x(fFrameReadyPort,&code,&notifyport,sizeof(notifyport),0) >= 0 )
	    {
//		printf( "notifying %d\n", notifyport );
		if( !saved )
		{
		    FILE *hf = fopen( "/tmp/fridgecam.jpg.tmp", "wb" );
		    if( hf != NULL )
		    {
			struct jpeg_compress_struct jpginfo;
			struct jpeg_error_mgr jpgerr;

			jpginfo.err = jpeg_std_error( &jpgerr );
			jpeg_create_compress( &jpginfo );
			
			jpeg_stdio_dest( &jpginfo, hf );

			jpginfo.in_color_space = JCS_RGB;
			jpginfo.image_width = 352;
			jpginfo.image_height = 288;
			jpginfo.input_components = 3;
//			jpeg_set_quality( &jpginfo, 75 );
			jpeg_set_defaults( &jpginfo );
			
			jpeg_start_compress( &jpginfo, TRUE );

			uint8 *data = fBitmap->LockRaster();
			uint8 ddata[352*3];
			uint8 *ddatap = ddata;
			for( int i=0; i<288; i++ )
			{
			    uint8 *dline = ddata;
			    uint32 *sline = (uint32*)(data+i*fBitmap->GetBytesPerRow());
			    for( int j=0; j<352; j++ )
			    {
				uint32 s = *sline++;
				*dline++ = s>>16;
				*dline++ = s>>8;
				*dline++ = s>>0;
			    }
			    jpeg_write_scanlines( &jpginfo, &ddatap, 1 );
			}
			fBitmap->UnlockRaster();
    
			jpeg_finish_compress( &jpginfo );
			jpeg_destroy_compress( &jpginfo );

			fclose( hf );
			rename( "/tmp/fridgecam.jpg.tmp", "/tmp/fridgecam.jpg" );
			saved = true;
		    }
		}
		send_msg( notifyport, code, &code, sizeof(code) );
	    }
	    else
		break;
	}
#endif
	
	bigtime_t nowtime = get_system_time();
	framecnt++;
	if( (nowtime-starttime)>10000000 )
	{
	    printf( "FPS: %f\n", float(framecnt)/(float(nowtime-starttime)/1000000.0f) );
	    starttime = nowtime;
	    framecnt = 0;
	}

    }
    fWorkQuit = false;
}

void FCApp::DrawString( os::View *view, const os::Point &pos, const char *text )
{
    view->SetDrawingMode( DM_OVER );

//    view->SetFgColor( 0,0,0 );
    view->SetFgColor( 255,255,255 );
    for( int iy=-1; iy<=1; iy+=1 )
	for( int ix=-1; ix<=1; ix+=1 )
	    view->DrawString( text, os::Point(pos.x+ix,pos.y+iy) );

//    view->SetFgColor( 255,255,255 );
    view->SetFgColor( 0,0,0 );
    view->DrawString( text, pos );
}

void FCApp::TimestampBitmap( os::Bitmap *pcBitmap, const char *message )
{
    os::FontHeight_s fontheight;
    fBitmapView->GetFontHeight( &fontheight );

    if( message )
    {
	fBitmapView->Flush();
	fBitmapView->GetFont()->SetSize( 10.0f );

	float strwidth = fBitmapView->GetStringWidth( message );
	DrawString( fBitmapView, os::Point((352-strwidth)/2,(288-(fontheight.nAscender-fontheight.nDescender))/2), message );
    }

    fBitmapView->Flush();
    fBitmapView->GetFont()->SetSize( 6.75f );

    time_t timet = time( NULL );
    char timestr[64]; // "Man 13 May 2000 11:22:33"
//    strftime( timestr, sizeof(timestr), "%a %d %b %Y %H:%M:%S", localtime(&timet) );
    strftime( timestr, sizeof(timestr), "%a %d %b %H:%M:%S", localtime(&timet) );

//    os::Color32_s backcol( 63, 63, 127 );
//    os::Color32_s textcol( 255, 255, 255 );
//    os::Color32_s backcol( 0, 0, 128 );
    os::Color32_s textcol( 255, 255, 255 );

//    fBitmapView->SetEraseColor( backcol );
//    fBitmapView->SetBgColor( backcol );
#if 0
    fBitmapView->SetDrawingMode( DM_ADD );

    fBitmapView->SetFgColor( backcol );
    fBitmapView->FillRect( os::Rect(0,(288-1)-(fontheight.nAscender-fontheight.nDescender),352-1,288-1) );
#else
    void *data = fBitmap->LockRaster();
    for( int iy=(288-1)-(fontheight.nAscender-fontheight.nDescender); iy<=288-1; iy++ )
    {
	uint32 *line = (uint32*)(((uint8*)data)+iy*fBitmap->GetBytesPerRow());
	for( int ix=0; ix<352; ix++ )
	{
	    uint32 col = line[ix];
	    int r = (col>>16)&0xff;
	    int g = (col>>8)&0xff;
	    int b = (col>>0)&0xff;
	    line[ix] = (r>>1)<<16 | (g>>1)<<8 | (b>>2)+(128-32);
	}
    }
    fBitmap->UnlockRaster();
#endif
    
    fBitmapView->SetDrawingMode( DM_OVER );
    
    float strwidth = ceil(fBitmapView->GetStringWidth(timestr));
    fBitmapView->SetFgColor( textcol );
    fBitmapView->DrawString( timestr, os::Point((352-3)-strwidth,288+fontheight.nDescender) );

    char string[128] = "FridgeCAM";
    FILE *hf = fopen( "/tmp/camnotes.txt", "rt" );
    if( hf != NULL )
    {
	fgets( string, sizeof(string), hf );
	fclose( hf );

	int stringlen = strlen( string );
	while( stringlen && string[stringlen-1]<=0x20 )
	    stringlen--;
	string[stringlen] = 0;
    }

    uint stringlen = fBitmapView->GetStringLength( string, 352.0f-(strwidth+8.0f) );
    if( stringlen!=strlen(string) && stringlen>=3 )
	strcpy( string+(stringlen-3), "..." ); // This only works with monospaced fonts...
    fBitmapView->DrawString( string, os::Point(2,288+fontheight.nDescender) );

    fBitmapView->Sync();
}

//-----------------------------------------------------------------------------
