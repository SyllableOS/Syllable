// dhcpc : A DHCP client for Syllable
// (C)opyright 2002-2003,2007 Kristian Van Der Vliet
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <dhcp.h>
#include <packet.h>
#include <interface.h>
#include <state.h>
#include <inbound.h>
#include <debug.h>

#include <time.h>
#include <unistd.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>
#include <posix/errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/sockios.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <atheos/semaphore.h>

static int setup_sockets( DHCPSessionInfo_s* info, bool outbound_broadcast );

static void state_init( DHCPSessionInfo_s *info );
static void state_selecting( DHCPSessionInfo_s *info );
static void state_requesting( DHCPSessionInfo_s *info );
static void state_bound( DHCPSessionInfo_s *info );
static void state_renewing( DHCPSessionInfo_s *info );
static void state_rebinding( DHCPSessionInfo_s *info );

static void do_release( DHCPSessionInfo_s *info );
static void sighandle( int signal );

#define MAX_SESSIONS 16
static struct
{
	pid_t pid;
	DHCPSessionInfo_s *session;
} dhcp_sessions[MAX_SESSIONS] = { -1, NULL };

static inline void waitrand( int max )
{
	uint64 i;

	srand(time(0));
	i = rand();

	while( i > max )
		i /= 2;

	sleep(i);
}

void dhcp_start( DHCPSessionInfo_s *info )
{
	debug(INFO,__FUNCTION__,"starting DHCP client\n");

	// Loop until the "shutdown" flag is true
	while(info->do_shutdown==false)
	{
		switch( get_state( info ) )
		{
			case STATE_NONE:
			{
				change_state( info, STATE_INIT );
				break;
			}

			case STATE_INIT:
			{
				if( info->attempts < 5 )
					state_init( info );
				else
				{
					debug(PANIC,__FUNCTION__,"tried more than 5 times, giving up\n");
					info->do_shutdown=true;
				}

				break;
			}

			case STATE_SELECTING:
			{
				state_selecting( info );
				break;
			}

			case STATE_REQUESTING:
			{
				state_requesting( info );
				break;
			}

			case STATE_BOUND:
			{
				state_bound( info );
				break;
			}

			case STATE_RENEWING:
			{
				state_renewing( info );
				break;
			}

			case STATE_REBINDING:
			{
				state_rebinding( info );
				break;
			}

			default:
			{
				debug(PANIC,__FUNCTION__,"unknown state 0x%.2x\n",get_state( info ));
				info->do_shutdown = true;
				break;
			}
		}
	}

	// Gracefully shut down, if we were at a state which requires us to
	// release the IP address
	if( get_state( info ) == STATE_SHUTDOWN)
		do_release( info );

	debug(INFO,__FUNCTION__,"shutting down state machine\n");
}

int dhcp_init( const char *if_name, DHCPSessionInfo_s **session )
{
	struct sigaction sa;
	int n;

	DHCPSessionInfo_s *info = calloc( 1, sizeof( DHCPSessionInfo_s ) );
	if( info == NULL )
	{
		debug(PANIC,__FUNCTION__,"unable to allocate session info\n");
		return(ENOMEM);
	}

	// Fill in some default values
	info->attempts = 0;
	info->lease_time = ( DEFAULT_LEASE_MINS * 60 );
	info->if_name = if_name;
	info->timeout = 10;	// Wait 10 seconds for a response from the server
	info->giaddr = 0;  // Assume to start that we're not being relayed.
	info->hw_type = 1;  // Also assume we're running on Ethernet.  Should change this.

	// Fill in the relevent runtime information
	info->state_lock = create_semaphore( "dhcp_state", 1, 0);
	if( !info->state_lock )
	{
		debug(PANIC,__FUNCTION__,"could not create mutex.\n");
		return(EINVAL);
	}

	// Bring up the interface, if it isn't already up
	if( bringup_interface( info ) != EOK )
	{
		debug(PANIC,__FUNCTION__,"could not bring up interface %s\n",info->if_name);
		return(EINVAL);
	}

	// Find the hardware address of the interface we are being started on
	if( get_mac_address( info ) != EOK )
	{
		debug(PANIC,__FUNCTION__,"could not get hardware address for %s\n",info->if_name);
		return(EINVAL);
	}

	// Create sockets which we can broadcast & receive on
	if( setup_sockets( info, true ) != EOK )
	{
		debug(PANIC,__FUNCTION__,"unable to initialise sockets\n");
		return(EINVAL);
	}

	// Set the initial state of the machine
	info->current_state = STATE_NONE;
	info->do_shutdown = false;

	// Setup signal handlers
	sa.sa_handler = sighandle;
	sigemptyset( &sa.sa_mask );
	sa.sa_flags = 0;

	sigaction( SIGTERM, &sa, NULL );
	sigaction( SIGALRM, &sa, NULL );

	// Seed the RNG
	srand(time(0));

	for( n = 0; n < MAX_SESSIONS; n++ )
		if( dhcp_sessions[n].pid < 0 )
		{
			debug( INFO, __FUNCTION__, "using dhcp_sessions[%d], pid = %d\n", n, getpid() );

			dhcp_sessions[n].pid = getpid();
			dhcp_sessions[n].session = info;
		}

	*session = info;
	return( EOK );
}

static int setup_sockets( DHCPSessionInfo_s *info, bool outbound_broadcast )
{
	// Create & bind an outbound socket and an inbound socket
	memset((char *)&info->out_sin, sizeof(info->out_sin),0);
	info->out_sin.sin_family = AF_INET;

	if( outbound_broadcast == true)
	{
		info->out_sin.sin_addr.s_addr = htonl(INADDR_BROADCAST);	// Send to all stations (Until we
																	// have a server IP from a DHCPOFFER)
	}
	else
		info->out_sin.sin_addr.s_addr = htonl(info->siaddr);		// Send only to the specified server IP address

	info->out_sin.sin_port = htons(REMOTE_PORT);	// 67

	// Build & bind the inbound socket
	info->socket_fd = socket( PF_INET,SOCK_DGRAM,IPPROTO_UDP );

	if( info->socket_fd < 0 )
	{
		debug(PANIC,__FUNCTION__,"could not create socket\n");
		return(EINVAL);
	}

	memset((char *)&info->in_sin, sizeof(info->in_sin),0);
	info->in_sin.sin_family = AF_INET;
	info->in_sin.sin_addr.s_addr = htonl(INADDR_ANY);
	info->in_sin.sin_port = htons(LOCAL_PORT);		// 68

	if (bind(info->socket_fd, (struct sockaddr *)&info->in_sin, sizeof(info->in_sin)) < 0)
	{
		debug(PANIC,__FUNCTION__,"could not bind socket to port %i\n",LOCAL_PORT);
		return(EINVAL);
	}

	// Set SO_BINDTODEVICE option to bind to our interface
	debug( INFO, __FUNCTION__, "forcing packets via interface %s\n", info->if_name );
	if( setsockopt( info->socket_fd, SOL_SOCKET, SO_BINDTODEVICE, info->if_name, strlen( info->if_name ) + 1 ) < 0 )
	{
		debug( INFO, __FUNCTION__, "failed to force packets via interface %s\n", info->if_name);
		close( info->socket_fd );
		return( EINVAL );
	}

	return(EOK);
}

int dhcp_shutdown( DHCPSessionInfo_s *info )
{
	if( info->state_lock )
		delete_semaphore( info->state_lock );

	// Close the sockets
	if( info->socket_fd > 0 )
		close( info->socket_fd );

	return( EOK );
}

// Each state of the DHCP client has a function to associated with it to perform
// the required transactions.

static void state_init( DHCPSessionInfo_s *info )
{
	DHCPPacket_s* packet;
	DHCPOption_s *option, *options_head;
	int error, current_option;
	socklen_t sizeof_out_sin;

	// We build DHCP DHCPDISCOVER packets & broadcast them to 255.255.255.255
	// After the packet is sent we move onto STATE_SELECTING and let that deal
	// with the details
	
	debug(INFO,__FUNCTION__,"lease time is now: %d\n", (unsigned int)info->lease_time);
	packet = build_packet( true, 0, 0, 0, info->if_hwaddress, info->boot_time);
	if( packet == NULL )
	{
		debug(PANIC,__FUNCTION__,"unable to create DHCPDISCOVER packet\n");
		return;	// FIXME: We should return a failure/sucess value
	}

	// Set up & attach a list of options
	// Set up all of the required options
	option = NULL;
	options_head = option;

	for( current_option = 1; current_option <= 5; current_option++ )
	{
		if( option == NULL )
		{
			option = calloc(1, sizeof( DHCPOption_s ) );
			if( option == NULL )
			{
				debug(PANIC,__FUNCTION__,"unable to allocate option data\n");
				free( packet );
				return;
			}

			options_head = option;
		}
		else
		{
			option->next = calloc(1, sizeof( DHCPOption_s ) );
			if( option->next == NULL )
			{
				debug(PANIC,__FUNCTION__,"unable to allocate option data\n");
				free( packet );
				freeoptions( options_head );
				return;
			}

			option = option->next;
		}

		switch( current_option )
		{
			case 1:
			{
				option->type = OPTION_MESSAGE;
				option->value = DHCPDISCOVER;
				break;
			}

			case 2:	// "Hint" at a lease time of an hour...
			{
				option->type = OPTION_LEASETIME;
				option->data = (uint8*)malloc(4);
				if( option->data == NULL )
				{
					debug(PANIC,__FUNCTION__,"unable to allocate data for lease time option\n");
					free( packet );
					freeoptions( options_head );
					return;
				}

				memcpy(option->data,&info->lease_time,4);
				option->next = NULL;
				break;
			}

			case 3:	// Please give us a list of DNS servers, gateways and any NTP servers
			{
				uint8_t dns_val = OPTION_DNSSERVERS;
				uint8_t routers_val = OPTION_ROUTERS;
				uint8_t ntp_val = OPTION_NTPSERVERS;

				option->type = OPTION_PARLIST;
				option->length = 3;
				option->data = (uint8*)malloc(option->length);
				if( option->data == NULL )
				{
					debug(PANIC,__FUNCTION__,"unable to allocate data for paramter list option\n");
					free( packet );
					freeoptions( options_head );
					return;
				}
				memcpy(option->data,&dns_val,1);
				memcpy((option->data + 1),&routers_val,1);
				memcpy((option->data + 2),&ntp_val,2);

				option->next = NULL;
				break;
			}

			case 4:
			{
				option->type = OPTION_CLIENTID;
				option->length = 7;					// FIXME: We assume that we are using Ethernet
				option->data = (uint8*)malloc(7);
				if( option->data == NULL )
				{
					debug(PANIC,__FUNCTION__,"unable to allocate data for client id option\n");
					free( packet );
					freeoptions( options_head );
					return;
				}
				memcpy(option->data,&info->hw_type,1);
				memcpy((option->data + 1),&info->if_hwaddress,6);

				option->next = NULL;
				break;
			}

			case 5: //End option
			{
				option->type = OPTION_END;
				option->length = 1;

				option->next = NULL;
				break;
			}

		}
	}

	// We now have a single linked list of options, pointed to by options_head.  Add
	// the options to the DHCPDISCOVER packet

	setoptions( packet, options_head );
	freeoptions( options_head );

	// Store some relevent details, and then broadcast the packet
	info->last_xid = packet->xid;
	info->last_secs = packet->secs;
	debug(INFO,__FUNCTION__,"xid for this packet is %X\n",(unsigned int)info->last_xid);
	debug(INFO,__FUNCTION__,"secs for this packet is %.0f\n",(double)info->last_secs);

	// Wait between 1 and 4 seconds before we send a packet
	waitrand(4);

	// Broadcast the packet to all stations
	sizeof_out_sin = (socklen_t)sizeof(info->out_sin);
	error = sendto( info->socket_fd, packet, sizeof(DHCPPacket_s), 0, (struct sockaddr*)&info->out_sin, sizeof_out_sin);
	if( error < 0 )
	{
		debug(PANIC,__FUNCTION__,"unable to broadcast DHCPDISCOVER packet\n");
		free( packet );
		info->do_shutdown=true;
		return;
	}

	debug(INFO,__FUNCTION__,"DHCPDISCOVER packet sent.\n");

	// We're done with this packet; free it
	free( packet );

	// Move to the next state and wait for a DHCPOFFER reply from any server
	change_state( info, STATE_SELECTING);
}

static void state_selecting( DHCPSessionInfo_s *info )
{
	DHCPPacket_s *in_packet;
	DHCPOption_s *options, *current_option;
	bool valid;
	uint8* buffer;			// Packet receive buffer
	size_t buffer_length;
	socklen_t sizeof_in_sin;
	int alarm_time;

	in_packet = NULL;
	options = NULL;

	// Wait for a reply packet on UDP port 68.  Note that if more than one
	// server responds, we select the first server (First come, first served!)
	alarm_time = RESPONSE_TIMEOUT;
	sizeof_in_sin = (socklen_t)sizeof(info->in_sin);

	while( info->do_shutdown == false )		// Keep getting data...
	{
		alarm(alarm_time);		// We'll sit in a loop and look at packets (Or just wait
									// for a packet) for a maximum of 1 minute.  We give up after that.

		buffer = malloc(sizeof(DHCPPacket_s));
		buffer_length = recvfrom( info->socket_fd, (void*)buffer, sizeof(DHCPPacket_s), 0, (struct sockaddr*)&info->in_sin, &sizeof_in_sin);

		alarm_time = alarm(0);
		if( alarm_time == 0 )
		{
			debug(PANIC,__FUNCTION__,"timeout after %i seconds waiting for response\n",RESPONSE_TIMEOUT);
			free(buffer);
			info->attempts += 1;
			change_state( info, STATE_INIT );
			return;
		}
		else
			debug(INFO,__FUNCTION__,"%i seconds left to obtain response\n",alarm_time);

		if( (int)buffer_length <= 0 )	// Probably caught SIGTERM
		{
			debug(PANIC,__FUNCTION__,"did not receive data while waiting for response\n");
			free(buffer);
			return;
		}

		debug(INFO,__FUNCTION__,"got a packet of %i bytes\n",buffer_length);

		// Place the buffer into a DHCP packet struct
		in_packet = (DHCPPacket_s*)calloc(1,sizeof(DHCPPacket_s));
		if( in_packet == NULL )
		{
			debug(PANIC,__FUNCTION__,"unable to allocate inbound packet\n");
			free(buffer);
			return;
		}

		memcpy( in_packet, buffer, buffer_length);
		free(buffer);
		buffer = NULL;

		// We now have an inbound packet from someplace, with some data in it.
		// Process the packet.

		if( in_packet->op == BOOT_REPLY )
		{
			debug(INFO,__FUNCTION__,"packet is a BOOT_REPLY\n");
		}
		else
		{
			debug(PANIC,__FUNCTION__,"packet is NOT a BOOT_REPLY\n");
			free(in_packet);
			continue;		// Try for another packet
		}

		// Parse the options field into a format we can more easily handle
		options = parseoptions( in_packet->options, ( buffer_length - 236 ) );

		if( options == NULL )
		{
			debug(PANIC,__FUNCTION__,"no options : this doesn't appear to be a valid reply\n");
			free(in_packet);
			continue;		// Try for another packet
		}

		// Check to see if this packet is correctly destined to us
		if( in_packet->xid != info->last_xid )
		{
			debug(WARNING,__FUNCTION__,"this xid %X does not match last xid %X\n",(unsigned int)in_packet->xid,(unsigned int)info->last_xid);
			free( in_packet );
			continue;		// Try for another packet
		}
		else
		{
			debug(INFO,__FUNCTION__,"this xid %X matchs last xid %X\n",(unsigned int)in_packet->xid,(unsigned int)info->last_xid);
			break;		// Stop getting packets
		}

	}	// End of while() loop

	// See if it is a DHCPOFFER
	current_option = options;
	valid = false;

	do
	{
		if( ( current_option->type == OPTION_MESSAGE ) &&
		    ( current_option->value == DHCPOFFER ) )
		{
			debug(INFO,__FUNCTION__,"got a valid DHCPOFFER with xid %X\n",(unsigned int)in_packet->xid);

			process_dhcpoffer( info, in_packet, options );
			valid = true;
			break;
		}
		else
			current_option = current_option->next;
	}
	while( current_option != NULL );

	freeoptions( options );

	// Now check & decide what to do next
	if( valid == false )
	{
		debug(WARNING,__FUNCTION__,"packet was not a valid DHCPOFFER\n");
		info->attempts += 1;
		change_state( info, STATE_INIT );	// Try again
	}
	else		
		change_state( info, STATE_REQUESTING );
    
	free(in_packet);
}

static void state_requesting( DHCPSessionInfo_s *info )
{
	DHCPPacket_s *packet, *in_packet;
	DHCPOption_s *option, *options_head, *in_options, *current_in_option;
	int current_option, error, reply_type;
	socklen_t sizeof_out_sin, sizeof_in_sin;
	uint8* buffer;			// Packet receive buffer
	size_t buffer_length;
	int alarm_time;

	in_packet = NULL;
	in_options = NULL;

	// Broadcast a DHCPREQUEST, with the previously discovered server identifier
	// & requested IP address options.  The server should reply with either a
	// DHCPACK or DHCPNAK.

	packet = build_packet( true, 0, 0, 0, info->if_hwaddress, info->boot_time);
	if( packet == NULL )
	{
		debug(PANIC,__FUNCTION__,"unable to create a DHCPREQUEST packet\n");
		return;
	}

	packet->secs = info->last_secs;
	info->last_xid = packet->xid;

	// Set up all of the required options
	option = NULL;
	options_head = option;

	for( current_option = 1; current_option <= 6; current_option++ )
	{
		if( option == NULL )
		{
			option = calloc(1, sizeof( DHCPOption_s ) );
			if( option == NULL )
			{
				debug(PANIC,__FUNCTION__,"unable to allocate option data\n");
				free( packet );
				return;
			}

			options_head = option;
		}
		else
		{
			option->next = calloc(1, sizeof( DHCPOption_s ) );
			if( option->next == NULL )
			{
				debug(PANIC,__FUNCTION__,"unable to allocate option data\n");
				free( packet );
				freeoptions( options_head );
				return;
			}

			option = option->next;
		}

		switch( current_option )
		{
			case 1:
			{
				option->type = OPTION_MESSAGE;
				option->value = DHCPREQUEST;
				break;
			}

			case 2:
			{
				char* ip;

				option->type = OPTION_SERVERID;
				option->data = (uint8*)malloc(4);
				if( option->data == NULL )
				{
					debug(PANIC,__FUNCTION__,"unable to allocate data for server id option\n");
					free( packet );
					freeoptions( options_head );
					return;
				}

				memcpy(option->data,&info->siaddr,4);

				ip = format_ip((uint32)*(uint32*)option->data);
				debug(INFO,__FUNCTION__,"requesting from server %s\n",ip);
				free(ip);

				break;
			}

			case 3:
			{
				char* ip;

				option->type = OPTION_REQUESTEDIP;
				option->data = (uint8*)malloc(4);
				if( option->data == NULL )
				{
					debug(PANIC,__FUNCTION__,"unable to allocate data for requested ip option\n");
					free( packet );
					freeoptions( options_head );
					return;
				}

				memcpy(option->data,&info->yiaddr,4);

				ip = format_ip((uint32)*(uint32*)option->data);
				debug(INFO,__FUNCTION__,"requesting ip address %s\n",ip);
				free(ip);

				option->next = NULL;
				break;
			}

			case 4:
			{
				option->type = OPTION_LEASETIME;
				option->data = (uint8*)malloc(4);
				if( option->data == NULL )
				{
					debug(PANIC,__FUNCTION__,"unable to allocate data for lease time option\n");
					free( packet );
					freeoptions( options_head );
					return;
				}

				memcpy(option->data,&info->lease_time,4);
				debug(INFO,__FUNCTION__,"requesting lease time of %i seconds\n",(unsigned int)info->lease_time);
				option->next = NULL;
				break;
			}

			case 5:
			{
				option->type = OPTION_CLIENTID;
				option->length = 7;					// FIXME: We assume that we are using Ethernet
				option->data = (uint8*)malloc(7);
				if( option->data == NULL )
				{
					debug(PANIC,__FUNCTION__,"unable to allocate data for client id option\n");
					free( packet );
					freeoptions( options_head );
					return;
				}
				memcpy(option->data,&info->hw_type,1);
				memcpy((option->data + 1),&info->if_hwaddress,6);

				option->next = NULL;
				break;
			}

			case 6: // End option
			{
				option->type = OPTION_END;
				option->length = 1;

				option->next = NULL;
				break;
			}

		}
	}

	// We now have a single linked list of options, pointed to by options_head.  Add
	// the options to the DHCPREQUEST packet

	setoptions( packet, options_head );
	freeoptions( options_head );

	// Broadcast the packet
	sizeof_out_sin = (socklen_t)sizeof(info->out_sin);
	error = sendto( info->socket_fd, packet, sizeof(DHCPPacket_s), 0, (struct sockaddr*)&info->out_sin, sizeof_out_sin);
	if( error < 0 )
	{
		debug(PANIC,__FUNCTION__,"unable to broadcast DHCPREQUEST packet\n");
		free( packet );
		return;
	}

	debug(INFO,__FUNCTION__,"DHCPREQUEST packet sent.\n");

	// We're done with this packet; free it
	free( packet );

	// Now wait for a DHCPACK response...
	alarm_time = info->timeout;
	sizeof_in_sin = (socklen_t)sizeof(info->in_sin);

	while( info->do_shutdown == false )		// Keep getting data...
	{
		alarm(alarm_time);	// Just in case the server does not respond

		buffer = malloc(sizeof(DHCPPacket_s));
		buffer_length = recvfrom( info->socket_fd, (void*)buffer, sizeof(DHCPPacket_s), 0, (struct sockaddr*)&info->in_sin, &sizeof_in_sin);

		alarm_time = alarm(0);
		if( alarm_time == 0 )
		{
			debug(PANIC,__FUNCTION__,"timeout after %i seconds waiting for response\n",(unsigned int)info->timeout);
			free(buffer);
			info->attempts += 1;
			change_state( info, STATE_INIT );
			return;
		}

		if( (int)buffer_length <= 0 )	// Probably caught SIGTERM
		{
			debug(PANIC,__FUNCTION__,"did not receive data while waiting for response\n");
			free(buffer);
			return;
		}

		debug(INFO,__FUNCTION__,"got a packet of %i bytes\n",buffer_length);

		// Place the buffer into a DHCP packet struct
		in_packet = (DHCPPacket_s*)calloc(1,sizeof(DHCPPacket_s));
		if( in_packet == NULL )
		{
			debug(PANIC,__FUNCTION__,"unable to allocate inbound packet\n");
			free(buffer);
			return;
		}

		memcpy( in_packet, buffer, buffer_length);
		free(buffer);
		buffer=NULL;

		// We now have an inbound packet from someplace, with some data in it.
		// Process the packet.

		if( in_packet->op == BOOT_REPLY )
		{
			debug(INFO,__FUNCTION__,"packet is a BOOT_REPLY\n");
		}
		else
		{
			debug(PANIC,__FUNCTION__,"packet is NOT a BOOT_REPLY\n");
			free(in_packet);
			info->attempts += 1;
			change_state( info, STATE_INIT );
			return;
		}

		// Parse the options field into a format we can more easily handle
		in_options = parseoptions( in_packet->options, ( buffer_length - 236 ) );

		if( in_options == NULL )
		{
			debug(PANIC,__FUNCTION__,"no options : this doesn't appear to be a valid reply\n");
			free(in_packet);
			info->attempts += 1;
			change_state( info, STATE_INIT );
			return;
		}

		// Check to see if this packet is correctly destined to us
		if( in_packet->xid != info->last_xid )
		{
			debug(WARNING,__FUNCTION__,"this xid %X does not match last xid %X\n",(unsigned int)in_packet->xid,(unsigned int)info->last_xid);
			free( in_packet );
			continue;		// Try for another packet.
		}
		else
		{
			debug(INFO,__FUNCTION__,"this xid %X matchs last xid %X\n",(unsigned int)in_packet->xid,(unsigned int)info->last_xid);
			break;	// Stop getting packets
		}

	}	// End of while loop

	// See if it is a DHCPACK
	current_in_option = in_options;
	reply_type = DHCPNOP;

	do
	{
		if( ( current_in_option->type == OPTION_MESSAGE ) &&
		    ( current_in_option->value == DHCPACK ) )
		{
			debug(INFO,__FUNCTION__,"got a valid DHCPACK with xid %X\n",(unsigned int)in_packet->xid);

			process_dhcpack( info, in_packet, in_options );
			reply_type = DHCPACK;
			break;
		}

		if( ( current_in_option->type == OPTION_MESSAGE ) &&
		    ( current_in_option->value == DHCPNAK ) )
		{
			debug(INFO,__FUNCTION__,"got a valid DHCPNAK with xid %X\n",(unsigned int)in_packet->xid);
			process_dhcpnak( info, in_packet, in_options );
			reply_type = DHCPNAK;
			break;
		}

		current_in_option = current_in_option->next;
	}
	while( current_in_option != NULL );

	freeoptions( in_options );

	// Check and decide what to do
	switch( reply_type )
	{
		case DHCPACK:
		{
			change_state( info, STATE_BOUND );
			break;
		}

		case DHCPNAK:
		{
			info->attempts += 1;
			change_state( info, STATE_INIT );
			break;
		}

		case DHCPNOP:
		{
			debug(WARNING,__FUNCTION__,"packet was not a valid DHCPACK or DHCPNAK\n");
			info->attempts += 1;
			change_state( info, STATE_INIT );
			break;
		}
	}

	free(in_packet);
}

static void state_bound( DHCPSessionInfo_s *info )
{
	char* ip;

	// Configure ourselves to match the values the DHCP server has supplied,
	// and wait for the lease to expire.  We can be interupted either when
	// the lease has expired, or by a signal (SIGTERM).

	// FIXME: Check that the IP adress we have been given is not in use
	// (E.g. via. ARP)  Send a DHCPDECLINE if the IP is in use.

	debug(INFO,__FUNCTION__,"configuring interface %s\n",info->if_name);

	// Calculate the timer values
	info->t3_time = info->lease_time;

	if( info->t1_time == 0)
		info->t1_time = info->t3_time * 0.5;

	if( info->t2_time == 0)
		info->t2_time = info->t3_time * 0.875;

	// Configure the network interface with the values we have from the
	// DHCP server (IP address, subnet mask)
	if( setup_interface( info ) != EOK )
	{
		debug(PANIC,__FUNCTION__,"unable to configure interface %s\n",info->if_name);
		info->do_shutdown=true;
		return;
	}

	// Now the interface is configured, we can close the socket
	if( info->socket_fd > 0 )
		close( info->socket_fd );

	// Now we can sleep until either the timer t1 runs out, or a signal (SIGTERM) wakes us up
	info->t3_state = TIMER_RUNNING;
	info->t2_state = TIMER_RUNNING;
	info->t1_state = TIMER_RUNNING;

	// Tell the parent to continue
	kill( getppid(), SIGUSR1 );

	debug(INFO,__FUNCTION__,"sleeping on timer t1\n");
	sleep(info->t1_time);

	// We must have woken up.  If it was because of a signal, then info->t1_state
	// will be TIMER_INTERUPTED
	if( info->t1_state == TIMER_INTERUPTED)
	{
		if( info->do_shutdown == true )
		{
			debug(INFO,__FUNCTION__,"SIGTERM while waiting for timer t1 to expire.  Releasing & shutting down\n");
		}
		else
		{
			debug(PANIC,__FUNCTION__,"something woke us up when waiting for timer t1 to expire, but it was not SIGTERM\n");
			info->do_shutdown = true;
		}

		change_state( info, STATE_SHUTDOWN);
		return;
	}
	else
		info->t1_state = TIMER_FINISHED;

	ip=format_ip(info->yiaddr);
	debug(INFO,__FUNCTION__,"t1 expired, renewing lease for ip %s\n",ip);
	free(ip);

	// Move to STATE_RENEWING
	change_state( info, STATE_RENEWING);
}

static void state_renewing( DHCPSessionInfo_s *info )
{
	// Renew the lease on the IP address we already have.

	DHCPPacket_s *packet, *in_packet;
	DHCPOption_s *option, *options_head, *in_options, *current_in_option;
	int current_option, error, reply_type;
	socklen_t sizeof_out_sin, sizeof_in_sin;
	uint8* buffer;			// Packet receive buffer
	size_t buffer_length;

	in_packet = NULL;
	in_options = NULL;

	// Re-open the socket so that we can send a packet
	if( setup_sockets( info, false) != EOK )
	{
		debug(PANIC,__FUNCTION__,"unable to open sockets\n");
		return;
	}

	// Unicast a DHCPREQUEST, without the requested IP address, server ID or client ID
	// options.  The server should reply with either a DHCPACK or DHCPNAK.

	packet = build_packet( true, info->ciaddr, 0, info->giaddr, info->if_hwaddress, info->boot_time);
	if( packet == NULL )
	{
		debug(PANIC,__FUNCTION__,"unable to create a DHCPREQUEST packet\n");
		return;
	}

	packet->secs = info->last_secs;
	info->last_xid = packet->xid;

	// Set up all of the required options
	option = NULL;
	options_head = option;

	for( current_option = 1; current_option <= 2; current_option++ )
	{
		if( option == NULL )
		{
			option = calloc(1, sizeof( DHCPOption_s ) );
			if( option == NULL )
			{
				debug(PANIC,__FUNCTION__,"unable to allocate option data\n");
				free( packet );
				return;
			}

			options_head = option;
		}
		else
		{
			option->next = calloc(1, sizeof( DHCPOption_s ) );
			if( option->next == NULL )
			{
				debug(PANIC,__FUNCTION__,"unable to allocate option data\n");
				free( packet );
				freeoptions( options_head );
				return;
			}

			option = option->next;
		}

		switch( current_option )
		{
			case 1:
			{
				option->type = OPTION_MESSAGE;
				option->value = DHCPREQUEST;
				break;
			}

			case 2:
			{
				option->type = OPTION_CLIENTID;
				option->length = 6;					// FIXME: We assume that we are using Ethernet
				option->data = (uint8*)malloc(6);
				if( option->data == NULL )
				{
					debug(PANIC,__FUNCTION__,"unable to allocate data for client id option\n");
					free( packet );
					freeoptions( options_head );
					return;
				}

				memcpy(option->data,&info->if_hwaddress,option->length);

				option->next = NULL;
				break;
			}
		}
	}

	// We now have a single linked list of options, pointed to by options_head.  Add
	// the options to the DHCPREQUEST packet

	setoptions( packet, options_head );
	freeoptions( options_head );

	// Unicast the packet & wait for a response, which should be either
	// DHCPACK or DHCPNAK.  If the alarm interupts us before we get a
	// response, then info->t2_state will be TIMER_INTERUPTED

	sizeof_out_sin = (socklen_t)sizeof(info->out_sin);
	error = sendto( info->socket_fd, packet, sizeof(DHCPPacket_s), 0, (struct sockaddr*)&info->out_sin, sizeof_out_sin);
	if( error < 0 )
	{
		debug(PANIC,__FUNCTION__,"unable to send DHCPREQUEST packet\n");
		free( packet );
		return;
	}

	debug(INFO,__FUNCTION__,"DHCPREQUEST packet sent.\n");

	// We're done with this packet; free it
	free( packet );

	// Setup an alarm with timer t2.  If the timer expires before we can
	// renew, then we have to move on to rebind.
	alarm( info->t2_time - info->t1_time );
	info->t2_state = TIMER_RUNNING;

	// Wait for a DHCPACK or DHCPNAK response...
	sizeof_in_sin = (socklen_t)sizeof(info->in_sin);

	while( info->t2_state != TIMER_INTERUPTED )		// Keep getting data...
	{
		buffer = malloc(sizeof(DHCPPacket_s));
		buffer_length = recvfrom( info->socket_fd, (void*)buffer, sizeof(DHCPPacket_s), 0, (struct sockaddr*)&info->in_sin, &sizeof_in_sin);

		// Check to ensure that the alarm did not interupt us before we got a response
		if( info->t2_state == TIMER_INTERUPTED)
		{
			debug(INFO,__FUNCTION__,"timer t2 expired before the server responded\n");
			free( buffer );
			info->attempts += 1;

			// Close the sockets & re-open them for broadcasts
			if( info->socket_fd != 0)
				close( info->socket_fd );

			if( info->socket_fd != 0)
				close( info->socket_fd );

			if( setup_sockets( info, true ) != EOK )
			{
				debug(PANIC,__FUNCTION__,"unable to open sockets\n");
				return;
			}

			change_state( info, STATE_REBINDING);
			return;
		}
		else
			alarm(0);		// Clear the alarm so that it does not bother us

		if( (int)buffer_length <= 0 )	// Probably caught SIGTERM
		{
			debug(PANIC,__FUNCTION__,"did not receive data while waiting for response\n");
			free(buffer);
			return;
		}

		debug(INFO,__FUNCTION__,"got a packet of %i bytes\n",buffer_length);

		// Place the buffer into a DHCP packet struct
		in_packet = (DHCPPacket_s*)calloc(1,sizeof(DHCPPacket_s));
		if( in_packet == NULL )
		{
			debug(PANIC,__FUNCTION__,"unable to allocate inbound packet\n");
			free(buffer);
			return;
		}

		memcpy( in_packet, buffer, buffer_length);
		free(buffer);
		buffer=NULL;

		// We now have an inbound packet from someplace, with some data in it.
		// Process the packet.

		if( in_packet->op == BOOT_REPLY )
		{
			debug(INFO,__FUNCTION__,"packet is a BOOT_REPLY\n");
		}
		else
		{
			debug(PANIC,__FUNCTION__,"packet is NOT a BOOT_REPLY\n");
			free(in_packet);
			info->attempts += 1;

			// Close the sockets & re-open them for broadcasts
			if( info->socket_fd != 0)
				close( info->socket_fd );

			if( info->socket_fd != 0)
				close( info->socket_fd );

			if( setup_sockets( info, true ) != EOK )
			{
				debug(PANIC,__FUNCTION__,"unable to open sockets\n");
				return;
			}

			// Try to reinitialise from scratch
			change_state( info, STATE_INIT);
			return;
		}

		// Parse the options field into a format we can more easily handle
		in_options = parseoptions( in_packet->options, ( buffer_length - 236 ) );

		if( in_options == NULL )
		{
			debug(PANIC,__FUNCTION__,"no options : this doesn't appear to be a valid reply\n");
			free(in_packet);
			info->attempts += 1;
			change_state( info, STATE_INIT);
			return;
		}

		// Check to see if this packet is correctly destined to us
		if( in_packet->xid != info->last_xid )
		{
			debug(WARNING,__FUNCTION__,"this xid %X does not match last xid %X\n",(unsigned int)in_packet->xid,(unsigned int)info->last_xid);
			free( in_packet );
			continue;		// Try for another packet.
		}
		else
		{
			debug(INFO,__FUNCTION__,"this xid %X matchs last xid %X\n",(unsigned int)in_packet->xid,(unsigned int)info->last_xid);
			break;
		}

	};	// End of while loop

	// See if it is a DHCPACK
	current_in_option = in_options;
	reply_type = DHCPNOP;

	do
	{
		if( ( current_in_option->type == OPTION_MESSAGE ) &&
		    ( current_in_option->value == DHCPACK ) )
		{
			debug(INFO,__FUNCTION__,"got a valid DHCPACK with xid %X\n",(unsigned int)in_packet->xid);

			process_dhcpack( info, in_packet, in_options );
			reply_type = DHCPACK;
			break;
		}

		if( ( current_in_option->type == OPTION_MESSAGE ) &&
		    ( current_in_option->value == DHCPNAK ) )
		{
			debug(INFO,__FUNCTION__,"got a valid DHCPNAK with xid %X\n",(unsigned int)in_packet->xid);
			process_dhcpnak( info, in_packet, in_options );
			reply_type = DHCPNAK;
			break;
		}

		current_in_option = current_in_option->next;
	}
	while( current_in_option != NULL );

	freeoptions( in_options );

	// Check and decide what to do
	switch( reply_type )
	{
		case DHCPACK:
		{
			// Close the sockets & re-open them for broadcasts
			if( info->socket_fd != 0)
				close( info->socket_fd );

			if( info->socket_fd != 0)
				close( info->socket_fd );

			if( setup_sockets( info, true ) != EOK )
			{
				debug(PANIC,__FUNCTION__,"unable to open sockets\n");
				return;
			}

			// We are now rebound to the same IP
			change_state( info, STATE_BOUND);
			break;
		}

		case DHCPNOP:
		{
			debug(WARNING,__FUNCTION__,"packet was not a valid DHCPACK or DHCPNAK\n");
			// Fall through
		}

		case DHCPNAK:
		{
			info->attempts += 1;

			// Close the sockets & re-open them for broadcasts
			if( info->socket_fd != 0)
				close( info->socket_fd );

			if( info->socket_fd != 0)
				close( info->socket_fd );

			if( setup_sockets( info, true) != EOK )
			{
				debug(PANIC,__FUNCTION__,"unable to open sockets\n");
				return;
			}

			change_state( info, STATE_INIT);
			break;
		}
	}

	free(in_packet);
}

static void state_rebinding( DHCPSessionInfo_s *info )
{
	// Attempt to renew our lease
	DHCPPacket_s *packet, *in_packet;
	DHCPOption_s *option, *options_head, *in_options, *current_in_option;
	int current_option, error, reply_type;
	socklen_t sizeof_out_sin, sizeof_in_sin;
	uint8* buffer;			// Packet receive buffer
	size_t buffer_length;

	in_packet = NULL;
	in_options = NULL;

	// Broadcast a DHCPREQUEST, without the requested IP address or server ID
	// options.  The server should reply with either a DHCPACK or DHCPNAK.

	packet = build_packet( true, info->ciaddr, 0, info->giaddr, info->if_hwaddress, info->boot_time);
	if( packet == NULL )
	{
		debug(PANIC,__FUNCTION__,"unable to create a DHCPREQUEST packet\n");
		return;
	}

	packet->secs = info->last_secs;
	info->last_xid = packet->xid;

	// Set up all of the required options
	option = NULL;
	options_head = option;

	for( current_option = 1; current_option <= 2; current_option++ )
	{
		if( option == NULL )
		{
			option = calloc(1, sizeof( DHCPOption_s ) );
			if( option == NULL )
			{
				debug(PANIC,__FUNCTION__,"unable to allocate option data\n");
				free( packet );
				return;
			}

			options_head = option;
		}
		else
		{
			option->next = calloc(1, sizeof( DHCPOption_s ) );
			if( option->next == NULL )
			{
				debug(PANIC,__FUNCTION__,"unable to allocate option data\n");
				free( packet );
				freeoptions( options_head );
				return;
			}

			option = option->next;
		}

		switch( current_option )
		{
			case 1:
			{
				option->type = OPTION_MESSAGE;
				option->value = DHCPREQUEST;
				break;
			}

			case 2:
			{
				option->type = OPTION_CLIENTID;
				option->length = 6;					// FIXME: We assume that we are using Ethernet
				option->data = (uint8*)malloc(6);
				if( option->data == NULL )
				{
					debug(PANIC,__FUNCTION__,"unable to allocate data for client id option\n");
					free( packet );
					freeoptions( options_head );
					return;
				}

				memcpy(option->data,&info->if_hwaddress,option->length);

				option->next = NULL;
				break;
			}
		}
	}

	// We now have a single linked list of options, pointed to by options_head.  Add
	// the options to the DHCPREQUEST packet

	setoptions( packet, options_head );
	freeoptions( options_head );

	// Unicast the packet & wait for a response, which should be either
	// DHCPACK or DHCPNAK.  If the alarm interupts us before we get a
	// response, then info->t3_state will be TIMER_INTERUPTED

	sizeof_out_sin = (socklen_t)sizeof(info->out_sin);
	error = sendto( info->socket_fd, packet, sizeof(DHCPPacket_s), 0, (struct sockaddr*)&info->out_sin, sizeof_out_sin);
	if( error < 0 )
	{
		debug(PANIC,__FUNCTION__,"unable to send DHCPREQUEST packet\n");
		free( packet );
		return;
	}

	debug(INFO,__FUNCTION__,"DHCPREQUEST packet sent.\n");

	// We're done with this packet; free it
	free( packet );

	// Setup an alarm with timer t3.  If the timer expires before we can
	// renew, then we have to move on to rebind.
	alarm( info->t3_time - info->t2_time );
	info->t3_state = TIMER_RUNNING;

	// Wait for a DHCPACK or DHCPNAK response...
	sizeof_in_sin = (socklen_t)sizeof(info->in_sin);

	while( info->t3_state != TIMER_INTERUPTED )		// Keep getting data...
	{
		buffer = malloc(sizeof(DHCPPacket_s));
		buffer_length = recvfrom( info->socket_fd, (void*)buffer, sizeof(DHCPPacket_s), 0, (struct sockaddr*)&info->in_sin, &sizeof_in_sin);

		// Check to ensure that the alarm did not interupt us before we got a response
		if( info->t3_state == TIMER_INTERUPTED)
		{
			debug(INFO,__FUNCTION__,"timer t3 expired before the server responded\n");
			free( buffer );
			info->attempts += 1;
			change_state( info, STATE_INIT );
			return;
		}
		else
			alarm(0);		// Clear the alarm so that it does not bother us

		if( (int)buffer_length <= 0 )	// Probably caught SIGTERM
		{
			debug(PANIC,__FUNCTION__,"did not receive data while waiting for response\n");
			free(buffer);
			return;
		}

		debug(INFO,__FUNCTION__,"got a packet of %i bytes\n",buffer_length);

		// Place the buffer into a DHCP packet struct
		in_packet = (DHCPPacket_s*)calloc(1,sizeof(DHCPPacket_s));
		if( in_packet == NULL )
		{
			debug(PANIC,__FUNCTION__,"unable to allocate inbound packet\n");
			free(buffer);
			return;
		}

		memcpy( in_packet, buffer, buffer_length);
		free(buffer);
		buffer = NULL;

		// We now have an inbound packet from someplace, with some data in it.
		// Process the packet.

		if( in_packet->op == BOOT_REPLY )
		{
			debug(INFO,__FUNCTION__,"packet is a BOOT_REPLY\n");
		}
		else
		{
			debug(PANIC,__FUNCTION__,"packet is NOT a BOOT_REPLY\n");
			free(in_packet);
			info->attempts += 1;

			// Close the sockets & re-open them for broadcasts
			if( info->socket_fd != 0)
				close( info->socket_fd );

			if( info->socket_fd != 0)
				close( info->socket_fd );

			if( setup_sockets( info, true ) != EOK )
			{
				debug(PANIC,__FUNCTION__,"unable to open sockets\n");
				return;
			}

			// Try to reinitialise from scratch
			change_state( info, STATE_INIT );
			return;
		}

		// Parse the options field into a format we can more easily handle
		in_options = parseoptions( in_packet->options, ( buffer_length - 236 ) );

		if( in_options == NULL )
		{
			debug(PANIC,__FUNCTION__,"no options : this doesn't appear to be a valid reply\n");
			free(in_packet);
			info->attempts += 1;
			change_state( info, STATE_INIT );
			return;
		}

		// Check to see if this packet is correctly destined to us
		if( in_packet->xid != info->last_xid )
		{
			debug(WARNING,__FUNCTION__,"this xid %X does not match last xid %X\n",(unsigned int)in_packet->xid,(unsigned int)info->last_xid);
			free( in_packet );
			continue;		// Try for another packet.
		}
		else
		{
			debug(INFO,__FUNCTION__,"this xid %X matchs last xid %X\n",(unsigned int)in_packet->xid,(unsigned int)info->last_xid);
			break;
		}

	}	// End of while loop

	// See if it is a DHCPACK
	current_in_option = in_options;
	reply_type = DHCPNOP;

	do
	{
		if( ( current_in_option->type == OPTION_MESSAGE ) &&
		    ( current_in_option->value == DHCPACK ) )
		{
			debug(INFO,__FUNCTION__,"got a valid DHCPACK with xid %X\n",(unsigned int)in_packet->xid);

			process_dhcpack( info, in_packet, in_options );
			reply_type = DHCPACK;
			break;
		}

		if( ( current_in_option->type == OPTION_MESSAGE ) &&
		    ( current_in_option->value == DHCPNAK ) )
		{
			debug(INFO,__FUNCTION__,"got a valid DHCPNAK with xid %X\n",(unsigned int)in_packet->xid);
			process_dhcpnak( info, in_packet, in_options );
			reply_type = DHCPNAK;
			break;
		}

		current_in_option = current_in_option->next;
	}
	while( current_in_option != NULL );

	freeoptions( in_options );

	// Check and decide what to do
	switch( reply_type )
	{
		case DHCPACK:
		{
			// Close the sockets & re-open them for broadcasts
			if( info->socket_fd != 0)
				close( info->socket_fd );

			if( info->socket_fd != 0)
				close( info->socket_fd );

			if( setup_sockets( info, true ) != EOK )
			{
				debug(PANIC,__FUNCTION__,"unable to open sockets\n");
				return;
			}

			// We are now rebound to the same IP
			change_state( info, STATE_BOUND );
			break;
		}

		case DHCPNOP:
		{
			debug(WARNING,__FUNCTION__,"packet was not a valid DHCPACK or DHCPNAK\n");
			// Fall through
		}

		case DHCPNAK:
		{
			info->attempts += 1;

			// Close the sockets & re-open them for broadcasts
			if( info->socket_fd != 0)
				close( info->socket_fd );

			if( info->socket_fd != 0)
				close( info->socket_fd );

			if( setup_sockets( info, true ) != EOK )
			{
				debug(PANIC,__FUNCTION__,"unable to open sockets\n");
				return;
			}

			// Attempt to initialise from scratch
			change_state( info, STATE_INIT );
			break;
		}
	}

	free(in_packet);
}

static void do_release( DHCPSessionInfo_s *info )
{
	DHCPPacket_s *packet;
	DHCPOption_s *option, *options_head;
	int error, current_option;
	socklen_t sizeof_out_sin;
	char* ip;

	// Send a DHCPRELEASE packet to the server
	ip = format_ip(info->yiaddr);
	debug(INFO,__FUNCTION__,"releasing IP address %s\n",ip);
	free(ip);

	// Re-open sockets, sending only to the DHCP server
	if( setup_sockets( info, false ) != EOK )
	{
		debug(PANIC,__FUNCTION__,"unable to open sockets\n");
		return;
	}

	// Build a DHCPRELEASE packet
	packet = build_packet( false, info->siaddr, 0, info->giaddr, info->if_hwaddress, info->boot_time);
	if( packet == NULL )
	{
		debug(PANIC,__FUNCTION__,"unable to create a DHCPRELEASE packet\n");
		return;
	}

	info->last_xid = packet->xid;

	// Set up & attach a list of options
	// Set up all of the required options
	option = NULL;
	options_head = option;

	for( current_option = 1; current_option <= 2; current_option++ )
	{
		if( option == NULL )
		{
			option = calloc(1, sizeof( DHCPOption_s ) );
			if( option == NULL )
			{
				debug(PANIC,__FUNCTION__,"unable to allocate option data\n");
				free( packet );
				return;
			}

			options_head = option;
		}
		else
		{
			option->next = calloc(1, sizeof( DHCPOption_s ) );
			if( option->next == NULL )
			{
				debug(PANIC,__FUNCTION__,"unable to allocate option data\n");
				free( packet );
				freeoptions( options_head );
				return;
			}

			option = option->next;
		}

		switch( current_option )
		{
			case 1:
			{
				option->type = OPTION_MESSAGE;
				option->value = DHCPRELEASE;
				break;
			}

			case 2:
			{
				option->type = OPTION_CLIENTID;
				option->data = (uint8*)malloc(6);
				if( option->data == NULL )
				{
					debug(PANIC,__FUNCTION__,"unable to allocate data for client id option\n");
					free( packet );
					freeoptions( options_head );
					return;
				}

				memcpy(option->data,&info->if_hwaddress,6);

				option->next = NULL;
				break;
			}
		}
	}

	// We now have a single linked list of options, pointed to by options_head.  Add
	// the options to the DHCPRELEASE packet

	setoptions( packet, options_head );
	freeoptions( options_head );

	// Send the packet
	sizeof_out_sin = (socklen_t)sizeof(info->out_sin);
	error = sendto( info->socket_fd, packet, sizeof(DHCPPacket_s), 0, (struct sockaddr*)&info->out_sin, sizeof_out_sin);
	if( error < 0 )
	{
		debug(PANIC,__FUNCTION__,"unable to send DHCPRELEASE packet\n");
		free( packet );
		return;
	}

	debug(INFO,__FUNCTION__,"DHCPRELEASE packet sent.\n");

	// We're done with this packet; free it
	free( packet );

	// There is no need to wait for the server to respond, just quit.
	return;
}

static void sighandle( int signal )
{
	pid_t pid = getpid();
	DHCPSessionInfo_s *session = NULL;
	int n;

	debug( INFO, __FUNCTION__, "received signal %d for process #%d\n", signal, pid );

	for( n = 0; n < MAX_SESSIONS; n++ )
		if( dhcp_sessions[n].pid == pid )
			session = dhcp_sessions[n].session;

	if( session )
	{
		switch( signal )
		{
			case SIGTERM:
			{
				session->do_shutdown=true;

				if( session->t1_state == TIMER_RUNNING )
					session->t1_state = TIMER_INTERUPTED;

				break;
			}

			case SIGALRM:
			{
				if( session->t2_state == TIMER_RUNNING )
					session->t2_state = TIMER_INTERUPTED;

				if( session->t3_state == TIMER_RUNNING )
					session->t3_state = TIMER_INTERUPTED;

				if( ( session->t2_state != TIMER_RUNNING ) &&
					( session->t3_state != TIMER_RUNNING ))
						session->do_shutdown=true;

				break;
			}
		}
	}
	else
		debug( PANIC, __FUNCTION__, "received signal for unknown process #%d!\n", pid );
}
