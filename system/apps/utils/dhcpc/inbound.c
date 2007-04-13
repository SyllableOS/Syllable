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

#include <inbound.h>
#include <dhcp.h>
#include <state.h>
#include <interface.h>
#include <debug.h>

#include <string.h>

void process_dhcpoffer( DHCPSessionInfo_s *info, DHCPPacket_s *in_packet, DHCPOption_s *options )
{
	DHCPOption_s *option;
	option = options;

	// Copy out relevent information from packet to info struct

	// The DHCP server IP address, if it is supplied in the siaddr
	// field (Otherwise it should be supplied as a server id option)
	if( in_packet->siaddr != 0 )
	{
		char* ip = format_ip(in_packet->siaddr);

		memcpy(&info->siaddr,&in_packet->siaddr,4);
		debug(INFO,__FUNCTION__,"server IP is %s\n",ip);
		free(ip);

	}

	// The IP address which is being offered to us
	if( in_packet->yiaddr != 0 )
	{
		char* ip = format_ip(in_packet->yiaddr);

		memcpy(&info->yiaddr,&in_packet->yiaddr,4);
		debug(INFO,__FUNCTION__,"offered IP is %s\n",ip);
		free(ip);
	}

	// The relay agent address, if one is used on this network
	if( in_packet->giaddr != 0 )
	{
		char* ip = format_ip(in_packet->giaddr);

		memcpy(&info->giaddr,&in_packet->giaddr,4);
		debug(INFO,__FUNCTION__,"DHCP relay agent IP is %s\n",ip);
		free(ip);
	}

	// The server name, if there is one
	if( strcmp(in_packet->sname,"") != 0)
	{
		memcpy(&info->sname,&in_packet->sname,64);
		debug(INFO,__FUNCTION__,"server name is %s\n",info->sname);
	}

	// Now get the subnet mask, server identifier & the lease time
	// from the supplied options
	do
	{
		switch( option->type )
		{
			case OPTION_MESSAGE:
				break;

			case OPTION_SUBNET:
			{
				if( option->length != 4 )
					debug(WARNING,__FUNCTION__,"subnet mask field is NOT 4 bytes long\n");

				if( option->data != NULL )
				{
					char* mask;

					memcpy(&info->subnetmask,option->data,4);

					mask = format_ip(info->subnetmask);
					debug(INFO,__FUNCTION__,"subnet mask is %s\n",mask);
					free(mask);
				}
				else
					debug(WARNING,__FUNCTION__,"subnet mask option data is NULL\n");

				break;

			}

			case OPTION_SERVERID:
			{
				if( option->length != 4 )
					debug(WARNING,__FUNCTION__,"server id field is NOT 4 bytes long\n");

				if( option->data != NULL )
				{
					char* ip;

					memcpy(&info->siaddr,option->data,4);

					ip = format_ip(info->siaddr);
					debug(INFO,__FUNCTION__,"server id is %s\n",ip);
					free(ip);
				}
				else
					debug(WARNING,__FUNCTION__,"server id option data is NULL\n");

				break;
			}

			case OPTION_LEASETIME:
			{
				if( option->length != 4 )
					debug(WARNING,__FUNCTION__,"lease time field is NOT 4 bytes long\n");

				if( option->data != NULL )
				{
					memcpy(&info->lease_time,option->data,4);
					debug(INFO,__FUNCTION__,"lease time is %u\n",(unsigned int)info->lease_time);
				}
				else
					debug(WARNING,__FUNCTION__,"lease time option data is NULL\n");

				break;
			}

			case OPTION_DNSSERVERS:
			{
				int servers;

				servers = (option->length / 4);		// Must always be a multiple of 4
				info->dns_server_count = servers;

				if( option->data != NULL )
				{
					int current_server;

					info->dns_servers = (uint32*)malloc(servers * sizeof(uint32*));
					if( info->dns_servers == NULL )
					{
						debug(PANIC,__FUNCTION__,"could not allocate space for DNS server entries\n");
						break;
					}

					for( current_server=0; current_server < servers; current_server++)
						memcpy(&info->dns_servers[current_server],&option->data[current_server*4],4);

					for( current_server=0; current_server < servers; current_server++)
					{
						char* ip;
						ip = format_ip(info->dns_servers[current_server]);
						debug(INFO,__FUNCTION__,"got DNS server entry %s\n",ip);
						free(ip);
					}
				}
				else
					debug(WARNING,__FUNCTION__,"DNS servers option data is NULL\n");

				break;
			}

			case OPTION_ROUTERS:
			{
				int routers;

				routers = (option->length / 4);		// Must always be a multiple of 4
				info->router_count = routers;

				if( option->data != NULL )
				{
					int current_router;

					info->routers = (uint32*)malloc(routers * sizeof(uint32*));
					if( info->routers == NULL )
					{
						debug(PANIC,__FUNCTION__,"could not allocate space for router entries\n");
						break;
					}

					for( current_router=0; current_router < routers; current_router++)
						memcpy(&info->routers[current_router],&option->data[current_router*4],4);

					for( current_router=0; current_router < routers; current_router++)
					{
						char* ip;
						ip = format_ip(info->routers[current_router]);
						debug(INFO,__FUNCTION__,"got router entry %s\n",ip);
						free(ip);
					}
				}
				else
					debug(WARNING,__FUNCTION__,"router option data is NULL\n");

				break;
			}

			case OPTION_NTPSERVERS:
			{
				int servers;

				servers = (option->length / 4);		// Must always be a multiple of 4
				info->ntp_server_count = servers;

				if( option->data != NULL )
				{
					int current_server;

					info->ntp_servers = (uint32_t*)malloc(servers * sizeof(uint32_t*));
					if( info->ntp_servers == NULL )
					{
						debug(PANIC,__FUNCTION__,"could not allocate space for NTP server entries\n");
						break;
					}

					for( current_server=0; current_server < servers; current_server++)
						memcpy(&info->ntp_servers[current_server],&option->data[current_server*4],4);

					for( current_server=0; current_server < servers; current_server++)
					{
						char* ip;
						ip = format_ip(info->ntp_servers[current_server]);
						debug(INFO,__FUNCTION__,"got NTP server entry %s\n",ip);
						free(ip);
					}
				}
				else
					debug(WARNING,__FUNCTION__,"NTP servers option data is NULL\n");

				break;
			}


			default:	// This is an option that we do not support (Or do not care about)
			{
				debug(INFO,__FUNCTION__,"reply has an extra option id %.2i\n",option->type);
				break;
			}
		}

		// Move onto the next option entry and process it
		option = option->next;
	}
	while( option != NULL );
}

void process_dhcpack( DHCPSessionInfo_s *info, DHCPPacket_s *in_packet, DHCPOption_s *options )
{
	DHCPOption_s *option;
	option = options;

	// Copy out relevent information from packet to info struct
	// The IP address which is being offered to us
	if( in_packet->yiaddr != 0 )
	{
		char* ip = format_ip(in_packet->yiaddr);

		memcpy(&info->yiaddr,&in_packet->yiaddr,4);
		debug(INFO,__FUNCTION__,"our IP is %s\n",ip);
		free(ip);
	}

	// The server name, if there is one
	if( strcmp(in_packet->sname,"") != 0)
	{
		memcpy(&info->sname,&in_packet->sname,64);
		debug(INFO,__FUNCTION__,"server name is %s\n",info->sname);
	}

	// Now get the lease time etc. options from the supplied options
	do
	{
		switch( option->type )
		{
			case OPTION_MESSAGE:
				break;

			case OPTION_SUBNET:
			{
				if( option->length != 4 )
					debug(WARNING,__FUNCTION__,"subnet mask field is NOT 4 bytes long\n");

				if( option->data != NULL )
				{
					char* mask;

					memcpy(&info->subnetmask,option->data,4);

					mask = format_ip(info->subnetmask);
					debug(INFO,__FUNCTION__,"subnet mask is %s\n",mask);
					free(mask);
				}
				else
					debug(WARNING,__FUNCTION__,"subnet mask option data is NULL\n");

				break;

			}

			case OPTION_SERVERID:
			{
				if( option->length != 4 )
					debug(WARNING,__FUNCTION__,"server id field is NOT 4 bytes long\n");

				if( option->data != NULL )
				{
					char* ip;

					memcpy(&info->siaddr,option->data,4);

					ip = format_ip(info->siaddr);
					debug(INFO,__FUNCTION__,"server id is %s\n",ip);
					free(ip);
				}
				else
					debug(WARNING,__FUNCTION__,"server id option data is NULL\n");

				break;
			}

			case OPTION_LEASETIME:
			{
				if( option->length != 4 )
					debug(WARNING,__FUNCTION__,"lease time field is NOT 4 bytes long\n");

				if( option->data != NULL )
				{
					memcpy(&info->lease_time,option->data,4);
					debug(INFO,__FUNCTION__,"lease time is %u\n",(unsigned int)info->lease_time);
				}
				else
					debug(WARNING,__FUNCTION__,"lease time option data is NULL\n");

				break;
			}

			case OPTION_T1_TIME:
			{
				if( option->length != 4 )
					debug(WARNING,__FUNCTION__,"t1 time field is NOT 4 bytes long\n");

				if( option->data != NULL )
				{
					memcpy(&info->t1_time,option->data,4);

					debug(INFO,__FUNCTION__,"t1 time is %i\n",(unsigned int)info->t1_time);
				}
				else
					debug(WARNING,__FUNCTION__,"t1 time option data is NULL\n");

				break;
			}

			case OPTION_T2_TIME:
			{
				if( option->length != 4 )
					debug(WARNING,__FUNCTION__,"t2 time field is NOT 4 bytes long\n");

				if( option->data != NULL )
				{
					memcpy(&info->t2_time,option->data,4);

					debug(INFO,__FUNCTION__,"t2 time is %i\n",(unsigned int)info->t2_time);
				}
				else
					debug(WARNING,__FUNCTION__,"t2 time option data is NULL\n");

				break;
			}

			default:	// This is an option that we do not support (Or do not care about)
			{
				debug(INFO,__FUNCTION__,"reply has an extra option id %.2i\n",option->type);
				break;
			}
		}

		// Move onto the next option entry and process it
		option = option->next;
	}
	while( option != NULL );
}

void process_dhcpnak( DHCPSessionInfo_s *info, DHCPPacket_s *in_packet, DHCPOption_s *options )
{
	DHCPOption_s *option;
	option = options;

	// Look for an error message in the options field
	do
	{
		switch( option->type )
		{
			case OPTION_MESSAGE:
				break;

			case OPTION_SERVER_MESSAGE:
			{
				if( option->data != NULL )
				{
					uint8* buffer;
					buffer = (uint8*)malloc(option->length);

					memcpy(buffer,option->data,option->length);

					debug(WARNING,__FUNCTION__,"NAK message from server is \"%s\"\n",(char*)buffer);
					free(buffer);

				}
				else
					debug(WARNING,__FUNCTION__,"server message option data is NULL\n");

				break;
			}

			default:	// This is an option that we do not support (Or do not care about)
			{
				debug(INFO,__FUNCTION__,"reply has an extra option id %.2i\n",option->type);
				break;
			}
		}

		// Move onto the next option entry and process it
		option = option->next;
	}
	while( option != NULL );
}

