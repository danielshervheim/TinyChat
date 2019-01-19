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

    int m_socketFd;
    char *m_username;
    char *m_userlist;
};

G_DEFINE_TYPE(Client, client, G_TYPE_OBJECT);



/* sets up the initial user list */
void userlist_update(Client *self, const char* buffer) {
    // buffer is of form "<username1> <username2> ... <usernamei>"

    // clear the userlist, just to be safe.
    memset(self->m_userlist, '\0', sizeof(char) * BUFFER_SIZE);

    // Make a copy of the buffer, to retokenize.
    char tmp_buffer[BUFFER_SIZE];
    memset(tmp_buffer, '\0', BUFFER_SIZE);
    strcpy(tmp_buffer, buffer);

    /* retokenize the buffer to remove your own username from the list, since
    you shouldn't be able to PM yourself. */
    char *token = strtok(tmp_buffer, " ");
    while (token != NULL) {
        if (strcmp(token, self->m_username) != 0) {
            strcat(self->m_userlist, token);
            strcat(self->m_userlist, " ");
        }
        token = strtok(NULL, " ");
    }

    // Signal that the userlist has been updated.
    g_signal_emit_by_name(self, "userlist-updated", self->m_userlist);
}



/* Parses the incoming whisper and signals that a new private message has arrived. */
void message_parse_whisper(Client *self, const char *buffer) {
    // buffer is of format "<sender> <message>"

    // strtok modifies string, so we first make a copy
    char tmp[BUFFER_SIZE];
    memset(tmp, '\0', BUFFER_SIZE);
    strcpy(tmp, buffer);

    char *sender = strtok(tmp, " ");
    const char *message = buffer + strlen(sender) + 1;

    g_signal_emit_by_name(self, "private-message-received", sender, message);
}



/* Parses the incoming broadcast and signals that a new message has arrived. */
void message_parse_broadcast(Client *self, const char *buffer) {
    // buffer is of format "<sender> <message>"

    // strtok modifies string, so we first make a copy
    char tmp[BUFFER_SIZE];
    memset(tmp, '\0', BUFFER_SIZE);
    strcpy(tmp, buffer);

    char *sender = strtok(tmp, " ");
    const char *message = buffer + strlen(sender) + 1;

    g_signal_emit_by_name(self, "message-received", sender, message);
}



/* Polls the server for new messages to read and handles them appropriatly. */
int server_poll(Client *self) {
    // if the connection was closed between polls, then quit polling.
    if (self->m_socketFd == -1) {
        return 0;
    }

    // tepmorary buffer to hold a potential new message.
    char tmp[BUFFER_SIZE];
    memset(tmp, '\0', BUFFER_SIZE);
    ssize_t nread;

    // read from the socket.
    if ((nread = read(self->m_socketFd, tmp, BUFFER_SIZE)) != -1) {
        // if nread == 0, then the socket was closed from the other end.
        if (nread == 0) {
            g_signal_emit_by_name(self, "connection-lost");
            return 0;  // return 0 to quit polling.
        }
        else {
            if (memcmp(tmp, "/whispered", strlen("/whispered")) == 0) {
				message_parse_whisper(self, tmp + strlen("/whispered") + 1);
			}
            else if (memcmp(tmp, "/broadcasted", strlen("/broadcasted")) == 0) {
				message_parse_broadcast(self, tmp + strlen("/broadcasted") + 1);
			}
            else if (memcmp(tmp, "/userlist", strlen("/userlist")) == 0) {
                userlist_update(self, tmp + strlen("/userlist") + 1);
			}
        }
    }

    // Otherwise, keep polling.
    return 1;
}



/* Attempts to connect to the server. */
int client_connect(Client *self, const char *port, const char *address, int *err) {
    struct addrinfo *address_info;

    // verifies that the address and port combo are valid.
    if (getaddrinfo(address, port, NULL, &address_info) < 0) {
		perror("getaddrinfo() failed");
        *err = -1;
		return 0;
	}

    // attempts to create an open socket on the system.
    if ((self->m_socketFd = socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol)) < 0) {
		perror("socket() failed");
        *err = -2;
		return 0;
	}

    // attempts to connect to the server via socket.
    if (connect(self->m_socketFd, address_info->ai_addr, address_info->ai_addrlen) < 0) {
		perror("connect() failed");
        *err = -3;
		return 0;
	}

    // make space for the userlist string and clear it.
    self->m_userlist = malloc(sizeof(char) * BUFFER_SIZE);
    memset(self->m_userlist, '\0', sizeof(char) * BUFFER_SIZE);

    // return success.
    return 1;
}



/* Closes the socket and frees the memory, essentially resetting the Client. */
void client_disconnect(Client *self) {
    // close the socket, if its still open.
    if (self->m_socketFd != -1) {
        close(self->m_socketFd);
        self->m_socketFd = -1;
    }

    // free the username memory, if its not been freed yet.
    if (self->m_username != NULL) {
        free(self->m_username);
        self->m_username = NULL;
    }

    // free the userlist memory, if its not been freed yet.
    if (self->m_userlist != NULL) {
        free(self->m_userlist);
        self->m_userlist = NULL;
    }
}



/* Attempts to login to the server. */
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

        // make space for the username string and fill it.
        self->m_username = malloc(sizeof(char) * MAX_USERNAME_LEN);
        memset(self->m_username, '\0', sizeof(char) * MAX_USERNAME_LEN);
        strcpy(self->m_username, username);

        // install the polling function to run every few hundred ms.
        g_timeout_add(MILLI_SLEEP_DUR, (void *)server_poll, self);

        // return success.
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



/* Sends the message to all connected users. */
int client_send_broadcast(Client *self, const char *message) {
    // formats the message so the server can parse it.
    char outgoing[BUFFER_SIZE];
    memset(outgoing, '\0', BUFFER_SIZE);
    sprintf(outgoing, "/broadcast %s", message);

    // writes it to the server.
    if (write(self->m_socketFd, outgoing, strlen(outgoing)) < 0) {
        printf("\'write\' failed during broadcast\n");
        return 0;
    }

    return 1;
}



/* Sends the message to the recipient. */
int client_send_private_message(Client *self, const char *recipient, const char *message) {
    // formats the message so the server can parse it.
    char outgoing[BUFFER_SIZE];
    memset(outgoing, '\0', BUFFER_SIZE);
    sprintf(outgoing, "/whisper %s %s", recipient, message);

    // writes it to the server.
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



/* Initializes the Client class */
static void client_class_init (ClientClass *class) {
    /* Fires on the client instance when a new message is received. */
    g_signal_new("message-received", CLIENT_TYPE_OBJECT, G_SIGNAL_RUN_FIRST,
    	0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER);

    /* Fires on the client instance when a new private message is received. */
    g_signal_new("private-message-received", CLIENT_TYPE_OBJECT, G_SIGNAL_RUN_FIRST,
    	0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER);

    /* Fires on the client instance when a user has joined or left the chat. */
    g_signal_new("userlist-updated", CLIENT_TYPE_OBJECT, G_SIGNAL_RUN_FIRST,
    	0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_POINTER);

    /* Fires on the client when the connection to the server is lost. */
    g_signal_new("connection-lost", CLIENT_TYPE_OBJECT, G_SIGNAL_RUN_FIRST,
    	0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}



/* Initializes the Client instance. */
static void client_init (Client *self) {
    // nullify the parameters initially.
	self->m_socketFd = -1;
    self->m_username = NULL;
    self->m_userlist = NULL;
}
