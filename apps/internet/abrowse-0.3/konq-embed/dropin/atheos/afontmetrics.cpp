#include <stdio.h>
#include <math.h>

#include "afontmetrics.h"
#include "afont.h"
#include <gui/font.h>

class QFontMetricsInternal
{
public:
    os::Font* m_pcFont;
};

QFontMetrics::QFontMetrics()
{
    d = new QFontMetricsInternal;
    d->m_pcFont = NULL;
}

QFontMetrics::QFontMetrics( const QFont& cFont )
{
    d = new QFontMetricsInternal;
    d->m_pcFont = cFont.GetFont();
    d->m_pcFont->AddRef();
}

QFontMetrics::QFontMetrics( const QFontMetrics& cOther )
{
    d = new QFontMetricsInternal;
    d->m_pcFont = cOther.d->m_pcFont;
    if ( d->m_pcFont != NULL ) {
	d->m_pcFont->AddRef();
    }
}

QFontMetrics::QFontMetrics( os::Font* pcFont )
{
    d = new QFontMetricsInternal;
    d->m_pcFont = pcFont;
    d->m_pcFont->AddRef();
}

QFontMetrics::~QFontMetrics()
{
    if ( d->m_pcFont != NULL ) {
	d->m_pcFont->Release();
    }
    delete d;
}

QFontMetrics& QFontMetrics::operator=( const QFontMetrics& cOther)
{
    if ( d->m_pcFont != NULL ) {
	d->m_pcFont->Release();
    }
    d->m_pcFont = cOther.d->m_pcFont;
    if ( d->m_pcFont != NULL ) {
	d->m_pcFont->AddRef();
    }
    return( *this );
}


void QFontMetrics::setFont( const QFont& cFont )
{
    if ( d->m_pcFont != NULL ) {
	d->m_pcFont->Release();
    }
    d->m_pcFont = cFont.GetFont();
    if ( d->m_pcFont != NULL ) {
	d->m_pcFont->AddRef();
    }
    
}

int QFontMetrics::height() const
{
    if ( d->m_pcFont == NULL ) {
	return( 0 );
    }
    os::font_height sHeight;
    d->m_pcFont->GetHeight( &sHeight );
    return( int(sHeight.ascender + sHeight.descender + sHeight.line_gap) + 1 );
}

int QFontMetrics::width( const QString& cString, int len ) const
{
    if ( d->m_pcFont == NULL ) {
	return( 0 );
    }
    QCString cUtf8Str = cString.utf8();
    const char* pzStr = cUtf8Str.data();
    int nByteLen;
    if ( len == -1 ) {
	nByteLen = strlen( pzStr );
    } else {
	nByteLen = 0;
	const char* p = pzStr;
	for ( int i = 0 ; i < len ; ++i ) {
	    int nCharLen = os::utf8_char_length( *p );
	    p += nCharLen;
	    nByteLen += nCharLen;
	}
    }
    return( int( ceil( d->m_pcFont->GetStringWidth( pzStr, nByteLen ) ) ) );
}

int QFontMetrics::width( QChar cChar ) const
{
    return( width( QString( cChar ) ) );
}
int QFontMetrics::width( char c ) const
{
    return width( (QChar) c );
}

int QFontMetrics::ascent() const
{
    if ( d->m_pcFont == NULL ) {
	return( 0 );
    }
    os::font_height sHeight;
    d->m_pcFont->GetHeight( &sHeight );
    return( int(sHeight.ascender + sHeight.line_gap) );
}

int QFontMetrics::descent() const
{
    if ( d->m_pcFont == NULL ) {
	return( 0 );
    }
    os::font_height sHeight;
    d->m_pcFont->GetHeight( &sHeight );
    return( int(sHeight.descender) );
}
    
int QFontMetrics::rightBearing( QChar c ) const
{
    return( width( c ) );
}
    
QRect QFontMetrics::boundingRect( const QString& cStr, int len = -1 ) const
{
    int nWidth = width( cStr, len );

    os::font_height sHeight;
    d->m_pcFont->GetHeight( &sHeight );
    
    return( QRect( QPoint( 0, -(sHeight.ascender + sHeight.line_gap) ), QPoint( nWidth - 1, sHeight.descender ) ) );
}

QRect QFontMetrics::boundingRect( QChar cChar ) const
{
    return( boundingRect( QString( cChar ) ) );
}
