#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <unistd.h>

// #include <sys/types.h>
// #include <sys/capability.h>
// #include <sys/prctl.h>
// #include <assert.h>
// #include <stdint.h>

#define STORE_PATH "/etc/lockd/stores/"
#define PROFILE_PATH "/etc/lockd/profiles/"
#define CONFIG_DELIM "="
#define CONFIG_COMMENT "#"

typedef enum {
	ROOT_NONE,
	ROOT_EFFECTIVE,
	ROOT_FULL
} root_type;

typedef struct {
	char *store;
	char *mount;
	char *run;
	char *user;
} profile_t;

typedef struct {
	char *key;
	char *value;
} keyvalue_t;

int is_valid_filename( char *filename ) {
	if ( filename[0] == '/' ) return 0;
	if ( strstr( filename, ".." ) != NULL ) return 0;
	return 1;
}

// for now, we just check for an absolute path
int is_valid_app_path( char *run_string ) {
	if ( run_string[0] != '/' ) return 0;
	return 1;
}


// places a null terminator on the left-most trailing whitespace,
// and returns a pointer to the first non-whitespace from the left
char* trim_whitespace( char *input ) {
	char *output = input;
	long int len = strlen( input );

	// grab the pointer to the first non-whitespace character
	for ( int i = 0; i < len; i++ ) {
		if ( input[i] != ' ' && input[i] != '\t' ) {
			output = input+i;
			break;
		}
	}

	long int newlen = strlen( output );

	// replace the last trailing whitespace with a NULL
	// (may also just replace the final NULL with NULL)
	for ( int i = newlen - 1; i >= 0; i-- ) {
		if ( output[i] != ' ' && output[i] != '\t' ) {
			output[i+1] = '\0';
			break;
		}
	}

	return output;
}

int parse_keyvalue( char *line, keyvalue_t *keyvalue ) {
	// strip trailing newlines from the input
	line[ strcspn( line, "\r\n" ) ] = '\0';

	// cut the line short at a comment character
	line[ strcspn( line, CONFIG_COMMENT ) ] = '\0';

	char *key = strtok( line, CONFIG_DELIM );
	if ( key == NULL ) return 0;

	char *value = strtok( NULL, CONFIG_DELIM );
	if ( value == NULL ) return 0;

	key = trim_whitespace( key );
	value = trim_whitespace( value );

	keyvalue->key   = calloc( strlen(  key  ) + 1, sizeof( char ) );
	keyvalue->value = calloc( strlen( value ) + 1, sizeof( char ) );

	strcpy( keyvalue->key,   key   );
	strcpy( keyvalue->value, value );
	
	return 1;
}

int parse_profile( FILE *fp, profile_t *profile ) {
	// TODO: can we fix this arbitrary limit?
	char line[256];

	while ( fgets( line, sizeof( line ), fp ) ) {
		keyvalue_t entry;
		
		int success = parse_keyvalue( line, &entry );

		if ( success == 0 ) continue;
		
		if( strcmp( entry.key, "store" ) == 0 ) {
			profile->store = entry.value;
		} else if ( strcmp( entry.key, "mount" ) == 0 ) {
			profile->mount = entry.value;
		} else if ( strcmp( entry.key, "run" ) == 0 ) {
			profile->run = entry.value;
		} else if ( strcmp( entry.key, "user" ) == 0 ) {
			profile->user = entry.value;
		} else {
			free( entry.key );
			free( entry.value );
		}
	}

	if ( profile->store == NULL ||
		 profile->mount == NULL ||
		 profile->run   == NULL ||
		 profile->user  == NULL ) {
		 return 0;
	}

	// printf( "store in parse_profile: %s\n", profile->store );

	return 1;
}

root_type test_suid() {
	uid_t euid = geteuid();
	if ( euid != 0 ) return ROOT_NONE;

	setuid( 0 );
	int uid = getuid();
	
	if ( uid != 0 ) return ROOT_EFFECTIVE;
	
	return ROOT_FULL;
}

void dump_string( char *string ) {
	long int length = strlen( string );
	for ( int i = 0; i < length; i++ ) {
		printf( "%x ", string[i] );
	}
	printf( "\n" );
}

char* concat( char *str1, char *str2 ) {
	// possibility of an integer overflow (unlikely)
	long int size = strlen( str1 ) + strlen( str2 ) + 1;
	// Such an overflow would result in a negative size into calloc
	// which casts all values to unsigned long int
	char *output = calloc( size, sizeof( char ) );
	sprintf( output, "%s%s", str1, str2 );
	return output;
}

int load_profile( char *profile_name, profile_t *profile ) {
	if ( !is_valid_filename( profile_name ) ) {
		printf( "Cannot load %s: invalid file name/path\n", profile_name );
		return 0;
	}

	char *profile_file = concat( PROFILE_PATH, profile_name );
	// printf( "%s\n", profile_file );
	
	FILE *fp = fopen( profile_file, "r" );
	if ( fp == NULL ) {
		printf( "No such file: %s\n", profile_file );
		free( profile_file );
		return 0;
	}
	free( profile_file );

	int parsed = parse_profile( fp, profile );
	fclose(fp);

	if ( parsed ) {
		return 1;
	} else {
		return 0;
	}
}


int validate_profile( profile_t *profile, char *user_name, char *user_home ) {

	if ( !is_valid_app_path( profile->run ) ) {
		printf( "profile run must point to an absolute path (got %s)\n", profile->run );
		return 0;
	}

	if ( !is_valid_filename( profile->store ) ) {
		printf( "store must be a path relative to the store directory (got %s)\n", profile->store );
		return 0;
	}


	if ( strcmp( user_name, profile->user ) != 0 ) {
		printf(
			"Cannot run profile: profile is configured for user %s but was run by user %s\n",
			profile->user,
			user_name
		 );
		return 0;
	}

	// prevent substring hacks like /home/a for /home/abc
	char *home = concat( user_home, "/" );
	
	if ( strncmp( home, profile->mount, strlen( home ) ) != 0 ) {
		printf(
			"Cannot run profile: mount point must be in user's home (%s)\n",
			home
		);
		return 0;
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

	extern char **environ;

	char *full_store_path = concat( STORE_PATH, profile.store );

	printf( "Full store path: %s\n", full_store_path );

	char *args[] = {
		"/usr/bin/unshare",
		"-m",
		"/etc/lockd/run",
		user_name,
		full_store_path,
		profile.mount,
		profile.run,
		'\0'
	};

	// and drop to full root to execute
	setreuid( 0, 0 );
	execve( "/usr/bin/unshare", args, environ );
	
	return 1;
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

int opt_user( char *user_name ) {
	printf( "Current user: %s\n", user_name );
	return 1;
}

int opt_help() {
	// TODO
	printf( "No option selected.\n" );
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

		char *profile_name = argv[2];
		profile_t profile = { NULL, NULL, NULL, NULL };
		int loaded = load_profile( profile_name, &profile );
		if ( !loaded ) {
			printf( "Failed to load profile: %s\n", profile_name );
			return 1;
		}

		if ( !validate_profile( &profile, user_name, user_home ) ) {
			return 1;
		}

		printf( "Profile is valid for this user.\n" );

		printf(
			"Store=%s\nMount=%s\nRun=%s\nUser=%s\n",
			profile.store,
			profile.mount,
			profile.run,
			profile.user
		 );
		
		return 0;
	} else if ( strcmp( argv[1], "who") == 0 ) {
		drop_root();
		return !opt_user( user_name );
	} else {
		drop_root();
		return opt_help();
	}
}
