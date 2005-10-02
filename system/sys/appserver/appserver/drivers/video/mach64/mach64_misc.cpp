#include "mach64.h"


/* Misc stuff */

//----------------------------------------------------------------
void ATImach64::SetColor(int nIndex, const Color32_s &sColor)
{
}
//----------------------------------------------------------------
int ATImach64::GetScreenModeCount() {
	if( m_bVESA )
		return( M64VesaDriver::GetScreenModeCount() );
	return( m_cScreenModeList.size() );
}
//----------------------------------------------------------------
bool ATImach64::GetScreenModeDesc(int nIndex, os::screen_mode *psMode) {
	if( m_bVESA )
		return( M64VesaDriver::GetScreenModeDesc( nIndex, psMode ) );
	if( nIndex < 0 || unsigned (nIndex) > m_cScreenModeList.size() )
		return( false );
	*psMode = m_cScreenModeList[nIndex];
		return( true );
}
//-----------------------------------------------------------------
os::screen_mode ATImach64::GetCurrentScreenMode() {
	if( m_bVESA )
		return( M64VesaDriver::GetCurrentScreenMode() );
	return( m_sCurrentMode );
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
		case CS_CMAP8:
		case CS_GRAY8:
			return( 8 );
		default:
			return( 8 );
	}
}




