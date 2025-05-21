#include <gtk/gtk.h>
#include <string>


class DialogUtils {
public:
    static void show_error_dialog(GtkWindow *parent, const std::string &message);
};