#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



int is_valid_address(const char *address, int *err) {
	if (strlen(address) <= 0) {
		*err = -1;
		return 0;
	}

	if (strlen(address) > MAX_ADDRESS_LEN) {
		*err = -2;
		return 0;
	}

	return 1;
}

int is_valid_port(const char *port, int *err) {
	if (atoi(port) < 1024) {
		*err = -1;
		return 0;
	}

	if (atoi(port) > 65535) {
		*err = -2;
		return 0;
	}

	return 1;
}

int is_valid_username(const char *username, int *err) {
	if (strlen(username) <= 0) {
		*err = -1;
		return 0;
	}

	if (strlen(username) > MAX_USERNAME_LEN) {
		*err = -2;
		return 0;
	}

	if (strchr(username, ' ') != NULL) {
		*err = -3;
		return 0;
	}

	return 1;
}