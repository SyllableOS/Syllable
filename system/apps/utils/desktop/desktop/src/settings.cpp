#include "settings.h"

DeskSettings::DeskSettings()
{
    string cString = (string) getenv("HOME");
    mkdir ((cString +  (string)"/Settings").c_str(),0700);
    mkdir ((cString + (string)"/Settings/Desktop").c_str(), 0700);
    mkdir ((cString + (string)"/Settings/Desktop/config").c_str(), 0700);
    mkdir ((cString + (string)"/Settings/Desktop/Startup").c_str(), 0700);
    mkdir ((cString + (string)"/Settings/Desktop/Themes").c_str(),0700);
    mkdir ((cString + (string)"/Documents").c_str(),0700);
    mkdir ((cString + (string)"/Documents/Pictures").c_str(), 0700);
    mkdir ((cString + (string)"/SettingsDesktop/Templates").c_str(), 0700);
    mkdir ((cString + "/Desktop").c_str(), 0700);
    mkdir ((cString + "/Trash").c_str(), 0700);

    sprintf(pzConfigFile,"%s/Settings/Desktop/desktop.cfg",getenv("HOME"));
    sprintf(pzConfigDir, "%s/Settings/Desktop/",getenv("HOME"));
    sprintf(pzImageDir, "%s/Documents/Pictures/",getenv("HOME"));
    sprintf(pzIconDir, "%s/Desktop/",getenv("HOME"));
	sprintf(pzExtDir, "%s/Settings/Desktop/Templates",getenv("HOME"));
    pcReturnMessage = new Message();

    DefaultSettings();
    ReadSettings();
}

void DeskSettings::DefaultSettings()
{
    zDeskImage= "^";
    bShowVer = false;
    bAlpha = false;
    cBgColor = Color32_s(200,200,200);
    cFgColor = Color32_s(0,0,0);
    nSizeImage = 0;
}

void DeskSettings::SaveSettings(Message* pcMessage)
{
    File *pcConfig = new File("/tmp/desktop.cfg", O_WRONLY | O_CREAT );
    if( pcConfig )
    {
        uint32 nSize = pcMessage->GetFlattenedSize( );
        void *pBuffer = malloc( nSize );
        pcMessage->Flatten( (uint8 *)pBuffer, nSize );

        pcConfig->Write( pBuffer, nSize );

        delete pcConfig;
        free( pBuffer );
    }
    system("/usr/bin/mv -f /tmp/desktop.cfg ~/Settings/Desktop/desktop.cfg"); /* find function that is better*/

}

Message* DeskSettings::GetSettings()
{
    pcReturnMessage->AddString("DeskImage",zDeskImage);
    pcReturnMessage->AddColor32("Back_Color",cBgColor);
    pcReturnMessage->AddColor32("Icon_Color",cFgColor);
    pcReturnMessage->AddBool("ShowVer", bShowVer);
    pcReturnMessage->AddInt32("SizeImage",nSizeImage);

    return(pcReturnMessage);
}

void DeskSettings::ReadSettings()
{

    FSNode *pcNode = new FSNode();

    if( pcNode->SetTo(pzConfigFile) == 0 )
    {
        File *pcConfig = new File( *pcNode );
        uint32 nSize = pcConfig->GetSize( );
        void *pBuffer = malloc( nSize );
        pcConfig->Read( pBuffer, nSize );
        Message *pcPrefs = new Message( );
        pcPrefs->Unflatten( (uint8 *)pBuffer );

        pcPrefs->FindColor32( "Back_Color", &cBgColor );
        pcPrefs->FindColor32( "Icon_Color",   &cFgColor );
        pcPrefs->FindString ( "DeskImage",  &zDeskImage  );
        pcPrefs->FindBool   ( "ShowVer",    &bShowVer   );
        pcPrefs->FindInt32  ( "SizeImage",  &nSizeImage);
        pcPrefs->FindBool   ( "Alphabet",   &bAlpha);
        pcPrefs->FindBool   ("Transparent", &bTrans);


    }
    else
    {
        Message* pcCreateSettings = GetSettings();
        DefaultSettings();
        SaveSettings(pcCreateSettings);
        ReadSettings();

    }
}



void DeskSettings::ResetSettings()
{

    FSNode *pcNode = new FSNode();

    if( pcNode->SetTo(pzConfigFile) == 0 )
    {
        File *pcConfig = new File( *pcNode );
        uint32 nSize = pcConfig->GetSize( );
        void *pBuffer = malloc( nSize );
        pcConfig->Read( pBuffer, nSize );
        Message *pcPrefs = new Message( );
        pcPrefs->Unflatten( (uint8 *)pBuffer );

        pcPrefs->FindColor32( "Back_Color", &cBgColor );
        pcPrefs->FindColor32( "Icon_Color",   &cFgColor );
        pcPrefs->FindString ( "DeskImage",  &zDeskImage  );
        pcPrefs->FindBool   ( "ShowVer",    &bShowVer   );
        pcPrefs->FindInt32  ( "SizeImage",  &nSizeImage);
        pcPrefs->FindBool   ( "Alphabet",   &bAlpha);

    }
    else
    {}
}


const char* DeskSettings::GetIconDir()
{
    return (pzIconDir);
}

const char* DeskSettings::GetSetDir()
{
    return (pzConfigDir);
}

const char* DeskSettings::GetConfigFile()
{
    return (pzConfigFile);
}

const char* DeskSettings::GetImageDir()
{
    return (pzImageDir);
}

const char* DeskSettings::GetExtDir()
{
    return (pzExtDir);
}

Color32_s DeskSettings::GetBgColor()
{
    return (cBgColor);
}

Color32_s DeskSettings::GetFgColor()
{
    return (cFgColor);
}

int32 DeskSettings::GetImageSize()
{
    return (nSizeImage);
}

bool DeskSettings::GetTrans()
{
    return (bTrans);
}

string DeskSettings::GetImageName()
{
    return( zDeskImage);
}

bool DeskSettings::GetVersion()
{
    return (bShowVer);
}





































