#include "debug.h"

Debug::Debug(const char* cDebug)
{
	if (DEBUG == 1){
		
	
		if (DEBUG_FILE == 1){
			FILE* fpDebug = fopen("/tmp/desktop.deb", "a");
			fprintf(fpDebug, "Debug: \"%s\".\n",cDebug);
			fclose(fpDebug);
	
		}else {
			printf( "Debug: \"%s\".\n",cDebug);
		}
	}
}	



