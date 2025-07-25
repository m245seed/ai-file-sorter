#include <iostream>
#include <cstdlib>
#include <shlobj.h>
#include <string>
#include <windows.h>
#include <vector>


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


bool isOpenCLAvailable() {
    HMODULE opencl = LoadLibraryA("OpenCL.dll");
    if (opencl) {
        FreeLibrary(opencl);
        return true;
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
    bool hasOpenCL = isOpenCLAvailable();

    std::string folderName;
    folderName += hasCuda ? "wcuda" : "wocuda";
    folderName += hasOpenCL ? "wopencl" : "woopencl";

    std::string ggmlPath = exeDir + "\\lib\\ggml\\" + folderName;
    addToPath(ggmlPath);
    AddDllDirectory(utf8ToUtf16(ggmlPath).c_str());

    launchMainApp();
    return EXIT_SUCCESS;
}