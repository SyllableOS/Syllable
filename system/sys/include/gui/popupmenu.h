#ifndef __FILE_GUI_POPUPMENU_H__
#define __FILE_GUI_POPUPMENU_H__

#include <gui/menu.h>
#include <gui/view.h>
#include <util/string.h>
#include <gui/image.h>
#include <util/resources.h>
#include <storage/seekableio.h>

namespace os
{
#if 0
}
#endif

/** Popupmenu gui element...
 * \ingroup gui
 * \par Description:
 * PopupMenu class... 
 * \author	Rick Caudill (syllable_desktop@hotpop.com)
 *****************************************************************************/
class PopupMenu : public View
{
public:	
	PopupMenu(const Rect&,const String&,const String&, Menu *pcMenu=NULL, Image* pcImage=NULL, uint32 nResizeMask=CF_FOLLOW_LEFT|CF_FOLLOW_TOP,uint32 nFlags = WID_FULL_UPDATE_ON_RESIZE | WID_WILL_DRAW );
	~PopupMenu();

	void			SetIcon(Image*);
	Image* 			GetIcon();
	String			GetLabel();
	void			SetLabel(const String&);
    bool			IsEnabled();
    void			SetEnabled(bool,bool bValidate=false);
    void			SetDrawArrow( bool bArrow );
    
    //methods inherited from os::View		
	virtual void 	Paint(const Rect&);
	virtual void	MouseDown(const Point&, uint32);
	virtual void    MouseUp(const Point& cPosition, uint32 nButtons, Message * pcData );
	virtual void 	MouseMove( const Point & cPosition, int nCode, uint32 nButtons, Message * pcData );
	Point			GetPreferredSize( bool bLargest ) const;

private:
    virtual void	__pm_reserved1();
    virtual void	__pm_reserved2();
    virtual void	__pm_reserved3();
    virtual void	__pm_reserved4();
    virtual void 	__pm_reserved5();
    virtual void 	__pm_reserved6();
    virtual void 	__pm_reserved7();
    virtual void 	__pm_reserved8();
    virtual void 	__pm_reserved9();
    virtual void 	__pm_reserved10();

private:
    PopupMenu& operator=( const PopupMenu& );
    PopupMenu( const PopupMenu& );

	class Private;
	Private* m;
	
	void			_OpenMenu(Point cPos);
	void			_RefreshDisplay();
};

}
#endif	














