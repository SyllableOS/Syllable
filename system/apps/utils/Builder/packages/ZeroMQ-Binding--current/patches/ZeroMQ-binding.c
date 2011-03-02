/* Copyright (c) 2010,2011 Kaj de Vos
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


Needs 0MQ 2.0.x (for zmq_init)
*/


#include <stdlib.h>
#include <string.h>

#include <zmq.h>
#include "reb-host.h"

const char *init_block =
	"REBOL [\n"
		"Title: {0MQ Binding}\n"
		"Name: ZeroMQ-binding\n"
		"Type: extension\n"
	"]\n"
	"export zmq: context [\n"
//			"poll-in: 1\n"
//			"poll-out: 2\n"
//			"poll-error: 4\n"

		"version: command [{Return 0MQ version.}]\n"

		"new-pool: command [{Return context handle.} app-threads [integer!] io-threads [integer!] flags [integer!]]\n"
			"poll: 1\n"
		"end-pool: command [{Clean up context.} pool [handle!]]\n"

		"open: command [{Open a socket.} pool [handle!] type [integer!]]\n"
			"pair: 0\n"
			"publish: 1\n"
			"subscribe: 2\n"
			"request: 3\n"
			"reply: 4\n"
			"extended-request: 5\n"
			"extended-reply: 6\n"
			"push: 7\n"
			"pull: 8\n"
		"close: command [{Clean up a socket from a context.} socket [handle!]]\n"

		"serve: command [{Set up server socket binding.} socket [handle!] end-point [url! string!]]\n"
		"connect: command [{Connect to a server socket.} socket [handle!] destination [url! string!]]\n"

		"no-block: 1\n"
		"send: command [{Send message.} socket [handle!] message [binary!] flags [integer!]]\n"
//			"send-more: \n"
		"receive: command [{Receive message.} socket [handle!] message [binary!] flags [integer!]]\n"

		"error: command [{Return last status.}]\n"
	"]"
;


RL_LIB *RL;

RXIEXT const char *RX_Init (int options, RL_LIB *library) {
	RL = library;
	if (! CHECK_STRUCT_ALIGN) return 0;

	return init_block;
}

RXIEXT int RX_Quit (int options) {
	return 0;
}


RXIEXT int RX_Call(int command, RXIFRM *arguments, void *dummy) {
	void *handle;
	zmq_msg_t message;
	int status;

	REBSER *series;
	u32 index, i, length, chr;

	char *data;

	switch (command) {
	case 0: {  // version
		int major, minor, patch;

		zmq_version (&major, &minor, &patch);

		RXA_TUPLE (arguments, 1) [0] = 3;
		RXA_TUPLE (arguments, 1) [1] = major;
		RXA_TUPLE (arguments, 1) [2] = minor;
		RXA_TUPLE (arguments, 1) [3] = patch;
		RXA_TYPE (arguments, 1) = RXT_TUPLE;

		return RXR_VALUE;
	}
	case 1: {  // new-pool
		if ((handle = zmq_init (RXA_INT64 (arguments, 1), RXA_INT64 (arguments, 2), RXA_INT64 (arguments, 3))) == NULL) break;

		RXA_HANDLE (arguments, 1) = handle;
		RXA_TYPE (arguments, 1) = RXT_HANDLE;

		return RXR_VALUE;
	}
	case 2: {  // end-pool
		if (zmq_term (RXA_HANDLE (arguments, 1))) break;

		return RXR_TRUE;
	}
	case 3: {  // open
		if ((handle = zmq_socket (RXA_HANDLE (arguments, 1), RXA_INT64 (arguments, 2))) == NULL) break;

		RXA_HANDLE (arguments, 1) = handle;
		RXA_TYPE (arguments, 1) = RXT_HANDLE;

		return RXR_VALUE;
	}
	case 4: {  // close
		if (zmq_close (RXA_HANDLE (arguments, 1))) break;

		return RXR_TRUE;
	}
	case 5: {  // serve
		series = RXA_SERIES (arguments, 2);
		index = RXA_INDEX (arguments, 2);
		length = RL_SERIES (series, RXI_SER_TAIL);

		if ((data = malloc (length - index + 1)) == NULL) {
			// FIXME: report ENOMEM
			break;
		}
		for (i = 0; index < length; ) {
			data [i++] = RL_GET_CHAR (series, index++);
		}
		data [index] = '\0';

		status = zmq_bind (RXA_HANDLE (arguments, 1), data);
		free (data);
		if (status) break;

		return RXR_TRUE;
	}
	case 6: {  // connect
		series = RXA_SERIES (arguments, 2);
		index = RXA_INDEX (arguments, 2);
		length = RL_SERIES (series, RXI_SER_TAIL);

		if ((data = malloc (length - index + 1)) == NULL) {
			// FIXME: report ENOMEM
			break;
		}
		for (i = 0; index < length; ) {
			data [i++] = RL_GET_CHAR (series, index++);
		}
		data [index] = '\0';

		status = zmq_connect (RXA_HANDLE (arguments, 1), data);
		free (data);
		if (status) break;

		return RXR_TRUE;
	}
	case 7: {  // send
		series = RXA_SERIES (arguments, 2);
		index = RXA_INDEX (arguments, 2);

//		R3 A110 FIXME: address miscomputed as if items are double bytes:
//		length = - RL_GET_STRING (series, index, (void **) &data);
		length = - RL_GET_STRING (series, 0, (void **) &data) - index;

		if (zmq_msg_init_size (&message, length)) break;

//		memcpy (zmq_msg_data (&message), data, length);  // Thread safe?
		memcpy (zmq_msg_data (&message), data + index, length);

		if (zmq_send (RXA_HANDLE (arguments, 1), &message, RXA_INT64 (arguments, 3))) {
			zmq_msg_close (&message);
			break;
		}
		if (zmq_msg_close (&message)) break;

		return RXR_TRUE;
	}
	case 8: {  // receive
		if (zmq_msg_init (&message)) break;

		if (zmq_recv (RXA_HANDLE (arguments, 1), &message, RXA_INT64 (arguments, 3))) {
			zmq_msg_close (&message);
			break;
		}
		data = zmq_msg_data (&message);
		length = zmq_msg_size (&message);

		series = RXA_SERIES (arguments, 2);
		index = RL_SERIES (series, RXI_SER_TAIL);

		for (i = 0; i < length; ) {
			RL_SET_CHAR (series, index++, data [i++]);
		}
		if (zmq_msg_close (&message)) break;

		return RXR_TRUE;
	}
	case 9: {  // error
		RXA_COUNT (arguments) = 2;

		RXA_INT64 (arguments, 1) = status = zmq_errno ();
		RXA_TYPE (arguments, 1) = RXT_INTEGER;

		data = (char *) zmq_strerror (status);
		series = RL_MAKE_STRING (strlen (data), FALSE);

		for (index = 0; (chr = data [index]); ++index) {
			RL_SET_CHAR (series, index, chr);
		}
		RXA_SERIES (arguments, 2) = series;
		RXA_INDEX (arguments, 2) = 0;
		RXA_TYPE (arguments, 2) = RXT_STRING;

		return RXR_BLOCK;
	}
	default:
		return RXR_NO_COMMAND;
	}

	return RXR_FALSE;
}
