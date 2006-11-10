/*
 *  Syllable Printer configuration preferences
 *
 *  (C)opyright 2006 - Kristian Van Der Vliet
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

#ifndef PRINTER_VIEW_H
#define PRINTER_VIEW_H

#include <cups/cups.h>
#include <cups/ppd.h>

#include <util/message.h>
#include <gui/rect.h>
#include <gui/view.h>
#include <gui/tabview.h>
#include <gui/stringview.h>
#include <gui/dropdownmenu.h>
#include <gui/textview.h>
#include <gui/button.h>
#include <gui/frameview.h>

#include <config.h>
#include <model_window.h>

typedef enum
{
	PROT_USB,
	PROT_PARALLEL,
	PROT_SMB,
	PROT_LPD,
	PROT_IPP,
	PROT_SOCKET,
	PROT_UNKNOWN,
} protocol_e;

enum pv_messages
{
	M_CHANGE,
	M_PROTOCOL_CHANGED
};

class PrintConfigView : public os::View
{
	public:
		PrintConfigView( const os::Rect &cFrame, CUPS_Printer *pcPrinter, os::Window *pcParent );
		~PrintConfigView();
		void AttachedToWindow( void );
		void AllAttached( void );
		void HandleMessage( os::Message *pcMessage );

		void SetDefault( bool bDefault );

		CUPS_Printer *  Save( void );

	private:
		void Layout( void );
		void GetProtocol( void );
		void DisplayDevice( void );
		void GenerateName( os::String cModel );

		CUPS_Printer *m_pcPrinter;
		ppd_file_t *m_hPPD;

		protocol_e m_nProtocol;

		os::FrameView *m_pcModelFrame;
		os::StringView *m_pcModel;
		os::Button *m_pcChange;

		ModelWindow *m_pcModelWindow;
		os::Window *m_pcParent;

		os::StringView *m_pcLabel1;	/* "The printer" */
		os::TextView *m_pcName;
		os::StringView *m_pcLabel2;	/* "is a" */

		os::DropdownMenu *m_pcProtocol;
		os::StringView *m_pcLabel3;	/* "printer, connected to" */

		os::View *m_pcDevice;	/* Not an instance in it's own right; points to one of the following three */
		os::DropdownMenu *m_pcUSBDevices;
		os::DropdownMenu *m_pcParallelDevices;
		os::TextView *m_pcRemoteDevice;
};

class PrintView : public os::View
{
	public:
		PrintView( const os::Rect &cFrame, CUPS_Printer *pcPrinter, os::Window *pcParent );
		~PrintView();

		void SetDefault( bool bDefault );

		CUPS_Printer *  Save( void );

	private:
		os::TabView *m_pcTabView;
		PrintConfigView *m_pcConfigView;
};

#endif	/* PRINTER_VIEW_H */
