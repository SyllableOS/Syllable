
extern int   base_address;	// Offset between virtual and physical addresses.
extern char* first_free_addr;	// First address after the first level loader.
extern char* loader_args;	// Argument string passed to the first level loader
extern char* RealPage;

#define LIN_TO_PHYS( a ) ((void*)(((char*)(a)) + base_address))
#define PHYS_TO_LIN( a ) ((void*)(((char*)(a)) - base_address))


int load_kernel_image( int nFile, void** ppEntry, int* pnKernelSize );
