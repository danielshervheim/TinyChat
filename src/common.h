#ifndef COMMON_H_
#define COMMON_H_


// login defines
#define MAX_PORT_LEN 5
#define MAX_ADDRESS_LEN 1024
#define MAX_USERNAME_LEN 16

// general defines
#define MAX_MESSAGE_LEN 256
#define MAX_CONCURRENT_USERS 10
#define BUFFER_SIZE (1024 + MAX_USERNAME_LEN * MAX_CONCURRENT_USERS)
#define MILLI_SLEEP_DUR 1
#define MICRO_SLEEP_DUR (MILLI_SLEEP_DUR * 1000.0)

// err: -1 too short, -2 too long
int is_valid_address(const char *address, int *err);

// err: -1 too small, -2 too large
int is_valid_port(const char *port, int *err);

// err: -1 too short, -2 too long, -3 contains spaces
int is_valid_username(const char *username, int *err);

#endif  // COMMON_H_
