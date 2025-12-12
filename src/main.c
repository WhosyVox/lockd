#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <unistd.h>
#include <errno.h>

#include <sys/wait.h>
#include <sys/mount.h>
#include <sched.h>
#include <sys/types.h>

#include <sys/stat.h>

// #include <assert.h>
// #include <stdint.h>

#include "profile.h"
#include "stringutils.h"

#include "opt_suid.h"
#include "opt_run.h"
#include "opt_validate.h"
#include "opt_help.h"

#include "file_validation.h"

void drop_root() {
	seteuid( getuid() );
}

int check_app_permissions() {
	char profiles_path[] = "/etc/lockd/profiles";
	char stores_path[] = "/etc/lockd/stores";

	int profiles_valid = check_item( profiles_path, 0755, is_directory );

	switch ( profiles_valid ) {
		case ITM_GOOD:
			// cool
		break;
		case ITM_MISSING:
			printf( "Unable to read %s: %s\n", profiles_path, strerror( errno ) );
			return 0;
		break;
		case ITM_BAD_TYPE:
			printf( "Item %s exists but is not a directory\n", profiles_path );
			return 0;
		break;
		case ITM_BAD_PERMISSIONS:
			printf( "Incorrect permissions on %s: expected 755\n", profiles_path );
			return 0;
		break;
		case ITM_BAD_OWNER:
			printf( "Item %s must be owned by root\n", stores_path );
			return 0;
		default:
			printf( "Unrecognised state %d on %s\n", profiles_valid, profiles_path );
		break;
	}

	int stores_valid = check_item( stores_path, 0700, is_directory );
	switch ( stores_valid ) {
		case ITM_GOOD:
			// cool
		break;
		case ITM_MISSING:
			printf( "Unable to read %s: %s\n", stores_path, strerror( errno ) );
			return 0;
		break;
		case ITM_BAD_TYPE:
			printf( "Item %s exists but is not a directory\n", stores_path );
			return 0;
		break;
		case ITM_BAD_PERMISSIONS:
			printf( "Incorrect permissions on %s: expected 700\n", stores_path );
			return 0;
		break;
		case ITM_BAD_OWNER:
			printf( "Item %s must be owned by root\n", stores_path );
			return 0;
		default:
			printf( "Unrecognised state %d on %s\n", stores_valid, stores_path );
		break;
	}

	return 1;
}



int main( int argc, char **argv ) {
	if ( argc < 2 ) {
		drop_root();
		return opt_help();
	}

	if ( check_app_permissions() == 0 ) {
		printf( "Environment is not in a correct state. Exiting.\n" );
		return 0;
	}
	
	uid_t uid = getuid();
	struct passwd *pw = getpwuid( uid );
	if ( pw == NULL ) {
		drop_root();
		printf( "Failed to load user data.\n" );
		return opt_help();
	}

	char *user_name = pw->pw_name;
	char *user_home = pw->pw_dir;

	if ( user_name == NULL || user_home == NULL ) {
		drop_root();
		printf( "Could not load user system information.\n" );
		return opt_help();
	}

	
	if ( strcmp( argv[1], "run" ) == 0 ) {
		// swap to effective user privileges
		setreuid( geteuid(), getuid() );
		char *profile_name = argv[2];
		return !opt_run( profile_name, user_name, user_home );
	} else if ( strcmp( argv[1], "suid" ) == 0 ) {
		// requires root euid for verification
		return !opt_suid();
	} else if ( strcmp( argv[1], "validate" ) == 0 ) {
		drop_root();
		if ( argc < 3 ) {
			printf( "Missing argument 2 to validate: the file to validate." );
			return 1;
		}
		char *profile_name = argv[2];
		return !opt_validate( profile_name, user_name, user_home );	
	} else {
		drop_root();
		return opt_help();
	}
}
