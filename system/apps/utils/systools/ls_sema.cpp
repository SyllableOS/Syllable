#include <stdio.h>
#include <atheos/semaphore.h>
#include <atheos/threads.h>

#include <util/string.h>

#include <map>
#include <string>



int main()
{
    thread_info sThreadInfo;
    sem_info sInfo;
    std::map<proc_id,std::string> cProcessMap;
    int nError;
    
    printf( "SemaID Holder Count     Name\n" );
    for ( nError = get_thread_info( -1, &sThreadInfo ) ; nError >= 0 ; nError = get_next_thread_info( &sThreadInfo ) ) {
	cProcessMap[sThreadInfo.ti_process_id] = sThreadInfo.ti_process_name;
    }

    for ( std::map<proc_id,std::string>::iterator i = cProcessMap.begin() ; i != cProcessMap.end() ; ++i ) {
	if ( (*i).first == 0 ) {
	    continue;
	}
	
	;
//	int nError = 1;
	bool bFirst = true;
	for ( nError = get_semaphore_info( -1, (*i).first, &sInfo ) == 0 ; nError == 1 ; nError = get_next_semaphore_info( &sInfo ) ) {
	    if ( bFirst ) {
		os::String cName;
		cName.Format( "%s (%d)", (*i).second.c_str(), (*i).first );
		printf( "\n==========================================================================\n" );
		printf( "=== %s %.*s\n", cName.c_str(), 69 - cName.Length(), "=====================================================================" );
		printf( "==========================================================================\n" );
		bFirst = false;
	    }
	    printf( "%-06d %-06d %-08d %s\n", sInfo.si_sema_id, sInfo.si_latest_holder, sInfo.si_count, sInfo.si_name );
	}
	
    }
/*
  printf( "SemaID Owner Holder Count     Name\n" );
  get_semaphore_info( -1, &sInfo );
  int nError = 1;
  printf( "SemaID Owner Holder Count     Name\n" );
  for (  ; nError == 1 ; nError = get_next_semaphore_info( &sInfo ) ) {
  printf( "%-06d %-06d %-06d %-08d %s\n", sInfo.si_sema_id, sInfo.si_owner, sInfo.si_latest_holder, sInfo.si_count, sInfo.si_name );
  }*/
}
