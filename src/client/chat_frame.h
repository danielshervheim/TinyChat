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

void chat_frame_userlist_updated(ChatFrame *self, const char *userlist);

void chat_frame_add_message(ChatFrame *self, const char *sender, const char *message);

void chat_frame_add_private_message(ChatFrame *self, const char *sender, const char *message);

void chat_frame_clear_message_entry(ChatFrame *self);

// clear_chat_frame
//

G_END_DECLS

#endif  // CHAT_FRAME_H_
