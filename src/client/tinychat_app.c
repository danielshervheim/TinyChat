//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#include "tinychat_app.h"

#include "client_window.h"
#include "tinychat_gresource.h"

struct _TinyChatApp {
    GtkApplication parent_instance;
};

G_DEFINE_TYPE(TinyChatApp, tinychat_app, GTK_TYPE_APPLICATION);

static void tinychat_app_init (TinyChatApp *self) {
	// initialize here (this fires once when the app is first started)
}

/* Fires when the user opens TinyChat without arguments (i.e. from the launcher) */
static void tinychat_app_activate(GApplication *app) {


	ClientWindow *cw = client_window_new();

	gtk_application_add_window(GTK_APPLICATION(app), GTK_WINDOW(cw));
	gtk_window_present(GTK_WINDOW(cw));
}

/* Fires when the user opens TinyPaint with arguments (i.e. right click->open in) */
static void tinychat_app_open (GApplication *app, GFile **files, gint n_files, const gchar *hint) {
   	// do nothing, tinychat doesn't support opening files
}

static void tinychat_app_class_init(TinyChatAppClass *class) {
    // virtual function overrides go here
    G_APPLICATION_CLASS(class)->activate = tinychat_app_activate;
    G_APPLICATION_CLASS(class)->open = tinychat_app_open;
}

TinyChatApp* tinychat_app_new(void) {
    return g_object_new(TINYCHAT_APP_TYPE_APPLICATION,
                        "application-id", "danshervheim.tinychat",
                        NULL);
}
