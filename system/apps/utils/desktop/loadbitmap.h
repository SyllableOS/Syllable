#ifndef LOADBITMAP_H
#define LOADBITMAP_H

#include <gui/bitmap.h>
#include <util/message.h>
#include <translation/translator.h>
#include <atheos/time.h>
#include <gui/guidefines.h>
#include <util/message.h>
#include <util/string.h>
#include <util/locker.h>
#include <util/resources.h>
#include <storage/file.h>

using namespace os;

Bitmap *LoadBitmapFromFile(const char *name);
Bitmap *LoadBitmapFromResource(const char *name);
Bitmap *LoadBitmapFromStream(StreamableIO* pcStream);
Bitmap *LoadIconFromStream(StreamableIO* pcStream);
Bitmap *LoadIconFromFile(const char*name);
#endif





