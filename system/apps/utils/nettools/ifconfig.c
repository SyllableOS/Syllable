#include <stdio.h>
#include <errno.h>
#include <getopt.h>

#include <atheos/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <sys/ioctl.h>

int g_nShowHelp = 0;
int g_nShowVersion = 0;

#define IP_ADR_LEN 4

static struct option const long_opts[] =
{
  {"help", no_argument, &g_nShowHelp, 1},
  {"version", no_argument, &g_nShowVersion, 1},
  {NULL, 0, NULL, 0}
};


void usage( const char* pzStr, int nLevel )
{
  printf( "Usage: %s [-i interface] [-g gateway] [address netmask]\n", pzStr );
  if ( nLevel > 0 ) {
    printf( "  -v --version          display version information and exit\n" );
    printf( "  -h --help             display this help and exit\n" );
  }
}

int parse_ipaddress( uint8* pAddress, const char* pzBuffer )
{
  int i;
  const char* pzPtr = pzBuffer;
  
  for ( i = 0 ; i < IP_ADR_LEN ; ++i ) {
    char* pzNext;
    
    pAddress[i] = strtol( pzPtr, &pzNext, 10 );
    if ( pzNext == pzPtr ) {
      return( -EINVAL );
    }
    while( *pzNext == ' ' || *pzNext == '\t' ) pzNext++;
    
    if ( i != (IP_ADR_LEN - 1) && pzNext[0] != '.' ) {
      return( -EINVAL );
    }
    pzPtr = pzNext + 1;
  }
  return( pzPtr - pzBuffer - 1 );
}

int main( int argc, char** argv )
{
    struct sockaddr_in sAddress;
    struct sockaddr_in sMask;
    uint8	 anAddress[4];
    uint8	 anMask[4];
  
    int	c;
  
    while( (c = getopt_long (argc, argv, "hv", long_opts, (int *) 0)) != EOF )
    {
	switch (c)
	{
	    case 0:
		break;
	    case 'v':
		g_nShowVersion = true;
		break;
	    case 'h':
		g_nShowHelp = true;
		break;
	    default:
		usage( argv[0], 0 );
		return( 1 );
	}
    }
  
    argc -= optind;
  
    if ( g_nShowHelp ) {
	usage( argv[0], 0 );
	exit( 1 );
    }

    if ( argc == 2 ) {
	struct ifreq  sIFReq;
	int nSocket;
	nSocket = socket( AF_INET, SOCK_STREAM, 0 );

	memset( &sIFReq, 0, sizeof( sIFReq ) );
	strcpy( sIFReq.ifr_name, argv[optind] );
	
	if ( nSocket < 0 ) {
	    fprintf( stderr, "Error: failed to create socket: %s\n", strerror( errno ) );
	    return( 1 );
	}
	if ( strcmp( argv[optind+1], "up" ) == 0 ) {
	    if ( ioctl( nSocket, SIOCGIFFLAGS, &sIFReq ) < 0 ) {
		fprintf( stderr, "Error: Failed to read interface flags: %s\n", strerror( errno ) );
		exit( 1 );
	    }
	    printf( "Start IF\n" );
	    sIFReq.ifr_flags |= IFF_UP;
	    if ( ioctl( nSocket, SIOCSIFFLAGS, &sIFReq ) < 0 ) {
		fprintf( stderr, "Error: Failed to set interface flags: %s\n", strerror( errno ) );
		exit( 1 );
	    }
	} else if ( strcmp( argv[optind+1], "down" ) == 0 ) {
	    if ( ioctl( nSocket, SIOCGIFFLAGS, &sIFReq ) < 0 ) {
		fprintf( stderr, "Error: Failed to read interface flags: %s\n", strerror( errno ) );
		exit( 1 );
	    }
	    printf( "Shut down IF\n" );
	    sIFReq.ifr_flags &= ~IFF_UP;
	    if ( ioctl( nSocket, SIOCSIFFLAGS, &sIFReq ) < 0 ) {
		fprintf( stderr, "Error: Failed to set interface flags: %s\n", strerror( errno ) );
		exit( 1 );
	    }
	} else {
	    close( nSocket );
	    usage( argv[0], 0 );
	    exit( 1 );
	}
	close( nSocket );
    } else if ( argc == 3 ) {
	struct ifreq  sIFReq;
	int nSocket;

    
	if ( parse_ipaddress( anAddress, argv[optind + 1] ) < 0 ) {
	    printf( "Invalid IP address\n" );
	    return( 1 );
	}
	if ( parse_ipaddress( anMask, argv[optind + 2] ) < 0 ) {
	    printf( "Invalid net mask\n" );
	    return( 1 );
	}
    
	nSocket = socket( AF_INET, SOCK_STREAM, 0 );

	if ( nSocket < 0 ) {
	    fprintf( stderr, "Error: failed to create socket: %s\n", strerror( errno ) );
	    return( 1 );
	}
    
	memset( &sIFReq, 0, sizeof( sIFReq ) );
	strcpy( sIFReq.ifr_name, argv[optind] );
	memcpy( &(((struct sockaddr_in*)&sIFReq.ifr_addr))->sin_addr, anAddress, 4 );

	if ( ioctl( nSocket, SIOCSIFADDR, &sIFReq ) < 0 ) {
	    printf( "Failed to set interface address: %s\n", strerror( errno ) );
	}
    
	memset( &sIFReq, 0, sizeof( sIFReq ) );
	strcpy( sIFReq.ifr_name, argv[optind] );
	memcpy( &(((struct sockaddr_in*)&sIFReq.ifr_addr))->sin_addr, anMask, 4 );
    
	if ( ioctl( nSocket, SIOCSIFNETMASK, &sIFReq ) < 0 ) {
	    printf( "Failed to set interface netmask: %s\n", strerror( errno ) );
	}
	close( nSocket );
    }
    return( 0 );
}
