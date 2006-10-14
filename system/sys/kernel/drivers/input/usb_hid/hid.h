#ifndef _HID_H_
#define _HID_H_



/*
 * Special tag indicating long items
 */

#define HID_ITEM_TAG_LONG	15


/*
 * HID report descriptor item type (prefix bit 2,3)
 */

#define HID_ITEM_TYPE_MAIN		0
#define HID_ITEM_TYPE_GLOBAL		1
#define HID_ITEM_TYPE_LOCAL		2
#define HID_ITEM_TYPE_RESERVED		3

/*
 * HID report descriptor main item tags
 */

#define HID_MAIN_ITEM_TAG_INPUT			8
#define HID_MAIN_ITEM_TAG_OUTPUT		9
#define HID_MAIN_ITEM_TAG_FEATURE		11
#define HID_MAIN_ITEM_TAG_BEGIN_COLLECTION	10
#define HID_MAIN_ITEM_TAG_END_COLLECTION	12

/*
 * HID report descriptor main item contents
 */

#define HID_MAIN_ITEM_CONSTANT		0x001
#define HID_MAIN_ITEM_VARIABLE		0x002
#define HID_MAIN_ITEM_RELATIVE		0x004
#define HID_MAIN_ITEM_WRAP		0x008	
#define HID_MAIN_ITEM_NONLINEAR		0x010
#define HID_MAIN_ITEM_NO_PREFERRED	0x020
#define HID_MAIN_ITEM_NULL_STATE	0x040
#define HID_MAIN_ITEM_VOLATILE		0x080
#define HID_MAIN_ITEM_BUFFERED_BYTE	0x100

/*
 * HID report descriptor collection item types
 */

#define HID_COLLECTION_PHYSICAL		0
#define HID_COLLECTION_APPLICATION	1
#define HID_COLLECTION_LOGICAL		2

/*
 * HID report descriptor global item tags
 */

#define HID_GLOBAL_ITEM_TAG_USAGE_PAGE		0
#define HID_GLOBAL_ITEM_TAG_LOGICAL_MINIMUM	1
#define HID_GLOBAL_ITEM_TAG_LOGICAL_MAXIMUM	2
#define HID_GLOBAL_ITEM_TAG_PHYSICAL_MINIMUM	3
#define HID_GLOBAL_ITEM_TAG_PHYSICAL_MAXIMUM	4
#define HID_GLOBAL_ITEM_TAG_UNIT_EXPONENT	5
#define HID_GLOBAL_ITEM_TAG_UNIT		6
#define HID_GLOBAL_ITEM_TAG_REPORT_SIZE		7
#define HID_GLOBAL_ITEM_TAG_REPORT_ID		8
#define HID_GLOBAL_ITEM_TAG_REPORT_COUNT	9
#define HID_GLOBAL_ITEM_TAG_PUSH		10
#define HID_GLOBAL_ITEM_TAG_POP			11

/*
 * HID report descriptor local item tags
 */

#define HID_LOCAL_ITEM_TAG_USAGE		0
#define HID_LOCAL_ITEM_TAG_USAGE_MINIMUM	1
#define HID_LOCAL_ITEM_TAG_USAGE_MAXIMUM	2
#define HID_LOCAL_ITEM_TAG_DESIGNATOR_INDEX	3
#define HID_LOCAL_ITEM_TAG_DESIGNATOR_MINIMUM	4
#define HID_LOCAL_ITEM_TAG_DESIGNATOR_MAXIMUM	5
#define HID_LOCAL_ITEM_TAG_STRING_INDEX		7
#define HID_LOCAL_ITEM_TAG_STRING_MINIMUM	8
#define HID_LOCAL_ITEM_TAG_STRING_MAXIMUM	9
#define HID_LOCAL_ITEM_TAG_DELIMITER		10

/*
 * HID usage tables
 */

#define HID_USAGE_PAGE		0xffff0000

#define HID_UP_GENDESK 		0x00010000
#define HID_UP_KEYBOARD 	0x00070000
#define HID_UP_LED 		0x00080000
#define HID_UP_BUTTON 		0x00090000
#define HID_UP_CONSUMER		0x000c0000
#define HID_UP_DIGITIZER 	0x000d0000
#define HID_UP_PID 		0x000f0000

#define HID_USAGE		0x0000ffff

#define HID_GD_POINTER		0x00010001
#define HID_GD_MOUSE		0x00010002
#define HID_GD_JOYSTICK		0x00010004
#define HID_GD_GAMEPAD		0x00010005
#define HID_GD_KEYBOARD		0x00010006
#define HID_GD_KEYPAD		0x00010007
#define HID_GD_MULTIAXIS	0x00010008
#define HID_GD_X		0x00010030
#define HID_GD_Y		0x00010031
#define HID_GD_Z		0x00010032
#define HID_GD_RX		0x00010033
#define HID_GD_RY		0x00010034
#define HID_GD_RZ		0x00010035
#define HID_GD_SLIDER		0x00010036
#define HID_GD_DIAL		0x00010037
#define HID_GD_WHEEL		0x00010038
#define HID_GD_HATSWITCH	0x00010039
#define HID_GD_BUFFER		0x0001003a
#define HID_GD_BYTECOUNT	0x0001003b
#define HID_GD_MOTION		0x0001003c
#define HID_GD_START		0x0001003d
#define HID_GD_SELECT		0x0001003e
#define HID_GD_VX		0x00010040
#define HID_GD_VY		0x00010041
#define HID_GD_VZ		0x00010042
#define HID_GD_VBRX		0x00010043
#define HID_GD_VBRY		0x00010044
#define HID_GD_VBRZ		0x00010045
#define HID_GD_VNO		0x00010046
#define HID_GD_FEATURE		0x00010047
#define HID_GD_UP		0x00010090
#define HID_GD_DOWN		0x00010091
#define HID_GD_RIGHT		0x00010092
#define HID_GD_LEFT		0x00010093




struct HID_class_descriptor_s {
	uint8  nDescriptorType;
	uint16 nDescriptorLength;
} __attribute__ ((packed));

struct HID_descriptor_s
{
	uint8  bLength;
	uint8  nDescriptorType;
	uint16 bcdHID;
	uint8 bCountryCode;
	uint8  nNumDescriptors;

	struct HID_class_descriptor_s asDesc[1];
} __attribute__ ((packed));

struct HID_field_s
{
	uint32 nFlags;
	uint32 nOffset;
	uint32 nSize;
	uint32 nCount;
	int32 nLogMin;
	int32 nLogMax;
	int32 nPhyMin;
	int32 nPhyMax;
	uint32* pLastValues;
	uint32 nNumUsages;
	uint32 anUsage[0];
};

struct HID_report_s
{
	uint32 nSize;
	struct HID_field_s* apsField[64];
	uint32 nNumFields;
	uint8* pBuffer;
	uint32 nCurrentOffset;
};

struct USB_hid_s
{
	signed char anData[8];
	char zName[128];
	bool bNumberedData;
	struct HID_report_s asReport[256];
	USB_device_s *psDev;
	USB_packet_s sIRQ;
	int nOpen;
	int nMaxpacket;
	
};

/* HID keycodes */
static unsigned char g_anHidKeyboard[256] = {
	  0,  0,  0,  0, 30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38,
	 50, 49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44,  2,  3,
	  4,  5,  6,  7,  8,  9, 10, 11, 28,  1, 14, 15, 57, 12, 13, 26,
	 27, 43, 84, 39, 40, 41, 51, 52, 53, 58, 59, 60, 61, 62, 63, 64,
	 65, 66, 67, 68, 87, 88, 99, 70,119,110,102,104,111,107,109,106,
	105,108,103, 69, 98, 55, 74, 78, 96, 79, 80, 81, 75, 76, 77, 71,
	 72, 73, 82, 83, 86,127,116,117, 85, 89, 90, 91, 92, 93, 94, 95,
	120,121,122,123,134,138,130,132,128,129,131,137,133,135,136,113,
	115,114,  0,  0,  0,124,  0,181,182,183,184,185,186,187,188,189,
	190,191,192,193,194,195,196,197,198,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 29, 42, 56,125, 97, 54,100,126,164,166,165,163,161,115,114,113,
	150,158,159,128,136,177,178,176,142,152,173,140,  0,  0,  0,  0
};

/* Translated AT key table (modified for USB keyboards ) */

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
    0x33,	/* 84	???	*/
    0x00,	/* 85	*/
    0x69,	/* 86 	NOTE : This key code was not defined in the original table! (< >)	*/
    0x0c,	/* 87   F11	*/
    0x0d,	/* 88   F12	*/
    0, /* 89 */
    0, /* 90 */
    0, /* 91 */
    0, /* 92 */
    0, /* 93 */
    0, /* 94 */
    0, /* 95 */
    0x5b, /* 96   ENTER */
    0x60, /* 97   RIGHT CONTROL */
    0, /* 98 */
    0x0e, /* 99   PRINT SCREEN */
    0x5f, /* 100  RIGHT ALT */
    0, /* 101 */
    0x20, /* 102  HOME */
    0x57, /* 103  UP */
    0x21, /* 104  PAGE UP */
    0x61, /* 105  LEFT */
    0x63, /* 106  RIGHT */
    0x35, /* 107  END */
    0x62, /* 108  DOWN */
    0x36, /* 109  PAGE_DOWN */
    0x1f, /* 110  INSERT */
    0x34, /* 111  DELETE */
    0, /* 112 */
    0, /* 113 */
    0, /* 114 */
    0, /* 115 */
    0, /* 116 */
    0, /* 117 */
    0, /* 118 */
    0x7f, /* 119  BREAK */
    0, /* 120 */
    0, /* 121 */
    0, /* 122 */
    0, /* 123 */
    0, /* 124 */
    0, /* 125 */
    0, /* 126 */
    0, /* 127 */
    0, /* 128 */
    0, /* 129 */
    0 /* 130 */
};


#endif











