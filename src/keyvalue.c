#include "keyvalue.h"
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
