#ifndef __F_WINSELECT_H__
#define __F_WINSELECT_H__

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

#include "layer.h"

#include <vector>

class SrvWindow;

class WinSelect : public Layer
{
public:
    WinSelect();
    ~WinSelect();

	void DrawWindowContent( SrvWindow* pcWindow );
	void RemoveWindow( SrvWindow* pcWindow );
	void UpdateWindow( SrvWindow* pcWindow );
    void UpdateWinList( bool bMoveToFront, bool bSetFocus );
    void Paint( const os::IRect& cUpdateRect, bool bUpdate );
    void Step( bool bForward );
    
    std::vector<SrvWindow*> GetWindows();
  
private:
    int	m_nCurSelect;
    std::vector<SrvWindow*> m_cWindows;
    SrvWindow*		  m_pcOldFocusWindow;
};



#endif // __F_WINSELECT_H__

