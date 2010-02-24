# Generate security keys for the OpenSSH server:

# Legacy RSA1 key for version 1 of the SSH protocol:
#if [ ! -f /etc/ssh/ssh_host_key ]
#then
#	/resources/indexes/programs/ssh-keygen -t rsa1 -b 1024 -f /etc/ssh/ssh_host_key -N ''
#	chmod 600 /etc/ssh/ssh_host_key
#fi

if [ ! -f /etc/ssh/ssh_host_dsa_key ]
then
	/resources/indexes/programs/ssh-keygen -d -f /etc/ssh/ssh_host_dsa_key -N ''
	chmod 600 /etc/ssh/ssh_host_dsa_key
fi
if [ ! -f /etc/ssh/ssh_host_rsa_key ]
then
	/resources/indexes/programs/ssh-keygen -t rsa -f /etc/ssh/ssh_host_rsa_key -N ''
	chmod 600 /etc/ssh/ssh_host_rsa_key
fi

# -4: IPv4, don't use IPv6
# -r: don't call socketpair()
# -e: log to standard error output
# -D: don't detach
# -d[dd]: debug mode
# -t: test mode
# -T: extended test mode
# Uncomment the following to start the OpenSSH server.
# Please be aware that Syllable Desktop does not have the needed security to make this safe:
#/resources/indexes/system-programs/sshd -4 -r -o 'UsePrivilegeSeparation no'
