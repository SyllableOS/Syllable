#include <stdio.h>

#include <util/application.h>
#include <gui/font.h>

#include "afont.h"

#include <map>

class QFontInternal
{
public:
    os::Font*	m_pcFont;
};

static std::map<int,os::Font*> g_cFonts;

os::Font* create_font( int nSize = 10 )
{
    std::map<int,os::Font*>::iterator i = g_cFonts.find( nSize );

    if ( i == g_cFonts.end() ) {
	os::font_properties sProp;
	os::Font::GetDefaultFont( DEFAULT_FONT_REGULAR, &sProp );
	sProp.m_vSize = float(nSize);
	os::Font* pcFont = new os::Font( DEFAULT_FONT_REGULAR ); // os::Font( sProp );
	pcFont->SetSize( nSize );
	pcFont->AddRef();
	g_cFonts.insert( std::make_pair( nSize, pcFont ) );
	return( pcFont );
    } else {
	(*i).second->AddRef();
	return( (*i).second );
    }
}

QFont::QFont()
{
    d = new QFontInternal;
    d->m_pcFont = create_font(); //   new os::Font( DEFAULT_FONT_REGULAR );
}
QFont::QFont( const QString& family, int pointSize = 12, int weight = Normal, bool italic = FALSE )
{
    d = new QFontInternal;
    d->m_pcFont = create_font( pointSize ); // new os::Font( DEFAULT_FONT_REGULAR );
}

QFont::QFont( const QFont& cOther )
{
    d = new QFontInternal;
    d->m_pcFont = create_font( cOther.pointSize() ); // new os::Font( *cOther.d->m_pcFont );
}

QFont::~QFont()
{
    d->m_pcFont->Release();
    delete d;
}

void QFont::setFamily( const QString &)
{
//    printf( "Warning: QFont::%s() not implemented\n", __FUNCTION__ );
}

QString QFont::family()	const
{
//    printf( "Warning: QFont::%s() not implemented\n", __FUNCTION__ );
    return QString();
}

void QFont::setCharSet( CharSet )
{
//    printf( "Warning: QFont::%s() not implemented\n", __FUNCTION__ );
}

QFont::CharSet QFont::charSet() const
{
//    printf( "Warning: QFont::%s() not implemented\n", __FUNCTION__ );
    return( ISO_8859_1 );
}
    

bool QFont::italic() const
{
//    printf( "Warning: QFont::%s() not implemented\n", __FUNCTION__ );
    return( false );
}

void QFont::setItalic( bool )
{
//    printf( "Warning: QFont::%s() not implemented\n", __FUNCTION__ );
}

int QFont::weight() const
{
//    printf( "Warning: QFont::%s() not implemented\n", __FUNCTION__ );
    return( Normal );
}

void QFont::setWeight( int )
{
//    printf( "Warning: QFont::%s() not implemented\n", __FUNCTION__ );
}

void QFont::setPointSize( int nSize )
{
    d->m_pcFont->Release();
    d->m_pcFont = create_font( nSize );
//    d->m_pcFont->SetSize( nSize );
}

void QFont::setPixelSize( int )
{
    printf( "Warning: QFont::%s() not implemented\n", __FUNCTION__ );
}
    
int QFont::pixelSize() const
{
    os::font_height sHeight;
    d->m_pcFont->GetHeight( &sHeight );
    return( int(sHeight.ascender + sHeight.descender + sHeight.line_gap) + 1 );
}

int QFont::pointSize() const
{
    return( int( d->m_pcFont->GetSize() ) );
}
    
QFont& QFont::operator=( const QFont& cOther )
{
    d->m_pcFont->Release();
    d->m_pcFont = create_font( cOther.d->m_pcFont->GetSize() ); // new os::Font( *cOther.d->m_pcFont );
//    printf( "Create/copy font\n" );
    return( *this );
}

bool QFont::operator==( const QFont& cOther ) const
{
    return( *d->m_pcFont == *cOther.d->m_pcFont );
}

bool QFont::operator!=( const QFont& cOther ) const
{
    return( *d->m_pcFont != *cOther.d->m_pcFont );
}


os::Font* QFont::GetFont() const
{
    return( d->m_pcFont );
}

void QFont::SetFont( os::Font* pcFont )
{
//    printf( "Set font\n" );
    d->m_pcFont->Release();
    d->m_pcFont = pcFont;
    d->m_pcFont->AddRef();
}
