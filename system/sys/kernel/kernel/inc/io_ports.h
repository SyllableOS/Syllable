/*
 *  arch/i386/mach-generic/io_ports.h
 *
 *  Machine specific IO port address definition for generic.
 *  Written by Osamu Tomita <tomita@cinet.co.jp>
 */
#ifndef _MACH_IO_PORTS_H
#define _MACH_IO_PORTS_H

/* i8253A PIT registers */
static const uint8_t PIT_MODE		= 0x43;
static const uint8_t PIT_CH0		= 0x40;
static const uint8_t PIT_CH2		= 0x42;

/* i8259A PIC registers */
static const uint8_t PIC_MASTER_CMD	= 0x20;
static const uint8_t PIC_MASTER_IMR	= 0x21;
#define       PIC_MASTER_ISR	PIC_MASTER_CMD
#define       PIC_MASTER_POLL	PIC_MASTER_CMD
#define       PIC_MASTER_OCW3	PIC_MASTER_CMD
static const uint8_t PIC_SLAVE_CMD	= 0xa0;
static const uint8_t PIC_SLAVE_IMR	= 0xa1;

/* i8259A PIC related value */
static const uint8_t PIC_CASCADE_IR	    = 2;
static const uint8_t MASTER_ICW4_DEFAULT   = 0x01;
static const uint8_t SLAVE_ICW4_DEFAULT    = 0x01;
static const uint8_t PIC_ICW4_AEOI	    = 2;

#endif /* !_MACH_IO_PORTS_H */
