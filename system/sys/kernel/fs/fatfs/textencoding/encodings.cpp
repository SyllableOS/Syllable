/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

//#include <UTF8.h>
#include "UnicodeMappings.h"
#include "EncodingConversions.h"

//#include <KernelExport.h>

//#include <malloc.h>
//#include <stdio.h>
#include <atheos/kernel.h>
#include <atheos/string.h>
#include <atheos/time.h>
#include <posix/errno.h>

#include "encodings.h"
#include "../util.h"

#include <macros.h>

extern "C" {
extern int debug_encodings;
}

#define B_OK 0
#define strerror(a)		"strerror_not_implemented"

#define TOUCH(x) ((void)(x))

#ifdef USER
	#include <stdio.h>
	#define dprintf printk
	#undef DEBUG
	int _assert_(char *,int,char *) {return 0;}
#endif

#define DPRINTF(a,b) if (debug_encodings > (a)) printk b

inline bool is_unicode_japanese(uint16 c)
{
	if (((c >= 0x3000) && (c <= 0x30ff)) ||
			((c >= 0x3200) && (c <= 0x3400)) ||
			((c >= 0x4e00) && (c <= 0x9fff)) ||
			((c >= 0xf900) && (c <= 0xfaff)) ||
			((c >= 0xfe30) && (c <= 0xfe6f)) ||
			((c >= 0xff00) && (c <= 0xffef)))
		return true;

	return false;
}

inline bool is_initial_sjis_byte(uchar c)
{
	return (((c >= 0x81) && (c <= 0x9F)) ||
			((c >= 0xE0) && (c <= 0xEF)) );
}


// takes a unicode name of unilen uchar's and converts to a utf8 name of at
// most utf8len uint8's
status_t unicode_to_utf8(const uchar *uni, uint32 unilen, uint8 *utf8, 
	uint32 utf8len)
{
	int32 state = 0;
	status_t result;
	uint32 origlen = unilen;

	DPRINTF(0, ("unicode_to_utf8\n"));

	result = convert_to_utf8(B_UNICODE_CONVERSION, (char *)uni,
			(int32 *)&unilen, (char *)utf8, (int32 *)&utf8len, &state);

	if (unilen < origlen) {
		printk("Name is too long (%x < %x)\n", unilen, origlen);
		return -ENAMETOOLONG;
	}

	return result;
}

const char acceptable[]="!#$%&'()-0123456789@ABCDEFGHIJKLMNOPQRSTUVWXYZ^_`{}~";
const char illegal[] = "\\/:*?\"<>|";
const char underbar[] = "+,;=[]"
		"\x83\x85\x88\x89\x8A\x8B\x8C\x8D"
		"\x93\x95\x96\x97\x98"
		"\xA0\xA1\xA2\xA3";
const char capitalize_from[] = "\x81\x82\x84\x86\x87\x91\x94\xA4";
const char capitalize_to[]   = "\x9A\x90\x8E\x8F\x80\x92\x99\xA5";

/* XXX: this doesn't work quite right, but it's okay for now since having
 * lfn's where they aren't needed doesn't hurt */
bool requires_long_name(const char *utf8, const uchar *unicode)
{
	int i, j;

	DPRINTF(0, ("requires_long_name\n"));

	for (i=0;unicode[i] || unicode[i+1];i+=2) {
		uint16 ar = (unicode[i+1] << 8) | unicode[i];
		if (is_unicode_japanese(ar)) {
			DPRINTF(1, ("All Japanese names require long file names for now\n"));
			return true;
		}
	}

	for (i=0;i<8;i++) {
		if (utf8[i] == 0) return false;
		if (utf8[i] == '.') break;
		/* XXX: should also check for upper-ascii stuff (requires matching
		 * unicode with msdostou table, but doesn't hurt for now */
		if (!strchr(acceptable, utf8[i])) return true;
	}

	if (utf8[i] == 0) return false;
	if ((i == 8) && (utf8[i] != '.')) return true; /* name too long */
	i++;
	if (utf8[i] == 0) return true; /* filenames with trailing periods */

	for (j=0;j<3;j++,i++) {
		if (utf8[i] == 0) return false;
		/* XXX: same here */
		if (!strchr(acceptable, utf8[i])) return true;
	}

	return (utf8[i] == 0) ? false : true;
}

status_t utf8_to_unicode(const char *utf8, uchar *uni, uint32 unilen)
{
	int32 state = 0;
	status_t result;
	uint32 i, utf8len, origlen;

	DPRINTF(0, ("utf8_to_unicode\n"));

	utf8len = origlen = strlen(utf8) + 1;

	result = convert_from_utf8(B_UNICODE_CONVERSION, utf8,
			(int32 *)&utf8len, (char *)uni, (int32 *)&unilen, &state);

	for (i=0;i<unilen;i+=2) {
		uni[i] ^= uni[i+1];
		uni[i+1] ^= uni[i];
		uni[i] ^= uni[i+1];
	}

	if (origlen < utf8len) {
		printk("Name is too long (%x < %x)\n", unilen, origlen);
		return -ENAMETOOLONG;
	}

	return (result == B_OK) ? unilen : result;
}

static
status_t munge_short_name_english(uchar nshort[11], uint64 value)
{
	char buffer[8];
	int len, i;

	DPRINTF(0, ("munge_short_name_english\n"));

	// short names must have only numbers following
	// the tilde and cannot begin with 0
	sprintf(buffer, "~%Ld", value);
	len = strlen(buffer);
	i = 7 - len;

	ASSERT((i > 0) && (i < 8));

	while ((nshort[i] == ' ') && (i > 0)) i--;
	i++;

	memcpy(nshort + i, buffer, len);

	return B_OK;
}

static
status_t munge_short_name_sjis(uchar nshort[11], uint64 value)
{
	char buffer[8];
	int len, i, last;

	DPRINTF(0, ("munge_short_name_sjis\n"));

	// short names must have only numbers following
	// the tilde and cannot begin with 0
	sprintf(buffer, "~%Ld", value);
	len = strlen(buffer);

	last = 0;
	for (i=0;i<=8-len;i++) {
		last = i;
		if (nshort[i] == ' ') break;
		if (is_initial_sjis_byte(nshort[i])) i++;
	}
	
	memcpy(nshort + last, buffer, len);
	memset(nshort + last + len, ' ', 8 - (last + len));

	return B_OK;
}

status_t munge_short_name2(uchar nshort[11], int encoding)
{
	uint64 value;

	DPRINTF(0, ("munge_short_name2\n"));

	value = get_system_time() % 100000LL;
	if (!value) value++;

	/* XXX: what if it's zero? */
	if (encoding == B_MS_DOS_CONVERSION)
		return munge_short_name_english(nshort, value);
	else if (encoding == B_SJIS_CONVERSION)
		return munge_short_name_sjis(nshort, value);

	return -EINVAL;
}

status_t munge_short_name1(uchar nshort[11], int iteration, int encoding)
{
	DPRINTF(0, ("munge_short_name1\n"));

	if (encoding == B_MS_DOS_CONVERSION)
		return munge_short_name_english(nshort, iteration);
	else if (encoding == B_SJIS_CONVERSION)
		return munge_short_name_sjis(nshort, iteration);

	return -EINVAL;
}

/*
	rules:
	1.	if there are multiple '.'s in the name, the last one signals the
		extension. else scandisk complains.
	2.  a lower case letter gets translated to its upper case equivalent in the
		short name. this includes weird upper-ascii letters.
	3.	devices (CON, PRN, etc) are not valid short names. (implemented in
		dir.c)
	4.  ignore leading '.'s; a name of all '.'s is illegal
*/

static status_t
generate_short_name_msdos(const uchar *utf8, const uchar *uni,
		uint32 unilen, uchar nshort[11])
{
	int i;
	char *cp;
	const uchar *puni;

	TOUCH(utf8);

	DPRINTF(0, ("generate_short_name_msdos\n"));

	for (i=0,puni=uni;(i<8)&&(puni<uni+unilen);puni+=2) {
		uint16 match;
		uint32 c;

		match = puni[0] + 0x100*puni[1];

		if (match == 0) return B_OK;
		if (match == '.') {
			/* skip leading dots */
			if (i == 0) continue; else break;
		}

		for (c=0;c<0x100;c++)
			if (msdostou[c] == match)
				break;

		if (c < 0x100) {
			if (strchr(illegal, c)) return -EINVAL;

			if ((c >= 'a') && (c <= 'z'))
				nshort[i++] = c - 'a' + 'A';
			else if (strchr(underbar, c))
				nshort[i++] = '_';
			else if ((cp = strchr(capitalize_from, c)) != NULL)
				nshort[i++] = capitalize_to[(int)(cp - capitalize_from)];
			else if (strchr(acceptable, c) || strchr(capitalize_to, c))
				nshort[i++] = c;
		}
	}

	/* find the final dot */
	for (puni = uni + unilen - 2;puni >= uni; puni-=2)
		if ((puni[1] == 0) && (puni[0] == '.'))
			break;

	if (puni < uni) return B_OK;

	puni += 2;

	for (i=8;(i<11)&&(puni<uni+unilen);puni+=2) {
		uint16 match;
		uint32 c;

		match = puni[0] + 0x100*puni[1];

		if (match == 0) return B_OK;

		for (c=0;c<0x100;c++)
			if (msdostou[c] == match)
				break;

		if (c < 0x100) {
			if (strchr(illegal, c)) return -EINVAL;

			if ((c >= 'a') && (c <= 'z'))
				nshort[i++] = c - 'a' + 'A';
			else if (strchr(underbar, c))
				nshort[i++] = '_';
			else if ((cp = strchr(capitalize_from, c)) != NULL)
				nshort[i++] = capitalize_to[(int)(cp - capitalize_from)];
			else if (strchr(acceptable, c) || strchr(capitalize_to, c))
				nshort[i++] = c;
		}
	}

	return B_OK;
}

static status_t
generate_short_name_sjis(const uchar *utf8, const uchar *uni,
		uint32 unilen, uchar nshort[11])
{
	uchar sjis[1024], *s, *t;
	int32 utf8len, sjislen;
	status_t result;
	int32 state = 0;

	if ( unilen > 1024 ) {
	    return( -ENAMETOOLONG );
	}
	TOUCH(uni);

	DPRINTF(0, ("generate_short_name_sjis\n"));

//	sjis = (uchar *)kmalloc(unilen, MEMF_KERNEL);
//	if (sjis == NULL) return -ENOMEM;

	utf8len = strlen((const char *)utf8) + 1;
	sjislen = unilen;

	result = convert_from_utf8(B_SJIS_CONVERSION, (char *)utf8,
			(int32 *)&utf8len, (char *)sjis, (int32 *)&sjislen, &state);

	if (result < 0) goto bi;

	if (utf8len < (int32)strlen((const char *)utf8) + 1) {
		result = -EINVAL;
		goto bi;
	}

	result = B_OK;

	s = sjis;
	while (*s == '.') s++;
	for (state=0;state<8;) {
		if (is_initial_sjis_byte(*s)) {
			if (state < 7) {
				nshort[state++] = *(s++);
			} else
				break;
		} else {
			if (*s == '.') break;
			if (*s == 0) goto bi;
			if ((*s >= 'a') && (*s <= 'z')) *s = *s - 'a' + 'A';
		}
		nshort[state++] = *(s++);
	}

	t = sjis + strlen((const char *)sjis) - 1;
	while ((t >= s) && (*t != '.'))
		t--;

	if (t >= s) {
		s = t+1;
		for (state=8;state<11;) {
			if (is_initial_sjis_byte(*s)) {
				if (state < 10) {
					nshort[state++] = *(s++);
				} else
					break;
			} else {
				if (*s == 0) break;
				if ((*s >= 'a') && (*s <= 'z')) *s = *s - 'a' + 'A';
			}
			nshort[state++] = *(s++);
		}
	}

bi:
	if (result < 0) {
	    printk("generate_short_name_sjis error: %x (%s)\n", result, strerror(result));
	}

//	kfree(sjis);	
	
	return result;
}

status_t generate_short_name(const uchar *name, const uchar *uni,
		uint32 unilen, uchar nshort[11], int *encoding)
{
	uint32 i;
	status_t result;

	DPRINTF(0, ("generate_short_name\n"));

	ASSERT(encoding != NULL);
	memset(nshort, ' ', 11);

	/* assume english by default */
	*encoding = B_MS_DOS_CONVERSION;

	/* check for japanese */
	for (i=0;i<unilen;i+=2) {
		uint16 ar = (uni[i+1] << 8) | uni[i];
		if (is_unicode_japanese(ar)) {
			*encoding = B_SJIS_CONVERSION;
			break;
		}
	}

	if (*encoding == B_MS_DOS_CONVERSION)
		result = generate_short_name_msdos(name, uni, unilen, nshort);
	else if (*encoding == B_SJIS_CONVERSION)
		result = generate_short_name_sjis(name, uni, unilen, nshort);
	else
		return -EINVAL;

	if (result == B_OK) {
		if (!memcmp(nshort, "           ", 11)) /* no blank names allowed */
			nshort[0] = '_';
	}

	return result;
}

/* called to convert a short ms-dos filename to utf8.
   XXX: encoding is assumed to be standard US code page, never shift-JIS
   JH: convert short names to lowercase using Windows flags.
*/
status_t msdos_to_utf8(uchar *msdos, uchar *utf8, uint32 utf8len, uint8 lcase)
{
	uchar normalized[8+1+3+1];
	int32 state, i, pos;

	DPRINTF(0, ("msdos_to_utf8\n"));

	pos = 0;
	for (i=0;i<8;i++) {
		if (msdos[i] == ' ') break;
		normalized[pos++] = ((i == 0) && (msdos[i] == 5)) ? 0xe5 :
		    ((lcase & 0x8) && (msdos[i] >= 'A') && (msdos[i] <= 'Z')) ?
		    (msdos[i] - 'A' + 'a') : msdos[i];
	}

	if (msdos[8] != ' ') {
		normalized[pos++] = '.';
		for (i=8;i<11;i++) {
			if (msdos[i] == ' ') break;
			normalized[pos++] = ((lcase & 0x10) &&
				(msdos[i] >= 'A') && (msdos[i] <= 'Z')) ?
			    (msdos[i] - 'A' + 'a') : msdos[i];
		}
	}

	normalized[pos++] = 0;

	state = 0;

	return convert_to_utf8(B_MS_DOS_CONVERSION, (char *)normalized, &pos,
			(char *)utf8, (int32 *)&utf8len, &state);
}

#ifdef USER

int main(int argc, char **argv)
{
	int i;

	for (i=1;i<argc;i++) {
		char unicode[512];
		status_t result;
		result = utf8_to_unicode(argv[i], unicode, 512);
		printf("result of %s = %x ", argv[i], result);
		if (result > 0) {
			char nshort[11];
			int enc;
			result = generate_short_name(unicode, result, nshort, &enc);
			printf("short [%s] ", nshort);
			munge_short_name1(nshort, 1234, B_MS_DOS_CONVERSION);
			printf("munged [%s] ", nshort);
			printf("long name: %x\n", requires_long_name(argv[i]));
		}
		printf("\n");
	}

	return 0;
}

#endif
