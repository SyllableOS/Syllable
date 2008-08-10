#ifndef __F_UTIL_KEYMAP_H__
#define __F_UTIL_KEYMAP_H__

#include <appserver/keymap.h>
#include <util/string.h>
#include <storage/directory.h>
#include <util/application.h>
#include <util/flattenable.h>
#include <gui/exceptions.h>

#include <vector>
#include <stdint.h>

namespace os
{
	

/** Keymap class
 * \ingroup util
 * \par Description:
 *	Keymaps are handled by the appserver, but as time goes on
 *  we realized that we needed some way of allowing the user to see this data.
 *
 *	\note This class will allow you to see the data, but not manipulate anything
 *	
 * 	\note A brief history on how keymaps work in Syllable:
 *		A keymap starts out as a plain text file in Syllable and then is converted into 
 *		a binary file for use in the appserver.  This binary file can be pretty hard
 *		to decipher, but if you need a glimpse into how it works take a look at
 *		appserver/keymap.h in your include directory.
 * \author	Rick Caudill(rick@syllable.org)
 *****************************************************************************/
	
class Keymap
{
public:
	
	/** Default Constructor...
     * \param cKeymap - an os::String that contains the actual keymap you want to look at.  If you just create
     * 					an instance with Keymap() the class will use the current keymap loaded by the appserver
     * \author Rick Caudill(rick@syllable.org)
	 **************************************************************************/	
	Keymap(const os::String& cKeymap="");
	virtual ~Keymap();
	
	
	/** Get the keymap directory
     * \author Rick Caudill(rick@syllable.org)
	 **************************************************************************/
	static os::String GetKeymapDirectory();
	
	/**	Gets all keymap names that the appserver can currently use
	 * \author Rick Caudill(rick@syllable.org)	
	 *
	 **************************************************************************/
	static std::vector<os::String> GetKeymapNames();
	
	
	/** Finds a character in the keymap based on the normal key set	
	 *
	 * \author Rick Caudill(rick@syllable.org)
	 **************************************************************************/
	int32_t Find(char zFind);
	
	/**	Finds all occurances of the character in all of the keymap
	 *
	 * \author Rick Caudill(rick@syllable.org)
	 **************************************************************************/	
	std::vector< std::pair<int32_t, int32_t> > FindAllOccurances(char zFind);
	
	/**	Returns an std::vector of ints corresponding to the normal keys in the keymap	
	 *
	 * \author Rick Caudill(rick@syllable.org)
	 **************************************************************************/		 
	std::vector<int32_t> GetKeys();
	
	/*	gets the actual keymap data that is found out by the appserver	*/
	keymap* GetKeymapData() const;


private:
	void	_Init(const os::String& cKeymap);

private:
	keymap* m_pcMap;
};
}
#endif






















