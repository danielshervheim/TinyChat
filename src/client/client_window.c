//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#include "client_window.h"

#include "chat_frame.h"
#include "login_frame.h"

#include "client.h"

struct _ClientWindow {
    GtkWindow parent_instance;

    // member instances go here

    Client *m_client;

    GtkStack *m_stack;
    ChatFrame *m_chat_frame;
    LoginFrame *m_login_frame;

};

G_DEFINE_TYPE(ClientWindow, client_window, GTK_TYPE_WINDOW);










void on_clientMessageReceived(ClientWindow *self, const char *sender, const char *message) {
    printf("%s: %s\n", sender, message);
}

void on_clientPrivateMessageReceived(ClientWindow *self, const char *sender, const char *message) {
    printf("%s: %s\n", sender, message);
}

void on_clientConnectionLost(ClientWindow *self) {
    client_disconnect(self->m_client);

    // switch stack back to login frame, destroy chat frame and replace with new instance
}


void on_clientUserJoined(ClientWindow *self, const char *username) {
    printf("%s joined\n", username);
}

void on_clientUserLeft(ClientWindow *self, const char *username) {
    printf("%s left\n", username);
}






/* Returns a new instance of ClientWindow. */
ClientWindow* client_window_new () {
    return g_object_new (CLIENT_WINDOW_TYPE_WINDOW, NULL);
}

/* Initializes the ClientWindow class */
static void client_window_class_init (ClientWindowClass *class) {
    // GObject property stuff would go here...
}

/* Initializes the ClientWindow instance. */
static void client_window_init (ClientWindow *self) {
    // set the window properties
	gtk_window_set_title(GTK_WINDOW(self), "TinyChat");
    gtk_widget_set_size_request(GTK_WIDGET(self), 320, 480);

    // setup the stack
    self->m_stack = GTK_STACK(gtk_stack_new());
    gtk_stack_set_transition_type(self->m_stack, GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_stack_set_transition_duration(self->m_stack, 150);

    // setup the frames
    self->m_login_frame = login_frame_new();
	self->m_chat_frame = chat_frame_new();

    // add the frames to the stack and set the login one as initial
    gtk_stack_add_named(self->m_stack, GTK_WIDGET(self->m_login_frame), "login_frame");
    gtk_stack_add_named(self->m_stack, GTK_WIDGET(self->m_chat_frame), "chat_frame");
    gtk_stack_set_visible_child(self->m_stack, GTK_WIDGET(self->m_login_frame));

    // add the stack to the window and show it
    gtk_container_add(GTK_CONTAINER(self), GTK_WIDGET(self->m_stack));
    gtk_widget_show_all(GTK_WIDGET(self->m_stack));


    self->m_client = client_new();
    // todo: connect these callbacks to chat_frame methods directly, and pass in self->m_chat_frame
    g_signal_connect_swapped(self->m_client, "message-received", (GCallback)on_clientMessageReceived, self);
    g_signal_connect_swapped(self->m_client, "private-message-received", (GCallback)on_clientPrivateMessageReceived, self);
    g_signal_connect_swapped(self->m_client, "connection-lost", (GCallback)on_clientConnectionLost, self);
    g_signal_connect_swapped(self->m_client, "user-joined", (GCallback)on_clientUserJoined, self);
    g_signal_connect_swapped(self->m_client, "user-left", (GCallback)on_clientUserLeft, self);


    // in press login button callback, connect and login to client








	// add stack and in stack spawn two instances of loginPane and chatPane
	// connect the needed signals from each
	// loginPane and chatPane both inherit from GtkBin -- only one item


	// this window has a parameter "Client m_client --- which is the main interface between the client connection and message passing code, etc"

	// this class overrides the following signals from the loginframe:
	// login-attempt()

    Client *client = client_new();
    int err;
    if (client_connect(client, "6002", "71.193.95.107", &err)) {
        if (client_login(client, "daniel", &err)) {

        }
        else {
            printf("login error %d\n", err);
        }
    }
    else {
        printf("connect error %d\n", err);
    }
}
