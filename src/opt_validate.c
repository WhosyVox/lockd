#include "opt_validate.h"
#include "profile.h"

int opt_validate( char *profile_name, char *user_name, char *user_home ) {
	if ( check_profile_permissions( profile_name ) == 0 ) {
		printf( "Profile file %s is not in a correct state.\n", profile_name );
		return 0;
	}

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
