//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#include "login_frame.h"

#include "client.h"
#include "common.h"

struct _LoginFrame {
    GtkBin parent_instance;

    GtkEntry *m_address;
    GtkEntry *m_port;
    GtkEntry *m_username;
};

G_DEFINE_TYPE(LoginFrame, login_frame, GTK_TYPE_BIN);



/* The user wants to login to the server. */
void on_connect_intent(LoginFrame *self) {
    // Get the text parameters.
	const gchar *address = gtk_entry_get_text(self->m_address);
	const gchar *port = gtk_entry_get_text(self->m_port);
	const gchar *username = gtk_entry_get_text(self->m_username);

    // And pass them via signal.
	g_signal_emit_by_name(self, "connect-intent", address, port, username);
}



/* Returns a new instance of LoginFrame. */
LoginFrame* login_frame_new () {
    return g_object_new (LOGIN_FRAME_TYPE_BIN, NULL);
}



/* Initializes the LoginFrame class */
static void login_frame_class_init (LoginFrameClass *class) {
	/* Passes the address, port, and username as a signal on this instance. */
    g_signal_new("connect-intent", LOGIN_FRAME_TYPE_BIN, G_SIGNAL_RUN_FIRST,
    	0, NULL, NULL, NULL, G_TYPE_NONE, 3, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER);
}



/* Initializes the LoginFrame instance. */
static void login_frame_init (LoginFrame *self) {
	// get a reference to the builder
	GtkBuilder *builder = gtk_builder_new_from_resource("/tinychat/glade/login_frame.glade");

	// get the main container from the builder and add it to the LoginFrame.
	GtkWidget *content = GTK_WIDGET(gtk_builder_get_object(builder, "login_box"));
	gtk_container_add(GTK_CONTAINER(self), content);

	// get the adjustments references and overide their handlers.
	self->m_address = GTK_ENTRY(gtk_builder_get_object(builder, "address_entry"));
    g_signal_connect_swapped(self->m_address, "activate", (GCallback)on_connect_intent, self);

	self->m_port = GTK_ENTRY(gtk_builder_get_object(builder, "port_entry"));
	g_signal_connect_swapped(self->m_port, "activate", (GCallback)on_connect_intent, self);

	self->m_username = GTK_ENTRY(gtk_builder_get_object(builder, "username_entry"));
	g_signal_connect_swapped(self->m_username, "activate", (GCallback)on_connect_intent, self);

    // get the login button reference and overide its handler.
	GtkWidget *connectButton = GTK_WIDGET(gtk_builder_get_object(builder, "connect_button"));
	g_signal_connect_swapped(connectButton, "clicked", (GCallback)on_connect_intent, self);

    // set the correct entry parameters.
	gtk_entry_set_max_length(self->m_port, 5);
	gtk_entry_set_max_length(self->m_username, MAX_USERNAME_LEN);

    // Show all in the LoginFrame.
	gtk_widget_show_all(content);

	// And finally, uunref the builder.
	g_object_unref(builder);
}
