#ifndef FRIDGECAM_FCAPP_H
#define FRIDGECAM_FCAPP_H
//-----------------------------------------------------------------------------

#include <gui/application.h>

//-----------------------------------------------------------------------------

class FCWin;
class CPiACam;
namespace os { class Bitmap; class View; }

class FCApp : public os::Application
{
public:
    enum messages
    {
	MSG_SETCAMERAPARMS = 'CAMP',
//	MSG_FRAMERATE = 'FRTE',
//	MSG_ENABLE_MANUAL_EXPOSURE = 'MEXP',
//	MSG_EXPOSURE = 'EXPO',
//	MSG_GAIN = 'GAIN',
//	MSG_COMPRESSION = 'CMPR',
//	MSG_DECIMATE = 'DECM',
//	MSG_AUTOTARGET = 'AUTA',
//	MSG_AUTOQUALITY = 'AUQU',
//	MSG_AUTOFRAMERATE = 'AUFR',
//	MSG_MANUALYQUALITY = 'MAYQ',
//	MSG_MANUALUVQUALITY = 'MAUV',
    };



    FCApp();
    ~FCApp();

    const os::Bitmap *GetBitmap() const { return fBitmap; }

    CPiACam *GetCamera() const { return fCam; }

protected:
    virtual bool OkToQuit();
    virtual void Quit();

    virtual void HandleMessage( os::Message *message );
    
private:
    static long _WorkThread( void *data );
    void WorkThread();

    void TimestampBitmap( os::Bitmap *pcBitmap, const char *msg=NULL );
    void DrawString( os::View *view, const os::Point &pos, const char *text );
    
    CPiACam		*fCam;
    
    port_id		fFrameReadyPort;
    thread_id		fWorkThread;
    volatile bool	fWorkQuit;
    os::MessageQueue	fWorkMessageQueue;

    os::Bitmap		*fCaptureBitmap;
    os::Bitmap		*fBitmap;
    os::View		*fBitmapView;

    FCWin 		*fWindow;
};

//-----------------------------------------------------------------------------
#endif
