#ifndef __F_STATUSBAR_H__
#define __F_STATUSBAR_H__

#include <gui/view.h>
#include <gui/font.h>


class StatusBar : public os::View
{
public:
    StatusBar( const os::Rect& cFrame, const std::string& cTitle );

    virtual os::Point	GetPreferredSize( bool bLargest ) const;
    virtual void	FrameSized( const os::Point& cDelta );
    virtual void	Paint( const os::Rect& cUpdateRect );

    void SetStatus( const std::string& cText );
    
private:
    os::font_height m_sFontHeight;
    std::string m_cStatusStr;
};


#endif // __F_STATUSBAR_H__
