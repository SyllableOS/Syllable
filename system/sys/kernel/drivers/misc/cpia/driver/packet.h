/* Copyright 1999 VLSI Vision Ltd. Free for use under the Gnu Public License */



/* packet length for both command and data packets 					*/
#define PACKET_LEN			8

/* reads of data from the camera, over the expected length will be 	*/
/* padded out with the byte below.									*/
#define DUMMY_DATA			0xFF

/* USB Setup packet byte definitions */
#define REQUEST_TYPE		0
#define REQUEST				1
#define VALUE_LO			2
#define VALUE_HI			3
#define INDEX_LO			4
#define INDEX_HI			5
#define LENGTH_LO			6
#define LENGTH_HI			7


/* CPIA use of the above USB Setup packet bytes */
#define CMD_TYPE			REQUEST_TYPE
#define CMD_DEST_ADDR		REQUEST
#define CMD_D0				VALUE_LO
#define CMD_D1				VALUE_HI
#define CMD_D2				INDEX_LO
#define CMD_D3				INDEX_HI
#define CMD_DATA_BYTES_LO  	LENGTH_LO
#define CMD_DATA_BYTES_HI  	LENGTH_HI

/* definition of CMD_TYPE bits */
#define READ_DATA_REQUEST	0x80		/* set for a data read */
#define CAMERA_REQUEST		0x40

/* definition of CMD_END value */
#define END_BYTE			0

