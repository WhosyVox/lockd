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

// #include <assert.h>
// #include <stdint.h>

#include "profile.h"
#include "stringutils.h"

#include "opt_suid.h"

#define STORE_PATH "/etc/lockd/stores/"


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

	// the uid and euid are still swapped
	int user_id = geteuid();

	extern char **environ;

	char *full_store_path = concat( STORE_PATH, profile.store );

	printf( "Full store path: %s\n", full_store_path );

	// and drop to full root to execute
	setreuid( 0, 0 );

	// start a new mount-isolated namespace
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


	//  1. create a loop device from full_store_path
	//  2. mount the block device to profile.mount
	// /3. Fork here
	// /4. child: drop to normal user privileges setreuid( user_id, user_id )
	// /5. child: exec to profile.run
	// /6. parent: waits until child terminates
	//  7. parent: then unmount filesystem
	//  8. parent: unmount loop device

	


	/*
	pid_t pid = fork();
	if ( pid == -1 ) {
		printf( "Failed to create child process: %s\n", strerrorname_np( errno ) );
		return 0;
	}
	
	if ( pid == 0 ) {
		// child
		setreuid( user_id, user_id );
		// printf( "Child privileges: %i, %i\n", getuid(), geteuid() );
		char** run_args = split_arguments( profile.run );
		execve( run_args[0], run_args , environ );
		return 1;
	} else {
		// parent
		// printf( "Parent privileges: %i, %i\n", getuid(), geteuid() );

		// wait for process to exit
		int status;
		do {
			waitpid( pid, &status, WUNTRACED );
			printf( "Exiting. Process has status code %i.\n", status );
		} while ( !WIFEXITED( status ) && !WIFSIGNALED( status ) );


		// todo: umount here

		
		return 1;
	}
	*/
	


	/*
	char *args[] = {
		"/etc/lockd/run",
		user_name,
		full_store_path,
		profile.mount,
		profile.run,
		'\0'
	};
	execve( "/etc/lockd/run", args, environ );
	*/

	
	return 1;
}


int opt_user( char *user_name ) {
	printf( "Current user: %s\n", user_name );
	return 1;
}

int opt_help() {
	// TODO
	printf( "No option selected.\n" );
	return 1;
}

int opt_validate( char *profile_name, char *user_name, char *user_home ) {
	profile_t profile = { NULL, NULL, NULL, NULL };
	int loaded = load_profile( profile_name, &profile );
	if ( !loaded ) {
		printf( "Failed to load profile: %s\n", profile_name );
		return 0;
	}

	if ( !validate_profile( &profile, user_name, user_home ) ) {
		return 0;
	}

	printf( "Profile is valid for this user.\n" );

	printf(
		"Store=%s\nMount=%s\nRun=%s\nUser=%s\n",
		profile.store,
		profile.mount,
		profile.run,
		profile.user
	 );

	 return 1;
}





void drop_root() {
	seteuid( getuid() );
}


int main( int argc, char **argv ) {
	if ( argc < 2 ) {
		drop_root();
		return opt_help();
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

	if ( user_name == NULL ) {
		drop_root();
		printf( "Could not determine user.\n" );
		return opt_help();
	}

	if ( user_home == NULL ) {
		drop_root();
		printf( "Could not determine user's home.\n" );
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
	} else if ( strcmp( argv[1], "who") == 0 ) {
		drop_root();
		return !opt_user( user_name );
	} else if ( strcmp( argv[1], "ns" ) == 0 ) {
		// drop_root();

		setuid( 0 );

		
		int in_namespace = unshare( CLONE_NEWNS );
		if ( in_namespace != 0 ) {
			printf( "Unshare failed: %s\n", strerrorname_np( errno ) );
		}

		// really?
		mount( "none", "/", NULL, MS_REC|MS_PRIVATE, NULL );

		int did_mount = system( "mount /etc/lockd/stores/whosy-firefox /home/whosy/.mozilla/firefox" );

				/*
		int did_mount = mount(
			"/etc/lockd/stores/whosy-firefox",
			"/home/whosy/.mozilla/firefox",
			"ext4",
			0, // MS_PRIVATE,
			NULL
		);
		*/

		if ( did_mount != 0 ) {
			printf( "Failed to mount: %s\n",  strerrorname_np( errno ) );
		}
		
		getchar();

		int did_umount;
		while ( 1 ) {
			did_umount = system( "umount /home/whosy/.mozilla/firefox" );
			// int did_umount = umount( "/home/whosy/.mozilla/firefox" );

			if ( did_umount != 0 ) {
				printf( "Failed to unmount: %s\n",  strerrorname_np( errno ) );
				continue;
			}
			break;
		}
		
	} else {
		drop_root();
		return opt_help();
	}
}
