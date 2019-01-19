//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#include "client_window.h"

#include "chat_frame.h"
#include "client.h"
#include "common.h"
#include "login_frame.h"

struct _ClientWindow {
    GtkWindow parent_instance;

    GtkStack *m_stack;
    LoginFrame *m_login_frame;
    ChatFrame *m_chat_frame;
    Client *m_client;
};

G_DEFINE_TYPE(ClientWindow, client_window, GTK_TYPE_WINDOW);



/* Attempts to login to the server via the ClientWindow Client member. */
void on_loginFrameConnectIntent(ClientWindow *self, const char *address, const char *port, const char *username) {
    int err;

    // verify that the address is valid and display an error dialog if its not.
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

    // verify that the port is valid and display an error dialog if its not.
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

    // verify that the username is valid and display an error dialog if its not.
    if (!is_valid_username(username, &err)) {
        GtkMessageDialog *dia = GTK_MESSAGE_DIALOG(gtk_message_dialog_new(GTK_WINDOW(self),
            GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE, "Invalid Username"));

        if (err == -1) {
            gtk_message_dialog_format_secondary_text(dia, "\'%s\' is too short.", username);
        }
        else if (err == -2) {
            gtk_message_dialog_format_secondary_text(dia, "\'%s\' is too long.", username);
        }
        else if (err == -3) {
            gtk_message_dialog_format_secondary_text(dia, "\'%s\' contains spaces.", username);
        }
        else {
            gtk_message_dialog_format_secondary_text(dia, "\'%s\' is not allowed.", username);
        }

        gtk_dialog_run(GTK_DIALOG(dia));
        gtk_widget_destroy(GTK_WIDGET(dia));

        return;
    }

    // attempt server connection, and display an error dialog if something went wrong.
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

    // attempt server login, and display an error dialog if something went wrong.
    if (!client_login(self->m_client, username, &err)) {
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

    // Set the ChatFrame as the visible stack child.
    gtk_stack_set_visible_child(self->m_stack, GTK_WIDGET(self->m_chat_frame));

    // Update the window title.
    char title[BUFFER_SIZE];
    memset(title, '\0', BUFFER_SIZE);
    sprintf(title, "%s @ %s:%s", username, address, port);
    gtk_window_set_title(GTK_WINDOW(self), title);
}



/* Reverts back to the login window when the Client's server connection is lost. */
void on_clientConnectionLost(ClientWindow *self) {
    // Disconnect from the client (officially).
    client_disconnect(self->m_client);

    // Show the LoginFrame as default.
    gtk_stack_set_visible_child(self->m_stack, GTK_WIDGET(self->m_login_frame));

    // Reset the ClientWindow title.
    gtk_window_set_title(GTK_WINDOW(self), "TinyChat");

    /* Reset the ChatFrame instance (deletes the previously displayed messages
    and clears the entry). */
    chat_frame_reset(self->m_chat_frame);

    // Display an error dialog alerting the user the connection was lost.
    GtkMessageDialog *dia = GTK_MESSAGE_DIALOG(gtk_message_dialog_new(GTK_WINDOW(self),
            GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE, "Connection Lost"));
    gtk_message_dialog_format_secondary_text(dia, "The connection to the server was lost. Sorry about that!");
    gtk_dialog_run(GTK_DIALOG(dia));
    gtk_widget_destroy(GTK_WIDGET(dia));
}



//
// GTK overidden methods
// Used for initializing and setting up the instance.
//

/* Returns a new instance of ClientWindow. */
ClientWindow* client_window_new () {
    return g_object_new (CLIENT_WINDOW_TYPE_WINDOW, NULL);
}



/* Initializes the ClientWindow class */
static void client_window_class_init (ClientWindowClass *class) { }



/* Initializes the ClientWindow instance. */
static void client_window_init (ClientWindow *self) {
    // set the window title and size.
    gtk_window_set_title(GTK_WINDOW(self), "TinyChat");
    gtk_widget_set_size_request(GTK_WIDGET(self), 480, 320);

    // set the css the window should use.
    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_resource(css, "/tinychat/css/tinychat_client.css");
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    // create the stack, login frame, chat frame, and client.
    self->m_stack = GTK_STACK(gtk_stack_new());
    self->m_login_frame = login_frame_new();
    self->m_chat_frame = chat_frame_new();
    self->m_client = client_new();

    // set the stack transition properties.
    gtk_stack_set_transition_type(self->m_stack, GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_stack_set_transition_duration(self->m_stack, 150);

    // disconnect from the Client when this ClientWindow is destroyed.
    g_signal_connect_swapped(self, "destroy",
        (GCallback)client_disconnect, self->m_client);

    // Begin login process when the LoginFrame button is pressed.
    g_signal_connect_swapped(self->m_login_frame, "connect-intent",
        (GCallback)on_loginFrameConnectIntent, self);

    // Send the private message via the Client when the ChatFrame send button is pressed.
    g_signal_connect_swapped(self->m_chat_frame, "send-private-message-intent",
        (GCallback)client_send_private_message, self->m_client);

    // Send the message via the Client when the ChatFrame send button is pressed.
    g_signal_connect_swapped(self->m_chat_frame, "send-message-intent",
        (GCallback)client_send_broadcast, self->m_client);

    // Pass the message to the ChatFrame to display, when one arrives from the Client.
    g_signal_connect_swapped(self->m_client, "message-received",
        (GCallback)chat_frame_add_message, self->m_chat_frame);

    // Pass the private message to the ChatFrame to display, when one arrives from the Client.
    g_signal_connect_swapped(self->m_client, "private-message-received",
        (GCallback)chat_frame_add_private_message, self->m_chat_frame);

    // Pass the updated userlist to the ChatFrame when one arrives from the Client.
    g_signal_connect_swapped(self->m_client, "userlist-updated",
        (GCallback)chat_frame_update_userlist, self->m_chat_frame);

    // Update this ClientWindow when the server connection is lost.
    g_signal_connect_swapped(self->m_client, "connection-lost",
        (GCallback)on_clientConnectionLost, self);

    // add the frames to the stack.
    gtk_stack_add_named(self->m_stack, GTK_WIDGET(self->m_login_frame), "login_frame");
    gtk_stack_add_named(self->m_stack, GTK_WIDGET(self->m_chat_frame), "chat_frame");

    // set the login frame as the default frame.
    gtk_stack_set_visible_child(self->m_stack, GTK_WIDGET(self->m_login_frame));

    // add the stack to the ClientWindow and show it.
    gtk_container_add(GTK_CONTAINER(self), GTK_WIDGET(self->m_stack));
    gtk_widget_show_all(GTK_WIDGET(self->m_stack));
}
