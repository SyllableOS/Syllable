
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

#ifndef __F_SYSINFOPANEL_H_
#define __F_SYSINFOPANEL_H_

#include <gui/layoutview.h>
#include <gui/textview.h>

#include <atheos/types.h>

#include <map>

using namespace os;

namespace os
{
	class TextView;
	class ListView;
	class ListViewStringRow;
}

#define APP_VERSION      "0.2.6"
#define APP_WINDOW_TITLE "Syllable Manager"

#define NEW_VALUE              0
#define OLD_VALUE              1

#define NUM_OF_MEMORY_ROWS     5
#define NUM_OF_ADDITIONAL_ROWS 8

/*******************************
 *  TIME RELATED #defines (duh)
 *******************************/
#define TIME_12_SETUP 1
#define TIME_24_SETUP 2

#define DATE_NO     0		/* No date shown */
#define DATE_USA    1		/* mm/dd/yyyy    */
#define DATE_GERMAN 2		/* dd.mm.yyyy    */
#define DATE_ENG    3		/* dd/mm/yyyy    */
#define DATE_JAPAN  4		/* yy/mm/dd      */



typedef struct
{

	uint64 nCoreSpeed;
	uint64 nBusSpeed;

} myCPUInfo;

class SysInfoPanel:public os::LayoutView
{
      public:

	SysInfoPanel( const os::Rect & cFrame );

//	virtual void Paint( const os::Rect & cUpdateRect );
//	virtual void KeyDown( const char *pzString, const char *pzRawString, uint32 nQualifiers );
	virtual void FrameSized( const os::Point & cDelta );
	virtual void AllAttached();
	virtual void HandleMessage( Message * pcMessage );

	void SetupPanel();
	void UpdateSysInfoPanel();
	//void SetDetail(bool bVal);

      private:
	void SetUpVersionView();
	void SetUpCPUView();
	void SetUpMemoryView();
	void SetUpAdditionView();

	void UpdateUptime( bool bUpdate );
	void UpdateAdditionalInfo( bool bUpdate );
	void UpdateMemoryInfo( bool bUpdate );

	ListViewStringRow *AddRow( char *pzCol1, char *pzCol2, char *pzCol3, int nRows );

	ListView *m_pcVersionView;
	ListView *m_pcCPUView;
	ListView *m_pcMemoryView;
	ListView *m_pcAdditionView;
	TextView *m_pcUptimeView;
	//bool          m_bDetail;

	StringView *m_pcUptime;
	StringView *m_pcDate;
	StringView *m_pcSpacer;

	int nTimeMode;
	int nDateMode;

	int m_nDay;

	system_info m_sSysInfo;
};

#endif // __F_SYSINFOPANEL_H_




