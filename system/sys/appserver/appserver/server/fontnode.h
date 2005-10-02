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

#ifndef	__F_FONT_NODE_H__
#define	__F_FONT_NODE_H__

#include <atheos/types.h>

#include <util/resource.h>
#include <util/locker.h>

#include <list>
#include <string>

#include "sfont.h"

class  SFont;
class  SFontInstance;
class  FontProperty;
struct Glyph;

namespace os
{
	class Messenger;
	struct font_properties;
};


class FontNode : public Resource
{
public:
    typedef std::list<std::pair<os::Messenger*,void*> > DependencyList_t;
    FontNode();

    Glyph* GetGlyph( int nChar );
  
    SFontInstance*	GetInstance( void ) const { return( m_pcInstance ); }

    void		SnapPointSize( float* pvSize ) const;
    status_t		SetProperties( const os::font_properties& sProps );
    status_t		SetProperties( const FontProperty& cFP );
    status_t		SetProperties( int nSize, int nShear, int nRotation, uint32 nFlags );
    status_t		SetFamilyAndStyle( const std::string& cFamily, const std::string& cStyle );

    DependencyList_t::iterator AddDependency( os::Messenger* pcTarget, void* pView );
    void		       RemoveDependency( DependencyList_t::iterator& cIterator );
    void		       NotifyDependent() const;
protected:
    ~FontNode();
private:
    status_t _SetProperties( const FontProperty& cFP );

    DependencyList_t	m_cDependencies;
    os::Locker		m_cDependenciesMutex;
    SFont*		m_pcFont;
    SFontInstance*	m_pcInstance;
    FontProperty	m_cProperties;
};

#endif	// __F_FONT_NODE_H__
