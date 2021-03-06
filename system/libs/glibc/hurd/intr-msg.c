/* Replacement for mach_msg used in interruptible Hurd RPCs.
   Copyright (C) 95, 96, 97, 98, 99 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include <mach.h>
#include <mach/mig_errors.h>
#include <mach/mig_support.h>
#include <hurd/signal.h>
#include <assert.h>

#include "intr-msg.h"


error_t
_hurd_intr_rpc_mach_msg (mach_msg_header_t *msg,
			 mach_msg_option_t option,
			 mach_msg_size_t send_size,
			 mach_msg_size_t rcv_size,
			 mach_port_t rcv_name,
			 mach_msg_timeout_t timeout,
			 mach_port_t notify)
{
  error_t err;
  struct hurd_sigstate *ss;
  const mach_msg_option_t user_option = option;
  const mach_msg_timeout_t user_timeout = timeout;

  struct clobber { int i[2]; };
  union msg
  {
    mach_msg_header_t header;
    mig_reply_header_t reply;
    struct
    {
      mach_msg_header_t header;
      int type;
      int code;
    } check;
    struct
    {
      mach_msg_header_t header;
      struct clobber data;
    } request;
  };
  union msg *const m = (void *) msg;
  mach_msg_bits_t msgh_bits;
  mach_port_t remote_port;
  mach_msg_id_t msgid;
  struct clobber save_data;

  if ((option & (MACH_SEND_MSG|MACH_RCV_MSG)) != (MACH_SEND_MSG|MACH_RCV_MSG)
      || _hurd_msgport_thread == MACH_PORT_NULL)
    {
      /* Either this is not an RPC (i.e., only a send or only a receive),
	 so it can't be interruptible; or, the signal thread is not set up
	 yet, so we cannot do the normal signal magic.  Do a normal,
	 uninterruptible mach_msg call instead.  */
      return __mach_msg (&m->header, option, send_size, rcv_size, rcv_name,
			 timeout, notify);
    }

  ss = _hurd_self_sigstate ();

  /* Save state that gets clobbered by an EINTR reply message.
     We will need to restore it if we want to retry the RPC.  */
  msgh_bits = m->header.msgh_bits;
  remote_port = m->header.msgh_remote_port;
  msgid = m->header.msgh_id;
  assert (rcv_size >= sizeof m->request);
  save_data = m->request.data;

  /* Tell the signal thread that we are doing an interruptible RPC on
     this port.  If we get a signal and should return EINTR, the signal
     thread will set this variable to MACH_PORT_NULL.  The RPC might
     return EINTR when some other thread gets a signal, in which case we
     want to restart our call.  */
  ss->intr_port = m->header.msgh_remote_port;

  /* A signal may arrive here, after intr_port is set, but before the
     mach_msg system call.  The signal handler might do an interruptible
     RPC, and clobber intr_port; then it would not be set properly when we
     actually did send the RPC, and a later signal wouldn't interrupt that
     RPC.  So, _hurd_setup_sighandler saves intr_port in the sigcontext,
     and sigreturn restores it.  */

 message:

  /* XXX
     At all points here (once SS->intr_port is set), the signal thread
     thinks we are "about to enter the syscall", and might mutate our
     return-value register.  This is bogus.
   */

  if (ss->cancel)
    {
      /* We have been cancelled.  Don't do an RPC at all.  */
      ss->intr_port = MACH_PORT_NULL;
      ss->cancel = 0;
      return EINTR;
    }

  /* Note that the signal trampoline code might modify our OPTION!  */
  err = INTR_MSG_TRAP (msg, option, send_size,
		       rcv_size, rcv_name, timeout, notify);

  switch (err)
    {
    case MACH_RCV_TIMED_OUT:
      if (user_option & MACH_RCV_TIMEOUT)
	/* The real user RPC timed out.  */
	break;
      else
	/* The operation was supposedly interrupted, but still has
	   not returned.  Declare it interrupted.  */
	goto interrupted;

    case MACH_SEND_INTERRUPTED: /* RPC didn't get out.  */
      if (!(option & MACH_SEND_MSG))
	{
	  /* Oh yes, it did!  Since we were not doing a message send,
	     this return code cannot have come from the kernel!
	     Instead, it was the signal thread mutating our state to tell
	     us not to enter this RPC.  However, we are already in the receive!
	     Since the signal thread thought we weren't in the RPC yet,
	     it didn't do an interrupt_operation.
	     XXX */
	  goto retry_receive;
	}
      /* FALLTHROUGH */

    case EINTR:
      /* Either the process was stopped and continued,
	 or the server doesn't support interrupt_operation.  */
      if (ss->intr_port != MACH_PORT_NULL)
	/* If this signal was for us and it should interrupt calls, the
	   signal thread will have cleared SS->intr_port.
	   Since it's not cleared, the signal was for another thread,
	   or SA_RESTART is set.  Restart the interrupted call.  */
	{
	  /* Make sure we have a valid reply port.  The one we were using
	     may have been destroyed by interruption.  */
	  m->header.msgh_local_port = rcv_name = __mig_get_reply_port ();
	  m->header.msgh_bits = msgh_bits;
	  option = user_option;
	  timeout = user_timeout;
	  goto message;
	}
      /* FALLTHROUGH */

    case MACH_RCV_PORT_DIED:
      /* Server didn't respond to interrupt_operation,
	 so the signal thread destroyed the reply port.  */
      /* FALLTHROUGH */

    interrupted:
      err = EINTR;

      /* The EINTR return indicates cancellation, so clear the flag.  */
      ss->cancel = 0;
      break;

    case MACH_RCV_INTERRUPTED:	/* RPC sent; no reply.  */
      option &= ~MACH_SEND_MSG;	/* Don't send again.  */
    retry_receive:
      if (ss->intr_port == MACH_PORT_NULL)
	{
	  /* This signal or cancellation was for us.  We need to wait for
             the reply, but not hang forever.  */
	  option |= MACH_RCV_TIMEOUT;
	  /* Never decrease the user's timeout.  */
	  if (!(user_option & MACH_RCV_TIMEOUT)
	      || timeout > _hurd_interrupted_rpc_timeout)
	    timeout = _hurd_interrupted_rpc_timeout;
	}
      else
	{
	  option = user_option;
	  timeout = user_timeout;
	}
      goto message;		/* Retry the receive.  */

    case MACH_MSG_SUCCESS:
      {
	/* We got a reply.  Was it EINTR?  */
	const union
	{
	  mach_msg_type_t t;
	  int i;
	} check =
	  { t: { MACH_MSG_TYPE_INTEGER_T, sizeof (integer_t) * 8,
		 1, TRUE, FALSE, FALSE, 0 } };

        if (m->reply.RetCode == EINTR &&
	    m->header.msgh_size == sizeof m->reply &&
	    m->check.type == check.i &&
	    !(m->header.msgh_bits & MACH_MSGH_BITS_COMPLEX))
	  {
	    /* It is indeed EINTR.  Is the interrupt for us?  */
	    if (ss->intr_port != MACH_PORT_NULL)
	      {
		/* Nope; repeat the RPC.
		   XXX Resources moved? */

		assert (m->header.msgh_id == msgid + 100);

		/* We know we have a valid reply port, because we just
		   received the EINTR reply on it.  Restore it and the
		   other fields in the message header needed for send,
		   since the header now reflects receipt of the reply.  */
		m->header.msgh_local_port = rcv_name;
		m->header.msgh_remote_port = remote_port;
		m->header.msgh_id = msgid;
		m->header.msgh_bits = msgh_bits;
		/* Restore the two words clobbered by the reply data.  */
		m->request.data = save_data;

		/* Restore the original mach_msg options.
		   OPTION may have had MACH_RCV_TIMEOUT added,
		   and/or MACH_SEND_MSG removed.  */
		option = user_option;
		timeout = user_timeout;

		/* Now we are ready to repeat the original message send.  */
		goto message;
	      }
	    else
	      /* The EINTR return indicates cancellation,
		 so clear the flag.  */
	      ss->cancel = 0;
	  }
      }
      break;

    default:			/* Quiet -Wswitch-enum.  */
    }

  ss->intr_port = MACH_PORT_NULL;

  return err;
}
