#!/bin/sh
# Perform system specific initialisation
#
# inetd is a special case and is always started first if inetutils is
# installed.

if [ -e /atheos/autolnk/sbin/inetd ]
then
  /atheos/autolnk/sbin/inetd &
fi

# Packages that require initalisation can include an init directory, which
# should contain the init script(s) E.g. Apache would have init/apache which
# would call apachectl, OpenSSH would have init/sshd which would start
# sshd etc.
# The package manager will collect all of these scripts together in
# /atheos/autolnk/init/; all we need to do is run each script in turn.

if [ -e /atheos/autolnk/init ]
then
  for pkg_init in 'ls /atheos/autolnk/init/'
  do
    sh $pkg_init
  done
fi

# Please add any additional configuration below this point

