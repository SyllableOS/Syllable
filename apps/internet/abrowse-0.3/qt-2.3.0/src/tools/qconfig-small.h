#define NO_CHECK
#ifndef QT_H
#endif // QT_H

#if !defined(QT_NO_QWS_DEPTH_16) ||  !defined(QT_NO_QWS_DEPTH_8) || !defined(QT_NO_QWS_DEPTH_32) || !defined(QT_NO_QWS_VGA_16)  
//We only need 1-bit support if we have a 1-bit screen
#define QT_NO_QWS_DEPTH_1
#endif


#define QT_NO_ACTION
#ifndef QT_NO_CODECS // moc?
#define QT_NO_TEXTCODEC
#endif
#define QT_NO_UNICODETABLES
#define QT_NO_IMAGEIO_BMP
#define QT_NO_IMAGEIO_PPM
#define QT_NO_IMAGEIO_XBM
#define QT_NO_IMAGEIO_XPM
#define QT_NO_IMAGEIO_PNG
#define QT_NO_ASYNC_IO
#define QT_NO_ASYNC_IMAGE_IO
#define QT_NO_FREETYPE
#define QT_NO_BDF
#define QT_NO_FONTDATABASE
#define QT_NO_TRANSLATION
#define QT_NO_MIME
#define QT_NO_SOUND
#define QT_NO_PROPERTIES

#define QT_NO_QWS_GFX_SPEED
#define QT_NO_NETWORK //??????????????
#define QT_NO_COLORNAMES
#define QT_NO_TRANSFORMATIONS
#define QT_NO_PRINTER
#define QT_NO_PICTURE

#define QT_NO_IMAGE_TRUECOLOR
//#define QT_NO_IMAGE_SMOOTHSCALE  //needed for iconset --> pushbutton
#define QT_NO_IMAGE_TEXT
#define QT_NO_DIR

#define QT_NO_TEXTSTREAM
#define QT_NO_DATASTREAM
#define QT_NO_QWS_SAVEFONTS
#define QT_NO_STRINGLIST
#define QT_NO_SESSIONMANAGER


#define QT_NO_DIALOG

#define QT_NO_SEMIMODAL

//#define QT_NO_STYLE //will require substantial work...

#define QT_NO_EFFECTS
#define QT_NO_COP

#define QT_NO_QWS_PROPERTIES

#define QT_NO_RANGECONTROL
#define QT_NO_SPLITTER
#define QT_NO_STATUSBAR
#define QT_NO_TABBAR
#define QT_NO_TOOLBAR
#define QT_NO_TOOLTIP
#define QT_NO_VALIDATOR
#define QT_NO_WHATSTHIS
#define QT_NO_WIDGETSTACK
#define QT_NO_ACCEL
#define QT_NO_SIZEGRIP
#define QT_NO_HEADER
#define QT_NO_WORKSPACE
#define QT_NO_LCDNUMBER
#define QT_NO_STYLE_MOTIF
#define QT_NO_STYLE_PLATINUM
#define QT_NO_PROGRESSBAR


#define QT_NO_QWS_HYDRO_WM_STYLE
#define QT_NO_QWS_BEOS_WM_STYLE
#define QT_NO_QWS_KDE2_WM_STYLE
#define QT_NO_QWS_KDE_WM_STYLE
//#define QT_NO_QWS_QPE_WM_STYLE
#define QT_NO_QWS_WINDOWS_WM_STYLE


//other widgets that could be removed:
//#define QT_NO_MENUDATA


//possible options:

//#define QT_NO_CURSOR
//#define QT_NO_LAYOUT
//#define QT_NO_QWS_MANAGER
//#define QT_NO_QWS_KEYBOARD
