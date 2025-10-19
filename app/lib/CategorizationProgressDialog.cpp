#include "CategorizationProgressDialog.hpp"
#include "MainApp.hpp"
#include "Logger.hpp"
#include <gtk/gtk.h>
#include <gtk/gtktypes.h>
#include <gobject/gsignal.h>


CategorizationProgressDialog::CategorizationProgressDialog(GtkWindow* parent, MainApp *main_app, gboolean show_subcategory_col)
    : m_MainApp(main_app), m_Dialog(nullptr), m_TextView(nullptr), m_StopButton(nullptr), buffer(nullptr)
{
    // Create the dialog
    m_Dialog = create_categorization_progress_dialog(parent);

    if (m_Dialog) {
        m_TextView = GTK_WIDGET(g_object_get_data(G_OBJECT(m_Dialog), "progress_text_view"));
        m_StopButton = GTK_WIDGET(g_object_get_data(G_OBJECT(m_Dialog), "stop_analysis_button"));

        if (m_TextView) {
            buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(m_TextView));
        }

        if (!m_StopButton) {
            g_warning("Stop button not found in the dialog!");
            return;
        }

        g_signal_connect(
            m_StopButton,
            "clicked",
            G_CALLBACK(+[](GtkButton*, gpointer user_data) {
                MainApp* app = static_cast<MainApp*>(user_data);
                if (!app) {
                    g_warning("MainApp pointer is null!");
                    return;
                }
                std::string message = "[STOP] Cancelling analysis...";
                app->progress_dialog->append_text(message);
                app->stop_analysis = true;
            }),
            m_MainApp
        );
    }
}


GtkWidget* CategorizationProgressDialog::create_categorization_progress_dialog(GtkWindow *parent)
{
    GtkWidget *dialog, *content_area, *scrolled_window, *button_box, *stop_button;

    dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Analyzing Files");
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);

    if (!dialog) {
        g_critical("Failed to create dialog.");
        return nullptr;
    }

    gtk_window_set_default_size(GTK_WINDOW(dialog), 800, 1000);
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_IN);
    gtk_box_pack_start(GTK_BOX(content_area), scrolled_window, TRUE, TRUE, 10);

    m_TextView = gtk_text_view_new();
    g_object_ref(m_TextView);
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(m_TextView));

    // Configure the text view
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(m_TextView), GTK_WRAP_WORD);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(m_TextView), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(m_TextView), FALSE);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(m_TextView), 16);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(m_TextView), 16);

    gtk_container_add(GTK_CONTAINER(scrolled_window), m_TextView);

    button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);

    gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
    gtk_box_pack_start(GTK_BOX(content_area), button_box, FALSE, TRUE, 10);

    stop_button = gtk_button_new_with_label("Stop Analysis");
    gtk_widget_set_margin_end(stop_button, 10);
    gtk_container_add(GTK_CONTAINER(button_box), stop_button);

    gtk_widget_show_all(dialog);

    g_object_set_data(G_OBJECT(dialog), "progress_text_view", m_TextView);
    g_object_set_data(G_OBJECT(dialog), "stop_analysis_button", stop_button);

    return dialog;
}


void CategorizationProgressDialog::show()
{
    if (m_Dialog) {
        gtk_widget_show(m_Dialog);
    }
}


void CategorizationProgressDialog::append_text(const std::string& text)
{
    if (!m_TextView) {
        if (auto logger = Logger::get_logger("core_logger")) {
            logger->error("CategorizationProgressDialog text view not initialized");
        }
        return;
    }

    GtkTextIter end_iter;
    gtk_text_buffer_get_end_iter(buffer, &end_iter);

    std::string decorated = text;
    if (!decorated.empty() && decorated.back() != '\n') {
        decorated.push_back('\n');
    }

    gtk_text_buffer_insert(buffer, &end_iter, decorated.c_str(), -1);

    gtk_text_buffer_get_end_iter(buffer, &end_iter);
    GtkAdjustment *vadj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(m_TextView));
    if (vadj) {
        gdouble upper = gtk_adjustment_get_upper(vadj);
        gdouble page_size = gtk_adjustment_get_page_size(vadj);
        gdouble value = upper - page_size;
        gtk_adjustment_set_value(vadj, value > 0 ? value : 0);
    } else {
        gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(m_TextView), &end_iter, 0.0, FALSE, 0.0, 1.0);
    }
}


void CategorizationProgressDialog::hide()
{
    if (m_Dialog) {
        gtk_widget_hide(m_Dialog);
    }
}


CategorizationProgressDialog::~CategorizationProgressDialog() {
    if (m_Dialog) {
        gtk_widget_destroy(m_Dialog);
        m_Dialog = nullptr;
    }

    if (m_TextView) {
        g_object_unref(m_TextView);
        m_TextView = nullptr;
    }

    m_StopButton = nullptr;
    buffer = nullptr;
}
