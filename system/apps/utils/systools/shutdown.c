#include <atheos/syscall.h>

int main(void) 
{
   printf("Shutting down...\n");
   syscall(__NR_apm_poweroff);
}
