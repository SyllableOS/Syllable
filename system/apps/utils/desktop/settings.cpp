#include "settings.h"

DeskSettings::DeskSettings()
{
	zDeskImage= "";
	bShowVer = false;
	bAlpha = false;
	cBgColor = Color32_s(200,200,200);
	cFgColor = Color32_s(0,0,0);
	nSizeImage = 1;
	
	sprintf(pzConfigFile,"%s/config/desktop/config/desktop.cfg",getenv("HOME"));
    sprintf(pzConfigDir, "%s/config/desktop/config/",getenv("HOME"));
    sprintf(pzImageDir, "%s/config/desktop/pictures/",getenv("HOME"));
    sprintf(pzIconDir, "%s/Desktop/",getenv("HOME"));
	
	pcReturnMessage = new Message();
	ReadSettings();
}

void DeskSettings::SaveSettings(Message* pcMessage)
{
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

        pcPrefs->FindColor32( "BackGround", &cBgColor );
        pcPrefs->FindColor32( "IconText",   &cFgColor );
        pcPrefs->FindString ( "DeskImage",  &zDeskImage  );
        pcPrefs->FindBool   ( "ShowVer",    &bShowVer   );
        pcPrefs->FindInt32  ( "SizeImage",  &nSizeImage);
        pcPrefs->FindBool   ( "Alphabet",   &bAlpha);

        //m_pcBitmap = ReadBitmap(zDImage.c_str());
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


