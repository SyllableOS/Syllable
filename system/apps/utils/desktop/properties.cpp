#include "properties.h"
char pzCgFile[1024];
char junk2[1024];
Color32_s c_bgColor, c_fgColor;
int32 nImageSize = 0;


MiscView::MiscView(const Rect & cFrame) : View(cFrame, "MiscView")
{
    pcLoginCheck = new CheckBox(Rect(0,0,0,0),"login_check","Remember the last user that logged in",NULL);
    pcLoginCheck->SetFrame(Rect(0,0,220,15) + Point(20,10));
    AddChild(pcLoginCheck);

    pcShowVerCheck = new CheckBox(Rect(0,0,0,0),"ver_check","Show Version on the Desktop",NULL);
    pcShowVerCheck->SetFrame(Rect(0,0,220,15) + Point(20,45));
    AddChild(pcShowVerCheck);
}



BackView::BackView(const Rect & cFrame) : View(cFrame, "BackView", CF_FOLLOW_ALL)
{


    pcScreenBmp = LoadBitmapFromResource("screen.png");
    pcList = new ListView(Rect(0,0,100,100),"listView",ListView::F_RENDER_BORDER);
    pcList->SetFrame(Rect(0,0,200,150) + Point (10,165));
    pcList->InsertColumn("Pictures",pcList->GetBounds().Width(),0);
    AddChild(pcList);

    pcString = new StringView(Rect(0,0,0,0),"","        Position:");
    pcString->SetFrame(Rect(0,0,120,25) + Point(225,185));
    AddChild(pcString);

    pcSizeDrop = new DropdownMenu(Rect(0,0,0,0),"Size_Drop");
    pcSizeDrop->SetFrame(Rect(0,0,97,15) + Point(225,205));
    AddChild(pcSizeDrop);

    pcSizeDrop->InsertItem(0,"Stretch Picture");
    pcSizeDrop->InsertItem(1,"Tile Picture");

    Paint(GetBounds());
    Defaults();
    ListFiles();




}


void BackView::ListFiles()
{

    t_List t_ImageList = ImageList();
    for (uint32 n = 0; n < t_ImageList.size(); n++)
    {
        vFolderRow.push_back(new ListViewStringRow());
        vFolderRow[n]->AppendString(t_ImageList[n]);
        pcList->InsertRow(vFolderRow[n]);
    }

}

t_List BackView::ImageList()
{

    t_List t_list;
    string zName;
    Directory* pcDir = new Directory();

    if(pcDir->SetTo("~/config/desktop/pictures/")==0)
    {
        while (pcDir->GetNextEntry(&zName))
            if (!(zName.find( ".jpg",0)==string::npos)
                    || !(zName.find( ".png",0)==string::npos) || !(zName.find( ".jpeg",0)==string::npos)
                    || !(zName.find( ".gif",0)==string::npos))
            {
                t_list.push_back(zName);
            }

    }
    delete pcDir;
    return (t_list);
}



void BackView::Defaults()
{
    pcSizeDrop->SetSelection(0);
    pcSizeDrop->SetReadOnly();
}

void BackView::Paint(const Rect& cUpdate)
{

    SetFgColor( get_default_color( COL_NORMAL ) );
    FillRect( cUpdate );
    DrawBitmap(pcScreenBmp,pcScreenBmp->GetBounds(),Rect(GetBounds().Width()/2-pcScreenBmp->GetBounds().Width()/2,10,pcScreenBmp->GetBounds().Height(),pcScreenBmp->GetBounds().Width()));
    Flush ();
    Sync();
}


ColorView::ColorView(const Rect & cFrame) : View(cFrame, "ColorView",CF_FOLLOW_ALL)
{
    pcScreenBmp = LoadBitmapFromResource("screen.png");

    pcListTheme = new ListView(Rect(0,0,100,100),"listView",ListView::F_RENDER_BORDER);
    pcListTheme->SetFrame(Rect(0,0,200,150) + Point (10,165));
    pcListTheme->InsertColumn("Themes",pcListTheme->GetBounds().Width(),0);
    AddChild(pcListTheme);


    pcColorDrop = new DropdownMenu(Rect(0,0,0,0),"Color_Drop_");
    pcColorDrop->SetFrame(Rect(0,0,150,15) + Point(225,205));
    AddChild(pcColorDrop);

    pcColorEdit = new ColorEdit(Rect(0,0,0,0),"Col",c_bgColor);
    pcColorEdit->SetFrame(Rect(0,0,150,90) + Point(225,225));
    AddChild(pcColorEdit);



    pcColorDrop->SetReadOnly();
    pcColorDrop->AppendItem("Icon Background Color");
    pcColorDrop->AppendItem("Icon Text Color");

    Paint(GetBounds());

    pcColorDrop->SetSelection(0);


}


void ColorView::Paint(const Rect& cUpdate)
{

    SetFgColor( get_default_color( COL_NORMAL ) );
    FillRect( cUpdate );
    DrawBitmap(pcScreenBmp,pcScreenBmp->GetBounds(),Rect(GetBounds().Width()/2-pcScreenBmp->GetBounds().Width()/2,10,pcScreenBmp->GetBounds().Height(),pcScreenBmp->GetBounds().Width()));
    Flush ();
    Sync();
}


PropTab::PropTab(const Rect & cFrame) : TabView(cFrame, "MiscView",CF_FOLLOW_ALL)
{
    Rect cframe = GetBounds();

    pcColor = new ColorView(cframe);
    pcMisc = new MiscView(cframe);
    pcBack = new BackView(cframe);

    InsertTab(0,"Set Background Image",pcBack);
    InsertTab(1,"Appearance",pcColor);
    InsertTab(2,"Other Options",pcMisc);


}


PropWin::PropWin() : Window(CRect(400,395), "Desktop Properties", "Desktop Properties", WND_NOT_RESIZABLE)
{
    sprintf(pzCgFile,"%s/config/desktop/config/desktop.cfg", getenv("HOME"));

    LoadPrefs();

    Rect cframe = GetBounds();
    cframe.Resize(5,5,-5,-45);
    pcPropTab = new PropTab(cframe);


    pcCancel = new Button(Rect(0,0,0,0),"cancel_but","Cancel",new Message(ID_PROP_CANCEL));
    pcSave = new Button(Rect(0,0,0,0),"save","Apply",new Message(ID_PROP_APPLY));
    pcClose = new Button(Rect(0,0,0,0),"close","OK",new Message(ID_PROP_CLOSE));

    pcSave->SetFrame(Rect(0,0,45,25) + Point (10,365));
    pcClose->SetFrame(Rect(0,0,45,25) + Point (65,365));
    pcCancel->SetFrame(Rect(0,0,45,25) + Point (120,365));



    AddChild(pcPropTab);
    AddChild(pcCancel);
    AddChild(pcSave);
    AddChild(pcClose);

    Defaults();

    pcPropTab->pcColor->pcColorDrop->SetTarget(this);
    pcPropTab->pcColor->pcColorDrop->SetSelectionMessage( new Message(ID_PEN_CHANGED));
    pcPropTab->pcBack->pcSizeDrop->SetSelection(nImageSize);
    pcPropTab->pcBack->pcList->SetSelChangeMsg(new Message(ID_PROP_LIST));

}




bool PropWin::OkToQuit(void)
{
    PostMessage(M_QUIT);
    return true;
}

void PropWin::HandleMessage(Message* pcMessage)
{
    switch (pcMessage->GetCode())
    {

    case ID_PEN_CHANGED:
        switch(pcPropTab->pcColor->pcColorDrop->GetSelection())
        {
        case 0:
            pcPropTab->pcColor->pcColorEdit->SetValue(c_bgColor);
            break;
        case 1:
            pcPropTab->pcColor->pcColorEdit->SetValue(c_fgColor);
            break;
        }

        break;

    case ID_PROP_CANCEL:

        OkToQuit();

        break;

    case ID_PROP_APPLY:

        Save();

        break;

    case ID_PROP_CLOSE:
        Save();
        OkToQuit();

        break;

    case ID_PROP_LIST:
        {
            string zPath = (string) getenv("HOME") + (string) "/config/desktop/pictures/" + pcPropTab->pcBack->ImageList()[pcPropTab->pcBack->pcList->GetLastSelected()];
            Bitmap* pcBitmap = LoadBitmapFromFile(zPath.c_str());
            Bitmap* pcSBitmap = new Bitmap (154, 114, CS_RGB32,Bitmap::SHARE_FRAMEBUFFER);

            Scale(pcBitmap,pcSBitmap, filter_mitchell, 0);

            pcPropTab->pcBack->Paint(pcPropTab->pcBack->GetBounds());
            pcPropTab->pcBack->DrawBitmap(pcSBitmap,pcSBitmap->GetBounds(),Rect(118,25,153,113));
        }
        break;

    default:
        HandleMessage(pcMessage);
        break;

    }
}



void PropWin::Defaults()
{
    int nImageList = -1;
    t_List t_list = pcPropTab->pcBack->ImageList();

    ReadLoginConfig();

    SetDefaultButton(pcSave);

    pcPropTab->pcMisc->pcShowVerCheck->SetValue(bShwVr);

    do
    {
        nImageList = nImageList + 1;
    }
    while ( strcmp(t_list[nImageList].c_str(),zImage.c_str())!=0);

    pcPropTab->pcBack->pcList->Select(nImageList,true,true);

}


void PropWin::ReadLoginConfig()
{

    char login_info[1024];

    ifstream filRead;
    filRead.open("/boot/atheos/sys/config/login.cfg");

    filRead.getline(junk2,1024);
    filRead.getline((char*)login_info,1024);
    filRead.getline(junk2,1024);
    filRead.getline(junk2,1024);
    filRead.getline(zLoginName,1024);

    filRead.close();

    if (strcmp(login_info,"true") == 0)
    {
        pcPropTab->pcMisc->pcLoginCheck->SetValue(true);
    }

    else
    {
        pcPropTab->pcMisc->pcLoginCheck->SetValue(false);
    }

}

void PropWin::SaveLoginConfig(bool b_Login, const char* zName)
{
    FILE* fin;
    fin = fopen("/tmp/login.cfg", "w");

    fprintf(fin,"<Login Name Option>\n");
    if (b_Login == true)
    {fprintf(fin,"true\n\n");}
    else
    {fprintf(fin,"false\n\n");}
    fprintf(fin, "<Login Name>\n");
    fprintf(fin,zName);
    fclose(fin);

    system("mv -f /tmp/login.cfg /boot/atheos/sys/config");
}



void PropWin::Save()
{
    bool bShowVer = pcPropTab->pcMisc->pcShowVerCheck->GetValue();
    bool bLogin = pcPropTab->pcMisc->pcLoginCheck->GetValue();
    string zPicSave;
    int32 nNewImageSize = pcPropTab->pcBack->pcSizeDrop->GetSelection();

    if (pcPropTab->pcBack->pcList->GetLastSelected() == -1)
        zPicSave = zImage;

    else
        zPicSave = pcPropTab->pcBack->ImageList()[pcPropTab->pcBack->pcList->GetLastSelected()];

    SavePrefs(bShowVer,bLogin,zPicSave, nNewImageSize);
    SaveLoginConfig(bLogin, zLoginName);
}


void PropWin::SavePrefs(bool bShow, bool bLogin, string zPic, int32 nNewImageSize )
{
    switch(pcPropTab->pcColor->pcColorDrop->GetSelection())
    {
    case 0:
        c_bgColor = pcPropTab->pcColor->pcColorEdit->GetValue();
        break;
    case 1:
        c_fgColor = pcPropTab->pcColor->pcColorEdit->GetValue();
        break;
    }

    Message *pcPrefs = new Message( );
    pcPrefs->AddColor32( "BackGround", c_bgColor );
    pcPrefs->AddColor32( "IconText",   c_fgColor );
    pcPrefs->AddString ( "DeskImage",  zPic  );
    pcPrefs->AddBool   ( "ShowVer",    bShow   );
    pcPrefs->AddInt32  ( "SizeImage",  nNewImageSize);


    File *pcConfig = new File("/tmp/desktop.cfg", O_WRONLY | O_CREAT );
    if( pcConfig )
    {
        uint32 nSize = pcPrefs->GetFlattenedSize( );
        void *pBuffer = malloc( nSize );
        pcPrefs->Flatten( (uint8 *)pBuffer, nSize );

        pcConfig->Write( pBuffer, nSize );

        delete pcConfig;
        free( pBuffer );
    }

    system("mv -f /tmp/desktop.cfg ~/config/desktop/config/");

}

void PropWin::LoadPrefs(void)
{
    FSNode *pcNode = new FSNode();
    if( pcNode->SetTo(pzCgFile ) == 0 )
    {
        File *pcConfig = new File( *pcNode );
        uint32 nSize = pcConfig->GetSize( );
        void *pBuffer = malloc( nSize );
        pcConfig->Read( pBuffer, nSize );
        Message *pcPrefs = new Message( );
        pcPrefs->Unflatten( (uint8 *)pBuffer );

        pcPrefs->FindColor32( "BackGround", &c_bgColor );
        pcPrefs->FindColor32( "IconText",   &c_fgColor );
        pcPrefs->FindString ( "DeskImage",  &zImage  );
        pcPrefs->FindBool   ( "ShowVer",    &bShwVr   );
        pcPrefs->FindInt32   ("SizeImage",   &nImageSize);
    }

}



