#ifndef __F_APPS_CLIPBOARD_H__
#define __F_APPS_CLIPBOARD_H__

/*
 *  The AtheOS application server
 *  Copyright (C) 1999  Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <atheos/types.h>
#include <map>
#include <string>

class SrvClipboard
{
public:
    static void	SetData( const char* pzName, uint8* pzData, int nSize );
    static uint8*	GetData( const char* pzName, int* pnSize );
  
private:
    SrvClipboard() { m_pData = NULL; }
  
    typedef std::map<std::string,SrvClipboard*> ClipboardMap;
  
    static ClipboardMap s_cClipboardMap;
    uint8*	m_pData;
    int		m_nSize;
};


#endif // __F_APPS_CLIPBOARD_H__
