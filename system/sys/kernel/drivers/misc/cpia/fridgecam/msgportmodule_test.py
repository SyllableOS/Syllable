#!/usr/bin/env python

import string
import struct
import msgport
import os

fridgecam_portid = string.atoi( open('/tmp/fridgecam.port').readline() )
print 'fridgecam port', fridgecam_portid
fridgecam_port = msgport.msgport( string.atoi(open('/tmp/fridgecam.port').readline()) )
reply_port = msgport.msgport( 'fridgecam reply', 1024 )
print 'reply port', reply_port.portid()

while 1:
	fridgecam_port.send( 0, struct.pack('i',reply_port.portid()) )
	reply_port.get( 4 )

	jpg_frame = open( '/tmp/fridgecam.jpg' ).read()
	print 'Got frame (%d bytes)'%len(jpg_frame)
