#include <util/application.h>
#include "archiver.cpp"
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


static void usage(const char* pzName, bool bFull)
{
    if(bFull){

        printf("Archiver is brought to you by Rick Caudill.\n");
        printf("Usage:\t Archiver [OPTION]... [FILE]...\n\n");
        //printf("  -e\tExtracts an archive from the command line.\n");
        printf("  -h\tDisplays this information.\n");
        printf("  -o\tOpens the file in Archiver.\n");
    }
}

int main(int argc, char** argv)
{
    ArcApp *thisApp;

    int c;
    int nNameLen = strlen(argv[0] );
    while((c = getopt_long (argc, argv,"-hoe",long_opts, (int*)0))!= EOF)
    {
        switch(c)
        {
        case 0:
            exit(0);
            break;
        case 'h':
            usage(argv[0],true);
            exit(0);
            break;
        case 'o':
            thisApp = new ArcApp(argv[2]);
            //thisApp->Run();
            //exit(0);
            break;
        case 'e':
            printf("This hasn't been implemented yet\n");
            exit(0);
            break;
            
        }
    }
    if (argc <=1){
        thisApp = new ArcApp();
    }

    thisApp->Run();
    return(0);
}



ArcApp :: ArcApp(char* cFileName) :Application("application/x-vnd.caudill-archiver")
{
    system( "mkdir ~/config 2> /dev/null" );
    system( "mkdir ~/config/Archiver 2> /dev/null" );
    //printf(cFileName);  // debug info

    win = new ArcWindow();
    win->LoadAtStart(cFileName);
    win->Show();
    win->MakeFocus();
   
}

ArcApp :: ArcApp() :Application("application/x-vnd.caudill-archiver")
{
    system( "mkdir ~/config 2> /dev/null" );
    system( "mkdir ~/config/Archiver 2> /dev/null" );
    win = new ArcWindow();
    win->Show();
    win->MakeFocus();

}


