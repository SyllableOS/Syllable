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

#ifndef __F_CONFIG_H__
#define __F_CONFIG_H__

#include <stdio.h>
#include <string>
#include <map>

#include <atheos/types.h>
#include <gui/view.h>
#include <gui/font.h>
#include <util/message.h>

class AppserverConfig
{
public:
    AppserverConfig();
    ~AppserverConfig();
    static AppserverConfig* GetInstance();
    
    int	LoadConfig( FILE* hFile, bool bActivateConfig );
    int SaveConfig();
    bool	IsDirty() const;
    void	SetConfig( const os::Message* pcConfig );
    void	GetConfig( os::Message* pcConfig );

      // Keyboard configurations:
    int		SetKeymapPath( const std::string& cPath );
    std::string	GetKeymapPath() const;

    void	SetKeyDelay( bigtime_t nDelay );
    void	SetKeyRepeat( bigtime_t nDelay );
    
    bigtime_t	GetKeyDelay() const;
    bigtime_t	GetKeyRepeat() const;

      // Mouse configurations:
    
    void	SetDoubleClickTime( bigtime_t nDelay );
    bigtime_t	GetDoubleClickTime() const;

    void	SetMouseSpeed( float nSpeed );
    float	GetMouseSpeed() const;

    void	SetMouseAcceleration( float nAccel );
    float	GetMouseAcceleration() const;

	void    SetMouseSwapButtons( bool bSwap );
	bool    GetMouseSwapButtons() const;

    void	SetPopoupSelectedWindows( bool bPopup );
    bool	GetPopoupSelectedWindows() const;

      // Font configuration:

    void		       GetDefaultFontNames( os::Message* pcArchive );
    const os::font_properties* GetFontConfig( const std::string& cName );
    status_t		       SetFontConfig( const std::string& cName, const os::font_properties& cProps );
    status_t		       AddFontConfig( const std::string& cName, const os::font_properties& cProps );
    status_t		       DeleteFontConfig( const std::string& cName );
    
      // Apperance/screen mode:
    int		SetWindowDecoratorPath( const std::string& cPath );
    std::string	GetWindowDecoratorPath() const;

    void	SetDesktopScreenMode( int nDesktop, const os::screen_mode& sMode );
    void	SetDefaultColor( os::default_color_t eIndex, const os::Color32_s& sColor );

      // Misc configurations:

    
private:
    static AppserverConfig* s_pcInstance;
    bool		m_bDirty;
    std::string	m_cWindowDecoratorPath;
    std::string	m_cKeymapPath;
    bigtime_t	m_nKeyDelay;
    bigtime_t	m_nKeyRepeat;
    bigtime_t	m_nDoubleClickDelay;
	float		m_nMouseSpeed;
	float		m_nMouseAcceleration;
	bool		m_bMouseSwapButtons;
    bool		m_bPopupSelectedWindows;	// Move window to front on "single-click"
    std::map<std::string,os::font_properties> m_cFontSettings;
};


#endif // __F_CONFIG_H__
