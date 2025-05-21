#include "DialogUtils.hpp"
#include <gtk/gtk.h>


void DialogUtils::show_error_dialog(GtkWindow *parent, const std::string &message) {
    GtkWidget *dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Error");
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *label = gtk_label_new(message.c_str());
    gtk_widget_set_margin_top(label, 10);
    gtk_widget_set_margin_bottom(label, 10);
    gtk_widget_set_margin_start(label, 20);
    gtk_widget_set_margin_end(label, 20);
    gtk_box_pack_start(GTK_BOX(content_area), label, TRUE, TRUE, 0);

    GtkWidget *ok_button = gtk_button_new_with_label("OK");
    gtk_widget_set_hexpand(ok_button, TRUE);
    gtk_widget_set_halign(ok_button, GTK_ALIGN_CENTER);
    g_signal_connect(ok_button, "clicked", G_CALLBACK(+[](GtkWidget *, gpointer dlg) {
        gtk_widget_destroy(GTK_WIDGET(dlg));
    }), dialog);
    gtk_box_pack_start(GTK_BOX(content_area), ok_button, FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);
}