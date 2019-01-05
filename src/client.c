//
// 2018-2019 Daniel Shervheim
// danielshervheim@gmail.com
// github.com/danielshervheim
//

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
int m_socket_fd = -1;
const gchar *m_username;
char m_userlist[MAX_CONCURRENT_USERS][MAX_NAME_LEN];
int m_message_odd_even = 0;



//
// FUNCTION headers
//

// utilities
int is_valid_address(const gchar *address);
int is_valid_port(const gchar *port);
int is_valid_username(const gchar *username);
void display_dialog(const gchar *title, const gchar *description, GtkMessageType type);

// messages
void message_parse_shout(const char* original_buf);
void message_parse_shoutcc(const char* buf);
void message_parse_whisper(const char* original_buf);
void message_parse_whispercc(const char* original_buf);
void message_display(const char* title, const char* body,
    GtkAlign halign, const char* title_class, const char* body_class);

// userlist
void userlist_clear();
void userlist_add_user_button(const char* username, GtkBox *box);
void userlist_update(const char* original_buf);

// message entry
void character_counter_update();
void message_entry_clear();
void message_entry_set(const char* buf);
void message_entry_verify();
void message_send(const gchar *entry_buf);

// server
void server_logout();
int server_poll();
void server_login();

// gui callbacks
void on_window_destroy(GtkWindow *window);
void on_connect_button_clicked();
void on_address_entry_activate();
void on_port_entry_activate();
void on_username_entry_activate();
void on_message_entry_changed();
void on_message_entry_activate();
void on_send_button_clicked();
void on_user_button_clicked(GtkButton *button);

int main(int argc, char* argv[]);



//
// UTILITIES
//

/* whether the provided address is valid */
int is_valid_address(const gchar *address) {
    return strlen(address) > 0;
}


/* whether the provided port is valid */
int is_valid_port(const gchar *port) {
    return (atoi(port) >= 1024) && (atoi(port) <= 65535);
}


/* whether the provided username is valid */
int is_valid_username(const gchar *username) {
	// verify there are no spaces in the username
	if (strchr(username, ' ') != NULL) {
		return 0;
	}
	else {
    	return strlen(username) > 0 && strlen(username) <= MAX_NAME_LEN;
	}
}


/* displays a dialog box and deletes itself once the user acknowledges it
note: there is no real interactability here, its more to just notify the user when
something has gone wrong, or needs their attention.  */
void display_dialog(const gchar *title, const gchar *description, GtkMessageType type) {
	// the main window
	GtkWindow *main_window = GTK_WINDOW(gtk_builder_get_object(m_builder, "main_window"));

	GtkMessageDialog *message_dialog = GTK_MESSAGE_DIALOG(gtk_message_dialog_new(
		main_window, GTK_DIALOG_MODAL, type, GTK_BUTTONS_CLOSE, "%s", title));

	gtk_message_dialog_format_secondary_text(message_dialog, "%s", description);

	if (gtk_dialog_run(GTK_DIALOG(message_dialog)) == GTK_RESPONSE_CLOSE) {
		gtk_widget_destroy(GTK_WIDGET(message_dialog));
	}
}



//
// MESSAGE parsing / display functions
//

void message_parse_shout(const char* original_buf) {
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

	message_display(title, message, GTK_ALIGN_START, "shout", "shout");
}


void message_parse_shoutcc(const char* buf) {
	// buf is of format "<message>"
	
	message_display("you said", buf, GTK_ALIGN_END, "shoutcc", "shoutcc");
}


void message_parse_whisper(const char* original_buf) {
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

	message_display(title, message, GTK_ALIGN_START, "whisper", "whisper");
}


void message_parse_whispercc(const char* original_buf) {
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

	message_display(title, message, GTK_ALIGN_END, "whispercc", "whispercc");
}


/* creates gui corresponding to the input and adds it to the message list */
void message_display(const char* title, const char* body,
	GtkAlign halign, const char* title_class, const char* body_class) {
	// gets the main message display box
	GtkBox *main_message_box = GTK_BOX(gtk_builder_get_object(m_builder, "messages_box"));

	// messages have the format:
	/*
		- content_box (#message_container)
			- title_label (#message_title)
			- body_label (#message_body)
	*/

	// create the content_box
	GtkBox *container_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
	gtk_widget_set_name(GTK_WIDGET(container_box), "message_container");

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
	gtk_box_pack_start(container_box, GTK_WIDGET(title_label), 0, 0, 0);
	gtk_box_pack_start(container_box, GTK_WIDGET(body_label), 0, 0, 0);

	// add an even-off class to the message_container
	GtkStyleContext *container_context = gtk_widget_get_style_context(GTK_WIDGET(container_box));
	if (m_message_odd_even == 0) {
		gtk_style_context_add_class(container_context, "even_message");
	}
	else {
		gtk_style_context_add_class(container_context, "odd_message");
	}
	m_message_odd_even = 1 - m_message_odd_even;

	// then pack the container into the messages box, and show it
	gtk_box_pack_start(main_message_box, GTK_WIDGET(container_box), 0, 0, 0);
	gtk_widget_show_all(GTK_WIDGET(container_box));
}



//
// USER LIST display functions
//

/* clears the userlist of all currently connected users */
void userlist_clear() {
	for (int i = 0; i < MAX_CONCURRENT_USERS; i++) {
		memset(m_userlist[i], '\0', MAX_NAME_LEN);
	}

	// clear out the connected users 
	GtkContainer *connected_users_box = GTK_CONTAINER(gtk_builder_get_object(m_builder, "connected_users_box"));
	gtk_container_foreach(connected_users_box, (GtkCallback)gtk_widget_destroy, NULL);
}


/* adds a button for the given username to the userlist popover */
void userlist_add_user_button(const char* username, GtkBox *box) {
	GtkButton *user_button = GTK_BUTTON(gtk_button_new_with_label(username));
	gtk_widget_set_name(GTK_WIDGET(user_button), "user_button");
	g_signal_connect(user_button, "clicked", (GCallback)on_user_button_clicked, NULL);

	gtk_box_pack_start(box, GTK_WIDGET(user_button), 0, 0, 0);
	gtk_widget_show_all(GTK_WIDGET(user_button));
}


/* updates the userlist (assumes its empty) with the usernames from the input buf */
void userlist_update(const char* original_buf) {
	// original_buf is of format "<username1> <username2> ... up to ... <username10>"

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
		userlist_add_user_button(m_userlist[i], connected_users_box);
	}
}



//
// MESSAGE ENTRY related functions
//

/* sets the remaining character counter based on the length of the current message */
void character_counter_update() {
	GtkLabel *char_counter = GTK_LABEL(gtk_builder_get_object(m_builder, "char_counter_label"));

	char tmp[BUF_SIZE];
	memset(tmp, '\0', BUF_SIZE);
	sprintf(tmp, "%d", MAX_MSG_LEN - gtk_entry_get_text_length(GTK_ENTRY(gtk_builder_get_object(m_builder, "message_entry"))));
	
	gtk_label_set_text(char_counter, tmp);
}


/* clears the current message buffer */
void message_entry_clear() {
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(m_builder, "message_entry")), "");
	character_counter_update();
}


/* replaces the current message with the input buf, and sets it as the active widget */
void message_entry_set(const char* buf) {
	GtkEntry *message_entry = GTK_ENTRY(gtk_builder_get_object(m_builder, "message_entry"));
	gtk_entry_set_text(message_entry, buf);

	gtk_entry_grab_focus_without_selecting(message_entry);  // set the message entry as the active widget
	gtk_editable_set_position(GTK_EDITABLE(message_entry), -1);  // move cursor to end of entry
	
	character_counter_update();
}


/* verifies that the current message is valid and clears it if its not */
void message_entry_verify() {
	GtkEntry *message_entry = GTK_ENTRY(gtk_builder_get_object(m_builder, "message_entry"));
	const gchar *buf = gtk_entry_get_text(message_entry);

	// check if current message starts with reserved character
	if (memcmp(buf, "/", strlen("/")) == 0) {
		message_entry_clear();
		display_dialog("Input Error", "Messages cannot start with the \"/\" character.", GTK_MESSAGE_WARNING);
	}
}


/* verifies that the message is correct, then sends it to the server */
void message_send(const gchar *entry_buf) {
	// note: the buffer passed in is memory-managed by gtk
	// so we must copy it before modifying it
	char buf[BUF_SIZE];
	memset(buf, '\0', BUF_SIZE);
	strcpy(buf, entry_buf);

	if (strlen(entry_buf) < 1) {
		display_dialog("Input Error", "Messages must be at least 1 character long.", GTK_MESSAGE_WARNING);
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
        	display_dialog("@ Error", "You cannot @ yourself.", GTK_MESSAGE_WARNING);
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
        	display_dialog("Delivery Failure", "Intended recipient is not connected to the server.", GTK_MESSAGE_ERROR);
        	return;
        }

        char outgoing[BUF_SIZE];
        memset(outgoing, '\0', BUF_SIZE);
        sprintf(outgoing, "/whisper %s %s", recipient, message);

        if (write(m_socket_fd, outgoing, strlen(outgoing)) < 0) {
        	perror("write() failed during /whisper");
        	display_dialog("Delivery Failure", "Message could not be delivered.", GTK_MESSAGE_ERROR);
        	return;
        }
        else {
        	message_entry_clear();
        }
	}
	else {
		// send as shout
		char outgoing[BUF_SIZE];
		memset(outgoing, '\0', BUF_SIZE);
		sprintf(outgoing, "/shout %s", buf);

		if (write(m_socket_fd, outgoing, strlen(outgoing)) < 0) {
			perror("write() failed during /shout");
			display_dialog("Delivery Failure", "Message could not be delievered.", GTK_MESSAGE_ERROR);
			return;
		}
		else {
			message_entry_clear();
		}
	}
}



//
// SERVER RELATIONSHIP management functions
//

/* closes the server connection and sets the login window as active, then
removes all previously displayed messages (should the user login again) */
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


/* polls the server to see if there are messages to be displayed */
int server_poll() {
	// temporary buffer to hold a potential message
	char buf[BUF_SIZE];
	memset(buf, '\0', BUF_SIZE);
	ssize_t nread;

	if ((nread = read(m_socket_fd, buf, BUF_SIZE)) != -1) {
		if (nread == 0) {
			display_dialog("Connection Lost", "Connection to the server has been lost.", GTK_MESSAGE_ERROR);
			server_logout();
			return 0;  // return 0 to stop polling function
		}
		else {
			// note: "cc" MUST come first because "whispered" is a subset of "whisperedcc"
			if (memcmp(buf, "/whisperedcc", strlen("/whisperedcc")) == 0) {
				message_parse_whispercc(buf + strlen("/whisperedcc") + 1);
			}
			else if (memcmp(buf, "/whispered", strlen("/whispered")) == 0) {
				message_parse_whisper(buf + strlen("/whispered") + 1);
			}
			else if (memcmp(buf, "/shoutedcc", strlen("/shoutedcc")) == 0) {
				message_parse_shoutcc(buf + strlen("/shoutedcc") + 1);
			}
			else if (memcmp(buf, "/shouted", strlen("/shouted")) == 0) {
				message_parse_shout(buf + strlen("/shouted") + 1);
			}
			else if (memcmp(buf, "/userlist", strlen("/userlist")) == 0) {
				userlist_clear();
				userlist_update(buf + strlen("/userlist") + 1);
			}
			// other commands here eventually...
		}
	}
	return 1;  // return 1 to poll again
}


/* verifies that the login information is valid, connects to the server,
and verifies that the user is allowed to join */
void server_login() {
	// get the address and verify its valid
	const gchar *address = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(m_builder, "address_entry")));
	if (!is_valid_address(address)) {
		display_dialog("Invalid Address", "Address must be 1 character or greater.", GTK_MESSAGE_WARNING);
		return;
	}

	// get the port and verify that its valid
	const gchar *port = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(m_builder, "port_entry")));
	if (!is_valid_port(port)) {
		display_dialog("Invalid Port", "Port must be between 1024 and 65535 inclusive.", GTK_MESSAGE_WARNING);
		return;
	}

	// get the username and verify that its valid
	const gchar *username = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(m_builder, "username_entry")));
	if (!is_valid_username(username)) {
		char invalid_username_message[BUF_SIZE];
		memset(invalid_username_message, '\0', BUF_SIZE);
        sprintf(invalid_username_message, "Username must be between 1 and %d characters inclusive, and must not contain spaces.", MAX_NAME_LEN);
		display_dialog("Invalid Username", invalid_username_message, GTK_MESSAGE_WARNING);
		return;
	}
	else {
		m_username = username;
	}

	// at this point the login information is valid, so attempt to connect to the server

	struct addrinfo *address_info;
	if (getaddrinfo(address, port, NULL, &address_info) < 0) {
		perror("getaddrinfo() failed");
		display_dialog("Connection Error", "Unable to resolve address.", GTK_MESSAGE_ERROR);
		return;
	}

	if ((m_socket_fd = socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol)) < 0) {
		perror("socket() failed");
		display_dialog("Connection Error", "Unable to allocate system resources.", GTK_MESSAGE_ERROR);
		return;
	}

	if (connect(m_socket_fd, address_info->ai_addr, address_info->ai_addrlen) < 0) {
		perror("connect() failed");
		display_dialog("Connection Error", "Unable to contact server.", GTK_MESSAGE_ERROR);
		return;
	}

	// at this point we are succesfully connected to the server, so initiate handshake

	char buf[BUF_SIZE];
	memset(buf, '\0', BUF_SIZE);
	sprintf(buf, "/join %s", username);

	if (write(m_socket_fd, buf, strlen(buf)) < 0) {
		perror("write() failed during handshake");
		display_dialog("Connection Error", "Unable to communicate with server.", GTK_MESSAGE_ERROR);
		server_logout();
		return;
	}

	// clear the buf and reuse it to read in the response
	memset(buf, '\0', BUF_SIZE);
	if (read(m_socket_fd, buf, BUF_SIZE) <= 0) {
		perror("read() failed during handshake");
		display_dialog("Connection Error", "Unable to communicate with server.", GTK_MESSAGE_ERROR);
		server_logout();
		return;
	}

	if (memcmp(buf, "/joinresponse ok", strlen("/joinresponse ok")) == 0) {
		// set the socket to be non-blocking so we can poll it
		if (fcntl(m_socket_fd, F_SETFL, O_NONBLOCK) < 0) {
			perror("fcntl() failed during handshake");
			display_dialog("Connection Error", "Unable to allocate system resources.", GTK_MESSAGE_ERROR);
			server_logout();
			return;
		}

		// initialize the chat gui
		message_entry_clear();

		// set the chat pane as the visible window
		gtk_stack_set_visible_child(
    		GTK_STACK(gtk_builder_get_object(m_builder, "main_stack")),
    		GTK_WIDGET(gtk_builder_get_object(m_builder, "chat_pane")));

		// begin server polling
		g_timeout_add(MILLI_SLEEP_DUR, server_poll, NULL);
	}
	else if (memcmp(buf, "/joinresponse username_taken", strlen("/joinresponse username_taken")) == 0) {
		display_dialog("Connection Denied", "Username is already in use.", GTK_MESSAGE_WARNING);
		server_logout();
	}
	else if (memcmp(buf, "/joinresponse server_full", strlen("/joinresponse server_full")) == 0) {
		display_dialog("Connection Denied", "Server is already full.", GTK_MESSAGE_WARNING);
		server_logout();
	}
	else {
		// something has gone seriously wrong...
		display_dialog("Connection Denied", "Something went seriously wrong...", GTK_MESSAGE_ERROR);
		server_logout();
	}
}



//
// GUI callbacks
//

/* logs out of the server and shuts down the gui */
void on_window_destroy(GtkWindow *window) {
	// close the connection to the server, if its open
	if (m_socket_fd != -1) {
		server_logout();
	}

	// unload gtk
    g_object_unref(m_builder);
    gtk_main_quit();
}


/* begin login when the user clicks the connect button */
void on_connect_button_clicked() {
	server_login();
}


/* begin login when the user presses enter in the address entry */
void on_address_entry_activate() {
	server_login();
}


/* begin login when the user presses enter in the port entry */
void on_port_entry_activate() {
    server_login();
}


/* begin login when the user presses enter in the username entry */
void on_username_entry_activate() {
    server_login();
}


/* verifies the message is still valid, then updates the appropriate counters
and labels when the user makes a change to the current message */
void on_message_entry_changed() {
	message_entry_verify();
	character_counter_update();
}


/* sends a message when the user presses enter in the message entry */
void on_message_entry_activate() {
	const gchar *buf = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(m_builder, "message_entry")));
	message_send(buf);
}


/* sends a message when the user clicks the send button */
void on_send_button_clicked() {
	const gchar *buf = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(m_builder, "message_entry")));
	message_send(buf);
}


/* replaces the current message entry with a template to the user whose button was clicked */
void on_user_button_clicked(GtkButton *button) {
	const gchar *username = gtk_button_get_label(button);
	
	// close the users popup, if its still open
	GtkPopover *users_popover = GTK_POPOVER(gtk_builder_get_object(m_builder, "users_popover"));
	gtk_popover_popdown(users_popover);

	// build the buffer to set as the entry
	char tmp[BUF_SIZE];
	memset(tmp, '\0', BUF_SIZE);
	sprintf(tmp, "@%s ", username);
	
	message_entry_clear();
	message_entry_set(tmp);
}



//
// MAIN sets up the gui and starts running it
//
int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);
    
    // load the glade files into the builder
    m_builder = gtk_builder_new();
    gtk_builder_add_from_file(m_builder, "style/client.glade", NULL);
    
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
    gtk_window_present(GTK_WINDOW(gtk_builder_get_object(m_builder, "main_window")));                
    gtk_main();
}
