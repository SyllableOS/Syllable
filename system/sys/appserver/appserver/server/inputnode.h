/*
 *  The AtheOS application server
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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

#ifndef __F_APPSERVER_INPUTNODE_H__
#define __F_APPSERVER_INPUTNODE_H__

#include <atheos/mouse.h>
#include <gui/region.h>
#include <util/messagequeue.h>

namespace os {
  class Message;
};

class InputNode
{
public:
    enum {
	IN_UNKNOWN,
	IN_MOUSE,
	IN_KEYBOARD,
	IN_JOYSTICK
    };
  
    InputNode();
    virtual ~InputNode();

    static void   SetMousePos( const os::Point& cPos );
    static os::Point GetMousePos();
    static uint32 GetMouseButtons();
  
    virtual bool Start() = 0;
    virtual int  GetType() = 0;
protected:
    static void EnqueueEvent( os::Message* pcEvent );
private:
	static void _CalculateMouseMovement( os::Point& cDeltaMove );
    friend void InitInputSystem();
    static int32 EventLoopEntry( void* pData );
    static void EventLoop();
    static int32 Loop( void* pData );
  
    static os::MessageQueue s_cEventQueue;
    static atomic_t	      s_nMouseMoveEventCount;
    static os::Point	  s_cMousePos;
    static uint32	      s_nMouseButtons;
};

void InitInputSystem();

typedef bool in_init();
typedef void in_uninit();
typedef int in_get_node_count();
typedef InputNode* in_get_input_node( int nIndex );

#endif // __F_APPSERVER_INPUTNODE_H__
