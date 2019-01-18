//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#include "client_window.h"

#include "chat_frame.h"
#include "login_frame.h"

struct _ClientWindow {
    GtkWindow parent_instance;

    // member instances go here

};

G_DEFINE_TYPE(ClientWindow, client_window, GTK_TYPE_WINDOW);







void on_testSignal(ChatFrame *frame, int x, int y) {
	printf("(%d, %d)\n", x, y);
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


	// add stack and in stack spawn two instances of loginPane and chatPane
	// connect the needed signals from each
	// loginPane and chatPane both inherit from GtkBin -- only one item


	// this window has a parameter "Client m_client --- which is the main interface between the client connection and message passing code, etc"

	// this class overrides the following signals from the loginframe:
	// login-attempt()
	
	LoginFrame *lf = login_frame_new();
	ChatFrame *cf = chat_frame_new();

	g_signal_connect(cf, "testsignal", (GCallback)on_testSignal, NULL);

	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
	gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(lf), 1, 1, 0);
	gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(cf), 1, 1, 0);

	gtk_container_add(GTK_CONTAINER(self), box);
	gtk_widget_show_all(box);

}