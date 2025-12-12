#include "stringutils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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


// outputs a null-terminated array of char*
char** split_arguments( char *input ) {
	char* buffer[64];
	int length = 0;

	char* token = strtok( input, " " );
	do {
		buffer[length] = token;
		length++;
		token = strtok( NULL, " " );
	} while ( token != NULL );
	buffer[length] = '\0';

	char** output = calloc( length + 1, sizeof( char* ) );
	memcpy( output, buffer, length * sizeof( char* ) );
	return output;
}
