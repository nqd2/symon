#include "symon_app.h"

#include "symon_constants.h"

#include <gtk/gtk.h>

static void symon_activate(GtkApplication *application, gpointer user_data)
{
    GtkWindow *window = gtk_application_get_active_window(application);

    (void)user_data;

    if (window == NULL) {
        g_autoptr(GtkBuilder) builder =
            gtk_builder_new_from_resource("/io/github/symon/SyMon/ui/main-window.ui");

        window = GTK_WINDOW(gtk_builder_get_object(builder, "main_window"));
        gtk_window_set_application(window, application);
    }

    gtk_window_present(window);
}

int symon_app_run(int argc, char **argv)
{
    g_autoptr(GtkApplication) application =
        gtk_application_new(SYMON_APPLICATION_ID, G_APPLICATION_DEFAULT_FLAGS);

    g_set_application_name(SYMON_APPLICATION_NAME);
    g_signal_connect(application, "activate", G_CALLBACK(symon_activate), NULL);

    return g_application_run(G_APPLICATION(application), argc, argv);
}
