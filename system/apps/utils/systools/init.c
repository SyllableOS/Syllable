#include <stdio.h>
#include <unistd.h>

int main()
{
    symlink( "boot/atheos", "/atheos" );
    symlink( "atheos/sys", "/system" );
    symlink( "atheos/sys/bin", "/bin" );
    symlink( "atheos/usr", "/usr" );
    symlink( "atheos/etc", "/etc" );
    symlink( "atheos/var", "/var" );
    symlink( "atheos/home", "/home" );
    symlink( "atheos/tmp", "/tmp" );

    if ( fork() == 0 ) {
	execl( "/system/appserver", "appserver", NULL );
	exit(1);
    }
    
    execl( "/system/init.sh", "init.sh", "normal", NULL );
    printf( "Failed to run init script!\n" );
    return( 1 );
}
