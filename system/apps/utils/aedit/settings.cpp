// AEdit -:-  (C)opyright 2000-2002 Kristian Van Der Vliet
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "settings.h"

#include <util/message.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/dir.h>

AEditSettings::AEditSettings(void)
{
	zSettingsFile=getenv("HOME");
	zSettingsFile+=AEDIT_SETTINGS_PATH;
}

AEditSettings::~AEditSettings()
{

}

void AEditSettings::SetWindowPos(Rect cPosition)
{
	cWindowPosition=cPosition;
	return;
}

Rect AEditSettings::GetWindowPos(void)
{
	return(cWindowPosition);
}

void AEditSettings::SetSaveOnExit(bool bSave)
{
	bSaveOnExit=bSave;
	return;
}

bool AEditSettings::GetSaveOnExit(void)
{
	return(bSaveOnExit);
}

int AEditSettings::Load(void)
{
	// Get the filesize & allocate a buffer large enough for it
	uint nFileSize;
	struct stat hSettingsFileStat;

	if(stat(zSettingsFile.c_str(),&hSettingsFileStat)==-1)
		return(-ESETTINGSSTAT);

	nFileSize=hSettingsFileStat.st_size;
	uint8* nBuffer=(uint8*)malloc(nFileSize);

	// Open & read the flattened data out of the config file
	int hSettingsFile;
	uint nBytesRead;

	hSettingsFile=open(zSettingsFile.c_str(),O_RDONLY,0);
	if(hSettingsFile==-1)
		return(-ESETTINGSREAD);

	nBytesRead=read(hSettingsFile,nBuffer,nFileSize);
	close(hSettingsFile);

	// Unflatten the message data
	Message* pcSettingsMessage=new Message((void*)nBuffer);

	if(pcSettingsMessage->GetCode()!=887)
	{
		free(nBuffer);
		return(-ESETTINGSREAD);
	}

	// Extract the settings from the message
	pcSettingsMessage->FindRect("win_pos",&cWindowPosition);
	pcSettingsMessage->FindBool("save_exit",&bSaveOnExit);

	// Clean up after ourselves & return
	delete(pcSettingsMessage);
	free(nBuffer);
	return(EOK);
}

int AEditSettings::Save(void)
{
	// Create a message to hold the settings data
	Message* pcSettingsMessage=new Message(887);		// Semi-random magic number

	pcSettingsMessage->AddRect("win_pos",cWindowPosition);
	pcSettingsMessage->AddBool("save_exit",bSaveOnExit);

	// Flatten the message
	uint nBufferSize=pcSettingsMessage->GetFlattenedSize();
	uint8* nBuffer=(uint8*)malloc(nBufferSize);
	pcSettingsMessage->Flatten(nBuffer,nBufferSize);

	// Write the file out
	int hSettingsFile;
	uint nBytesWritten;

	hSettingsFile=open(zSettingsFile.c_str(),O_WRONLY,0);
	if(hSettingsFile==-1)
	{
		hSettingsFile=creat(zSettingsFile.c_str(),0666);

		if(hSettingsFile==-1)
			return(-ESETTINGSCREATE);

		hSettingsFile=open(zSettingsFile.c_str(),O_WRONLY,0);
		if(hSettingsFile==-1)
			return(-ESETTINGSWRITE);
	}

	nBytesWritten=write(hSettingsFile,nBuffer,nBufferSize);
	if(nBytesWritten!=nBufferSize)
	{
		close(hSettingsFile);
		return(-ESETTINGSWRITE);
	}
	
	// Clean up after ourselves & return
	close(hSettingsFile);
	free(nBuffer);
	delete pcSettingsMessage;

	return(EOK);
}


