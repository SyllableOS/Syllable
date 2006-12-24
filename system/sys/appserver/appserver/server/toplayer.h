/*
 *  The Syllable application server
 *	Copyright (C) 2005 - Syllable Team
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

#ifndef	__F_TOPLAYER_H__
#define	__F_TOPLAYER_H__

#include "layer.h"

class TopLayer : public Layer
{
public:
	TopLayer( SrvBitmap* pcBitmap );
	
	// Public methods
	void LayerFrameChanged( Layer* pcChild, os::IRect cFrame );
	void RedrawLayer( Layer* pcBackbufferedLayer, Layer* pcChild, bool bRedrawChildren );
	void FreeBackbuffers( void );
	
	// Implementation of the layer methods
	void SetFrame( const os::Rect& cRect );
	void RebuildRegion( bool bForce );
	void MoveChilds( void );
	void InvalidateNewAreas( void );
	void UpdateIfNeeded();
	
	void AddChild( Layer* pcChild, bool bTopmost );
    void RemoveChild( Layer* pcChild );
    
    void UpdateRegions( bool bForce = true );
};

#endif





