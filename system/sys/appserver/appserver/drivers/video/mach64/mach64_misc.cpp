#include "mach64.h"


/* Misc stuff */

//----------------------------------------------------------------
void ATImach64::SetColor(int nIndex, const Color32_s &sColor)
{
}
//----------------------------------------------------------------
int ATImach64::GetScreenModeCount() {
	return( m_cScreenModeList.size() );
}
//----------------------------------------------------------------
bool ATImach64::GetScreenModeDesc(int nIndex, ScreenMode *psMode) {
	if( nIndex < 0 || unsigned (nIndex) > m_cScreenModeList.size() )
		return( false );
	*psMode = m_cScreenModeList[nIndex];
		return( true );
}
//-----------------------------------------------------------------
int ATImach64::GetHorizontalRes() {
	if( unsigned(m_nCurrentMode) < m_cScreenModeList.size() )
		return( m_cScreenModeList[m_nCurrentMode].m_nWidth );
	return( -1 );
}
// ----------------------------------------------------------------
int ATImach64::GetVerticalRes() {
	if( unsigned(m_nCurrentMode) < m_cScreenModeList.size() )
		return( m_cScreenModeList[m_nCurrentMode].m_nHeight );
	return( -1 );
}
// ----------------------------------------------------------------
int ATImach64::GetBytesPerLine() { 
	if( unsigned (m_nCurrentMode) < m_cScreenModeList.size() )
		return( m_cScreenModeList[m_nCurrentMode].m_nBytesPerLine );
	return( -1 );
}
// ----------------------------------------------------------------
color_space ATImach64::GetColorSpace() {
	if( unsigned(m_nCurrentMode) < m_cScreenModeList.size() )
		return( m_cScreenModeList[m_nCurrentMode].m_eColorSpace );
	return( CS_CMAP8 );
}
// -----------------------------------------------------------------
int ATImach64::BytesPerPixel (color_space cs) {
	switch( cs ) {
		case    CS_RGB32:
		case    CS_RGBA32:
			return( 4 );
		case    CS_RGB24:
			return( 3 );
		case    CS_RGB16:
		case    CS_RGB15:
		case    CS_RGBA15:
			return( 2 );
		case CS_CMAP8:
		case CS_GRAY8:
			return( 1 );
		default:
			return( 1 );
	}
}
//  --------------------------------------------------------------
int ATImach64::BitsPerPixel (color_space cs) {
	switch( cs ) {
		case    CS_RGB32:
		case    CS_RGBA32:
			return( 32 );
		case    CS_RGB24:
			return( 24 );
		case    CS_RGB16:
			return( 16 );
		case    CS_RGB15:
		case    CS_RGBA15:
			return( 15 );
		case CS_CMAP8:
		case CS_GRAY8:
			return( 8 );
		default:
			return( 8 );
	}
}


