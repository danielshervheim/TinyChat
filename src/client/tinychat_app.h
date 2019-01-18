//
// Copyright © Daniel Shervheim, 2018-2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#ifndef TINYCHAT_APP_H_
#define TINYCHAT_APP_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TINYCHAT_APP_TYPE_APPLICATION (tinychat_app_get_type ())
G_DECLARE_FINAL_TYPE(TinyChatApp, tinychat_app, TINYCHAT_APP, APPLICATION, GtkApplication)

/* Returns a pointer to a new instance of TinyChatApp. */
TinyChatApp* tinychat_app_new(void);

G_END_DECLS

#endif  // TINYCHAT_APP_H_
