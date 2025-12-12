#include <stdio.h>

#define CONFIG_DELIM "="
#define CONFIG_COMMENT "#"
#define PROFILE_PATH "/etc/lockd/profiles/"

// TODO: solve this
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

int parse_keyvalue( char* line, keyvalue_t *keyvalue );

int parse_profile( FILE *fp, profile_t *profile );
int check_profile_permissions( char *profile_path );
int load_profile( char *profile_name, profile_t *profile );
int validate_profile( profile_t *profile, char *user_name, char *user_home );
