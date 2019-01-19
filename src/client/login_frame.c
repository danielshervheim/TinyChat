//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#include "login_frame.h"

#include "client.h"

struct _LoginFrame {
    GtkBin parent_instance;
    // member instances go here

    GtkEntry *m_address;
    GtkEntry *m_port;
    GtkEntry *m_username;

};

G_DEFINE_TYPE(LoginFrame, login_frame, GTK_TYPE_BIN);





void on_connect_intent(LoginFrame *self) {
	const gchar *address = gtk_entry_get_text(self->m_address);
	const gchar *port = gtk_entry_get_text(self->m_port);
	const gchar *username = gtk_entry_get_text(self->m_username);
	g_signal_emit_by_name(self, "connect-intent", address, port, username);
}




/* Returns a new instance of LoginFrame. */
LoginFrame* login_frame_new () {
    return g_object_new (LOGIN_FRAME_TYPE_BIN, NULL);
}

/* Initializes the LoginFrame class */
static void login_frame_class_init (LoginFrameClass *class) {
	/* Fires on the LoginFrame instance when the user presses the connect button. */
    g_signal_new("connect-intent", LOGIN_FRAME_TYPE_BIN, G_SIGNAL_RUN_FIRST,
    	0, NULL, NULL, NULL, G_TYPE_NONE, 3, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER);
}

/* Initializes the LoginFrame instance. */
static void login_frame_init (LoginFrame *self) {
	// get a reference to the builder
	GtkBuilder *builder = gtk_builder_new_from_resource("/tinychat/glade/login_frame.glade");

	// get the main container from the builder and add it to the window
	GtkWidget *content = GTK_WIDGET(gtk_builder_get_object(builder, "login_box"));
	gtk_container_add(GTK_CONTAINER(self), content);
	gtk_widget_show_all(content);

	// get the adjustments reference
	self->m_address = GTK_ENTRY(gtk_builder_get_object(builder, "address_entry"));
	self->m_port = GTK_ENTRY(gtk_builder_get_object(builder, "port_entry"));
	self->m_username = GTK_ENTRY(gtk_builder_get_object(builder, "username_entry"));
	
	// get the login button reference and set its icon
	GtkWidget *connectButton = GTK_WIDGET(gtk_builder_get_object(builder, "connect_button"));
	gtk_button_set_image(GTK_BUTTON(connectButton),
		gtk_image_new_from_resource("/tinychat/icons/icons8-enter-16.png"));

	// connect the appropriate signal handlers
	g_signal_connect_swapped(connectButton, "clicked", (GCallback)on_connect_intent, self);
	g_signal_connect_swapped(self->m_address, "activate", (GCallback)on_connect_intent, self);
	g_signal_connect_swapped(self->m_port, "activate", (GCallback)on_connect_intent, self);
	g_signal_connect_swapped(self->m_username, "activate", (GCallback)on_connect_intent, self);

	// unref the builder
	g_object_unref(builder);
}
