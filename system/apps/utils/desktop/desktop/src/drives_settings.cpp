#include "drives_settings.h"
#include <gui/requesters.h>
/*
** name:       add_disk
** purpose:    Adds disks to DiskInfo
** parameters: std::vector<DiskInfo>* pcList
**             std::string& cPath
** returns:  
*/
static void add_disk( std::vector<DiskInfo>* pcList, const std::string& cPath )
{
    DiskInfo cInfo;
    cInfo.m_nDiskFD = open( cPath.c_str(), O_RDONLY );
    if ( cInfo.m_nDiskFD < 0 )
    {
        return;
    }
    if ( ioctl( cInfo.m_nDiskFD, IOCTL_GET_DEVICE_GEOMETRY, &cInfo.m_sDriveInfo ) < 0 )
    {
        close( cInfo.m_nDiskFD );
        return;
    }
    cInfo.m_cPath = cPath;
    pcList->push_back( cInfo );
} /* end add_disk()*/


/*
** name:       find_drives
** purpose:    Finds all drives in cPath
** parameters: DiskInfo* pcList
**             std::string& cPath
** returns:  
*/
static void find_drives( std::vector<DiskInfo>* pcList, const std::string& cPath )
{
    DIR* hDir = opendir( cPath.c_str() );
    if ( hDir == NULL )
    {
        return;
    }
    struct dirent* psEntry;
    while( (psEntry = readdir(hDir)) != NULL )
    {
        if ( strcmp( psEntry->d_name, "." ) == 0 || strcmp( psEntry->d_name, ".." ) == 0 )
        {
            continue;
        }
        std::string cFullPath = cPath + psEntry->d_name;
        struct stat sStat;
        if ( stat( cFullPath.c_str(), &sStat ) < 0 )
        {
            continue;
        }
        if ( S_ISDIR( sStat.st_mode ) )
        {
            cFullPath += '/';
            find_drives( pcList, cFullPath );
        }
        else if ( !strcmp( psEntry->d_name, "raw" ) == 0 )
        {
            add_disk( pcList, cFullPath );
        }
    }
    closedir( hDir );
} /*end find_drives()*/

/*
** name:       DriveWindow(Window* pcWindow)
** purpose:    Main Constructor for DriveWindow
** parameters: Window* pcWindow
** returns:  
*/
DriveWindow::DriveWindow(Window* pcWindow, float fIcon):Window(CRect(400,300), "Drive_Settings", "Mount Drives at Startup", WND_NOT_RESIZABLE)
{
    fIconPoint = fIcon;
    pcList = new ListView(Rect(0,0,400,275),"layout_list_view",ListView::F_MULTI_SELECT,CF_FOLLOW_ALL);
    pcList->InsertColumn("Device",150,0);
    pcList->InsertColumn("Mount Point", 150,1);
    pcList->InsertColumn("Desktop Icon", 100,2);
    AddChild(pcList);

    pcOk = new Button(Rect(25,279,75,300), "Ok_but", "OK", new Message(D_DRIVE_OK));
    pcOk->SetTabOrder(0);
    AddChild(pcOk);

    pcCancel = new Button(Rect(85,279,135,300), "cancel_but", "Cancel", new Message(D_DRIVE_CANCEL));
    pcCancel->SetTabOrder(1);
    AddChild(pcCancel);

    pcAdd = new Button(Rect(145,279,195,300), "add_but", "Add", new Message(D_DRIVE_ADD));
    pcAdd->SetTabOrder(2);
    AddChild(pcAdd);

    pcRemove = new Button(Rect(205, 279, 255, 300),"remove_but","Remove",new Message(D_DRIVE_REMOVE));
    pcRemove->SetTabOrder(3);
    AddChild(pcRemove);

    pcRefresh = new Button(Rect(265,279,315,300), "refresh_but","Refresh",new Message(D_DRIVE_REFRESH));
    pcRefresh->SetTabOrder(4);
    AddChild(pcRefresh);

    SetFocusChild(pcOk);
    SetDefaultButton(pcOk);

    SetupDrives();

} /* end DriveWindow Constructor*/

/*
** name:       ~DriveWindow
** purpose:    DriveWindow Destructor
** parameters:
** returns:  
*/
DriveWindow::~DriveWindow()
{} /*end DriveWindow Destructor*/


/*
** name:       HandleMessage()
** purpose:    Handles the messages passed through the window
** parameters: Message* pcMessage
** returns:  
*/
void DriveWindow::HandleMessage(Message* pcMessage)
{
    switch (pcMessage->GetCode())
    {
    case D_DRIVE_OK:
        {
            SaveInit();
            SaveDesktop();
            Close();
            break;
        }
    case D_DRIVE_CANCEL:
        Close();
        break;

    case D_DRIVE_REMOVE:
        pcList->RemoveRow(pcList->GetLastSelected());
        break;

    case D_DRIVE_ADD:
        {
            pcAddWindow = new AddWindow(this);
            pcAddWindow->Show();
            pcAddWindow->MakeFocus();
        }
        break;

    case D_ADD_PASS:
        {
            pcAddWindow->Hide();
            const char *cString, *zString, *iString;
            pcMessage->FindString("pass_device",&cString);
            pcMessage->FindString("pass_mount", &zString);
            pcMessage->FindString("pass_icon",&iString);
            ListViewStringRow* pcRow = new ListViewStringRow();
            pcRow->AppendString(cString);
            pcRow->AppendString(zString);
            pcRow->AppendString(iString);
            pcList->InsertRow(pcRow);
            pcAddWindow->Close();
            break;
        }

    case D_DRIVE_REFRESH:
        pcList->Clear();
        SetupDrives();
        break;
    }
} /*end DriveWindow::HandleMessage()*/

/*
** name:       SetupDrives
** purpose:    Places all the information in ~/Settings/Desktop/config/drives.cfg into DriveWindow ListView
** parameters:
** returns:  
*/
void DriveWindow::SetupDrives()
{
    FILE *fin;
    char pzFileName[200];
    sprintf(pzFileName,"%s/Settings/Desktop/config/drives.cfg",getenv("HOME"));
    fin = fopen(pzFileName,"r+");
    if(!fin)
        ;
    else
    {
        int j =0;
        while(fscanf(fin, "%s %s %s", pzDriveDevice[j], pzDriveMount[j], pzDriveIcon[j]) !=EOF)
        {
            ListViewStringRow* pcRow = new ListViewStringRow();
            pcRow->AppendString((string)pzDriveDevice[j]);
            pcRow->AppendString((string)pzDriveMount[j]);
            pcRow->AppendString((string)pzDriveIcon[j]);
            pcList->InsertRow(pcRow);
            j++;
        }

        fclose(fin);
    }
} /*end DriveWindow::SetupDrives*/

/*
** name:       GetInit()
** purpose:    Returns all of /system/user_init.sh in a char pointer
** parameters:
** returns:    char*
*/
char* DriveWindow::GetInit()
{
    uint32 nFileSize=0;
    struct stat sFileStat;
    stat("/atheos/sys/user_init.sh",&sFileStat);
    nFileSize=sFileStat.st_size;
    char* pnBuffer=(char*)malloc(nFileSize+1);
    pnBuffer=(char*)memset(pnBuffer,0,nFileSize+1);
    uint32 nCount;
    FILE *hFile=fopen("/atheos/sys/user_init.sh","r");
    for(nCount=0;nCount<nFileSize;nCount++)
    {
        pnBuffer[nCount]=fgetc(hFile);
    }

    fclose(hFile);

    pnBuffer[nFileSize-1]=0;
    return(pnBuffer);
} /*end DriveWindow::GetLimit()*/


/*
** name:       SaveInit
** purpose:    Saves all updated information from the listview
** parameters:
** returns:  
*/
void DriveWindow::SaveInit()
{
    uint i=0;
    char pzFileName[200];
    sprintf(pzFileName,"%s/Settings/Desktop/config/drives.cfg",getenv("HOME"));
    char* pzText = GetInit();
    fout = fopen("/atheos/sys/user_init.sh","w");
    fprintf(fout,pzText);
    fprintf(fout,"\n\n#This is from the mount menu");
    for (i=0; i< pcList->GetRowCount();i++)
    {
        ListViewStringRow* pcRow =  (ListViewStringRow*)pcList->GetRow(i);
        fprintf(fout,"\nmount %s %s", pcRow->GetString(0).c_str(),pcRow->GetString(1).c_str());

    }
    fclose(fout);
    fout = fopen(pzFileName,"w+");
    for (i=0; i< pcList->GetRowCount();i++)
    {
        ListViewStringRow* pcRow =  (ListViewStringRow*)pcList->GetRow(i);
        fprintf(fout,"%s %s %s\n", pcRow->GetString(0).c_str(),pcRow->GetString(1).c_str(), pcRow->GetString(2).c_str());
    }
    fclose(fout);
} /*end SaveInit()*/


void DriveWindow::SaveDesktop()
{
    uint i;
    char pzFileName[200];
    int nPoint = (int)fIconPoint;
    for (i=0; i< pcList->GetRowCount();i++)
    {
        sprintf(pzFileName, "%s/Desktop/Drive[%d].desktop", getenv("HOME"),i);
        ListViewStringRow* pcRow =  (ListViewStringRow*)pcList->GetRow(i);
        fprintf(fout,"%s %s\n", pcRow->GetString(0).c_str(),pcRow->GetString(1).c_str());

        FILE* fout = fopen(pzFileName, "w");
        fprintf(fout, "[Icon Name]\n");
        fprintf(fout, "Drive [%d]\n",i);
        fprintf(fout,"\n[Icon Picture]\n");
        fprintf(fout,"/system/icons/drive.icon\n");
        fprintf(fout,"\n[Execute]\n");
        fprintf(fout, "LBrowser %s\n",pcRow->GetString(1).c_str());
        fprintf(fout, "\n[Icon Position]\n");
        fprintf(fout, "36 %d",nPoint+=53);
        fclose(fout);
    }
}

/*
** name:       AddWindow()
** purpose:    AddWindow Constructor
** parameters: Window* pcWindow
** returns:  
*/
AddWindow::AddWindow(Window* pcWindow) : Window(Rect(0,0,308,100),"add_window", "Add a device...", WND_NOT_RESIZABLE | WND_NOT_MOVEABLE)
{
    uint nCount;
    std::vector<DiskInfo> cDiskInfoList;

    pcWin = pcWindow;
    CenterInWindow(pcWin);

    pcDriveIcon = new CheckBox(Rect(5,60,GetBounds().Width(),75), "drive_icon","Create icon on Desktop", NULL);
    AddChild(pcDriveIcon);

    pcOk = new Button(Rect(10,80,50,100),"ok_but", "Ok",new Message(D_ADD_OK));
    AddChild(pcOk);

    pcCancel = new Button(Rect(60,80,100,100),"can_but","Cancel",new Message(D_ADD_CANCEL));
    AddChild(pcCancel);

    pcDeviceDrop = new DropdownMenu(Rect(80,8,285,25),"");
    AddChild(pcDeviceDrop);

    pcDeviceString = new StringView(Rect(5,5,75,30),"","Device:");
    AddChild(pcDeviceString);

    pcMountString = new StringView(Rect(5,30,75,55),"mount_string","Mount Point:");
    AddChild(pcMountString);

    pcText = new TextView(Rect(80,35,285,53),"mount_text","");
    AddChild(pcText);

    ImageButton* pcImageBut = new ImageButton(Rect(290,33,306,49),"blah","open",new Message(D_ADD_OPEN),GetImage("up.png"),ImageButton::IB_TEXT_BOTTOM,false, false, false, CF_FOLLOW_NONE);
    AddChild(pcImageBut);

    find_drives( &cDiskInfoList, "/dev/disk/" );

    for ( nCount =0; nCount < cDiskInfoList.size() ; nCount++ )
    {
        pcDeviceDrop->AppendItem(cDiskInfoList[nCount].m_cPath.c_str() );
    }


    pcDeviceDrop->SetReadOnly();
    pcDeviceDrop->SetSelection(0);
    SetFocusChild(pcDeviceDrop);

    pcDeviceDrop->SetTabOrder(0);
    pcText->SetTabOrder(1);
    pcOk->SetTabOrder(2);
    pcCancel->SetTabOrder(3);
    SetDefaultButton(pcOk);
} /*end AddWindow Constructor*/

/*
** name:       GetImage()
** purpose:    Gets a bitmapimage from pzFile
** parameters: const char* pzFile
** returns:    BitmapImage pointer
*/
BitmapImage* AddWindow::GetImage(const char* pzFile)
{
    Resources cCol(get_image_id());
    ResStream* pcStream = cCol.GetResourceStream(pzFile);
    BitmapImage* pcImage = new BitmapImage(pcStream);
    return (pcImage);
} /*end AddWindow::GetImage()*/

/*
** name:       ~AddWindow
** purpose:    AddWindow destructor
** parameters:
** returns:  
 
AddWindow::~AddWindow()
{
} end AddWindow Destructor*/

/*
** name:       HandleMessage
** purpose:    Handles messages to and from the window
** parameters: Message* pcMessage
** returns:  
*/
void AddWindow::HandleMessage(Message* pcMessage)
{
    switch (pcMessage->GetCode())
    {
    case D_ADD_OK:
        {
            bool bDriveIcon = pcDriveIcon->GetValue().AsBool();
            Message* pcMsg = new Message(D_ADD_PASS);
            pcMsg->AddString("pass_device", pcDeviceDrop->GetCurrentString());
            pcMsg->AddString("pass_mount",pcText->GetBuffer()[0]);
            if (bDriveIcon == true)
                pcMsg->AddString("pass_icon","true");
            else
                pcMsg->AddString("pass_icon", "false");

            DispatchMessage(pcMsg, pcWin);
        }
        break;

    case D_ADD_CANCEL:
        Close();
        break;

    case D_ADD_OPEN:
        {
            pcOpenRequest = new FileRequester(FileRequester::LOAD_REQ, new Messenger(this),getenv("$HOME"),FileRequester::NODE_DIR,false,NULL,NULL,true,true,"Open","Cancel");
            pcOpenRequest->Show();
            pcOpenRequest->MakeFocus();
        }

    case M_LOAD_REQUESTED:
        {

            if (pcMessage->FindString("file/path",&pzFPath) == 0)
            {
                strcpy((char*)pzStorePath.c_str(),pzFPath.c_str());
                struct stat my_stat;
                stat(pzStorePath.c_str(),&my_stat);

                if(my_stat.st_mode & S_IFDIR)
                {
                    pcText->Set(pzStorePath.c_str());
                }

                else
                {
                    Alert* m_pcError = new Alert("Must be a directory","You must select a directory.",Alert::ALERT_INFO,0x00,"OK",NULL);
                    m_pcError->Go(new Invoker());
                }


            }
            break;
        }
    }
} /*end AddWindow::HandleMessage()*/











