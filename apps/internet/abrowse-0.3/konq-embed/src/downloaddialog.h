#ifndef __F_DOWNLOADDIALOG_H__
#define __F_DOWNLOADDIALOG_H__

#include <gui/window.h>
#include <gui/layoutview.h>

#include <util/message.h>
#include <util/messenger.h>

namespace os
{
    class Button;
    class StringView;
    class ProgressBar;
    class FileRequester;
}

class StatusView;

class DownloadRequesterView : public os::LayoutView
{
public:
    DownloadRequesterView( const os::Rect&      cFrame,
			   const std::string&   cURL,
			   const std::string&   cPreferredName,
			   const std::string&   cType,
			   off_t	        nContentSize,
			   const os::Messenger& cMsgTarget,
			   const os::Message&   cOkMsg,
			   const os::Message&   cCancelMessage);
    ~DownloadRequesterView();

    virtual void HandleMessage( os::Message* pcMessage );
    virtual void AllAttached();
    
private:
    static std::string s_cDownloadPath;

    os::Button*	m_pcOkBut;
    os::Button* m_pcCancelBut;

    os::Messenger m_cMsgTarget;
    os::Message	  m_cOkMsg;
    os::Message   m_cCancelMsg;
    os::Message*  m_pcTermMsg;
    
    std::string	       m_cPreferredName;
    os::FileRequester* m_pcFileReq;
};


class DownloadRequester : public os::Window
{
public:
    DownloadRequester( const os::Rect&      cFrame,
		       const std::string&   cTitle,
		       const std::string&   cURL,
		       const std::string&   cPreferredName,
		       const std::string&   cType,
		       off_t		    nContentSize,
		       const os::Messenger& cMsgTarget,
		       const os::Message&   cOkMsg,
		       const os::Message&   cCancelMessage );
    ~DownloadRequester();
private:
    DownloadRequesterView* m_pcTopView;
};


class DownloadProgressView : public os::LayoutView
{
public:
    DownloadProgressView( const os::Rect& cFrame,
			  const std::string& cURL,
			  const std::string& cPath,
			  off_t nTotalSize,
			  bigtime_t nStartTime,
			  const os::Messenger& cMsgTarget,
			  const os::Message& cCancelMessage );
    ~DownloadProgressView();
    virtual void HandleMessage( os::Message* pcMessage );
    virtual void AllAttached();
    
    void UpdateProgress( off_t nBytesReceived );

    void ClearTermMsg() { m_pcTermMsg = NULL; }
private:
    os::ProgressBar* m_pcProgressBar;
    StatusView*	     m_pcStatusView;
    os::Button*	     m_pcCancelBut;

    
    bigtime_t	  m_nStartTime;
    off_t 	  m_nTotalSize;
    
    os::Messenger m_cMsgTarget;
    os::Message   m_cCancelMsg;
    os::Message*  m_pcTermMsg;
};


class DownloadProgress : public os::Window
{
public:
    DownloadProgress( const os::Rect& cFrame,
		      const std::string& cTitle,
		      const std::string& cURL,
		      const std::string& cPath,
		      off_t nTotalSize,
		      bigtime_t nStartTime,
		      const os::Messenger& cMsgTarget,
		      const os::Message& cCancelMessage );
    ~DownloadProgress();

    void UpdateProgress( off_t nBytesReceived );
    void Terminate();
private:
    DownloadProgressView* m_pcTopView;
    std::string		  m_cTitle;
    off_t		  m_nTotalSize;
};



#endif // __F_DOWNLOADDIALOG_H__
