#ifndef __F_AFRAME_H__
#define __F_AFRAME_H__



#include <qwidget.h>




class QFramePrivate;

class QFrame : public QWidget
{
    Q_OBJECT
public:
    enum Shape { NoFrame  = 0,				// no frame
		 Box	  = 0x0001,			// rectangular box
		 Panel    = 0x0002,			// rectangular panel
		 WinPanel = 0x0003,			// rectangular panel (Windows)
		 HLine    = 0x0004,			// horizontal line
		 VLine    = 0x0005,			// vertical line
		 StyledPanel = 0x0006,			// rectangular panel depending on the GUI style
		 PopupPanel = 0x0007,			// rectangular panel depending on the GUI style
		 MShape   = 0x000f			// mask for the shape
    };
    enum Shadow { Plain    = 0x0010,			// plain line
		  Raised   = 0x0020,			// raised shadow effect
		  Sunken   = 0x0030,			// sunken shadow effect
		  MShadow  = 0x00f0 };			// mask for the shadow
    
    QFrame( QWidget* pcParent, const char* pzName );
    ~QFrame();
    virtual void setFrameStyle( int );
    int		frameStyle()	const;
private:
    QFramePrivate* d;
};

#endif // __F_AFRAME_H__
