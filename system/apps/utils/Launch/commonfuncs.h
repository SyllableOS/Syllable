#ifndef _COMMON_FUNCS_H
#define _COMMON_FUNCS_H

#include <util/string.h>
#include <storage/file.h>
#include <storage/path.h>
#include "execute.h"

bool LaunchFile(const String& cFile);
bool Launch(const String& cFile);
bool IsExecutable(const String& cString);
bool IsWebsite(const String& cWebsite);
bool IsDirectory(const String& cDir);

#endif //COMMON_FUNCS_H

