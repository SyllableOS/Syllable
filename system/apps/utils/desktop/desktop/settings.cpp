#include "settings.h"
//#include "debug.h"

DeskSettings::DeskSettings()
{
	system("mkdir ~/config 2> /dev/null");
    system("mkdir ~/config/desktop 2> /dev/null");
    system("mkdir ~/config/desktop/config 2> /dev/null");
    system("mkdir ~/config/desktop/startup 2> /dev/null");
    system("mkdir ~/config/desktop/pictures 2> /dev/null");
    system("mkdir ~/config/desktop/disks 2> /dev/null");
    system("mkdir ~/Desktop 2> /dev/null");
    system("mkdir ~/Trash 2> /dev/null");
	
	
	sprintf(pzConfigFile,"%s/config/desktop/config/desktop.cfg",getenv("HOME"));
    sprintf(pzConfigDir, "%s/config/desktop/config/",getenv("HOME"));
    sprintf(pzImageDir, "%s/config/desktop/pictures/",getenv("HOME"));
    sprintf(pzIconDir, "%s/Desktop/",getenv("HOME"));
	
	pcReturnMessage = new Message();
	
	//Debug("DeskSettings::DeskSettings(), launching ReadSettings()");
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
    
     system("/usr/bin/mv -f /tmp/desktop.cfg ~/config/desktop/config");
  
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
	
	//Debug("DeskSettings::ReadSettings() reads the settings from ~/config/desktop/config/desktop.cfg");
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
        
    
    }else {
    	//Debug ("DeskSettings::ReadSettings(), ~/config/desktop/config/desktop.cfg is\n   nonexsistant, we will try to create it");
		Message* pcCreateSettings = GetSettings();
		DefaultSettings();
		SaveSettings(pcCreateSettings);
		ReadSettings();
		
	}
}



void DeskSettings::ResetSettings()
{
	
	//Debug("DeskSettings::ResetSettings() reads the settings from ~/config/desktop/config/desktop.cfg");
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
    
    }else {
    	//Debug ("DeskSettings::ResetSettings(), ~/config/desktop/config/desktop.cfg is\n   nonexsistant, we will try to create it");
	}
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





















