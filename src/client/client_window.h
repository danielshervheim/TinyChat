//
// Copyright © Daniel Shervheim, 2018-2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#ifndef CLIENT_WINDOW_H_
#define CLIENT_WINDOW_H_

#include <gtk/gtk.h>


G_BEGIN_DECLS

#define CLIENT_WINDOW_TYPE_WINDOW (client_window_get_type ())
G_DECLARE_FINAL_TYPE(ClientWindow, client_window, CLIENT_WINDOW, WINDOW, GtkWindow)

/* Returns a new ClientWindow instance. */
ClientWindow* client_window_new(void);

G_END_DECLS

#endif  // CLIENT_WINDOW_H_
