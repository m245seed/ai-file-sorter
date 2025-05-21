#include "DialogUtils.hpp"
#include "ErrorMessages.hpp"
#include "LLMDownloader.hpp"
#include "LLMSelectionDialog.hpp"
#include "Utils.hpp"
#include <iostream>


void LLMSelectionDialog::on_llm_radio_toggled(GtkWidget *widget, gpointer data)
{
    LLMSelectionDialog *dlg = static_cast<LLMSelectionDialog*>(data);
    bool is_local_selected = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dlg->local_llm_button));

    gtk_widget_set_sensitive(dlg->download_button, is_local_selected);

    dlg->ok_btn = gtk_dialog_get_widget_for_response(GTK_DIALOG(dlg->dialog), GTK_RESPONSE_OK);

    LLMDownloader::DownloadStatus status = dlg->downloader->get_download_status();

    bool enable_ok = !is_local_selected || status == LLMDownloader::DownloadStatus::Complete;
    gtk_widget_set_sensitive(dlg->ok_btn, enable_ok);

    if (!is_local_selected) {
        gtk_widget_hide(dlg->file_info_box);
        gtk_widget_hide(dlg->details_frame);
        gtk_widget_hide(dlg->progress_bar);
        gtk_widget_hide(dlg->download_button);
        GtkWindow *win = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(dlg->dialog)));
        gtk_window_resize(win, 1, 1);
    } else {
        gtk_widget_show(dlg->file_info_box);
        gtk_widget_show(dlg->details_frame);
        gtk_widget_show(dlg->progress_bar);
        gtk_widget_show(dlg->download_button);
    }
}


LLMSelectionDialog::LLMSelectionDialog()
{
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
    remote_llm_button = gtk_radio_button_new_with_label(NULL, "Remote LLM (ChatGPT 4o-mini)");
    local_llm_button = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(remote_llm_button), "Local LLM (LLaMa 3.2 3B)");
    gtk_box_pack_start(GTK_BOX(radio_box), remote_llm_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(radio_box), local_llm_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(main_box), radio_box, FALSE, FALSE, 5);

    // Download button
    downloader = std::make_unique<LLMDownloader>();

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
    
    download_button = gtk_button_new_with_label("Download");
    bool download_complete = false;

    switch (downloader->get_download_status()) {
        case LLMDownloader::DownloadStatus::Complete:
            download_complete = true;
            break;
        case LLMDownloader::DownloadStatus::InProgress:
            download_button = gtk_button_new_with_label("Resume download");
            gtk_widget_set_sensitive(download_button, FALSE);
            break;
        case LLMDownloader::DownloadStatus::NotStarted:
            download_button = gtk_button_new_with_label("Download");
            gtk_widget_set_sensitive(download_button, FALSE);
            break;
    }

    gtk_widget_set_sensitive(download_button, FALSE);
    gtk_box_pack_start(GTK_BOX(button_box), download_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(main_box), button_box, FALSE, FALSE, 5);

    // File information
    // A separate box for file info
    file_info_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(file_info_box), 10);

    // Labels
    remote_url_label = gtk_label_new(nullptr);
    local_path_label = gtk_label_new(nullptr);
    file_size_label = gtk_label_new(nullptr);

    // Format and set text
    gtk_label_set_text(GTK_LABEL(remote_url_label),
        ("<b>Remote URL:</b> <span foreground='blue'>" + downloader->url + "</span>").c_str());
    gtk_label_set_use_markup(GTK_LABEL(remote_url_label), TRUE);
    gtk_label_set_selectable(GTK_LABEL(remote_url_label), TRUE);
    gtk_label_set_xalign(GTK_LABEL(remote_url_label), 0.0f);  // Left align

    gtk_label_set_text(GTK_LABEL(local_path_label),
        ("<b>Local path:</b> <span foreground='darkgreen'>" + downloader->download_destination + "</span>").c_str());
    gtk_label_set_use_markup(GTK_LABEL(local_path_label), TRUE);
    gtk_label_set_selectable(GTK_LABEL(local_path_label), TRUE);
    gtk_label_set_xalign(GTK_LABEL(local_path_label), 0.0f);

    if (Utils::is_network_available()) {
        downloader->init_if_needed();
    }

    update_file_size_label();

    // Add labels to info box
    gtk_box_pack_start(GTK_BOX(file_info_box), remote_url_label, FALSE, FALSE, 3);
    gtk_box_pack_start(GTK_BOX(file_info_box), local_path_label, FALSE, FALSE, 3);
    gtk_box_pack_start(GTK_BOX(file_info_box), file_size_label, FALSE, FALSE, 3);

    // Optional: wrap in a frame to visually group
    details_frame = gtk_frame_new("Download details");
    gtk_container_add(GTK_CONTAINER(details_frame), file_info_box);

    // Add the frame to your main box
    gtk_box_pack_start(GTK_BOX(main_box), details_frame, FALSE, FALSE, 10);

    // Progress bar (wider + css fallback)
    GtkWidget *progress_container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(progress_container, GTK_ALIGN_CENTER);
    gtk_widget_set_vexpand(progress_container, TRUE);
    progress_bar = gtk_progress_bar_new();
    gtk_widget_set_size_request(progress_bar, 350, -1);
    gtk_widget_set_name(progress_bar, "custom-progressbar");
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progress_bar), TRUE);

    // CSS for more consistent height
    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_provider,
        "#custom-progressbar progress, #custom-progressbar trough { min-height: 30px; }", -1, NULL);
    GtkStyleContext *context = gtk_widget_get_style_context(progress_bar);
    gtk_style_context_add_provider(context,
            GTK_STYLE_PROVIDER(css_provider),
            GTK_STYLE_PROVIDER_PRIORITY_USER);

    g_object_unref(css_provider);

    gtk_widget_set_hexpand(progress_bar, TRUE);
    gtk_widget_set_vexpand(progress_bar, FALSE);

    gtk_box_pack_start(GTK_BOX(progress_container), progress_bar, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(main_box), progress_container, FALSE, FALSE, 5);

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
    
    if (download_complete) {
        gtk_widget_hide(download_button);
        gtk_widget_hide(progress_bar);
    }

    on_llm_radio_toggled(GTK_WIDGET(local_llm_button), this);
}


void LLMSelectionDialog::update_file_size_label() {
    std::string size_str = Utils::format_size(downloader->real_content_length);
    gtk_label_set_text(GTK_LABEL(file_size_label),
        ("<b>File size:</b> " + size_str).c_str());
    gtk_label_set_use_markup(GTK_LABEL(file_size_label), TRUE);
    gtk_label_set_xalign(GTK_LABEL(file_size_label), 0.0f);
}


int LLMSelectionDialog::run()
{
    return gtk_dialog_run(GTK_DIALOG(dialog));
}


GtkWidget* LLMSelectionDialog::get_widget()
{
    return dialog;
}


void LLMSelectionDialog::on_selection_changed(GtkWidget *widget, gpointer data)
{
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(local_llm_button))) {
        gtk_widget_set_sensitive(download_button, TRUE);
    } else {
        gtk_widget_set_sensitive(download_button, FALSE);
    }
}


void LLMSelectionDialog::on_download_button_clicked(GtkWidget *widget, gpointer data)
{
    if (!Utils::is_network_available()) {
        DialogUtils::show_error_dialog(GTK_WINDOW(this->dialog), ERR_NO_INTERNET_CONNECTION);
        return;
    }

    if (!downloader->is_inited()) {
        downloader->init_if_needed();
        update_file_size_label();
    }    

    GtkWidget *cancel_btn = gtk_dialog_get_widget_for_response(GTK_DIALOG(this->dialog), GTK_RESPONSE_CANCEL);
    if (is_downloading) {
        downloader->cancel_download();
        is_downloading = false;
        gtk_button_set_label(GTK_BUTTON(download_button), "Resume download");
        gtk_widget_set_sensitive(cancel_btn, TRUE);
        gtk_window_set_deletable(GTK_WINDOW(dialog), TRUE);
        return;
    }

    // Start or resume download
    gtk_widget_set_sensitive(cancel_btn, FALSE);
    gtk_window_set_deletable(GTK_WINDOW(dialog), FALSE);
    is_downloading = true;
    gtk_widget_set_sensitive(download_button, TRUE);
    gtk_button_set_label(GTK_BUTTON(download_button), "Pause download");

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.0);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), "Starting download...");

    downloader->start_download(
        [this](double progress) {
            g_idle_add([](gpointer user_data) -> gboolean {
                auto *wrapper = static_cast<std::pair<LLMSelectionDialog*, double> *>(user_data);
                wrapper->first->update_progress(wrapper->second);
                delete wrapper;
                return FALSE;
            }, new std::pair<LLMSelectionDialog*, double>(this, progress));            
        },
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
        },
        [this](const std::string& status) {
            g_idle_add([](gpointer user_data) -> gboolean {
                auto *wrapper = static_cast<std::pair<LLMSelectionDialog*, std::string> *>(user_data);
                wrapper->first->update_progress_text(wrapper->second);
                delete wrapper;
                return FALSE;
            }, new std::pair<LLMSelectionDialog*, std::string>(this, status));
        }
    );
}


void LLMSelectionDialog::update_progress(double fraction)
{
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), fraction);
}



void LLMSelectionDialog::on_download_complete()
{
    g_idle_add_full(G_PRIORITY_DEFAULT, [](gpointer data) -> gboolean {
        auto* self = static_cast<LLMSelectionDialog*>(data);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(self->progress_bar), "Download complete!");
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(self->progress_bar), 1.0);
        gtk_widget_hide(self->download_button);
        gtk_widget_hide(self->progress_bar);
    
        GtkWidget *ok_btn = gtk_dialog_get_widget_for_response(GTK_DIALOG(self->dialog), GTK_RESPONSE_OK);
        gtk_widget_set_sensitive(ok_btn, TRUE);
        return G_SOURCE_REMOVE;
    }, this, nullptr);
}


void LLMSelectionDialog::update_progress_text(const std::string& text)
{
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), text.c_str());
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