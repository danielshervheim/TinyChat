//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#include "chat_frame.h"

#include "common.h"

struct _ChatFrame {
    GtkBin parent_instance;
    // member instances go here

    GtkEntry *m_message_entry;

    GtkLabel *m_character_counter;

    GtkComboBoxText *m_userlist_comboBoxText;
};

G_DEFINE_TYPE(ChatFrame, chat_frame, GTK_TYPE_BIN);



void on_send_intent(ChatFrame *self) {
    const gchar *recipient = gtk_combo_box_text_get_active_text(self->m_userlist_comboBoxText);
    const gchar *message = gtk_entry_get_text(self->m_message_entry);

    if (strcmp(recipient, "Everyone") == 0) {
        g_signal_emit_by_name(self, "send-message-intent", message);
    }
    else {
        g_signal_emit_by_name(self, "send-private-message-intent", recipient, message);
    }
}


void on_message_entry_changed(ChatFrame *self) {
    const gchar *text = gtk_entry_get_text(self->m_message_entry);
    int text_len = strlen(text);

    if (memcmp(text, "/", strlen("/")) == 0) {
        gtk_entry_set_text(self->m_message_entry, "");
        text_len = 0;

        GtkMessageDialog *dia = GTK_MESSAGE_DIALOG(gtk_message_dialog_new(GTK_WINDOW(
            gtk_widget_get_toplevel(GTK_WIDGET(self))),
            GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE, "Invalid Message"));
        gtk_message_dialog_format_secondary_text(dia, "Messages may not start with \'/\'.");

        gtk_dialog_run(GTK_DIALOG(dia));
        gtk_widget_destroy(GTK_WIDGET(dia));
    }

    char tmp[MAX_MESSAGE_LEN];
    memset(tmp, '\0', MAX_MESSAGE_LEN);
    sprintf(tmp, "%d", MAX_MESSAGE_LEN - text_len);
    gtk_label_set_text(self->m_character_counter, tmp);
}





void chat_frame_userlist_updated(ChatFrame *self, const char *userlist) {
    // clear userlist
    gtk_combo_box_text_remove_all(self->m_userlist_comboBoxText);
    gtk_combo_box_text_append(self->m_userlist_comboBoxText, NULL, "Everyone");
    gtk_combo_box_set_active(GTK_COMBO_BOX(self->m_userlist_comboBoxText), 0);

    char tmp_userlist[BUFFER_SIZE];
    memset(tmp_userlist, '\0', BUFFER_SIZE);
    strcpy(tmp_userlist, userlist);

    char *token = strtok(tmp_userlist, " ");

    while (token != NULL) {
    	gtk_combo_box_text_append(self->m_userlist_comboBoxText, NULL, token);
    	token = strtok(NULL, " ");
    }
}


/*
void chat_frame_set_initial_userlist(ChatFrame *self, const char *userlist) {
    char tmp_userlist[BUFFER_SIZE];
    memset(tmp_userlist, '\0', BUFFER_SIZE);
    strcpy(tmp_userlist, userlist);

    char *token = strtok(tmp_userlist, " ");

    while (token != NULL) {
    	gtk_combo_box_text_append(self->m_userlist_comboBoxText, NULL, token);
    	token = strtok(NULL, " ");
    }
}
*/


void chat_frame_clear_message_entry(ChatFrame *self) {
    gtk_entry_set_text(self->m_message_entry, "");
}



void chat_frame_add_message(ChatFrame *self, const char *sender, const char *message) {
    printf("m: %s %s\n", sender, message);
}

void chat_frame_add_private_message(ChatFrame *self, const char *sender, const char *message) {
    printf("pm: %s %s\n", sender, message);
}






/* Returns a new instance of ChatFrame. */
ChatFrame* chat_frame_new () {
    return g_object_new (CHAT_FRAME_TYPE_BIN, NULL);
}

/* Initializes the ChatFrame class */
static void chat_frame_class_init (ChatFrameClass *class) {
    /* Fires when the user intends to send a message */
    g_signal_new("send-message-intent", CHAT_FRAME_TYPE_BIN, G_SIGNAL_RUN_FIRST,
      0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_POINTER);

    g_signal_new("send-private-message-intent", CHAT_FRAME_TYPE_BIN, G_SIGNAL_RUN_FIRST,
        0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER);
}

/* Initializes the ChatFrame instance. */
static void chat_frame_init (ChatFrame *self) {
    // get a reference to the builder
    GtkBuilder *builder = gtk_builder_new_from_resource("/tinychat/glade/chat_frame.glade");

    // get the main container from the builder and add it to the window
    GtkWidget *content = GTK_WIDGET(gtk_builder_get_object(builder, "chat_box"));
    gtk_container_add(GTK_CONTAINER(self), content);



    // get references to the message box and set its max length
    self->m_message_entry = GTK_ENTRY(gtk_builder_get_object(builder, "message_entry"));
    gtk_entry_set_max_length(self->m_message_entry, MAX_MESSAGE_LEN);

    // get reference to send button and set its image
    GtkButton *sendButton = GTK_BUTTON(gtk_builder_get_object(builder, "send_button"));
    gtk_button_set_image(sendButton,
        gtk_image_new_from_resource("/tinychat/icons/icons8-sent-16.png"));

    // connect the send-message-intent signal handlers
    g_signal_connect_swapped(self->m_message_entry, "activate", (GCallback)on_send_intent, self);
    g_signal_connect_swapped(sendButton, "clicked", (GCallback)on_send_intent, self);

    // connect references for input verification
    g_signal_connect_swapped(self->m_message_entry, "changed", (GCallback)on_message_entry_changed, self);

    // get references to and connect the character counter
    self->m_character_counter = GTK_LABEL(gtk_builder_get_object(builder, "char_counter_label"));

    // todo: userlist
    self->m_userlist_comboBoxText = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
    gtk_widget_set_tooltip_text(GTK_WIDGET(self->m_userlist_comboBoxText), "Message Recipient(s)");
    gtk_box_pack_start(GTK_BOX(gtk_builder_get_object(builder, "commands_box")), GTK_WIDGET(self->m_userlist_comboBoxText), 0, 1, 0);
    gtk_box_reorder_child(GTK_BOX(gtk_builder_get_object(builder, "commands_box")), GTK_WIDGET(self->m_userlist_comboBoxText), 0);



    gtk_widget_show_all(content);
}
