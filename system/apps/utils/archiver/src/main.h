#ifndef MAIN_H
#define MAIN_H
#include <util/application.h>
#include "archiver.h"
#include <iostream>
#include <sys/ioctl.h>
#include <getopt.h>
int main(int argc, char** argv);

using namespace os;


static int g_nShowHelp = 0;
static int g_nOpen = 0;
static struct option const long_opts[] =
    {
        {"help", no_argument,&g_nShowHelp,1},
        {"open", no_argument,&g_nOpen,1},
        {"extract",no_argument,NULL,1},
        {NULL,0,NULL,0}
    };

class ArcApp : public Application
{
public:

    ArcApp(char* cFileName);
    ArcApp();

private:

    ArcWindow* win;
};
#endif
