/****************************************************************************
** $Id$
**
** Various macros etc. to ease porting from Qt 1.x to 2.0.  THIS FILE
** WILL CHANGE OR DISAPPEAR IN THE NEXT VERSION OF Qt.
**
** Created : 980824
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#ifndef Q1XCOMPATIBILITY_H
#define Q1XCOMPATIBILITY_H


#define Event_None                QEvent::None
#define Event_Timer               QEvent::Timer
#define Event_MouseButtonPress    QEvent::MouseButtonPress
#define Event_MouseButtonRelease  QEvent::MouseButtonRelease
#define Event_MouseButtonDblClick QEvent::MouseButtonDblClick
#define Event_MouseMove           QEvent::MouseMove
#define Event_KeyPress            QEvent::KeyPress
#define Event_KeyRelease          QEvent::KeyRelease
#define Event_FocusIn             QEvent::FocusIn
#define Event_FocusOut            QEvent::FocusOut
#define Event_Enter               QEvent::Enter
#define Event_Leave               QEvent::Leave
#define Event_Paint               QEvent::Paint
#define Event_Move                QEvent::Move
#define Event_Resize              QEvent::Resize
#define Event_Create              QEvent::Create
#define Event_Destroy             QEvent::Destroy
#define Event_Show                QEvent::Show
#define Event_Hide                QEvent::Hide
#define Event_Close               QEvent::Close
#define Event_Quit                QEvent::Quit
#define Event_Accel               QEvent::Accel
#define Event_Clipboard           QEvent::Clipboard
#define Event_SockAct             QEvent::SockAct
#define Event_DragEnter           QEvent::DragEnter
#define Event_DragMove            QEvent::DragMove
#define Event_DragLeave           QEvent::DragLeave
#define Event_Drop                QEvent::Drop
#define Event_DragResponse        QEvent::DragResponse
#define Event_ChildInserted       QEvent::ChildInserted
#define Event_ChildRemoved        QEvent::ChildRemoved
#define Event_LayoutHint          QEvent::LayoutHint
#define Event_ActivateControl     QEvent::ActivateControl
#define Event_DeactivateControl   QEvent::DeactivateControl
#define Event_User                QEvent::User

#define Q_TIMER_EVENT(x)        ((QTimerEvent*)x)
#define Q_MOUSE_EVENT(x)        ((QMouseEvent*)x)
#define Q_KEY_EVENT(x)          ((QKeyEvent*)x)
#define Q_FOCUS_EVENT(x)        ((QFocusEvent*)x)
#define Q_PAINT_EVENT(x)        ((QPaintEvent*)x)
#define Q_MOVE_EVENT(x)         ((QMoveEvent*)x)
#define Q_RESIZE_EVENT(x)       ((QResizeEvent*)x)
#define Q_CLOSE_EVENT(x)        ((QCloseEvent*)x)
#define Q_SHOW_EVENT(x)         ((QShowEvent*)x)
#define Q_HIDE_EVENT(x)         ((QHideEvent*)x)
#define Q_CUSTOM_EVENT(x)       ((QCustomEvent*)x)


#define NoButton	Qt::NoButton
#define LeftButton	Qt::LeftButton
#define RightButton	Qt::RightButton
#define MidButton	Qt::MidButton
#define MouseButtonMask	Qt::MouseButtonMask
#define ShiftButton	Qt::ShiftButton
#define ControlButton	Qt::ControlButton
#define AltButton	Qt::AltButton
#define KeyButtonMask	Qt::KeyButtonMask

// Painter device types (is-A)

#define PDT_UNDEF    	QInternal::UndefinedDevice
#define PDT_WIDGET	QInternal::Widget
#define PDT_PIXMAP	QInternal::Pixmap
#define PDT_PRINTER    	QInternal::Printer
#define PDT_PICTURE	QInternal::Picture
#define PDT_SYSTEM	QInternal::System
#define PDT_MASK	QInternal::DeviceTypeMask

// Painter device flags

#define PDF_EXTDEV	QInternal::ExternalDevice

// old qpaindevicedefs.h stuff

#define PDC_BEGIN QPaintDevice::PdcBeg
#define PDC_DRAWARC QPaintDevice::PdcDrawArc
#define PDC_DRAWCHORD QPaintDevice::PdcDrawChord
#define PDC_DRAWELLIPSE QPaintDevice::PdcDrawEllipse
#define PDC_DRAWIMAGE QPaintDevice::PdcDrawImage
#define PDC_DRAWLINE QPaintDevice::PdcDrawLine
#define PDC_DRAWLINESEGS QPaintDevice::PdcDrawLineSegments/g
#define PDC_DRAWPIE QPaintDevice::PdcDrawPie
#define PDC_DRAWPIXMAP QPaintDevice::PdcDrawPixmap
#define PDC_DRAWPOINT QPaintDevice::PdcDrawPoint
#define PDC_DRAWPOLYGON QPaintDevice::PdcDrawPolygon
#define PDC_DRAWPOLYLINE QPaintDevice::PdcDrawPolyline
#define PDC_DRAWQUADBEZIER QPaintDevice::PdcDrawQuadBezier
#define PDC_DRAWRECT QPaintDevice::PdcDrawRect
#define PDC_DRAWROUNDRECT QPaintDevice::PdcDrawRoundRect
#define PDC_DRAWTEXT QPaintDevice::PdcDrawText
#define PDC_DRAWTEXT2 QPaintDevice::PdcDrawText2
#define PDC_DRAWTEXT2FRMT QPaintDevice::PdcDrawText2Formatted
#define PDC_DRAWTEXTFRMT QPaintDevice::PdcDrawTextFormatted
#define PDC_DRAW_FIRST QPaintDevice::PdcDrawFirst
#define PDC_DRAW_LAST QPaintDevice::PdcDrawLast
#define PDC_END QPaintDevice::PdcEnd
#define PDC_LINETO QPaintDevice::PdcLineTo
#define PDC_MOVETO QPaintDevice::PdcMoveTo
#define PDC_NOP QPaintDevice::PdcNOP
#define PDC_RESERVED_START QPaintDevice::PdcReservedStart
#define PDC_RESERVED_STOP QPaintDevice::PdcReservedStop
#define PDC_RESTORE QPaintDevice::PdcRestore
#define PDC_RESTOREWMATRIX QPaintDevice::PdcRestoreWMatrix
#define PDC_SAVE QPaintDevice::PdcSave
#define PDC_SAVEWMATRIX QPaintDevice::PdcSaveWMatrix
#define PDC_SETBKCOLOR QPaintDevice::PdcSetBkColor
#define PDC_SETBKMODE QPaintDevice::PdcSetBkMode
#define PDC_SETBRUSH QPaintDevice::PdcSetBrush
#define PDC_SETBRUSHORIGIN QPaintDevice::PdcSetBrushOrigin
#define PDC_SETCLIP QPaintDevice::PdcSetClip
#define PDC_SETCLIPRGN QPaintDevice::PdcSetClipRegion
#define PDC_SETDEV QPaintDevice::PdcSetdev
#define PDC_SETFONT QPaintDevice::PdcSetFont
#define PDC_SETPEN QPaintDevice::PdcSetPen
#define PDC_SETROP QPaintDevice::PdcSetROP
#define PDC_SETTABARRAY QPaintDevice::PdcSetTabArray
#define PDC_SETTABSTOPS QPaintDevice::PdcSetTabStops
#define PDC_SETUNIT QPaintDevice::PdcSetUnit
#define PDC_SETVIEWPORT QPaintDevice::PdcSetViewport
#define PDC_SETVXFORM QPaintDevice::PdcSetVXform
#define PDC_SETWINDOW QPaintDevice::PdcSetWindow
#define PDC_SETWMATRIX QPaintDevice::PdcSetWMatrix
#define PDC_SETWXFORM QPaintDevice::PdcSetWXform

#define PDM_WIDTH QPaintDeviceMetrics::PdmWidth
#define PDM_HEIGHT QPaintDeviceMetrics::PdmHeight
#define PDM_WIDTHMM QPaintDeviceMetrics::PdmWidthMM
#define PDM_HEIGHTMM QPaintDeviceMetrics::PdmHeightMM
#define PDM_NUMCOLORS QPaintDeviceMetrics::PdmNumColors
#define PDM_DEPTH QPaintDeviceMetrics::PdmDepth


#endif // Q1XCOMPATIBILITY_H
