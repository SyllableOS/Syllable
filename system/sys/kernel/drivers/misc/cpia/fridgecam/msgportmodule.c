//-----------------------------------------------------------------------------
#include <stdio.h>
#include <python1.5/Python.h>
//-------------------------------------
#include <atheos/msgport.h>
//-----------------------------------------------------------------------------

typedef struct
{
    PyObject_HEAD;
    bool        own;
    port_id	port;
} MsgPortObject;

static void msgport_dealloc( MsgPortObject *cpia );
static PyObject *msgport_getattr( MsgPortObject *self, char *name );

static PyTypeObject MsgPortType =
{
    PyObject_HEAD_INIT( &PyType_Type )
    0,					// ob_size
    "msgport",				// tp_name
    sizeof(MsgPortObject),		// tp_size
    0,					// tp_itemsize

      // methods:
    (destructor)msgport_dealloc,	// tp_dealloc
    NULL,				// tp_print
    (getattrfunc)msgport_getattr,	// tp_getattr
    NULL,				// tp_setattr
    NULL,				// tp_compare
    NULL,				// tp_repr
    NULL,				// tp_as_number
    NULL,				// tp_as_sequence
    NULL,				// tp_as_mapping
    NULL,				// tp_hash
    NULL,				// tp_call
    NULL,				// tp_str
    NULL,				// tp_getattro
    NULL,				// tp_setattro
    NULL,				// tp_as_buffer
    0,					// tp_flags
    NULL,				// tp_doc
};     

static PyObject *msgport_error;

//-----------------------------------------------------------------------------

static MsgPortObject *msgport_new( const char *name, int maxcount )
{
    MsgPortObject *msgport;

    msgport = PyObject_NEW( MsgPortObject, &MsgPortType );
    if( msgport == NULL )
	return NULL;

    msgport->own = true;
    msgport->port = create_port( name, maxcount );
    if( msgport->port < 0 )
    {
	PyMem_DEL( msgport );
	return NULL;
    }

    return msgport;
}

static MsgPortObject *msgport_new_existing( int port )
{
    MsgPortObject *msgport;

    msgport = PyObject_NEW( MsgPortObject, &MsgPortType );
    if( msgport == NULL )
	return NULL;

    msgport->own = false;
    msgport->port = port;

    return msgport;
}

static void msgport_dealloc( MsgPortObject *msgport )
{
    if( msgport->own )
    {
	delete_port( msgport->port );
    }
    PyMem_DEL( msgport );
}

//-----------------------------------------------------------------------------

static PyObject *msgport_portid( MsgPortObject *self, PyObject *args )
{
    if( !PyArg_ParseTuple(args, "") )
	return NULL;

    return Py_BuildValue( "i", self->port );
}

static PyObject *msgport_send( MsgPortObject *self, PyObject *args )
{
    unsigned int code;
    const char *data;
    int datasize;

    if( !PyArg_ParseTuple(args, "is#",&code,&data,&datasize) )
	return NULL;

//    printf( "send_msg %d bytes to %d\n", datasize, self->port );
    if( send_msg(self->port,code,data,datasize) < 0 )
    {
	PyErr_Format( msgport_error, "could not send data" );
	return NULL;
    }	

    Py_INCREF( Py_None );
    return Py_None;
}

static PyObject *msgport_get( MsgPortObject *self, PyObject *args )
{
    int size = 1024*1024; // a silly maxsize...
    unsigned long code;
    char *buffer = NULL;
    PyObject *ret;

    if( !PyArg_ParseTuple(args, "|i",&size) )
	return NULL;

    buffer = malloc( size );
    if( buffer == NULL )
    {
	PyErr_Format( msgport_error, "out of memory" );
	return NULL;
    }
    
      // TODO: get_msg does not return the actual message size :(
    if( get_msg(self->port,&code,buffer,size) < 0 )
    {
	PyErr_Format( msgport_error, "could not read data (it might be too big)" );
	free( buffer );
	return NULL;
    }	

    ret = Py_BuildValue( "is#", code, buffer,size );
    free( buffer );

    return ret;
}

//-----------------------------------------------------------------------------

static PyMethodDef msgport_methods[] =
{
    { "portid", (PyCFunction)msgport_portid, METH_VARARGS, NULL },
    { "send",   (PyCFunction)msgport_send,   METH_VARARGS, NULL },
    { "get",    (PyCFunction)msgport_get,    METH_VARARGS, NULL },
    { NULL,     NULL }
};

static PyObject *msgport_getattr( MsgPortObject *self, char *name )
{
    return Py_FindMethod( msgport_methods, (PyObject*)self, name );
}

//-----------------------------------------------------------------------------

static PyObject *msgportmodule_new( PyObject *self, PyObject *args )
{
    char *name;
    int maxcount;
    int port;
    MsgPortObject *msgport;

    if( PyArg_ParseTuple(args, "si",&name,&maxcount) )
	msgport = msgport_new( name, maxcount );
    else if( PyArg_ParseTuple(args, "i",&port) )
	msgport = msgport_new_existing( port );
    else
	return NULL;

    return (PyObject*)msgport;
}  

//-----------------------------------------------------------------------------

PyMethodDef msgportmodule_methods[] =
{
    { "msgport", msgportmodule_new, METH_VARARGS, NULL },
    { NULL,      NULL }
};

void initmsgport() 
{
    PyObject *module = Py_InitModule( "msgport", msgportmodule_methods );
    PyObject *module_dict = PyModule_GetDict( module );


    Py_INCREF( &MsgPortType );
    
    PyDict_SetItemString( module_dict, "MsgPortType", (PyObject *)&MsgPortType );

    
    msgport_error = PyErr_NewException( "msgport.error", NULL, NULL );
    PyDict_SetItemString( module_dict, "error", msgport_error );
} 

//-----------------------------------------------------------------------------

