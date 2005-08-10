#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>

#include <sys/ioctl.h>
#include <atheos/types.h>
#include <sys/socket.h>
#include <net/nettypes.h>
#include <net/if.h>
#include <netinet/in.h>

/**
 * The name of the executable (determined from argv[0] in main()) for use
 * in error messages.
 */
char *g_pzProgram = NULL;

/**
 * Flags used to store the options selected after command-line processing.
 */
int   g_nShowAll = 0;
int   g_nShowHelp = 0;
int   g_nShowVersion = 0;
int   g_nSocket = -1;


/**
 * Option information for GNU getopt_long().
 */
static struct option const g_LongOpts[] =
{
  { "list",    no_argument, &g_nShowAll,      1},
  { "help",    no_argument, &g_nShowHelp,     1},
  { "version", no_argument, &g_nShowVersion,  1},
  { NULL,      0,           NULL,             0}
};

/**
 * Prototypes
 * Look for regexp '^fn-name' to find definition of function 'fn-name'.
 */
void usage(int nLevel);
void show_interface(struct ifreq *ifr);
void show_all_interfaces(void);
int  ifconfig(int argc, char **argv);
int  parse_ipv4( const char *str, uint8 *addr /* 4 bytes */ );

/**
 * Commands
 * These take a pointer to the remaining command line arguments and return
 * the number of arguments they used, or -1 if an error occurred.
 */
typedef int (*CommandFunc)( struct ifreq *ifr, char **args );

struct Command {
  const char *name;
  CommandFunc handler;
};

typedef struct Command Command;

/**
 * Command handler prototypes.
 */
int activate_if( struct ifreq *ifr, char **args );
int deactivate_if( struct ifreq *ifr, char **args );
int set_inet_address( struct ifreq *ifr, char **args );
int set_netmask( struct ifreq *ifr, char **args );
int set_gateway( struct ifreq *ifr, char **args );
int set_broadcast( struct ifreq *ifr, char **args );
int set_peer( struct ifreq *ifr, char **args );
int set_mtu( struct ifreq *ifr, char **args );


/**
 * Command dispatch table.
 */
Command g_asCommandTable[] = {
  { "up",      activate_if },
  { "down",    deactivate_if },
  { "inet",    set_inet_address },
  { "netmask", set_netmask },
  { "gateway", set_gateway },
  { "broadcast", set_broadcast },
  { "peer",    set_peer },
  { "mtu",     set_mtu },
  { NULL,      NULL }
};


/**
 * Do it =)
 */
int
main( int argc, char** argv ) {
  int   c, result = EXIT_SUCCESS;
    
  /* get program name, without leading pathname */
  if( (g_pzProgram = strrchr(*argv, '/')) )
    g_pzProgram++;
  else
    g_pzProgram = *argv;
    
  /* parse options */
  while( (c = getopt_long (argc, argv, "ahv", g_LongOpts, (int *) 0)) != EOF ) {
    
    switch(c) {
    case 0:
      break;
      
    case 'a':
      g_nShowAll = 1;
      break;
      
    case 'v':
      g_nShowVersion = 1;
      break;
    
    case 'h':
      g_nShowHelp = 1;
      break;
    
    default:
      usage(0);
      return EXIT_FAILURE;
    }
  }
  
  if( g_nShowHelp || g_nShowVersion ) {
    /* user specifically wants help/version information */
    usage(1);
    return EXIT_SUCCESS;
  }
  
  /* decrement argc/increment argv to remove option parameters */
  argc -= optind;
  argv += optind;
  
  /* Will be displaying or adjusting interfaces.
   * Start by allocating a global socket for interface ioctls.
   */
  g_nSocket = socket(AF_INET, SOCK_STREAM, 0);
    
  if( g_nSocket < 0 ) {
    (void)fprintf(stderr,
       "%s: Unable to create socket to browse interfaces\n  (%d: %s)\n",
       g_pzProgram, errno, strerror(errno) );
  
    return EXIT_FAILURE;
  }
  
  if( argc == 0 || g_nShowAll ) {
    show_all_interfaces();
  } else {
    if( ifconfig(argc, argv) < 0 )
      result = EXIT_FAILURE;
  }
  
  /* close the socket */
  close(g_nSocket);
  
  return result;
}


/**
 * Display usage message, with option summary if nLevel > 0.
 */
void
usage(int nLevel )
{
  printf( "Usage: %s [options] [interface [commands...] ]\n",
          g_pzProgram );
  if ( nLevel > 0 ) {
    printf( " Display or adjust the current status of <interface>.\n" );
    printf( " Options:\n");
    printf( "  -a --list             list information for all interfaces\n" );
    printf( "  -v --version          display version information and exit\n" );
    printf( "  -h --help             display this help and exit\n" );
    printf( "\n Commands:\n" );
    printf( "  up                    activate the network interface\n" );
    printf( "  down                  activate the network interface\n" );
    printf( "  inet <addr>           set the interface address to <addr>\n" );
    printf( "  netmask <mask>        set the interface address to <mask>\n" );
    printf( "  gateway <gw>          set the gateway address to <gw>\n" );
    printf( "  broadcast <bcast>     set the broadcast address to <bcast>\n" );
    printf( "  peer <peer>           set the peer address to <peer>\n" );
    printf( "  mtu <mtu>             set the MTU to <mtu> (if supported)\n" );
  } else {
    printf( " Use %s --help for help on options.\n", g_pzProgram);
  }
}

/**
 * Structure and values for interface flags to English mapping.
 */
struct flag_name {
  unsigned short flag;
  const char     *name, *notname;
};

#define IFF_DYNAMIC 0x8000

/* flags are checked in the order below. */
struct flag_name FLAG_NAMES[] = {
  { IFF_UP, "up", "down" },
  { IFF_BROADCAST, "broadcast address", NULL },
  { IFF_DEBUG, "debug", NULL },
  { IFF_LOOPBACK, "loopback", NULL },
  { IFF_POINTOPOINT, "point-to-point", "broadcast" },
  { IFF_DYNAMIC, "dynamic", NULL },
  { IFF_NOTRAILERS, "no trailers", NULL },
  { IFF_RUNNING, "running", "stopped" },
  { IFF_NOARP, "no ARP", "ARP" },
  { IFF_PROMISC, "promiscuous", NULL },
  { IFF_MULTICAST, "multicast", NULL },
  { IFF_ALLMULTI, "promiscuous multicast", NULL },
  { IFF_MASTER, "master", NULL },
  { IFF_SLAVE, "slave", NULL },
  { IFF_PORTSEL, "media selectable", NULL },
  { IFF_AUTOMEDIA, "media autodetected", NULL },
  { 0, NULL, NULL }
};

/**
 * Print verbose, human-readable (sysadmin-readable =) summary of a single
 * attached interface.  The ifreq structure passed is assumed to contain the
 * name and IP address, and additional queries are run to determine other
 * information.
 */
void
show_interface(struct ifreq *ifr) {
  struct ifreq   req;
  uint8          *ip;
  char           local_ip[16],
                 local_mask[16],
                 remote_ip[28];
  int            nErr;
  unsigned short flags;

  /* Be nice and work on a copy */
  memcpy(&req, ifr, sizeof(req));
  
  /* grab interface flags. */

  if( (nErr = ioctl(g_nSocket, SIOCGIFFLAGS, &req)) ) {
    (void)fprintf(stderr,
       "%s: Unable to retrieve interface flags for %s\n (%d: %s)\n",
       g_pzProgram, ifr->ifr_name, errno, strerror(errno));
  } else {
    short comma = 0;
    struct flag_name *check;
    
    flags = req.ifr_flags;

    printf("%-16s: < ", ifr->ifr_name); /* 16 is max name size, in net.h */
    
    check = FLAG_NAMES;

    while(check->flag) {
      if( (check->flag & flags) ) {
        if(comma)
          printf(", ");
        else
          comma = 1;

        printf(check->name);
      } else if( check->notname != NULL ) {
        if(comma)
          printf(", ");
        else
          comma = 1;

        printf(check->notname);
      }

      check++;
    }

    if( ! comma )
      printf("none");

    printf(" >\n");
  }
  
  if( ! (flags & IFF_UP) || ! (flags & IFF_RUNNING) ) {
    printf("\n");
    return;
  }
  
  /* The local IP address was passed in. */
  ip = (uint8 *)&((struct sockaddr_in *)&(req.ifr_addr))->sin_addr;
  
  sprintf(local_ip, "%d.%d.%d.%d", 
         (int)(ip[0]), (int)(ip[1]), (int)(ip[2]), (int)(ip[3]));
  
  if( (nErr = ioctl(g_nSocket, SIOCGIFNETMASK, &req)) ) {
    (void)fprintf(stderr, "%s: Could not retrieve netmask for %s\n (%d: %s)\n",
                  g_pzProgram, ifr->ifr_name, errno, strerror(errno));
    
    strcpy(local_mask, "<n/a>");
  } else {
    sprintf(local_mask, "%d.%d.%d.%d", 
           (int)(ip[0]), (int)(ip[1]), (int)(ip[2]), (int)(ip[3]));
  }

  printf("  IP: %-15s  Netmask: %-15s  ", local_ip, local_mask);

  if( (flags & IFF_BROADCAST) ) {
    if( (nErr = ioctl(g_nSocket, SIOCGIFBRDADDR, &req)) ) {
      (void)fprintf(stderr,
                    "\n%s: Could not retrieve broadcast address for %s\n"
                    " (%d: %s)\n",
                    g_pzProgram, ifr->ifr_name, errno, strerror(errno));
      
      printf( "Broadcast: <n/a>\n" );
    } else {
      printf( "Broadcast: %d.%d.%d.%d\n", 
              (int)(ip[0]), (int)(ip[1]), (int)(ip[2]), (int)(ip[3]) );
    }
  } else if( (flags & IFF_POINTOPOINT) ) {
    if( (nErr = ioctl(g_nSocket, SIOCGIFDSTADDR, &req)) ) {
      (void)fprintf(stderr,
                    "\n%s: Could not retrieve remote peer address for %s\n"
                    " (%d: %s)\n",
                    g_pzProgram, ifr->ifr_name, errno, strerror(errno));
      
      printf( "Peer: <n/a>\n" );
    } else {
      printf( "Peer: %d.%d.%d.%d\n", 
              (int)(ip[0]), (int)(ip[1]), (int)(ip[2]), (int)(ip[3]) );
    }
  } else {
    printf( "Remote: <n/a>\n" );
  }
  
  printf( "  " );
  
  /* Print hardware address, if any */
  if( (flags & IFF_LOOPBACK) == 0 &&
      (nErr = ioctl(g_nSocket, SIOCGIFHWADDR, &req)) == 0 ) {
    uint8 *mac = (uint8 *)(req.ifr_hwaddr.sa_data);

    printf( "Hardware Address: %.2X:%.2X:%.2X:%.2X:%.2X:%.2X            ",
            (uint)mac[0], (uint)mac[1], (uint)mac[2], (uint)mac[3],
            (uint)mac[4], (uint)mac[5] );
  }

  if( (flags & IFF_LOOPBACK) == 0 &&
      (nErr = ioctl(g_nSocket, SIOCGIFMTU, &req)) == 0 ) {
    printf( "MTU: %5d\n", req.ifr_mtu );
  }
  
  printf("\n");
}


/**
 * Enumerate the attached interfaces and display each.
 */
void
show_all_interfaces(void) {
  int nInterfaces, nErr;
  struct ifreq *sIFReqs = NULL;
  struct ifconf sIFConf;

  /* Count attached interfaces. */
  if( (nInterfaces = ioctl(g_nSocket, SIOCGIFCOUNT, NULL)) > 0 ) {
    /* Allocate memory for nInterfaces. */

    sIFReqs = (struct ifreq *)calloc(nInterfaces, sizeof(struct ifreq));

    if( sIFReqs == NULL ) {
      (void)fprintf(stderr,
        "%s: Could not allocate memory to store interface information.\n",
	g_pzProgram);
      
      return;
    }

    /* set up sIFConf */
    sIFConf.ifc_len = nInterfaces * sizeof(struct ifreq);
    sIFConf.ifc_req = sIFReqs;

    /* Get the interface list */
    if( (nErr = ioctl(g_nSocket, SIOCGIFCONF, &sIFConf)) ) {
      (void)fprintf(stderr, 
        "%s: Could not retrieve interface configuration list\n (%d: %s)\n",
	g_pzProgram, errno, strerror(errno));
      
      goto error_exit;
    }

    /* Loop through interfaces */
    while(nInterfaces--) {
      
      show_interface(sIFConf.ifc_req++);
    }
    
    free(sIFReqs);
    sIFReqs = NULL;
  }
  
  /* All went well */
  return;
  
error_exit:
  if(sIFReqs)
    free(sIFReqs);
  
  return;
}


int
ifconfig(int argc, char **argv) {
  struct ifreq sIFReq;
  int          nErr;
  char         *pzIFName;
  
  if(argc == 0)
    return -1;
  
  /* First parameter is interface name */
  pzIFName = *argv++;
  --argc;
  
  
  /* initialise interface information request structure */
  memset(&sIFReq, 0, sizeof(sIFReq));

  strncpy(sIFReq.ifr_name, pzIFName, IFNAMSIZ - 1);
  sIFReq.ifr_name[IFNAMSIZ - 1] = 0;
  
  /* lookup interface information */
  if( (nErr = ioctl(g_nSocket, SIOCGIFADDR, &sIFReq)) ) {
    (void)fprintf(stderr,
       "%s: Could not locate address information for interface \'%s\'.\n"
       " (%d: %s)\n", g_pzProgram, *argv, errno, strerror(errno));
      
    return -1;
  }
  
  if( argc ) {
    /* Process commands */
    while( argc > 0 ) {
      int         result;
      Command     *command;
      
      for( command = g_asCommandTable; command->name != NULL; ++command )
        if( strcmp( command->name, *argv ) == 0 )
          break;
      
      if( command->handler ) {
        --argc; /* take off command name */
        result = (command->handler)( &sIFReq, ++argv );
      } else {
        fprintf( stderr,
                 "%s: Unknown command requested: %s\n", g_pzProgram, *argv );
        result = -1;
      }

      if( result < 0 )
        return -1;
      
      
      argv += result;
      argc -= result;
    }
  } else {
    /* Show interface information */
    show_interface(&sIFReq);
  }

  return 0;
}

/**
 * Read an IP address.
 * Returns the number of components read.
 */
int
parse_ipv4( const char *str, uint8 *addr /* 4 bytes */ ) {
  int i, result, chunks[4];

  result = sscanf( str, "%d.%d.%d.%d",
                   chunks, chunks + 1, chunks + 2, chunks + 3 );
  
  for( i = 0; i < result; ++i )
    addr[i] = (uint8)(chunks[i] & 0xFF);

  for( i = result; i < 4; ++i )
    addr[i] = 0;

  return result;
}


/**
 * Flag set/clear routines.
 */
static int
set_if_flag( struct ifreq *ifr, int flag ) {
  /* get interface flags */
  if( ioctl(g_nSocket, SIOCGIFFLAGS, ifr) < 0 ) {
    fprintf( stderr,
             "%s: Unable to read interface flags for \'%s\'.\n"
             " (%d: %s)\n",
             g_pzProgram, ifr->ifr_name,
             errno, strerror(errno));
    
    return -1;
  }

  ifr->ifr_flags |= flag;
  
  /* set interface flags */
  if( ioctl(g_nSocket, SIOCSIFFLAGS, ifr) < 0 ) {
    fprintf( stderr,
             "%s: Could not change interface flags for \'%s\'.\n"
             " (%d: %s)\n",
             g_pzProgram, ifr->ifr_name,
             errno, strerror(errno));
    
    return -1;
  }
  
  return 0;
}

static int
clear_if_flag( struct ifreq *ifr, int flag ) {
  /* get interface flags */
  if( ioctl(g_nSocket, SIOCGIFFLAGS, ifr) < 0 ) {
    fprintf( stderr,
             "%s: Unable to read interface flags for \'%s\'.\n"
             " (%d: %s)\n",
             g_pzProgram, ifr->ifr_name,
             errno, strerror(errno));
    
    return -1;
  }

  ifr->ifr_flags &= ~flag;
  
  /* set interface flags */
  if( ioctl(g_nSocket, SIOCSIFFLAGS, ifr) < 0 ) {
    fprintf( stderr,
             "%s: Could not change interface flags for \'%s\'.\n"
             " (%d: %s)\n",
             g_pzProgram, ifr->ifr_name,
             errno, strerror(errno));
    
    return -1;
  }
  
  return 0;
}



/**
 * Bring an interface up.
 */
int
activate_if( struct ifreq *ifr, char **args ) {
  if( set_if_flag( ifr, IFF_UP ) < 0 )
    return -1;

  return 0;
}

/**
 * Take an interface down.
 */
int
deactivate_if( struct ifreq *ifr, char **args ) {
  if( clear_if_flag( ifr, IFF_UP ) < 0 )
    return -1;

  return 0;
}

/**
 * Set an interface's IPv4 netmask given a four-element array of uint8.
 */
static int
set_netmask_quad( struct ifreq *ifr, uint8 *mask ) {
  struct sockaddr_in *addr = (struct sockaddr_in *)(&ifr->ifr_addr);
  
  memcpy( &(addr->sin_addr), mask, 4 );
  
  if( ioctl(g_nSocket, SIOCSIFNETMASK, ifr) < 0 ) {
    fprintf( stderr,
             "%s: Could not set netmask for %s.\n"
             " (%d: %s)\n",
             g_pzProgram, ifr->ifr_name,
             errno, strerror(errno) );

    return -1;
  }
  
  return 0;
}

/**
 * Set an interface's IPv4 address.
 * The IPv4 address may be specified alone, or followed by a mask
 * (CIDR format).  e.g. "192.168.0.1" or "192.168.0.1/24".
 */
int
set_inet_address( struct ifreq *ifr, char **args ) {
  struct sockaddr_in *ip;
  char               *mask;
  
  if( *args == NULL ) {
    fprintf( stderr,
             "%s: Usage: inet requires parameter.\n",
             g_pzProgram );

    return -1;
  }

  if( (mask = strchr( *args, '/' )) != NULL )
    *mask++ = 0;
  
  /* interface address being set: parse address from command line */
  memset(&(ifr->ifr_addr), 0, sizeof(struct sockaddr_in));

  ip = (struct sockaddr_in *)&(ifr->ifr_addr);

  ip->sin_family = PF_INET;
  ip->sin_port = 0;
  
  /* set IP address */
  if( parse_ipv4( *args, (uint8*)(&ip->sin_addr) ) < 4 ) {
    fprintf( stderr,
             "%s: %s is not a valid host address.\n",
             g_pzProgram, *args );

    return -1;
  }

  if( ioctl(g_nSocket, SIOCSIFADDR, ifr) < 0 ) {
    fprintf( stderr,
             "%s: Could not set IP address for %s.\n"
             " (%d: %s)\n",
             g_pzProgram, ifr->ifr_name,
             errno, strerror(errno));

    return -1;
  }
  
  /* Check if we were also asked to set the netmask */
  if( mask ) {
    uint8 mask_ip[4], bits, *quad;

    switch( parse_ipv4( mask, mask_ip ) ) {
      case 0:
        fprintf( stderr,
                 "%s: Ignoring invalid mask '%s'\n",
                 g_pzProgram, mask );
        break;

      case 1:
        if( mask_ip[0] <= 32 ) {
          /* Assume it was a CIDR bit count, convert to dotted quad */
          for( bits = mask_ip[0], quad = mask_ip; bits > 8; bits -= 8, ++quad )
            *quad = 0xFF;
        
          /* Set last few bits */
          *quad++ = 0xFF << (8 - bits);
          
          /* Zero left-over chunks */
          while( quad < mask_ip + 4 )
            *quad++ = 0;
        }
        /* drop through intentionally */
        
      default:
        /* Use mask_ip as a dotted quad */
        if( set_netmask_quad( ifr, mask_ip ) < 0 )
          return -1;
        break;
    }
  }
  
  return 1;
}

/**
 * Sets an interface's netmask.
 */
int
set_netmask( struct ifreq *ifr, char **args ) {
  uint8 mask[4];
  
  if( *args == NULL ) {
    fprintf( stderr,
             "%s: Usage: netmask requires parameter.\n",
             g_pzProgram );

    return -1;
  }
  
  if( parse_ipv4( *args, mask ) < 4 ) {
    fprintf( stderr,
             "%s: Invalid netmask '%s'\n",
             g_pzProgram, *args );
    return -1;
  }
  
  if( set_netmask_quad( ifr, mask ) < 0 )
    return -1;
  
  return 1;
}

int
set_gateway( struct ifreq *ifr, char **args ) {
  if( *args == NULL ) {
    fprintf( stderr,
             "%s: Usage: gateway requires parameter.\n",
             g_pzProgram );

    return -1;
  }
  
  fprintf( stderr, "set_gateway not implemented\n" );

  return 1;
}

int
set_broadcast( struct ifreq *ifr, char **args ) {
  if( *args == NULL ) {
    fprintf( stderr,
             "%s: Usage: broadcast requires parameter.\n",
             g_pzProgram );

    return -1;
  }
  
  fprintf( stderr, "set_broadcast not implemented\n" );

  return 1;
}

int
set_peer( struct ifreq *ifr, char **args ) {
  if( *args == NULL ) {
    fprintf( stderr,
             "%s: Usage: peer requires parameter.\n",
             g_pzProgram );

    return -1;
  }
  
  fprintf( stderr, "set_peer not implemented\n" );
  
  return 1;
}

int
set_mtu( struct ifreq *ifr, char **args ) {
  int mtu = 0;
  
  if( *args == NULL ) {
    fprintf( stderr,
             "%s: Usage: mtu requires parameter.\n",
             g_pzProgram );

    return -1;
  }
  
  if( sscanf( *args, "%d", &mtu ) < 1 ) {
    fprintf( stderr,
             "%s: Invalid MTU: %s\n",
             g_pzProgram, *args );
    
    return -1;
  }
  
  ifr->ifr_mtu = mtu;

  if( ioctl(g_nSocket, SIOCSIFMTU, ifr) < 0 ) {
    fprintf( stderr,
             "%s: Could not set MTU for %s.\n"
             " (%d: %s)\n",
             g_pzProgram, ifr->ifr_name,
             errno, strerror(errno));

    return -1;
  }

  return 1;
}

