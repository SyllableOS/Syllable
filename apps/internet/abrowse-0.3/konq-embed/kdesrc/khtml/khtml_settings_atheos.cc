
#include "khtml_settings.h"

#include "khtmldefaults.h"

#define MAXFONTSIZES 15
// xx-small, x-small, small, medium, large, x-large, xx-large, ...
const int defaultXSmallFontSizes[MAXFONTSIZES] =
  {  5, 6, 7,  8,  9, 10, 12, 14, 16, 18, 24, 28, 34, 40, 48 };
const int defaultSmallFontSizes[MAXFONTSIZES] =
  {  6, 7,  8,  9, 10, 12, 14, 16, 18, 24, 28, 34, 40, 48, 56 };
const int defaultMediumFontSizes[MAXFONTSIZES] =
  {  7,  8,  9, 10, 12, 14, 16, 18, 24, 28, 34, 40, 48, 56, 68 };
const int defaultLargeFontSizes[MAXFONTSIZES] =
  {  9, 10, 11, 12, 14, 16, 20, 24, 28, 34, 40, 48, 56, 68, 82 };
const int defaultXLargeFontSizes[MAXFONTSIZES] =
  { 10, 12, 14, 16, 24, 28, 34, 40, 48, 56, 68, 82, 100, 120, 150 };



KHTMLSettings::KHTMLSettings()
{
    m_textColor = QColor( 0, 0, 0 );
    m_linkColor = QColor( 0, 0, 255 );
    m_vLinkColor = QColor( 255, 0, 0 );

    m_bChangeCursor = false;
    m_underlineLink = true;
    m_hoverLink     = true;

    m_bAutoLoadImages = true;

    
//    QString m_encoding;
    
//    QFont::CharSet m_charset;

//    QString availFamilies;

    m_fontSize    = 1;
    m_minFontSize = HTML_DEFAULT_MIN_FONT_SIZE;
//    QValueList<int>     m_fontSizes;
//    QFont::CharSet m_script;
    
    resetFontSizes();
}

void KHTMLSettings::init()
{
}
    
bool KHTMLSettings::isJavaEnabled( const QString& /*hostname*/ )
{
    return( false );
}

bool KHTMLSettings::isJavaScriptEnabled( const QString& /*hostname*/ )
{
    return( true );
}

bool KHTMLSettings::isPluginsEnabled( const QString& /*hostname*/ )
{
    return( false );
}

bool KHTMLSettings::isCSSEnabled( const QString& /*hostname*/ )
{
    return( true );
}


QString KHTMLSettings::userStyleSheet() const
{
    return( "" );
}
    

void KHTMLSettings::resetFontSizes()
{
    m_fontSizes.clear();
    for ( int i = 0; i < MAXFONTSIZES; i++ )
	switch( m_fontSize ) {
	    case -1:
		m_fontSizes << defaultXSmallFontSizes[ i ];
		break;
	    case 0:
		m_fontSizes << defaultSmallFontSizes[ i ];
		break;
	    case 2:
		m_fontSizes << defaultLargeFontSizes[ i ];
		break;
	    case 3:
		m_fontSizes << defaultXLargeFontSizes[ i ];
		break;
	    case 1:
	    default:
		m_fontSizes << defaultMediumFontSizes[ i ];
		break;
	}
    
}

    // these two can be set. Mainly for historical reasons (the method in KHTMLPart exists...)
void KHTMLSettings::setStdFontName(const QString& /*n*/ )
{
}

void KHTMLSettings::setFixedFontName(const QString& /*n*/ )
{
}

    // Font settings
QString KHTMLSettings::stdFontName() const
{
    return( "" );
}

QString KHTMLSettings::fixedFontName() const
{
    return( "" );
}

QString KHTMLSettings::serifFontName() const
{
    return( "" );
}

QString KHTMLSettings::sansSerifFontName() const
{
    return( "" );
}

QString KHTMLSettings::cursiveFontName() const
{
    return( "" );
}

QString KHTMLSettings::fantasyFontName() const
{
    return( "" );
}

void KHTMLSettings::setDefaultCharset( QFont::CharSet /*c*/, bool /*b*/ )
{
}

void KHTMLSettings::setCharset( QFont::CharSet /*c*/ )
{
}

void KHTMLSettings::setScript( QFont::CharSet c )
{
    m_script = c;
}




QString KHTMLSettings::settingsToCSS() const
{
    
    // lets start with the link properties
    QString str = "a:link {\ncolor: ";
    str += m_linkColor.name();
    str += ";";
    if(m_underlineLink)
	str += "\ntext-decoration: underline;";

    if( m_bChangeCursor )
    {
	str += "\ncursor: pointer;";
        str += "\n}\ninput[type=image] { cursor: pointer;";
    }
    str += "\n}\n";
    str += "a:visited {\ncolor: ";
    str += m_vLinkColor.name();
    str += ";";
    if(m_underlineLink)
	str += "\ntext-decoration: underline;";

    if( m_bChangeCursor )
	str += "\ncursor: pointer;";
    str += "\n}\n";

    if(m_hoverLink)
        str += "a:link:hover { text-decoration: underline; }\n";
	
    return str;
}
