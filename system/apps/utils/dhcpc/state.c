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

#include <state.h>
#include <dhcp.h>
#include <debug.h>

#include <atheos/semaphore.h>
#include <posix/errno.h>

int change_state( DHCPSessionInfo_s *info, int new_state )
{
	int error = EOK;

	debug(INFO,__FUNCTION__,"changing state to 0x%.2x\n",new_state);

	lock_semaphore( info->state_lock );

	switch(info->current_state)
	{
		case STATE_NONE:
		{
			if( new_state != STATE_INIT )
			{
				error = EINVAL;
				break;
			}

			info->current_state = new_state;
			break;
		}

		case STATE_INIT:
		{
			if( new_state != STATE_SELECTING )
			{
				error = EINVAL;
				break;
			}

			info->current_state = new_state;
			info->boot_time = time(NULL);
			break;
		}

		case STATE_SELECTING:
		{
			if( ( new_state != STATE_SELECTING ) &&
				( new_state != STATE_REQUESTING ) &&
			    ( new_state != STATE_INIT ) )
			{
				error = EINVAL;
				break;
			}

			info->current_state = new_state;
			break;
		}

		case STATE_REQUESTING:
		{
			if( ( new_state != STATE_BOUND ) &&
				( new_state != STATE_INIT ) )
			{
				error = EINVAL;
				break;
			}

			info->current_state = new_state;
			break;
		}

		case STATE_BOUND:
		{
			if( ( new_state != STATE_RENEWING ) &&
				( new_state != STATE_INIT ) &&
				( new_state != STATE_SHUTDOWN ) )
			{
				error = EINVAL;
				break;
			}

			info->current_state = new_state;
			break;
		}

		case STATE_RENEWING:
		{
			if( ( new_state != STATE_BOUND ) &&
				( new_state != STATE_REBINDING ) &&
				( new_state != STATE_INIT ) )
			{
				error = EINVAL;
				break;
			}

			info->current_state = new_state;
			break;
		}

		case STATE_REBINDING:
		{
			if( ( new_state != STATE_BOUND ) &&
				( new_state != STATE_INIT ) )
			{
				error = EINVAL;
				break;
			}

			info->current_state = new_state;
			break;
		}

		default:
		{
			debug(PANIC,__FUNCTION__,"invalid state 0x%.2x\n",new_state);
			error = EINVAL;
			break;
		}
	}

	unlock_semaphore( info->state_lock );

	if( error != EOK )
		debug(ERROR,__FUNCTION__,"attempted an invalid transition from 0x%.2x to 0x%.2x\n",info->current_state,new_state);

	return( error );
}

int get_state( DHCPSessionInfo_s *info )
{
	return( info->current_state );
}

