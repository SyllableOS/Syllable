#include <gui/inputbox.h>

using namespace os;

class InputBox::Private
{
public:
    Private(os::Messenger* pcTarget,const os::String& cLabel,const os::String& cText, const os::String& cOkBut, const os::String& cCancelBut,bool bNumeric)
    {
		if(m_pcTarget == NULL)
		{
			m_pcTarget = new os::Messenger(os::Application::GetInstance());
			m_bCreatedTarget = true;
		}
		else
		{
			m_pcTarget = pcTarget;
			m_bCreatedTarget = false;
		}

		m_cLabelText = cLabel;
		m_cInputText = cText;
		m_cOkButtonText = cOkBut;
		m_cCancelButtonText = cCancelBut;
		m_bNumeric = bNumeric;

		Layout();
    }

	~Private()
	{
		if (m_bCreatedTarget)
			delete m_pcTarget;
	}

	void SetNumeric(bool bNumeric)
	{
		m_bNumeric = bNumeric;
		m_pcInputTextBox->SetNumeric(m_bNumeric);
	}

	void SetOkButtonText(const os::String& cButtonText)
	{
		m_cOkButtonText = cButtonText;
		m_pcOkButton->LabelChanged(m_cOkButtonText);
	}

	void SetCancelButtonText(const os::String& cButtonText)
	{
		m_cCancelButtonText = cButtonText;
		m_pcCancelButton->LabelChanged(m_cCancelButtonText);
	}

    void SetLabelText(const os::String& cText)
	{
		m_cLabelText = cText;
		m_pcInputStringView->SetString(m_cLabelText);
	}

	void SetInputText(const os::String& cText)
	{
		m_cInputText = cText;
		m_pcInputTextBox->Set(m_cInputText.c_str());
	}


	bool GetNumeric() const
	{
		return (m_bNumeric);
	}

	os::String GetOkButtonText() const
	{
		return (m_cOkButtonText);
	}

	os::String GetCancelButtonText() const
	{
		return (m_cCancelButtonText);
	}

	os::String GetLabelText() const
	{
		return (m_cLabelText);
	}

	os::String GetInputText() const
	{
		return (m_pcInputTextBox->GetBuffer()[0]);
	}

	void Layout()
	{
		m_pcLayoutView = new os::LayoutView(os::Rect(0,0,1,1),"input_box_layout");
		
		os::VLayoutNode* pcRoot = new os::VLayoutNode("inputbox_root_node");

		os::HLayoutNode* pcButtonNode = new os::HLayoutNode("inputbox_button_node");
		pcButtonNode->SetVAlignment(os::ALIGN_RIGHT);
		pcButtonNode->SetHAlignment(os::ALIGN_RIGHT);
		pcButtonNode->AddChild(m_pcCancelButton = new os::Button(os::Rect(),"input_cancelbut",m_cCancelButtonText,new os::Message(M_BOX_REQUESTER_CANCELED)));
		pcButtonNode->AddChild(new HLayoutSpacer("button_spacer1",3,3));
		pcButtonNode->AddChild(m_pcOkButton = new os::Button(os::Rect(),"input_okbut",m_cOkButtonText,new os::Message(M_BOX_REQUESTED)));
		pcButtonNode->SameWidth("input_okbut","input_cancelbut",NULL);


		pcRoot->AddChild(new os::VLayoutSpacer("inputbox_spacer0",3,3));
		pcRoot->AddChild(m_pcInputStringView = new os::StringView(os::Rect(),"inputbox_stringview",m_cLabelText));
		pcRoot->AddChild(m_pcInputTextBox = new os::TextView(os::Rect(),"inputbox_textview",m_cInputText.c_str()));
		pcRoot->AddChild(new os::VLayoutSpacer("inputbox_spacer1",3,3));
		pcRoot->AddChild(pcButtonNode);
		pcRoot->AddChild(new os::VLayoutSpacer("inputbox_spacer2",5,5));

		m_pcLayoutView->SetRoot(pcRoot);

		m_pcInputTextBox->SetTabOrder(NEXT_TAB_ORDER);
		m_pcOkButton->SetTabOrder(NEXT_TAB_ORDER);
		m_pcCancelButton->SetTabOrder(NEXT_TAB_ORDER);
	}

    os::StringView* m_pcInputStringView;
    os::TextView* m_pcInputTextBox;
    os::LayoutView* m_pcLayoutView;
	os::Button* m_pcCancelButton;
	os::Button* m_pcOkButton;
    
    os::String m_cTitle;
    os::String m_cLabelText;
    os::String m_cOkButtonText;
    os::String m_cCancelButtonText;
	os::String m_cInputText;
    bool m_bNumeric, m_bCreatedTarget;
	os::Messenger* m_pcTarget;
};

/** InputBox constructor.
 * \par Description:
 * InputBox constructor. 
 * \param pcTarget 		- os::Messeneger pointer to the parent(used to send messages).
 * \param cTitle 		- The title of the window
 * \param cLabel 		- The label displaying a message to the user
 * \param cText  		- The default input text
 * \param cOkBut 		- The okay button text
 * \param cCancelBut 	- The cancel button text
 * \param bNumeric 		- Whether or not the dialog is for numbers only
 * \param nFlags 		- The flags for the window
 * \author	Rick Caudill (rick@syllable.org)
 *****************************************************************************/
InputBox::InputBox(os::Messenger* pcTarget, const os::String& cTitle,const os::String& cLabel, const os::String& cText,const os::String& cOkBut,const os::String& cCancelBut,bool bNumeric, uint32 nFlags) : Window(os::Rect(0,0,1,1),"input_box",cTitle,nFlags)
{
	CenterInScreen();

    m = new Private(pcTarget,cLabel,cText,cOkBut,cCancelBut,bNumeric);
    
	AddChild(m->m_pcLayoutView);
	ResizeTo(m->m_pcLayoutView->GetPreferredSize(false));

	SetFocusChild(m->m_pcInputTextBox);
	SetDefaultButton(m->m_pcOkButton);
}

InputBox::~InputBox()
{
	delete m;
}

/**
 * \par Description:
 * Returns the value of the InputBox(as a Variant). 
 *
 * Essentially all we have is a TextView, so you would think that we would want to
 * return this as an os::String, right?  No, and the reasoning behind returning it
 * as an os::Vairant is so you can do operations like: GetValue().AsFloat(), GetValue().AsString()
 * \author	Rick Caudill (rick@syllable.org)
 *****************************************************************************/
os::Variant InputBox::GetValue() const
{
    return (os::Variant(m->m_pcInputTextBox->GetBuffer()[0]));
}


/**
 * \par Description:
 * SetNumeric tells the os::InputBox whether or not it is for Numeric input only.
 * Setting this to true will mean that only numbers(0-9), one decimal point(.), and
 * one e(e) can be punched in.   
 * \param bNumeric 		- Whether or not the dialog is for numbers only
 * \author	Rick Caudill (rick@syllable.org)
 *****************************************************************************/
void InputBox::SetNumeric(bool bNumeric)
{
	m->SetNumeric(bNumeric);
}

/**
 * \par Description:
 * SetLabelText sets the text that is displayed in the InputBox's StringView.
 * \param cText 		- the new text to set the label to
 * \author	Rick Caudill (rick@syllable.org)
 *****************************************************************************/
void InputBox::SetLabelText(const os::String& cText)
{
	m->SetLabelText(cText);
}

/**
 * \par Description:
 * SetInputText sets the text that is displayed in the InputBox's TextView.
 * \param cText 		- the new text to set the TextView to
 * \author	Rick Caudill (rick@syllable.org)
 *****************************************************************************/
void InputBox::SetInputText(const os::String& cText)
{
	m->SetInputText(cText);
}


/**
 * \par Description:
 * SetOkButtonText sets the text that is displayed on the okay button.
 * \param cText 		- the new text to set the okay button to
 * \author	Rick Caudill (rick@syllable.org)
 *****************************************************************************/
void InputBox::SetOkButtonText(const os::String& cText)
{
	m->SetOkButtonText(cText);
}

/**
 * \par Description:
 * SetCancelButtonText sets the text that is displayed on the cancel button.
 * \param cText 		- the new text to set the cancel button to
 * \author	Rick Caudill (rick@syllable.org)
 *****************************************************************************/
void InputBox::SetCancelButtonText(const os::String& cText)
{
	m->SetCancelButtonText(cText);
}


/**
 * \par Description:
 * Returns whether or not the InputBox is numeric.
 * \author	Rick Caudill (rick@syllable.org)
 *****************************************************************************/
bool InputBox::GetNumeric() const
{
	return (m->GetNumeric());
}


/**
 * \par Description:
 * Returns the ok button text(i.e., the text that is shown on the Ok Button).
 * \author	Rick Caudill (rick@syllable.org)
 *****************************************************************************/
os::String InputBox::GetOkButtonText() const
{
	return (m->GetOkButtonText());
}


/**
 * \par Description:
 * Returns the cancel button text(i.e., the text that is shown on the CancelButton).
 * \author	Rick Caudill (rick@syllable.org)
 *****************************************************************************/
os::String InputBox::GetCancelButtonText() const
{
	return (m->GetCancelButtonText());
}

/**
 * \par Description:
 * Returns the label text(i.e., the text that is shown by the os::StringView).
 * \author	Rick Caudill (rick@syllable.org)
 *****************************************************************************/
os::String InputBox::GetLabelText() const
{
	return (m->GetLabelText());
}


/**
 * \par Description:
 * Returns the input text.  This can be used in lieu of GetValue() if you just want to
 * capture the text inputed by the user.
 * \author	Rick Caudill (rick@syllable.org)
 *****************************************************************************/
os::String InputBox::GetInputText() const
{
	return (m->GetInputText());
}


bool InputBox::OkToQuit()
{
	/*a simple test for null...  it should never be null, but you never know :)*/
	if (m->m_pcTarget != NULL)
	{
		Message *pcMsg = new Message( M_BOX_REQUESTER_CANCELED );
		pcMsg->AddPointer( "source", this );
		m->m_pcTarget->SendMessage( pcMsg );
		delete pcMsg;
	}
	else
		dbprintf("InputBox::HandleMessage: No place to send messages to.\n");

	Show(false);
	return (false);
}

void InputBox::HandleMessage(os::Message* pcMessage)
{
	switch(pcMessage->GetCode())
	{
		case M_BOX_REQUESTED:
		{
			/*a simple test for null...  it should never be null, but you never know :)*/
			if (m->m_pcTarget != NULL)
			{
				Message *pcMsg = new Message( M_BOX_REQUESTED );
				pcMsg->AddPointer( "source", this );
				pcMsg->AddVariant("value",GetValue());
				m->m_pcTarget->SendMessage( pcMsg );
				delete pcMsg;
			}
			else
				dbprintf("InputBox::HandleMessage: No place to send messages to.\n");

			Show(false);			
			break;
		}
		
		case M_BOX_REQUESTER_CANCELED:
		{
			/*a simple test for null...  it should never be null, but you never know :)*/
			if (m->m_pcTarget != NULL)
			{
				Message *pcMsg = new Message( M_BOX_REQUESTER_CANCELED );
				pcMsg->AddPointer( "source", this );
				m->m_pcTarget->SendMessage( pcMsg );
				delete pcMsg;
			}
			else
				dbprintf("InputBox::HandleMessage: No place to send messages to.\n");
			
			Show(false);
			break;
		}

		default:
			Window::HandleMessage(pcMessage);
			break;
	}
}



