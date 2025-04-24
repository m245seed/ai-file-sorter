#pragma once
#include <gtk/gtk.h>
#include <atomic>
#include <mutex>
#include <thread>

class LLMSelectionDialog {
public:
    static void on_llm_radio_toggled(GtkWidget *widget, gpointer data);
    LLMSelectionDialog();
    ~LLMSelectionDialog();
    int run();
    GtkWidget* get_widget(); 

private:
    GtkWidget *dialog;
    GtkWidget *main_box;
    GtkWidget *title_label;
    GtkWidget *remote_llm_button;
    GtkWidget *local_llm_button;
    GtkWidget *download_button;
    GtkWidget *progress_bar;
    GtkWidget *continue_button;

    std::unique_ptr<LLMDownloader> downloader;
    std::thread download_thread;
    std::atomic<bool> is_downloading{false};
    std::mutex download_mutex;

    void on_selection_changed(GtkWidget *widget, gpointer data);
    void on_download_button_clicked(GtkWidget *widget, gpointer data);
    void update_progress();
    void on_download_complete();
};