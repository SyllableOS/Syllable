
#include <qstring.h>

struct QFontDef;
class QFontInternal;

namespace os
{
    class Font;
};

class QFont
{
public:
    enum CharSet   { ISO_8859_1,  Latin1 = ISO_8859_1, AnyCharSet,
		     ISO_8859_2,  Latin2 = ISO_8859_2,
		     ISO_8859_3,  Latin3 = ISO_8859_3,
		     ISO_8859_4,  Latin4 = ISO_8859_4,
		     ISO_8859_5,
		     ISO_8859_6,
		     ISO_8859_7,
		     ISO_8859_8,
		     ISO_8859_9,  Latin5 = ISO_8859_9,
		     ISO_8859_10, Latin6 = ISO_8859_10,
		     ISO_8859_11, TIS620 = ISO_8859_11,
		     ISO_8859_12,
		     ISO_8859_13, Latin7 = ISO_8859_13,
		     ISO_8859_14, Latin8 = ISO_8859_14,
		     ISO_8859_15, Latin9 = ISO_8859_15,
		     KOI8R,
		     Set_Ja, Set_1 = Set_Ja,
		     Set_Ko,
		     Set_Th_TH,
		     Set_Zh,
		     Set_Zh_TW,
		     Set_N = Set_Zh_TW,
		     Unicode,
		     /* The following will need to be re-ordered later,
			since we accidentally left no room below "Unicode".
		        (For binary-compatibility that cannot change yet).
			The above will be obsoleted and a same-named list
			added below.
		     */
		     Set_GBK,
		     Set_Big5,

		     TSCII,
		     KOI8U,
		     CP1251,
		     PT154,
		     /* The following are font-specific encodings that
			we shouldn't need in a perfect world.
		     */
		     // 8-bit fonts
		     JIS_X_0201 = 0xa0,
		     // 16-bit fonts
		     JIS_X_0208 = 0xc0, Enc16 = JIS_X_0208,
		     KSC_5601,
		     GB_2312,
		     Big5
    };
    enum StyleHint { Helvetica, Times, Courier, OldEnglish,  System, AnyStyle,
		     SansSerif	= Helvetica,
		     Serif	= Times,
		     TypeWriter = Courier,
		     Decorative = OldEnglish};
    enum StyleStrategy { PreferDefault = 0x0001,
			  PreferBitmap = 0x0002,
			  PreferDevice = 0x0004,
			  PreferOutline = 0x0008,
			  ForceOutline = 0x0010,
			  PreferMatch = 0x0020,
			  PreferQuality = 0x0040 };
    enum Weight	   { Light = 25, Normal = 50, DemiBold = 63,
		     Bold  = 75, Black	= 87 };
    QFont();
    QFont( const QString& /*family*/, int pointSize = 12, int weight = Normal, bool italic = FALSE );
    QFont( const QFont & );
    ~QFont();

    void    setFamily( const QString &);
    QString family() const;
    void    setCharSet( CharSet );

    CharSet	charSet() const;
    

    bool	italic() const;
    void	setItalic( bool );

    int		weight() const;
    void	setWeight( int );

    void	setPointSize( int );
    void	setPixelSize( int );
    
    int		pixelSize() const;
    int		pointSize() const;
    
    QFont      &operator=( const QFont & );
    bool	operator==( const QFont & ) const;
    bool	operator!=( const QFont & ) const;

private:
    friend class QPainter;
    friend class QFontMetrics;
    
    os::Font* GetFont() const;
    void SetFont( os::Font* pcFont );
    
    QFontInternal* d;
};
