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



/* Fires once when the app first starts. */
static void tinychat_app_init (TinyChatApp *self) { }



/* Fires when the user opens TinyChat without arguments (i.e. from the launcher) */
static void tinychat_app_activate(GApplication *app) {
    // create a new ClientWindow instance.
    ClientWindow *cw = client_window_new();

    // add it to this app and show it.
	gtk_application_add_window(GTK_APPLICATION(app), GTK_WINDOW(cw));
	gtk_window_present(GTK_WINDOW(cw));
}



/* Fires when the user opens TinyPaint with arguments (i.e. right click->open in). */
static void tinychat_app_open (GApplication *app, GFile **files, gint n_files, const gchar *hint) { }



/* Initializes the TinyChatAppClass. */
static void tinychat_app_class_init(TinyChatAppClass *class) {
    // virtual function overrides go here
    G_APPLICATION_CLASS(class)->activate = tinychat_app_activate;
    G_APPLICATION_CLASS(class)->open = tinychat_app_open;
}



/* Returns a new instance of TinyChatApp. */
TinyChatApp* tinychat_app_new(void) {
    return g_object_new(TINYCHAT_APP_TYPE_APPLICATION,
                        "application-id", "danshervheim.tinychat",
                        NULL);
}
