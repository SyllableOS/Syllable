#ifndef TSETTINGS_H
#define TSETTINGS_H


#include <gui/gfxtypes.h>
#include <gui/point.h>


using namespace os;


struct sColor
{
    uint8 red;
    uint8 green;
    uint8 blue;
    char *name;
};

extern struct sColor Colors[];


class TermSettings
{
  public:
    static Color32 GetPaletteColor( const uint8 index );
    static uint8 GetPaletteIndex( const Color32 &col );

      TermSettings();

    void Print() const;

    void RevertToDefaults();
    void Load();
    void Save( bool bSavePositionOnly = false ) const;

    void SetPosition( const IPoint & pos );
    const IPoint & GetPosition() const;

    void SetSize( const IPoint & size );
    const IPoint & GetSize() const;

    void SetNormalTextColor( const Color32 &color );
    void SetNormalBackgroundColor( const Color32 &color );
    void SetSelectedTextColor( const Color32 &color );
    void SetSelectedBackgroundColor( const Color32 &color );

    const Color32 &GetNormalTextColor() const;
    const Color32 &GetNormalBackgroundColor() const;
    const Color32 &GetSelectedTextColor() const;
    const Color32 &GetSelectedBackgroundColor() const;

    void SetIBeam( bool ibeam );
    bool GetIBeam() const;

  private:
      IPoint m_cPosition;
    IPoint m_cSize;
    Color32 m_cNormalTextColor;
    Color32 m_cNormalBackgroundColor;
    Color32 m_cSelectedTextColor;
    Color32 m_cSelectedBackgroundColor;
    bool m_bIBeam;
};


#endif // TSETTINGS_H
