//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#include "client.h"

#include "common.h"

struct _Client {
    GObject parent_instance;
    // member instances go here
    char m_port[MAX_PORT_LEN];
    char m_address[MAX_ADDRESS_LEN];
    char m_username[MAX_USERNAME_LEN];
    int m_socketFd;
};

G_DEFINE_TYPE(Client, client, G_TYPE_OBJECT);

/* Returns a new instance of Client. */
Client* client_new () {
    return g_object_new (CLIENT_TYPE_OBJECT, NULL);
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

    /* Fires on the client instance when there is a change to the connected users. */
    g_signal_new("user-list-updated", CLIENT_TYPE_OBJECT, G_SIGNAL_RUN_FIRST,
    	0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_POINTER);

    /* Fires on the client when the connection to the server is lost. */
    g_signal_new("connection-lost", CLIENT_TYPE_OBJECT, G_SIGNAL_RUN_FIRST,
    	0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

/* Initializes the Client instance. */
static void client_init (Client *self) {
	printf("client init\n");

	memset(self->m_port, '\0', MAX_PORT_LEN);
	memset(self->m_address, '\0', MAX_ADDRESS_LEN);
	memset(self->m_username, '\0', MAX_USERNAME_LEN);

	self->m_socketFd = -1;
}
