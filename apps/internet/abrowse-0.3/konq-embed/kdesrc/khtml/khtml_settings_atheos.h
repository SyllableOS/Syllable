


#include <qcolor.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qfont.h>
#include <qmap.h>



/**
 * Settings for the HTML view.
 */
class KHTMLSettings
{
public:

    /**
     * This enum specifies whether Java/JavaScript execution is allowed.
     */
    enum KJavaScriptAdvice {
	KJavaScriptDunno=0,
	KJavaScriptAccept,
	KJavaScriptReject
    };

    /**
     * @internal Constructor
     */
    KHTMLSettings();

    /** Called by constructor and reparseConfiguration */
    void init();

    // Autoload images
    bool autoLoadImages() { return m_bAutoLoadImages; }

    
    bool isJavaEnabled( const QString& hostname = QString::null );
    bool isJavaScriptEnabled( const QString& hostname = QString::null );
    bool isPluginsEnabled( const QString& hostname = QString::null );
    bool isCSSEnabled( const QString& hostname = QString::null );

    QString userStyleSheet() const;
    
    QString settingsToCSS() const;

    const QValueList<int> &fontSizes() const { return m_fontSizes; }
    int minFontSize() const { return m_minFontSize; }
    QFont::CharSet charset() const { return m_charset; }
    QString availableFamilies() const { return availFamilies; }
    void resetFontSizes();

    // these two can be set. Mainly for historical reasons (the method in KHTMLPart exists...)
    void setStdFontName(const QString &n);
    void setFixedFontName(const QString &n);
    
    // Font settings
    QString stdFontName() const;
    QString fixedFontName() const;
    QString serifFontName() const;
    QString sansSerifFontName() const;
    QString cursiveFontName() const;
    QString fantasyFontName() const;

    void setDefaultCharset( QFont::CharSet c, bool b );
    void setCharset( QFont::CharSet c );
    QFont::CharSet script() const { return m_script; }
    void setScript( QFont::CharSet c );
    const QString &encoding() const { return m_encoding; }
    
private:
    QColor m_textColor;
    QColor m_linkColor;
    QColor m_vLinkColor;

    bool m_bChangeCursor;
    bool m_underlineLink;
    bool m_hoverLink;

    bool m_bAutoLoadImages;
    
    QString m_encoding;
    
    QFont::CharSet m_charset;

    QString availFamilies;
    
    int m_fontSize;
    int m_minFontSize;
    QValueList<int>     m_fontSizes;
    QFont::CharSet m_script;
    
};
