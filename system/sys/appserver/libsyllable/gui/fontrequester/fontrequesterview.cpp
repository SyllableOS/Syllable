#include "fontrequesterview.h"

FontRequesterView::FontRequesterView(FontHandle* pcHandle, bool bEnableAdvanced) : LayoutView(Rect(0,0,1,1),"font_layout",NULL,CF_FOLLOW_ALL)
{
	pcFontHandle = pcHandle;
	m_bShowAdvanced = bEnableAdvanced;

	LayoutSystem();
	SetupFonts();

	pcFontListView->SetSelChangeMsg(new Message(M_FONT_TYPE_CHANGED));
	pcFontFaceListView->SetSelChangeMsg(new Message(M_FONT_FACE_CHANGED));
	pcFontSizeListView->SetSelChangeMsg(new Message(M_FONT_SIZE_CHANGED));					


	InitialFont();
}

void FontRequesterView::InitialFont()
{
	font_properties sProps;
	sProps = GetFontProperties(pcFontHandle);
	
	String cFamily = sProps.m_cFamily;
	String cStyle = sProps.m_cStyle;
	float vSize = sProps.m_vSize;
	
	if ( sProps.m_nFlags  & FPF_SMOOTHED)
		pcAntiCheck->SetValue(true);
	
	for (int i=0; i<pcFontListView->GetRowCount(); i++)
	{
		ListViewStringRow* pcRow = (ListViewStringRow*)pcFontListView->GetRow(i);
		if (cFamily == pcRow->GetString(0))
		{
			pcFontListView->Select(i,true);
			LoadFontTypes();
			for (int j=0; j<pcFontFaceListView->GetRowCount(); j++)
			{
				ListViewStringRow* pcRowTwo = (ListViewStringRow*)pcFontFaceListView->GetRow(j);
				if (cStyle == pcRowTwo->GetString(0))
				{
					pcFontFaceListView->Select(j);
				}
			}
		}
	}
	
	for (int i=0; i<pcFontSizeListView->GetRowCount(); i++)
	{
		
		ListViewStringRow* pcRow = (ListViewStringRow*)pcFontSizeListView->GetRow(i);
		String cSize;
		cSize.Format("%.1f",ceil(vSize));
		if (cSize == pcRow->GetString(0))
		{
			pcFontSizeListView->Select(i);
		}
	}
	
	sProps.m_vSize = ceil(sProps.m_vSize);  //must run ceiling of it
	
	Font* pcFont = new Font(sProps);
	String cText;
	cText.Format("%s %s %.1fpt",pcFont->GetFamily().c_str(),pcFont->GetStyle().c_str(),pcFont->GetSize());
	pcTextStringViewOne->SetString(pcFont->GetFamily());
	pcTextStringViewTwo->SetString(cText);		
	
	pcTextStringViewOne->SetFont(pcFont);
	pcTextStringViewTwo->SetFont(pcFont);
	
	pcFont->Release();		
}

void FontRequesterView::LayoutSystem()
{
	VLayoutNode* pcRoot = new VLayoutNode("fontrequester_layout");
	pcRoot->SetBorders(Rect(5,5,5,5));
	
	pcFontListView = new ListView(Rect(),"font_types_list",ListView::F_RENDER_BORDER | ListView::F_NO_HEADER | ListView::F_NO_AUTO_SORT);
	pcFontListView->InsertColumn("Font",(int)GetBounds().Height());

	pcFontFaceListView = new ListView(Rect(),"font_face_list",ListView::F_RENDER_BORDER | ListView::F_NO_HEADER | ListView::F_NO_AUTO_SORT);
	pcFontFaceListView->InsertColumn("Face",(int)GetBounds().Height());

	pcFontSizeListView = new ListView(Rect(),"font_size_list",ListView::F_RENDER_BORDER | ListView::F_NO_HEADER | ListView::F_NO_AUTO_SORT);
	pcFontSizeListView->InsertColumn("Size",(int)GetBounds().Height());

	HLayoutNode* pcRotationNode = new HLayoutNode("rotation_node");
	pcRotationNode->AddChild(pcRotationString = new StringView(Rect(),"rotation_string","Rotate:"));
	pcRotationNode->AddChild(new HLayoutSpacer("",2,2));
	pcRotationNode->AddChild(pcRotationSpinner = new Spinner(Rect(),"",0,new Message(M_FONT_ROTATE_CHANGED)));
	pcRotationSpinner->SetMinValue(0);
	pcRotationSpinner->SetMaxValue(360);
	pcRotationSpinner->SetStep(1.0);
	pcRotationSpinner->SetValue(pcFontHandle->vRotation);
	if (!m_bShowAdvanced)
	{
		pcRotationSpinner->SetEnable(false);
	}
	

	HLayoutNode* pcShearNode = new HLayoutNode("shear_node");
	pcShearNode->AddChild(pcShearString = new StringView(Rect(),"shear_string","Shear:"));
	pcShearNode->AddChild(new HLayoutSpacer("",2,2));
	pcShearNode->AddChild(pcShearSpinner = new Spinner(Rect(),"",0,new Message(M_FONT_SHEAR_CHANGED)));
	pcShearSpinner->SetMinValue(0);
	pcShearSpinner->SetMaxValue(360);
	pcShearSpinner->SetStep(1.0);
	pcShearSpinner->SetValue(pcFontHandle->vShear);
	if (!m_bShowAdvanced)
	{
		pcShearSpinner->SetEnable(false);
	}

	VLayoutNode* pcOtherFontNode = new VLayoutNode("other_node");
	pcOtherFontNode->AddChild(pcAntiCheck = new CheckBox(Rect(),"anti_check","A_ntiAlias",new Message(M_FONT_ALIAS_CHANGED)));
	pcAntiCheck->SetValue(pcFontHandle->nFlags & FPF_SMOOTHED ? true : false);
	pcOtherFontNode->AddChild(new VLayoutSpacer("",30,30));
	pcOtherFontNode->AddChild(pcRotationNode);
	pcOtherFontNode->AddChild(new VLayoutSpacer("",10,10));
	pcOtherFontNode->AddChild(pcShearNode);
	pcOtherFontNode->LimitMaxSize(Point(pcOtherFontNode->GetPreferredSize(false).x+70,pcOtherFontNode->GetPreferredSize(false).y));
	
	HLayoutNode* pcFontNode = new HLayoutNode("");
	pcFontNode->AddChild(new HLayoutSpacer("",5,5));
	pcFontNode->AddChild(pcFontListView);
	pcFontNode->AddChild(new HLayoutSpacer("",2,2));
	pcFontNode->AddChild(pcFontFaceListView);
	pcFontNode->AddChild(new HLayoutSpacer("",2,2));
	pcFontNode->AddChild(pcFontSizeListView);
	pcFontNode->AddChild(new HLayoutSpacer("",5,5));
	pcFontNode->AddChild(pcOtherFontNode);
	

	pcTextFrameView = new FrameView(Rect(),"text_farme","");
	pcTextFrameView->SetBgColor(255,255,255);
	VLayoutNode* pcTextNode = new VLayoutNode("");
	pcTextNode->SetHAlignment(ALIGN_CENTER);
	
	pcTextNode->AddChild(pcTextStringViewOne = new StringView(Rect(),"","This Is A Test String",ALIGN_CENTER));
	pcTextNode->AddChild(new VLayoutSpacer("",1,1));
	pcTextNode->AddChild(pcTextStringViewTwo = new StringView(Rect(),"","This Is A Test String",ALIGN_CENTER));
	pcTextStringViewTwo->SetFgColor(200,200,200);

	pcTextNode->LimitMaxSize(pcTextNode->GetPreferredSize(true));
	pcTextFrameView->SetRoot(pcTextNode);
	
	pcRoot->AddChild(pcTextFrameView);
	pcRoot->AddChild(new VLayoutSpacer("",5,5));	
	pcRoot->AddChild(pcFontNode);

	
	SetRoot(pcRoot);

	SetFrame(Rect(0,0,GetPreferredSize(false).x, GetPreferredSize(false).y));	
}

void FontRequesterView::AllAttached()
{
	pcFontListView->SetTarget(this);
	pcFontFaceListView->SetTarget(this);
	pcFontSizeListView->SetTarget(this);
	
	pcRotationSpinner->SetTarget(this);
	pcShearSpinner->SetTarget(this);
	
	pcAntiCheck->SetTarget(this);
}

void FontRequesterView::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_FONT_ALIAS_CHANGED:
			AliasChanged();
			break;
			
		case M_FONT_ROTATE_CHANGED:
			RotateChanged();
			break;
			
		case M_FONT_SIZE_CHANGED:
			SizeHasChanged();
			break;
			
		case M_FONT_FACE_CHANGED:
			FaceHasChanged();
			break;
			
		case M_FONT_TYPE_CHANGED:
			LoadFontTypes();
			FontHasChanged();
			break;
			
		case M_FONT_SHEAR_CHANGED:
			ShearChanged();
			break;
			
		default:
			GetWindow()->HandleMessage(pcMessage);
			break;
			
	}
}

void FontRequesterView::SetupFonts()
{
	float sz,del=1.0;
	char zSizeLabel[8];

	for (sz=5.0; sz < 30.0; sz += del)
	{
		ListViewStringRow* pcRow = new ListViewStringRow();
		sprintf(zSizeLabel,"%.1f",sz);
		pcRow->AppendString(zSizeLabel);
		pcFontSizeListView->InsertRow(pcRow);
		
		if (sz == 10.0)
			del = 2.0;
		else if (sz == 16.0)
			del = 4.0;
   }
   
   	// Add Family
	int fc = Font::GetFamilyCount();
	int i=0;
	char zFFamily[FONT_FAMILY_LENGTH];

	for (i=0; i<fc; i++)
	{
		if (Font::GetFamilyInfo(i,zFFamily) == 0)
		{
			Font::GetStyleCount(zFFamily);
			ListViewStringRow* pcRow = new ListViewStringRow();
			pcRow->AppendString(zFFamily);
			pcFontListView->InsertRow(pcRow);			
		}
	}	
}

void FontRequesterView::LoadFontTypes()
{
	ListViewStringRow* pcFamilyRow = (ListViewStringRow*)pcFontListView->GetRow(pcFontListView->GetLastSelected());
	String cFamily = pcFamilyRow->GetString(0);
	char zFStyle[FONT_STYLE_LENGTH];
	int i; 
	int nStyleCount=0;
	uint32 nFlags;
	nStyleCount = Font::GetStyleCount(cFamily.c_str());
	
	pcFontFaceListView->Clear();	
	
	for (i=0; i<nStyleCount; i++)
	{
		if (Font::GetStyleInfo(cFamily.c_str(),i,zFStyle,&nFlags) == 0)
		{
			ListViewStringRow* pcRow = new ListViewStringRow();
			pcRow->AppendString(zFStyle);
			pcFontFaceListView->InsertRow(pcRow);
		}
	}
}

void FontRequesterView::FontHasChanged()
{
	pcFontFaceListView->SetSelChangeMsg(NULL);
	pcFontFaceListView->Select(0);
	
	
	ListViewStringRow* pcRowTwo = (ListViewStringRow*) pcFontListView->GetRow(pcFontListView->GetLastSelected());
	strcpy(pcFontHandle->zFamilyName,pcRowTwo->GetString(0).c_str());
	
	ListViewStringRow* pcRowThree = (ListViewStringRow*) pcFontFaceListView->GetRow(0);
	strcpy(pcFontHandle->zStyleName,pcRowThree->GetString(0).c_str());
	
	Font* pcFont = new Font(GetFontProperties(pcFontHandle));
//	pcFont->AddRef();

	String cText;
	cText.Format("%s %s %.1fpt",pcFont->GetFamily().c_str(),pcFont->GetStyle().c_str(),pcFont->GetSize());

	pcTextStringViewOne->SetString(pcFont->GetFamily());
	pcTextStringViewTwo->SetString(cText);		
	
	pcTextStringViewOne->SetFont(pcFont);
	pcTextStringViewTwo->SetFont(pcFont);

	pcFont->Release();
	
	pcFontFaceListView->SetSelChangeMsg(new Message(M_FONT_FACE_CHANGED));
}

void FontRequesterView::SizeHasChanged()
{
	ListViewStringRow* pcRow = (ListViewStringRow*)pcFontSizeListView->GetRow(pcFontSizeListView->GetLastSelected());
	pcFontHandle->vSize = atof(pcRow->GetString(0).c_str());
	Font* pcFont = new Font(GetFontProperties(pcFontHandle));	
//	pcFont->AddRef();

	String cText;
	cText.Format("%s %s %.1fpt",pcFont->GetFamily().c_str(),pcFont->GetStyle().c_str(),pcFont->GetSize());
	pcTextStringViewOne->SetString(pcFont->GetFamily());
	pcTextStringViewTwo->SetString(cText);		
	
	pcTextStringViewOne->SetFont(pcFont);
	pcTextStringViewTwo->SetFont(pcFont);

	pcFont->Release();	
}

void FontRequesterView::FaceHasChanged()
{
	ListViewStringRow* pcRow = (ListViewStringRow*)pcFontFaceListView->GetRow(pcFontFaceListView->GetLastSelected());
	strcpy(pcFontHandle->zStyleName,pcRow->GetString(0).c_str());
	
	Font* pcFont = new Font(GetFontProperties(pcFontHandle));
//	pcFont->AddRef();

	String cText;
	cText.Format("%s %s %.1fpt",pcFont->GetFamily().c_str(),pcFont->GetStyle().c_str(),pcFont->GetSize());
	pcTextStringViewOne->SetString(pcFont->GetFamily());
	pcTextStringViewTwo->SetString(cText);		
	
	pcTextStringViewOne->SetFont(pcFont);
	pcTextStringViewTwo->SetFont(pcFont);

	pcFont->Release();
		
}

void FontRequesterView::AliasChanged()
{
	if (pcAntiCheck->GetValue().AsBool() == true)
		pcFontHandle->nFlags = FPF_SYSTEM | FPF_SMOOTHED;
	else
		pcFontHandle->nFlags = FPF_SYSTEM;
	
	Font* pcFont = new Font(GetFontProperties(pcFontHandle));
//	pcFont->AddRef();


	String cText;
	cText.Format("%s %s %.1fpt",pcFont->GetFamily().c_str(),pcFont->GetStyle().c_str(),pcFont->GetSize());
	pcTextStringViewOne->SetString(pcFont->GetFamily());
	pcTextStringViewTwo->SetString(cText);		
	
	pcTextStringViewOne->SetFont(pcFont);
	pcTextStringViewTwo->SetFont(pcFont);

	pcFont->Release();
}

void FontRequesterView::ShearChanged()
{
	pcFontHandle->vShear = pcShearSpinner->GetValue().AsFloat();
	
	Font* pcFont = new Font(GetFontProperties(pcFontHandle));
//	pcFont->AddRef();

	String cText;
	cText.Format("%s %s %.1fpt",pcFont->GetFamily().c_str(),pcFont->GetStyle().c_str(),pcFont->GetSize());
	pcTextStringViewOne->SetString(pcFont->GetFamily());
	pcTextStringViewTwo->SetString(cText);		
	
	pcTextStringViewOne->SetFont(pcFont);
	pcTextStringViewTwo->SetFont(pcFont);

	pcFont->Release();		
}

void FontRequesterView::RotateChanged()
{
	pcFontHandle->vRotation = pcRotationSpinner->GetValue().AsFloat();
	
	Font* pcFont = new Font(GetFontProperties(pcFontHandle));
//	pcFont->AddRef();

	String cText;
	cText.Format("%s %s %.1fpt",pcFont->GetFamily().c_str(),pcFont->GetStyle().c_str(),pcFont->GetSize());
	pcTextStringViewOne->SetString(pcFont->GetFamily());
	pcTextStringViewTwo->SetString(cText);		
	
	pcTextStringViewOne->SetFont(pcFont);
	pcTextStringViewTwo->SetFont(pcFont);	
	
	pcFont->Release();
}

FontHandle* FontRequesterView::GetFontHandle()
{
	return pcFontHandle;
}

FontRequesterView::~FontRequesterView()
{
	delete( pcFontHandle );
}




