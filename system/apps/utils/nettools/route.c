#include <stdio.h>
#include <errno.h>
#include <getopt.h>
#include <malloc.h>

#include <netinet/in.h>
#include <net/net.h>
#include </ainc/net/route.h>
#include <sys/ioctl.h>

int g_nShowHelp = 0;
int g_nShowVersion = 0;

#define IP_ADR_LEN 4

static struct option const long_opts[] =
{
  {"interface", required_argument, NULL, 'i' },
  {"gateway", required_argument, NULL, 'g' },
  {"help", no_argument, &g_nShowHelp, 1},
  {"version", no_argument, &g_nShowVersion, 1},
  {NULL, 0, NULL, 0}
};




void usage( const char* pzStr, int nLevel )
{
  printf( "Usage: %s [add|del|list] [-i interface] [-g gateway] [address netmask]\n", pzStr );
  if ( nLevel > 0 ) {
    printf( "  -i --interface=name   specify a network interface\n" );
    printf( "  -g --gateway=address  set the gateway address\n" );
    printf( "  -v --version          display version information and exit\n" );
    printf( "  -h --help             display this help and exit\n" );
  }
}

char* format_ip( int nAddress )
{
    static char zBuffer[4][128];
    static int i = 0;
    char* pzRet;
    
    sprintf( zBuffer[i], "%d.%d.%d.%d", nAddress & 0xff, (nAddress >> 8 & 0xff), (nAddress >> 16 & 0xff), (nAddress >> 24 & 0xff) );
    pzRet = zBuffer[i];
    i = (i + 1) % 4;
    return( pzRet );
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


static void list_routs(void)
{
    int nSocket = socket( AF_INET, SOCK_STREAM, 0 );
    struct rttable* psRtTable;
    struct rtabentry* psTable;
    int nError;
    int i;
    
    psRtTable = malloc( sizeof( struct rttable ) + sizeof( struct rtabentry ) * 128 );

    if ( psRtTable == NULL ) {
	fprintf( stderr, "Out of memory: %s\n", strerror( errno ) );
	exit( 1 );
    }
    psTable = (struct rtabentry*)(psRtTable + 1 );
    
    if ( nSocket < 0 ) {
	fprintf( stderr, "Failed to create socket: %s\n", strerror( errno ) );
	exit( 1 );
    }
    psRtTable->rtt_count = 128;
    
    nError = ioctl( nSocket, SIOCGETRTAB, psRtTable );
    if ( nError < 0 ) {
	fprintf( stderr, "Failed to get routing table: %s\n", strerror( errno ) );
	exit( 1 );
    }
    for ( i = 0 ; i < psRtTable->rtt_count ; ++i ) {
	printf( "%02d: %s, %s, %s, %s\n", i, psTable[i].rt_dev,
		format_ip( ((struct sockaddr_in*)&psTable[i].rt_dst)->sin_addr.s_addr ),
		format_ip( ((struct sockaddr_in*)&psTable[i].rt_genmask)->sin_addr.s_addr ),
		format_ip( ((struct sockaddr_in*)&psTable[i].rt_gateway)->sin_addr.s_addr ));
    }
    free( psRtTable );
    close( nSocket );
}

void add_route( struct rtentry* psRoute )
{
    int nSocket = socket( AF_INET, SOCK_STREAM, 0 );
    
    if ( nSocket < 0 ) {
	fprintf( stderr, "Failed to create socket: %s\n", strerror( errno ) );
	exit( 1 );
    }

    if ( ioctl( nSocket, SIOCADDRT, psRoute ) < 0 ) {
	fprintf( stderr, "Failed to add route: %s\n", strerror( errno ) );
	exit( 1 );
    }
    close( nSocket );
}

void del_route( struct rtentry* psRoute )
{
    int nSocket = socket( AF_INET, SOCK_STREAM, 0 );
    
    if ( nSocket < 0 ) {
	fprintf( stderr, "Failed to create socket: %s\n", strerror( errno ) );
	exit( 1 );
    }

    if ( ioctl( nSocket, SIOCDELRT, psRoute ) < 0 ) {
	fprintf( stderr, "Failed to delete route: %s\n", strerror( errno ) );
	exit( 1 );
    }
    close( nSocket );
}

int main( int argc, char** argv )
{
    uint8	anAddress[4];
    uint8	anMask[4];
    uint8	anGateway[4] = {0,0,0,0};
    const char* pzGateway   = NULL;
    struct rtentry sRoute;
    int	c;

    memset( &sRoute, 0, sizeof( sRoute ) );

    sRoute.rt_metric = 1;
    
    while( (c = getopt_long (argc, argv, "hvi:g:", long_opts, (int *) 0)) != EOF )
    {
	switch (c)
	{
	    case 0:
		break;
	    case 'g':
		pzGateway = optarg;
		sRoute.rt_flags |= RTF_GATEWAY;
		break;
	    case 'i':
		sRoute.rt_dev = optarg;
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
    if ( argc - optind == 1 && strcmp( argv[optind], "list" ) == 0 ) {
	list_routs();
	exit( 0 );
    }
    
    if ( argc - optind != 3 ) {
	usage( argv[0], 0 );
	exit(1);
    }
    if ( pzGateway != NULL ) {
	if ( parse_ipaddress( anGateway, pzGateway ) < 0 ) {
	    printf( "Invalid GW address\n" );
	    return( 1 );
	}
	memcpy( &(((struct sockaddr_in*)&sRoute.rt_gateway))->sin_addr, anGateway, 4 );
    }
    if ( parse_ipaddress( anAddress, argv[optind + 1] ) < 0 ) {
	printf( "Invalid IP address: %s\n", argv[optind + 1] );
	return( 1 );
    }
    if ( parse_ipaddress( anMask, argv[optind + 2] ) < 0 ) {
	printf( "Invalid net mask: %s\n", argv[optind + 2] );
	return( 1 );
    }
    memcpy( &(((struct sockaddr_in*)&sRoute.rt_dst))->sin_addr, anAddress, 4 );
    memcpy( &(((struct sockaddr_in*)&sRoute.rt_genmask))->sin_addr, anMask, 4 );

    printf( "Addr: i: %s %s Mask: %s, GW: %s\n", sRoute.rt_dev,
	    format_ip( (((struct sockaddr_in*)&sRoute.rt_dst))->sin_addr.s_addr ),
	    format_ip( (((struct sockaddr_in*)&sRoute.rt_genmask))->sin_addr.s_addr ),
	    format_ip( (((struct sockaddr_in*)&sRoute.rt_gateway))->sin_addr.s_addr ) );
    
    if ( strcmp( argv[optind + 0], "add" ) == 0 ) {
	add_route( &sRoute );
    } else if ( strcmp( argv[optind + 0], "del" ) == 0 ) {
	del_route( &sRoute );
    }
    return( 0 );
}
