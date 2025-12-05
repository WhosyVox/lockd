#include "profile.h"
#include "stringutils.h"
#include <string.h>
#include <stdlib.h>

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
