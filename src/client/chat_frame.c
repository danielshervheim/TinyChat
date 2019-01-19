//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#include "chat_frame.h"

#include "common.h"

struct _ChatFrame {
    GtkBin parent_instance;

    GtkBox *m_messageBox;
    GtkComboBoxText *m_userlist_comboBoxText;
    GtkEntry *m_message_entry;
    GtkLabel *m_character_counter;
};

G_DEFINE_TYPE(ChatFrame, chat_frame, GTK_TYPE_BIN);



/* A utility function, returns a GtkWidget representing the passed in message
parameters, ready to be displayed. */
GtkWidget* message_display_new(const char* title, const char* body,
    GtkAlign halign, const char* title_class, const char* body_class) {

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
    gtk_label_set_line_wrap_mode(body_label, PANGO_WRAP_WORD);

    // make both body and title selectable
    gtk_label_set_selectable(title_label, 1);
    gtk_label_set_selectable(body_label, 1);

    // pack the label(s) into the content_box
    gtk_box_pack_start(container_box, GTK_WIDGET(title_label), 0, 0, 0);
    gtk_box_pack_start(container_box, GTK_WIDGET(body_label), 0, 0, 0);


    // then pack the container into the messages box, and show it
    return GTK_WIDGET(container_box);
}



/* Adds a message received from another user to the ChatFrame. */
void chat_frame_add_message(ChatFrame *self, const char *sender, const char *message) {
    char title[BUFFER_SIZE];
    memset(title, '\0', BUFFER_SIZE);
    sprintf(title, "%s said", sender);

    GtkWidget *messageObj = message_display_new(title, message, GTK_ALIGN_START,
        "broadcast", "broadcast");

    gtk_box_pack_start(self->m_messageBox, messageObj, 0, 0, 0);
    gtk_widget_show_all(messageObj);
}



/* Adds a message you sent to the ChatFrame. */
void add_sent_message(ChatFrame *self, const char *message) {
    GtkWidget *messageObj = message_display_new("you said", message, GTK_ALIGN_END,
    "broadcast", "broadcast");

    gtk_box_pack_start(self->m_messageBox, messageObj, 0, 0, 0);
    gtk_widget_show_all(messageObj);
}



/* Adds a private message received from another user to the ChatFrame. */
void add_sent_private_message(ChatFrame *self, const char *recipient, const char *message) {
    char title[BUFFER_SIZE];
    memset(title, '\0', BUFFER_SIZE);
    sprintf(title, "you whispered to %s", recipient);

    GtkWidget *messageObj = message_display_new(title, message, GTK_ALIGN_END,
        "broadcast", "broadcast");

    gtk_box_pack_start(self->m_messageBox, messageObj, 0, 0, 0);
    gtk_widget_show_all(messageObj);
}



/* Adds a private message you sent to the ChatFrame. */
void chat_frame_add_private_message(ChatFrame *self, const char *sender, const char *message) {
    char title[BUFFER_SIZE];
    memset(title, '\0', BUFFER_SIZE);
    sprintf(title, "%s whispered to you", sender);

    GtkWidget *messageObj = message_display_new(title, message, GTK_ALIGN_START,
        "whisper", "whisper");

    gtk_box_pack_start(self->m_messageBox, messageObj, 0, 0, 0);
    gtk_widget_show_all(messageObj);
}



/* Fires when the user intends to send a message from the ChatFrame. Specifically
when the send button is pressed or the enter key is hit in the message entry. */
void on_send_intent(ChatFrame *self) {
    const gchar *recipient = gtk_combo_box_text_get_active_text(self->m_userlist_comboBoxText);
    const gchar *message = gtk_entry_get_text(self->m_message_entry);

    // Parses the recipient and fires the appropriate signal.
    if (strcmp(recipient, "Everyone") == 0) {
        g_signal_emit_by_name(self, "send-message-intent", message);
        add_sent_message(self, message);
    }
    else {
        g_signal_emit_by_name(self, "send-private-message-intent", recipient, message);
        add_sent_private_message(self, recipient, message);
    }

    // Resets the message entry to blank.
    gtk_entry_set_text(self->m_message_entry, "");
}

/* Fires when the message entry is changed, to verify that users aren't starting
a message with the "/" character, which is reserved for the server to pass commands. */
void on_message_entry_changed(ChatFrame *self) {
    const gchar *text = gtk_entry_get_text(self->m_message_entry);

    /* displays an error dialog and clears the message entry if it starts with
    a reserved character. */
    if (memcmp(text, "/", strlen("/")) == 0) {
        gtk_entry_set_text(self->m_message_entry, "");

        GtkMessageDialog *dia = GTK_MESSAGE_DIALOG(gtk_message_dialog_new(GTK_WINDOW(
            gtk_widget_get_toplevel(GTK_WIDGET(self))),
            GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE, "Invalid Message"));
        gtk_message_dialog_format_secondary_text(dia, "Messages may not start with \'/\'.");

        gtk_dialog_run(GTK_DIALOG(dia));
        gtk_widget_destroy(GTK_WIDGET(dia));
    }

    // updates the character counter based on the message entry length.
    int text_len = strlen(gtk_entry_get_text(self->m_message_entry));
    char tmp[MAX_MESSAGE_LEN];
    memset(tmp, '\0', MAX_MESSAGE_LEN);
    sprintf(tmp, "%d", MAX_MESSAGE_LEN - text_len);
    gtk_label_set_text(self->m_character_counter, tmp);
}



/* Clears the userlist combobox and repopulates it based on the updated
userlist string. */
void chat_frame_update_userlist(ChatFrame *self, const char *userlist) {
    // clear the combo box.
    gtk_combo_box_text_remove_all(self->m_userlist_comboBoxText);

    // append the "Everyone" recipient and set it as default.
    gtk_combo_box_text_append(self->m_userlist_comboBoxText, NULL, "Everyone");
    gtk_combo_box_set_active(GTK_COMBO_BOX(self->m_userlist_comboBoxText), 0);

    // make a copy of the userlist to tokenize.
    char tmp_userlist[BUFFER_SIZE];
    memset(tmp_userlist, '\0', BUFFER_SIZE);
    strcpy(tmp_userlist, userlist);

    // add each token back to the userlist combo box.
    char *token = strtok(tmp_userlist, " ");
    while (token != NULL) {
    	gtk_combo_box_text_append(self->m_userlist_comboBoxText, NULL, token);
    	token = strtok(NULL, " ");
    }
}



/* Returns a new instance of ChatFrame. */
ChatFrame* chat_frame_new () {
    return g_object_new (CHAT_FRAME_TYPE_BIN, NULL);
}



/* Resets the ChatFrame instance. */
void chat_frame_reset(ChatFrame *self) {
    // delete all previously displayed messages.
    gtk_container_foreach(GTK_CONTAINER(self->m_messageBox), (GtkCallback)gtk_widget_destroy, NULL);

    // reset the message entry.
    gtk_entry_set_text(self->m_message_entry, "");
}



/* Initializes the ChatFrame class */
static void chat_frame_class_init (ChatFrameClass *class) {
    /* Fires when the user intends to send a message */
    g_signal_new("send-message-intent", CHAT_FRAME_TYPE_BIN, G_SIGNAL_RUN_FIRST,
        0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_POINTER);

    /* Fires when the user intends to send a message */
    g_signal_new("send-private-message-intent", CHAT_FRAME_TYPE_BIN, G_SIGNAL_RUN_FIRST,
        0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER);
}



/* Initializes the ChatFrame instance. */
static void chat_frame_init (ChatFrame *self) {
    // get a reference to the builder.
    GtkBuilder *builder = gtk_builder_new_from_resource("/tinychat/glade/chat_frame.glade");

    // get references to message box.
    self->m_messageBox = GTK_BOX(gtk_builder_get_object(builder, "messages_box"));

    // get a reference to the commandsBox area.
    GtkBox *commandsBox = GTK_BOX(gtk_builder_get_object(builder, "commands_box"));

    // set the userlist combo box to a new GtkComboBoxText instance, set its properties.
    self->m_userlist_comboBoxText = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
    gtk_widget_set_tooltip_text(GTK_WIDGET(self->m_userlist_comboBoxText), "Message Recipient(s)");

    // pack the userlist box into the commands box and set it to be on the far left.
    gtk_box_pack_start(commandsBox, GTK_WIDGET(self->m_userlist_comboBoxText), 0, 1, 0);
    gtk_box_reorder_child(commandsBox, GTK_WIDGET(self->m_userlist_comboBoxText), 0);

    // get the message entry, set its properties, and handlers.
    self->m_message_entry = GTK_ENTRY(gtk_builder_get_object(builder, "message_entry"));
    gtk_entry_set_max_length(self->m_message_entry, MAX_MESSAGE_LEN);
    g_signal_connect_swapped(self->m_message_entry, "activate",
        (GCallback)on_send_intent, self);
    g_signal_connect_swapped(self->m_message_entry, "changed",
        (GCallback)on_message_entry_changed, self);

    // get the character counter.
    self->m_character_counter = GTK_LABEL(gtk_builder_get_object(builder, "char_counter_label"));

    // get the send button, and set its signals.
    GtkButton *sendButton = GTK_BUTTON(gtk_builder_get_object(builder, "send_button"));
    g_signal_connect_swapped(sendButton, "clicked", (GCallback)on_send_intent, self);

    // get the main container from the builder and add it to the ChatFrame.
    GtkWidget *content = GTK_WIDGET(gtk_builder_get_object(builder, "chat_box"));
    gtk_container_add(GTK_CONTAINER(self), content);
    gtk_widget_show_all(content);

    // and finally, uunref the builder.
	g_object_unref(builder);
}
