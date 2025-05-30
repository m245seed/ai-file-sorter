#pragma once
#include "LLMDownloader.hpp"
#include "Types.hpp"
#include <gtk/gtk.h>
#include <atomic>
#include <mutex>
#include <thread>


class LLMSelectionDialog {
public:
    static void on_llm_radio_toggled(GtkWidget *widget, gpointer data);
    LLMSelectionDialog(Settings& settings);
    ~LLMSelectionDialog();
    LLMChoice get_selected_llm_choice() const;
    int run();
    GtkWidget* get_widget(); 

private:
    GtkWidget *dialog;
    GtkWidget *main_box;
    GtkWidget *title_label;
    GtkWidget *remote_url_label;
    GtkWidget *local_path_label;
    GtkWidget *file_size_label;
    GtkWidget *remote_llm_button;
    GtkWidget *local_llm_button;
    GtkWidget *download_button;
    GtkWidget *progress_bar;
    GtkWidget *ok_btn;
    GtkWidget* file_info_box;
    GtkWidget* details_frame;

    std::unique_ptr<LLMDownloader> downloader;
    std::thread download_thread;
    std::atomic<bool> is_downloading{false};
    std::mutex download_mutex;

    void on_selection_changed(GtkWidget *widget, gpointer data);
    void on_download_button_clicked(GtkWidget *widget, gpointer data);
    void update_progress(double fraction);
    void on_download_complete();
    void update_progress_text(const std::string &text);
    void update_file_size_label();
    void init_progress_bar();
    
    Settings& settings;
    LLMChoice selected_choice;
};