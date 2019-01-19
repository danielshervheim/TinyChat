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

};

G_DEFINE_TYPE(ChatFrame, chat_frame, GTK_TYPE_BIN);



void on_send_message_intent(ChatFrame *self) {
    const gchar *text = gtk_entry_get_text(self->m_message_entry);
    g_signal_emit_by_name(self, "send-message-intent", text);
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








/* Returns a new instance of ChatFrame. */
ChatFrame* chat_frame_new () {
    return g_object_new (CHAT_FRAME_TYPE_BIN, NULL);
}

/* Initializes the ChatFrame class */
static void chat_frame_class_init (ChatFrameClass *class) {
    /* Fires when the user intends to send a message */
    g_signal_new("send-message-intent", CHAT_FRAME_TYPE_BIN, G_SIGNAL_RUN_FIRST,
      0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_POINTER);
}

/* Initializes the ChatFrame instance. */
static void chat_frame_init (ChatFrame *self) {
    // get a reference to the builder
    GtkBuilder *builder = gtk_builder_new_from_resource("/tinychat/glade/chat_frame.glade");

    // get the main container from the builder and add it to the window
    GtkWidget *content = GTK_WIDGET(gtk_builder_get_object(builder, "chat_box"));
    gtk_container_add(GTK_CONTAINER(self), content);
    gtk_widget_show_all(content);


    // get references to the message box and set its max length
    self->m_message_entry = GTK_ENTRY(gtk_builder_get_object(builder, "message_entry"));
    gtk_entry_set_max_length(self->m_message_entry, MAX_MESSAGE_LEN);

    // get reference to send button and set its image
    GtkButton *sendButton = GTK_BUTTON(gtk_builder_get_object(builder, "send_button"));
    gtk_button_set_image(sendButton,
        gtk_image_new_from_resource("/tinychat/icons/icons8-sent-16.png"));

    // connect the send-message-intent signal handlers
    g_signal_connect_swapped(self->m_message_entry, "activate", (GCallback)on_send_message_intent, self);
    g_signal_connect_swapped(sendButton, "clicked", (GCallback)on_send_message_intent, self);

    // connect references for input verification
    g_signal_connect_swapped(self->m_message_entry, "changed", (GCallback)on_message_entry_changed, self);

    // get references to and connect the character counter
    self->m_character_counter = GTK_LABEL(gtk_builder_get_object(builder, "char_counter_label"));

    // todo: userlist
}
