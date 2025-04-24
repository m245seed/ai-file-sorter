#include "LLMDownloader.hpp"
#include "LLMSelectionDialog.hpp"


void LLMSelectionDialog::on_llm_radio_toggled(GtkWidget *widget, gpointer data) {
    LLMSelectionDialog *dlg = static_cast<LLMSelectionDialog*>(data);
    bool is_local = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dlg->local_llm_button));

    gtk_widget_set_sensitive(dlg->download_button, is_local);
}


LLMSelectionDialog::LLMSelectionDialog() {
    dialog = gtk_dialog_new_with_buttons("Choose LLM Mode",
                                         NULL,
                                         GTK_DIALOG_MODAL,
                                         "Cancel", GTK_RESPONSE_CANCEL,
                                         "OK", GTK_RESPONSE_OK,
                                         NULL);
    
    gtk_widget_set_size_request(dialog, 500, 400);
    main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_halign(main_box, GTK_ALIGN_CENTER);
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), main_box);

    // Title
    title_label = gtk_label_new("Select LLM Mode:");
    gtk_widget_set_halign(title_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(main_box), title_label, FALSE, FALSE, 5);

    // Radio buttons
    GtkWidget *radio_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_halign(radio_box, GTK_ALIGN_CENTER);
    remote_llm_button = gtk_radio_button_new_with_label(NULL, "Remote LLM");
    local_llm_button = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(remote_llm_button), "Local LLM");
    gtk_box_pack_start(GTK_BOX(radio_box), remote_llm_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(radio_box), local_llm_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(main_box), radio_box, FALSE, FALSE, 5);

    // Download button
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
    download_button = gtk_button_new_with_label("Download");
    gtk_widget_set_sensitive(download_button, FALSE);
    gtk_box_pack_start(GTK_BOX(button_box), download_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(main_box), button_box, FALSE, FALSE, 5);

    // Progress bar (wider + css fallback)
    GtkWidget *progress_container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(progress_container, GTK_ALIGN_CENTER);
    gtk_widget_set_vexpand(progress_container, TRUE);
    progress_bar = gtk_progress_bar_new();
    gtk_widget_set_size_request(progress_bar, 350, -1);
    gtk_widget_set_name(progress_bar, "custom-progressbar");

    // CSS for more consistent height
    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_provider,
        "#custom-progressbar progress, #custom-progressbar trough { min-height: 30px; }", -1, NULL);
    GtkStyleContext *context = gtk_widget_get_style_context(progress_bar);
    gtk_style_context_add_provider(context,
            GTK_STYLE_PROVIDER(css_provider),
            GTK_STYLE_PROVIDER_PRIORITY_USER);

    gtk_widget_set_hexpand(progress_bar, TRUE);
    gtk_widget_set_vexpand(progress_bar, FALSE);

    gtk_box_pack_start(GTK_BOX(progress_container), progress_bar, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(main_box), progress_container, FALSE, FALSE, 5);

    // Get OK button and disable initially
    GtkWidget *ok_button = gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    gtk_widget_set_sensitive(ok_button, FALSE);

    // Signals
    g_signal_connect(remote_llm_button, "toggled", G_CALLBACK(on_llm_radio_toggled), this);
    g_signal_connect(local_llm_button, "toggled", G_CALLBACK(on_llm_radio_toggled), this);

    g_signal_connect(download_button, "clicked", G_CALLBACK(+[](GtkWidget *widget, gpointer data) {
        LLMSelectionDialog *dlg = static_cast<LLMSelectionDialog*>(data);
        dlg->on_download_button_clicked(widget, data);
    }), this);

    GtkWidget *ok_btn = gtk_dialog_get_widget_for_response(GTK_DIALOG(this->dialog), GTK_RESPONSE_OK);
    gtk_widget_set_sensitive(ok_btn, TRUE);
    gtk_widget_show_all(dialog);
}


int LLMSelectionDialog::run() {
    return gtk_dialog_run(GTK_DIALOG(dialog));
}

GtkWidget* LLMSelectionDialog::get_widget() {
    return dialog;
}


void LLMSelectionDialog::on_selection_changed(GtkWidget *widget, gpointer data) {
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(local_llm_button))) {
        gtk_widget_set_sensitive(download_button, TRUE);
        gtk_widget_set_sensitive(continue_button, FALSE);
    } else {
        gtk_widget_set_sensitive(download_button, FALSE);
        gtk_widget_set_sensitive(continue_button, TRUE);
    }
}


void LLMSelectionDialog::on_download_button_clicked(GtkWidget *widget, gpointer data) {
    if (is_downloading) return;

    is_downloading = true;
    gtk_widget_set_sensitive(download_button, FALSE);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.0);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), "Starting download...");

    downloader = std::make_unique<LLMDownloader>();

    downloader->start_download(
        // progress callback
        [this](double progress) {
            g_idle_add([](gpointer user_data) -> gboolean {
                auto *dlg = static_cast<LLMSelectionDialog *>(user_data);
                dlg->update_progress();
                return FALSE;
            }, this);
        },
        // on complete
        [this]() {
            {
                std::lock_guard<std::mutex> lock(download_mutex);
                is_downloading = false;
            }
            g_idle_add([](gpointer user_data) -> gboolean {
                auto *dlg = static_cast<LLMSelectionDialog *>(user_data);
                dlg->on_download_complete();
                return FALSE;
            }, this);
        }
    );
}


void LLMSelectionDialog::update_progress() {
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 1.0);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), "Downloading... Done!");
}


void LLMSelectionDialog::on_download_complete() {
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), "Download complete!");
    gtk_widget_set_sensitive(continue_button, TRUE);
}


LLMSelectionDialog::~LLMSelectionDialog() {
    {
        std::lock_guard<std::mutex> lock(download_mutex);
        is_downloading = false;
    }
    if (download_thread.joinable()) {
        download_thread.join();
    }
    gtk_widget_destroy(dialog);
}