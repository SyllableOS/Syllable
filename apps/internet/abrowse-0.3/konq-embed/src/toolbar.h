#ifndef __F_TOOLBAR_H__
#define __F_TOOLBAR_H__

#include <util/message.h>
#include <util/messenger.h>
#include <gui/view.h>

#include <vector>

class ToolButton
{
public:
    ToolButton( const std::string& cImagePath, const os::Message& cMsg );
    ~ToolButton();

    
    
private:
    friend class ToolBar;
    
    os::Bitmap* m_pcImage;
    os::Message m_cMessage;
    bool	m_bIsEnabled;
};

class ToolBar : public os::View
{
public:
    ToolBar( const os::Rect& cFrame, const std::string& cName );

    virtual void	Paint( const os::Rect& cUpdateRect );
    virtual void	MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData );
    virtual void	MouseDown( const os::Point& cPosition, uint32 nButtons );
    virtual void	MouseUp( const os::Point& cPosition, uint32 nButtons, os::Message* pcData );
    virtual os::Point	GetPreferredSize( bool bLargest ) const;
    
    void SetTarget( const os::Messenger& cTarget );    
    void EnableButton( int nIndex, bool bEnabled );
    void AddButton( const std::string& cImagePath, const os::Message& cMsg );

private:
    os::Rect GetButtonRect( int nIndex, bool bFull );

    os::Messenger		m_cTarget;
    std::vector<ToolButton*>	m_cButtons;
    int				m_nSelectedButton;
};



#endif // __F_TOOLBAR_H__
