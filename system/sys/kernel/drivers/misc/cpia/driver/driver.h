// Copyright 1999 Jesper Hansen. Free for use under the Gnu Public License
#ifndef CPIAPPCDRIVER_DRIVER_H
#define CPIAPPCDRIVER_DRIVER_H
//-----------------------------------------------------------------------------
//-------------------------------------
#include <atheos/kdebug.h>
#include <atheos/isa_io.h>
//-------------------------------------
//-----------------------------------------------------------------------------

//#define ERROR(_x) 
#define ERROR(_x) printk _x

//#define WARNING(_x) 
#define WARNING(_x) printk _x

#define INFO(_x) 
//#define INFO(_x) printk _x

#define FUNC(_x) 
//#define FUNC(_x) printk _x

//-----------------------------------------------------------------------------

#define TOUCH(x) ((void)(x))

//#define _inp(_port) (isa->read_io_8(_port))
//#define _outp(_port,_data) (isa->write_io_8(_port, _data))

#define _inp(_port) inb(_port)
#define _outp(_port,_data) outb(_data,_port)

//-----------------------------------------------------------------------------
#endif
