/*
 *  AtheMgr - System Manager for AtheOS
 *  Copyright (C) 2001 John Hall
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#ifndef __F_DRIVESPANEL_H_
#define __F_DRIVESPANEL_H_

#include <gui/layoutview.h>
#include <gui/textview.h>
#include <gui/listview.h>

#include <atheos/types.h>

#include <map>

using namespace os;

namespace os
{
	class TextView;
	class ListView;
	class ListViewStringRow;
}

#define NUM_OF_HD_ROWS 50

class DriveRow : public os::ListViewStringRow
{
	public:
	off_t		nSize;
};

class DrivesPanel:public os::LayoutView
{
      public:

	DrivesPanel( const os::Rect & cFrame );

	virtual void FrameSized( const os::Point & cDelta );
	virtual void AllAttached();
	virtual void HandleMessage( Message * pcMessage );

	void SetupPanel();
	void UpdatePanel();

      private:
	void SetUpHDView();

	void UpdateHDInfo( bool bUpdate );

	DriveRow *AddRow( int nRow, off_t nSize, const char* pzCol, ... );

	ListView *m_pcHDView;

	off_t off_tHDSize[NUM_OF_HD_ROWS];	/* More than 10 mounts == problems */
};

#endif // __F_DRIVESPANEL_H_


