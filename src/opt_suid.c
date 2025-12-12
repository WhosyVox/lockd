#include <unistd.h>
#include <stdio.h>
#include "opt_suid.h"


root_type test_suid() {
	uid_t euid = geteuid();
	if ( euid != 0 ) return ROOT_NONE;

	setuid( 0 );
	int uid = getuid();
	
	if ( uid != 0 ) return ROOT_EFFECTIVE;
	
	return ROOT_FULL;
}

int opt_suid() {
	int is_suid = test_suid();
	if ( is_suid == ROOT_FULL ) {
		printf( "Running as full root\n" );
	} else if ( is_suid == ROOT_EFFECTIVE ) {
		printf( "Only running as effective root\n" );
	} else if ( is_suid == ROOT_NONE ) {
		printf( "Failed to run as root\n" );
	} else {
		printf( "SUID returned %i\n", is_suid );
	}
	return 1;
}