#ifndef __F_CATEDIT_CATALOG_H__
#define __F_CATEDIT_CATALOG_H__

#include <atheos/types.h>
#include <util/string.h>
#include <util/resource.h>
#include <util/locker.h>
#include <util/string.h>
#include <storage/streamableio.h>
#include <map>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent

class Locale;

class CCatalog : public Resource
{
public:
	CCatalog( );
	CCatalog( StreamableIO* pcSource  );
	CCatalog( String& cName, Locale *pcLocale = NULL );
    ~CCatalog();

	const String& GetString( uint32 nID ) const;
	void SetString( uint32 nID, const String& cStr ) const;

	const String GetComment( uint32 nID ) const;
	void SetComment( uint32 nID, const String& cStr ) const;

	const String GetMnemonic( uint32 nID ) const;
	void SetMnemonic( uint32 nID, const String& cStr ) const;

	status_t Load( StreamableIO* pcSource );
	status_t Save( StreamableIO* pcDest );

	typedef std::map<uint32, String> StringMap;
	typedef StringMap::const_iterator const_iterator;

      // STL iterator interface to the strings.
    const_iterator begin() const;
    const_iterator end() const;

	int Lock() const;
	int Unlock() const;

private:
	struct FileHeader;
	class Private;
	Private *m;
};


} // end of namespace

#endif // __F_CATEDIT_CATALOG_H__


