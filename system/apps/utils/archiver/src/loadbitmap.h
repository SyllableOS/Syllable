#ifndef LOADBITMAP_H
#define LOADBITMAP_H

#include <gui/image.h>
#include <util/message.h>
#include <translation/translator.h>
#include <atheos/time.h>
#include <gui/guidefines.h>
#include <util/message.h>
#include <util/string.h>
#include <util/locker.h>
#include <util/resources.h>
#include <storage/file.h>
#include <util/string.h>

using namespace os;


BitmapImage *LoadImageFromResource(const String&);
BitmapImage *LoadImageFromFile(const String&);

#endif




