#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>


#include <vector>
#include <string>

int g_nTermWidth = 80;
int g_nTermHeight = 25;
bool	g_bSizeChanged = false;
bool    g_bGotSigWINCH = false;

#define	min(a,b)	(((a)<(b)) ? (a) : (b) )

void get_term_width()
{
    struct winsize sWinSize;

    if ( ioctl( 1, TIOCGWINSZ, &sWinSize ) >= 0 ) {
	if ( g_nTermWidth != sWinSize.ws_col ) {
	    g_nTermWidth = sWinSize.ws_col;
	    g_bSizeChanged = true;
	}
	g_nTermHeight= sWinSize.ws_row;
    }
}

void sig_winch( int nSig )
{
    g_bGotSigWINCH = true;
}

int main( int argc, char** argv )
{
    get_term_width();

    std::vector<std::string> cHistory;

    signal( SIGWINCH, sig_winch );
    int nHistorySize = 0;
    for (;;) {
	char anBuffer[1024];
	
	if ( fgets( anBuffer, 1024, stdin ) != NULL ) {
	    for ( int i = 0 ; i < 1024 ; ++i ) {
		if ( anBuffer[i] == '\n' || anBuffer[i] == '\r' || anBuffer[i] == '\0' ) {
		    anBuffer[i] = '\0';
		    break;
		}
	    }
	    if ( anBuffer[0] == '\0' ) {
		continue;
	    }
	    if ( anBuffer[strlen(anBuffer)-1] == '\n' ) {
		anBuffer[strlen(anBuffer)-1] == '\0';
	    }
	    cHistory.push_back(anBuffer);
	    if ( nHistorySize == 1024 ) {
		cHistory.erase( cHistory.begin() );
		nHistorySize--;
	    }
	    nHistorySize++;
	    anBuffer[g_nTermWidth] = '\0';
	    printf( "%s\n", anBuffer );
	}
	if ( g_bGotSigWINCH ) {
	    g_bGotSigWINCH = false;
	    get_term_width();
	    if ( g_bSizeChanged ) {
		write( 1, "\x1b[H;", 4 );
		for ( int i = g_nTermHeight ; i > 0  ; --i ) {
		    if ( i >= cHistory.size() ) {
			break;
		    }
		    strncpy( anBuffer, cHistory[cHistory.size() - i].c_str(), g_nTermWidth );
		    anBuffer[g_nTermWidth] = '\0';
		    printf( "%s\x1b[K\n", anBuffer );
//		    write( 1, cHistory[cHistory.size() - i - 1].c_str(), min( cHistory[cHistory.size() - i - 1].size(), g_nTermWidth ) );
//		    write( 1, "\x1b[K", 3 );
		}
		printf( "\x1b[J" );
//		write( 1, "\x1b[J", 3 );
		g_bSizeChanged = false;
	    }
	}
	fflush( stdout );
    }

    
}
