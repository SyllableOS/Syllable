/***********************************************************
 *	Copyright (C) 1997, Be Inc.  All rights reserved.
 *
 *  FILE:	glutBlocker.cpp
 *
 *	DESCRIPTION:	helper class for GLUT event loop.
 *		if a window receives an event, wake up the event loop.
 ***********************************************************/

/***********************************************************
 *	Headers
 ***********************************************************/
#include "glutBlocker.h"
#include <atheos/semaphore.h>
/***********************************************************
 *	Global variable
 ***********************************************************/
GlutBlocker gBlock;

/***********************************************************
 *	Member functions
 ***********************************************************/
GlutBlocker::GlutBlocker() {
	gSem = create_semaphore("gSem",1,0);
	eSem = create_semaphore("eSem",0,0);
	events = false;
	sleeping = false;
}

GlutBlocker::~GlutBlocker() {
	delete_semaphore(eSem);
	delete_semaphore(gSem);
}

void GlutBlocker::WaitEvent() {
	lock_semaphore(gSem);
	if(!events) {			// wait for new event
		sleeping = true;
		unlock_semaphore(gSem);
		lock_semaphore(eSem);	// next event will release eSem
	} else {
		unlock_semaphore(gSem);
	}
}

void GlutBlocker::WaitEvent(bigtime_t usecs) {
	lock_semaphore(gSem);
	if(!events) {			// wait for new event
		sleeping = true;
		unlock_semaphore(gSem);
	//	lock_semaphore_x(eSem, 1, B_TIMEOUT, usecs);	// wait for next event or timeout
		lock_semaphore_x(eSem, 1, 0, usecs);	// wait for next event or timeout
	} else {
		unlock_semaphore(gSem);
	}
}

void GlutBlocker::NewEvent() {
	lock_semaphore(gSem);
	events = true;		// next call to WaitEvent returns immediately
	if(sleeping) {
		sleeping = false;
		unlock_semaphore(eSem);	// if event loop is blocking, wake it up
	}
	unlock_semaphore(gSem);
}
