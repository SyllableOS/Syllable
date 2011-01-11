/* Copyright (c) 2010,2011 Kaj de Vos

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.


Needs cURL >= 7.17 (for options copying)
(Would need cURL >= 7.19.4 to compile with CURLOPT_FTP_CREATE_MISSING_DIRS enums)
*/


#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>
#include "reb-host.h"

const char *init_block =
	"REBOL [\n"
		"Title: {cURL Binding}\n"
		"Name: cURL-binding\n"
		"Type: extension\n"
	"]\n"
	"export curl: context [\n"
		"version: command [{Return cURL version.}]\n"

		"new-group: command [{Return session group handle.}]\n"
		"end-group: command [{Clean up session group.} group [handle!]]\n"

		"new-session: command [{Return session handle.}]\n"
		"end-session: command [{Clean up session.} session [handle!]]\n"
		"reset: command [{Clear session settings.} session [handle!]]\n"

		"read: command [{Prepare read action.} session [handle!] buffer [binary!]]\n"
		"write: command [{Prepare write action.} session [handle!] data [binary!]]\n"

		"do: command [{Perform single prepared action on a URL.} session [handle!] target [url! string!]]\n"

		"open: command [{Open a URL.} group [handle!] session [handle!] target [url! string!]]\n"
		"close: command [{Clean up a session from a group.} group [handle!] session [handle!]]\n"
		"do-events: command [{Service all sessions in group. Return number of unfinished sessions.} group [handle!]]\n"
		"max-wait: command [{Determine timeout in milliseconds for events processing, or NONE.} group [handle!]]\n"
		"message: command [{Fetch a status message.} group [handle!]]\n"

		"progress: command [{Set progress callback function.} session [handle!] context [object!] 'function [lit-word!]]\n"

		"group-error: command [{Return last group error.} group [handle!]]\n"
		"session-error: command [{Return last session error.} session [handle!]]\n"
		"trace: command [{Activate trace output to stderr.} session [handle!] switch [logic!]]\n"
		"include-headers: command [{Include headers in content body.} session [handle!] switch [logic!]]\n"
		"fail-on-error: command [{Fail silently on HTTP return codes >= 400.} session [handle!] switch [logic!]]\n"

		"timeout: command [{Set timeout period.} session [handle!] seconds [integer!]]\n"
		"connect-timeout: command [{Set connection timeout period.} session [handle!] seconds [integer!]]\n"
		"max-redirections: command [{Limit location redirections.} session [handle!] number [integer! none!]]\n"
		"unrestricted-auth: command [{Unlimited reauthentication on redirections.} session [handle!] switch [logic!]]\n"
		"verify-peer: command [{Verify remote SSL certificate authenticity.} session [handle!] switch [logic!]]\n"
		"verify-host: command [{Verify that SSL certificate belongs to server.} session [handle!] level [integer!]]\n"

		"headers: command [{Set HTTP request headers.} session [handle!] strings [block!]]\n"
		"new-file-rights: command [{Set permissions for newly created remote files.} session [handle!] mask [integer!]]\n"
		"new-dir-rights: command [{Set permissions for newly created remote directories.} session [handle!] mask [integer!]]\n"

		"min-speed: command [{Set low speed limit.} session [handle!] speed [integer!] {Bytes per second} duration [integer!] {Seconds}]\n"
	"]"
;


RL_LIB *RL;

RXIEXT const char *RX_Init(int options, RL_LIB *library) {
	RL = library;
	if (!CHECK_STRUCT_ALIGN) return 0;

	if (curl_global_init(CURL_GLOBAL_ALL)) return 0;  // Not thread safe

	return init_block;
}

RXIEXT int RX_Quit(int options) {
	curl_global_cleanup();  // Not thread safe

	return 0;
}


struct group {
	CURLM *handle;
	CURLMcode status;
};

struct session {
	CURL *handle;
	CURLcode status;

	REBSER *input;

	REBSER *output;
	u64 progress;

	struct curl_slist *headers;

	// Progress callback
	REBSER *context;
	u32 word;
};


// Callback for storing a buffer of read data

size_t write_buffer(char *buffer, size_t width, size_t chunks, void *series) {
	u32 tail, index, length = chunks * width;

	tail = RL_SERIES(series, RXI_SER_TAIL);

	for (index = 0; index < length; ) {
		RL_SET_CHAR(series, tail++, buffer[index++]);
	}

	return index;
}

// Callback for filling the buffer for writing

size_t read_buffer(char *buffer, size_t width, size_t chunks, void *session) {
	char *data;
	u32 tail, length = chunks * width;

//	R3 A110 FIXME: address miscomputed as if items are double bytes:
//	tail = - RL_GET_STRING(((struct session *) session)->output, ((struct session *) session)->progress, (void **) &data);
	tail = - RL_GET_STRING(((struct session *) session)->output, 0, (void **) &data) - ((struct session *) session)->progress;

	if (tail < length) length = tail;

//	memcpy(buffer, data, length);  // Thread safe?
	memcpy(buffer, data + ((struct session *) session)->progress, length);
	((struct session *) session)->progress += length;

	return length;
}

// Callback for progress status

int progress(void *session, double download_goal, double download_now, double upload_goal, double upload_now) {
	RXICBI callback;
	RXIARG arguments[5];

	CLEAR(&callback, sizeof(callback));
	CLEAR(arguments, sizeof(arguments));

	callback.obj = ((struct session *) session)->context;
	callback.word = ((struct session *) session)->word;
	callback.args = arguments;

	RXI_COUNT(arguments) = 4;
	RXI_TYPE(arguments, 1) = RXT_DECIMAL;
	RXI_TYPE(arguments, 2) = RXT_DECIMAL;
	RXI_TYPE(arguments, 3) = RXT_DECIMAL;
	RXI_TYPE(arguments, 4) = RXT_DECIMAL;
	arguments[1].dec64 = download_goal;
	arguments[2].dec64 = download_now;
	arguments[3].dec64 = upload_goal;
	arguments[4].dec64 = upload_now;

	RL_CALLBACK(&callback);  // Should return RXT_INTEGER

	return callback.result.int64;
}


RXIEXT int RX_Call(int command, RXIFRM *arguments, void *dummy) {
	CURLM *group_handle;
	CURLMcode group_status;
	struct group *group;

	CURL *handle;
	CURLcode status;
	struct session *session;

	REBSER *series;
	u32 index, i, length, chr;
	char *string;

	int rest;

	switch (command) {
	case 0: {  // version
		string = curl_version();

		series = RL_MAKE_STRING(strlen(string), FALSE);

		for (index = 0; (chr = string[index]); ++index) {
			RL_SET_CHAR(series, index, chr);
		}
		RXA_SERIES(arguments, 1) = series;
		RXA_INDEX(arguments, 1) = 0;
		RXA_TYPE(arguments, 1) = RXT_STRING;

		return RXR_VALUE;
	}
	case 1: {  // new-group
		if ((group = malloc(sizeof(struct session))) == NULL) return RXR_FALSE;

		if ((group->handle = curl_multi_init()) == NULL) return RXR_FALSE;

		RXA_HANDLE(arguments, 1) = group;
		RXA_TYPE(arguments, 1) = RXT_HANDLE;

		return RXR_VALUE;
	}
	case 2: {  // end-group
		group = RXA_HANDLE(arguments, 1);

		group_status = curl_multi_cleanup(group->handle);
		free(group);
		if (group_status) goto group_error;

		return RXR_UNSET;
	}
	case 3: {  // new-session
		if ((session = malloc(sizeof(struct session))) == NULL) {
//			session->status = CURLE_OUT_OF_MEMORY;
//			break;
			return RXR_FALSE;
		}
		session->handle = handle = curl_easy_init();
		session->status = CURLE_OK;
		session->input = session->output = NULL;
		session->headers = NULL;

		RXA_HANDLE(arguments, 1) = session;
		RXA_TYPE(arguments, 1) = RXT_HANDLE;

		return RXR_VALUE;
	}
	case 4: {  // end-session
		session = RXA_HANDLE(arguments, 1);

		curl_easy_cleanup(session->handle);
		curl_slist_free_all(session->headers);

		if ((series = session->input) != NULL) RL_PROTECT_GC(series, 0);
		if ((series = session->output) != NULL) RL_PROTECT_GC(series, 0);

		free(session);

		return RXR_UNSET;
	}
	case 5: {  // reset
		session = RXA_HANDLE(arguments, 1);
		curl_easy_reset(session->handle);

		return RXR_UNSET;
	}
	case 6: {  // read
		session = RXA_HANDLE(arguments, 1);
		handle = session->handle;

		if ((series = session->input) != NULL) RL_PROTECT_GC(series, 0);
		session->input = series = RXA_SERIES(arguments, 2);
		RL_PROTECT_GC(series, 1);

		if (status = curl_easy_setopt(handle, CURLOPT_WRITEDATA, series)) break;
		if (status = curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_buffer)) break;

		if (status = curl_easy_setopt(handle, CURLOPT_FTP_CREATE_MISSING_DIRS, 0L)) break;

		if (status = curl_easy_setopt(handle, CURLOPT_UPLOAD, 0L)) break;

//		RXA_ARG(arguments, 1) = RXA_ARG(arguments, 2);
//		RXA_TYPE(arguments, 1) = RXT_BINARY;

//		return RXR_VALUE;
		return RXR_TRUE;
	}
	case 7: {  // write
		session = RXA_HANDLE(arguments, 1);
		handle = session->handle;

		if ((series = session->output) != NULL) RL_PROTECT_GC(series, 0);
		session->output = series = RXA_SERIES(arguments, 2);
		RL_PROTECT_GC(series, 1);

		session->progress = RXA_INDEX(arguments, 2);

		if (status = curl_easy_setopt(handle, CURLOPT_READDATA, session)) break;
		if (status = curl_easy_setopt(handle, CURLOPT_READFUNCTION, read_buffer)) break;

//		64 bits size doesn't seem to work:
//		if (status = curl_easy_setopt(handle, CURLOPT_INFILESIZE_LARGE, (curl_off_t) (RL_SERIES(series, RXI_SER_TAIL) - session->progress))) break;
		if (status = curl_easy_setopt(handle, CURLOPT_INFILESIZE, (long) (RL_SERIES(series, RXI_SER_TAIL) - session->progress))) break;

//		if (status = curl_easy_setopt(handle, CURLOPT_FTP_CREATE_MISSING_DIRS, CURLFTP_CREATE_DIR_RETRY)) break;
		if (status = curl_easy_setopt(handle, CURLOPT_FTP_CREATE_MISSING_DIRS, 2L)) break;

		if (status = curl_easy_setopt(handle, CURLOPT_UPLOAD, 1L)) break;

		return RXR_TRUE;
	}
	case 8: {  // do
		session = RXA_HANDLE(arguments, 1);
		handle = session->handle;

		series = RXA_SERIES(arguments, 2);
		index = RXA_INDEX(arguments, 2);
		length = RL_SERIES(series, RXI_SER_TAIL);

		if ((string = malloc(length - index + 1)) == NULL) {
			status = CURLE_OUT_OF_MEMORY;
			break;
		}
		for (i = 0; index < length; ) {
			string[i++] = RL_GET_CHAR(series, index++);
		}
		string[index] = '\0';

		status = curl_easy_setopt(handle, CURLOPT_URL, string);
		free(string);
		if (status) break;

		if (status = curl_easy_perform(handle)) break;

		return RXR_TRUE;
	}
	case 9: {  // open
		group = RXA_HANDLE(arguments, 1);

		session = RXA_HANDLE(arguments, 2);
		handle = session->handle;

		series = RXA_SERIES(arguments, 3);
		index = RXA_INDEX(arguments, 3);
		length = RL_SERIES(series, RXI_SER_TAIL);

		if ((string = malloc(length - index + 1)) == NULL) {
			status = CURLE_OUT_OF_MEMORY;
			break;
		}
		for (i = 0; index < length; ) {
			string[i++] = RL_GET_CHAR(series, index++);
		}
		string[index] = '\0';

		status = curl_easy_setopt(handle, CURLOPT_URL, string);
		free(string);
		if (status) break;

		if (group_status = curl_multi_add_handle(group->handle, handle)) goto group_error;

		return RXR_TRUE;
	}
	case 10: {  // close
		group = RXA_HANDLE(arguments, 1);
		session = RXA_HANDLE(arguments, 2);

		if (group_status = curl_multi_remove_handle(group->handle, session->handle)) goto group_error;

		return RXR_TRUE;
	}
	case 11: {  // do-events
		group = RXA_HANDLE(arguments, 1);
		group_handle = group->handle;

		while ((group_status = curl_multi_perform(group_handle, &rest)) == CURLM_CALL_MULTI_PERFORM);

		if (group_status) goto group_error;

		RXA_INT64(arguments, 1) = rest;
		RXA_TYPE(arguments, 1) = RXT_INTEGER;

		return RXR_VALUE;
	}
	case 12: {  // max-wait
		long timeout;

		group = RXA_HANDLE(arguments, 1);

		if (group_status = curl_multi_timeout(group->handle, &timeout)) goto group_error;

		if (timeout == -1L) return RXR_NONE;

		RXA_INT64(arguments, 1) = timeout;
		RXA_TYPE(arguments, 1) = RXT_INTEGER;

		return RXR_VALUE;
	}
	case 13: {  // message
		CURLMsg *message;

		group = RXA_HANDLE(arguments, 1);
		group_handle = group->handle;

		while ((message = curl_multi_info_read(group_handle, &rest)) != NULL)
			if (message->msg == CURLMSG_DONE) {
				RXA_HANDLE(arguments, 1) = message->easy_handle;
				RXA_TYPE(arguments, 1) = RXT_HANDLE;

				if (status = message->data.result) {
					RXA_COUNT(arguments) = 3;

					RXA_INT64(arguments, 2) = status;
					RXA_TYPE(arguments, 2) = RXT_INTEGER;

					string = (char *) curl_easy_strerror(status);
					series = RL_MAKE_STRING(strlen(string), FALSE);

					for (index = 0; (chr = string[index]); ++index) {
						RL_SET_CHAR(series, index, chr);
					}
					RXA_SERIES(arguments, 3) = series;
					RXA_INDEX(arguments, 3) = 0;
					RXA_TYPE(arguments, 3) = RXT_STRING;

					return RXR_BLOCK;
				} else {
					return RXR_VALUE;
				}
			}

		return RXR_NONE;
	}
	case 14: {  // progress
		session = RXA_HANDLE(arguments, 1);
		handle = session->handle;

		session->context = RXA_OBJECT(arguments, 2);
		session->word = RXA_WORD(arguments, 3);

		if (status = curl_easy_setopt(handle, CURLOPT_PROGRESSDATA, session)) break;
		if (status = curl_easy_setopt(handle, CURLOPT_PROGRESSFUNCTION, progress)) break;

		if (status = curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0L)) break;

		return RXR_TRUE;
	}
	case 15: {  // group-error
		group = RXA_HANDLE(arguments, 1);

		RXA_COUNT(arguments) = 2;

		RXA_INT64(arguments, 1) = group->status;
		RXA_TYPE(arguments, 1) = RXT_INTEGER;

		string = (char *) curl_multi_strerror(group->status);
		series = RL_MAKE_STRING(strlen(string), FALSE);

		for (index = 0; (chr = string[index]); ++index) {
			RL_SET_CHAR(series, index, chr);
		}
		RXA_SERIES(arguments, 2) = series;
		RXA_INDEX(arguments, 2) = 0;
		RXA_TYPE(arguments, 2) = RXT_STRING;

		return RXR_BLOCK;
	}
	case 16: {  // session-error
		session = RXA_HANDLE(arguments, 1);

		RXA_COUNT(arguments) = 2;

		RXA_INT64(arguments, 1) = session->status;
		RXA_TYPE(arguments, 1) = RXT_INTEGER;

		string = (char *) curl_easy_strerror(session->status);
		series = RL_MAKE_STRING(strlen(string), FALSE);

		for (index = 0; (chr = string[index]); ++index) {
			RL_SET_CHAR(series, index, chr);
		}
		RXA_SERIES(arguments, 2) = series;
		RXA_INDEX(arguments, 2) = 0;
		RXA_TYPE(arguments, 2) = RXT_STRING;

		return RXR_BLOCK;
	}
	case 17: {  // trace
		session = RXA_HANDLE(arguments, 1);

		if (status = curl_easy_setopt(session->handle, CURLOPT_VERBOSE, RXA_LOGIC(arguments, 2))) break;

		return RXR_TRUE;
	}
	case 18: {  // include-headers
		session = RXA_HANDLE(arguments, 1);

		if (status = curl_easy_setopt(session->handle, CURLOPT_HEADER, RXA_LOGIC(arguments, 2))) break;

		return RXR_TRUE;
	}
	case 19: {  // fail-on-error
		session = RXA_HANDLE(arguments, 1);

		if (status = curl_easy_setopt(session->handle, CURLOPT_FAILONERROR, RXA_LOGIC(arguments, 2))) break;

		return RXR_TRUE;
	}
	case 20: {  // timeout
		session = RXA_HANDLE(arguments, 1);

		if (status = curl_easy_setopt(session->handle, CURLOPT_TIMEOUT, (long) RXA_INT64(arguments, 2))) break;

		return RXR_TRUE;
	}
	case 21: {  // connect-timeout
		session = RXA_HANDLE(arguments, 1);

		if (status = curl_easy_setopt(session->handle, CURLOPT_CONNECTTIMEOUT, (long) RXA_INT64(arguments, 2))) break;

		return RXR_TRUE;
	}
	case 22: {  // max-redirections
		BOOL unlimited = RXA_TYPE(arguments, 2) == RXT_NONE;
		long level = unlimited ? -1L : RXA_INT64(arguments, 2);

		session = RXA_HANDLE(arguments, 1);
		handle = session->handle;

		if (status = curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, level ? 1L : 0L)) break;

		if (status = curl_easy_setopt(handle, CURLOPT_MAXREDIRS, level)) break;

		return RXR_TRUE;
	}
	case 23: {  // unrestricted-auth
		session = RXA_HANDLE(arguments, 1);

		if (status = curl_easy_setopt(session->handle, CURLOPT_UNRESTRICTED_AUTH, RXA_LOGIC(arguments, 2))) break;

		return RXR_TRUE;
	}
	case 24: {  // verify-peer
		session = RXA_HANDLE(arguments, 1);

		if (status = curl_easy_setopt(session->handle, CURLOPT_SSL_VERIFYPEER, RXA_LOGIC(arguments, 2))) break;

		return RXR_TRUE;
	}
	case 25: {  // verify-host
		session = RXA_HANDLE(arguments, 1);

		if (status = curl_easy_setopt(session->handle, CURLOPT_SSL_VERIFYHOST, (long) RXA_INT64(arguments, 2))) break;

		return RXR_TRUE;
	}
	case 26: {  // headers
		struct curl_slist *headers;
		RXIARG value;
//		u32 type;
		REBSER *item;
		u32 ind, len;

		session = RXA_HANDLE(arguments, 1);

		curl_slist_free_all(session->headers);  // Thread safe?
		headers = NULL;

		series = RXA_SERIES(arguments, 2);
		index = RXA_INDEX(arguments, 2);
		length = RL_SERIES(series, RXI_SER_TAIL);

		while (index < length) {
			if (RL_GET_VALUE(series, index++, &value) == RXT_STRING) {
				item = value.series;
				ind = value.index;
				len = RL_SERIES(item, RXI_SER_TAIL);

				if ((string = malloc(len - ind + 1)) == NULL) {
					status = CURLE_OUT_OF_MEMORY;
					goto error;
				}
				for (i = 0; ind < len; ) {
					string[i++] = RL_GET_CHAR(item, ind++);
				}
				string[i] = '\0';

				headers = curl_slist_append(headers, string);
				free(string);
			}
		}
		if (status = curl_easy_setopt(session->handle, CURLOPT_HTTPHEADER, session->headers = headers)) break;

		return RXR_TRUE;
	}
	case 27: {  // new-file-rights
		session = RXA_HANDLE(arguments, 1);

		if (status = curl_easy_setopt(session->handle, CURLOPT_NEW_FILE_PERMS, (long) RXA_INT64(arguments, 2))) break;

		return RXR_TRUE;
	}
	case 28: {  // new-dir-rights
		session = RXA_HANDLE(arguments, 1);

		if (status = curl_easy_setopt(session->handle, CURLOPT_NEW_DIRECTORY_PERMS, (long) RXA_INT64(arguments, 2))) break;

		return RXR_TRUE;
	}
	case 29: {  // min-speed
		session = RXA_HANDLE(arguments, 1);
		handle = session->handle;

		if (status = curl_easy_setopt(handle, CURLOPT_LOW_SPEED_LIMIT, (long) RXA_INT64(arguments, 2))) break;

		if (status = curl_easy_setopt(handle, CURLOPT_LOW_SPEED_TIME, (long) RXA_INT64(arguments, 3))) break;

		return RXR_TRUE;
	}
	default:
		return RXR_NO_COMMAND;
	}

error:
	session->status = status;
	return RXR_FALSE;

group_error:
	group->status = group_status;
	return RXR_FALSE;
}
