//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#include "client_window.h"

#include "chat_frame.h"
#include "login_frame.h"

#include "client.h"

#include "common.h"

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
    // disconnect from the client officially
    client_disconnect(self->m_client);

    // switch back to login frame
    gtk_stack_set_visible_child(self->m_stack, GTK_WIDGET(self->m_login_frame));

    // delete the current chat_window instance
    gtk_widget_destroy(GTK_WIDGET(self->m_chat_frame));
    self->m_chat_frame = NULL;

    // spawn error message
    GtkMessageDialog *dia = GTK_MESSAGE_DIALOG(gtk_message_dialog_new(GTK_WINDOW(self),
            GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE, "Connection Lost"));
    gtk_message_dialog_format_secondary_text(dia, "The connection to the server was lost. Sorry about that!");
    gtk_dialog_run(GTK_DIALOG(dia));
    gtk_widget_destroy(GTK_WIDGET(dia));
}


void on_clientUserJoined(ClientWindow *self, const char *username) {
    printf("%s joined\n", username);
}

void on_clientUserLeft(ClientWindow *self, const char *username) {
    printf("%s left\n", username);
}




void on_loginFrameConnectIntent(ClientWindow *self, const char *address, const char *port, const char *username) {
    // tmp
    printf("address: %s port: %s username: %s\n", address, port, username);

    int err;
    if (!is_valid_address(address, &err)) {
        GtkMessageDialog *dia = GTK_MESSAGE_DIALOG(gtk_message_dialog_new(GTK_WINDOW(self),
            GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE, "Invalid Address"));

        if (err == -1) {
            gtk_message_dialog_format_secondary_text(dia, "\'%s\' is too short.", address);
        }
        else {
            gtk_message_dialog_format_secondary_text(dia, "\'%s\' is too long.", address);
        }

        gtk_dialog_run(GTK_DIALOG(dia));
        gtk_widget_destroy(GTK_WIDGET(dia));

        return;
    }

    if (!is_valid_port(port, &err)) {
        GtkMessageDialog *dia = GTK_MESSAGE_DIALOG(gtk_message_dialog_new(GTK_WINDOW(self),
            GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE, "Invalid Port"));

        if (err == -1) {
            gtk_message_dialog_format_secondary_text(dia, "\'%s\' is too small.", port);
        }
        else {
            gtk_message_dialog_format_secondary_text(dia, "\'%s\' is too large.", port);
        }

        gtk_dialog_run(GTK_DIALOG(dia));
        gtk_widget_destroy(GTK_WIDGET(dia));

        return;
    }

    if (!is_valid_username(username, &err)) {
        GtkMessageDialog *dia = GTK_MESSAGE_DIALOG(gtk_message_dialog_new(GTK_WINDOW(self),
            GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE, "Invalid Username"));

        if (err == -1) {
            gtk_message_dialog_format_secondary_text(dia, "\'%s\' is too short.", username);
        }
        else if (err == -2) {
            gtk_message_dialog_format_secondary_text(dia, "\'%s\' is too long.", username);
        }
        else {
            gtk_message_dialog_format_secondary_text(dia, "\'%s\' contains spaces.", username);   
        }

        gtk_dialog_run(GTK_DIALOG(dia));
        gtk_widget_destroy(GTK_WIDGET(dia));

        return;
    }

    // attempt server connection
    if (!client_connect(self->m_client, port, address, &err)) {
        GtkMessageDialog *dia = GTK_MESSAGE_DIALOG(gtk_message_dialog_new(GTK_WINDOW(self),
            GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE, "Connection Error"));

        if (err == -1) {
            gtk_message_dialog_format_secondary_text(dia, "\'getaddrinfo\' system call failed.");
        }
        else if (err == -2) {
            gtk_message_dialog_format_secondary_text(dia, "\'socket\' system call failed.");
        }
        else {
            gtk_message_dialog_format_secondary_text(dia, "\'connect\' system call failed.");   
        }

        gtk_dialog_run(GTK_DIALOG(dia));
        gtk_widget_destroy(GTK_WIDGET(dia));
        
        return;
    }

    // attempt server login
    if (!client_login(self->m_client, username, &err)) {
        // (err: -1 username taken, -2 server full, -3 unspecified)
        GtkMessageDialog *dia = GTK_MESSAGE_DIALOG(gtk_message_dialog_new(GTK_WINDOW(self),
            GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE, "Login Error"));

        if (err == -1) {
            gtk_message_dialog_format_secondary_text(dia, "\'%s\' is already taken.", username);
        }
        else if (err == -2) {
            gtk_message_dialog_format_secondary_text(dia, "The server is already full.");
        }
        else {
            gtk_message_dialog_format_secondary_text(dia, "Sorry, we don't know what went wrong.");   
        }

        gtk_dialog_run(GTK_DIALOG(dia));
        gtk_widget_destroy(GTK_WIDGET(dia));
        
        return;
    }

    // todo: create a new chat frame instance, and set the initally connected users
    self->m_chat_frame = chat_frame_new();
    // chat_frame_set_initial_userlist(self->m_chat_frame, client->m_userlist); ????? 

    // switch to the new chat frame
    gtk_stack_set_visible_child(self->m_stack, GTK_WIDGET(self->m_chat_frame));
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

    // setup the login frame
    self->m_login_frame = login_frame_new();

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
    g_signal_connect_swapped(self->m_login_frame, "connect-intent", (GCallback)on_loginFrameConnectIntent, self);
}
