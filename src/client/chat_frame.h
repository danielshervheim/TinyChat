//
// Copyright © Daniel Shervheim, 2018-2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#ifndef CHAT_FRAME_H_
#define CHAT_FRAME_H_

#include <gtk/gtk.h>


G_BEGIN_DECLS

#define CHAT_FRAME_TYPE_BIN (chat_frame_get_type ())
G_DECLARE_FINAL_TYPE(ChatFrame, chat_frame, CHAT_FRAME, BIN, GtkBin)

/* Returns a new ChatFrame instance. */
ChatFrame* chat_frame_new(void);

G_END_DECLS

#endif  // CHAT_FRAME_H_
