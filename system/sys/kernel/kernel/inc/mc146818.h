
/* mc146818rtc.h - register definitions for the Real-Time-Clock / CMOS RAM
 * Copyright Torsten Duwe <duwe@informatik.uni-erlangen.de> 1993
 * derived from Data Sheet, Copyright Motorola 1984 (!).
 * It was written to be part of the Linux operating system.
 */

/* permission is hereby granted to copy, modify and redistribute this code
 * in terms of the GNU Library General Public License, Version 2 or later,
 * at your option.
 */

#ifndef _MC146818RTC_H
#define _MC146818RTC_H
#include <atheos/isa_io.h>

#ifdef __cplusplus
extern "C"
{
#if 0
}				/*make emacs indention work */
#endif
#endif


#ifndef RTC_PORT
#define RTC_PORT(x)	(0x70 + (x))
static const bool RTC_ALWAYS_BCD = true;
#endif

static inline uint8_t CMOS_READ( const uint8_t addr )
{
	outb_p( (addr), RTC_PORT(0) );
	return inb_p( RTC_PORT(1) );
}

static inline void CMOS_WRITE( const uint8_t val, const uint8_t addr )
{
	outb_p( (addr), RTC_PORT(0) );
	outb_p( (val), RTC_PORT(1) );
}

/**********************************************************************
 * register summary
 **********************************************************************/
static const uint8_t RTC_SECONDS	= 0;
static const uint8_t RTC_SECONDS_ALARM	= 1;
static const uint8_t RTC_MINUTES	= 2;
static const uint8_t RTC_MINUTES_ALARM	= 3;
static const uint8_t RTC_HOURS		= 4;
static const uint8_t RTC_HOURS_ALARM	= 5;

/* RTC_*_alarm is always true if 2 MSBs are set */
static const uint8_t RTC_ALARM_DONT_CARE = 0xC0;

static const uint8_t RTC_DAY_OF_WEEK	= 6;
static const uint8_t RTC_DAY_OF_MONTH	= 7;
static const uint8_t RTC_MONTH		= 8;
static const uint8_t RTC_YEAR		= 9;

/* control registers - Moto names
 */
static const uint8_t RTC_REG_A		= 10;
static const uint8_t RTC_REG_B		= 11;
static const uint8_t RTC_REG_C		= 12;
static const uint8_t RTC_REG_D		= 13;

/**********************************************************************
 * register details
 **********************************************************************/
#define RTC_FREQ_SELECT		RTC_REG_A

/* update-in-progress  - set to "1" 244 microsecs before RTC goes off the bus,
 * reset after update (may take 1.984ms @ 32768Hz RefClock) is complete,
 * totalling to a max high interval of 2.228 ms.
 */
static const uint8_t RTC_UIP		= 0x80;
static const uint8_t RTC_DIV_CTL	= 0x70;
   /* divider control: refclock values 4.194 / 1.049 MHz / 32.768 kHz */
static const uint8_t RTC_REF_CLCK_4MHZ	= 0x00;
static const uint8_t RTC_REF_CLCK_1MHZ	= 0x10;
static const uint8_t RTC_REF_CLCK_32KHZ = 0x20;
   /* 2 values for divider stage reset, others for "testing purposes only" */
static const uint8_t RTC_DIV_RESET1	= 0x60;
static const uint8_t RTC_DIV_RESET2	= 0x70;
  /* Periodic intr. / Square wave rate select. 0=none, 1=32.8kHz,... 15=2Hz */
static const uint8_t RTC_RATE_SELECT 	= 0x0F;

/**********************************************************************/
#define RTC_CONTROL		RTC_REG_B

static const uint8_t RTC_SET		= 0x80;	// disable updates for clock setting
static const uint8_t RTC_PIE		= 0x40;	// periodic interrupt enable
static const uint8_t RTC_AIE		= 0x20;	// alarm interrupt enable
static const uint8_t RTC_UIE		= 0x10;	// update-finished interrupt enable
static const uint8_t RTC_SQWE		= 0x08;	// enable square-wave output
static const uint8_t RTC_DM_BINARY	= 0x04;	// all time/date values are BCD if clear
static const uint8_t RTC_24H		= 0x02;	// 24 hour mode - else hours bit 7 means pm
static const uint8_t RTC_DST_EN	= 0x01;	// auto switch DST - works f. USA only

/**********************************************************************/
#define RTC_INTR_FLAGS		RTC_REG_C

/* caution - cleared by read */
static const uint8_t RTC_IRQF		= 0x80;	// any of the following 3 is active
static const uint8_t RTC_PF		= 0x40;
static const uint8_t RTC_AF		= 0x20;
static const uint8_t RTC_UF		= 0x10;

/**********************************************************************/
#define RTC_VALID		RTC_REG_D

static const uint8_t RTC_VRT		= 0x80;		/* valid RAM and time */

/**********************************************************************/

/* example: !(CMOS_READ(RTC_CONTROL) & RTC_DM_BINARY)
 * determines if the following two #defines are needed
 */
#ifndef BCD_TO_BIN
#define BCD_TO_BIN(val) ((val)=((val)&15) + ((val)>>4)*10)
#endif

#ifndef BIN_TO_BCD
#define BIN_TO_BCD(val) ((val)=(((val)/10)<<4) + (val)%10)
#endif

/*
 * The struct used to pass data via the following ioctl. Similar to the
 * struct tm in <time.h>, but it needs to be here so that the kernel
 * source is self contained, allowing cross-compiles, etc. etc.
 */

struct rtc_time
{
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

/*
 * ioctl calls that are permitted to the /dev/rtc interface, if
 * CONFIG_RTC was enabled.
 */

#define RTC_AIE_ON	_IO('p', 0x01)	/* Alarm int. enable on         */
#define RTC_AIE_OFF	_IO('p', 0x02)	/* ... off                      */
#define RTC_UIE_ON	_IO('p', 0x03)	/* Update int. enable on        */
#define RTC_UIE_OFF	_IO('p', 0x04)	/* ... off                      */
#define RTC_PIE_ON	_IO('p', 0x05)	/* Periodic int. enable on      */
#define RTC_PIE_OFF	_IO('p', 0x06)	/* ... off                      */

#define RTC_ALM_SET	_IOW('p', 0x07, struct rtc_time)	/* Set alarm time  */
#define RTC_ALM_READ	_IOR('p', 0x08, struct rtc_time)	/* Read alarm time */
#define RTC_RD_TIME	_IOR('p', 0x09, struct rtc_time)	/* Read RTC time   */
#define RTC_SET_TIME	_IOW('p', 0x0a, struct rtc_time)	/* Set RTC time    */
#define RTC_IRQP_READ	_IOR('p', 0x0b, unsigned long)	/* Read IRQ rate   */
#define RTC_IRQP_SET	_IOW('p', 0x0c, unsigned long)	/* Set IRQ rate    */


#ifdef __cplusplus
}
#endif

#endif /* _MC146818RTC_H */
