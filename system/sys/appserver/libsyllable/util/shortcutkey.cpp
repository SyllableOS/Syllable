#include <util/shortcutkey.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

using namespace os;

ShortcutKey::ShortcutKey( const char* pzKey, uint32 nQualifiers = 0 )
{
	if( ( strncmp( pzKey, "ALT+", 4 ) == 0 ) || ( strncmp( pzKey, "alt+", 4 ) == 0 ) ) {
		pzKey += 4;
		nQualifiers |= QUAL_ALT;
	} else if( ( strncmp( pzKey, "SHIFT+", 6 ) == 0 ) || ( strncmp( pzKey, "SHIFT+", 6 ) == 0 ) ) {
		pzKey += 6;
		nQualifiers |= QUAL_SHIFT;
	} else if( ( strncmp( pzKey, "CTRL+", 5 ) == 0 ) || ( strncmp( pzKey, "CTRL+", 5 ) == 0 ) ) {
		pzKey += 5;
		nQualifiers |= QUAL_CTRL;
	}

	m_pzKey = new char[ strlen( pzKey ) + 1 ];
	strcpy( m_pzKey, pzKey );
	m_nQualifiers = nQualifiers;
}

ShortcutKey::ShortcutKey( const char cKey, uint32 nQualifiers = 0 )
{
	m_pzKey = new char[2];
	m_pzKey[0] = cKey;
	m_pzKey[1] = 0;
	m_nQualifiers = nQualifiers;
}

ShortcutKey::ShortcutKey( const ShortcutKey& cShortcut ) {
	m_nQualifiers = cShortcut.m_nQualifiers;
	m_pzKey = new char[ strlen( cShortcut.m_pzKey ) + 1 ];
	strcpy( m_pzKey, cShortcut.m_pzKey );
}

ShortcutKey::ShortcutKey()
{
	m_pzKey = new char[1];
	m_pzKey[0] = 0;
	m_nQualifiers = 0;
}

ShortcutKey::~ShortcutKey()
{
	delete []m_pzKey;
}

bool ShortcutKey::operator<( const ShortcutKey& c ) const
{
	return strcmp( m_pzKey, c.m_pzKey ) < 0;
}

bool ShortcutKey::operator==( const ShortcutKey& c ) const
{
	uint32 nOnes = m_nQualifiers | c.m_nQualifiers;
	uint32 nZeros = m_nQualifiers & c.m_nQualifiers;
	return	( ( nZeros & QUAL_CTRL ) || ( ! ( nOnes & QUAL_CTRL ) ) ) &&
			( ( nZeros & QUAL_SHIFT ) || ( ! ( nOnes & QUAL_SHIFT ) ) ) &&
			( ( nZeros & QUAL_ALT ) || ( ! ( nOnes & QUAL_ALT ) ) ) &&
			( strcmp( m_pzKey, c.m_pzKey ) == 0 ) ;
}

