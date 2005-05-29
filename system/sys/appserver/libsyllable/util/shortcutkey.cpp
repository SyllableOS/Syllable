#include <util/shortcutkey.h>
#include <gui/font.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wctype.h>

using namespace os;

ShortcutKey::ShortcutKey( const String& cKey, uint32 nQualifiers )
{
	const char *pzKey = cKey.c_str();
	if( ( strncasecmp( pzKey, "ALT+", 4 ) == 0 ) ) {
		pzKey += 4;
		nQualifiers |= QUAL_ALT;
	} else if( ( strncasecmp( pzKey, "SHIFT+", 6 ) == 0 ) ) {
		pzKey += 6;
		nQualifiers |= QUAL_SHIFT;
	} else if( ( strncasecmp( pzKey, "CTRL+", 5 ) == 0 ) ) {
		pzKey += 5;
		nQualifiers |= QUAL_CTRL;
	}

	_SetKey( pzKey );
	m_nQualifiers = nQualifiers;
}

ShortcutKey::ShortcutKey( const uint32 nKey, uint32 nQualifiers )
{
	m_nKey = nKey;
	m_nQualifiers = nQualifiers;
}

ShortcutKey::ShortcutKey( const ShortcutKey& cShortcut )
{
	m_nQualifiers = cShortcut.m_nQualifiers;
	m_nKey = cShortcut.m_nKey;
}

ShortcutKey::ShortcutKey()
{
	m_nKey = 0;
	m_nQualifiers = 0;
}

ShortcutKey::~ShortcutKey()
{
}

bool ShortcutKey::operator<( const ShortcutKey& c ) const
{
	if( !IsValid() || !c.IsValid() ) {
		dbprintf( "ShortcutKey::operator<() called with invalid shortcut!\n" );
	}

	if( m_nKey == c.m_nKey ) {
		return m_nQualifiers < c.m_nQualifiers ;
	} else {
		return m_nKey < c.m_nKey ;
	}
}

bool ShortcutKey::operator==( const ShortcutKey& c ) const
{
	uint32 nOnes = m_nQualifiers | c.m_nQualifiers;
	uint32 nZeros = m_nQualifiers & c.m_nQualifiers;

	if( !IsValid() ) return false;
	
	return	( ( nZeros & QUAL_CTRL ) || ( ! ( nOnes & QUAL_CTRL ) ) ) &&
			( ( nZeros & QUAL_SHIFT ) || ( ! ( nOnes & QUAL_SHIFT ) ) ) &&
			( ( nZeros & QUAL_ALT ) || ( ! ( nOnes & QUAL_ALT ) ) ) &&
			( m_nKey == c.m_nKey  ) ;
}

void ShortcutKey::SetFromLabel( const String& cLabel )
{
	SetFromLabel( cLabel.c_str() );
}

void ShortcutKey::SetFromLabel( const char* pzLabel )
{
	int i, len = strlen( pzLabel );

	for( i = 0; i < len; i+=utf8_char_length( pzLabel[i] ) ) {
		if( pzLabel[i] == '_' && pzLabel[i+1] != '_' ) {
			_SetKey( &pzLabel[i+1] );
			return;
		}
	}
	_SetKey( "" );
}

bool ShortcutKey::IsValid() const
{
	return ( m_nKey );
}

void ShortcutKey::_SetKey( const char* pzKey )
{
	uint32 unicode = utf8_to_unicode( pzKey );

	m_nKey = towupper( unicode );
}

uint32 ShortcutKey::GetKeyCode() const
{
	return m_nKey;
}

uint32 ShortcutKey::GetQualifiers() const
{
	return m_nQualifiers;
}

status_t ShortcutKey::Flatten( uint8* pBuffer, size_t nSize ) const
{
	if( nSize >= 16 ) {
		uint32* p = (uint32*)pBuffer;
		p[0] = m_nKey;
		p[1] = m_nQualifiers;
		p[2] = 0;
		p[3] = 0;
		return 0;
	} else {
		return -1;
	}
}

status_t ShortcutKey::Unflatten( const uint8* pBuffer )
{
	uint32* p = (uint32*)pBuffer;
	m_nKey = p[0];
	m_nQualifiers = p[1];
	return 0;
}
