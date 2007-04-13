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

#include <packet.h>
#include <debug.h>
#include <dhcp.h>

#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <posix/errno.h>

DHCPPacket_s* build_packet( bool is_broadcast, uint32_t client, uint32_t server, uint32_t gateway, uint8_t* hwaddress, clock_t boot_time )
{
	DHCPPacket_s* packet;

	packet = calloc( 1, sizeof( DHCPPacket_s ));
	if( packet == NULL )
	{
		debug(PANIC,__FUNCTION__,"unable to alloc DHCP packet\n");
		return(NULL);
	}

	// Fill in the header details and option feilds
	if( hwaddress != NULL )
		memcpy( packet->chaddr, hwaddress, 6 );	// FIXME: This assumes a hardware address
														// length of 6 bytes, which is correct for
														// Ethernet addresses
	else
	{
		debug(PANIC,__FUNCTION__,"no hardware address specified\n");
		free( packet );
		return( NULL );
	}

	packet->op = BOOT_REQUEST;	// Packets from the client are always requests

	packet->htype = 1;	// FIXME: Again both the htype and the hlen always assume
	packet->hlen = 6;		// that we are using Ethernet.
	packet->hops = 0;		// Always 0 from a DHCP client

	packet->xid = getxid();	// Generate a transaction ID for this packet

	packet->secs = (uint16)( ( clock() - boot_time ) / CLOCKS_PER_SEC );

	if( is_broadcast )
		packet->flags = 0x01;	// Indicate if we are sending a broadcast packet

	if( client != 0 )
		packet->ciaddr = client;	// Only set our IP address if we have one

	if( server != 0 )
		packet->siaddr = server;	// Only send to a specific server if the server is known

	if( gateway !=0 )
		packet->giaddr = gateway;	// Add a BOOTP gateway option, if it exists.
	

	return( packet );
}

uint32_t getxid( void )
{
	return( (uint32_t) rand() );
}

int setoptions( DHCPPacket_s* packet, DHCPOption_s *options )
{
	DHCPOption_s* current_option;	// The current option we are processing from the list
	int offset;						// The current byte within the options field that is unused

	if( packet == NULL )
	{
		debug( PANIC, __FUNCTION__, "packet is NULL\n" );
		return( EINVAL );
	}

	if( options == NULL )
	{
		debug( PANIC, __FUNCTION__, "options is NULL\n" );
		return( EINVAL );
	}

	// Set the magic cookie values
	packet->options[0] = 0x63;
	packet->options[1] = 0x82;
	packet->options[2] = 0x53;
	packet->options[3] = 0x63;

	offset = 4;
	current_option = options;

	// Add each option to the packet
	do
	{
		if( current_option == NULL )
			break;

		debug(INFO,__FUNCTION__,"type = 0x%.2x\n",current_option->type);
		debug(INFO,__FUNCTION__,"value = %i\n",current_option->value);

		switch( current_option->type )
		{
			case OPTION_REQUESTEDIP:
			{
				packet->options[offset] = OPTION_REQUESTEDIP;
				packet->options[offset+1] = 4;
				memcpy(packet->options + (offset + 2), current_option->data, 4);

				offset+=6;
				break;
			}

			case OPTION_LEASETIME:
			{
				packet->options[offset] = OPTION_LEASETIME;
				packet->options[offset+1] = 4;
				memcpy(packet->options + (offset + 2), current_option->data, 4);

				offset+=6;
				break;
			}

			case OPTION_MESSAGE:
			{
				packet->options[offset] = OPTION_MESSAGE;
				packet->options[offset+1] = 1;
				packet->options[offset+2] = current_option->value;

				offset+=3;
				break;
			}

			case OPTION_SERVERID:
			{
				packet->options[offset] = OPTION_SERVERID;
				packet->options[offset+1] = 4;
				memcpy(packet->options + (offset + 2), current_option->data, 4);

				offset+=6;
				break;
			}

			case OPTION_CLIENTID:
			{
				packet->options[offset] = OPTION_CLIENTID;
				packet->options[offset+1] = current_option->length;
				memcpy(packet->options + (offset + 2), current_option->data, current_option->length);

				offset+=(2+current_option->length);
				break;
			}

			case OPTION_PARLIST:
			{
				packet->options[offset] = OPTION_PARLIST;
				packet->options[offset+1] = current_option->length;
				memcpy(packet->options + (offset + 2), current_option->data, current_option->length);

				offset+=(2+current_option->length);
				break;
			}

			case OPTION_END:
			{
				packet->options[offset] = OPTION_END;

				offset+=1;
				break;
			}

			default:
			{
				debug(WARNING,__FUNCTION__,"unknown option type 0x%.2x\n", current_option->type);
				continue;
			}
		}
		// Move onto the next option entry and process it
		current_option = current_option->next;
	}
	while( current_option != NULL );

#ifdef PKT_TRACE
	{
		int i;

		debug(INFO,"","packet options dump follows\n");
		debug(INFO,"","===========================\n");
		for(i=0; i<312; i+=6)
		{
			debug(INFO,"","%.2x %.2x %.2x %.2x %.2x %.2x\n",packet->options[i],packet->options[i+1],packet->options[i+2],packet->options[i+3],packet->options[i+4],packet->options[i+5]);
		}
		debug(INFO,"","===========================\n");
	}
#endif

	return( EOK );
}

DHCPOption_s* parseoptions( uint8_t* buffer, size_t len )
{
	uint8_t offset;
	DHCPOption_s *option, *option_head;

	// Parse the options buffer into a single linked list of DHCPOption_s structs
	// and return it to the caller

	if( buffer == NULL )
	{
		debug(WARNING,__FUNCTION__,"buffer is NULL\n");
		return( NULL );
	}

	// Look for the magic cookie sequence 0x63, 0x82, 0x53, 0x63
	for( offset=0; offset < len; offset++ )
	{
		if( buffer[offset] == 0x63 )
		{
			if( buffer[offset+1] == 0x82 &&
			    buffer[offset+2] == 0x53 &&
			    buffer[offset+3] == 0x63 )
			{
				offset+=4;
				break;
			}
		}
	}

	if( offset == len )
	{
		debug(WARNING,__FUNCTION__,"did not find magic cookie sequence in buffer\n");
		return( NULL );
	}
	else
		debug(INFO,__FUNCTION__,"found end of magic cookie sequence at offset %i\n",offset);

	option = NULL;
	option_head = NULL;

#ifdef PKT_TRACE
	{
		int i;

		debug(INFO,"","packet options dump follows\n");
		debug(INFO,"","===========================\n");
		for(i=0; i<312; i+=6)
		{
			debug(INFO,"","%.2x %.2x %.2x %.2x %.2x %.2x\n",buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5]);
		}
		debug(INFO,"","===========================\n");
	}
#endif

	// Look through the buffer and pull out each option in turn
	for( ; offset < len; offset++ )
	{
		if( buffer[offset] == OPTION_NONE )
			continue;

		if( buffer[offset] == OPTION_END )
			break;

		// Allocate space for the option
		if( option == NULL )
		{
			option = calloc(1, sizeof( DHCPOption_s ) );
			option_head = option;
		}
		else
		{
			option->next = calloc(1, sizeof( DHCPOption_s ) );
			option = option->next;

		}

		option->type = buffer[offset];
		if( buffer[offset+1] == 1 )		// If this option is 1 byte long
		{
			option->length = 1;
			option->value = buffer[offset+2];
			option->data = NULL;
			option->next = NULL;

			offset+=2;		// Offset will move to start of next
							// option in the offset++ of this for() loop
		}
		else
		{
			option->length = buffer[offset+1];
			option->data = (uint8*)malloc(option->length);
			memcpy( option->data, (buffer + (offset + 2 ) ), option->length);

#ifdef PKT_TRACE
			debug(INFO,"","buffer dump for option type 0x%.2x follows\n",option->type);
			debug(INFO,"","===========================\n");
			debug(INFO,"","0x%.2x 0x%.2x 0x%.2x 0x%.2x\n",option->data[0],option->data[1],option->data[2],option->data[3]);
			debug(INFO,"","\n");
#endif
			option->next = NULL;

			offset+=(1 + option->length);	// Must remember that the for() loop
												// will do option++, too
		}
	}

	// Return the DHCPOption_s* as the head of a single linked list
	return( option_head );
}

int freeoptions( DHCPOption_s* options )
{
	DHCPOption_s *current_option, *next_option;

	// Walk the single linked list of options and free all data associated with it
	if( options == NULL )
	{
		debug(WARNING,__FUNCTION__,"options is NULL\n");
		return( EINVAL );
	}

	current_option = options;
	next_option = NULL;

	do
	{
		if( current_option->data != NULL )
		{
			debug(INFO,__FUNCTION__,"freeing option data\n");
			free( current_option->data );
		}

		if( current_option->next != NULL )
		{
			next_option = current_option->next;
			free( current_option );
			current_option = next_option;
		}
		else
			break;

	}
	while( current_option != NULL );

	return( EOK );
}

