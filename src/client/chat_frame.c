//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#include "chat_frame.h"

struct _ChatFrame {
    GtkBin parent_instance;
    // member instances go here

};

G_DEFINE_TYPE(ChatFrame, chat_frame, GTK_TYPE_BIN);







// tmp

void on_button_press(ChatFrame *self) {
	printf("button pressed\n");
	g_signal_emit_by_name(self, "testsignal", 10, 24);
}








/* Returns a new instance of ChatFrame. */
ChatFrame* chat_frame_new () {
    return g_object_new (CHAT_FRAME_TYPE_BIN, NULL);
}

/* Initializes the ChatFrame class */
static void chat_frame_class_init (ChatFrameClass *class) {
    // GObject property stuff would go here...

	g_signal_new("testsignal", CHAT_FRAME_TYPE_BIN, G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_INT);

   /* guint
g_signal_new (const gchar *signal_name,
              GType itype,
              GSignalFlags signal_flags,
              guint class_offset,
              GSignalAccumulator accumulator,
              gpointer accu_data,
              GSignalCMarshaller c_marshaller,
              GType return_type,
              guint n_params,
              ...);
              */
}

/* Initializes the ChatFrame instance. */
static void chat_frame_init (ChatFrame *self) {
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

	// initialization goes here
	GtkWidget *label = gtk_label_new("Chatting");
	gtk_box_pack_start(GTK_BOX(box), label, 1, 1, 0);


	GtkWidget *button = gtk_button_new();
	gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_resource("/tinychat/icons/icons8-sent-16.png"));
	gtk_box_pack_start(GTK_BOX(box), button, 1, 1, 0);

	g_signal_connect_swapped(button, "clicked", (GCallback)on_button_press, self);

	gtk_container_add(GTK_CONTAINER(self), box);
	gtk_widget_show_all(box);
}
