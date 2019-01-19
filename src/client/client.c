//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#include "client.h"

#include "common.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>



struct _Client {
    GObject parent_instance;
    // member instances go here
    char m_username[MAX_USERNAME_LEN];
    char m_userlist[MAX_CONCURRENT_USERS][MAX_USERNAME_LEN];
    int m_userlist_i;
    int m_socketFd;
};

G_DEFINE_TYPE(Client, client, G_TYPE_OBJECT);













/* sets up the initial user list */
void userlist_setup(Client *self, const char* buffer) {
    // buffer is of form "<username1> <username2> ... <usernamei>"

    for (int i = 0; i < MAX_CONCURRENT_USERS; i++) {
        memset(self->m_userlist[i], '\0', MAX_USERNAME_LEN);
    }

    // strtok modifies buffer, so we first make a copy
    char tmp[BUFFER_SIZE];
    memset(tmp, '\0', BUFFER_SIZE);
    strcpy(tmp, buffer);


    // fill up this instances userlist by tokenizing
    char *token = strtok(tmp, " ");

    while (token != NULL) {
    	strcpy(self->m_userlist[self->m_userlist_i], token);
    	token = strtok(NULL, " ");
    	self->m_userlist_i++;
    }
}



void userlist_joined(Client *self, const char *username) {
    strcpy(self->m_userlist[self->m_userlist_i], username);
    g_signal_emit_by_name(self, "user-joined", username);
}



void userlist_left(Client *self, const char *username) {
    int x = 0;

    for (int x = 0; x < self->m_userlist_i; x++) {
        if (strcmp(self->m_userlist[x], username) == 0) {
            break;
        }
    }

    g_signal_emit_by_name(self, "user-left", username);

    for (int i = x; i < self->m_userlist_i; i++) {
        memset(self->m_userlist[i], '\0', MAX_USERNAME_LEN);
        memcpy(self->m_userlist[i], self->m_userlist[i+1], MAX_USERNAME_LEN);
    }

    self->m_userlist_i--;

    for (int i = 0; i < self->m_userlist_i; i++) {
        printf("%d %s\n", i, self->m_userlist[i]);
    }
}








void message_parse_whisper(Client *self, const char *buffer) {
    // buf is of format "<sender> <message>"

    // strtok modifies string, so we first make a copy
    char tmp[BUFFER_SIZE];
    memset(tmp, '\0', BUFFER_SIZE);
    strcpy(tmp, buffer);

    char *sender = strtok(tmp, " ");
    const char *message = buffer + strlen(sender) + 1;

    g_signal_emit_by_name(self, "private-message-received", sender, message);
}




void message_parse_broadcast(Client *self, const char *buffer) {
    // buf is of format "<sender> <message>"

    // strtok modifies string, so we first make a copy
    char tmp[BUFFER_SIZE];
    memset(tmp, '\0', BUFFER_SIZE);
    strcpy(tmp, buffer);

    char *sender = strtok(tmp, " ");
    const char *message = buffer + strlen(sender) + 1;

    g_signal_emit_by_name(self, "message-received", sender, message);
}









int server_poll(Client *self) {
    // if the connection was closed between polls, then quit polling
    if (self->m_socketFd == -1) {
        return 0;
    }

    char tmp[BUFFER_SIZE];
    memset(tmp, '\0', BUFFER_SIZE);
    ssize_t nread;

    if ((nread = read(self->m_socketFd, tmp, BUFFER_SIZE)) != -1) {
        if (nread == 0) {
            g_signal_emit_by_name(self, "connection-lost");

            // call logout here?
            return 0;
        }
        else {
            if (memcmp(tmp, "/whispered", strlen("/whispered")) == 0) {
				message_parse_whisper(self, tmp + strlen("/whispered") + 1);
			}
            else if (memcmp(tmp, "/broadcasted", strlen("/broadcasted")) == 0) {
				message_parse_broadcast(self, tmp + strlen("/broadcasted") + 1);
			}
            else if (memcmp(tmp, "/joined", strlen("/joined")) == 0) {
                userlist_joined(self, tmp + strlen("/joined") + 1);
			}
            else if (memcmp(tmp, "/left", strlen("/left")) == 0) {
                userlist_left(self, tmp + strlen("/left") + 1);
			}
        }
    }

    return 1;
}









// returns 1 on success, 0 on failure
int client_connect(Client *self, const char *port, const char *address, int *err) {
    struct addrinfo *address_info;

    if (getaddrinfo(address, port, NULL, &address_info) < 0) {
		perror("getaddrinfo() failed");
        *err = -1;
		return 0;
	}

    if ((self->m_socketFd = socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol)) < 0) {
		perror("socket() failed");
        *err = -2;
		return 0;
	}

    if (connect(self->m_socketFd, address_info->ai_addr, address_info->ai_addrlen) < 0) {
		perror("connect() failed");
        *err = -3;
		return 0;
	}

    return 1;  // success
}

void client_disconnect(Client *self) {
    close(self->m_socketFd);
    self->m_socketFd = -1;
}

// returns 1 on success, 0 on failure
int client_login(Client *self, const char *username, int *err) {
    char tmp[BUFFER_SIZE];
    memset(tmp, '\0', BUFFER_SIZE);
    sprintf(tmp, "/join %s", username);

    // write to the server to request login
    if (write(self->m_socketFd, tmp, strlen(tmp)) < 0) {
        perror("write() failed during handshake");
        *err = -3;
        return 0;
    }

    // clear the buffer to reuse it
    memset(tmp, '\0', BUFFER_SIZE);

    // read response from server
    if (read(self->m_socketFd, tmp, BUFFER_SIZE) <= 0) {
		perror("read() failed during handshake");
		*err = -3;
		return 0;
	}

    // check the response
    if (memcmp(tmp, "/joinresponse ok", strlen("/joinresponse ok")) == 0) {
        // set the socket to be non-blocking so we can poll it
		if (fcntl(self->m_socketFd, F_SETFL, O_NONBLOCK) < 0) {
			perror("fcntl() failed during handshake");
			*err = -3;
			return 0;
		}

        userlist_setup(self, tmp + strlen("/joinresponse ok") + 1);

        g_timeout_add(MILLI_SLEEP_DUR, (void *)server_poll, self);

        return 1;
    }
    else if (memcmp(tmp, "/joinresponse username_taken", strlen("/joinresponse username_taken")) == 0) {
        *err = -1;
        return 0;
	}
	else if (memcmp(tmp, "/joinresponse server_full", strlen("/joinresponse server_full")) == 0) {
        *err = -2;
        return 0;
	}
	else {
        *err = -3;
        return 0;
	}

    return 1;
}

// returns 1 on success, 0 on failure
int client_send_broadcast(Client *self, const char *message) {
    return 1;
}

// returns 1 on success, 0 on unspecified error, -1 on recipient not connected
int client_send_private_message(Client *self, const char *recipient, const char *message) {
    return 1;
}











/* Returns a new instance of Client. */
Client* client_new () {
    return g_object_new (CLIENT_TYPE_OBJECT, NULL);
}

/* Destroys the instance. */
void client_destroy(Client *self) {
    // todo: destroy client here
}

/* Initializes the Client class */
static void client_class_init (ClientClass *class) {
    // tmp
    printf("client class init\n");

    // installing signals

    /* Fires on the client instance when a new message is received. */
    g_signal_new("message-received", CLIENT_TYPE_OBJECT, G_SIGNAL_RUN_FIRST,
    	0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER);

    /* Fires on the client instance when a new private message is received. */
    g_signal_new("private-message-received", CLIENT_TYPE_OBJECT, G_SIGNAL_RUN_FIRST,
    	0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER);

    /* Fires on the client instance when a user has left the chat. */
    g_signal_new("user-left", CLIENT_TYPE_OBJECT, G_SIGNAL_RUN_FIRST,
    	0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_POINTER);

    /* Fires on the client instance when a user has joined the chat. */
    g_signal_new("user-joined", CLIENT_TYPE_OBJECT, G_SIGNAL_RUN_FIRST,
    	0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_POINTER);

    /* Fires on the client when the connection to the server is lost. */
    g_signal_new("connection-lost", CLIENT_TYPE_OBJECT, G_SIGNAL_RUN_FIRST,
    	0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

/* Initializes the Client instance. */
static void client_init (Client *self) {
	printf("client init\n");

	memset(self->m_username, '\0', MAX_USERNAME_LEN);

    self->m_userlist_i = 0;

	self->m_socketFd = -1;
}
