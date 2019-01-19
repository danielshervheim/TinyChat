//
// Copyright © Daniel Shervheim, 2018-2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#ifndef CHAT_FRAME_H_
#define CHAT_FRAME_H_

#include <gtk/gtk.h>

#include "common.h"

G_BEGIN_DECLS

#define CHAT_FRAME_TYPE_BIN (chat_frame_get_type ())
G_DECLARE_FINAL_TYPE(ChatFrame, chat_frame, CHAT_FRAME, BIN, GtkBin)

/* Returns a new ChatFrame instance. */
ChatFrame* chat_frame_new(void);

/* Resets the ChatFrame to a like new state - erases all the previously
displayed messages, clears the userlist, etc */
void chat_frame_reset(ChatFrame *self);

/* Updates the list of users available to send messages to. */
void chat_frame_update_userlist(ChatFrame *self, const char *userlist);

/* Adds a message (received from Client) to the ChatFrame and displays it. */
void chat_frame_add_message(ChatFrame *self, const char *sender, const char *message);

/* Adds a private message (received from Client) to the ChatFrame and displays it. */
void chat_frame_add_private_message(ChatFrame *self, const char *sender, const char *message);

G_END_DECLS

#endif  // CHAT_FRAME_H_
