#ifndef _NEWTYPE_H_
#define _NEWTYPE_H_


class NewTypeWin : public os::Window
{
public:
	NewTypeWin( os::Looper* pcParent, os::Rect cFrame );
	~NewTypeWin();
	void HandleMessage( os::Message* pcMessage );
	
	
	 
private:
	os::Looper*			m_pcParent;
	os::LayoutView*		m_pcView;
	os::Button*			m_pcOk;
	os::TextView*		m_pcInput;	
};

#endif
