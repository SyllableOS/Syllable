#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <malloc.h>

#include <netinet/in.h>
#include <net/net.h>
#include <net/route.h>
#include <sys/ioctl.h>

int g_nShowHelp = 0;
int g_nShowVersion = 0;

#define IP_ADR_LEN 4
#define VERSION	"0.5"

static struct option const long_opts[] = {
	{"long", required_argument, NULL, 'l'},
	{"help", no_argument, &g_nShowHelp, 1},
	{"version", no_argument, &g_nShowVersion, 1},
	{NULL, 0, NULL, 0}
};




void usage( const char *pzStr, int nLevel )
{
	printf( "Usage: %s [-l] (add|del) net <address> mask <netmask> [gw <gateway>]\n", pzStr );
	printf( "       %s [-l] list\n", pzStr );
	if ( nLevel > 0 )
	{
		printf( "  -l --long             show full netmask\n" );
		printf( "  -v --version          display version information and exit\n" );
		printf( "  -h --help             display this help and exit\n" );
	}
}

char *format_ip( uint8* anIp )
{
	static char zBuffer[4][128];
	static int i = 0;
	char *pzRet;

	sprintf( zBuffer[i], "%d.%d.%d.%d", (int)anIp[0], (int)anIp[1], (int)anIp[2], (int)anIp[3] );
	pzRet = zBuffer[i];
	i = ( i + 1 ) % 4;

	return ( pzRet );
}

int parse_ipaddress( uint8 *pAddress, const char *pzBuffer )
{
	int i;
	const char *pzPtr = pzBuffer;

	for ( i = 0; i < IP_ADR_LEN; ++i )
	{
		char *pzNext;

		pAddress[i] = strtol( pzPtr, &pzNext, 10 );
		if ( pzNext == pzPtr )
		{
			return ( -EINVAL );
		}
		while ( *pzNext == ' ' || *pzNext == '\t' )
			pzNext++;

		if ( i != ( IP_ADR_LEN - 1 ) && pzNext[0] != '.' )
		{
			return ( -EINVAL );
		}
		pzPtr = pzNext + 1;
	}
	return ( pzPtr - pzBuffer - 1 );
}

static void print_route( uint8* anIp, uint8* anMask, uint8* anGw, uint16 nFlags, bool bLong )
{
	char *pzIpAddr, *pzGateway = "";

	if( ( nFlags & RTF_GATEWAY ) != 0 )
	{
		if( anGw != NULL )
			pzGateway = format_ip( anGw );
		else
			pzGateway = "not specified";
	}

	if( bLong )
	{
		printf( "%-15s  %-15s  %-15s  0x%4.4hX\n", format_ip( anIp ), format_ip( anMask ), pzGateway, nFlags );
	}
	else
	{
		int nMaskBits = 0;
		unsigned long nIpAddr;
		char acDestBuf[20];
		char acFlags[16], *pcFlag = acFlags;
		struct flag {
			uint16 nFlag;
			char cFlag, cNotFlag;
		} *psFlag, FLAG_NAMES[] = {
			{ RTF_UP, 'U', 0 },
			{ RTF_HOST, 'H', 0 },
			{ RTF_GATEWAY, 'G', 0 },
			{ RTF_STATIC, 'S', 0 },
			{ RTF_DYNAMIC, 'D', 0 },
			{ 0, 0, 0 }
		};
		
		/* Work out flags */
		for( psFlag = FLAG_NAMES; psFlag->nFlag != 0; ++psFlag )
		{
			if( (nFlags & psFlag->nFlag) != 0 )
			{
				if( psFlag->cFlag != 0 )
					*pcFlag++ = psFlag->cFlag;
			}
			else
			{
				if( psFlag->cNotFlag != 0 )
					*pcFlag++ = psFlag->cNotFlag;
			}
		}
		
		*pcFlag = 0;
		
		/* Count bits in netmask */
		for( nIpAddr = *((unsigned long *)anMask); nIpAddr != 0; nIpAddr >>= 1 )
			nMaskBits += nIpAddr & 1;

		if( nMaskBits == 0 )
			strcpy( acDestBuf, "default" );
		else if( nMaskBits == 32 )
			strcpy( acDestBuf, format_ip( anIp ) );
		else
			sprintf( acDestBuf, "%s/%d", format_ip( anIp ), nMaskBits );

		printf( "%-18s  %-15s  %s\n", acDestBuf, pzGateway, acFlags );
	}
}

static void list_routes( bool bLong )
{
	int nSocket = socket( AF_INET, SOCK_STREAM, 0 );
	struct rttable *psRtTable;
	struct rtabentry *psTable;
	int nError;
	int i;

	psRtTable = malloc( sizeof( struct rttable ) + sizeof( struct rtabentry ) * 128 );

	if ( psRtTable == NULL )
	{
		fprintf( stderr, "Out of memory: %s\n", strerror( errno ) );
		exit( 1 );
	}
	psTable = ( struct rtabentry * )( psRtTable + 1 );

	if ( nSocket < 0 )
	{
		fprintf( stderr, "Failed to create socket: %s\n", strerror( errno ) );
		exit( 1 );
	}
	psRtTable->rtt_count = 128;

	nError = ioctl( nSocket, SIOCGETRTAB, psRtTable );
	if ( nError < 0 )
	{
		fprintf( stderr, "Failed to get routing table: %s\n", strerror( errno ) );
		exit( 1 );
	}
	
	if( bLong )
		printf( "Index  Interface   Destination      Netmask          Remote           Flags\n" );
	else
		printf( "Index  Interface   Destination         Remote           Flags\n" );

	for ( i = 0; i < psRtTable->rtt_count; ++i )
	{
		printf( "%5d  %-10s  ", i, psTable[i].rt_dev );
		print_route( (uint8*)&((struct sockaddr_in *)&psTable[i].rt_dst)->sin_addr,
			(uint8*)&((struct sockaddr_in *)&psTable[i].rt_genmask)->sin_addr,
			(uint8*)&((struct sockaddr_in *)&psTable[i].rt_gateway)->sin_addr,
				        psTable[i].rt_flags, bLong );
	}

	free( psRtTable );
	close( nSocket );
}

void add_route( struct rtentry *psRoute )
{
	int nSocket = socket( AF_INET, SOCK_STREAM, 0 );

	if ( nSocket < 0 )
	{
		fprintf( stderr, "Failed to create socket: %s\n", strerror( errno ) );
		exit( 1 );
	}

	if ( ioctl( nSocket, SIOCADDRT, psRoute ) < 0 )
	{
		fprintf( stderr, "Failed to add route: %s\n", strerror( errno ) );
		exit( 1 );
	}
	close( nSocket );
}

void del_route( struct rtentry *psRoute )
{
	int nSocket = socket( AF_INET, SOCK_STREAM, 0 );

	if ( nSocket < 0 )
	{
		fprintf( stderr, "Failed to create socket: %s\n", strerror( errno ) );
		exit( 1 );
	}

	if ( ioctl( nSocket, SIOCDELRT, psRoute ) < 0 )
	{
		fprintf( stderr, "Failed to delete route: %s\n", strerror( errno ) );
		exit( 1 );
	}
	close( nSocket );
}

int main( int argc, char **argv )
{
	uint8 anAddress[4];
	uint8 anMask[4];
	uint8 anGateway[4] = { 0, 0, 0, 0 };
	const char *pzGateway = NULL, *pzProgram;
	const char *pzCommand, *pzNetAddr, *pzNetMask;
	struct rtentry sRoute;
	bool bLong = false;
	bool bNetSet = false, bMaskSet = false, bGwSet = false;
	int c;
	
	if ( (pzProgram = strrchr( *argv, '/' )) != NULL )
		++pzProgram;
	else
		pzProgram = *argv;

	memset( &sRoute, 0, sizeof( sRoute ) );

	sRoute.rt_metric = 0;
	sRoute.rt_flags = RTF_UP | RTF_STATIC; /* Not necessary */

	while ( ( c = getopt_long( argc, argv, "hlqvi:g:", long_opts, ( int * )0 ) ) != EOF )
	{
		switch ( c )
		{
		case 0:
			break;
		case 'l':
			bLong = true;
			break;
		case 'v':
			g_nShowVersion = true;
			break;
		case 'h':
			g_nShowHelp = true;
			break;
		default:
			usage( pzProgram, 0 );
			return ( EXIT_FAILURE );
		}
	}

	argc -= optind;
	argv += optind;
	
	if ( (pzCommand = *argv++) == NULL )
	{
		usage( pzProgram, 0 );
	
		return ( EXIT_FAILURE );
	}
	
	/* Check command is valid */
	if ( strcmp( pzCommand, "list" ) == 0 )
	{
		list_routes( bLong );
		
		return ( EXIT_SUCCESS );
	}
	else if ( strcmp( pzCommand, "add" ) != 0 && strcmp( pzCommand, "del" ) != 0 )
	{
		usage( pzProgram, 0 );

		return ( EXIT_FAILURE );
	}
	
	
	/* Add or delete a route */
	/* Look for address parameter */
	while ( *argv )
	{
		const char *pzInstruction = *argv++;
    char *pzArg = *argv++;
		
		if ( pzArg == NULL )
		{
			/* No argument */
			usage( pzProgram, 0 );

			return ( EXIT_FAILURE );
		}

		if ( strcmp( pzInstruction, "net" ) == 0 )
		{
			char *pzStr;
			
			if( bNetSet == true )
			{
				usage( pzProgram, 0 );

				return ( EXIT_FAILURE );
			}
			
			if( strcmp( pzArg, "default" ) == 0 )
			{
				bNetSet = true;
				bMaskSet = true;
			}
			else
			{
				if( (pzStr = strchr( pzArg, '/' )) != NULL )
				{
					/* CIDR notation in address parameter */
					int nBits;
					uint32 nIpAddr = 0xFFFFFFFF;
					
					*pzStr++ = 0;
					
					if( (nBits = atoi( pzStr )) < 32 )
						nIpAddr <<= (32 - nBits);

					*((uint32 *)&((struct sockaddr_in *)&sRoute.rt_genmask)->sin_addr) = htonl( nIpAddr );
					bMaskSet = true;
				}
				
				/* Interpret as IP address */
				parse_ipaddress( (uint8*)&((struct sockaddr_in *)&sRoute.rt_dst)->sin_addr, pzArg );
				bNetSet = true;
			}
		}
		else if ( strcmp( pzInstruction, "mask" ) == 0 )
		{
			if( bMaskSet == true )
			{
				usage( pzProgram, 0 );

				return ( EXIT_FAILURE );
			}
			
			parse_ipaddress( (uint8*)&((struct sockaddr_in *)&sRoute.rt_genmask)->sin_addr, pzArg );
			bMaskSet = true;
		}
		else if ( strcmp( pzInstruction, "gw" ) == 0 )
		{
			if( bGwSet == true )
			{
				usage( pzProgram, 0 );

				return ( EXIT_FAILURE );
			}
			
			parse_ipaddress( (uint8*)&((struct sockaddr_in *)&sRoute.rt_gateway)->sin_addr, pzArg );
			sRoute.rt_flags |= RTF_GATEWAY;
			bGwSet = true;
		}
		else
		{
			usage( pzProgram, 0 );

			return ( EXIT_FAILURE );
		}
	}
	
	/* Check for required arguments */
	if( !bNetSet || !bMaskSet )
	{
		usage( pzProgram, 0 );

		return ( EXIT_FAILURE );
	}
	
	/* Perform action */
	if ( strcmp( pzCommand, "add" ) == 0 )
	{
		add_route( &sRoute );
	}
	else if ( strcmp( pzCommand, "del" ) == 0 )
	{
		del_route( &sRoute );
	}
	
	return ( EXIT_SUCCESS );
}
