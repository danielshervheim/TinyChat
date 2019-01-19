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

/* Attempts to connect to the server.
(ret: 1 success, 0 failure. err: -1 getaddrinfo, -2 socket, -3 connect) */
int client_connect(Client *self, const char *port, const char *address, int *err);

/* Closes the socket and frees the memory, essentially resetting the Client. */
void client_disconnect(Client *self);

/* Attempts to login to the server.
(ret: 1 success, 0 failure. err: -1 username taken, -2 server full, -3 unspecified)  */
int client_login(Client *self, const char *username, int *err);

/* Sends the message to all connected users.
(ret: 1 success, 0 failure). */
int client_send_broadcast(Client *self, const char *message);

/* Sends the message to the recipient.
(ret: 1 success, 0 failure). */
int client_send_private_message(Client *self, const char *recipient, const char *message);

G_END_DECLS

#endif  // CLIENT_H_
