#include <stdio.h>
#include <atheos/msgport.h>
#include <atheos/threads.h>


int main()
{
    port_info sInfo;

    get_port_info( -1, &sInfo );
    int nError = 1;
    printf( "PortID Owner  Max Cur Priv/Pub  Name\n" );
    for (  ; nError == 1 ; nError = get_next_port_info( &sInfo ) ) {
	printf( "%-06d %-06d %-03d %-03d %s\t%s\n", sInfo.pi_port_id, sInfo.pi_owner, sInfo.pi_max_count, sInfo.pi_cur_count, (sInfo.pi_flags ==  MSG_PORT_PRIVATE) ? "Private" : "Public" , sInfo.pi_name);
    }
}
