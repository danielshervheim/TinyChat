//
// Copyright © Daniel Shervheim, 2018-2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#ifndef LOGIN_FRAME_H_
#define LOGIN_FRAME_H_

#include <gtk/gtk.h>


G_BEGIN_DECLS

#define LOGIN_FRAME_TYPE_BIN (login_frame_get_type ())
G_DECLARE_FINAL_TYPE(LoginFrame, login_frame, LOGIN_FRAME, BIN, GtkBin)

/* Returns a new LoginFrame instance. */
LoginFrame* login_frame_new(void);

G_END_DECLS

#endif  // LOGIN_FRAME_H_
