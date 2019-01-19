//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#include "tinychat_app.h"

#include <gtk/gtk.h>

int main(int argc, char *argv[]) {
    return g_application_run(G_APPLICATION(tinychat_app_new()), argc, argv);
}
