
#include <util/settings.h>

#include <stdio.h>
#include "settings.h"

/**
 * 16 colors palette
 */
struct sColor Colors[] = {
    {0x00, 0x00, 0x00, "Black"},    //  0
    {0xbf, 0x00, 0x00, "Red"},  //  1
    {0x00, 0xbf, 0x00, "Green"},    //  2
    {0xbf, 0xbf, 0x00, "Brown"},    //  3
    {0x00, 0x00, 0xbf, "Blue"}, //  4
    {0xbf, 0x00, 0xbf, "Magenta"},  //  5
    {0x00, 0xbf, 0xbf, "Cyan"}, //  6
    {0xdf, 0xdf, 0xdf, "LightGray"},    //  7
    {0x80, 0x80, 0x80, "Dark gray"},    //  8
    {0x00, 0x00, 0xff, "Light blue"},   //    9
    {0x00, 0xff, 0x00, "Light green"},  //   a
    {0x00, 0xff, 0xff, "Light cyan"},   //    b
    {0xff, 0x00, 0x00, "Light red"},    //     c
    {0xff, 0x00, 0xff, "Light magenta"},    // d
    {0xff, 0xff, 0x00, "Light brown"},  //   e
    {0xff, 0xff, 0xff, "white"} //          f
};

/**
 * Get RGB color from palette index
 *
 * This static method returns RGB color from the provided index.
 * Index range is 0 - 15.
 *
 */
Color32 TermSettings::GetPaletteColor( const uint8 index )
{
    if( index < 16 ) {
        return Color32 ( Colors[index].red, Colors[index].green,
            Colors[index].blue );
    } else {
        return Color32 ( 0, 0, 0 );
    }
}

/**
 * Get palette index from RGB color
 *
 * If the provided RGB color is within the 16-color palette, this
 * static method will return the corresponding index ; else, it will
 * return zero (which is black).
 *
 */
uint8 TermSettings::GetPaletteIndex( const Color32 &col )
{
    uint8 i;

    for( i = 0; i < 16; i++ ) {
        if( col.red == Colors[i].red && col.green == Colors[i].green
            && col.blue == Colors[i].blue ) {
            return i;
        }
    }
    return 0;
}

/**
 * TermSettings constructor
 *
 * Try to load user's settings from filesystem, or initialize to factory settings
 *
 */
TermSettings::TermSettings()
{
    RevertToDefaults();
    Load();
}

void TermSettings::Print() const
{
    printf
        ( "[ptr(%p) Pos(%d,%d) Size(%d,%d) Colors(%d,%d,%d,%d) IBeam(%s)]\n",
        this, m_cPosition.x, m_cPosition.y, m_cSize.x, m_cSize.y,
        GetPaletteIndex( m_cNormalTextColor ),
        GetPaletteIndex( m_cNormalBackgroundColor ),
        GetPaletteIndex( m_cSelectedTextColor ),
        GetPaletteIndex( m_cSelectedBackgroundColor ),
        ( m_bIBeam ? "true" : "false" ) );
}

/**
 * Reinitialize structure from factory settings
 *
 */
void TermSettings::RevertToDefaults()
{
    m_cPosition = IPoint( 100, 100 );
    m_cSize = IPoint( 80, 25 );

/* White on black : */
    m_cNormalTextColor = GetPaletteColor( 0xf );
    m_cNormalBackgroundColor = GetPaletteColor( 0x0 );
    m_cSelectedTextColor = GetPaletteColor( 0xf );
    m_cSelectedBackgroundColor = GetPaletteColor( 0x8 );
    m_bIBeam = true;

/* Black on white :
    m_cNormalTextColor = GetPaletteColor( 0x0 );
    m_cNormalBackgroundColor = GetPaletteColor( 0xf );
    m_cSelectedTextColor = GetPaletteColor( 0x0 );
    m_cSelectedBackgroundColor = GetPaletteColor( 0x8 );
    m_bIBeam = false;
*/
}

/**
 * Load user settings from filesystem
 *
 * If user's settings cannot be read, reverts to factory settings.
 *
 */
void TermSettings::Load()
{
    /* Initialize to factory settings */
    RevertToDefaults();
    /* Load from user's settings */
    os::Settings * pcSettings = new os::Settings();
    pcSettings->Load();
    m_cPosition = pcSettings->GetIPoint( "Position", m_cPosition );
    m_cSize = pcSettings->GetIPoint( "Size", m_cSize );
    m_cNormalTextColor =
        pcSettings->GetColor32( "NormalTextColor", m_cNormalTextColor );
    m_cNormalBackgroundColor =
        pcSettings->GetColor32( "NormalBackgroundColor",
        m_cNormalBackgroundColor );
    m_cSelectedTextColor =
        pcSettings->GetColor32( "SelectedTextColor", m_cSelectedTextColor );
    m_cSelectedBackgroundColor =
        pcSettings->GetColor32( "SelectedBackgroundColor",
        m_cSelectedBackgroundColor );
    m_bIBeam = pcSettings->GetBool( "IBeam", m_bIBeam );
    delete( pcSettings );
}

/**
 * Save current state to user's settings on filesystem
 *
 * if \a bSavePositionOnly is true, only size and position will be overwritten.
 * Otherwise, all settings (colors, cursor, size and position) will be saved.
 *
 */
void TermSettings::Save( bool bSavePositionOnly ) const
{
    os::Settings * pcSettings = new os::Settings();
    pcSettings->Load();

/*    printf("Saving settings "); Print();*/

    pcSettings->SetIPoint( "Position", m_cPosition );
    pcSettings->SetIPoint( "Size", m_cSize );

    if( !bSavePositionOnly ) {
        pcSettings->SetColor32( "NormalTextColor", m_cNormalTextColor );
        pcSettings->SetColor32( "NormalBackgroundColor",
            m_cNormalBackgroundColor );
        pcSettings->SetColor32( "SelectedTextColor", m_cSelectedTextColor );
        pcSettings->SetColor32( "SelectedBackgroundColor",
            m_cSelectedBackgroundColor );
        pcSettings->SetBool( "IBeam", m_bIBeam );
    }
    pcSettings->Save();

    delete( pcSettings );
}

void TermSettings::SetPosition( const IPoint & pos )
{
    m_cPosition = pos;
}

const IPoint & TermSettings::GetPosition() const
{
    return m_cPosition;
}

void TermSettings::SetSize( const IPoint & size )
{
    m_cSize = size;
}

const IPoint & TermSettings::GetSize() const
{
    return m_cSize;
}

void TermSettings::SetNormalTextColor( const Color32 &color )
{
    m_cNormalTextColor = color;
}

void TermSettings::SetNormalBackgroundColor( const Color32 &color )
{
    m_cNormalBackgroundColor = color;
}

void TermSettings::SetSelectedTextColor( const Color32 &color )
{
    m_cSelectedTextColor = color;
}

void TermSettings::SetSelectedBackgroundColor( const Color32 &color )
{
    m_cSelectedBackgroundColor = color;
}

const Color32 &TermSettings::GetNormalTextColor() const
{
    return m_cNormalTextColor;
}

const Color32 &TermSettings::GetNormalBackgroundColor() const
{
    return m_cNormalBackgroundColor;
}

const Color32 &TermSettings::GetSelectedTextColor() const
{
    return m_cSelectedTextColor;
}

const Color32 &TermSettings::GetSelectedBackgroundColor() const
{
    return m_cSelectedBackgroundColor;
}

void TermSettings::SetIBeam( bool ibeam )
{
    m_bIBeam = ibeam;
}

bool TermSettings::GetIBeam() const
{
    return m_bIBeam;
}
