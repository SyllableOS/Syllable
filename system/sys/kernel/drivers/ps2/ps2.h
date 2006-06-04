/*
 *  The Syllable kernel
 *  Keyboard and PS2 registers
 *  Copyright (C) 2006 Arno Klenke
 *  Parts of this code are from the linux kernel
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
 
 
#ifndef _PS2_H_
#define _PS2_H_

/* Registers */
#define PS2_COMMAND_REG		0x64
#define PS2_STATUS_REG		0x64
#define PS2_DATA_REG		0x60

/* Status register bits */

#define PS2_STS_PARITY		0x80
#define PS2_STS_TIMEOUT		0x40
#define PS2_STS_AUXDATA		0x20
#define PS2_STS_KEYLOCK		0x10
#define PS2_STS_CMDDAT		0x08
#define PS2_STS_MUXERR		0x04
#define PS2_STS_IBF			0x02
#define	PS2_STS_OBF			0x01

/* Control register bits */
#define PS2_CTR_KBDINT		0x01
#define PS2_CTR_AUXINT		0x02
#define PS2_CTR_IGNKEYLOCK	0x08
#define PS2_CTR_KBDDIS		0x10
#define PS2_CTR_AUXDIS		0x20
#define PS2_CTR_XLATE		0x40

/* Commands */
#define PS2_CMD_RCTR		0x20
#define PS2_CMD_WCTR		0x60
#define PS2_CMD_TEST		0xaa

#define PS2_CMD_KBD_DISABLE	0xad
#define PS2_CMD_KBD_ENABLE	0xae
#define PS2_CMD_KBD_TEST	0xab
#define PS2_CMD_KBD_LOOP	0xd2

#define PS2_CMD_AUX_DISABLE	0xa7
#define PS2_CMD_AUX_ENABLE	0xa8
#define PS2_CMD_AUX_TEST	0xa9
#define PS2_CMD_AUX_SEND	0xd4
#define PS2_CMD_AUX_LOOP	0xd3

/* Keyboard commands */
#define PS2_CMD_KBD_SETLEDS	0xed

/* Aux device commands */
#define PS2_AUX_SET_RES	0xe8		/* set resolution */
#define PS2_AUX_SET_SCALE11	0xe6		/* set 1:1 scaling */
#define PS2_AUX_SET_SCALE21	0xe7		/* set 2:1 scaling */
#define PS2_AUX_GET_SCALE	0xe9		/* get scaling factor */
#define PS2_AUX_SET_STREAM	0xea		/* set stream mode */
#define PS2_AUX_SET_SAMPLE	0xf3		/* set sample rate */
#define PS2_AUX_ENABLE_DEV	0xf4		/* enable aux device */
#define PS2_AUX_DISABLE_DEV	0xf5		/* disable aux device */
#define PS2_AUX_RESET	0xff		/* reset aux device */

/* Translated AT key table */

static const uint8 g_anRawKeyTab[] =
{
    0x00,	/*	NO KEY	*/
    0x01,	/* 1   ESC	*/
    0x12,	/* 2   1	*/
    0x13,	/* 3   2	*/
    0x14,	/* 4   3	*/
    0x15,	/* 5   4	*/
    0x16,	/* 6   5	*/
    0x17,	/* 7   6	*/
    0x18,	/* 8   7	*/
    0x19,	/* 9   8	*/
    0x1a,	/* 10   9	*/
    0x1b,	/* 11   0	*/
    0x1c,	/* 12   -	*/
    0x1d,	/* 13   =	*/
    0x1e,	/* 14   BACKSPACE	*/
    0x26,	/* 15   TAB	*/
    0x27,	/* 16   Q	*/
    0x28,	/* 17   W	*/
    0x29,	/* 18   E	*/
    0x2a,	/* 19   R	*/
    0x2b,	/* 20   T	*/
    0x2c,	/* 21   Y	*/
    0x2d,	/* 22   U	*/
    0x2e,	/* 23   I	*/
    0x2f,	/* 24   O	*/
    0x30,	/* 25   P	*/
    0x31,	/* 26   [ {	*/
    0x32,	/* 27   ] }	*/
    0x47,	/* 28   ENTER (RETURN)	*/
    0x5c,	/* 29   LEFT CONTROL	*/
    0x3c,	/* 30   A	*/
    0x3d,	/* 31   S	*/
    0x3e,	/* 32   D	*/
    0x3f,	/* 33   F	*/
    0x40,	/* 34   G	*/
    0x41,	/* 35   H	*/
    0x42,	/* 36   J	*/
    0x43,	/* 37   K	*/
    0x44,	/* 38   L	*/
    0x45,	/* 39   ; :	*/
    0x46,	/* 40   ' "	*/
    0x11,	/* 41   ` ~	*/
    0x4b,	/* 42   LEFT SHIFT	*/
    0x33,	/* 43 	NOTE : This key code was not defined in the original table! (' *)	*/
    0x4c,	/* 44   Z	*/
    0x4d,	/* 45   X	*/
    0x4e,	/* 46   C	*/
    0x4f,	/* 47   V	*/
    0x50,	/* 48   B	*/
    0x51,	/* 49   N	*/
    0x52,	/* 50   M	*/
    0x53,	/* 51   , <	*/
    0x54,	/* 52   . >	*/
    0x55,	/* 53   / ?	*/
    0x56,	/* 54   RIGHT SHIFT	*/
    0x24,	/* 55   *            (KEYPAD)	*/
    0x5d,	/* 56   LEFT ALT	*/
    0x5e,	/* 57   SPACEBAR	*/
    0x3b,	/* 58   CAPSLOCK	*/
    0x02,	/* 59   F1	*/
    0x03,	/* 60   F2	*/
    0x04,	/* 61   F3	*/
    0x05,	/* 62   F4	*/
    0x06,	/* 63   F5	*/
    0x07,	/* 64   F6	*/
    0x08,	/* 65   F7	*/
    0x09,	/* 66   F8	*/
    0x0a,	/* 67   F9	*/
    0x0b,	/* 68   F10	*/
    0x22,	/* 69   NUMLOCK      (KEYPAD)	*/
    0x0f,	/* 70   SCROLL LOCK	*/
    0x37,	/* 71   7 HOME       (KEYPAD)	*/
    0x38,	/* 72   8 UP         (KEYPAD)	*/
    0x39,	/* 73   9 PGUP       (KEYPAD)	*/
    0x25,	/* 74   -            (KEYPAD)	*/
    0x48,	/* 75   4 LEFT       (KEYPAD)	*/
    0x49,	/* 76   5            (KEYPAD)	*/
    0x4a,	/* 77   6 RIGHT      (KEYPAD)	*/
    0x3a,	/* 78   +            (KEYPAD)	*/
    0x58,	/* 79   1 END        (KEYPAD)	*/
    0x59,	/* 80   2 DOWN       (KEYPAD)	*/
    0x5a,	/* 81   3 PGDN       (KEYPAD)	*/
    0x64,	/* 82   0 INSERT     (KEYPAD)	*/
    0x65,	/* 83   . DEL        (KEYPAD)	*/
    0x7e,	/* 84		SYSRQ	*/
    0x00,	/* 85	*/
    0x69,	/* 86 	NOTE : This key code was not defined in the original table! (< >)	*/
    0x0c,	/* 87   F11	*/
    0x0d	/* 88   F12	*/
};

static const uint8 g_anExtRawKeyTab[] =
{
    0x00,	/*	0	*/
    0x00,	/*	1	*/
    0x00,	/*		2	*/
    0x00,	/*		3	*/
    0x00,	/*		4	*/
    0x00,	/*		5	*/
    0x00,	/*		6	*/
    0x00,	/*		7	*/
    0x00,	/*		8	*/
    0x00,	/*		9	*/
    0x00,	/*		10	*/
    0x00,	/*		11	*/
    0x00,	/*		12	*/
    0x00,	/*		13	*/
    0x00,	/*		14	*/
    0x00,	/*		15	*/
    0x00,	/*		16	*/
    0x00,	/*		17	*/
    0x00,	/*		18	*/
    0x00,	/*		19	*/
    0x00,	/*		20	*/
    0x00,	/*		21	*/
    0x00,	/*		22	*/
    0x00,	/*		23	*/
    0x00,	/*		24	*/
    0x00,	/*		25	*/
    0x00,	/*		26	*/
    0x00,	/*		27	*/
    0x5b,	/*		28   ENTER        (KEYPAD)	*/
    0x60,	/*		29   RIGHT CONTROL	*/
    0x00,	/*		30	*/
    0x00,	/*		31	*/
    0x00,	/*		32	*/
    0x00,	/*		33	*/
    0x00,	/*		34	*/
    0x00,	/*		35	*/
    0x00,	/*		36	*/
    0x00,	/*		37	*/
    0x00,	/*		38	*/
    0x00,	/*		39	*/
    0x00,	/*		40	*/
    0x00,	/*		41	*/
    0x00,	/*		42   PRINT SCREEN (First code)	*/
    0x00,	/*		43	*/
    0x00,	/*		44	*/
    0x00,	/*		45	*/
    0x00,	/*		46	*/
    0x00,	/*		47	*/
    0x00,	/*		48	*/
    0x00,	/*		49	*/
    0x00,	/*		50	*/
    0x00,	/*		51	*/
    0x00,	/*		52	*/
    0x23,	/*		53   /            (KEYPAD)	*/
    0x00,	/*		54	*/
    0x0e,	/*		55   PRINT SCREEN (Second code)	*/
    0x5f,	/*		56   RIGHT ALT	*/
    0x00,	/*		57	*/
    0x00,	/*		58	*/
    0x00,	/*		59	*/
    0x00,	/*		60	*/
    0x00,	/*		61	*/
    0x00,	/*		62	*/
    0x00,	/*		63	*/
    0x00,	/*		64	*/
    0x00,	/*		65	*/
    0x00,	/*		66	*/
    0x00,	/*		67	*/
    0x00,	/*		68	*/
    0x00,	/*		69	*/
    0x7f,	/*		70	 BREAK	*/
    0x20,	/*		71   HOME         (NOT KEYPAD)	*/
    0x57,	/*		72   UP           (NOT KEYPAD)	*/
    0x21,	/*		73   PAGE UP      (NOT KEYPAD)	*/
    0x00,	/*		74	*/
    0x61,	/*		75   LEFT         (NOT KEYPAD)	*/
    0x00,	/*		76	*/
    0x63,	/*		77   RIGHT        (NOT KEYPAD)	*/
    0x00,	/*		78	*/
    0x35,	/*		79   END          (NOT KEYPAD)	*/
    0x62,	/*		80   DOWN         (NOT KEYPAD)	*/
    0x36,	/*		81   PAGE DOWN    (NOT KEYPAD)	*/
    0x1f,	/*		82   INSERT       (NOT KEYPAD)	*/
    0x34,	/*		83   DELETE       (NOT KEYPAD)	*/
    0x00,	/*		84	*/
    0x00,	/*		85	*/
    0x00,	/*		86	*/
    0x00,	/*		87	*/
    0x00,	/*		88	*/
    0x00,	/*		89	*/
    0x00,	/*		90	*/
    0x00,	/*		91	*/
    0x00,	/*		92	*/
    0x00,	/*		93	*/
    0x00,	/*		94	*/
    0x00,	/*		95	*/
    0x00,	/*		96	*/
    0x00,	/*		97	*/
    0x00,	/*		98	*/
    0x00,	/*		99	*/
    0x00,	/*		100	*/
    0x00,	/*		101	*/
    0x00,	/*		102	*/
    0x00,	/*		103	*/
    0x00,	/*		104	*/
    0x00,	/*		105	*/
    0x00,	/*		106	*/
    0x00,	/*		107	*/
    0x00,	/*		108	*/
    0x00,	/*		109	*/
    0x00,	/*		110	*/
    0x00,	/*		111   MACRO	*/
    0x00,	/*		112	*/
    0x00,	/*		113	*/
    0x00,	/*		114	*/
    0x00,	/*		115	*/
    0x00,	/*		116	*/
    0x00,	/*		117	*/
    0x00,	/*		118	*/
    0x00,	/*		119	*/
    0x00,	/*		120	*/
    0x00,	/*		121	*/
    0x00,	/*		122	*/
    0x00,	/*		123	*/
    0x00,	/*		124	*/
    0x00,	/*		125	*/
    0x00,	/*		126	*/
    0x00,	/*		127	*/
    0x00,	/*		128	*/
    0x00,	/*		129	*/

    0x00,	/*		130	*/
    0x00,	/*		131	*/
    0x00,	/*		132	*/
    0x00,	/*		133	*/
    0x00,	/*		134	*/
    0x00,	/*		135	*/
    0x00,	/*		136	*/
    0x00,	/*		137	*/
    0x00,	/*		138	*/
    0x00,	/*		139	*/
    0x00,	/*		130	*/

    0x00,	/*		1	*/
    0x00,	/*		2	*/
    0x00,	/*		3	*/
    0x00,	/*		4	*/
    0x00,	/*		5	*/
    0x00,	/*		6	*/
    0x00,	/*		7	*/
    0x00,	/*		8	*/
    0x00,	/*		9	*/

    0x00,	/*		150	*/
    0x00,	/*		151	*/
    0x00,	/*		152	*/
    0x00,	/*		153	*/
    0x00,	/*		154	*/
    0x00,	/*		155	*/
    0x00,	/*		156	*/
    0x00,	/*		157	*/
    0x00,	/*		158	*/
    0x00,	/*		159	*/

    0x00,	/*		0	*/
    0x00,	/*		1	*/
    0x00,	/*		2	*/
    0x00,	/*		3	*/
    0x00,	/*		4	*/
    0x00,	/*		5	*/
    0x00,	/*		6	*/
    0x00,	/*		7	*/
    0x00,	/*		8	*/
    0x00,	/*		9	*/

    0x00,	/*		170	*/
    0x00,	/*		171	*/
    0x00,	/*		172	*/
    0x00,	/*		173	*/
    0x00,	/*		174	*/
    0x00,	/*		175	*/
    0x00,	/*		176	*/
    0x00,	/*		177	*/
    0x00,	/*		178	*/
    0x00,	/*		179	*/

    0x00,	/*		0	*/
    0x00,	/*		1	*/
    0x00,	/*		2	*/
    0x00,	/*		3	*/
    0x00,	/*		4	*/
    0x00,	/*		5	*/
    0x00,	/*		6	*/
    0x00,	/*		7	*/
    0x00,	/*		8	*/
    0x00,	/*		9	*/

    0x00,	/*		190	*/
    0x00,	/*		191	*/
    0x00,	/*		192	*/
    0x00,	/*		193	*/
    0x00,	/*		194	*/
    0x00,	/*		195	*/
    0x00,	/*		196	*/
    0x00,	/*		197	*/
    0x00,	/*		198	*/
    0x00,	/*		199	*/

    0x00,	/*		0	*/
    0x00,	/*		1	*/
    0x00,	/*		2	*/
    0x00,	/*		3	*/
    0x00,	/*		4	*/
    0x00,	/*		5	*/
    0x00,	/*		6	*/
    0x00,	/*		7	*/
    0x00,	/*		8	*/
    0x00,	/*		9	*/

    0x00,	/*		210	*/
    0x00,	/*		211	*/
    0x00,	/*		212	*/
    0x00,	/*		213	*/
    0x00,	/*		214	*/
    0x00,	/*		215	*/
    0x00,	/*		216	*/
    0x00,	/*		217	*/
    0x00,	/*		218	*/
    0x00,	/*		219	*/

    0x00,	/*		0	*/
    0x00,	/*		1	*/
    0x00,	/*		2	*/
    0x00,	/*		3	*/
    0x00,	/*		4	*/
    0x00,	/*		5	*/
    0x00,	/*		6	*/
    0x00,	/*		7	*/
    0x00,	/*		8	*/
    0x00,	/*		9	*/

    0x00,	/*		230	*/
    0x00,	/*		231	*/
    0x00,	/*		232	*/
    0x00,	/*		233	*/
    0x00,	/*		234	*/
    0x00,	/*		235	*/
    0x00,	/*		236	*/
    0x00,	/*		237	*/
    0x00,	/*		238	*/
    0x00,	/*		239	*/

    0x00,	/*		0	*/
    0x00,	/*		1	*/
    0x00,	/*		2	*/
    0x00,	/*		3	*/
    0x00,	/*		4	*/
    0x00,	/*		5	*/
    0x00,	/*		6	*/
    0x00,	/*		7	*/
    0x00,	/*		8	*/
    0x00,	/*		9	*/

    0x00,	/*		250	*/
    0x00,	/*		251	*/
    0x00,	/*		252	*/
    0x00,	/*		253	*/
    0x00,	/*		254	*/
    0x00,	/*		255	*/
    0x00,	/*		256	*/
};

#endif
 
 


