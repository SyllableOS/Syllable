/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */

#ifndef __F_UTIL_TYPECODES_H__
#define __F_UTIL_TYPECODES_H__

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent





/**\anchor os_util_typecodes
 * \par Description:
 *	Various type codes used to identify data-types by various classes in
 *	the toolkit. Most notably the os::Message class and os::Flattenable.
 *	For classes not part of the OS namespace, use codes equal to, or higher
 *	than, T_USER.
 *****************************************************************************/

enum type_code
{
    T_ANY_TYPE,
    T_POINTER,
    T_INT8,
    T_INT16,
    T_INT32,
    T_INT64,
    T_BOOL,
    T_FLOAT,
    T_DOUBLE,
    T_STRING,
    T_IRECT,
    T_IPOINT,
    T_MESSAGE,
    T_COLOR32,
    T_FILE,	// obsolete
    T_MEM_OBJ,	// obsolete
    T_RECT,
    T_POINT,
    T_VARIANT,
    T_RAW,
    
    T_SHORTCUTKEY,
	T_FONT,
    T_DATETIME,
    
    T_USER = 0x80000000
};

} // end of namespace


#endif // __F_UTIL_TYPECODES_H__

