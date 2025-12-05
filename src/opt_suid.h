typedef enum {
	ROOT_NONE,
	ROOT_EFFECTIVE,
	ROOT_FULL
} root_type;

root_type test_suid();
int opt_suid();
