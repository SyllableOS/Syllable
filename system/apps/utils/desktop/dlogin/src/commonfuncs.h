#ifndef COMMON_FUNCS_H
#define COMMON_FUNCS_H

#include "include.h"
using namespace os;

class LoginWindow;

int 				BecomeUser(struct passwd *psPwd, LoginWindow* pcWindow);
void 				UpdateLoginConfig(os::String zName);
void 				WriteLoginConfigFile();
const char* 		ReadLoginOption();
void 				CheckLoginConfig();
os::String			GetSyllableVersion();
os::BitmapImage* 	LoadImageFromResource( const os::String &cName );

#endif
