#include "main.h"
#include "settings.h"

static void usage(const char* pzName, bool bFull)
{
    if(bFull){
		printf("Archiver is brought to you by Rick Caudill.\n");
        printf("Usage:\t Archiver [OPTION]... [FILE]...\n\n");
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
            thisApp->Run();
            exit(0);
            break;
        case 'e':
            printf("This hasn't been implemented yet\n");
            exit(0);
            break;
            
        }
    }
    if (argc <=1){
        thisApp = new ArcApp("");
    }
    thisApp->Run();
    return(0);
}



ArcApp :: ArcApp(char* argv) :Application("application/x-vnd.RGC.archiver")
{
	system( "mkdir ~/Settings/Archiver 2> /dev/null" );
	pcSettings = new AppSettings();
	
	win = new ArcWindow(pcSettings);
	
	if (argv != "")
	{
    	win->LoadAtStart(argv);
    }
	win->CenterInScreen();
	win->Show();
    win->MakeFocus();   
}
