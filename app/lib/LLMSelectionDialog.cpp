#include "DialogUtils.hpp"
#include "ErrorMessages.hpp"
#include "LLMDownloader.hpp"
#include "LLMSelectionDialog.hpp"
#include "Settings.hpp"
#include "Utils.hpp"
#include <iostream>


void LLMSelectionDialog::on_llm_radio_toggled(GtkWidget *widget, gpointer data)
{
    LLMSelectionDialog *dlg = static_cast<LLMSelectionDialog*>(data);

    // Hvilken modell har brukeren valgt?
    bool is_local_3b = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dlg->local_llm_3b_button));
    bool is_local_7b = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dlg->local_llm_7b_button));
    bool is_local_selected = is_local_3b || is_local_7b;

    dlg->selected_choice = is_local_3b ? LLMChoice::Local_3b :
                            is_local_7b ? LLMChoice::Local_7b :
                                          LLMChoice::Remote;

    dlg->ok_btn = gtk_dialog_get_widget_for_response(GTK_DIALOG(dlg->dialog), GTK_RESPONSE_OK);
    gtk_widget_set_sensitive(dlg->download_button, is_local_selected);

    // Helper for hiding a group of widgets
    auto hide_widgets = [&](std::initializer_list<GtkWidget*> widgets) {
        for (auto w : widgets) gtk_widget_hide(w);
    };
    auto show_widgets = [&](std::initializer_list<GtkWidget*> widgets) {
        for (auto w : widgets) gtk_widget_show(w);
    };

    // Si l'utilisateur choisit un modèle local...
    if (!is_local_selected) {
        hide_widgets({ dlg->file_info_box, dlg->details_frame, dlg->progress_bar, dlg->download_button });
        gtk_widget_set_sensitive(dlg->ok_btn, TRUE);
        GtkWindow *win = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(dlg->dialog)));
        gtk_window_resize(win, 1, 1); // Shrink window
        return;
    }

    const char* env_var = (dlg->selected_choice == LLMChoice::Local_3b)
                          ? "LOCAL_LLM_3B_DOWNLOAD_URL"
                          : "LOCAL_LLM_7B_DOWNLOAD_URL";

    const char* env_url = std::getenv(env_var);
    if (!env_url) {
        gtk_label_set_text(GTK_LABEL(dlg->remote_url_label), "Missing download URL environment variable.");
        gtk_widget_set_sensitive(dlg->ok_btn, FALSE);
        return;
    }

    LLMDownloader::DownloadStatus status = LLMDownloader::DownloadStatus::NotStarted;

    try {
        dlg->downloader->set_download_url(env_url);
        if (Utils::is_network_available()) {
            dlg->downloader->init_if_needed();
        }

        // Update download info UI
        std::string url_markup = "<b>Remote URL:</b> <span foreground='blue'>" + dlg->downloader->get_download_url() + "</span>";
        gtk_label_set_markup(GTK_LABEL(dlg->remote_url_label), url_markup.c_str());

        std::string path_markup = "<b>Local path:</b> <span foreground='darkgreen'>" + dlg->downloader->get_download_destination() + "</span>";
        gtk_label_set_markup(GTK_LABEL(dlg->local_path_label), path_markup.c_str());

        dlg->update_file_size_label();

        status = dlg->downloader->get_download_status();

        // Determine button label
        const char* label_text = (status == LLMDownloader::DownloadStatus::Complete)     ? "Re-download" :
                                 (status == LLMDownloader::DownloadStatus::InProgress)   ? "Resume download" :
                                                                                           "Download";
        gtk_button_set_label(GTK_BUTTON(dlg->download_button), label_text);

    } catch (const std::exception& ex) {
        gtk_label_set_text(GTK_LABEL(dlg->remote_url_label), ex.what());
        gtk_widget_set_sensitive(dlg->ok_btn, FALSE);
        return;
    }

    // Enable OK only if download is already complete
    gtk_widget_set_sensitive(dlg->ok_btn, status == LLMDownloader::DownloadStatus::Complete);

    show_widgets({ dlg->file_info_box, dlg->details_frame });

    if (status == LLMDownloader::DownloadStatus::Complete) {
        hide_widgets({ dlg->progress_bar, dlg->download_button });
    } else {
        show_widgets({ dlg->progress_bar, dlg->download_button });
    }

    dlg->init_progress_bar();
}


LLMSelectionDialog::LLMSelectionDialog(Settings& settings) :
    settings(settings), selected_choice(LLMChoice::Unset)
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

    // Descriptions
    GtkWidget *radio_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget* label_remote_desc = gtk_label_new(
    "Fast and accurate, but requires internet connection.");
    gtk_label_set_xalign(GTK_LABEL(label_remote_desc), 0.0f);
    gtk_widget_set_margin_bottom(label_remote_desc, 10);
    GtkWidget* label_3b_desc = gtk_label_new(
    "Less precise, but works quickly even on CPUs. Good for lightweight local use.");
    gtk_label_set_xalign(GTK_LABEL(label_3b_desc), 0.0f);
    gtk_widget_set_margin_bottom(label_3b_desc, 10);

    GtkWidget* label_7b_desc = gtk_label_new(
        "Quite precise. Slower on CPU, but performs much better with GPU acceleration.\n"
        "Supports: Nvidia (CUDA), OpenCL, Apple (Metal), CPU.");
    gtk_label_set_xalign(GTK_LABEL(label_7b_desc), 0.0f);
    gtk_widget_set_margin_bottom(label_7b_desc, 10);

    // Radio buttons
    gtk_widget_set_halign(radio_box, GTK_ALIGN_CENTER);
    GtkWidget* remote_llm_label = gtk_label_new(nullptr);

    gtk_label_set_markup(GTK_LABEL(remote_llm_label), "<b>Remote LLM (ChatGPT 4o-mini)</b>");
    remote_llm_button = gtk_radio_button_new(NULL);
    gtk_container_add(GTK_CONTAINER(remote_llm_button), remote_llm_label);

    GtkWidget* local_llm_3b_label = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(local_llm_3b_label), "<b>Local LLM (LLaMa 3b v3.2 Instruct Q8)</b>");
    local_llm_3b_button = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(remote_llm_button));
    gtk_container_add(GTK_CONTAINER(local_llm_3b_button), local_llm_3b_label);

    GtkWidget* local_llm_7b_label = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(local_llm_7b_label), "<b>Local LLM (Mistral 7b Instruct v0.2 Q5)</b>");
    local_llm_7b_button = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(remote_llm_button));
    gtk_container_add(GTK_CONTAINER(local_llm_7b_button), local_llm_7b_label);

    gtk_box_pack_start(GTK_BOX(radio_box), local_llm_7b_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(radio_box), label_7b_desc, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(radio_box), local_llm_3b_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(radio_box), label_3b_desc, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_box), radio_box, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(radio_box), remote_llm_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(radio_box), label_remote_desc, FALSE, FALSE, 0);

    // Downloader
    const char* default_url = std::getenv("LOCAL_LLM_3B_DOWNLOAD_URL");

    if (!default_url) {
        throw std::runtime_error("Required environment variable for selected model is not set");
    }

    downloader = std::make_unique<LLMDownloader>(default_url);

    // Download button
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);

    const char* label_text = "Download";
    bool download_complete = false;

    switch (downloader->get_download_status()) {
        case LLMDownloader::DownloadStatus::Complete:
            download_complete = true;
            break;
        case LLMDownloader::DownloadStatus::InProgress:
            label_text = "Resume download";
            break;
        case LLMDownloader::DownloadStatus::NotStarted:
            label_text = "Download";
            break;
    }

    download_button = gtk_button_new_with_label(label_text);
    gtk_widget_set_sensitive(download_button, FALSE);
    gtk_box_pack_start(GTK_BOX(button_box), download_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(main_box), button_box, FALSE, FALSE, 5);

    // File info
    file_info_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(file_info_box), 10);

    remote_url_label = gtk_label_new(nullptr);
    local_path_label = gtk_label_new(nullptr);
    file_size_label = gtk_label_new(nullptr);

    gtk_label_set_markup(GTK_LABEL(remote_url_label),
        ("<b>Remote URL:</b> <span foreground='blue'>" + downloader->get_download_url() + "</span>").c_str());

    gtk_label_set_selectable(GTK_LABEL(remote_url_label), TRUE);
    gtk_label_set_xalign(GTK_LABEL(remote_url_label), 0.0f);

    gtk_label_set_markup(GTK_LABEL(local_path_label),
        ("<b>Local path:</b> <span foreground='darkgreen'>" + downloader->get_download_destination() + "</span>").c_str());
    gtk_label_set_selectable(GTK_LABEL(local_path_label), TRUE);
    gtk_label_set_xalign(GTK_LABEL(local_path_label), 0.0f);

    update_file_size_label();

    gtk_box_pack_start(GTK_BOX(file_info_box), remote_url_label, FALSE, FALSE, 3);
    gtk_box_pack_start(GTK_BOX(file_info_box), local_path_label, FALSE, FALSE, 3);
    gtk_box_pack_start(GTK_BOX(file_info_box), file_size_label, FALSE, FALSE, 3);

    details_frame = gtk_frame_new("Download details");
    gtk_container_add(GTK_CONTAINER(details_frame), file_info_box);
    gtk_box_pack_start(GTK_BOX(main_box), details_frame, FALSE, FALSE, 10);

    // Progress bar
    GtkWidget *progress_container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(progress_container, GTK_ALIGN_CENTER);
    gtk_widget_set_vexpand(progress_container, TRUE);
    progress_bar = gtk_progress_bar_new();
    gtk_widget_set_size_request(progress_bar, 350, -1);
    gtk_widget_set_name(progress_bar, "custom-progressbar");
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progress_bar), TRUE);

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

    // Signal connections
    g_signal_connect(remote_llm_button, "toggled", G_CALLBACK(on_llm_radio_toggled), this);
    g_signal_connect(local_llm_3b_button, "toggled", G_CALLBACK(on_llm_radio_toggled), this);
    g_signal_connect(local_llm_7b_button, "toggled", G_CALLBACK(on_llm_radio_toggled), this);

    g_signal_connect(download_button, "clicked", G_CALLBACK(+[](GtkWidget *widget, gpointer data) {
        LLMSelectionDialog *dlg = static_cast<LLMSelectionDialog*>(data);
        dlg->on_download_button_clicked(widget, data);
    }), this);

    GtkWidget *ok_btn = gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    gtk_widget_set_sensitive(ok_btn, TRUE);

    gtk_widget_hide(download_button);
    gtk_widget_hide(details_frame);
    gtk_widget_hide(progress_bar);

    gtk_widget_show_all(dialog);

    if (download_complete) {
        gtk_widget_hide(download_button);
        gtk_widget_hide(progress_bar);
    }

    LLMChoice current_choice = settings.get_llm_choice();
    switch (current_choice) {
        case LLMChoice::Remote:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remote_llm_button), TRUE);
            selected_choice = LLMChoice::Remote;
            break;
        case LLMChoice::Local_3b:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(local_llm_3b_button), TRUE);
            selected_choice = LLMChoice::Local_3b;
            break;
        case LLMChoice::Local_7b:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(local_llm_7b_button), TRUE);
            selected_choice = LLMChoice::Local_7b;
            break;
        default:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(local_llm_7b_button), TRUE);
            selected_choice = LLMChoice::Local_7b;
            break;
    }

    init_progress_bar();

    g_idle_add([](gpointer data) -> gboolean {
        LLMSelectionDialog* dlg = static_cast<LLMSelectionDialog*>(data);

        GtkToggleButton* btn = nullptr;
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dlg->remote_llm_button))) {
            btn = GTK_TOGGLE_BUTTON(dlg->remote_llm_button);
        } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dlg->local_llm_3b_button))) {
            btn = GTK_TOGGLE_BUTTON(dlg->local_llm_3b_button);
        } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dlg->local_llm_7b_button))) {
            btn = GTK_TOGGLE_BUTTON(dlg->local_llm_7b_button);
        }

        if (btn) {
            on_llm_radio_toggled(GTK_WIDGET(btn), dlg);
        }

        return G_SOURCE_REMOVE;
    }, this);
}


void LLMSelectionDialog::init_progress_bar()
{
    if (!downloader->is_inited()) {
        downloader->init_if_needed();
    }

    FILE* fp = fopen(downloader->get_download_destination().c_str(), "rb");
    if (!fp) return;

    fseek(fp, 0, SEEK_END);
    long current_size = ftell(fp);
    fclose(fp);

    long total = downloader->get_real_content_length();
    if (total > 0 && current_size > 0) {
        double progress = static_cast<double>(current_size) / total;
        update_progress(progress);
    }
}


LLMChoice LLMSelectionDialog::get_selected_llm_choice() const
{
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(local_llm_3b_button))) {
        return LLMChoice::Local_3b;
    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(local_llm_7b_button))) {
        return LLMChoice::Local_7b;
    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(remote_llm_button))) {
        return LLMChoice::Remote;
    }
    return LLMChoice::Unset;
}


void LLMSelectionDialog::update_file_size_label() {
    std::string size_str = Utils::format_size(downloader->get_real_content_length());
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
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(local_llm_3b_button)) ||
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(local_llm_7b_button))) {
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