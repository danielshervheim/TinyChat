//
// Copyright © Daniel Shervheim, 2018-2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#ifndef CLIENT_H_
#define CLIENT_H_

#include <gtk/gtk.h>


G_BEGIN_DECLS

#define CLIENT_TYPE_OBJECT (client_get_type ())
G_DECLARE_FINAL_TYPE(Client, client, CLIENT, OBJECT, GObject)

/* Returns a new Client instance. */
Client* client_new(void);

void client_destroy(Client *self);

// returns 1 on success, 0 on failure
// (err: -1 getaddrinfo, -2 socket, -3 connect)
int client_connect(Client *self, const char *port, const char *address, int *err);

void client_disconnect(Client *self);

// returns 1 on success, 0 on failure
// (err: -1 username taken, -2 server full, -3 unspecified)
int client_login(Client *self, const char *username, int *err);

// returns 1 on success, 0 on failure
int client_send_broadcast(Client *self, const char *message);

// returns 1 on success, 0 on unspecified error, -1 on recipient not connected
int client_send_private_message(Client *self, const char *recipient, const char *message);

/*
Signals emitted
===============

broadcast-received (Client *self, const char *sender, const char *message)

private-message-received (Client *self, const char *sender, const char *message)

user-left (Client *self, const char *username)

user-joined (Client *self, const char *username)

userlist-updated (Client *self, const char **users, int numusers)

connection-lost (Client *self)
*/

void install_timer(Client *self);

G_END_DECLS

#endif  // CLIENT_H_
