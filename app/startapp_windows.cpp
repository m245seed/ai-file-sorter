#include <iostream>
#include <cstdlib>
#include <shlobj.h>
#include <string>
#include <windows.h>
#include <vector>

#include <gtk/gtk.h>


typedef unsigned int cl_uint;
typedef int cl_int;
typedef void* cl_platform_id;
typedef void* cl_device_id;

#define CL_SUCCESS 0
#define CL_DEVICE_TYPE_ALL 0xFFFFFFFF


std::wstring utf8ToUtf16(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size_needed);
    return wstr;
}


bool isCudaAvailable() {
    for (int i = 9; i <= 20; ++i) {
        std::string dllName = "cudart64_" + std::to_string(i) + ".dll";
        HMODULE lib = LoadLibraryA(dllName.c_str());
        if (lib) {
            FreeLibrary(lib);
            return true;
        }
    }
    return false;
}


bool isNvidiaDriverAvailable() {
    const char* dlls[] = {
        "nvml.dll",
        "nvcuda.dll",
        "nvapi64.dll"
    };

    for (const auto& dll : dlls) {
        HMODULE lib = LoadLibraryA(dll);
        if (lib) {
            FreeLibrary(lib);
            return true;
        }
    }
    return false;
}


std::string getExecutableDirectory() {
    WCHAR buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    std::wstring wpath(buffer);
    size_t pos = wpath.find_last_of(L"\\/");
    std::wstring dir = wpath.substr(0, pos);
    return std::string(dir.begin(), dir.end());
}


void addToPath(const std::string& directory) {
    char* pathEnv = nullptr;
    size_t requiredSize = 0;

    getenv_s(&requiredSize, nullptr, 0, "PATH");
    if (requiredSize == 0) {
        std::cerr << "Failed to retrieve PATH environment variable." << std::endl;
        return;
    }

    pathEnv = new char[requiredSize];
    getenv_s(&requiredSize, pathEnv, requiredSize, "PATH");

    std::string newPath = std::string(pathEnv) + ";" + directory;
    delete[] pathEnv;

    // Update PATH in the current process
    if (_putenv_s("PATH", newPath.c_str()) != 0) {
        std::cerr << "Failed to set PATH environment variable." << std::endl;
    } else {
        std::cout << "Updated PATH: " << newPath << std::endl;
    }
}


void showCudaDownloadDialog(GtkWindow* parent = nullptr) {
    GtkWidget* dialog = gtk_message_dialog_new(
        parent,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_WARNING,
        GTK_BUTTONS_NONE,
        nullptr
    );

    gtk_window_set_title(GTK_WINDOW(dialog), "CUDA Toolkit Missing");

    // Center the dialog on the screen (even if parent is null)
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);

    // Set formatted primary and secondary text
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
        "A compatible NVIDIA GPU was detected, but the CUDA Toolkit is missing.\n\n"
        "CUDA is required for GPU acceleration in this application.\n\n"
        "Would you like to download and install it now?"
    );

    gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog),
        "<b>CUDA Toolkit Not Found</b>"
    );

    // Add "Download" and "Ignore" buttons
    gtk_dialog_add_button(GTK_DIALOG(dialog), "_Ignore (not recommended)", GTK_RESPONSE_CANCEL);
    gtk_dialog_add_button(GTK_DIALOG(dialog), "_Download CUDA Toolkit", GTK_RESPONSE_OK);

    // Set default response
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

    // Run the dialog and capture response
    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if (result == GTK_RESPONSE_OK) {
        ShellExecuteA(nullptr, "open", "https://developer.nvidia.com/cuda-downloads", nullptr, nullptr, SW_SHOWNORMAL);
        exit(EXIT_SUCCESS);
    }
}


void launchMainApp() {
    std::string exePath = "AI File Sorter.exe";
    if (WinExec(exePath.c_str(), SW_SHOW) < 32) {
        std::cerr << "Failed to launch the application." << std::endl;
    }
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    std::string exeDir = getExecutableDirectory();

    if (!SetCurrentDirectoryA(exeDir.c_str())) {
        std::cerr << "Failed to set current directory: " << exeDir << std::endl;
        return EXIT_FAILURE;
    }

    std::string dllPath = exeDir + "\\lib";
    addToPath(dllPath);

    bool hasCuda = isCudaAvailable();
    bool hasNvidiaDriver = isNvidiaDriverAvailable();

    if (hasNvidiaDriver && !hasCuda) {
        gtk_init(nullptr, nullptr);
        showCudaDownloadDialog();
    }

    std::string folderName;
    folderName += hasCuda && hasNvidiaDriver ? "wcuda" : "wocuda";

    std::string ggmlPath = exeDir + "\\lib\\ggml\\" + folderName;
    addToPath(ggmlPath);
    AddDllDirectory(utf8ToUtf16(ggmlPath).c_str());

    launchMainApp();
    return EXIT_SUCCESS;
}
