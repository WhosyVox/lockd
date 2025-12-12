#include <sys/stat.h>

typedef int (*item_type_function)(struct stat*);

typedef enum {
	ITM_GOOD = 0,
	ITM_MISSING = 1,
	ITM_BAD_TYPE = 2,
	ITM_BAD_PERMISSIONS = 4,
	ITM_BAD_OWNER = 8,
} item_validation;

int is_file( struct stat *item );
int is_directory( struct stat *item );

item_validation check_item(
	char *path,
	int target_permissions,
	item_type_function type_check
);
