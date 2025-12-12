#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/wait.h>
#include <sys/mount.h>
#include <sched.h>

#include "opt_run.h"
#include "profile.h"
#include "stringutils.h"
#include "file_validation.h"

// start a new mount-isolated namespace
int create_namespace() {
	int in_namespace = unshare( CLONE_NEWNS );
	if ( in_namespace != 0 ) {
		printf( "Unshare failed: %s\n", strerrorname_np( errno ) );
		return 0;
	}

	// prevent namespace mount from leaking
	int isolated = mount( "none", "/", NULL, MS_REC|MS_PRIVATE, NULL );
	if ( isolated != 0 ) {
		printf( "Failed to isolate namespace: %s\n", strerrorname_np( errno ) );
		return 0;
	}

	return 1;
}

int mount_store( char *store_path, char *mount_path, int user_id ) {
	// option 1 - fork/exec and get status

	pid_t pid = fork();
	if ( pid == -1 ) {
		printf( "Failed to create child process: %s\n", strerrorname_np( errno ) );
		return 0;
	}
	
	if ( pid == 0 ) {
		// child

		char id_string[8];
		sprintf( id_string, "%d", user_id );

		char *args[] = {
			"/usr/bin/bindfs",
			"-o",
			"nonempty",
			"-u",
			id_string,
			store_path,
			mount_path,
			NULL	
		};

		execve( "/usr/bin/bindfs", args, environ );

		
		/*
		char *args[] = {
			"/usr/bin/mount",
			store_path,
			mount_path,
			NULL	
		};

		execve( "/usr/bin/mount", args, environ );
		*/

		
		return 1;
	} else {
		// parent
		// wait for process to exit
		int status;
		do {
			waitpid( pid, &status, WUNTRACED );
			// printf( "Exiting. Process has status code %i.\n", status );
		} while ( !WIFEXITED( status ) && !WIFSIGNALED( status ) );
	
		return ( status != -1);
	}
	return 0;
}

int unmount_store( char *mount_path, int retries ) {
		int did_unmount;
		int tries = 0;
		do {
			did_unmount = umount( mount_path );
			if ( did_unmount == -1 ) {
				tries++;
			} else {
				break;
			}
			if ( tries > retries ) {
				return 0;
			}
			sleep( 1 );
		} while ( 1 );

	return 1;
}

/*
	TODO:
	/1. verify the store exists
	/2. verify the store permissions are correct (root:root 770)
	3. verify the mount location does exist
*/

int check_store_permissions( char *store_path ) {
	int store_valid = check_item( store_path, 0700, is_directory );
	switch ( store_valid ) {
		case ITM_GOOD:
			// cool
		break;
		case ITM_MISSING:
			printf( "Unable to read %s: %s\n", store_path, strerror( errno ) );
			return 0;
		break;
		case ITM_BAD_TYPE:
			printf( "Item %s exists but is not a file\n", store_path );
			return 0;
		break;
		case ITM_BAD_PERMISSIONS:
			printf( "Incorrect permissions on %s: expected 700\n", store_path );
			return 0;
		case ITM_BAD_OWNER:
			printf( "Profile %s must be owned by root\n", store_path );
			return 0;
		break;
		default:
			printf( "Unrecognised state %d on %s\n", store_valid, store_path );
		break;
	}
	
	return 1;
}



int opt_run( char *profile_name, char *user_name, char *user_home ) {
	profile_t profile = { NULL, NULL, NULL, NULL };
	int loaded = load_profile( profile_name, &profile );
	if ( !loaded ) {
		printf( "Failed to load profile: %s\n", profile_name );
		return 0;
	}

	if ( !validate_profile( &profile, user_name, user_home ) ) {
		return 0;
	}

	char *full_store_path = concat( STORE_PATH, profile.store );

	// the uid and euid are still swapped
	int user_id = geteuid();

	extern char **environ;


	// and drop to full root to execute
	setreuid( 0, 0 );

	// check store state, has to be done with root permissions
	if ( check_store_permissions( full_store_path ) == 0 ) {
		printf( "Store %s is not in a correct state. Exiting.\n", full_store_path );
		return 0;
	}


	int ns_status = create_namespace();
	if ( ns_status == 0 ) return 0;

	int did_mount = mount_store( full_store_path, profile.mount, user_id );
	if ( !did_mount ) {
		printf( "Failed to mount store.\n" );
		return 0;
	}

	pid_t pid = fork();
	if ( pid == -1 ) {
		printf( "Failed to create child process: %s\n", strerrorname_np( errno ) );
		return 0;
	}
	
	if ( pid == 0 ) {
		// child
		setreuid( user_id, user_id );
		char** run_args = split_arguments( profile.run );
		execve( run_args[0], run_args , environ );
		return 1;
	} else {
		// parent

		// wait for process to exit
		int status;
		do {
			waitpid( pid, &status, WUNTRACED );
			// printf( "Exiting. Process has status code %i.\n", status );
		} while ( !WIFEXITED( status ) && !WIFSIGNALED( status ) );

		int tries = 10;
		int did_unmount = unmount_store( profile.mount, tries );
		if ( did_unmount == 0 ) {
			printf( "Failed to unmount filesystem: %s\n", strerrorname_np( errno ) );
		}
		
		return 1;
	}
	
	return 1;
}
