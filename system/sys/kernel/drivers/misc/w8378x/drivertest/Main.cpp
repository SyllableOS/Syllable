#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

//#include <kernel/OS.h>

#include "../w8378x_driver.h"

#define snooze Delay

int main()
{
	int hf = open( "/dev/misc/w8378x", O_RDWR );
	printf( "hf = %d\n", hf );

	if ( hf < 0 ) {
	  printf( "Failed to open device: %s\n", strerror( errno ) );
	}
	assert( hf >= 0 );

	while( 1 )
	{
		int temp1=0, temp2=0, temp3=0;
		int fan1=0, fan2=0, fan3=0;

		ioctl( hf, W8378x_READ_TEMP1, &temp1 );
		ioctl( hf, W8378x_READ_TEMP2, &temp2 );
		ioctl( hf, W8378x_READ_TEMP3, &temp3 );

		ioctl( hf, W8378x_READ_FAN1, &fan1 );
		ioctl( hf, W8378x_READ_FAN2, &fan2 );
		ioctl( hf, W8378x_READ_FAN3, &fan3 );

		printf( "Temp1:%.1fc  Temp2:%.1fc  Temp3:%.1fc  Fan1:%drpm  Fan2:%drpm  Fan3:%drpm\n",
			float(temp1)/256.0f, float(temp2)/256.0f, float(temp3)/256.0f,
			fan1, fan2, fan3 );
		
		snooze( 2000000 );
	}


	close( hf );
	return 0;
}
