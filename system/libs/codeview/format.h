/*	libcodeview.so - A programmers editor widget for Atheos
	Copyright (c) 2001 Andreas Engh-Halstvedt
	Copyright (c) 2003 Henrik Isaksson

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Library General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.
	
	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Library General Public License for more details.
	
	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free
	Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
	MA 02111-1307, USA
*/

#ifndef F_CODEVIEW_FORMAT_H
#define F_CODEVIEW_FORMAT_H

#include <gui/gfxtypes.h>
#include <atheos/types.h>

#include <string>

namespace cv
{

/** Base class for formatter contexts.
 * \par Description:
 *	Context should be subclassed by formatters that require more
 *	complex context handling than a single 32 bit integer.
 *
 * \sa Format
 * \author	Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
class CodeViewContext
{
public:
    CodeViewContext( uint32 n ) { nContext = n; }
    CodeViewContext() { nContext = 0; }
    virtual int operator==( const CodeViewContext& c ) const { return nContext == c.nContext; }
    uint32	nContext;
};

/** Base class for formatter styles.
 * \par Description:
 *	This class holds style information for different contexts.
 *
 * \sa Format
 * \author	Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
class CodeViewStyle
{
public:
    os::Color32_s	sColor;
};

/**The base class for all source code formaters.
 *
 * To implement syntax coloring and other features for a new language,
 * you must write a class inheriting from this and set it on the
 * %CodeView with CodeView::SetFormat()
 *
 * \author Andreas Engh-Halstvedt (aeh@operamail.com)
 */
class Format{
public:

    /**Get the number of different styles supported by this format.
     * Each format has a maximum number of styles it may use.
     * This number can be in the range 1-255, and should not change 
     * during use, but the format is not required to actually use 
     * all of them.
     *
     * \return The number of styles supported by this format.
     */
    virtual uint GetStyleCount() = 0;

    /**Get the name of the given style.
     * This method must return a name for each of the styles
     * in the range from 0 through getStyleCount()-1.
     * The method is not defined for styles outside of this range, but
     * implementors are encouraged to handle this gracefully.
     * <p>The names are intended used in configuration dialogs.
     *
     * \param style The number of the style to return.
     * \return A string containing the display name of the given style.
     */
    virtual const string& GetStyleName( char nStyle ) = 0;

    /**Set the color used to display the given style.
     * This method must accept a color for each of the styles
     * in the range from 0 through getStyleCount()-1.
     * The method is not defined for methods outside of this range, but
     * implementors are encouraged to handle this gracefully.
     *
     * \param nStyle The number of the style to set.
     * \param cStyle The color to display the given style with.
     */
    virtual void SetStyle( char nStyle, const CodeViewStyle& cStyle ) = 0;

    /**Get the color used to display the given style.
     * This method must return a color for each of the styles
     * in the range from 0 through getStyleCount()-1.
     * The method is not defined for methods outside of this range, but
     * implementors are encouraged to handle this gracefully.
     *
     * \param nStyle The number of the style to return.
     * \return The color to display the given style with.
     */
    virtual const CodeViewStyle& GetStyle( char nStyle ) = 0;

    /**Parse a line and assign styles to each character.
     * The method will parse a single line of text and store the
     * correct style to use for each character. After the method
     * returns, character text[n] should be displayed using the style
     * stored in style[n].
     * <p>The format is responsible for resizing the style string 
     * to the correct size.
     * <p>To correctly parse constructs spanning multiple lines,
     * the format may store per-line context information in a 32-bit
     * value. For the first line this value is defined to be 0, but 
     * for all other lines the value returned for the previous line
     * should be passed as the context parameter.	
     *
     * \param text The line of text to parse.
     * \param style A string to put the styles into.
     * \param context A value representing the current context.
     * \return A value representing the new context.
     */
    virtual CodeViewContext Parse( const std::string &cText, std::string &cStyle,
	CodeViewContext nContext ) = 0;

	
    /**Get the string used to prefix the next line.
     * The format will analyze the given line and return a string
     * to be used as prefix to the next line. This may be used to
     * implement auto indenting of code.
     * <p>As an example, for C++ the line "\t\tFlush();" may return
     * the string "\t\t".
     *
     * \param cText The line to analyze.
     * \param bUseTabs Use TABs to indent.
     * \param nTabSize Length of each TAB.
     * \return The string to prefix the next string with.
     * \sa CodeView::setTabSize()
     * \sa CodeView::getTabSize()
     * \sa CodeView::setUseTab()
     * \sa CodeView::getUseTab()
     */
    virtual std::string GetIndentString( const std::string &cText, bool bUseTabs, uint nTabSize)=0;
	
    /**Get the character index of the previous word limit.
     * What is considered a word is up to the format.
     * \param nLine The line to analyze.
     * \param nChr The character index to start from.
     * \return The character index of the previous word limit, or 0
     * if the string if none are found.
     */
    virtual uint GetPreviousWordLimit( const std::string &cLine, uint nChr)=0;

    /**Get the character index of the next word limit.
     * What is considered a word is up to the format.
     * \param nLine The line to analyze.
     * \param nChr The character index to start from.
     * \return The character index of the next word limit, or the length
     * if the string if none are found.
     */
    virtual uint GetNextWordLimit( const std::string &cLine, uint nChr) = 0;

    /** Find foldable sections.
     * Returns nOldFoldLevel + 1 if cLine contains something that can start
     * a foldable section. Returns nOldFoldLevel - 1 if the line contains
     * something that can end a foldable section. If the line doesn't contain
     * any of those, nOldFoldLevel is returned unchanged.
     * \param cLine A line parse.
     * \param nOldFoldLevel Set to 0 in the first call. In subsequent calls,
     * it should be passed the value returned by the previous call.
     */
    virtual int GetFoldLevel( const std::string &cLine, int nOldFoldLevel ) { return nOldFoldLevel; }
};

} /* end of namespace */

#endif /* F_CODEVIEW_FORMAT_H */

