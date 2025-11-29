#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <pwd.h>
#include <sys/types.h>
#include <sys/capability.h>
#include <sys/prctl.h>
#include <unistd.h>

#define STORE_PATH "/etc/lockd/stores/"
#define PROFILE_PATH "/etc/lockd/profiles/"
#define CONFIG_DELIM '='

#define NOT_ROOT 0
#define EFFECTIVE_ROOT 1
#define FULL_ROOT 2

typedef struct {
	char *store;
	char *mount;
	char *run;
} profile_t;

typedef struct {
	char *key;
	char *value;
} keyvalue_t;


// TODO: at some point we need to validate that profile.run is a full path

// TODO: allow only valid file names, and sub-directories
int is_valid_filename( char *filename ) {
	// check for $ and ..
	return 1;
}

// TODO: better way to split on character
// + strip newline from each line
int parse_keyvalue( char *line, keyvalue_t *keyvalue  ) {

	printf( "\n%s", line );

	char *first_match = strchr( line, CONFIG_DELIM );
	if ( first_match == NULL ) {
		return 0;
	}

	 printf( "Matched character %c\n", first_match[0] );


	int match = first_match - line;

	printf( "Match at index %i\n", match );
	
	char *key = calloc( match+1, sizeof( char ) );
	strncpy( key, line, match );

	printf( "Key: %s\n", key );

	// we subtract an extra 1 to avoid copying the newline
	int value_size = strlen( line ) - match - 1 - 1;
	char *value = calloc( value_size + 1, sizeof( char ) );
	strncpy( value, line+match+1, value_size );

	printf( "Value: %s\n\n", value );

	keyvalue->key = key;
	keyvalue->value = value;
	
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
		} else {
			free( entry.value );
		}
		free( entry.key );
	}

	if ( profile->store == NULL ||
		 profile->mount == NULL ||
		 profile->run   == NULL ) {
		 return 0;
	}

	return 1;
}


// You can't trust environment variables to check for the user
// So instead we query the process info to determine who the caller is
char* get_user() {
	uid_t uid = getuid();
	struct passwd *pw = getpwuid( uid );
	if ( pw ) {
		char *user = pw->pw_name;
		return user;
	} else {
		return NULL;
	}	
}


int test_suid() {
	uid_t euid = geteuid();
	if ( euid != 0 ) return NOT_ROOT;

	setuid( 0 );
	int uid = getuid();
	
	if ( uid != 0 ) return EFFECTIVE_ROOT;
	
	return FULL_ROOT;
}

void dump_string( char *string ) {
	long int length = strlen( string );
	for ( int i = 0; i < length; i++ ) {
		printf( "%x ", string[i] );
	}
	printf( "\n" );
}

char* concat( char *str1, char *str2 ) {
	long int size = strlen( str1 ) + strlen( str2 ) + 1;
	char *output = calloc( size, sizeof( char ) );
	sprintf( output, "%s%s", str1, str2 );
	return output;
}


int load_profile( char *profile_name, profile_t *profile ) {
	if ( !is_valid_filename( profile_name ) ) {
		printf( "Cannot load %s: invalid file name/path", profile_name );
		return 0;
	}

	char *profile_file = concat( PROFILE_PATH, profile_name );
	printf( "%s\n", profile_file );
	
	FILE *fp = fopen( profile_file, "r" );
	if ( fp == NULL ) {
		printf( "No such file: %s\n", profile_file );
		free( profile_file );
		return 0;
	}
	free( profile_file );

	int parsed = parse_profile( fp, profile );
	fclose(fp);
	
	return parsed;
}



int opt_run( char *profile_name ) {
	profile_t profile = { NULL, NULL, NULL };
	int loaded = load_profile( profile_name, &profile );
	if ( !loaded ) {
		printf( "Failed to load profile: %s\n", profile_name );
		return 0;
	}
	
	// printf( "store: %s\n", profile.store );
	// printf( "mount: %s\n", profile.mount );
	// printf( "run:   %s\n", profile.run   );

	char *user = get_user();

	extern char **environ;

	char *full_store_path = concat( STORE_PATH, profile.store );

	printf( "Profile path: %s\n", full_store_path );

	char *args[] = {
		"/usr/bin/unshare",
		"-m",
		"/etc/lockd/run",
		user,
		full_store_path,
		profile.mount,
		profile.run,
		'\0'
	};

	setuid( 0 );
	execve( "/usr/bin/unshare", args, environ );
	
	return 1;
}

int opt_suid() {
	int is_suid = test_suid();
	if ( is_suid == FULL_ROOT ) {
		printf( "Running as full root\n" );
	} else if ( is_suid == EFFECTIVE_ROOT ) {
		printf( "Only running as effective root\n" );
	} else if ( is_suid == NOT_ROOT ) {
		printf( "Failed to run as root\n" );
	} else {
		printf( "SUID returned %i\n", is_suid );
	}
	return 1;
}

int opt_user() {
	char* user = get_user();
	if ( user != NULL ) {
		printf( "Current user: %s\n", user );
		return 1;
	}
	printf( "User could not be determined.\n" );
	return 0;
}

int opt_help() {
	// TODO
	printf( "No option selected.\n" );
	return 1;
}


int main( int argc, char **argv  ) {
	if ( argc < 2 ) return opt_help();

	if ( strcmp( argv[1], "run" ) == 0 ) {
		char *profile_name = argv[2];
		return !opt_run( profile_name );
	} else if ( strcmp( argv[1], "suid" ) == 0 ) {
		return !opt_suid();
	} else if ( strcmp( argv[1], "who") == 0 ) {
		return !opt_user();
	} else {
		return opt_help();
	}
}
