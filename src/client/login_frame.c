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

};

G_DEFINE_TYPE(LoginFrame, login_frame, GTK_TYPE_BIN);

/* Returns a new instance of LoginFrame. */
LoginFrame* login_frame_new () {
    return g_object_new (LOGIN_FRAME_TYPE_BIN, NULL);
}

/* Initializes the LoginFrame class */
static void login_frame_class_init (LoginFrameClass *class) {
    // GObject property stuff would go here...
}

/* Initializes the LoginFrame instance. */
static void login_frame_init (LoginFrame *self) {
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

	// initialization goes here
	GtkWidget *label = gtk_label_new("Login");
	gtk_box_pack_start(GTK_BOX(box), label, 1, 1, 0);


	GtkWidget *button = gtk_button_new();
	gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_resource("/tinychat/icons/icons8-enter-16.png"));
	gtk_box_pack_start(GTK_BOX(box), button, 1, 1, 0);

	gtk_container_add(GTK_CONTAINER(self), box);
	gtk_widget_show_all(box);
}