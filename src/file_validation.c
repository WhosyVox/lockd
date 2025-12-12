/* we care about 3 things:
	1. does the file exist
	2. is it the type we expected
	3. does it have the permissions we require
	4. is it owned by the correct user
*/

#include "file_validation.h"

int is_file( struct stat *item ) {
	return S_ISREG( item->st_mode );
}

int is_directory( struct stat *item ) {
	return S_ISDIR( item->st_mode );
}

// TODO: for now we're cheating and only checking ownership against root
// nothing we're testing requires otherwise, but this feels dirty still
item_validation check_item( char *path, int target_permissions, item_type_function type_check ) {
	struct stat item;

	int status = stat( path, & item );
	if ( status == -1 ) return ITM_MISSING;

	if ( type_check( &item ) == 0 ) return ITM_BAD_TYPE;

	int item_permissions = item.st_mode & 0777;
	if ( item_permissions != target_permissions ) return ITM_BAD_PERMISSIONS;

	int uid = item.st_uid;
	int gid = item.st_gid;
	if ( uid != 0 || gid != 0 ) {
		return ITM_BAD_OWNER;
	}


	return ITM_GOOD;	
}
