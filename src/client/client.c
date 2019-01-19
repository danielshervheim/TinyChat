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
    char *m_userlist;
    int m_socketFd;
};

G_DEFINE_TYPE(Client, client, G_TYPE_OBJECT);












/* sets up the initial user list */
void userlist_setup(Client *self, const char* buffer) {
    // buffer is of form "<username1> <username2> ... <usernamei>"

    // retokenize to remove your own username from the list

    memset(self->m_userlist, '\0', sizeof(char) * BUFFER_SIZE);

    char tmp_buffer[BUFFER_SIZE];
    memset(tmp_buffer, '\0', BUFFER_SIZE);
    strcpy(tmp_buffer, buffer);


    char *token = strtok(tmp_buffer, " ");

    while (token != NULL) {
        if (strcmp(token, self->m_username) != 0) {
            strcat(self->m_userlist, token);
            strcat(self->m_userlist, " ");
        }
        token = strtok(NULL, " ");
    }

    // strcpy(self->m_userlist, buffer);
    g_signal_emit_by_name(self, "userlist-updated", self->m_userlist);
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
            else if (memcmp(tmp, "/userlist", strlen("/userlist")) == 0) {
                userlist_setup(self, tmp + strlen("/userlist") + 1);
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

    self->m_userlist = malloc(sizeof(char) * BUFFER_SIZE);
    memset(self->m_userlist, '\0', sizeof(char) * BUFFER_SIZE);
    return 1;  // success
}

void client_disconnect(Client *self) {
    if (self->m_socketFd != -1) {
        close(self->m_socketFd);
        self->m_socketFd = -1;        
    }

    if (self->m_userlist != NULL) {
        free(self->m_userlist);
        self->m_userlist = NULL;
    }
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

        memset(self->m_username, '\0', MAX_USERNAME_LEN);
        strcpy(self->m_username, username);
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
    char outgoing[BUFFER_SIZE];
    memset(outgoing, '\0', BUFFER_SIZE);
    sprintf(outgoing, "/broadcast %s", message);

    if (write(self->m_socketFd, outgoing, strlen(outgoing)) < 0) {
        printf("\'write\' failed during broadcast\n");
        return 0;
    }

    return 1;
}

// returns 1 on success, 0 on unspecified error, -1 on recipient not connected
int client_send_private_message(Client *self, const char *recipient, const char *message) {
    char outgoing[BUFFER_SIZE];
    memset(outgoing, '\0', BUFFER_SIZE);
    sprintf(outgoing, "/whisper %s %s", recipient, message);

    if (write(self->m_socketFd, outgoing, strlen(outgoing)) < 0) {
        printf("\'write\' failed during whisper\n");
        return 0;
    }

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
    g_signal_new("userlist-updated", CLIENT_TYPE_OBJECT, G_SIGNAL_RUN_FIRST,
    	0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_POINTER);

    /* Fires on the client when the connection to the server is lost. */
    g_signal_new("connection-lost", CLIENT_TYPE_OBJECT, G_SIGNAL_RUN_FIRST,
    	0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

/* Initializes the Client instance. */
static void client_init (Client *self) {
	printf("client init\n");

	memset(self->m_username, '\0', MAX_USERNAME_LEN);

    self->m_userlist = NULL;

	self->m_socketFd = -1;
}
