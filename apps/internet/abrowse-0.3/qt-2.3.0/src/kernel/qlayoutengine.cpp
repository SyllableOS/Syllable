/****************************************************************************
** $Id$
**
** Implementation of QLayout functionality
**
** Created : 981231
**
** Copyright (C) 1998-2000 Trolltech AS.  All rights reserved.
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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It may change from version to
// version without notice, or even be removed.
//
//


#include "qlayoutengine_p.h"

#ifndef QT_NO_LAYOUT

static inline int toFixed( int i ) { return i * 256; }
static inline int fRound( int i ) {
    return  i % 256 < 128 ? i / 256 : 1 + i / 256;
}
/*
  \internal
  This is the main workhorse of the QGridLayout. It portions out
  available space to the chain's children.

  The calculation is done in fixed point: "fixed" variables are scaled
  by a factor of 256.

  If the layout runs "backwards" (i.e. RightToLeft or Up) the layout
  is computed mirror-reversed, and it is the callers responsibility do
  reverse the values before use.

  chain contains input and output parameters describing the geometry.
  count is the count of items in the chain,
  pos and space give the interval (relative to parentWidget topLeft.)
*/

void qGeomCalc( QArray<QLayoutStruct> &chain, int start, int count, int pos,
		      int space, int spacer )
{
    typedef int fixed;
    int cHint = 0;
    int cMin = 0;
    int cMax = 0;
    int sumStretch = 0;
    int spacerCount = 0;

    bool wannaGrow = FALSE; // anyone who really wants to grow?
    //    bool canShrink = FALSE; // anyone who could be persuaded to shrink?

    int i; //some hateful compilers do not handle for loops correctly
    for ( i = start; i < start+count; i++ ) {
	chain[i].done = FALSE;
	cHint += chain[i].sizeHint;
	cMin += chain[i].minimumSize;
	cMax += chain[i].maximumSize;
	sumStretch += chain[i].stretch;
	if ( !chain[i].empty )
	    spacerCount++;
	wannaGrow = wannaGrow ||  chain[i].expansive;
    }

    int extraspace = 0;
    if ( spacerCount )
	spacerCount -= 1; //only spacers between things
    if ( space < cMin + spacerCount*spacer ) {
	//	qDebug("not enough space");
	for ( i = start; i < start+count; i++ ) {
	    chain[i].size = chain[i].minimumSize;
	    chain[i].done = TRUE;
	}
    } else if ( space < cHint + spacerCount*spacer ) {
	int n = count;
	int space_left = space - spacerCount*spacer;
	int overdraft = cHint - space_left;
	//first give to the fixed ones:
	for ( i = start; i < start+count; i++ ) {
	    if ( !chain[i].done && chain[i].minimumSize >= chain[i].sizeHint) {
		chain[i].size = chain[i].sizeHint;
		chain[i].done = TRUE;
		space_left -= chain[i].sizeHint;
		// sumStretch -= chain[i].stretch; NOT USED
		n--;
	    }
	}
	bool finished = n == 0;
	while ( !finished ) {
	    finished = TRUE;
	    fixed fp_over = toFixed( overdraft );
	    fixed fp_w = 0;

	    for ( i = start; i < start+count; i++ ) {
		if ( chain[i].done )
		    continue;
		// if ( sumStretch <= 0 )
		fp_w += fp_over / n;
		// else
		//    fp_w += (fp_space * chain[i].stretch) / sumStretch;
		int w = fRound( fp_w );
		chain[i].size = chain[i].sizeHint - w;
		fp_w -= toFixed( w ); //give the difference to the next
		if ( chain[i].size < chain[i].minimumSize ) {
		    chain[i].done = TRUE;
		    chain[i].size = chain[i].minimumSize;
		    finished = FALSE;
		    overdraft -= chain[i].sizeHint - chain[i].minimumSize;
		    // sumStretch -= chain[i].stretch; NOT USED
		    n--;
		    break;
		}
	    }
	}
    } else { //extra space
	int n = count;
	int space_left = space - spacerCount*spacer;
	//first give to the fixed ones, and handle non-expansiveness
	for ( i = start; i < start+count; i++ ) {
	    if ( !chain[i].done && ( chain[i].maximumSize <= chain[i].sizeHint
				     || wannaGrow && !chain[i].expansive )) {
		chain[i].size = chain[i].sizeHint;
		chain[i].done = TRUE;
		space_left -= chain[i].sizeHint;
		sumStretch -= chain[i].stretch;
		n--;
	    }
	}
	extraspace =  space_left;
	/*
	  Do a trial distribution and calculate how much it is off.
	  If there are more deficit pixels than surplus pixels,
	  give the minimum size items what they need, and repeat.
	  Otherwise give to the maximum size items, and repeat.
	
	  I have a wonderful mathematical proof for the correctness of
	  this principle, but unfortunately this comment is too
	  small to contain it.
	*/
	
	int surplus, deficit;
	do {
	    surplus = deficit = 0;
	    fixed fp_space = toFixed( space_left );
	    fixed fp_w = 0;
	    for ( i = start; i < start+count; i++ ) {
		if ( chain[i].done )
		    continue;
		extraspace = 0;
		if ( sumStretch <= 0 )
		    fp_w += fp_space / n;
		else
		    fp_w += (fp_space * chain[i].stretch) / sumStretch;
		int w = fRound( fp_w );
		chain[i].size = w;
		fp_w -= toFixed( w ); //give the difference to the next
		if ( w < chain[i].sizeHint ) {
		    deficit +=  chain[i].sizeHint - w;
		} else if ( w > chain[i].maximumSize ) {
		    surplus += w - chain[i].maximumSize;
		}
	    }
	    if ( deficit > 0 && surplus <= deficit ) {
		//give to the ones that have too little
		for ( i = start; i < start+count; i++ ) {
		    if ( !chain[i].done &&
			 chain[i].size < chain[i].sizeHint ) {
			chain[i].size = chain[i].sizeHint;
			chain[i].done = TRUE;
			space_left -= chain[i].sizeHint;
			sumStretch -= chain[i].stretch;
			n--;
		    }
		}
	    }
	    if ( surplus > 0 && surplus >= deficit ) {
		//take from the ones that have too much
		for ( i = start; i < start+count; i++ ) {
		    if ( !chain[i].done &&
			 chain[i].size > chain[i].maximumSize ) {
			chain[i].size = chain[i].maximumSize;
			chain[i].done = TRUE;
			space_left -= chain[i].maximumSize;
			sumStretch -= chain[i].stretch;
			n--;
		    }
		}
	    }
	} while ( n > 0 && surplus != deficit );
	if ( n == 0 )
	    extraspace = space_left;
    }

    //as a last resort, we distribute the unwanted space equally
    //among the spacers (counting the start and end of the chain).

    //### should do a sub-pixel allocation of extra space
    int extra = extraspace / ( spacerCount + 2 );
    int p = pos+extra;
    for ( i = start; i < start+count; i++ ) {
	chain[i].pos = p;
	p = p + chain[i].size;
	if ( !chain[i].empty )
	    p += spacer+extra;
    }
}

#endif //QT_NO_LAYOUT
