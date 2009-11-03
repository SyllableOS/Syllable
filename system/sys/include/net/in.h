#ifndef __F_NET_IN_H__
#define __F_NET_IN_H__

#define IP_TOS		1
#define IP_TTL		2
#define IP_HDRINCL	3
#define IP_OPTIONS	4
#define IP_ROUTER_ALERT	5
#define IP_RECVOPTS	6
#define IP_RETOPTS	7
#define IP_PKTINFO	8
#define IP_PKTOPTIONS	9
#define IP_MTU_DISCOVER	10
#define IP_RECVERR	11
#define IP_RECVTTL	12
#define IP_RECVTOS	13
#define IP_MTU		14

/* BSD compatibility */
#define IP_RECVRETOPTS	IP_RETOPTS

/* IP_MTU_DISCOVER values */
#define IP_PMTUDISC_DONT		0	/* Never send DF frames */
#define IP_PMTUDISC_WANT		1	/* Use per route hints	*/
#define IP_PMTUDISC_DO			2	/* Always DF		*/

#define IP_MULTICAST_IF			32
#define IP_MULTICAST_TTL 		33
#define IP_MULTICAST_LOOP 		34
#define IP_ADD_MEMBERSHIP		35
#define IP_DROP_MEMBERSHIP		36

/* These need to appear somewhere around here */
#define IP_DEFAULT_MULTICAST_TTL        1
#define IP_DEFAULT_MULTICAST_LOOP       1

#if 0
/* Request struct for multicast socket ops */
/* Not yet implemented in Syllable */
struct ip_mreq 
{
	struct in_addr imr_multiaddr;	/* IP multicast address of group */
	struct in_addr imr_interface;	/* local IP address of interface */
};

struct ip_mreqn
{
	struct in_addr	imr_multiaddr;		/* IP multicast address of group */
	struct in_addr	imr_address;		/* local IP address of interface */
	int		imr_ifindex;		/* Interface index */
};

struct in_pktinfo
{
	int		ipi_ifindex;
	struct in_addr	ipi_spec_dst;
	struct in_addr	ipi_addr;
};
#endif

#endif	/* __F_NET_IN_H__ */
