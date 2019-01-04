//
// 2018-2019 Daniel Shervheim
// danielshervheim@gmail.com
// github.com/danielshervheim
//

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// network includes
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "common.h"



//
// GLOBAL variables
//

GtkBuilder *m_builder;
GtkWindow *m_main_window;
int m_socket_fd = -1;
const gchar *m_username;
char m_userlist[MAX_CONCURRENT_USERS][MAX_NAME_LEN];
int m_message_odd_even = 0;



//
// FUNCTION headers
//

// utilities
void display_message_dialog(const gchar *title, const gchar *description, GtkMessageType type);
int is_valid_address(const gchar *address);
int is_valid_port(const gchar *port);
int is_valid_username(const gchar *username);

// client display functions
void client_display_message(const char* title, const char* body, GtkAlign halign, const char* title_class, const char* body_class);
void client_parse_shout(const char* original_buf);
void client_parse_shoutcc(const char* buf);
void client_parse_whisper(const char* original_buf);
void client_parse_whispercc(const char* original_buf);
void client_add_user_button(const char* username, GtkBox *box);
void client_update_userlist(const char* original_buf);
void client_clear_userlist();
void client_update_message_entry();
void client_clear_message_entry();
void client_set_message_entry(const char* buf);
void client_update_char_counter_label();

// server management functions
void server_send_message(const gchar *entry_buf);
int server_poll();
void server_login();
void server_logout();

// gui callbacks
void on_window_destroy(GtkWindow *window);
void on_connect_button_clicked();
void on_address_entry_activate();
void on_port_entry_activate();
void on_username_entry_activate();
void on_message_entry_activate();
void on_message_entry_changed();
void on_send_button_clicked();
void on_user_button_clicked(GtkButton *button);



//
// UTILITIES
//

void display_message_dialog(const gchar *title, const gchar *description, GtkMessageType type) {
	GtkMessageDialog *message_dialog = GTK_MESSAGE_DIALOG(gtk_message_dialog_new(
		m_main_window, GTK_DIALOG_MODAL, type, GTK_BUTTONS_CLOSE, "%s", title));

	gtk_message_dialog_format_secondary_text(message_dialog, "%s", description);

	if (gtk_dialog_run(GTK_DIALOG(message_dialog)) == GTK_RESPONSE_CLOSE) {
		gtk_widget_destroy(GTK_WIDGET(message_dialog));
	}
}

int is_valid_address(const gchar *address) {
    return strlen(address) > 0;
}

int is_valid_port(const gchar *port) {
    return (atoi(port) >= 1024) && (atoi(port) <= 65535);
}

int is_valid_username(const gchar *username) {
	// verify there are no spaces in the username
	if (strchr(username, ' ') != NULL) {
		return 0;
	}
	else {
    	return strlen(username) > 0 && strlen(username) <= MAX_NAME_LEN;
	}
}



//
// CLIENT functions
//

void client_display_message(const char* title, const char* body,
	GtkAlign halign, const char* title_class, const char* body_class) {
	// gets the main message display box
	GtkBox *main_message_box = GTK_BOX(gtk_builder_get_object(m_builder, "messages_box"));

	// messages have the format:
	/*
		- message_container (#message_container)
			- content_box (#message_content_area)
				- title_label (#message_title)
				- body_label (#message_body)
			- seperator (#message_seperator)
	*/

	// create the content_box
	GtkBox *content_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
	gtk_widget_set_name(GTK_WIDGET(content_box), "message_content_area");

	// create the title label and apply its class
	GtkLabel *title_label = GTK_LABEL(gtk_label_new(title));
	gtk_widget_set_name(GTK_WIDGET(title_label), "message_title");
	GtkStyleContext *title_context = gtk_widget_get_style_context(GTK_WIDGET(title_label));
	gtk_style_context_add_class(title_context, title_class);

	// create the body_label and apply its class
	GtkLabel *body_label = GTK_LABEL(gtk_label_new(body));
	gtk_widget_set_name(GTK_WIDGET(body_label), "message_body");
	GtkStyleContext *body_context = gtk_widget_get_style_context(GTK_WIDGET(body_label));
	gtk_style_context_add_class(body_context, title_class);

	// apply the halign styles to the label(s)
	gtk_widget_set_halign(GTK_WIDGET(title_label), halign);
	gtk_widget_set_halign(GTK_WIDGET(body_label), halign);

	// apply the line-wrap settings to the body label
	gtk_label_set_line_wrap (body_label, 1);
	gtk_label_set_line_wrap_mode(body_label, PANGO_WRAP_CHAR);

	// make both body and title selectable
	gtk_label_set_selectable(title_label, 1);
	gtk_label_set_selectable(body_label, 1);

	// pack the label(s) into the content_box
	gtk_box_pack_start(content_box, GTK_WIDGET(title_label), 0, 0, 0);
	gtk_box_pack_start(content_box, GTK_WIDGET(body_label), 0, 0, 0);

	// create the message_container and seperator
	GtkBox *message_container = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
	gtk_widget_set_name(GTK_WIDGET(message_container), "message_container");
	GtkWidget *seperator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_set_name(GTK_WIDGET(seperator), "message_seperator");

	// add an even-odd class to the message_container for css styling purposes
	GtkStyleContext *container_context = gtk_widget_get_style_context(GTK_WIDGET(message_container));
	if (m_message_odd_even == 0) {
		gtk_style_context_add_class(container_context, "even_message");
	}
	else {
		gtk_style_context_add_class(container_context, "odd_message");
	}
	m_message_odd_even = 1 - m_message_odd_even;

	// and pack them
	gtk_box_pack_start(message_container, GTK_WIDGET(content_box), 0, 0, 0);
	gtk_box_pack_start(message_container, seperator, 0, 0, 0);

	// finally, show them all
	gtk_box_pack_start(main_message_box, GTK_WIDGET(message_container), 0, 0, 0);
	gtk_widget_show_all(GTK_WIDGET(message_container));
}

void client_parse_shout(const char* original_buf) {
	// buf is of format "<sender> <message>"
	
	// strtok modifies string, so we first make a copy
	char buf[BUF_SIZE];
	memset(buf, '\0', BUF_SIZE);
	strcpy(buf, original_buf);

	char *sender = strtok(buf, " ");
	const char *message = original_buf + strlen(sender) + 1;

	// format the outgoing title
	char title[BUF_SIZE];
	memset(title, '\0', BUF_SIZE);
	sprintf(title, "%s said", sender);

	client_display_message(title, message, GTK_ALIGN_START, "shout", "shout");
}

void client_parse_shoutcc(const char* buf) {
	// buf is of format "<message>"
	
	client_display_message("you said", buf, GTK_ALIGN_END, "shoutcc", "shoutcc");
}

void client_parse_whisper(const char* original_buf) {
	// buf is of format "<sender> <message>"
	
	// strtok modifies string, so we first make a copy
	char buf[BUF_SIZE];
	memset(buf, '\0', BUF_SIZE);
	strcpy(buf, original_buf);

	char *sender = strtok(buf, " ");
	const char *message = original_buf + strlen(sender) + 1;

	// format the outgoing title
	char title[BUF_SIZE];
	memset(title, '\0', BUF_SIZE);
	sprintf(title, "%s @ you", sender);

	client_display_message(title, message, GTK_ALIGN_START, "whisper", "whisper");
}

void client_parse_whispercc(const char* original_buf) {
	// buf is of format "<recipient> <message>"
	
	// strtok modifies string, so we first make a copy
	char buf[BUF_SIZE];
	memset(buf, '\0', BUF_SIZE);
	strcpy(buf, original_buf);

	char *recipient = strtok(buf, " ");
	const char *message = original_buf + strlen(recipient) + 1;

	// format the outgoing title
	char title[BUF_SIZE];
	memset(title, '\0', BUF_SIZE);
	sprintf(title, "you @ %s", recipient);

	client_display_message(title, message, GTK_ALIGN_END, "whispercc", "whispercc");
}

void client_add_user_button(const char* username, GtkBox *box) {
	GtkButton *user_button = GTK_BUTTON(gtk_button_new_with_label(username));
	gtk_widget_set_name(GTK_WIDGET(user_button), "user_button");
	g_signal_connect(user_button, "clicked", (GCallback)on_user_button_clicked, NULL);

	gtk_box_pack_start(box, GTK_WIDGET(user_button), 0, 0, 0);
	gtk_widget_show_all(GTK_WIDGET(user_button));
}

void client_update_userlist(const char* original_buf) {
	// original_buf is of format "<username1> <username2> ... up to ... <username10>"

	// clear out the userlist first
	client_clear_userlist();

	// strtok modifies buffer, so we first make a copy
	char buf[BUF_SIZE];
	memset(buf, '\0', BUF_SIZE);
	strcpy(buf, original_buf);

	// then repopulate it
	char *token = strtok(buf, " ");
	int num_connected = 0;

	while (token != NULL) {
		strcpy(m_userlist[num_connected], token);
		token = strtok(NULL, " ");
		num_connected++;
	}

	char tmp[BUF_SIZE];
	memset(tmp, '\0', BUF_SIZE);

	// update the label(s)
	GtkLabel *maxconnected_label = GTK_LABEL(gtk_builder_get_object(m_builder, "maxconnected_label"));
	sprintf(tmp, "%d", MAX_CONCURRENT_USERS);
	gtk_label_set_text(maxconnected_label, tmp);

	GtkLabel *numconnected_label = GTK_LABEL(gtk_builder_get_object(m_builder, "numconnected_label"));
	memset(tmp, '\0', BUF_SIZE);
	sprintf(tmp, "%d", num_connected);
	gtk_label_set_text(numconnected_label, tmp);

	// fill the box with each new user as a button
	GtkBox *connected_users_box = GTK_BOX(gtk_builder_get_object(m_builder, "connected_users_box"));
	for (int i = 0; i < num_connected; i++) {
		client_add_user_button(m_userlist[i], connected_users_box);
	}
}

void client_clear_userlist() {
	for (int i = 0; i < MAX_CONCURRENT_USERS; i++) {
		memset(m_userlist[i], '\0', MAX_NAME_LEN);
	}

	// clear out the connected users 
	GtkContainer *connected_users_box = GTK_CONTAINER(gtk_builder_get_object(m_builder, "connected_users_box"));
	gtk_container_foreach(connected_users_box, (GtkCallback)gtk_widget_destroy, NULL);
}

void client_update_message_entry() {
	GtkEntry *message_entry = GTK_ENTRY(gtk_builder_get_object(m_builder, "message_entry"));
	const gchar *buf = gtk_entry_get_text(message_entry);

	// check if message starts with reserved character
	if (memcmp(buf, "/", strlen("/")) == 0) {
		client_clear_message_entry();
		display_message_dialog("Input Error", "Messages cannot start with the \"/\" character.", GTK_MESSAGE_WARNING);
	}
	else {
		client_update_char_counter_label();
	}
}

void client_clear_message_entry() {
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(m_builder, "message_entry")), "");
	client_update_char_counter_label();
}

void client_set_message_entry(const char* buf) {
	GtkEntry *message_entry = GTK_ENTRY(gtk_builder_get_object(m_builder, "message_entry"));
	client_clear_message_entry();
	gtk_entry_grab_focus_without_selecting(message_entry);
	gtk_entry_set_text(message_entry, buf);
	gtk_editable_set_position(GTK_EDITABLE(message_entry), -1);  // move cursor to end of entry
	client_update_char_counter_label();
}

void client_update_char_counter_label() {
	// update the character counter label
	GtkLabel *char_counter = GTK_LABEL(gtk_builder_get_object(m_builder, "char_counter_label"));
	
	char tmp[BUF_SIZE];
	memset(tmp, '\0', BUF_SIZE);
	sprintf(tmp, "%d", MAX_MSG_LEN - gtk_entry_get_text_length(GTK_ENTRY(gtk_builder_get_object(m_builder, "message_entry"))));
	
	gtk_label_set_text(char_counter, tmp);
}



//
// SERVER functions
//

void server_send_message(const gchar *entry_buf) {
	// note: the buffer passed in is memory-managed by gtk
	// so we must copy it before modifying it
	char buf[BUF_SIZE];
	memset(buf, '\0', BUF_SIZE);
	strcpy(buf, entry_buf);

	if (strlen(entry_buf) < 1) {
		display_message_dialog("Input Error", "Messages must be at least 1 character long.", GTK_MESSAGE_WARNING);
	}
	else if (memcmp(buf, "@", strlen("@")) == 0) {
		// send as whisper

		// strtok modifies the original string so we must
		// make another copy to single out the recipient
		char rec_buf[BUF_SIZE];
		memset(rec_buf, '\0', BUF_SIZE);
		strcpy(rec_buf, buf + strlen("@"));

		char *recipient = strtok(rec_buf, " ");
		char *message = buf + strlen("@") + strlen(recipient) + 1;

		// verify the recipient is not the user themself
        if (strcmp(recipient, m_username) == 0) {
        	display_message_dialog("@ Error", "You cannot @ yourself.", GTK_MESSAGE_WARNING);
        	return;
        }

        // verify the intended recipient is connected
        int recipient_is_connected = 0;
        for (int i = 0; i < MAX_CONCURRENT_USERS; i++) {
        	if (strcmp(recipient, m_userlist[i]) == 0) {
        		recipient_is_connected = 1;
        		i = MAX_CONCURRENT_USERS;
        	}
        }
        if (recipient_is_connected == 0) {
        	display_message_dialog("Delivery Failure", "Intended recipient is not connected to the server.", GTK_MESSAGE_ERROR);
        	return;
        }

        char outgoing[BUF_SIZE];
        memset(outgoing, '\0', BUF_SIZE);
        sprintf(outgoing, "/whisper %s %s", recipient, message);

        if (write(m_socket_fd, outgoing, strlen(outgoing)) < 0) {
        	perror("write() failed during /whisper");
        	display_message_dialog("Delivery Failure", "Message could not be delivered.", GTK_MESSAGE_ERROR);
        	return;
        }
        else {
        	client_clear_message_entry();
        }
	}
	else {
		// send as shout
		char outgoing[BUF_SIZE];
		memset(outgoing, '\0', BUF_SIZE);
		sprintf(outgoing, "/shout %s", buf);

		if (write(m_socket_fd, outgoing, strlen(outgoing)) < 0) {
			perror("write() failed during /shout");
			display_message_dialog("Delivery Failure", "Message could not be delievered.", GTK_MESSAGE_ERROR);
			return;
		}
		else {
			client_clear_message_entry();
		}
	}
}

int server_poll() {
	// temporary buffer to hold a potential message
	char buf[BUF_SIZE];
	memset(buf, '\0', BUF_SIZE);
	ssize_t nread;

	if ((nread = read(m_socket_fd, buf, BUF_SIZE)) != -1) {
		if (nread == 0) {
			display_message_dialog("Connection Lost", "Connection to the server has been lost.", GTK_MESSAGE_ERROR);
			server_logout();
			return 0;
		}
		else {
			// note: "cc" MUST come first because "whispered" is a subset of "whisperedcc"
			if (memcmp(buf, "/whisperedcc", strlen("/whisperedcc")) == 0) {
				client_parse_whispercc(buf + strlen("/whisperedcc") + 1);
			}
			else if (memcmp(buf, "/whispered", strlen("/whispered")) == 0) {
				client_parse_whisper(buf + strlen("/whispered") + 1);
			}
			else if (memcmp(buf, "/shoutedcc", strlen("/shoutedcc")) == 0) {
				client_parse_shoutcc(buf + strlen("/shoutedcc") + 1);
			}
			else if (memcmp(buf, "/shouted", strlen("/shouted")) == 0) {
				client_parse_shout(buf + strlen("/shouted") + 1);
			}
			else if (memcmp(buf, "/userlist", strlen("/userlist")) == 0) {
				client_update_userlist(buf + strlen("/userlist") + 1);
			}
			// other commands here eventually...
		}
	}

	return 1;
}

void server_login() {
	// get the address and verify its valid
	const gchar *address = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(m_builder, "address_entry")));
	if (!is_valid_address(address)) {
		display_message_dialog("Invalid Address", "Address must be 1 character or greater.", GTK_MESSAGE_WARNING);
		return;
	}

	// get the port and verify that its valid
	const gchar *port = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(m_builder, "port_entry")));
	if (!is_valid_port(port)) {
		display_message_dialog("Invalid Port", "Port must be between 1024 and 65535 inclusive.", GTK_MESSAGE_WARNING);
		return;
	}

	// get the username and verify that its valid
	const gchar *username = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(m_builder, "username_entry")));
	if (!is_valid_username(username)) {
		char invalid_username_message[BUF_SIZE];
        sprintf(invalid_username_message, "Username must be between 1 and %d characters inclusive, and must not contain spaces.", MAX_NAME_LEN);
		display_message_dialog("Invalid Username", invalid_username_message, GTK_MESSAGE_WARNING);
		return;
	}
	else {
		m_username = username;
	}

	// at this point the login information is valid, so attempt to connect to the server

	struct addrinfo *address_info;
	if (getaddrinfo(address, port, NULL, &address_info) < 0) {
		perror("getaddrinfo() failed");
		display_message_dialog("Connection Error", "Unable to resolve address.", GTK_MESSAGE_ERROR);
		return;
	}

	if ((m_socket_fd = socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol)) < 0) {
		perror("socket() failed");
		display_message_dialog("Connection Error", "Unable to allocate system resources.", GTK_MESSAGE_ERROR);
		return;
	}

	if (connect(m_socket_fd, address_info->ai_addr, address_info->ai_addrlen) < 0) {
		perror("connect() failed");
		display_message_dialog("Connection Error", "Unable to contact server.", GTK_MESSAGE_ERROR);
		return;
	}

	// at this point we are succesfully connected to the server, so initiate handshake

	char buf[BUF_SIZE];
	memset(buf, '\0', BUF_SIZE);
	sprintf(buf, "/join %s", username);

	if (write(m_socket_fd, buf, strlen(buf)) < 0) {
		perror("write() failed during handshake");
		display_message_dialog("Connection Error", "Unable to communicate with server.", GTK_MESSAGE_ERROR);
		server_logout();
		return;
	}

	// clear the buf and reuse it to read in the response
	memset(buf, '\0', BUF_SIZE);
	if (read(m_socket_fd, buf, BUF_SIZE) <= 0) {
		perror("read() failed during handshake");
		display_message_dialog("Connection Error", "Unable to communicate with server.", GTK_MESSAGE_ERROR);
		server_logout();
		return;
	}

	if (memcmp(buf, "/joinresponse ok", strlen("/joinresponse ok")) == 0) {
		// set the socket to be non-blocking so we can poll it
		if (fcntl(m_socket_fd, F_SETFL, O_NONBLOCK) < 0) {
			perror("fcntl() failed during handshake");
			display_message_dialog("Connection Error", "Unable to allocate system resources.", GTK_MESSAGE_ERROR);
			server_logout();
			return;
		}

		// initialize the chat gui
		client_clear_message_entry();

		// set the chat pane as the visible window
		gtk_stack_set_visible_child(
    		GTK_STACK(gtk_builder_get_object(m_builder, "main_stack")),
    		GTK_WIDGET(gtk_builder_get_object(m_builder, "chat_pane")));

		// begin server polling
		g_timeout_add(MILLI_SLEEP_DUR, server_poll, NULL);
	}
	else if (memcmp(buf, "/joinresponse username_taken", strlen("/joinresponse username_taken")) == 0) {
		display_message_dialog("Connection Denied", "Username is already in use.", GTK_MESSAGE_WARNING);
		server_logout();
	}
	else if (memcmp(buf, "/joinresponse server_full", strlen("/joinresponse server_full")) == 0) {
		display_message_dialog("Connection Denied", "Server is already full.", GTK_MESSAGE_WARNING);
		server_logout();
	}
	else {
		// something has gone seriously wrong...
		display_message_dialog("Connection Denied", "Something went seriously wrong...", GTK_MESSAGE_ERROR);
		server_logout();
	}
}

void server_logout() {
	close(m_socket_fd);
	m_socket_fd = -1;

	// set the login pane as the visible window
	gtk_stack_set_visible_child(
    	GTK_STACK(gtk_builder_get_object(m_builder, "main_stack")),
    	GTK_WIDGET(gtk_builder_get_object(m_builder, "login_pane")));

	// remove the currently printed messages
	gtk_container_foreach(GTK_CONTAINER(gtk_builder_get_object(m_builder, "messages_box")),
		(GtkCallback)gtk_widget_destroy, NULL);
}




//
// GUI callbacks
//

void on_window_destroy(GtkWindow *window) {
	if (window == m_main_window) {
		
		// close the connection to the server, if its open
		if (m_socket_fd != -1) {
			server_logout();
		}

		// unload gtk
        g_object_unref(m_builder);
        gtk_main_quit();
	}
}

void on_connect_button_clicked() {
	server_login();
}

void on_address_entry_activate() {
	server_login();
}

void on_port_entry_activate() {
    server_login();
}

void on_username_entry_activate() {
    server_login();
}

void on_message_entry_activate() {
	const gchar *buf = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(m_builder, "message_entry")));
	server_send_message(buf);
}

void on_message_entry_changed() {
	client_update_message_entry();
}

void on_send_button_clicked() {
	const gchar *buf = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(m_builder, "message_entry")));
	server_send_message(buf);
}

void on_user_button_clicked(GtkButton *button) {
	const gchar *username = gtk_button_get_label(button);
	
	// close the users popup, if its still open
	GtkPopover *users_popover = GTK_POPOVER(gtk_builder_get_object(m_builder, "users_popover"));
	gtk_popover_popdown(users_popover);

	// set the message entry to whisper mode
	char tmp[BUF_SIZE];
	memset(tmp, '\0', BUF_SIZE);
	sprintf(tmp, "@%s ", username);
	client_set_message_entry(tmp);
}



//
// Runs the client GUI
//
int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);
    
    // load the glade files into the builder
    m_builder = gtk_builder_new();
    gtk_builder_add_from_file(m_builder, "style/client.glade", NULL);
    
    // save a reference to the main window
    m_main_window = GTK_WINDOW(gtk_builder_get_object(m_builder, "main_window"));
    
    // make sure the login pane is the first visible widget
    gtk_stack_set_visible_child(
    	GTK_STACK(gtk_builder_get_object(m_builder, "main_stack")),
    	GTK_WIDGET(gtk_builder_get_object(m_builder, "login_pane")));
    
    // set the message entry max length
    gtk_entry_set_max_length(GTK_ENTRY(gtk_builder_get_object(m_builder, "message_entry")), MAX_MSG_LEN);

   	// set the stylsheet
    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(css_provider, "style/client.css", NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default (), GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    // start the gui
    gtk_builder_connect_signals(m_builder, NULL);
    gtk_window_present(m_main_window);                
    gtk_main();
}
